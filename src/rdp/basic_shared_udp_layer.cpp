#include "p2engine/packet.hpp"
#include "p2engine/utilities.hpp"
#include "p2engine/safe_buffer_io.hpp"
#include "p2engine/broadcast_socket.hpp"
#include "p2engine/rdp/urdp_visitor.hpp"
#include "p2engine/rdp/basic_shared_udp_layer.hpp"

NAMESPACE_BEGIN(p2engine);
NAMESPACE_BEGIN(urdp);

typedef urdp_packet_basic_format packet_format_type;

void __dummy_callback(const error_code&, size_t){}

basic_shared_udp_layer::this_type_container 
	basic_shared_udp_layer::s_shared_this_type_pool_;
fast_mutex basic_shared_udp_layer::s_shared_this_type_pool_mutex_;
basic_shared_udp_layer::allocator_wrap_handler 
	basic_shared_udp_layer::s_dummy_callback(boost::bind(&__dummy_callback,_1,_2));

basic_shared_udp_layer::flow_token::smart_ptr 
	basic_shared_udp_layer::create_flow_token( io_service& ios,
	const endpoint_type& local_edp,
	void* flow,
	const recvd_data_handler_type& handler,
	error_code& ec
	)
{
	basic_shared_udp_layer::shared_ptr obj=create(ios,local_edp,ec);
	return create_flow_token(obj,flow,handler,ec);
}

//called by flow,when listened a passive connect request
basic_shared_udp_layer::flow_token::smart_ptr 
	basic_shared_udp_layer::create_flow_token(
	shared_layer_sptr udplayer,
	void* flow,//the flow that has been created when received SYN
	const recvd_data_handler_type& handler,
	error_code& ec
	)
{
	BOOST_ASSERT(udplayer);
	uint32_t flowID=INVALID_FLOWID;
	udplayer->register_flow(flow,handler,flowID,ec);
	flow_token::smart_ptr rst(new flow_token(flowID,udplayer,flow));
	return rst;
}

//called by acceptor to listen at a local endpoint
basic_shared_udp_layer::acceptor_token::shared_ptr 
	basic_shared_udp_layer::create_acceptor_token(
	io_service& ios,
	const endpoint_type& local_edp,
	void* acceptor,
	const recvd_request_handler_type& handler,
	const std::string domainName,
	error_code& ec
	)
{
	shared_ptr obj=create(ios,local_edp,ec);
	if (!ec) obj->register_acceptor(acceptor,domainName,handler,ec);
	acceptor_token::shared_ptr rst (new acceptor_token(domainName,obj,acceptor));
	return rst;
}

basic_shared_udp_layer::shared_ptr 
	basic_shared_udp_layer::create(io_service& ios, const
	endpoint_type& local_edp,
	error_code& ec)
{
	bool anyport=(!local_edp.port());
	bool anyaddr=p2engine::is_any(local_edp.address());
	if ((anyport||anyaddr))
	{
		fast_mutex::scoped_lock lock(s_shared_this_type_pool_mutex_);

		this_type_container::iterator itr=s_shared_this_type_pool_.begin();
		for (;itr!=s_shared_this_type_pool_.end();++itr)
		{
			BOOST_ASSERT(itr->second);
			shared_ptr net_obj=itr->second->shared_ptr_from_this();

			bool address_match=(local_edp.address()==net_obj->local_endpoint_.address());
			bool port_match=(local_edp.port()==net_obj->local_endpoint_.port());
			if (((anyport&&address_match)||(anyaddr&&port_match))&&net_obj->is_open())
			{
				return net_obj;
			}
		}
	}
	else
	{
		fast_mutex::scoped_lock lock(s_shared_this_type_pool_mutex_);
		this_type_container::iterator iter = s_shared_this_type_pool_.find(local_edp);
		if (iter != s_shared_this_type_pool_.end()&&iter->second->is_open())
		{
			BOOST_ASSERT(iter->second);
			return iter->second->shared_ptr_from_this();
		}
	}
	shared_ptr net_obj;
	endpoint_type edp(local_edp);
	for (int i=(anyport?16:1);i>0;--i)
	{
		if (anyport)
			edp.port(random<unsigned short>(1024,10000));
		try
		{
			net_obj= shared_ptr(new this_type(ios, edp,ec));
		}
		catch (...)
		{
			LOG(
				LogError("catched exception when create basic_shared_udp_layer");
			);
			continue;
		}
		if (!ec&&net_obj->is_open())
		{
			net_obj->start();
			break;
		}
	}
	BOOST_ASSERT(net_obj);
	if (ec)
	{
		DEBUG_SCOPE(
			std::cout<<"!!!!!!create basic_shared_udp_layer ERROR:"<<ec.message()
			<<",  endpoint:"<<edp<<std::endl;
		BOOST_ASSERT(0);
		);
	}
	return net_obj;
}


basic_shared_udp_layer::basic_shared_udp_layer(io_service& ios, 
	const endpoint_type& local_edp,error_code& ec)
	: basic_engine_object(ios)
	, socket_(ios)
	, id_allocator_(true,64)
	, flows_cnt_(0)
	, state_(INIT)
	, continuous_recv_cnt_(0)
{
	this->set_obj_desc("basic_shared_udp_layer");
	socket_.open(local_edp.protocol(), ec);
	if (ec)
	{
		LOG(
			LogError("unable to open udp socket, error:%d, %s",
			ec.value(),ec.message().c_str());
		);
		socket_.close(ec);
		return;
	}
#if defined IP_MTU_DISCOVER
	/* Linux */
# define IP_OPT_DONT_FRAG IP_MTU_DISCOVER
# define DONT_FRAG_VALUE IP_PMTUDISC_DO
#elif defined IP_DONTFRAG
	/* FreeBSD */
# define IP_OPT_DONT_FRAG IP_DONTFRAG
# define DONT_FRAG_VALUE 1
#elif defined IP_DONTFRAGMENT
	/* Winsock2 */
# define IP_OPT_DONT_FRAG IP_DONTFRAGMENT
# define DONT_FRAG_VALUE 1
#endif
#ifdef IP_OPT_DONT_FRAG
	typedef boost::asio::detail::socket_option::boolean<IPPROTO_IP, IP_OPT_DONT_FRAG> 
		do_not_fragment;
	socket_.set_option(do_not_fragment(!DONT_FRAG_VALUE),ec);
#endif
#ifndef IPTOS_THROUGHPUT
#	define	IPTOS_TOS_MASK		0x1E
#	define	IPTOS_TOS(tos)		((tos)&IPTOS_TOS_MASK)
#	define	IPTOS_LOWDELAY		0x10
#	define	IPTOS_THROUGHPUT	0x08
#	define	IPTOS_RELIABILITY	0x04
#	define	IPTOS_MINCOST		0x02
#	define	IPTOS_PREC_NETCONTROL		0xe0
#	define	IPTOS_PREC_INTERNETCONTROL	0xc0
#	define	IPTOS_PREC_CRITIC_ECP		0xa0
#	define	IPTOS_PREC_FLASHOVERRIDE	0x80
#	define	IPTOS_PREC_FLASH			0x60
#	define	IPTOS_PREC_IMMEDIATE		0x40
#	define	IPTOS_PREC_PRIORITY			0x20
#	define	IPTOS_PREC_ROUTINE			0x00
#endif
	int tos=(IPTOS_RELIABILITY|IPTOS_THROUGHPUT);
	socket_.set_option(type_of_service(tos),ec);
	asio::socket_base::non_blocking_io nonblock_command(true);
	socket_.io_control(nonblock_command,ec);
	socket_.native_non_blocking(true, ec);
	disable_icmp_unreachable(socket_.native());
	socket_.set_option(asio::socket_base::reuse_address(false),ec);
	socket_.set_option(do_not_fragment(false),ec);
	ec.clear();
	socket_.set_option(asio::socket_base::receive_buffer_size(1024*1024),ec);
	if (ec)
		socket_.set_option(asio::socket_base::receive_buffer_size(512*1024),ec);
	ec.clear();
	socket_.set_option(asio::socket_base::send_buffer_size(2*1024*1024),ec);
	if (ec)
		socket_.set_option(asio::socket_base::send_buffer_size(1024*1024),ec);

	ec.clear();
	socket_.bind(local_edp, ec);
	if (ec)
	{
		LOG(
			LogError("unable to bind udp socket with endpoint %s, error %d %s",
			endpoint_to_string(local_edp).c_str(),ec.value(),
			ec.message().c_str());
		);
		error_code err;
		socket_.close(err);
	}
	else
	{
		local_endpoint_=socket_.local_endpoint(ec);
		if (ec)
		{
			LOG(
				LogError("unable get_local_endpoint, error %d %s",
				ec.value(),ec.message().c_str())
				);
			error_code err;
			socket_.close(err);
			local_endpoint_=endpoint_type();
			return;
		}
		else
		{
			fast_mutex::scoped_lock lock(s_shared_this_type_pool_mutex_);
			s_shared_this_type_pool_.insert(std::make_pair(local_endpoint_, this));
		}
	}
	recv_buffer_.recreate(mtu_size);
#ifdef RUDP_SCRAMBLE
	zero_8_bytes_.resize(8);
	memset(buffer_cast<char*>(zero_8_bytes_),0,zero_8_bytes_.size());
#endif
};

basic_shared_udp_layer::~basic_shared_udp_layer()
{
	{
		fast_mutex::scoped_lock lock(s_shared_this_type_pool_mutex_);
		s_shared_this_type_pool_.erase(local_endpoint_);
		LOG(
			std::cout<<"basic_shared_udp_layer size="<<s_shared_this_type_pool_.size()<<"\n"
			);
	}
	close_without_protector();
	//if(lingerSendTimer_)
	//{
	//	lingerSendTimer_->cancel();
	//	lingerSendTimer_.reset();
	//}

	BOOST_ASSERT(acceptors_.empty());
	DEBUG_SCOPE(
		for (std::size_t i=0;i<flows_.size();++i)
		{
			BOOST_ASSERT(flows_[i].flow==NULL);
		}
		);
}

void basic_shared_udp_layer::start()
{
	if (state_!=INIT)
		return;
	state_=STARTED;
	get_io_service().post(boost::bind(&this_type::async_receive,SHARED_OBJ_FROM_THIS));
}

bool basic_shared_udp_layer::is_shared_endpoint(const endpoint_type& endpoint)
{
	fast_mutex::scoped_lock lock(s_shared_this_type_pool_mutex_);
	return s_shared_this_type_pool_.find(endpoint)!=s_shared_this_type_pool_.end(); 
}

//#define _LOST_TEST
#ifdef _LOST_TEST
#	define _LOST_RATE 0.25
#endif
void basic_shared_udp_layer::handle_receive(const error_code& ec, 
	std::size_t bytes_transferred)
{
	//OBJ_PROTECTOR(protector);
	if (state_!=STARTED)
		return;
	if (!ec)
	{
#if defined(_LOST_TEST) && defined(P2ENGINE_DEBUG)
		WARNING("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
		WARNING("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
		WARNING("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
		WARNING("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
		WARNING("!!!!!!!!!!!!!!! LOST_TEST is OPEN !!!!!!!!!!!!!\n");
		WARNING("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
		WARNING("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
		WARNING("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
		WARNING("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
		if (in_probability(1.0-_LOST_RATE))
#endif
		{
			global_remote_to_local_speed_meter()+=bytes_transferred;
			do_handle_received(recv_buffer_.buffer_ref(0,bytes_transferred));
			recv_buffer_.recreate(mtu_size);
		}
		async_receive();
	}
	else
	{
		LOG(
			LogWarning("basic_shared_udp_layer receiving error"
			", local_endpoint=%s, errno=%d, error msg=:%s",
			endpoint_to_string(local_endpoint_).c_str(),
			ec.value(),ec.message().c_str()
			);
		);

		if (ec == asio::error::message_size)
		{
			LOG(
				LogWarning("the packet is too long, drop it, still keep receiving");
			);
			error_code err;
			static char tmp[0xffff+1];
			std::size_t len=socket_.available(err);
			if (!err&&len>sizeof(tmp))
			{
				char * longTmpBuf=(char*)std::malloc(len);
				global_remote_to_local_speed_meter()+=
					socket_.receive(asio::buffer(longTmpBuf,len),0,err);//too long，drop it
				std::free(longTmpBuf);
			}
			else if(len>0)
			{
				if (len<2*mtu_size)
				{
					recv_buffer_.recreate(len);
					bytes_transferred =socket_.receive(
						asio::buffer(buffer_cast<char*>(recv_buffer_),len),0,err);
					global_remote_to_local_speed_meter()+=bytes_transferred;
					if (!err)
						do_handle_received(recv_buffer_.buffer_ref(0,bytes_transferred));
					recv_buffer_.recreate(mtu_size);
				}
				else
				{
					global_remote_to_local_speed_meter()+=
						socket_.receive(asio::buffer(tmp,sizeof(tmp)),0,err);//too long，drop it
				}
			}
			async_receive();
		}
		else if (
			true//忽略所有错误
			//ec != errc::host_unreachable
			//ec != errc::fault
			//&& ec != errc::connection_reset
			//&& ec != errc::connection_refused
			//&& ec != errc::connection_aborted
			)// don't stop listening on recoverable errors
		{
			LOG(
				LogWarning("the packet is too long, drop it, still keep receiving");
			);
			async_receive();
		}
		else
		{
			LOG(
				LogWarning("the packet is too long, drop it, still keep receiving");
			);
		}
	}
}

void basic_shared_udp_layer::async_receive()
{
	if (state_!=STARTED)
		return;

	using p2engine::buffer_cast;	
	if (!recv_handler_)
	{
		this->recv_handler_.reset(new allocator_wrap_handler(
			boost::bind(&this_type::handle_receive,SHARED_OBJ_FROM_THIS,_1, _2)
			));
	}
	error_code ec;
	if (socket_.available(ec)>0&&!ec&&++continuous_recv_cnt_<2)
	{
		BOOST_ASSERT(recv_buffer_.size()>0);
		size_t len=socket_.receive_from(
			asio::buffer(buffer_cast<char*>(recv_buffer_),recv_buffer_.size()),
			sender_endpoint_,0,ec
			);
		handle_receive(ec,len);
	}
	else
	{
		continuous_recv_cnt_=0;
		socket_.async_receive_from(
			asio::buffer(buffer_cast<char*>(recv_buffer_),recv_buffer_.size()),
			sender_endpoint_,*recv_handler_
			);
	}
}

error_code basic_shared_udp_layer::register_acceptor(const void* acc,
	const std::string& domainName,
	const recvd_request_handler_type& callBack,
	error_code& ec
	)
{
	fast_mutex::scoped_lock lockAcceptor(acceptor_mutex_);

	acceptor_element elm;
	elm.acceptor=(void*)acc;
	elm.handler=callBack;
	BOOST_AUTO(insertRst,acceptors_.insert(std::make_pair(domainName,elm)));
	if (!insertRst.second)
		ec=asio::error::already_open; 
	else
		ec.clear();
	return ec;
}

void basic_shared_udp_layer::register_flow(const void* flow, 
	const recvd_data_handler_type& callBack,
	uint32_t& id, error_code& ec)
{
	OBJ_PROTECTOR(protector);

	fast_mutex::scoped_lock lock(flow_mutex_);

	while(released_id_catch_.size()>0)
	{
		if (!released_id_keeper_.is_keeped(released_id_catch_.front()))
		{
			id_allocator_.release_id(released_id_catch_.front());
			released_id_catch_.pop_front();
		}
		else
		{
			break;
		}
	}

	id=id_allocator_.alloc_id();
	if (id>=INVALID_FLOWID)
	{
		__release_flow_id(id);
		ec=asio::error::no_descriptors;
		id=INVALID_FLOWID;
		return;//too much,drop it
	}
	else
	{			
		if (static_cast<uint32_t>(flows_.size())<=id)
		{
			if (flows_.size()==0)
				flows_.reserve(512);
			flows_.resize(id+1);
		}
		BOOST_ASSERT(!flows_[id].flow&&!flows_[id].handler);
		flows_[id].flow=const_cast<void*>(flow);
		flows_[id].handler=callBack;
		ec.clear();
		++flows_cnt_;
	}
}

void basic_shared_udp_layer::unregister_flow(uint32_t flow_id,void* flow)
{
	fast_mutex::scoped_lock lock(flow_mutex_);

	if (flow_id!=INVALID_FLOWID&&flow_id<flows_.size())
	{
		BOOST_ASSERT(flows_[flow_id].flow==flow);
		UNUSED_PARAMETER(flow);
		__release_flow_id(flow_id);
	}
	else
	{
		BOOST_ASSERT(0);
	}
	if (flows_cnt_==0&&acceptors_.empty())
	{
		recv_handler_.reset();//the handler is cycle refed，we must release it，otherwise THIS will not be deleted
	}
}

void basic_shared_udp_layer::__release_flow_id(int id)
{
	--flows_cnt_;
	flows_[id].reset();
	lingerSends_.erase(id);

	released_id_catch_.push_back(id);
	released_id_keeper_.try_keep(id, seconds(64));//we don't reuse the id in a short period 
	while(released_id_catch_.size()>0)
	{
		if (!released_id_keeper_.is_keeped(released_id_catch_.front()))
		{
			id_allocator_.release_id(released_id_catch_.front());
			released_id_catch_.pop_front();
		}
		else
		{
			break;
		}
	}
}

void  basic_shared_udp_layer::unregister_acceptor(const void*acptor)
{
	fast_mutex::scoped_lock lock(acceptor_mutex_);

	acceptor_container::iterator itr=acceptors_.begin();
	for (;itr!=acceptors_.end();++itr)
	{
		if (itr->second.acceptor==const_cast<void*>(acptor))
		{
			acceptors_.erase(itr);
			break;
		}
	}
	if (flows_cnt_==0&&acceptors_.empty())
	{
		recv_handler_.reset();//the handler is cycle refed，we must release it，otherwise THIS will not be deleted
	}
}

std::size_t basic_shared_udp_layer::async_send_to(const safe_buffer& safebuffer,
	const endpoint_type& ep, error_code& ec)
{
	std::size_t len=buffer_size(safebuffer);
	if(len==0)
		return 0;

#ifdef RUDP_SCRAMBLE
	boost::array<asio::const_buffer,2> safe_sndbufs={{
		zero_8_bytes_.to_asio_const_buffer(),
			safebuffer.to_asio_const_buffer()
	}};
	socket_.async_send_to(safe_sndbufs,ep,s_dummy_callback);
	global_local_to_remote_speed_meter()+=(len+zero_8_bytes_.size());
	return len;
	/*
	boost::array<asio::const_buffer,2> sndbufs={{
	asio::buffer(buffer_cast<const char*>(zero_8_bytes_),zero_8_bytes_.size()),
	asio::buffer(buffer_cast<const char*>(safebuffer),safebuffer.size())
	}};
	global_local_to_remote_speed_meter()+=(len+zero_8_bytes_.size());
	socket_.send_to(sndbufs,ep,0,ec);
	if (ec)
	{
	if (ec == boost::asio::error::would_block
	||ec == boost::asio::error::try_again
	)
	{
	ec.clear();
	boost::array<asio::const_buffer,2> safe_sndbufs={{
	zero_8_bytes_.to_asio_const_buffer(),
	safebuffer.to_asio_const_buffer()
	}};
	socket_.async_send_to(safe_sndbufs,ep,s_dummy_callback);
	}
	}
	return len;
	*/
#else
	global_local_to_remote_speed_meter()+=len;
	socket_.async_send_to(safebuffer.to_asio_const_buffer(),ep,s_dummy_callback);
	return len;
	/*
	socket_.send_to(
	asio::buffer(buffer_cast<const char*>(safebuffer),buffer_size(safebuffer)),
	ep,0,ec);
	if (ec)
	{
	if (ec == boost::asio::error::would_block
	||ec == boost::asio::error::try_again
	)
	{
	ec.clear();
	socket_.async_send_to(safebuffer.to_asio_const_buffer(),ep,s_dummy_callback);
	}
	}
	return len;
	*/
#endif
}

void basic_shared_udp_layer::do_handle_received(const safe_buffer& buffer)
{
#ifdef RUDP_SCRAMBLE
	if (buffer.length()<packet_format_type::format_size()+(std::size_t)8)//8 bytes of zero
	{
		return;//too short,drop it
	}
	safe_buffer buf=buffer.buffer_ref(8);
	do_handle_received_urdp_msg(buf);
#else
	if (buffer.length()<packet_format_type::format_size())
	{
		return;//too short,drop it
	}
	do_handle_received_urdp_msg(const_cast<safe_buffer&>(buffer));
#endif
}

void basic_shared_udp_layer::do_handle_received_urdp_msg(safe_buffer& buffer)
{
	packet<packet_format_type> urdpHeaderDef(buffer);
	uint32_t dstPeerID=INVALID_FLOWID;

	if (is_conn_request_vistor<packet_format_type>()(urdpHeaderDef))
	{
		fast_mutex::scoped_lock lockAcceptor(acceptor_mutex_);

		std::string domainName=get_demain_name_vistor<packet_format_type>()(urdpHeaderDef,buffer);
		//检查是否有监听在这一domain上的acceptor
		acceptor_container::iterator itr=acceptors_.find(domainName);
		if (itr==acceptors_.end())
		{
			if (unreachable_endpoint_keeper_.try_keep(sender_endpoint_,seconds(2)))
			{
				error_code ec;
				safe_buffer buf=make_refuse_vistor<packet_format_type>()(urdpHeaderDef);
				async_send_to(buf,sender_endpoint_,ec);
			}
			return;//drop
		}

		request_uuid id;
		id.remoteEndpoint=sender_endpoint_;
		id.remotePeerID=get_src_peer_id_vistor<packet_format_type>()(urdpHeaderDef);
		id.session=get_session_vistor<packet_format_type>()(urdpHeaderDef);

		BOOST_AUTO(keeperItr,request_uuid_keeper_.find(id));
		if (keeperItr!=request_uuid_keeper_.end())
		{//已经给对方分配了一个flow，查找到这个flow响应request
			dstPeerID=keeperItr->flow_id;
			id.flow_id=dstPeerID;
		}
		else
		{//新来的，分配一个flow，并记录flow id
			acceptor_element& acceptorElement= itr->second;
			id.flow_id=acceptorElement.handler(id.remoteEndpoint);
			BOOST_ASSERT(std::find(released_id_catch_.begin(),released_id_catch_.end(),id.flow_id)==released_id_catch_.end());
			request_uuid_keeper_.try_keep(id,seconds(60));
			dstPeerID=id.flow_id;
		}
		BOOST_ASSERT(dstPeerID!=INVALID_FLOWID);
	}
	else
	{
		dstPeerID=get_dst_peer_id_vistor<packet_format_type>()(urdpHeaderDef);
	}

	fast_mutex::scoped_lock lockFlow(flow_mutex_);
	if(dstPeerID!=INVALID_FLOWID
		&&dstPeerID<(uint32_t)flows_.size()
		&&flows_[dstPeerID].flow
		&&flows_[dstPeerID].handler
		//&&(*(flows_[dstPeerID].handler)
		)
	{
		BOOST_ASSERT(std::find(released_id_catch_.begin(),released_id_catch_.end(),dstPeerID)==released_id_catch_.end());
		lockFlow.unlock();
		flows_[dstPeerID].handler(buffer,sender_endpoint_);
	}
	else
	{
		//DO nothing! Do not send refuse! Others , remote will send refuse too,and local send refuse, remote send refuse ....
		//CPU usage will be very hight.
		if(!is_refuse_vistorr<packet_format_type>()(urdpHeaderDef))
		{
			error_code ec;
			safe_buffer buf=make_refuse_vistor<packet_format_type>()(urdpHeaderDef);
			async_send_to(buf,sender_endpoint_,ec);
		}
	}
}

NAMESPACE_END(urdp);
NAMESPACE_END(p2engine);
