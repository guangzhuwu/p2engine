//
// urdp_flow.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2009, GuangZhu Wu  <guangzhuwu@gmail.com>
//
//This program is free software; you can redistribute it and/or modify it 
//under the terms of the GNU General Public License or any later version.
//
//This program is distributed in the hope that it will be useful, but 
//WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
//or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License 
//for more details.
//
//You should have received a copy of the GNU General Public License along 
//with this program; if not, contact <guangzhuwu@gmail.com>.
//

#ifndef P2ENGINE_BASIC_SIMPLE_RUDP_FLOW_H
#define P2ENGINE_BASIC_SIMPLE_RUDP_FLOW_H

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "p2engine/push_warning_option.hpp"
#include "p2engine/config.hpp"
#include <set>
#include <boost/logic/tribool.hpp>
#include <boost/optional.hpp>
#include "p2engine/pop_warning_option.hpp"

#include "p2engine/fast_stl.hpp"
#include "p2engine/packet.hpp"
#include "p2engine/keeper.hpp"
#include "p2engine/timer.hpp"
#include "p2engine/speed_meter.hpp"
#include "p2engine/operation_mark.hpp"
#include "p2engine/shared_access.hpp"
#include "p2engine/wrappable_integer.hpp"

#include "p2engine/rdp/rdp_fwd.hpp"
#include "p2engine/rdp/const_define.hpp"
#include "p2engine/rdp/basic_shared_udp_layer.hpp"

namespace p2engine { namespace urdp{

	class urdp_packet_basic_format;
	class urdp_packet_unreliable_format;
	class urdp_packet_reliable_format;

	class urdp_flow
		: public basic_engine_object
		, public basic_flow_adaptor
		, public operation_mark
		, public fssignal::trackable
	{
		typedef urdp_flow this_type;
		SHARED_ACCESS_DECLARE;

	public:
		typedef uint16_t message_type;
		typedef basic_connection_adaptor connection_type;
		typedef urdp_packet_basic_format packet_format_type;
		typedef urdp_packet_unreliable_format unreliable_packet_format_type;
		typedef urdp_packet_reliable_format reliable_packet_format_type;
		typedef basic_acceptor_adaptor acceptor_type;

		typedef basic_shared_udp_layer shared_layer_type;
		typedef shared_layer_type::flow_token flow_token_type;

		typedef rough_timer timer_type;
		typedef int32_t     time32_type;

		typedef boost::shared_ptr<shared_layer_type> shared_layer_sptr;
		typedef flow_token_type::smart_ptr flow_token_sptr;
		typedef boost::shared_ptr<acceptor_type> acceptor_sptr;
		typedef boost::shared_ptr<connection_type>  connection_sptr;
		typedef boost::shared_ptr<timer_type> timer_sptr;

		typedef asio::ip::udp::resolver resolver_type;
		typedef resolver_type::iterator resolver_iterator;
		typedef resolver_type::query	resolver_query;

		friend class basic_connection_adaptor;
		friend class basic_acceptor_adaptor;

		enum ShutdownMode{SD_NONE, SD_GRACEFUL, SD_FORCEFUL};
		enum TcpState {INIT, LISTEN, SYN_SENT, SYN_RCVD, ESTABLISHED, CLOSING,
			WATING_FIN_ACK, CLOSED};
		enum SendFlags { sfNone, sfDelayedAck, sfImmediateAck};
		enum {RECV_BUF_SIZE = 0xffff+16*1024,SND_BUF_SIZE =RECV_BUF_SIZE*3/2};

	public:
		static shared_ptr create_for_active_connect(connection_sptr sock,
			io_service& ios, const endpoint_type& local_edp, error_code& ec
			);

		static shared_ptr create_for_passive_connect(io_service& ios,
			acceptor_sptr acceptor,	shared_layer_sptr sharedLayer,
			const endpoint_type& remote_edp, error_code& ec
			);

	protected:
		urdp_flow(io_service&);
		virtual ~urdp_flow();
		void __init();
		void called_by_sharedlayer_on_recvd(const safe_buffer& buf, const endpoint_type& from);

	public:
		bool is_open()const
		{
			if (m_token)
				return m_token->shared_layer->is_open();
			return false;
		}
		virtual bool is_connected()const
		{
			return is_open()&&m_state==ESTABLISHED;
		}
		virtual void set_socket(connection_sptr sock)
		{
			m_socket=sock.get();
		}
		virtual uint32_t flow_id()const;
		virtual void close(bool graceful=true);
		int session_id()const{return m_session_id;}
		TcpState state() const { return m_state; }
		void async_connect(const endpoint_type& remoteEnp,const std::string&domainName,
			const time_duration& time_out=boost::date_time::pos_infin
			);
		void async_connect(const std::string& remote_host, int port, 
			const std::string&domainName, 
			const time_duration& time_out=boost::date_time::pos_infin
			);

		void async_send(const safe_buffer& buf, message_type msgType
			,boost::logic::tribool reliable);

		void keep_async_receiving();
		void block_async_receiving()
		{
			b_keep_recving_=false;
		}

		void ping_interval(const time_duration& t);
		time_duration ping_interval()const
		{
			return milliseconds(m_ping_interval);
		}
		void ping(error_code& ec);

		endpoint_type local_endpoint(error_code& ec)const;
		endpoint_type remote_endpoint(error_code& ec)const;

		time_duration rtt() const
		{
			return milliseconds(m_srtt);
		}
		double alive_probability()const;
		double local_to_remote_speed()const
		{
			return out_speed_meter_.bytes_per_second();
		}
		double remote_to_local_speed()const
		{
			return in_speed_meter_.bytes_per_second();
		}

		double local_to_remote_lost_rate() const
		{
			return local_to_remote_lost_rate_;
		}
		double remote_to_local_lost_rate() const
		{
			return __calc_remote_to_local_lost_rate(tick_now());
		}

		//FOR NAT PUNCH
		//  If A want to connect B which is behind NAT, A must use make_punch_packet 
		//tocreate a punch packet before connect B. Then A send the punch packet to 
		//relayserver which should relay the punch packet to B.
		//  When B recvd the punch packet relayed by relayserver, B will send the
		//punch packet to A for twice or more.
		virtual safe_buffer make_punch_packet(error_code& ec,const endpoint& externalEdp);
		virtual void on_received_punch_request(const safe_buffer& buf);

	private:
		template<typename IntType>
		static bool mod_less(IntType n1, IntType n2)
		{
			static wrappable_less<IntType> comp;
			return comp(n1,n2);
		}
		template<typename IntType>
		static bool mod_less_equal(IntType n1, IntType n2)
		{
			static wrappable_less_equal<IntType> comp;
			return comp(n1,n2);
		}
		template<typename IntType>
		static long mod_minus(IntType n1, IntType n2)
		{
			static wrappable_minus<IntType> comp;
			return (long)comp(n1,n2);
		}
		static time32_type tick_now()
		{
			return (time32_type)system_time::tick_count();
		}

		struct SSegment:object_allocator {
			safe_buffer buf;
			uint32_t seq;
			int8_t xmit;
			uint8_t ctrlType;
			SSegment(uint32_t s,uint8_t c) 
				: seq(s),xmit(0), ctrlType(c) 
			{ }
		};

		struct RSegment:object_allocator {
			safe_buffer buf;
			uint32_t seq;
			bool operator<(RSegment const& rhs)const
			{
				return mod_less(seq,rhs.seq);
			}
		};

		struct SUnraliableSegment:object_allocator {
			safe_buffer buf;
			int32_t id;
			time32_type timeout;
			uint16_t pktID;
			uint16_t msgType;
			int8_t remainXmit;
			uint8_t control;

			SUnraliableSegment(){
				static int32_t id_seed__=0;
				id=id_seed__++;
			}

			bool operator <(const SUnraliableSegment& rhs)const{
				if (timeout!=rhs.timeout)
					return mod_less(timeout,rhs.timeout);
				return mod_less(id,rhs.id);
			}
		};

		typedef std::deque<SSegment> SSegmentList;
		typedef std::set<RSegment> RSegmentList;
		typedef std::deque<safe_buffer> RUnraliablePacketList;
		typedef std::set<SUnraliableSegment> SUnraliableSegmentList;

		void __async_receive(op_stamp_t mark);
		int  __recv(safe_buffer& buf,error_code& ec);
		int  __send(safe_buffer buf,uint16_t msgType, boost::logic::tribool reliable,
			error_code& ec);
		void __close(bool graceful);

		int __packet_as_reliable_and_sendout(uint32_t seq, uint8_t control,
			const safe_buffer* data, time32_type now);
		int __packet_as_unreliable_and_sendout(SUnraliableSegment& seg, time32_type now); 
		uint32_t __queue(const char * data, std::size_t len, uint8_t ctrlType,
			std::size_t reserveLen=0);
		bool __transmit(const SSegmentList::iterator& seg, time32_type now);

		bool __process(const safe_buffer& buf,const endpoint_type& from);

		void __attempt_send(SendFlags sflags = sfNone);

		void __to_closed_state();

		bool __clock_check(time32_type now, long& nTimeout);
		void __on_clock(VC9_BIND_BUG_PARAM_DECLARE);

		void __adjust_mtu();
		void __incress_rto();
		void __updata_rtt(long msec);

		void __schedule_timer(time32_type now,bool calledInOnClock=false);

		//判断是否收到了完整的packet，-1：错误发生，0：would block；>0：可读
		int __can_let_transport_read();

		void __allert_connected(const error_code&);
		void __allert_disconnected(const error_code&);
		void __allert_received(const safe_buffer& buf);
		void __allert_readable();
		void __allert_writeable();
		void __allert_accepted();

		void __calculate_local_to_remote_lost_rate(
			const packet<unreliable_packet_format_type>&,time32_type now);
		double __calc_remote_to_local_lost_rate(time32_type now,
			wrappable_integer<int8_t>* id=NULL) const;

		void  __handle_resolve(error_code ec, resolver_iterator itr,
			const std::string& domainName,  const time_duration& time_out,
			op_stamp_t mark);

	protected:
		//有关unreliable
		RUnraliablePacketList m_unreliable_rlist;
		SUnraliableSegmentList m_unreliable_slist;
		timed_keeper_set<uint16_t> m_unreliable_rkeeper;

		uint16_t m_unreliable_pktid;
		//host& m_host;
		ShutdownMode m_shutdown;

		time32_type m_establish_time;

		// TCB data
		uint16_t m_session_id;
		bool m_detect_readable:1;
		bool m_detect_writable:1;

		// Incoming data
		RSegmentList m_rlist;
		uint32_t m_rcv_nxt, m_rcv_wnd, m_rlen;

		// Outgoing data
		SSegmentList m_slist,m_retrans_slist;
		uint32_t m_snd_nxt, m_snd_wnd, m_slen,m_snd_una;
		uint32_t m_mss;
		time32_type m_t_rto_base;

		// Timestamp tracking
		uint16_t m_t_recent;
		uint16_t m_t_recent_now;
		time32_type m_t_lastrecv,m_t_lastsend,m_t_lasttraffic;
		time32_type m_t_last_on_clock;

		// Round-trip calculation
		time32_type m_rttvar, m_srtt, m_rto;
		uint32_t m_lastack;

		// Congestion avoidance, Fast retransmit/recovery, Delayed ACKs
		uint32_t m_ssthresh, m_cwnd;
		uint32_t m_recover;
		time32_type m_t_ack;
		uint8_t  m_dup_acks;

		uint32_t m_remote_peer_id;
		time32_type m_ping_interval;

	protected:
		shared_ptr m_self_holder;
		endpoint_type m_remote_endpoint;
		TcpState m_state;
		timer_sptr m_timer;
		connection_type*    m_socket;
		flow_token_sptr m_token;
		boost::weak_ptr<acceptor_type> m_acceptor;

		int m_dissconnect_reason;

		boost::optional<time32_type> m_t_close_base;

		bool b_ignore_all_pkt_:1;
		bool b_keep_recving_:1;
		bool b_active_:1;
		bool b_timer_posted_:1;
		bool b_close_called_:1;

	protected:
		rough_speed_meter in_speed_meter_;
		rough_speed_meter out_speed_meter_;
		mutable double remote_to_local_lost_rate_;
		mutable double local_to_remote_lost_rate_;
		typedef wrappable_integer<int8_t> seqno_mark_type;
		typedef std::map<seqno_mark_type,time32_type> seqno_mark_map;
		mutable seqno_mark_map recvd_seqno_mark_for_lost_rate_;
		int8_t id_for_lost_rate_;

		boost::scoped_ptr<resolver_type> resolver_;
	};

}//namespace urdp
}//namespace p2engine

#endif//basic_urdp_flow_h__

