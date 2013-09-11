#include "p2engine/safe_buffer_io.hpp"
#include "p2engine/atomic.hpp"
#include "p2engine/rdp/urdp_visitor.hpp"
#include "p2engine/rdp/urdp_flow.hpp"
#include "p2engine/rdp/const_define.hpp"

NAMESPACE_BEGIN(p2engine);
NAMESPACE_BEGIN(urdp);

namespace
{
	typedef urdp_flow::time32_type time32_type;

	const time32_type MIN_RTO  =  250; // 250 ms(RFC1122, Sec 4.2.3.1 "fractions of a second")
	const time32_type DEF_RTO  =  3*1000; // 3 seconds (RFC1122, Sec 4.2.3.1)
	const time32_type MAX_RTO  =  8*1000;// (RFC is 60 seconds)
	const time32_type ACK_DELAY =  100; // 100 milliseconds

	const time32_type DEFAULT_TIMEOUT = 10*1000; 
	const time32_type CLOSED_TIMEOUT = 60 * 1000; // If the connection is closed, once per minute

	const time32_type IDLE_PING = 600 * 1000; //(note: WinXP SP2 firewall udp timeout is 90 seconds)
	const time32_type IDLE_TIMEOUT = 20 * 1000; //;rfc is 1200s
	const time32_type WAIT_FIN_ACK_TIMEOUT = 10 * 1000; //;rfc is 1200s

	const time32_type MIN_CLOCK_CHECK_TIME=30;

	template<typename Type>
	inline Type bound(Type lower, Type middle, Type upper) 
	{	
		return std::min<Type>(std::max<Type>(lower, middle), upper);
	}

	inline time32_type scape_zero(time32_type t)
	{
		return t? t: ++t;
	}
}

DEBUG_SCOPE(
	static atomic<int32_t>& counter()
{
	static atomic<int32_t> counter_(0);
	BOOST_ASSERT(counter_>=0);
	return counter_;
}
);
#if defined(_DEBUG) || defined(_DEBUG_SCOPE)
#	define DEBUG_FLOW_CNT
#endif

#ifdef DEBUG_FLOW_CNT
std::set<urdp_flow*> s_urdp_flow_map;
#endif


boost::shared_ptr<urdp_flow> urdp_flow::create_for_active_connect(connection_sptr sock, 
																  io_service& ios, 
																  const endpoint_type& local_edp, 
																  error_code& ec )
{
	shared_ptr obj(new this_type(ios), shared_access_destroy<this_type>());
	obj->b_active_=true;
	obj->m_socket=sock.get();
	obj->m_state=INIT;
	obj->m_self_holder=obj;//hold self
	obj->m_token=shared_layer_type::create_flow_token(ios, local_edp, obj.get(), 
		boost::bind(&this_type::called_by_sharedlayer_on_recvd, obj.get(), _1, _2), 
		ec);
	return obj;
}

boost::shared_ptr<urdp_flow> urdp_flow::create_for_passive_connect(io_service& ios, 
																   acceptor_sptr acceptor, 
																   shared_layer_sptr sharedLayer, 
																   const endpoint_type& remote_edp, 
																   error_code& ec )
{
	shared_ptr obj(new this_type(ios), shared_access_destroy<this_type>());
	obj->m_acceptor=acceptor;
	obj->m_remote_endpoint=remote_edp;
	obj->b_active_=false;
	obj->m_state=LISTEN;
	obj->m_self_holder=obj;//hold self
	obj->m_token=shared_layer_type::create_flow_token(sharedLayer, obj.get(), 
		boost::move(boost::bind(&this_type::called_by_sharedlayer_on_recvd, obj.get(), _1, _2)), 
		ec);
	obj->m_timer->async_wait(milliseconds(IDLE_TIMEOUT));//if nothing happened in IDLE_TIMEOUT, close
	return obj;
}

urdp_flow::urdp_flow(io_service& ios)
	: basic_engine_object(ios)
	, m_state(INIT)
	, remote_to_local_lost_rate_(-1)
	, local_to_remote_lost_rate_(-1)
	, in_speed_meter_(milliseconds(2000))
	, out_speed_meter_(milliseconds(2000))
{
	set_obj_desc("urdp_flow");
	__init();

#ifdef DEBUG_FLOW_CNT
	urdp_flow* ptr=0;
	int mapSize=s_urdp_flow_map.size();
	for (std::set<urdp_flow*>::iterator itr=s_urdp_flow_map.begin();
		itr!=s_urdp_flow_map.end();++itr)
	{
		ptr=*itr;
	}
	s_urdp_flow_map.insert(this);
#endif

	DEBUG_SCOPE(
		counter()++;
	);
}

urdp_flow::~urdp_flow() 
{
	BOOST_ASSERT(m_state==CLOSED);

#ifdef DEBUG_FLOW_CNT
	s_urdp_flow_map.erase(this);
#endif

	DEBUG_SCOPE(
		counter()--;
	//std::cout<<"~urdp_flow()---------------------urdp_flow_count:"<<counter()<<std::endl;
	);
}

void urdp_flow::__init()
{
	time32_type now = tick_now();

	this->next_op_stamp();

	if (m_timer)
	{
		m_timer->cancel();
		m_timer->time_signal().clear();
	}
	else
	{
		m_timer=timer_type::create(get_io_service());
		m_timer->set_obj_desc("urdp_flow::m_timer");
	}
	m_timer->time_signal().bind(&this_type::__on_clock, this VC9_BIND_BUG_PARAM_DUMMY);
	m_t_close_base=now+IDLE_TIMEOUT;//we will close this if not connected in 20 seconds.
	m_t_last_on_clock=now;

	m_shutdown=SD_NONE;
	m_dissconnect_reason=__DISCONN_INIT;
	m_socket=NULL;
	//m_acceptor=NULL;
	m_remote_peer_id=get_invalid_peer_id_vistor<packet_format_type>()();
	m_ping_interval=DEFAULT_TIMEOUT;
	m_state = INIT;
	m_snd_wnd=RECV_BUF_SIZE/2;
	m_rcv_wnd = RECV_BUF_SIZE;
	m_snd_una=m_snd_nxt=random<uint32_t>(0xff, 0x7fffffff);
	m_slen = 0;
	//m_rcv_nxt=0;//it will be inited when shakehand
	m_rlen = 0;
	m_detect_readable = true;
	m_detect_writable = false;
	m_t_ack = 0;

	//m_msslevel = 0;
	//m_largest = 0;
	//m_mtu_advise = MAX_PACKET;
	m_mss =MTU_SIZE;//DEFAULT_MSS;

	m_t_rto_base = 0;

	m_cwnd = 2 * m_mss;
	m_ssthresh = RECV_BUF_SIZE;
	m_t_lastrecv = m_t_lastsend = m_t_lasttraffic = now;

	m_dup_acks = 0;
	m_recover = 0;

	m_t_recent =0;
	m_lastack =m_rcv_nxt;

	m_rto = MIN_RTO;
	m_srtt = m_rttvar = 0;//0表示未知大小

	static uint16_t session_id_seed=random<uint16_t>(0, 0xffff);
	session_id_seed+=random<uint16_t>(1, 64);
	m_session_id=session_id_seed;


	b_keep_recving_=false;
	b_close_called_=false;
	b_timer_posted_=false;
	b_ignore_all_pkt_=false;
}

void urdp_flow::called_by_sharedlayer_on_recvd(const safe_buffer& buf, 
											   const endpoint_type& from)
{
	if (!__process(buf, from))
	{
		if (!b_ignore_all_pkt_)
		{
			__packet_as_reliable_and_sendout(m_snd_nxt, CTRL_RST, NULL, tick_now());
			b_ignore_all_pkt_=true;//we will not send to remote RST again
		}
	}
	BOOST_ASSERT(m_shutdown==SD_NONE||!m_socket||m_t_close_base&&!m_timer->is_idle());
}

uint32_t urdp_flow::flow_id()const
{
	if (is_open())
		return m_token->flow_id;
	return get_invalid_peer_id_vistor<packet_format_type>()();
}

void urdp_flow::async_connect(const endpoint_type& remoteEnp, 
							  const std::string& domainName, const time_duration& time_out)
{
	BOOST_ASSERT(m_token);
	if (m_state > INIT) 
	{
		error_code ec=(m_state==ESTABLISHED)?
			asio::error::already_connected : asio::error::in_progress;
		operation_mark_post(SHARED_OBJ_FROM_THIS, 
			boost::bind(&this_type::__allert_connected, this, ec)
			);
		return;
	}

	time_duration t;
	time32_type now=tick_now();
	if (time_out==boost::date_time::pos_infin
		||time_out==boost::date_time::neg_infin
		||time_out.total_milliseconds()<=0
		)
	{
		t=milliseconds(IDLE_TIMEOUT);//if nothing happened in 20s, close
	}
	else
	{
		t=time_out;
	}
	BOOST_ASSERT(t>milliseconds(500));
	m_timer->async_wait(t);
	m_t_close_base=now+t.total_milliseconds();
	m_remote_endpoint=remoteEnp;
	b_active_=true;
	m_state = SYN_SENT;

	//send connect request 
	__queue(domainName.c_str(), domainName.length(), CTRL_CONNECT);
	__attempt_send();
	__schedule_timer(now);
}

void urdp_flow::async_connect(const std::string& remote_host, int port, 
							  const std::string&domainName, const time_duration& time_out
							  )
{
	error_code err;
	address_v4 addr=address_v4::from_string(remote_host.c_str(), err);
	if (!err)//if the format of xxx.xxx.xxx.xxx
	{
		async_connect(udp::endpoint(addr, (unsigned short)port), domainName, time_out);
	}
	else
	{
		if (!resolver_)
			resolver_.reset(new resolver_type(get_io_service()));
		resolver_query qy(remote_host, boost::lexical_cast<std::string>(port));
		resolver_->async_resolve(qy, boost::bind(&this_type::__handle_resolve, 
			SHARED_OBJ_FROM_THIS, _1, _2, domainName, time_out, op_stamp()));
	}
}

void urdp_flow::__handle_resolve(error_code ec, resolver_iterator itr, 
								 const std::string& domainName, 
								 const time_duration& time_out, op_stamp_t mark)
{
	if (is_canceled_op(mark))
		return;

	if (ec||itr==resolver_iterator())
		__allert_connected(ec);
	else
		async_connect(udp::endpoint(*itr), domainName, time_out);
}

void urdp_flow::keep_async_receiving()
{
	if (!b_keep_recving_)
	{
		b_keep_recving_=true;
		get_io_service().post(boost::bind(&this_type::__async_receive, 
			SHARED_OBJ_FROM_THIS, this->op_stamp())
			);
	}
}

void urdp_flow::__async_receive(op_stamp_t mark)
{
	if (is_canceled_op(mark)||m_state==CLOSED)
		return;
	if (!b_keep_recving_)
		return;

	error_code ec;
	safe_buffer buf;
	__recv(buf, ec);
	if (!ec)
	{
		__allert_received(buf);
		__async_receive(mark);
	}
	else
	{
		if (ec != asio::error::would_block
			&& ec != asio::error::try_again)
		{
			__allert_disconnected(ec);
		}
		else
		{
			__schedule_timer(tick_now());
		}
	}
}

void urdp_flow::async_send(const safe_buffer& buf, message_type msgType, 
						   boost::logic::tribool reliable)
{
	error_code ec;
	__send(buf, msgType, reliable, ec);
	if (ec)
	{
		if (ec == asio::error::would_block || ec == asio::error::try_again)
		{
			//do nothing? 
		}
		else
		{
			operation_mark_post(SHARED_OBJ_FROM_THIS, 
				boost::bind(&this_type::__allert_disconnected, this, ec)
				);
		}
	}
	else if(reliable)
	{
		operation_mark_post(SHARED_OBJ_FROM_THIS, 
			boost::bind(&this_type::__allert_writeable, this)
			);
	}
	if (!(!reliable))//reliable or semireliable
		__schedule_timer(tick_now());
}

safe_buffer urdp_flow::make_punch_packet(error_code& ec, const endpoint& externalEdp)
{
	if (is_open())
	{
		time32_type now = tick_now();
		double remoteToLocalLostrate=(remote_to_local_lost_rate_<0.0?0.0:remote_to_local_lost_rate_);

		packet<reliable_packet_format_type> writer;
		writer.set_control(CTRL_PUNCH);
		writer.set_peer_id(m_token->flow_id);
		//writer.set_bandwidth_recving();
		writer.set_lostrate_recving(uint32_t(remoteToLocalLostrate/LOST_RATE_PRECISION));
		writer.set_id_for_lost_detect(id_for_lost_rate_++);
		writer.set_session_id(m_session_id);
		writer.set_window((uint16_t)std::min(m_rcv_wnd, (uint32_t)0xffff));//any value
		writer.set_time_sending((uint16_t)scape_zero(now));//any value
		writer.set_time_echo(m_t_recent+((uint16_t)(now)-m_t_recent_now));//any value
		writer.set_seqno(m_snd_nxt);//any value
		writer.set_ackno(m_rcv_nxt);//any value

		//write local_endpoint
		safe_buffer_io io(&writer.buffer());
		io<<externalEdp;
		return writer.buffer();
	}
	else
	{
		ec=boost::asio::error::bad_descriptor;
		return safe_buffer();
	}
}

void urdp_flow::on_received_punch_request(const safe_buffer& buf)
{
	if (buf.length()<reliable_packet_format_type::format_size())
		return;
	safe_buffer endpoint_buf=buf.buffer_ref(reliable_packet_format_type::format_size());
	endpoint_type remote_edp;
	safe_buffer_io io(&endpoint_buf);
	io.read_v4_endpoint(remote_edp);

	error_code ec;
	if(m_token&&remote_edp.port())
	{
		for (int i=0;i<2;++i)//send 2 times
		{
			m_token->shared_layer->async_send_to(
				buf.buffer_ref(0, packet_format_type::format_size()), remote_edp, ec
				);
		}
	}
}

void urdp_flow::__on_clock(VC9_BIND_BUG_PARAM) 
{
	b_timer_posted_=false;

	if (m_state == CLOSED)
		return;

	time32_type now=tick_now();
	bool haveSentReliableMsg=false;
	m_t_last_on_clock=now;

	DEBUG_SCOPE(;
	if (m_state==CLOSING)
	{
		BOOST_ASSERT(m_t_rto_base&&(m_retrans_slist.size()||m_slist.size()));
	}
	else if(m_state==WATING_FIN_ACK)
	{
		BOOST_ASSERT(m_t_close_base);
	}
	);

	//check if it's time to close
	if(m_t_close_base && mod_less_equal(*m_t_close_base, now))
	{
		//__packet_as_reliable_and_sendout(m_snd_nxt, CTRL_RST, NULL, 0);
		__allert_disconnected(asio::error::timed_out);
		return;
	}

	// Check for idle timeout
	if ((m_state == ESTABLISHED) 
		&& mod_less_equal(m_t_lastrecv+m_ping_interval+DEFAULT_TIMEOUT, now)
		) 
	{
		__allert_disconnected(asio::error::timed_out);
		return;
	}

	// resend reliable packets?
	if (m_t_rto_base && mod_less_equal(m_t_rto_base+m_rto, now)) 
	{
		if (m_retrans_slist.empty()) 
		{
			m_t_rto_base=0;
			BOOST_ASSERT(false);//m_rto_base!=0，m_retrans_slist must not be empty
		} 
		else 
		{
			//send the oldest unacked packet
			if (!__transmit(m_retrans_slist.begin(), now)) 
			{
				//std::cout<<int((void*)this)<<"~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~retrans timed_out\n";
				__allert_disconnected(asio::error::timed_out);
				return;
			}
			haveSentReliableMsg=true;
			long nInFlight =mod_minus(m_snd_nxt, m_snd_una);
			BOOST_ASSERT(nInFlight>0);
			m_ssthresh = std::max((uint32_t)nInFlight*3/4, 2*m_mss);
			m_cwnd=std::max(m_ssthresh/2, 2*m_mss);//m_cwnd=2*m_mss;
			m_t_rto_base=scape_zero(now);//m_rto_base=0 means not set, so we use 1
			__incress_rto();
		}
	}

	//is it time to send next unreliable packet?
	while(!m_unreliable_slist.empty())
	{
		SUnraliableSegmentList::iterator itr=m_unreliable_slist.begin();
		SUnraliableSegment& seg = const_cast<SUnraliableSegment&>(*itr);
		if (mod_less_equal(seg.timeout, now))
		{
			__packet_as_unreliable_and_sendout(seg, now);
			if(seg.remainXmit<=0)
				m_unreliable_slist.erase(itr);
			else
			{
				SUnraliableSegment segcpy=seg;
				m_unreliable_slist.erase(itr);
				m_unreliable_slist.insert(segcpy);
			}
		}
		else
			break;
	}

	// Check for ping timeout 
	// Do not care about haveSentMsg!!
	time32_type pingBaseTime=(b_active_?m_t_lastsend:(m_t_lastrecv+5000));
	if ((m_state == ESTABLISHED) 
		&&mod_less_equal(pingBaseTime+m_ping_interval, now)
		&&m_slist.empty()
		&&m_retrans_slist.empty()
		) 
	{
		//std::cout<<int((void*)this)<<"~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~pint timed_out\n";
		error_code ec;
		ping(ec);
		haveSentReliableMsg=true;
	}

	// Check if it's time to probe closed windows
	if (m_state == ESTABLISHED
		&&(m_snd_wnd == 0) 
		&&mod_less_equal(m_t_lastsend + m_rto, now)
		) 
	{
		if (mod_minus(now, m_t_lastrecv) >= 15000)
		{
			__packet_as_reliable_and_sendout(m_snd_nxt, CTRL_RST, NULL, now);
			__allert_disconnected(asio::error::timed_out);
			__to_closed_state();
			return;
		}
		else if (!haveSentReliableMsg)
		{
			// probe the window
			__packet_as_reliable_and_sendout(m_snd_nxt-1, CTRL_ACK, NULL, now);
			haveSentReliableMsg=true;
		}
		// back off retransmit timer
		__incress_rto();
	}

	// Check if it's time to _send delayed acks
	if (!haveSentReliableMsg&&m_t_ack&&mod_less_equal(m_t_ack+ACK_DELAY, now)) 
	{
		__packet_as_reliable_and_sendout(m_snd_nxt, CTRL_ACK, NULL, now);
		haveSentReliableMsg=true;
	}

	__schedule_timer(now, true);
}

void urdp_flow::ping_interval(const time_duration& t)
{
	bool scheduTime=m_ping_interval>t.total_milliseconds();
	m_ping_interval=(time32_type)std::min<tick_type>(
		t.total_milliseconds(), (std::numeric_limits<time32_type>::max)()
		);
	if (scheduTime)
		__schedule_timer(tick_now());
}

void urdp_flow::ping(error_code& ec)
{
	if (!is_connected())
	{
		ec=boost::asio::error::not_connected;
		return;
	}
	if (m_slist.empty()&&m_retrans_slist.empty())
	{
		//send CTRL_ACK with a dummy bytes, to make remote send ACK to local.
		char dummy[1];
		__queue(dummy, 1, CTRL_ACK);
	}
	__attempt_send();
}

urdp_flow::endpoint_type urdp_flow::local_endpoint(error_code& ec)const
{
	if (is_open())
		return m_token->shared_layer->local_endpoint(ec);
	ec=asio::error::not_socket;
	return endpoint_type();
}

urdp_flow::endpoint_type urdp_flow::remote_endpoint(error_code& ec)const
{
	if (is_open())
	{
		BOOST_ASSERT(!m_remote_endpoint.address().is_unspecified()&&m_remote_endpoint.port());
		return m_remote_endpoint;
	}
	ec=asio::error::not_socket;
	return endpoint_type();
}

double urdp_flow::alive_probability()const
{
	if (!is_connected())
		return 0.0;

	time32_type now=tick_now();
	//this connection is just created, just believe it to be alive
	if (mod_less(now, m_establish_time+3000)) 
		return 1.0;

	time32_type rTT=m_srtt;
	double deadline=std::max<time32_type>(5000, 
		std::min<time32_type>(10000, std::min(m_ping_interval<<1, rTT<<2)));
	double a=(double)(mod_minus(now, m_t_lastrecv)-(int)(m_ping_interval+rTT));
	if (a<=0)
		return 1.0;
	double p=(deadline-a)/(deadline);
	if (p<=0)
		return 0.0;
	return p*p;
}

void urdp_flow::close(bool graceful) 
{
	if (is_canceled_op(op_stamp()))
		return;

	m_dissconnect_reason|=DISCONN_LOCAL;
	__close(graceful);
}

//
// Internal Implementation
//

void urdp_flow::__close(bool graceful) 
{
	if (!m_token||m_state==CLOSED)
	{
		__to_closed_state();
		return;
	}

	if (b_close_called_)
		return;

	set_cancel();
	m_socket=NULL;
	b_close_called_=true;

	if (!graceful||m_state<ESTABLISHED)
	{
		m_shutdown=SD_FORCEFUL;
	}
	else if(m_shutdown==SD_NONE)
	{
		m_shutdown=SD_GRACEFUL;
	}

	time32_type now=tick_now();

	BOOST_ASSERT(SD_NONE!=m_shutdown);
	if (SD_FORCEFUL==m_shutdown)
	{
		__packet_as_reliable_and_sendout(m_snd_nxt, CTRL_RST, NULL, now);
		__to_closed_state();
	}
	else if (SD_GRACEFUL==m_shutdown)
	{
		m_state=CLOSING;
		if (m_retrans_slist.empty()&&m_slist.empty())
		{
			m_t_close_base=now+WAIT_FIN_ACK_TIMEOUT;
			m_state=WATING_FIN_ACK;
			__packet_as_reliable_and_sendout(m_snd_nxt, CTRL_FIN, NULL, now);
		}
		__schedule_timer(now);
	}
	else
	{
		BOOST_ASSERT(0);
	}
}

void urdp_flow::__to_closed_state() 
{
	set_cancel();

	if (m_state==CLOSED)
	{
		BOOST_ASSERT(!m_self_holder);
		return ;
	}

	BOOST_ASSERT(m_self_holder);
	m_state=CLOSED;
	m_token.reset();
	if (m_timer)
	{
		m_timer->cancel();
		m_timer.reset();
	}
	m_socket=NULL;
	m_acceptor.reset();
	m_slist.clear();
	m_retrans_slist.clear();
	m_rlist.clear();
	m_t_rto_base=0;

	//To close state we must reset self_holder_, otherwise, we cant delete this_ptr
	//because of the "cycles of shared_ptr". 
	//But, if we just release self_holder_ here, and the urdp reset shared_ptr of this_ptr, 
	//then, this_ptr will be deleted at once. Unfortunately, this function may be
	//called by another function of this_type, so, Crash!
	//Post is the right way. 
	if (m_self_holder)
	{
		struct flow_holder{
			static void holde(boost::shared_ptr<urdp_flow>h){
				BOOST_ASSERT(!h->m_self_holder);
			};
		};
		get_io_service().post(boost::bind(&flow_holder::holde,m_self_holder));
		m_self_holder.reset();//this_ptr will not be deleted , because we post a shared_ptr of this
	}
}

void urdp_flow::__schedule_timer(time32_type now, bool calledInOnClock)
{
	if (m_state==CLOSED)
	{
		BOOST_ASSERT(!m_self_holder);
		return ;
	}
	BOOST_ASSERT(m_self_holder);

	//if expire soon, do not reschedule
	bool isIdle=m_timer->is_idle();
	int64_t expireTimeFromNow=m_timer->expires_from_now().total_milliseconds();
	if (!isIdle&&expireTimeFromNow>0&&expireTimeFromNow<MIN_CLOCK_CHECK_TIME)
		return;

	long t=(std::numeric_limits<long>::max)();
	if (__clock_check(now, t)&&!b_timer_posted_)
	{
		if(t<=0)
		{
			if (calledInOnClock)
			{
				__on_clock();
			}
			else
			{
				b_timer_posted_=true;
				get_io_service().post(boost::bind(&this_type::__on_clock, 
					SHARED_OBJ_FROM_THIS VC9_BIND_BUG_PARAM_DUMMY)
					);
			}
		}
		else
		{
			if(isIdle||t<expireTimeFromNow)
				m_timer->async_wait(milliseconds(t));
			BOOST_ASSERT(!m_timer->is_idle());
		}
	}
	BOOST_ASSERT((!m_timer->is_idle()||b_timer_posted_)||!m_t_close_base);
}

int urdp_flow::__can_let_transport_read() 
{
	if (m_state != ESTABLISHED) 
		return -1;

	BOOST_ASSERT(m_self_holder);
	//check unreliable packet
	if (!m_unreliable_rlist.empty())
	{
		return (int)m_unreliable_rlist.front().size();
	}

	//check reliable packet
	if (m_rlen < 4) //lent(2byte), msgType(2byte)
		return 0;
	RSegmentList::iterator itr(m_rlist.begin());

	BOOST_ASSERT(itr->buf.size()>=1);
	char bufForLen[2];
	bufForLen[0]=buffer_cast<const char*>(itr->buf)[0];
	if (itr->buf.size()>=2)
	{
		bufForLen[1]=buffer_cast<const char*>(itr->buf)[1];
	}
	else
	{
		++itr;
		bufForLen[1]=buffer_cast<const char*>(itr->buf)[0];//0, not 1 !!!!
	}
	const char* pbufForLen=bufForLen;
	uint16_t packetLen=read_uint16_ntoh(pbufForLen);
	uint32_t maxReadLen = packetLen+4;//the length not include lent(2byte)and msgType(2byte), so, +4
	if (maxReadLen>m_rlen)
		return 0;
	return maxReadLen;
}

int urdp_flow::__recv(safe_buffer& buf, error_code& ec) 
{
	if (!m_socket||m_state != ESTABLISHED) 
	{
		BOOST_ASSERT(m_socket);//if __recv is called, m_socket should not be NULL 
		ec=asio::error::not_connected;
		return -1;
	}

	BOOST_ASSERT(m_self_holder);

	int maxReadLen =__can_let_transport_read();
	if (maxReadLen==0)
	{
		m_detect_readable = true;
		ec=asio::error::would_block;
		return -1;
	}
	BOOST_ASSERT(maxReadLen>0);

	//read unreliable packet
	if (!m_unreliable_rlist.empty())
	{
		buf=m_unreliable_rlist.front();
		m_unreliable_rlist.pop_front();
		return (int)buf.size();
	}

	//read reliable packet
	safe_buffer_io io(&buf);
	io.prepare(maxReadLen);
	int readLen=0;
	while (maxReadLen-readLen>0)
	{
		RSegmentList::iterator itr=m_rlist.begin();
		RSegment& segment=const_cast<RSegment&>(*itr);
		int lenth=(std::min<int>)(maxReadLen-readLen, segment.buf.size());
		io.write(buffer_cast<char*>(segment.buf), lenth);
		segment.buf=segment.buf.buffer_ref(lenth);//consume(lenth);
		m_rlen -=lenth;
		readLen+=lenth;

		if (segment.buf.size()==0)
		{
			m_rlist.erase(m_rlist.begin());
		}
		else
		{
			segment.seq+=lenth;//the seq+lenth MUST be greater than (++itr)->seq.
			break;
		}
	}
	BOOST_ASSERT(maxReadLen==readLen);

	if ((RECV_BUF_SIZE - m_rlen - m_rcv_wnd) >=(std::min<uint32_t>)(RECV_BUF_SIZE / 2, m_mss)) 
	{
		bool bWasClosed = (m_rcv_wnd == 0); // !?! Not sure about this was closed business

		m_rcv_wnd = RECV_BUF_SIZE - m_rlen;

		if (bWasClosed) 
			__attempt_send(sfImmediateAck);
	}

	uint16_t msgLen;
	io>>msgLen;//trim tow bytes which are length of this packet
	BOOST_ASSERT(msgLen+2==(int)buf.length());
	return maxReadLen-2;
}

int urdp_flow::__send(safe_buffer buf, uint16_t msgType, 
					  boost::logic::tribool reliable, error_code& ec)
{
	if (!m_socket||m_state != ESTABLISHED) 
	{
		BOOST_ASSERT(m_socket);//if __send is called, m_socket should not be NULL 
		ec=asio::error::not_connected;
		return -1;
	}

	BOOST_ASSERT(m_self_holder);

	if (reliable)//reliable
	{
		BOOST_ASSERT(reliable.value==boost::logic::tribool::true_value);

		//write packet chunk header
		uint16_t bufLen=(uint16_t)buf.size();
		BOOST_ASSERT(bufLen<=(std::numeric_limits<uint16_t>::max)());
		char header[4];
		char* ptoHeader=header;
		write_uint16_hton(bufLen, ptoHeader);
		write_uint16_hton((uint16_t)msgType, ptoHeader);
		__queue(header, 4, CTRL_DATA, buf.size());
		int written = __queue(buffer_cast<char*>(buf), bufLen, CTRL_DATA, 0);//write data
		__attempt_send();

		if (m_slen >= SND_BUF_SIZE) 
		{
			m_detect_writable = true;
			ec=asio::error::would_block;
			return -1;
		}
		return written;
	}
	else//semireliable&unreliable
	{
		size_t bufLen=buf.size();
		BOOST_ASSERT(bufLen<=2*m_mss);
		if (bufLen>2*m_mss||bufLen==0)
			return (int)bufLen;//do not send packet with length>mss

		time32_type now=tick_now();

		SUnraliableSegment seg;
		seg.buf=buf;
		seg.msgType=msgType;
		seg.pktID=++m_unreliable_pktid;

		if (!reliable)//unreliable
		{
			BOOST_ASSERT(reliable.value==boost::logic::tribool::false_value);
			seg.control=CTRL_UNRELIABLE_DATA;
			seg.remainXmit=1;
		}
		else//semireliable
		{
			BOOST_ASSERT(reliable.value==boost::logic::tribool::indeterminate_value);
			seg.control=CTRL_SEMIRELIABLE_DATA;
			double lostRate=local_to_remote_lost_rate();
			if (lostRate>0.1)
			{
				seg.timeout=now+random(10, 30);//resend
				seg.remainXmit=3;
			}
			else if (lostRate>0.01||lostRate<0)//<0 means not known
			{
				seg.timeout=now+random(20, 50);//resend
				seg.remainXmit=2;
			}
			else
			{
				seg.remainXmit=1;
			}
		}
		__packet_as_unreliable_and_sendout(seg, now);
		if (seg.remainXmit>0)
			m_unreliable_slist.insert(seg);
		return (int)bufLen;//如果长度长度大于mss，则直接丢弃而不发
	}
}

uint32_t urdp_flow::__queue(const char * data, std::size_t len, uint8_t ctrlType, 
							std::size_t reserveLen) 
{
	BOOST_ASSERT(m_self_holder);

	std::size_t writeLen=0;
	while (len-writeLen>0)
	{
		// We can concatenate data if the last segment is the same type
		// (control v. regular data), and has not been transmitted yet
		if (ctrlType==CTRL_DATA
			&& !m_slist.empty() 
			&& (m_slist.back().xmit == 0)
			&& (m_slist.back().ctrlType == ctrlType) 
			&& (m_slist.back().buf.size()<m_mss)
			//&& (m_slist.back().buf.size()-m_mss>8)//
			) 
		{
			safe_buffer& bf=m_slist.back().buf;
			std::size_t canWriteLen=std::min(m_mss-bf.size(), len-writeLen);
			safe_buffer_io io(&bf);
			io.write(data+writeLen, canWriteLen);
			writeLen+=canWriteLen;
			m_slen+=canWriteLen;
		}
		else 
		{
			std::size_t canWriteLen=std::min(m_mss, len-writeLen);
			std::size_t newBufferLen=canWriteLen;
			if (writeLen+canWriteLen==len)
			{
				newBufferLen=std::min(m_mss, canWriteLen+reserveLen);
				if (newBufferLen>(m_mss/2))
					newBufferLen=m_mss;
			}
			SSegment sseg(m_snd_una + m_slen, ctrlType);
			safe_buffer_io io(&sseg.buf);
			io.prepare(newBufferLen);
			io.write(data+writeLen, canWriteLen);
			writeLen+=canWriteLen;
			m_slen+=canWriteLen;
			m_slist.push_back(sseg);
			//std::cout<<"~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"<<sseg.buf.size()<<std::endl;
		}
	}
	return (uint32_t)writeLen;
}

int urdp_flow::__packet_as_unreliable_and_sendout(SUnraliableSegment& seg, time32_type now) 
{
	BOOST_ASSERT(m_self_holder);

	double remoteToLocalLostrate=(remote_to_local_lost_rate_<0.0?0.0:remote_to_local_lost_rate_);

	const size_t format_size=unreliable_packet_format_type::format_size();
	safe_buffer urdp_header_buf(format_size+4);//4 bytes for pktID and msgType

	packet<unreliable_packet_format_type> urdp_header(urdp_header_buf,true);
	urdp_header.set_control(seg.control);
	urdp_header.set_peer_id(seg.control==CTRL_CONNECT?m_token->flow_id:m_remote_peer_id);
	urdp_header.set_lostrate_recving(uint32_t(remoteToLocalLostrate/LOST_RATE_PRECISION));
	urdp_header.set_id_for_lost_detect(id_for_lost_rate_++);
	urdp_header.set_session_id(m_session_id);
	urdp_header.set_window((uint16_t)std::min(m_rcv_wnd, (uint32_t)0xffff));

	//write packet chunk header
	char* ptoHeader=buffer_cast<char*>(urdp_header.buffer())+format_size;
	write_uint16_hton((uint16_t)seg.pktID, ptoHeader);
	write_uint16_hton((uint16_t)seg.msgType, ptoHeader);

	size_t dataLen=seg.buf.length();
	boost::array<safe_buffer, 2> bufVec={{urdp_header.buffer(),seg.buf}};
	error_code ec;
	m_token->shared_layer->async_send_to(bufVec, m_remote_endpoint, ec);

	out_speed_meter_+=(seg.buf.length()+format_size);

	seg.remainXmit--;
	m_t_lasttraffic=m_t_lastsend = now;
	seg.timeout=now+random(10, 30);

	return (int)dataLen;
}

int urdp_flow::__packet_as_reliable_and_sendout(uint32_t seq, uint8_t control, 
												const safe_buffer* data, time32_type now) 
{
	BOOST_ASSERT(m_self_holder);

	double remoteToLocalLostrate=(remote_to_local_lost_rate_<0.0?0.0:remote_to_local_lost_rate_);

	const size_t format_size=reliable_packet_format_type::format_size();
	packet<reliable_packet_format_type> urdp_header;
	urdp_header.set_control(control);
	urdp_header.set_peer_id(control==CTRL_CONNECT?m_token->flow_id:m_remote_peer_id);
	//TODO:
	//h.set_bandwidth_recving();
	urdp_header.set_lostrate_recving(uint32_t(remoteToLocalLostrate/LOST_RATE_PRECISION));
	urdp_header.set_id_for_lost_detect(id_for_lost_rate_++);
	urdp_header.set_session_id(m_session_id);
	urdp_header.set_window((uint16_t)std::min(m_rcv_wnd, (uint32_t)0xffff));
	urdp_header.set_time_sending((uint16_t)scape_zero(now));
	urdp_header.set_time_echo(m_t_recent+((uint16_t)(now)-m_t_recent_now));
	urdp_header.set_seqno(seq);
	urdp_header.set_ackno(m_rcv_nxt);

	size_t dataLen=data?data->size():0;
	out_speed_meter_+=(format_size+dataLen);

	if (dataLen)
	{
		boost::array<safe_buffer, 2> bufVec={{urdp_header.buffer(),*data}};
		error_code ec;//FIXME：ignor??
		m_token->shared_layer->async_send_to(bufVec, m_remote_endpoint, ec);
	}
	else
	{
		error_code ec;//FIXME：ignor??
		m_token->shared_layer->async_send_to(urdp_header.buffer(), 
			m_remote_endpoint, ec);
	}

	m_lastack = m_rcv_nxt;
	m_t_ack = 0;//we have ACK remote
	m_t_lasttraffic=m_t_lastsend = now;
	return dataLen;
}

bool urdp_flow::__clock_check(time32_type now, long& nTimeout) 
{
	if (m_state == CLOSED)
		return false;

	BOOST_ASSERT(m_self_holder);

	nTimeout =std::max(m_ping_interval, DEFAULT_TIMEOUT) ;
	if (m_t_close_base)
		nTimeout = std::min(nTimeout, mod_minus(*m_t_close_base, now));
	if (m_t_ack)
		nTimeout = std::min(nTimeout, mod_minus(m_t_ack + ACK_DELAY, now));
	if (m_t_rto_base) 
		nTimeout = std::min(nTimeout, mod_minus(m_t_rto_base + m_rto, now));
	if (m_snd_wnd == 0)
		nTimeout = std::min(nTimeout, mod_minus(m_t_lastsend + m_rto, now));
	if (m_state >= ESTABLISHED&&m_slist.empty()&&m_retrans_slist.empty())
		nTimeout = std::min(nTimeout, mod_minus(m_t_lasttraffic + m_ping_interval, now));
	if(!m_unreliable_slist.empty())
		nTimeout = std::min(nTimeout, mod_minus(m_unreliable_slist.begin()->timeout, now));

	long lastCheckElapsed=mod_minus(now, m_t_last_on_clock);
	BOOST_ASSERT(lastCheckElapsed>=0);
	if (lastCheckElapsed<(long)MIN_CLOCK_CHECK_TIME)
		nTimeout=std::max(((long)MIN_CLOCK_CHECK_TIME-lastCheckElapsed), nTimeout);
	if (nTimeout<0)
		nTimeout=0;

	return true;
}

void urdp_flow::__calculate_local_to_remote_lost_rate(
	const packet<unreliable_packet_format_type>& urdp_header ,time32_type now)
{
	//calculate lost rate
	wrappable_integer<int8_t>idForLostDetect=urdp_header.get_id_for_lost_detect();
	__calc_remote_to_local_lost_rate(now, &idForLostDetect);
	if (local_to_remote_lost_rate_<0)
	{
		local_to_remote_lost_rate_
			=urdp_header.get_lostrate_recving()*LOST_RATE_PRECISION;
	}
	else
	{
		double lostRate=urdp_header.get_lostrate_recving()*LOST_RATE_PRECISION;
		BOOST_ASSERT(lostRate>=0);
		if (lostRate<0)
			lostRate=0;
		double a=std::max(200.0, out_speed_meter_.bytes_per_second())
			/(global_local_to_remote_speed_meter().bytes_per_second()+1.0);
		BOOST_ASSERT(a>=0);
		if (a>1.0)a=1.0;
		if(a<0)a=0;
		local_to_remote_lost_rate_=lostRate;

		double oldRate=global_local_to_remote_lost_rate();
		double newRate=(oldRate*(1.0-a)+lostRate*a);
		set_global_local_to_remote_lost_rate(oldRate*0.125+newRate*0.875);
	}
}

bool urdp_flow::__process(const safe_buffer& sbuf, const endpoint_type& from) 
{	
	if (m_state==CLOSED)
	{
		BOOST_ASSERT(!m_self_holder);
		return true;
	}

	BOOST_ASSERT(m_self_holder&&m_self_holder.get()==this);

	time32_type now = tick_now();
	do{
		const std::size_t unreliable_format_size
			=unreliable_packet_format_type::format_size();
		if (sbuf.length()<unreliable_format_size)
			return false;
		packet<unreliable_packet_format_type> urdp_header(sbuf);

		//check session_id and remote_endpoint
		if (m_state>=SYN_SENT)
		{
			if (urdp_header.get_session_id()!= m_session_id
				||(m_state>=ESTABLISHED&&m_remote_endpoint!=from)
				) 
				return false;
		}

		m_t_lasttraffic = m_t_lastrecv = now;
		m_snd_wnd = urdp_header.get_window();
		in_speed_meter_+=sbuf.length();

		//calculate lost rate
		__calculate_local_to_remote_lost_rate(urdp_header,now);

		//is it unreliable msg
		int ctrlType=urdp_header.get_control();
		if (CTRL_UNRELIABLE_DATA==ctrlType
			||CTRL_SEMIRELIABLE_DATA==ctrlType
			)
		{
			if (sbuf.size()>=unreliable_format_size+4
				&&m_socket&&ESTABLISHED==m_state)
			{
				safe_buffer pkt=sbuf.buffer_ref(unreliable_format_size);
				safe_buffer_io sbio(&pkt);
				uint16_t id;
				sbio>>id;
				if (CTRL_UNRELIABLE_DATA==ctrlType||m_unreliable_rkeeper.try_keep(id, seconds(8)))
				{
					m_unreliable_rlist.push_back(pkt);
					__allert_readable();
				}
				return true;
			}
			return false;
		}
	}while(0);

	if (sbuf.length()<reliable_packet_format_type::format_size())
		return false;

	packet<reliable_packet_format_type> urdp_header(sbuf);
	uint32_t len=(uint32_t)sbuf.size();
	uint32_t rcvdDataLen=len-reliable_packet_format_type::format_size();
	const char* data=buffer_cast<char*>(sbuf)+reliable_packet_format_type::format_size();

	bool notifyConnected=false;
	//bool notifyDisconnected=false;
	bool notifyWritable=false;
	bool notifyReadable=false;
	bool notifyAccepet=false;
	bool bConnect = false;
	bool shouldImediateAck=false;

	uint32_t seqno=urdp_header.get_seqno();
	uint32_t ackno=urdp_header.get_ackno();

	switch (urdp_header.get_control())
	{
	case CTRL_NETWORK_UNREACHABLE:
		if (urdp_header.get_session_id()==m_session_id)
		{
			if (m_state == SYN_SENT)
				__allert_connected(asio::error::host_unreachable);
			else
			{
				m_dissconnect_reason|=DISCONN_REMOTE;
				__allert_disconnected(asio::error::connection_reset);
			}
		}
		return true;

	case CTRL_CONNECT:
		bConnect = true;
		if (m_state == LISTEN)
		{
			m_state = SYN_RCVD;

			m_remote_peer_id=urdp_header.get_peer_id();
			m_session_id=urdp_header.get_session_id();
			m_lastack=m_rcv_nxt=seqno+rcvdDataLen;
			m_remote_endpoint=from;
			m_t_recent =urdp_header.get_time_sending();
			m_t_recent_now=now;
			m_t_lasttraffic = m_t_lastrecv = now;

			//send ACK imediatelly
			char myPeerID[4];
			char* pMyPeerID=myPeerID;
			write_int32_hton(m_token->flow_id, pMyPeerID);
			__queue(myPeerID, 4, CTRL_CONNECT_ACK);
			__attempt_send(sfImmediateAck);
			__schedule_timer(now);
		}
		return true;

	case CTRL_CONNECT_ACK:
		if (m_state == SYN_SENT) 
		{
			if (urdp_header.get_session_id()!=m_session_id)
				return false;
			if (rcvdDataLen<4)
				return false;
			//const char* pHisPeerID=data;
			m_remote_peer_id=read_uint32_ntoh(data);
			m_state = ESTABLISHED;
			m_lastack=m_rcv_nxt=seqno;
			notifyConnected=true;// !!
			shouldImediateAck=true;
		}
		break;

	case CTRL_PUNCH:
		m_remote_endpoint=from;
		m_t_lasttraffic = m_t_lastrecv = now;
		if (m_retrans_slist.size()==1)//must be request pkt
		{
			SSegment& seg=m_retrans_slist.front();
			if (seg.ctrlType==CTRL_CONNECT)
			{
				seg.xmit=1;//set to 1
				__transmit(m_retrans_slist.begin(), now);//send conn request impdiatelly
			}
		}
		return true;

	case CTRL_RST:
		m_dissconnect_reason|=DISCONN_REMOTE;
		__allert_disconnected(asio::error::connection_reset);
		return true;

	case CTRL_FIN:
		m_dissconnect_reason|=DISCONN_REMOTE;
		__packet_as_reliable_and_sendout(m_snd_nxt, CTRL_FIN_ACK, NULL, now);
		__allert_disconnected(asio::error::connection_reset);
		return true;

	case CTRL_FIN_ACK:
		__allert_disconnected(error_code());
		return true;

	case CTRL_DATA:
	case CTRL_ACK:
		break;

	default:
		BOOST_ASSERT(0);
		break;
	}

	if (m_state>=SYN_SENT)
	{
		if (m_remote_endpoint!=from)
			return false;//drop it
	}
	m_t_lasttraffic = m_t_lastrecv = now;

	// Update timestamp //seqno<=ACK<seq+dataLen
	if (mod_less_equal(seqno, m_lastack) 
		&& mod_less(m_lastack, seqno+ rcvdDataLen)
		) 
	{
		m_t_recent_now=now;
		m_t_recent =urdp_header.get_time_sending() ;
	}

	//m_snd_una<h.ackno<=m_snd_nxt
	if (mod_less(m_snd_una, ackno)&&mod_less_equal(ackno, m_snd_nxt)) 
	{		
		// Calculate round-trip time
		uint16_t timeEcho=(uint16_t)urdp_header.get_time_echo();
		if (timeEcho) 
		{
			long rtt = mod_minus((uint16_t)now, timeEcho);
			__updata_rtt(rtt);
		}

		m_snd_wnd = urdp_header.get_window();
		uint32_t nAcked =mod_minus(ackno, m_snd_una);
		m_snd_una = ackno;
		m_slen -= nAcked;
		BOOST_ASSERT(mod_less_equal(m_snd_una, m_snd_nxt));
		if (m_snd_una == m_snd_nxt)
			m_t_rto_base=0;
		else if(!m_t_rto_base)
			m_t_rto_base=scape_zero(now);

		//printf("acked:------------------------------------------:%d\n", m_snd_una);

		while (!m_retrans_slist.empty())
		{
			long eraseLen=mod_minus(m_snd_una, m_retrans_slist.front().seq);
			BOOST_ASSERT(eraseLen>=0);
			if (eraseLen>0)
				m_retrans_slist.pop_front();
			else
				break;
		}
		if (m_retrans_slist.empty())
			m_t_rto_base=0;

		BOOST_ASSERT(m_retrans_slist.empty()||m_retrans_slist.front().seq==m_snd_una);

		//fast retrans & fast recover
		if (m_dup_acks >= 3) 
		{
			if (m_snd_una >= m_recover) 
			{ 
				//exit recovery。
				long nInFlight =mod_minus(m_snd_nxt, m_snd_una);
				BOOST_ASSERT(nInFlight>=0);
				m_cwnd=std::min(m_ssthresh, (uint32_t)nInFlight + m_mss);
				m_dup_acks = 0;
			} 
			else 
			{
				//recovery retransmit
				if (!__transmit(m_retrans_slist.begin(), now)) 
				{
					//std::cout << "recovery retransmit"<<m_retrans_slist.begin()->seq<<std::endl;;
					__allert_disconnected(asio::error::timed_out);
					return true;
				}
				m_cwnd += m_mss - std::min(nAcked, m_cwnd);
			}
		} 
		else 
		{
			// Slow start, congestion avoidance
			m_dup_acks = 0;
			if (m_cwnd < m_ssthresh)
				m_cwnd += m_mss;
			else
				m_cwnd += (uint32_t)std::max((uint64_t)1, (uint64_t)m_mss*(uint64_t)m_mss/m_cwnd);
		}

		if ((m_state == SYN_RCVD) && !bConnect) 
		{
			m_state = ESTABLISHED;
			notifyAccepet=true;
		}
	} 
	else if (ackno== m_snd_una) //!(m_snd_una<h.ackno<=m_snd_nxt)
	{
		// !?! Note, tcp says don't do this... but otherwise how does a closed window become open?
		m_snd_wnd =urdp_header.get_window();

		// Check duplicate acks
		if (rcvdDataLen > 0) 
		{
			// it's a dup ack, but with a data payload, so don't modify m_dup_acks
		} 
		else if (m_snd_una != m_snd_nxt) 
		{
			++m_dup_acks;
			//3 duplicate ACKs
			if (m_dup_acks == 3) 
			{ 
				uint32_t nInFlight =mod_minus(m_snd_nxt, m_snd_una);
				m_ssthresh = std::max((nInFlight*3)/4, 3*m_mss);//m_ssthresh = std::max(nInFlight / 2, 2 * m_mss);
				m_recover = m_snd_nxt;
				BOOST_ASSERT(!m_retrans_slist.empty());
				if (!__transmit(m_retrans_slist.begin(), now)) 
				{
					__allert_disconnected(asio::error::timed_out);
					return true;
				}
			} 
			else if (m_dup_acks > 3)
			{
				//(Fast Recover)
				m_cwnd += m_mss;
			}
		} 
		else //!(m_snd_una != m_snd_nxt) 
		{
			m_dup_acks = 0;
		}
	}

	// Conditions were acks must be sent:
	// 1) Segment is too old (they missed an ACK) (immediately)
	// 2) Segment is too new (we missed a segment) (immediately)
	// 3) Segment has data (so we need to ACK!) (delayed)
	// ... so the only time we don't need to ACK, is an empty segment that points to rcv_nxt!

	SendFlags sflags = sfNone;
	if (seqno != m_rcv_nxt||shouldImediateAck)
		sflags = sfImmediateAck; // (Fast Recovery)
	else if (rcvdDataLen != 0)
		sflags = sfDelayedAck;

	// Adjust the incoming segment to fit our receive buffer
	if (seqno < m_rcv_nxt) 
	{
		uint32_t nAdjust = m_rcv_nxt - seqno;
		if (nAdjust < rcvdDataLen) 
		{
			seqno += nAdjust;
			data += nAdjust;
			rcvdDataLen -= nAdjust;
		} 
		else 
		{
			rcvdDataLen = 0;
			seqno=m_rcv_nxt;
		}
	}

	if (!m_rlist.empty())
	{
		if (mod_minus(seqno+rcvdDataLen, m_rcv_nxt)>=RECV_BUF_SIZE)
			rcvdDataLen=0;
	}

	bool bIgnoreData = (urdp_header.get_control()!=CTRL_DATA) || (m_shutdown != SD_NONE);
	bool bNewData = false;

	if (rcvdDataLen > 0) 
	{
		if (bIgnoreData) 
		{
			if (seqno== m_rcv_nxt) 
				m_rcv_nxt += rcvdDataLen;
		} 
		else 
		{
			//uint32_t nOffset = seqno - m_rcv_nxt;
			//memcpy(m_rbuf + m_rlen + nOffset, seg.data, seg.len);
			//std::cout<<" seqno------:"<< seqno<<"   m_rcv_nxt:"<<m_rcv_nxt<<std::endl;
			RSegment rseg;
			rseg.seq=seqno;
			safe_buffer_io io(&(rseg.buf));
			io.write(data, rcvdDataLen);
			std::pair<RSegmentList::iterator, bool >insertRst=m_rlist.insert(rseg);

			/*		if (m_rlist.size()>15)
			{
			bool t=m_detect_readable;
			//std::cout<<"sd"<<std::endl;
			}*/

			if (insertRst.second)
			{
				if (m_rcv_wnd<rcvdDataLen)
					m_rcv_wnd=0;
				else
					m_rcv_wnd -= rcvdDataLen;
			}

			if (seqno == m_rcv_nxt) 
			{
				//BOOST_ASSERT(insertRst.second);
				//m_rlen += rcvdDataLen;
				//m_rcv_nxt += rcvdDataLen;
				//m_rcv_wnd -= rcvdDataLen;
				RSegmentList::iterator it = m_rlist.begin();
				while ((it != m_rlist.end()) && mod_less_equal(it->seq , m_rcv_nxt)) 
				{
					uint32_t seqEnd=it->seq + it->buf.size();
					if (mod_less(m_rcv_nxt, seqEnd))
					{
						bNewData = true;
						//sflags = sfImmediateAck; // (Fast Recovery)
						int nAdjust =mod_minus(seqEnd, m_rcv_nxt);
						m_rlen += nAdjust;
						m_rcv_nxt += nAdjust;
					}
					++it;
				}
			} 
		}
	}

	if (m_state==CLOSING&&ackno==m_snd_nxt)
	{
		BOOST_ASSERT(m_socket==NULL);
		m_t_close_base=now+WAIT_FIN_ACK_TIMEOUT;
		m_state=WATING_FIN_ACK;
		__packet_as_reliable_and_sendout(m_snd_nxt, CTRL_FIN, NULL, now);
		__schedule_timer(now);
		return true;
	}	

	__attempt_send(sflags);

	// If we have new data, notify the user
	if (bNewData && m_detect_readable) {
		//m_detect_readable = false;
		notifyReadable=true;
		/*	if (m_connection) {
		m_connection->on_readable(this);
		}*/
		//notify(evRead);
	}
	// If we make room in the _send queue, notify the user
	// The goal it to make sure we always have at least enough data to fill the
	// window. We'd like to notify the app when we are halfway to that point.
	const uint32_t kIdealRefillSize = (SND_BUF_SIZE + RECV_BUF_SIZE) / 2;
	if (m_detect_writable && (m_slen < kIdealRefillSize)) 
	{
		//m_detect_writable = false;
		notifyWritable=true;
		//if (m_connection)
		//	m_connection->on_writeable(this);
		//notify(evWrite);
	}

	if (notifyConnected)
	{
		__allert_connected(error_code());
	}
	if (notifyWritable)
	{
		__allert_writeable();
	}
	if (notifyReadable)
	{
		__allert_readable();
	}
	if (notifyAccepet)
	{
		__allert_accepted();
	}

	return true;
}

void urdp_flow::__allert_connected(const error_code & ec)
{
	m_establish_time=tick_now();
	m_t_close_base.reset();

	BOOST_ASSERT(b_active_);
	if(!m_socket||is_canceled_op(op_stamp()))
	{
		__close(true);
		return;
	}

	if (!ec)
	{
		if (resolver_)
			resolver_.reset();
		keep_async_receiving();
		m_socket->on_connected(ec);
	}
	else
	{
		m_socket->on_connected(ec);
		__to_closed_state();
	}
}

void urdp_flow::__allert_disconnected(const error_code&ec)
{
	if(!m_socket||is_canceled_op(op_stamp()))
	{
		__to_closed_state();
		return;
	}

	if (m_socket&&(m_dissconnect_reason&DISCONN_LOCAL)==0&&m_state!=CLOSED)
	{
		//error_code ec=(m_dissconnect_reason&DISCONN_ERROR)==DISCONN_ERROR?
		//	asio::error::connection_aborted : asio::error::connection_reset;
		if (m_state>=ESTABLISHED)
			m_socket->on_disconnected(ec);
		else if(b_active_)
			m_socket->on_connected(ec);
	}
	else
	{
	}
	__to_closed_state();
}

void urdp_flow::__allert_readable()
{
	if(!m_socket||is_canceled_op(op_stamp()))
	{
		__close(true);
		return;
	}

	if (m_detect_readable&&__can_let_transport_read())
	{
		m_detect_readable=false;
		if (b_keep_recving_)
			__async_receive(this->op_stamp());
	}
}

void urdp_flow::__allert_writeable()
{
	if (!m_socket||is_canceled_op(this->op_stamp()))
	{
		__close(true);
		return;
	}

	if (m_slen<SND_BUF_SIZE)
	{
		m_detect_writable=false;
		m_socket->on_writeable();
	}
}

void urdp_flow::__allert_accepted()
{
	if (is_canceled_op(this->op_stamp()))
	{
		__close(true);
		return;
	}

	m_establish_time=tick_now();
	m_t_close_base.reset();
	acceptor_sptr acc(m_acceptor.lock());
	if (acc)
	{
		keep_async_receiving();
		acc->accept_flow(SHARED_OBJ_FROM_THIS);
	}
	else
	{
		close();
	}
}

void urdp_flow::__allert_received(const safe_buffer& buf)
{
	if (!m_socket||is_canceled_op(this->op_stamp()))
	{
		__close(true);
		return;
	}

	m_socket->on_received(buf);
}

void urdp_flow::__updata_rtt(long rtt)
{
	if (rtt >= 0) 
	{
		rtt=std::max((long)20, rtt);
		if (m_srtt == 0) 
		{
			m_srtt = rtt;
			m_rttvar = rtt / 2;
		} 
		else 
		{
			m_rttvar = ((3 * m_rttvar + abs(long(rtt - m_srtt)))>>2);// (3 * m_rttvar + abs(long(rtt - m_srtt)))/ 4;
			m_srtt = ((7 * m_srtt + rtt)>>3); // (7 * m_srtt + rtt)/ 8;
		}
		m_rto = bound(MIN_RTO, m_srtt + std::max((time32_type)1, m_rttvar<<2), MAX_RTO);
#if 0
		//std::cout<<"cwnd: "<<m_cwnd 
		<< ", rtt: " << rtt
			<< ", srtt: " << m_rx_srtt
			<< ", rto: " << m_rx_rto<<std::endl;
#endif // _DEBUGMSG
	} 
	else 
	{
		//std::cout<<rtt<<std::endl;
		//BOOST_ASSERT(false);
	}
}


bool urdp_flow::__transmit(const SSegmentList::iterator& itr, time32_type now)
{
	BOOST_ASSERT(!m_slist.empty()||!m_retrans_slist.empty());

	SSegment& seg=*itr;

	if (seg.xmit >= ((m_state < ESTABLISHED)?16:8)
		&&mod_minus(now, m_t_lastrecv) >= 10000
		)
	{
		return false;
	}

	BOOST_ASSERT(seg.buf.size()<=m_mss);
	__packet_as_reliable_and_sendout(seg.seq, seg.ctrlType, &seg.buf, now);
	if (m_state<ESTABLISHED&&seg.xmit<=1)//try resend fast
		__packet_as_reliable_and_sendout(seg.seq, seg.ctrlType, &seg.buf, now);

	if (seg.xmit == 0) 
		m_snd_nxt += seg.buf.size();
	seg.xmit += 1;

	if (!m_t_rto_base)
	{
		//we have send a piece without ack, so we reset m_t_rto_base to now
		m_t_rto_base =scape_zero(now);
	}
	return true;
}

void urdp_flow::__attempt_send(SendFlags sflags) 
{
	time32_type now = tick_now();

	if (mod_minus(now, m_t_lastsend) > static_cast<long>(m_rto))
		m_cwnd =std::max(2*m_mss, m_cwnd/2);

	while (true)
	{
		uint32_t cwnd = m_cwnd;
		if ((m_dup_acks == 1) || (m_dup_acks == 2)) 
			cwnd += m_dup_acks * m_mss;// Limited Transmit
		uint32_t nWindow = std::min(m_snd_wnd, cwnd);
		uint32_t nInFlight =mod_minus(m_snd_nxt, m_snd_una);
		uint32_t nUseable = (nInFlight < nWindow) ? (nWindow - nInFlight) : 0;

		uint32_t nAvailable = std::min(m_slen - nInFlight, m_mss);

		if (nAvailable > nUseable) 
		{
			if (nUseable * 4 < nWindow&&nUseable<m_mss)
				nAvailable = 0;// RFC 813 - avoid SWS
			else 
				nAvailable = nUseable;
		}

		bool immediateAck=(nAvailable == 0||mod_less(m_snd_una, m_snd_nxt) && (nAvailable < m_mss));
		if (m_slist.empty()) 
		{
			// If this is an immediate ack, or the second delayed ack
			if ((sflags == sfImmediateAck) || immediateAck&&m_t_ack) 
				__packet_as_reliable_and_sendout(m_snd_nxt, CTRL_ACK, NULL, now);
			else if(sflags != sfNone)
				m_t_ack = scape_zero(now);
			return;    
		}

		// Find the next segment to transmit
		BOOST_ASSERT(!m_slist.empty());
		if (!__transmit(m_slist.begin(), now)) 
			return;// TODO: consider closing socket
		BOOST_ASSERT(!m_slist.empty());
		m_retrans_slist.push_back(m_slist.front());
		m_slist.pop_front();
		sflags = sfNone;
	}
}

void urdp_flow::__incress_rto()
{
	// Back off retransmit timer. Note: the limit is lower when connecting.
	if(m_state < ESTABLISHED)
	{
		time32_type rto_limit=time32_type(MIN_RTO*random(2.0, 3.0));
		m_rto += std::min(rto_limit, m_rto);
	}
	else
	{
		m_rto<<=1;
		if (m_srtt>0)
		{//make it more aggresive than TCP 
			int maxRto=(m_srtt<<4);
			if (m_rto>maxRto&&m_rto>MIN_RTO)
				m_rto=maxRto;
		}
	}
	m_rto=bound(MIN_RTO, m_rto, MAX_RTO);
}


double urdp_flow::__calc_remote_to_local_lost_rate(time32_type now, 
												   wrappable_integer<int8_t>* id) const
{
	if (m_state!=ESTABLISHED)
		return 1.0;
	bool calc=false;
	double old_remote_to_local_lost_rate=remote_to_local_lost_rate_;
	wrappable_greater_equal<seqno_mark_type::int_type> MARK_GREATER_EQUAL;
	wrappable_minus<seqno_mark_type::int_type> MARK_MINUS;
	seqno_mark_map& qm=recvd_seqno_mark_for_lost_rate_;
	BOOST_AUTO(itr, qm.begin());
	BOOST_AUTO(end, qm.end());
	BOOST_AUTO(rbg, qm.rbegin());
	while((itr=qm.begin())!=end
		&&(
		qm.size()>=(std::size_t)(seqno_mark_type::max_distance()-1)
		||mod_less<time32_type>(itr->second+3000, now)//3000ms
		||((int)qm.size()>1&&MARK_MINUS(rbg->first, itr->first)<=0)
		||(id&&MARK_MINUS(*id, itr->first)<0)//mark_seq is ordered, for reason of TCP
		||(id&&MARK_GREATER_EQUAL(itr->first, *id)&&MARK_GREATER_EQUAL(*id, rbg->first))
		)
		)//wrappable_integer<int8_t> The phenomenon: A>B B>C but A<C may appear here, we should avoid it.
	{
		qm.erase(itr);
		calc=true;
	}
	if (id)
	{
		if (qm.empty())
			qm.insert(std::make_pair(*id, now));
		else
			qm.insert(--qm.end(), std::make_pair(*id, now));
		calc=true;
	}
	if (calc)
	{
		if (!qm.empty())
		{
			BOOST_ASSERT(MARK_MINUS(qm.rbegin()->first, qm.begin()->first)>=0);
			double cnt=MARK_MINUS(qm.rbegin()->first, qm.begin()->first)+1;
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

	if (qm.empty())
		remote_to_local_lost_rate_/=2;
	if (remote_to_local_lost_rate_<0)
		remote_to_local_lost_rate_=0.0;
	if (remote_to_local_lost_rate_>1.0)
		remote_to_local_lost_rate_=1.0;

	if (calc||old_remote_to_local_lost_rate!=remote_to_local_lost_rate_)
	{
		double a=std::max(200.0, in_speed_meter_.bytes_per_second())/(global_remote_to_local_speed_meter().bytes_per_second()+1.0);
		if (a>1.0) a=1.0;
		if (a<0.0) a=0.0;
		double oldRate=global_remote_to_local_lost_rate();
		double newRate=(oldRate*(1.0-a)+remote_to_local_lost_rate_*a);
		set_global_remote_to_local_lost_rate(oldRate*0.125+newRate*0.875);
	}

	return remote_to_local_lost_rate_;
}

NAMESPACE_END(urdp);
NAMESPACE_END(p2engine);
