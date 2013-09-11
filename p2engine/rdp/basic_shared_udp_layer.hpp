//
// basic_shared_udp_layer.hpp
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

#ifndef BASIC_RUDP_UDP_LAYER_H__
#define BASIC_RUDP_UDP_LAYER_H__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "p2engine/push_warning_option.hpp"
#include "p2engine/config.hpp"
#include <queue>
#include <vector>
#include <list>
#include <boost/noncopyable.hpp>
#include <boost/unordered_map.hpp>
#include "p2engine/pop_warning_option.hpp"

#include "p2engine/handler_allocator.hpp"
#include "p2engine/basic_engine_object.hpp"
#include "p2engine/socket_utility.hpp"
#include "p2engine/safe_buffer.hpp"
#include "p2engine/logging.hpp"
#include "p2engine/local_id_allocator.hpp"
#include "p2engine/keeper.hpp"
#include "p2engine/speed_meter.hpp"
#include "p2engine/trafic_statistics.hpp"
#include "p2engine/rdp/const_define.hpp"
#include "p2engine/fast_stl.hpp"

#define RUDP_SCRAMBLE

namespace p2engine { namespace urdp{

	class basic_shared_udp_layer 
		: public basic_engine_object
	{
		typedef basic_shared_udp_layer this_type;
		SHARED_ACCESS_DECLARE;

		BOOST_STATIC_CONSTANT(std::size_t, mtu_size=MTU_SIZE+128);

	public:
		typedef asio::ip::udp::endpoint endpoint_type;
		typedef asio::ip::udp::socket	udp_socket_type;

		typedef shared_ptr	  shared_layer_sptr;

		typedef boost::function<void(safe_buffer&,const endpoint&)> 
			recvd_data_handler_type;
		typedef boost::function<int(const endpoint&)> 
			recvd_request_handler_type;

		struct flow_element
			:object_allocator
		{
			recvd_data_handler_type handler;
			void* flow;

			flow_element()
				:flow(NULL){}
			flow_element(const recvd_data_handler_type& h,void* f)
				:handler(h),flow(f){}

			void reset()
			{
				flow=NULL;
				handler=recvd_data_handler_type();
			}
		};
		struct acceptor_element 
			:public object_allocator
		{
			recvd_request_handler_type handler;
			void* acceptor;

			acceptor_element():acceptor(NULL){}
		};

		typedef boost::unordered_map<std::string,acceptor_element> acceptor_container;
		typedef std::vector<flow_element>			flow_container;
		typedef boost::unordered_map<int,std::list<safe_buffer> >  linger_send_container;
		typedef std::map<endpoint_type, this_type*>			this_type_container;

		typedef basic_local_id_allocator<uint32_t> local_id_allocator;

	public:
		enum{INIT,STARTED,STOPED};

		struct flow_token:basic_object,basic_intrusive_ptr<flow_token>
		{
			typedef flow_token this_type;
			typedef boost::intrusive_ptr<this_type> smart_ptr;

			shared_layer_sptr shared_layer;
			const uint32_t  flow_id;
			void* flow;

			flow_token(uint32_t flowID,const shared_layer_sptr& udpLayer, void* flow)
				:shared_layer(udpLayer),flow_id(flowID),flow(flow)
			{
				set_obj_desc("basic_shared_udp_layer::flow_token");
			}
			~flow_token()
			{
				shared_layer->unregister_flow(flow_id,flow);
			}
		};

		struct acceptor_token:public basic_object
		{
			typedef acceptor_token this_type;
			typedef boost::shared_ptr<this_type> shared_ptr;

			shared_layer_sptr shared_layer;
			const std::string domain;
			void* acceptor;

			acceptor_token(const std::string& domainName,
				shared_layer_sptr udpLayer,void*acc)
				:shared_layer(udpLayer),domain(domainName),acceptor(acc)
			{
				set_obj_desc("basic_shared_udp_layer::acceptor_token");
			}
			~acceptor_token()
			{
				shared_layer->unregister_acceptor(acceptor);
			}
		};
	
	public:
		//called by urdp to connect a remote endpoint
		static  flow_token::smart_ptr create_flow_token(
			io_service& ios,
			const endpoint_type& local_edp,
			void* flow,
			const recvd_data_handler_type& handler,
			error_code& ec
			);

		//called by flow,when listened a passive connect request
		static  flow_token::smart_ptr create_flow_token(
			shared_layer_sptr udplayer,
			void* flow,//the flow that has been created when received SYN
			const recvd_data_handler_type& handler,
			error_code& ec
			);

		//called by acceptor to listen at a local endpoint
		static  acceptor_token::shared_ptr create_acceptor_token(
			io_service& ios,
			const endpoint_type& local_edp,
			void* acceptor,
			const recvd_request_handler_type& handler,
			const std::string domainName,
			error_code& ec
			);

	protected:
		static shared_ptr create(io_service& ios, 
			const endpoint_type& local_edp, error_code& ec
			);

		static bool is_shared_endpoint(const endpoint_type& endpoint);

	public:
		virtual ~basic_shared_udp_layer();

		udp_socket_type& socket()
		{
			return socket_;
		}

		bool is_open()const
		{
			return socket_.is_open();
		}

		shared_ptr shared_ptr_from_this() {return SHARED_OBJ_FROM_THIS;}

		endpoint_type local_endpoint( error_code&ec)const
		{
			return socket_.local_endpoint(ec);
		}

		int flow_count()const
		{
			return flows_cnt_;
		}

	protected:
		basic_shared_udp_layer(io_service& ios, const endpoint_type& local_edp,
			error_code& ec);

		void start();

		void cancel()
		{
			OBJ_PROTECTOR(protector);
			close_without_protector();
		}

		void close_without_protector()
		{
			error_code ec;
			socket_.close(ec);
			state_=STOPED;
		}

		void handle_receive(const error_code& ec, std::size_t bytes_transferred);
		void async_receive();

	protected:
		error_code register_acceptor(const void* acc,
			const std::string& domainName,
			const recvd_request_handler_type& callBack,
			error_code& ec
			);

		void register_flow(const void* flow, 
			const recvd_data_handler_type& callBack,
			uint32_t& id, error_code& ec);

		void unregister_flow(uint32_t flow_id,void* flow);
		void  unregister_acceptor(const void*acptor);

	public:
		static double out_bytes_per_second()
		{
			return global_local_to_remote_speed_meter().bytes_per_second();
		}
		static double in_bytes_per_second()
		{
			return global_remote_to_local_speed_meter().bytes_per_second();
		}

		//std::size_t async_send_to(void const* p, std::size_t len,
		//	const endpoint_type& ep, error_code& ec)
		//{
		//	
		//}

		std::size_t async_send_to(const safe_buffer& safebuffer,
			const endpoint_type& ep,error_code& ec);

		template <typename ConstBuffers>
		std::size_t async_send_to(const ConstBuffers& bufs,
			const endpoint_type& ep,error_code& ec);

	protected:
		void __release_flow_id(int id);

		void do_handle_received(const safe_buffer& buffer);
		void do_handle_received_urdp_msg(safe_buffer& buffer);

	protected:
		struct request_uuid{
			endpoint_type remoteEndpoint;
			uint32_t remotePeerID;
			uint32_t session;
			uint32_t flow_id;
			bool operator <(const request_uuid& rhs)const
			{
				if (session!=rhs.session)
					return session<rhs.session;
				if (remotePeerID!=rhs.remotePeerID)
					return remotePeerID<rhs.remotePeerID;
				return remoteEndpoint<rhs.remoteEndpoint;
			}
		};

		udp_socket_type socket_;
		endpoint_type local_endpoint_;
		safe_buffer recv_buffer_;
		endpoint_type sender_endpoint_;
		timed_keeper_set<request_uuid> request_uuid_keeper_;
		local_id_allocator id_allocator_;
		std::list<int> released_id_catch_;
		timed_keeper_set<int> released_id_keeper_;
		timed_keeper_set<endpoint_type> unreachable_endpoint_keeper_;
		acceptor_container	  acceptors_;
		flow_container        flows_;
		int                   flows_cnt_;
		linger_send_container lingerSends_;
		fast_mutex flow_mutex_;
		fast_mutex acceptor_mutex_;
		//rough_timer_shared_ptr lingerSendTimer_;
		int state_;
		int	continuous_recv_cnt_;

		//recv handler(for performance)
		
		typedef handler_allocator_wrap<
			boost::function<void(const error_code&,size_t)> 
		>::wrap_type allocator_wrap_handler;

		boost::scoped_ptr<allocator_wrap_handler> recv_handler_;

		static this_type_container s_shared_this_type_pool_;
		static fast_mutex s_shared_this_type_pool_mutex_;
		static allocator_wrap_handler s_dummy_callback;

#ifdef RUDP_SCRAMBLE
		safe_buffer zero_8_bytes_;
#endif
	};

	template <typename ConstBuffers>
	inline std::size_t basic_shared_udp_layer::async_send_to(
		const ConstBuffers& bufs,const endpoint_type& ep,error_code& ec
		)
	{
#ifdef RUDP_SCRAMBLE
		std::size_t len=0;
		std::vector<asio::const_buffer>sndbufs;
		sndbufs.reserve(bufs.size()+1);
		sndbufs.push_back(zero_8_bytes_.to_asio_const_buffer());
		to_asio_mutable_buffers(bufs,sndbufs);
		BOOST_ASSERT(sndbufs.size()==bufs.size()+1);
		BOOST_FOREACH(const asio::const_buffer& buf,sndbufs)
		{
			len+=p2engine::buffer_size(buf);
		}
		if(len==zero_8_bytes_.size())
			return 0;
		global_local_to_remote_speed_meter()+=len;
		socket_.async_send_to(sndbufs,ep,s_dummy_callback);

		/*
		socket_.send_to(sndbufs,ep,0,ec);
		if (ec)
		{
			if (ec == boost::asio::error::would_block
				||ec == boost::asio::error::try_again
				)
			{
				socket_.async_send_to(sndbufs,ep,s_dummy_callback);
				ec.clear();
			}
		}
		*/
		return len-zero_8_bytes_.size();
#else
		std::size_t len=0;
		std::vector<asio::const_buffer>sndbufs;
		sndbufs.reserve(bufs.size());
		to_asio_mutable_buffers(bufs,sndbufs);
		BOOST_ASSERT(sndbufs.size()==bufs.size());
		BOOST_FOREACH(const asio::const_buffer& buf,sndbufs)
		{
			len+=p2engine::buffer_size(buf);
		}
		if(len==0)
			return 0;
		global_local_to_remote_speed_meter()+=len;
		socket_.async_send_to(sndbufs,ep,s_dummy_callback);

		/*
		socket_.send_to(sndbufs,ep,0,ec);
		if (ec)
		{
			if (ec == boost::asio::error::would_block
				||ec == boost::asio::error::try_again
				)
			{
				ec.clear();
				socket_.async_send_to(sndbufs,ep,s_dummy_callback);
			}
		}
		*/
		return len;
#endif
	}


}
}// namespace p2engine

#endif//BASIC_RUDP_UDP_LAYER_H__
