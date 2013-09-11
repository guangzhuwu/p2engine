//
// http_connection_impl.hpp
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
#ifndef P2ENGINE_HTTP_CONNECTION_IMPL_HPP
#define P2ENGINE_HTTP_CONNECTION_IMPL_HPP

#include "p2engine/operation_mark.hpp"
#include "p2engine/timer.hpp"
#include "p2engine/coroutine.hpp"
#include "p2engine/handler_allocator.hpp"
#include "p2engine/safe_buffer.hpp"
#include "p2engine/http/http_connection_base.hpp"
#include "p2engine/ssl_stream_wrapper.hpp"


namespace p2engine { namespace http { namespace  detail{

	class basic_http_connection_impl
		: public basic_engine_object
		, public operation_mark
		, public fssignal::trackable
	{
		typedef basic_http_connection_impl  this_type;
		SHARED_ACCESS_DECLARE;

		typedef variant_endpoint endpoint_type;
		typedef variant_endpoint endpoint;
		typedef asio::ip::tcp::resolver resolver_type;
		typedef resolver_type::iterator resolver_iterator;
		typedef resolver_type::query	resolver_query;
		typedef rough_timer timer_type;
		typedef boost::shared_ptr<timer_type> timer_sptr;
		typedef boost::asio::ip::tcp::socket::lowest_layer_type lowest_layer_type;

	protected:
		basic_http_connection_impl(http_connection_base&conn,bool enable_ssl,bool passive);
		~basic_http_connection_impl();

	public:
		static shared_ptr create(http_connection_base&conn,bool enable_ssl,bool passive)
		{
			return shared_ptr(new this_type(conn,enable_ssl,passive),
				shared_access_destroy<this_type>());
		}

		error_code open(const endpoint_type& local_edp, error_code& ec,
			const time_duration& life_time=boost::date_time::pos_infin,
			const proxy_settings& ps=proxy_settings()
			);

		void async_connect(const std::string& remote_host, int port, 
			const time_duration& time_out=seconds(60)
			);

		void async_connect(const endpoint& remote_edp,
			const time_duration& time_out=seconds(60)
			);

		//reliable send
		void async_send(const safe_buffer& buf);

		void keep_async_receiving();

		void block_async_receiving()
		{
			is_recv_blocked_=true;
		}

		void close(bool greaceful=true);

		bool is_open() const
		{
			return socket_impl_->is_open();
		}
		bool is_connected()const
		{
			return state_==CONNECTED;
		}

		endpoint local_endpoint(error_code& ec)const
		{
			return socket_impl_->local_endpoint(ec);
		}
		endpoint remote_endpoint(error_code& ec)const
		{
			return socket_impl_->remote_endpoint(ec);
		}

		lowest_layer_type& lowest_layer() 
		{
			return socket_impl_->lowest_layer();
		}

		std::size_t overstocked_send_size()const
		{
			return send_bufs_len_;
		}

	protected:
		void do_keep_receiving(op_stamp_t stamp);

		void __init();

		error_code __open(const endpoint_type& local_edp,error_code& ec);

		void __to_close_state(const error_code& ec, op_stamp_t stamp);

		void __retry_async_resolve_connect_coro(op_stamp_t stamp);
		void __async_resolve_connect_coro(op_stamp_t stamp, coroutine coro=coroutine(),
			error_code err=error_code(), resolver_iterator itr=resolver_iterator());


		void __async_send_handler(error_code ec, std::size_t len,op_stamp_t);

		void __async_recv_handler(error_code ec, std::size_t len,op_stamp_t);

		void __allert_connected(error_code ec, op_stamp_t stamp);

		void __dispatch_packet(safe_buffer& buf,op_stamp_t  stamp);

	private:
		static void __on_shutdown(boost::shared_ptr<ssl_stream_wrapper<tcp::socket> > sock, 
			error_code ec)
		{
			if (sock)
			{
				error_code e;
				sock->lowest_layer().close(e);
			}
		}
	protected:
		boost::shared_ptr<ssl_stream_wrapper<tcp::socket> > socket_impl_;
		http_connection_base* connection_;
		enum{INIT, OPENED, CONNECTING, CONNECTED,CLOSING, CLOSED} state_;
		enum{RECVING,RECVED} recv_state_;
		timer_sptr conn_retry_timer_;
		timer_sptr resolve_timer_;
		timer_sptr conn_timeout_timer_,life_timer_;
		std::string remote_host_;
		resolver_type resolver_;
		boost::scoped_ptr<resolver_query> resolver_query_;
		endpoint_type remote_edp_;
		endpoint_type local_edp_;
		std::queue<safe_buffer> send_bufs_;
		std::size_t send_bufs_len_;
		asio::streambuf recv_buf_;
		time_duration time_out_;

		bool sending_:1;
		bool is_passive_:1;
		bool enable_ssl_:1;
		bool is_recv_blocked_:1;
		bool is_header_recvd_:1;
		bool is_gzip_:1;
	};

} // namespace detail 
} // namespace http
} // namespace p2engine

#endif // P2ENGINE_HTTP_CONNECTION_IMPL_HPP
