#include "p2engine/utilities.hpp"
#include "p2engine/safe_buffer_io.hpp"
#include "p2engine/trafic_statistics.hpp"
#include "p2engine/socket_utility.hpp"
#include "p2engine/rdp/basic_shared_tcp_layer.hpp"
#include "p2engine/rdp/trdp_flow.hpp"
#include "p2engine/rdp/const_define.hpp"

NAMESPACE_BEGIN(p2engine);
NAMESPACE_BEGIN(trdp);

DEBUG_SCOPE(

	static atomic<int32_t>& counter()
{
	static atomic<int32_t> counter_(0);
	BOOST_ASSERT(counter_>=0);
	return counter_;
}
);

inline int64_t NOW(){
	return system_time::tick_count();
}

trdp_flow::trdp_flow(io_service& ios, bool realTimeUtility, bool passiveMode)
	:basic_engine_object(ios)
	,socket_impl_(ios)
	,is_passive_(passiveMode)
	,is_recv_blocked_(true)
	,is_realtime_utility_(realTimeUtility)
	,resolver_(ios)
	,sending_(false)
	,remote_to_local_lost_rate_(-1)
	,local_to_remote_lost_rate_(-1)
	,id_for_lost_rate_(0)
	,in_speed_meter_(milliseconds(2000))
	,out_speed_meter_(milliseconds(2000))
{
	__init();
	DEBUG_SCOPE(
		counter()++;
	//std::cout<<"trdp_flow~~~~~~~~~~~~~~~~~~~~~~trdp_flow_count:"<<counter()<<std::endl;
	);
}

trdp_flow::~trdp_flow()
{
	__close(true);

	DEBUG_SCOPE(
		counter()--;
	);
}

void trdp_flow::__init()
{
	set_cancel();
	next_op_stamp();

	srtt_=0;
	flowid_=random<uint32_t>(0,INVALID_FLOWID);
	connection_=NULL;
	sending_=false;
	qued_sending_size_=0;
	while(!send_bufs_.empty())
		send_bufs_.pop();
	//can_send_=false;
	recv_state_=RECVED;
	state_=(is_passive_?OPENED:INIT);
	ping_interval_=seconds(15);
	if (conn_timer_)
	{
		conn_timer_->cancel();
	}
	else
	{
		conn_timer_=timer_type::create(get_io_service());
		conn_timer_->set_obj_desc("trdp_flow::conn_timer_");
	}
	if (ping_timer_)
	{
		ping_timer_->cancel();
	}
	else
	{
		ping_timer_=timer_type::create(get_io_service());
		ping_timer_->set_obj_desc("trdp_flow::ping_timer_");
		ping_timer_->time_signal().bind(&this_type::__do_ping,this);//bind signal
	}
}

double trdp_flow::__remote_to_local_lost_rate(int8_t* id)const
{
	if (state_!=CONNECTED)
		return 1.0;

	msec_type now=NOW();
	bool calc=false;
	double  old_remote_to_local_lost_rate=remote_to_local_lost_rate_;
	wrappable_greater_equal<seqno_mark_type::int_type> MARK_GREATER_EQUAL;
	wrappable_minus<seqno_mark_type::int_type> MARK_MINUS;
	seqno_mark_map& qm=recvd_seqno_mark_for_lost_rate_;
	while(!qm.empty()
		&&(
		qm.size()>=(std::size_t)(seqno_mark_type::max_distance()-1)
		||qm.begin()->second+3000<now//3000ms
		||((int)qm.size()>1&&MARK_MINUS(qm.rbegin()->first,qm.begin()->first)<=0)
		||(id&&MARK_MINUS(*id,qm.begin()->first)<0)//mark_seq is ordered,for reason of TCP
		||(id&&MARK_GREATER_EQUAL(qm.begin()->first,*id)&&MARK_GREATER_EQUAL(*id,qm.rbegin()->first))
		)
		)//wrappable_integer<int8_t> The phenomenon: A>B B>C but A<C may appear here, we should avoid it.
	{
		qm.erase(qm.begin());
		calc=true;
	}
	if (id)
	{
		qm.insert(std::make_pair(*id,now));
		calc=true;
	}
	if (calc)
	{
		if (!qm.empty())
		{
			BOOST_ASSERT(MARK_MINUS(qm.rbegin()->first,qm.begin()->first)>=0);
			double cnt=MARK_MINUS(qm.rbegin()->first,qm.begin()->first)+1;
			if (cnt>=qm.size())
			{
				double lostRate=1.0-(double)(qm.size())/cnt;

				BOOST_ASSERT(lostRate<=1.0&&lostRate>=0.0);
				if (remote_to_local_lost_rate_<0)
					remote_to_local_lost_rate_=lostRate;
				else
					remote_to_local_lost_rate_=remote_to_local_lost_rate_*0.125+lostRate*0.875;
			}
			else 
			{
				BOOST_ASSERT(0);//bug!!
				if(remote_to_local_lost_rate_<0)
					remote_to_local_lost_rate_=0;
			}
		}
		else
		{
			remote_to_local_lost_rate_=0;
		}
	}

	if (remote_to_local_lost_rate_<0)
		remote_to_local_lost_rate_=0.0;
	if (remote_to_local_lost_rate_>1.0)
		remote_to_local_lost_rate_=1.0;

	if (calc||old_remote_to_local_lost_rate!=remote_to_local_lost_rate_)
	{
		double a=std::max(in_speed_meter_.bytes_per_second(),200.0)/(global_remote_to_local_speed_meter().bytes_per_second()+1.0);
		if (a>1.0) a=1.0;
		if (a<0.0) a=0.0;
		double oldRate=global_remote_to_local_lost_rate();
		double newRate=(oldRate*(1.0-a)+remote_to_local_lost_rate_*a);
		set_global_remote_to_local_lost_rate(oldRate*0.125+newRate*0.875);
	}

	return remote_to_local_lost_rate_;
}

double trdp_flow::__local_to_remote_lost_rate(int8_t* id)const
{
	if (state_!=CONNECTED)
		return 1.0;

	wrappable_greater_equal<seqno_mark_type::int_type> MARK_GREATER_EQUAL;
	wrappable_minus<seqno_mark_type::int_type> MARK_MINUS;
	seqno_mark_map& qm=send_seqno_mark_for_lost_rate_;
	msec_type now=NOW();
	while(!qm.empty()
		&&(qm.size()>=(std::size_t)(seqno_mark_type::max_distance()-1)
		||((int)qm.size()>1&&MARK_MINUS(qm.rbegin()->first,qm.begin()->first)<=0)
		||(id&&MARK_MINUS(*id,qm.begin()->first)<0)//mark_seq is ordered,for reason of TCP
		||(!id&&MARK_MINUS(id_for_lost_rate_,qm.begin()->first)<0)//mark_seq is ordered,for reason of TCP
		||qm.begin()->second+3000<now//3000ms
		||(id&&MARK_GREATER_EQUAL(qm.begin()->first,*id)&&MARK_GREATER_EQUAL(*id,qm.rbegin()->first))
		)
		)//wrappable_integer<int8_t> The phenomenon: A>B B>C but A<C may appear here, we should avoid it.
	{
		qm.erase(qm.begin());
	}
	if (id)
		qm.insert(std::make_pair(*id,now));
	if (!qm.empty())
	{
		double cnt=(id? MARK_MINUS(qm.rbegin()->first,qm.begin()->first)+1
			:MARK_MINUS(id_for_lost_rate_,qm.begin()->first)+1);
		BOOST_ASSERT(cnt>=1.0);
		if (cnt>=qm.size())
		{
			double lostRate=((cnt==qm.size())?0:(1.0-(double)(qm.size())/cnt));
			BOOST_ASSERT(lostRate<=1.0&&lostRate>=0.0);
			if (local_to_remote_lost_rate_<0)
				local_to_remote_lost_rate_=lostRate;
			else
				local_to_remote_lost_rate_=local_to_remote_lost_rate_*0.125+lostRate*0.875;
		}
		else 
		{
			BOOST_ASSERT(0);//bug!!!
			if(local_to_remote_lost_rate_<0)
				local_to_remote_lost_rate_=0;
		}
	}
	else
	{
		local_to_remote_lost_rate_=0;
	}
	if (local_to_remote_lost_rate_<0)
		local_to_remote_lost_rate_=0.0;
	if (local_to_remote_lost_rate_>1.0)
		local_to_remote_lost_rate_=1.0;

	double a=std::max(out_speed_meter_.bytes_per_second(),200.0)/(global_local_to_remote_speed_meter().bytes_per_second()+1.0);
	if (a>1.0) a=1.0;
	BOOST_ASSERT(a>=0);
	double oldRate=global_local_to_remote_lost_rate();
	double newRate=(oldRate*(1.0-a)+local_to_remote_lost_rate_*a);
	set_global_local_to_remote_lost_rate(oldRate*0.125+newRate*0.875);

	return local_to_remote_lost_rate_;
}

//called by shared_tcp_layer when accepted a socket. so, the socket 
//can receive the domain that the remote endpoint requested.
void trdp_flow::waiting_domain(shared_layer_sptr shared_layer)
{
	shared_layer_=shared_layer;

	//start time out timer
	conn_timer_->time_signal().clear();
	conn_timer_->time_signal().bind(&this_type::__do_close, this);
	conn_timer_->async_wait(seconds(5));

	__waiting_domain_coro(error_code(),0,op_stamp());
}

void trdp_flow::__waiting_domain_coro(const error_code& ec, std::size_t len, 
									  op_stamp_t stamp, coroutine coro)
{
	if(is_canceled_op(stamp))//all canceled operation will not be invoked
		return;
	//async read the length
	CORO_REENTER(coro)
	{
		CORO_YIELD(
			asio::async_read(socket_impl_, 
			asio::buffer(recv_header_buf_,2),asio::transfer_at_least(2),
			make_alloc_handler(
			boost::bind(&this_type::__waiting_domain_coro,
			SHARED_OBJ_FROM_THIS,_1,_2,op_stamp(),coro)
			)
			)
			);
		//now, we have read the length of domain, check and process
		if (ec||len!=2)
		{
			close();
			return;
		}
		else
		{
			char* p=&recv_header_buf_[0];
			int pktLen=read_uint16_ntoh(p);
			if (pktLen<2)
			{
				close();
				return;
			}
			recv_buf_.recreate(pktLen);
		}

		//yield async read the domain data
		CORO_YIELD(
			asio::async_read(socket_impl_,
			asio::buffer(buffer_cast<char*>(recv_buf_),recv_buf_.length()),
			asio::transfer_at_least(recv_buf_.length()),
			make_alloc_handler(
			boost::bind(&this_type::__waiting_domain_coro,
			SHARED_OBJ_FROM_THIS,_1,_2,op_stamp(),coro)
			)
			)
			);
		//now, we have read the domain
		if (ec||len!=recv_buf_.length())
		{
			close();
			return;
		}
		else
		{
			safe_buffer_io io(&recv_buf_);
			char pktType;
			int8_t id;
			io>>id>>pktType;

			in_speed_meter_+=recv_buf_.length()+2;
			global_remote_to_local_speed_meter()+=recv_buf_.length()+2;
			__remote_to_local_lost_rate(&id);

			if (pktType!=(char)CONN_PKT)
			{
				close();
				return;
			}
			shared_layer_sptr sharedLayer=shared_layer_.lock();
			if (sharedLayer)
			{
				acceptor_type* acc=sharedLayer->find_acceptor(
					std::string(buffer_cast<char*>(recv_buf_),recv_buf_.length())
					);
				if (acc)
				{
					accept(error_code());//,SHARED_OBJ_FROM_THIS
					acc->accept_flow(SHARED_OBJ_FROM_THIS);
				}
				else
					accept(asio::error::connection_refused);
			}
		}
	}
}

void trdp_flow::accept(const error_code& ec)
{
	conn_timer_->cancel();
	if (state_==CLOSED)
		return;
	if (ec)
	{
		close();
		return;
	}
	else
	{
		//the first two bytes is Packet Length and the next two bytes is 
		//fingerprint.
		safe_buffer buffer;
		safe_buffer_io io(&buffer);
		io<<uint16_t(2+0)<<(uint8_t)id_for_lost_rate_<<(char)ACCEPT_PKT;
		state_=CONNECTED;

		BOOST_ASSERT(sending_==false&&send_bufs_.empty());
		//sending_=true;
		asio::async_write(socket_impl_,
			buffer.to_asio_const_buffers_1(),
			make_alloc_handler(boost::bind(&this_type::__handle_sent_packet,
			SHARED_OBJ_FROM_THIS,_1,_2,false,
			op_cancel()))//using op_cancel()(NOT op_stamp()) to prevent call back sendout_signal
			);
	}
}

void trdp_flow::connect(const std::string& remote_host, int port, 
						const std::string& domainName)
{
	BOOST_ASSERT(!is_passive_);
	error_code err;
	address_v4 addr=address_v4::from_string(remote_host.c_str(),err);
	if (!err)//if the format of xxx.xxx.xxx.xxx
	{
		remote_edp_=tcp::endpoint(addr,(unsigned short)port);
		connect(remote_edp_,domainName);
	}
	else
	{
		remote_host_=remote_host;
		remote_edp_.port((unsigned short)port);
		connect(endpoint_type(),domainName);
	}
}

void trdp_flow::connect(const endpoint_type& remote_edp, const std::string& domainName)
{
	error_code ec;
	if (state_==INIT||state_==CLOSED)
		open(ec);
	else if (state_!=OPENED)
		ec=asio::error::in_progress;

	next_op_stamp();

	if (ec)
	{
		get_io_service().post(
			make_alloc_handler(boost::bind(&this_type::__allert_connected,
			SHARED_OBJ_FROM_THIS,ec,op_stamp()))
			);
		return;
	}

	state_=CONNECTING;

	//make the domain request string
	char domainLenBuf[2];
	char* p=domainLenBuf;
	write_uint16_hton(uint16_t(2+domainName.length()), p);
	domain_=std::string(domainLenBuf,domainLenBuf+2)
		+std::string(1,(char)id_for_lost_rate_)
		+std::string(1,(char)CONN_PKT)
		+domainName;

	if (remote_edp.port())//we know the remote_endpoint
	{
		remote_edp_=remote_edp;
		__async_resolve_connect_coro(error_code(),resolver_iterator(),op_stamp());
	}
	else//we know the remote_host name
	{
		//BOOST_ASSERT(remote_host_.length()>0);
		resolver_query qy(remote_host_,boost::lexical_cast<std::string>(remote_edp_.port()));
		__async_resolve_connect_coro(error_code(),resolver_iterator(),op_stamp(),coroutine(),&qy);
	}
}

void trdp_flow::__close(bool greacful)
{
	set_cancel();
	while(!send_bufs_.empty())
	{
		send_bufs_.pop();
	}

	if (conn_timer_)
	{
		conn_timer_->cancel();
		conn_timer_.reset();
	}
	if (ping_timer_)
	{
		ping_timer_->cancel();
		ping_timer_.reset();
	}
	if (state_==CLOSED)
		return;
	if(socket_impl_.is_open())
	{
		error_code ec;
		if (greacful)
			socket_impl_.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
		socket_impl_.close(ec);
	}
	state_=CLOSED;
	connection_=NULL;
}

error_code trdp_flow::ping(error_code& ec)
{
	if (state_!=CONNECTED)
	{
		ec=asio::error::not_connected;
		return ec;
	}
	ec.clear();
	if (is_open())
	{
		safe_buffer buf;
		safe_buffer_io io(&buf);
		io<<(int64_t)NOW();
		__send(buf,(char)PING_PKT,false);
	}
	return ec;
}

void trdp_flow::ping_interval(const time_duration& t)
{
	BOOST_ASSERT(ping_timer_&&!ping_timer_->time_signal().empty());
	if (t!=ping_interval_)
	{
		ping_interval_=t;
		if (ping_timer_)
			ping_timer_->async_keep_waiting(seconds(0),ping_interval_);
	}
}

void trdp_flow::keep_async_receiving()
{
	if (is_recv_blocked_&&recv_state_!=RECVING)
	{
		is_recv_blocked_=false;
		do_keep_receiving(error_code(),recv_buf_.size(),op_stamp());
	}
}

void trdp_flow::do_keep_receiving(error_code ec, std::size_t len, 
								  op_stamp_t stamp, coroutine coro)
{
	//all canceled operation will not be invoked
	if(is_canceled_op(stamp)||state_==CLOSED)
		return;

	uint16_t packetLen;
	char* p=&recv_header_buf_[0];
	CORO_REENTER(coro)
	{
		for(;;)
		{
			//in case of chaos, do not check "is_recv_blocked_" 
			//while reading length and data, yield async receive len
			recv_state_=RECVING;
			CORO_YIELD(asio::async_read(socket_impl_,
				asio::buffer(&recv_header_buf_[0],2),
				asio::transfer_all(),
				make_alloc_handler(boost::bind(&this_type::do_keep_receiving,
				SHARED_OBJ_FROM_THIS,_1,_2,stamp,coro)))
				);
			if (ec)
			{
				__to_close_state(ec,stamp);
				return;
			}
			p=&recv_header_buf_[0];
			packetLen=read_uint16_ntoh(p);
			if (packetLen==0)
			{
				recv_state_=RECVED;
				//check "is_recv_blocked_" before reading the next packet
				if (is_recv_blocked_)
					return;
				continue;
			}

			recv_buf_.recreate(packetLen);
			//yield async receive data
			CORO_YIELD(asio::async_read(socket_impl_,
				asio::buffer(buffer_cast<char*>(recv_buf_),packetLen),
				asio::transfer_at_least(packetLen),
				make_alloc_handler(boost::bind(&this_type::do_keep_receiving,
				SHARED_OBJ_FROM_THIS,_1,_2,stamp,coro)))
				);
			recv_state_=RECVED;
			if (ec)
			{
				__to_close_state(ec,stamp);
				return;
			}

			//read a packet successfully. check "is_recv_blocked_" before processing it
			//if is_recv_blocked_ is false, process the packet
			if (is_recv_blocked_)
				return;
			if (!__process_data(stamp))
				return;

			//check "is_recv_blocked_" before reading the next packet
			if (is_recv_blocked_)
				return;

		}
	}
}

void trdp_flow::__async_resolve_connect_coro(error_code err, 
											 resolver_iterator itr, 
											 op_stamp_t stamp, 
											 coroutine coro, 
											 resolver_query* qy)
{
	if(is_canceled_op(stamp)||state_==CLOSED)//all canceled operation will not be invoked
		return;
	CORO_REENTER(coro)
	{
		// Start the conn timer
		{
			error_code timeoutErr=asio::error::timed_out;
			conn_timer_->time_signal().clear();
			conn_timer_->time_signal().bind(&this_type::__to_close_state,this,timeoutErr,stamp);
			conn_timer_->async_wait(seconds(10));
		}
		//if qy is not null, we yield async resolve
		if (qy)
		{
			CORO_YIELD(resolver_.async_resolve(*qy,
				make_alloc_handler(boost::bind(&this_type::__async_resolve_connect_coro,
				SHARED_OBJ_FROM_THIS,_1,_2,stamp,coro,qy))) 
				);

			if (err||itr==resolver_iterator())
			{
				__to_close_state(err,stamp);
				return;
			}
			remote_edp_.address(((resolver_type::endpoint_type)(*itr++)).address());
		}

		//yield async connect
		CORO_YIELD(socket_impl_.async_connect(remote_edp_,
			make_alloc_handler(boost::bind(&this_type::__async_resolve_connect_coro,
			SHARED_OBJ_FROM_THIS,_1,itr,stamp,coro,(resolver_query*)NULL)
			))
			);

		while(err && itr != resolver_iterator())
		{
			//close socket that failed to connect, and open a new one
			socket_impl_.close(err);
			__open(err);
			if (err)
			{
				__to_close_state(err,stamp);
				return;
			}

			// retart the conn timer
			{
				error_code timeoutErr=asio::error::timed_out;
				conn_timer_->time_signal().clear();
				conn_timer_->time_signal().bind(&this_type::__to_close_state,
					this,timeoutErr,stamp);
				conn_timer_->async_wait(seconds(6));
			}

			remote_edp_.address(resolver_type::endpoint_type(*itr++).address());
			//yeld async connect
			CORO_YIELD(socket_impl_.async_connect(remote_edp_,
				make_alloc_handler(boost::bind(&this_type::__async_resolve_connect_coro,
				SHARED_OBJ_FROM_THIS,_1,itr,stamp,coro,(resolver_query*)NULL)))
				);
		}
		if (err)
		{
			__to_close_state(err,stamp);
			return;
		}

		//now lowlayer connected
		conn_timer_->cancel();
		conn_timer_->time_signal().clear();
		__domain_request_coro(error_code(),0,stamp);
	}
}

void trdp_flow::__domain_request_coro(error_code ec, std::size_t len, 
									  op_stamp_t stamp, coroutine coro)
{
	//all canceled operation will not be invoked
	if(is_canceled_op(stamp)||state_==CLOSED)
		return;
	CORO_REENTER(coro)
	{
		//yield send request
		ec=asio::error::timed_out;
		conn_timer_->time_signal().clear();
		conn_timer_->time_signal().bind(&this_type::__to_close_state,this,ec,stamp);
		conn_timer_->async_wait(seconds(6));
		CORO_YIELD(asio::async_write(socket_impl_,
			asio::buffer(domain_.c_str(),domain_.length()),
			asio::transfer_all(),
			make_alloc_handler(boost::bind(&this_type::__domain_request_coro,
			SHARED_OBJ_FROM_THIS,_1,_2,stamp,coro)))
			);
		if (ec)
		{
			__to_close_state(ec,stamp);
			return;
		}

		//yield async receive domain reply
		CORO_YIELD(asio::async_read(socket_impl_,
			asio::buffer(recv_header_buf_,4),
			asio::transfer_all(),
			make_alloc_handler(boost::bind(&this_type::__domain_request_coro,
			SHARED_OBJ_FROM_THIS,_1,_2,stamp,coro)))
			);
		if (ec)
		{
			__to_close_state(ec,stamp);
			return;
		}
		else if (len!=4||recv_header_buf_[3]!=(char)(char)ACCEPT_PKT)
		{
			ec=asio::error::connection_refused;
			__to_close_state(ec,stamp);
			return;
		}
		state_=CONNECTED;
		conn_timer_->time_signal().clear();
		conn_timer_->cancel();
		if (connection_)
		{
			keep_async_receiving();
			connection_->on_connected(error_code());
		}
	}
}

bool trdp_flow::__process_data(op_stamp_t stamp)
{
	//all canceled operation will not be invoked
	if(is_canceled_op(stamp)||state_==CLOSED)
		return false;

	if (recv_buf_.length()<2)
	{
		error_code ec=asio::error::fault;
		__to_close_state(ec,stamp);
		return false;
	}

	char pktType;
	int8_t id;
	safe_buffer_io io(&recv_buf_);
	io>>id;
	io>>pktType;

	in_speed_meter_+=recv_buf_.length()+2;
	global_remote_to_local_speed_meter()+=recv_buf_.length()+2;
	__remote_to_local_lost_rate(&id);

	switch(pktType)
	{
	case (char)PING_PKT:
		{
			if (recv_buf_.length()!=sizeof(int64_t))
			{
				error_code ec=asio::error::fault;
				__to_close_state(ec,stamp);
				return false;
			}
			safe_buffer pingReply;
			safe_buffer_io io(&pingReply);
			io.write(buffer_cast<char*>(recv_buf_),p2engine::buffer_size(recv_buf_));
			__send(pingReply,(char)PONG_PKT,false);
		}
		return true;
	case (char)PONG_PKT:
		{
			if (recv_buf_.length()!=sizeof(int64_t))
			{
				error_code ec=asio::error::fault;
				__to_close_state(ec,stamp);
				return false;
			}
			int64_t t;
			io>>t;
			__update_rtt(t);
		}
		return true;
	case (char)DATA_PKT:
		if (recv_buf_.length()>0)
		{
			safe_buffer buf=recv_buf_.buffer_ref(0);
			if (connection_)
				connection_->on_received(buf);
			else
				return false;
		}
		return true;
	case (char)CONN_PKT:
	case (char)ACCEPT_PKT://accept pkt is processed by __handle_recvd_domain_reply
	default:
		return false;
	}
}

error_code trdp_flow::__open(error_code& ec)
{
	__init();
	if (state_!=INIT)
	{
		ec=asio::error::already_open;
		return ec;
	}
	is_passive_=false;
	socket_impl_.open(asio::ip::tcp::endpoint(local_edp_).protocol(), ec);
	if (!ec)
	{
		if (local_edp_!=endpoint_type())
			socket_impl_.bind(asio::ip::tcp::endpoint(local_edp_),ec);
	}
	if (ec)
	{
		error_code err;
		socket_impl_.close(err);
		return ec;
	}
	if (!ec)
	{
		//asio::socket_base::reuse_address reuse_address_option(false);
		//socket_impl_.set_option(reuse_address_option);
		if (is_realtime_utility_)
		{
			error_code e;
			//using a small buffer
			asio::socket_base::receive_buffer_size receive_buffer_size_option(64*1024);
			asio::socket_base::send_buffer_size send_buffer_size_option(4*1024);
			asio::socket_base::send_low_watermark send_low_watermark_option(4);
			asio::socket_base::receive_low_watermark receive_low_watermark_option(4);
			type_of_service type_of_service_option(0x20);

			socket_impl_.set_option(receive_buffer_size_option,e);
			socket_impl_.set_option(send_buffer_size_option,e);
			socket_impl_.set_option(send_low_watermark_option,e);
			socket_impl_.set_option(receive_low_watermark_option,e);
			socket_impl_.set_option(type_of_service_option,e);
		}
		else
		{
			error_code e;
			//using a larg buffer
			asio::socket_base::receive_buffer_size receive_buffer_size_option(1024*1024);
			asio::socket_base::send_buffer_size send_buffer_size_option(512*1024);
			//asio::socket_base::send_low_watermark send_low_watermark_option(512);
			type_of_service type_of_service_option(0x20);

			socket_impl_.set_option(receive_buffer_size_option, e);
			socket_impl_.set_option(send_buffer_size_option, e);
			//socket_impl_.set_option(send_low_watermark_option);
			socket_impl_.set_option(type_of_service_option,e);
		}
		state_=OPENED;
	}
	return ec;
}

void trdp_flow::__send(const safe_buffer& buffer, char type, bool alertWritable,
					   uint16_t* msgType, bool fromQue)
{
	if (state_!=CONNECTED)
	{
		error_code ec=asio::error::not_connected;
		get_io_service().post(make_alloc_handler(
			boost::bind(&this_type::__to_close_state,SHARED_OBJ_FROM_THIS,ec,op_stamp())
			));
		return;
	}

	//if is sending, push this packet into sendque
	if ((!send_bufs_.empty()&&!fromQue)||sending_)
	{
		const msec_type MAX_TIME=(std::numeric_limits<msec_type>::max)();
		msec_type now=NOW();
		if (!send_bufs_.empty()
			&&send_bufs_.front().outTime!=MAX_TIME
			&&send_bufs_.front().outTime+10000<now
			)
		{
			error_code ec=asio::error::connection_aborted;
			get_io_service().post(make_alloc_handler(
				boost::bind(&this_type::__to_close_state,SHARED_OBJ_FROM_THIS,ec,op_stamp())
				));
			return;
		}

		send_emlment elm;
		elm.buf=buffer;
		elm.alertWritable=alertWritable;
		elm.msgType=(msgType?(*msgType):(INVALID_MSGTYPE));
		if (alertWritable)
			elm.outTime=MAX_TIME;
		else
			elm.outTime=now+250;//un-reliable packets, will be discard if they could not be sent with 250ms
		send_bufs_.push(elm);

		if (send_bufs_.size()>4096)
		{

		}

		return;
	}

	++id_for_lost_rate_;

	safe_buffer len;
	safe_buffer_io io(&len);
	io<<uint16_t(2+buffer.length()+(msgType?2:0));
	io<<(char)id_for_lost_rate_;
	io<<(char)type;
	if (msgType)
		io<<uint16_t(*msgType);

	out_speed_meter_+=len.length()+buffer.length();
	global_local_to_remote_speed_meter()+=len.length()+buffer.length();
	__local_to_remote_lost_rate(&id_for_lost_rate_);

	if(buffer.length()>0)
	{
		boost::array<p2engine::asio_const_buffers_1,2> vec=
		{
			{
				len.to_asio_const_buffers_1(),
					buffer.to_asio_const_buffers_1()
			}
		};
		asio::async_write(socket_impl_,vec,
			asio::transfer_all(),
			make_alloc_handler(boost::bind(&this_type::__handle_sent_packet,
			SHARED_OBJ_FROM_THIS,_1,_2,alertWritable,op_stamp()))
			);
	}
	else
	{
		asio::async_write(socket_impl_,len.to_asio_const_buffers_1(),
			asio::transfer_all(),
			make_alloc_handler(boost::bind(&this_type::__handle_sent_packet,
			SHARED_OBJ_FROM_THIS,_1,_2,alertWritable,op_stamp()))
			);
	}

	sending_=true;
}

void trdp_flow::__handle_sent_packet(const error_code& ec, std::size_t bytes_trans,
									 bool allertWritable, op_stamp_t stamp)
{
	sending_=false;

	//all canceled operation will not be invoked
	if(is_canceled_op(stamp)||state_==CLOSED)
		return;

	if (ec)
	{
		if (connection_)
			connection_->on_disconnected(ec);
		return;
	}

	if (connection_&&allertWritable)
		connection_->on_writeable();
	if(is_canceled_op(stamp)||state_==CLOSED)
		return;

	int64_t now=NOW();
	while (!sending_&&!send_bufs_.empty())
	{
		send_emlment elm=send_bufs_.front();
		send_bufs_.pop();
		if (elm.outTime>=now)
		{
			uint16_t*  pMsgType=((elm.msgType==INVALID_MSGTYPE)?NULL:&elm.msgType);
			__send(elm.buf,DATA_PKT,elm.alertWritable,pMsgType,true);
		}
		else
		{//not sent because of timeout, remember id++ and calculate local to remote lost rate
			++id_for_lost_rate_;
			local_to_remote_lost_rate();
		}
	}

}

void trdp_flow::__to_close_state(const error_code& ec, op_stamp_t stamp)
{
	if(is_canceled_op(stamp)||state_==CLOSED)//all canceled operation will not be invoked
		return;
	BOOST_ASSERT(ec);
	if (connection_)
	{
		if (is_passive_)
		{
			if (state_==CONNECTED)
				connection_->on_disconnected(ec);
		}
		else
		{
			if (state_<CONNECTED)
				connection_->on_connected(ec);
			else
				connection_->on_disconnected(ec);
		}
	}
	close();
}

void trdp_flow::__allert_connected(error_code ec, op_stamp_t stamp)
{
	if(is_canceled_op(stamp)||state_==CLOSED)//all canceled operation will not be invoked
		return;
	if (connection_)
		connection_->on_connected(ec);
}

void trdp_flow::__update_rtt(int64_t echoTime)
{
	int64_t now=NOW();
	int rtt_msec=(int)(now-echoTime);
	if (rtt_msec>0&&rtt_msec<10000)
	{
		if (srtt_==0)//not __init
		{
			srtt_ = rtt_msec;		// srtt = rtt
			rttvar_ = rtt_msec/2;	// rttvar = rtt / 2
		}
		else
		{
			srtt_ = (7*srtt_ + rtt_msec)/8;
			rttvar_ = (3*rttvar_+(abs(rtt_msec - srtt_)- rttvar_))/4;
		}
		rto_ = srtt_ + 4*rttvar_;
		rto_=(std::max)(6000,rto_);
		rto_=(std::min)(100,rto_);
	}
}


NAMESPACE_END(trdp);
NAMESPACE_END(p2engine);
