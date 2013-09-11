//
// urdp_connection.hpp
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

#ifndef p2engine_urdp_socket_h__
#define p2engine_urdp_socket_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "p2engine/push_warning_option.hpp"
#include "p2engine/config.hpp"
#include <list>
#include <string>
#include "p2engine/pop_warning_option.hpp"

#include "p2engine/typedef.hpp"
#include "p2engine/shared_access.hpp"
#include "p2engine/fssignal.hpp"
#include "p2engine/safe_buffer.hpp"
#include "p2engine/operation_mark.hpp"
#include "p2engine/connection.hpp"
#include "p2engine/rdp/rdp_fwd.hpp"
#include "p2engine/rdp/urdp_flow.hpp"

namespace p2engine{ namespace urdp{

	template<typename BaseConnectionType>
	class basic_urdp_connection
		: public BaseConnectionType
		, public basic_connection_adaptor
		, public operation_mark
	{
		typedef basic_urdp_connection<BaseConnectionType> this_type;
		SHARED_ACCESS_DECLARE;

		BOOST_STATIC_ASSERT((boost::is_same<BaseConnectionType,basic_connection>::value)
			||(boost::is_base_and_derived<basic_connection,BaseConnectionType>::value));

		enum{OPENED,CLOSED};

	public:
		typedef BaseConnectionType connection_base_type;
		typedef urdp_flow flow_type;
		typedef this_type socket_type;
		typedef urdp_packet_basic_format packet_format_type;
		typedef basic_acceptor_adaptor acceptor_type;

		typedef typename urdp_flow::shared_layer_type shared_layer_type;
		typedef typename urdp_flow::flow_token_type flow_token_type;

		typedef rough_timer timer_type;

		typedef boost::shared_ptr<shared_layer_type> shared_layer_sptr;
		typedef boost::shared_ptr<socket_type> connection_sptr;
		typedef boost::shared_ptr<timer_type> timer_sptr;

		typedef typename BaseConnectionType::connection_t connection_t;

	protected:
		friend class urdp_flow;
		template<typename Socket,typename SocketBase> friend class basic_urdp_acceptor;

	protected:
		basic_urdp_connection(io_service& ios,bool realTimeUsage, bool passive)
			: BaseConnectionType(ios,realTimeUsage,passive)
		{
			this->set_obj_desc("urdp_connection");
			init(passive);
		}

		virtual ~basic_urdp_connection()
		{
			__close(true);
		}

		void init(bool passive)
		{
			next_op_stamp();
			if (!passive)
			{
				state_=CLOSED;
				this->is_passive_=false;
			}
			else
			{
				state_=OPENED;
				this->is_passive_=true;
			}
		}

		void set_flow(flow_sptr flow)
		{
			BOOST_ASSERT(!flow_);
			BOOST_ASSERT(this->is_passive_);
			BOOST_ASSERT(boost::dynamic_pointer_cast<flow_type>(flow));
			error_code ec;
			cached_remote_endpoint_=flow->remote_endpoint(ec);
			flow_=boost::static_pointer_cast<flow_type>(flow);
		}

	public:
		static shared_ptr create(io_service& ios,bool realTimeUsage,
			bool passive=false)
		{
			return shared_ptr(new this_type(ios,realTimeUsage,passive),
				shared_access_destroy<this_type>());
		}

	public:
		virtual error_code open(const endpoint& local_edp, error_code& ec,
			const proxy_settings& ps=proxy_settings()
			)
		{
			set_cancel();
			next_op_stamp();
			if (flow_)
				ec=asio::error::already_open;
			else
			{
				flow_=flow_type::create_for_active_connect(SHARED_OBJ_FROM_THIS,
					this->get_io_service(),local_edp,ec);
			}
			return ec;
		}

		virtual void async_connect(const std::string& remote_host, int port, 
			const std::string& domainName,
			const time_duration& time_out=boost::date_time::pos_infin
			)
		{
			set_cancel();
			next_op_stamp();
			error_code ec;
			if (!flow_)open(endpoint(),ec);
			flow_->async_connect(remote_host,port,domainName,time_out);
		}
		virtual void  async_connect(const endpoint& peer_endpoint,
			const std::string& domainName,
			const time_duration& time_out=boost::date_time::pos_infin
			)
		{
			set_cancel();
			next_op_stamp();
			error_code ec;
			if (!flow_)
				open(endpoint(),ec);
			flow_->async_connect(peer_endpoint,domainName,time_out);
		}
		//reliable send
		virtual void async_send_reliable(const safe_buffer& buf, message_type msgType)
		{
			if (flow_)
				flow_->async_send(buf,msgType,true);
		}
		//unreliable send
		virtual void async_send_unreliable(const safe_buffer& buf, message_type msgType)
		{
			if (flow_)
				flow_->async_send(buf,msgType,false);
		}
		//partial reliable send. message will be send for twice within about 100ms.
		//reliable is not confirmed.
		virtual void async_send_semireliable(const safe_buffer& buf, message_type msgType)
		{
			if (flow_)
				flow_->async_send(buf,msgType,boost::indeterminate);
		}

		virtual void keep_async_receiving()
		{
			if (flow_)
				flow_->keep_async_receiving();
		}
		virtual void block_async_receiving()
		{
			if (flow_)
				flow_->block_async_receiving();
		}

		virtual void close(bool greaceful=true)
		{
			__close(greaceful);
		}

		virtual bool is_open() const
		{
			if (flow_) return flow_->is_open(); 
			return false;
		}
		virtual bool is_connected() const
		{
			if (flow_) return flow_->is_connected(); 
			return false;
		}

		virtual void ping_interval(const time_duration& t)
		{
			if (flow_) flow_->ping_interval(t);
		}

		virtual time_duration ping_interval()const
		{
			if (flow_)
				return flow_->ping_interval();
			return seconds(0xffffffffUL);
		}

		virtual endpoint local_endpoint(error_code& ec)const
		{
			if (flow_)
				return flow_->local_endpoint(ec);
			else
			{
				ec=asio::error::not_socket;
				return endpoint();
			}
		}

		virtual endpoint remote_endpoint(error_code& ec)const
		{
			if (flow_)
			{
				return flow_->remote_endpoint(ec);
			}
			else
			{
				ec=asio::error::not_socket;
				return cached_remote_endpoint_;
			}
		}

		virtual connection_t connection_category()const
		{
			return basic_connection::UDP;
		}

		virtual const std::string& domain()const
		{
			return domain_;
		}

		virtual time_duration rtt() const
		{
			if (flow_)
				return flow_->rtt();
			return seconds(0x7fffffff);
		}
		virtual double alive_probability()const
		{
			if (flow_)
				return flow_->alive_probability();
			return 0.0;
		}
		virtual double local_to_remote_speed()const
		{
			if (flow_)
				return flow_->local_to_remote_speed();
			return 0.0;
		}
		virtual double remote_to_local_speed()const
		{
			if (flow_)
				return flow_->remote_to_local_speed();
			return 0.0;
		}
		virtual double local_to_remote_lost_rate()  const
		{
			if (flow_)
				return flow_->local_to_remote_lost_rate();
			return 1.0;
		}
		virtual double remote_to_local_lost_rate() const
		{
			if (flow_)
				return flow_->remote_to_local_lost_rate();
			return 1.0;
		}

		virtual safe_buffer make_punch_packet(error_code& ec,const endpoint& externalEdp)
		{
			ec.clear();
			if (!flow_)
			{
				boost::shared_ptr<flow_type> flow_s=flow_type::create_for_active_connect(
					SHARED_OBJ_FROM_THIS,this->get_io_service(),cached_remote_endpoint_,ec);
				safe_buffer rst=flow_s->make_punch_packet(ec,externalEdp);
				flow_s->close(false);
				return rst;
			}
			return flow_->make_punch_packet(ec,externalEdp);
		}

		virtual void on_received_punch_request(safe_buffer& buf)
		{
			error_code ec;
			if (!flow_)
			{
				boost::shared_ptr<flow_type> flow_s=flow_type::create_for_active_connect(
					SHARED_OBJ_FROM_THIS,this->get_io_service(),cached_remote_endpoint_,ec);
				flow_s->on_received_punch_request(buf);
				flow_s->close(false);
				return;
			}
			if(!ec)
				flow_->on_received_punch_request(buf);
		}

		virtual void ping(error_code& ec)
		{
			if (flow_)
				flow_->ping(ec);
			else
				ec=boost::asio::error::not_connected;
		}

		virtual uint32_t session_id()const
		{
			if (flow_)
				return flow_->session_id();
			return INVALID_FLOWID;
		}

	public:
		virtual const std::string& get_domain()const
		{
			return domain();
		}


	private:
		//////////////////////////////////////////////////////////////////////////
		//called by flow
		void on_connected(const error_code& ec)
		{
			error_code e=ec;
			cached_remote_endpoint_=flow_->remote_endpoint(e);
			if (!e)
			{
				state_=OPENED;
			}
			else 
			{
				flow_.reset();
				state_=CLOSED;
			}
			if (!is_canceled_op(op_stamp()))
			{
				this->dispatch_connected(ec);
			}
		}
		void on_disconnected(const error_code& ec)
		{
			state_=CLOSED;
			boost::shared_ptr<flow_type> flowHolder=flow_;
			flow_.reset();
			if (!is_canceled_op(op_stamp()))
			{
				this->dispatch_disconnected(ec);
			}
			set_cancel();
			return;
		}

		void on_received(const safe_buffer& buf)
		{
			this->extract_and_dispatch_message(buf);
		}

		void on_writeable()
		{
			if (state_!=CLOSED)
				this->writable_signal()();
		}

	private:
		void __close(bool greaceful)
		{
			set_cancel();
			if (flow_)
			{
				boost::shared_ptr<flow_type> flowHolder=flow_;
				flow_.reset();
				flowHolder->close(greaceful);
			}
			else
			{
				BOOST_ASSERT(state_==CLOSED);
			}
			state_=CLOSED;
			this->disconnect_all_slots();
		}

	private:
		boost::shared_ptr<flow_type> flow_;
		std::string domain_;
		int state_;
		endpoint cached_remote_endpoint_;
	};
} // namespace urdp
} // namespace p2engine

#endif // urdp_socket_h__

