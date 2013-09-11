#include "p2engine/http/http_connection_impl.hpp"
#include "p2engine/uri.hpp"
#include "p2engine/gzip.hpp"
#include "p2engine/broadcast_socket.hpp"

namespace p2engine { namespace http { namespace detail {

	basic_http_connection_impl::basic_http_connection_impl(
		http_connection_base&conn,
		bool enable_ssl,
		bool passive
		)
		:basic_engine_object(conn.get_io_service())
		,connection_(&conn)
		,resolver_(conn.get_io_service())
		,is_passive_(passive)
		,enable_ssl_(enable_ssl)
		,is_gzip_(false)
	{
		set_obj_desc("basic_http_connection_impl");
		__init();
	}

	basic_http_connection_impl::~basic_http_connection_impl()
	{
		BOOST_ASSERT(!connection_);
	}

	error_code basic_http_connection_impl::open(
		const endpoint_type& local_edp, error_code& ec,
		const time_duration& life_time,
		const proxy_settings& 
		)
	{
		BOOST_ASSERT(connection_);
		set_cancel();
		next_op_stamp();
		if (life_time!=boost::date_time::pos_infin
			&&life_time!=boost::date_time::neg_infin)
		{
			error_code timeoutErr=asio::error::timed_out;
			life_timer_=timer_type::create(get_io_service());
			life_timer_->set_obj_desc("basic_http_connection_impl::life_timer_");
			life_timer_->time_signal().bind(&this_type::__to_close_state,
				this,timeoutErr,op_stamp());
		}
		return __open(local_edp,ec);
	}

	void basic_http_connection_impl::async_connect(
		const std::string& remote_host, int port, 
		const time_duration& time_out
		)
	{
		BOOST_ASSERT(connection_);
		BOOST_ASSERT(!is_passive_);
		error_code err;
		address addr=address::from_string(remote_host.c_str(),err);
		if (!err)//if the format of xxx.xxx.xxx.xxx
		{
			remote_host_.clear();
			remote_edp_=endpoint(addr,port);
			async_connect(remote_edp_,time_out);
		}
		else
		{
			remote_host_=remote_host;
			remote_edp_=endpoint();
			remote_edp_.port(port);
			async_connect(endpoint_type(),time_out);
		}
	}

	void basic_http_connection_impl::async_connect(const endpoint& remote_edp, 
		const time_duration& time_out
		)
	{
		BOOST_ASSERT(connection_);

		set_cancel();
		next_op_stamp();

		error_code ec;
		if (state_==INIT)
			__open(endpoint_type(),ec);
		else if (state_!=OPENED)
			ec=asio::error::in_progress;
		if (!connection_)
			ec=asio::error::not_socket;

		if (ec)
		{
			get_io_service().post( make_alloc_handler(
				boost::bind(&this_type::__allert_connected,
				SHARED_OBJ_FROM_THIS,ec,op_stamp()
				)
				));
		}

		time_out_=time_out;
		// Start the conn timer
		if (time_out_!=boost::date_time::pos_infin
			&&time_out_!=boost::date_time::neg_infin
			)
		{
			error_code timeoutErr=asio::error::timed_out;
			conn_timeout_timer_->time_signal().clear();
			conn_timeout_timer_->time_signal().bind(&this_type::__to_close_state,
				this,timeoutErr,op_stamp());
			conn_timeout_timer_->async_wait(time_out_);
		}
		
		resolver_query_.reset();
		state_=CONNECTING;
		if (!is_any(remote_edp.address())&&remote_edp.port())//we know the remote_endpoint
		{
			remote_edp_=remote_edp;
			__async_resolve_connect_coro(op_stamp());
		}
		else if(!remote_host_.empty())//we know the remote_host name
		{
			if (remote_edp_.port()==0)
				remote_edp_.port(80);
			resolver_query_.reset(new resolver_query(remote_host_,
				boost::lexical_cast<std::string>(remote_edp_.port())));
			__async_resolve_connect_coro(op_stamp());
		}
		else 
		{
			ec=boost::asio::error::host_not_found;
			get_io_service().post( make_alloc_handler(
				boost::bind(&this_type::__allert_connected,
				SHARED_OBJ_FROM_THIS,ec,op_stamp()
				)
				));
		}
	}

	void basic_http_connection_impl::async_send(const safe_buffer& buf)
	{
		if (state_!=CONNECTED)
			return;

		BOOST_ASSERT(connection_);

		send_bufs_len_+=buf.length();

		if (sending_)
		{
			send_bufs_.push(buf);
		}
		else
		{
			BOOST_ASSERT(send_bufs_len_==buf.length());
			sending_=true;
			asio::async_write(*socket_impl_,
				buf.to_asio_const_buffers_1(),
				asio::transfer_all(),
				make_alloc_handler(
				boost::bind(&this_type::__async_send_handler,
				SHARED_OBJ_FROM_THIS,_1,_2,op_stamp())
				)
				);
		}
	}

	void basic_http_connection_impl::keep_async_receiving()
	{
		BOOST_ASSERT(connection_);

		if (is_recv_blocked_&&recv_state_!=RECVING)
		{
			is_recv_blocked_=false;
			if (is_header_recvd_&&recv_buf_.size()>0)
			{
				get_io_service().post(make_alloc_handler(
					boost::bind(&this_type::__async_recv_handler,
					SHARED_OBJ_FROM_THIS,error_code(),recv_buf_.size(),
					op_stamp())
					));
			}
			else
				do_keep_receiving(op_stamp());
		}
	}

	void basic_http_connection_impl::do_keep_receiving(op_stamp_t stamp)
	{
		//all canceled operation will not be invoked
		if(is_canceled_op(stamp)||state_!=CONNECTED)
			return;

		recv_state_=RECVING;
		asio::async_read(*socket_impl_,
			recv_buf_,
			asio::transfer_at_least(1),
			make_alloc_handler(
			boost::bind(&this_type::__async_recv_handler,
			SHARED_OBJ_FROM_THIS,_1,_2,stamp)
			)
			);
	}

	void basic_http_connection_impl::close(bool greaceful)
	{
		if (CLOSED==state_)
			return;

		set_cancel();
		if (life_timer_)
		{
			life_timer_->cancel();
			life_timer_.reset();
		}
		if (conn_retry_timer_)
		{
			conn_retry_timer_->cancel();
			conn_retry_timer_.reset();
		}
		connection_=NULL;
		if (greaceful&&!send_bufs_.empty())
		{
			state_=CLOSING;
		}
		else
		{
			error_code ec;
			socket_impl_->async_close(
				boost::bind(&this_type::__on_shutdown,socket_impl_,_1)
				);
			state_=CLOSED;
		}
	}

	void basic_http_connection_impl::__init()
	{
		BOOST_ASSERT(connection_);
		sending_=false;
		is_recv_blocked_=true;
		is_header_recvd_=false;
		recv_state_=RECVED;
		state_=INIT;
		recv_buf_.consume(recv_buf_.size());
		if(!socket_impl_)
			socket_impl_.reset(new ssl_stream_wrapper<tcp::socket>(
			connection_->get_io_service(), enable_ssl_));
		while(!send_bufs_.empty())
		{
			send_bufs_.pop();
		}
		if (conn_timeout_timer_)
		{
			conn_timeout_timer_->cancel();
		}
		else
		{
			conn_timeout_timer_=timer_type::create(get_io_service());
			conn_timeout_timer_->set_obj_desc("basic_http_connection_impl::conn_timeout_timer_");
		}
		if (is_passive_) 
			state_=CONNECTED;
		send_bufs_len_=0;
	}

	error_code basic_http_connection_impl::__open(const endpoint_type& local_edp,
		error_code& ec)
	{
		__init();
		if (state_!=INIT)
		{
			ec=asio::error::already_open;
			return ec;
		}
		BOOST_ASSERT(!is_passive_);
		is_passive_=false;
		local_edp_=local_edp;
		socket_impl_->open(asio::ip::tcp::endpoint(local_edp_).protocol(), ec);
		if (!ec)
		{
			if (local_edp_.port()!=0)
				socket_impl_->bind(asio::ip::tcp::endpoint(local_edp_),ec);
			if (ec)
			{
				error_code err;
				socket_impl_->async_close(
					boost::bind(&this_type::__on_shutdown,socket_impl_,ec)
					);
				socket_impl_.reset(new ssl_stream_wrapper<tcp::socket>(
					connection_->get_io_service(), enable_ssl_)
					);
				return ec;
			}
			else
			{
				error_code e;
				//using a larg buffer
				asio::socket_base::receive_buffer_size
					receive_buffer_size_option(256*1024);
				asio::socket_base::send_buffer_size 
					send_buffer_size_option(256*1024);
				//asio::socket_base::send_low_watermark send_low_watermark_option(512);
				socket_impl_->set_option(receive_buffer_size_option, e);
				socket_impl_->set_option(send_buffer_size_option, e);
				//socket_impl_->set_option(send_low_watermark_option);
				state_=OPENED;
			}
		}
		return ec;
	}

	void basic_http_connection_impl::__to_close_state(const error_code& ec, 
		op_stamp_t stamp)
	{
		//all canceled operation will not be invoked
		if(state_!=CLOSING&&(is_canceled_op(stamp)||state_==CLOSED))
			return;

		//close() will set connection_ to NULL.
		//copy state heer
		http_connection_base* conn=connection_;
		int s=state_;

		close();

		if (conn)
		{
			if (is_passive_)
			{
				if (s==CONNECTED)
					conn->dispatch_disconnected(ec);
			}
			else
			{
				if (s<CONNECTED)
					conn->dispatch_connected(ec);
				else
					conn->dispatch_disconnected(ec);
			}
		}
	}

	void basic_http_connection_impl::__async_resolve_connect_coro(op_stamp_t stamp,
		coroutine coro,error_code err, resolver_iterator itr
		)
	{
		//all canceled operation will not be invoked
		if(is_canceled_op(stamp)||state_==CLOSED||!connection_)
			return;

		CORO_REENTER(coro)
		{
			state_=CONNECTING;
			//if qy is not null, we yield async resolve
			if (resolver_query_)
			{
				if (resolve_timer_)
					resolve_timer_->cancel();
				resolve_timer_=timer_type::create(get_io_service());
				resolve_timer_->set_obj_desc("basic_http_connection_impl::resolve_timer_");
				resolve_timer_->time_signal().bind(
					&this_type::__retry_async_resolve_connect_coro,this,stamp
					);
				resolve_timer_->async_wait(seconds(3));
				CORO_YIELD(
					resolver_.async_resolve(*resolver_query_,make_alloc_handler(
					boost::bind(&this_type::__async_resolve_connect_coro,
					SHARED_OBJ_FROM_THIS,stamp,coro,_1,_2))
					) 
					);
				if (resolve_timer_)
				{
					resolve_timer_->cancel();
					resolve_timer_.reset();
				}
				if (err||itr==resolver_iterator())
				{
					__retry_async_resolve_connect_coro(stamp);
					return;
				}
				remote_edp_.address(resolver_type::endpoint_type(*itr++).address());
				if (remote_edp_.port()==0)
					remote_edp_.port(80);
			}

			//yield async connect
			CORO_YIELD(socket_impl_->async_connect(remote_edp_,
				make_alloc_handler(
				boost::bind(&this_type::__async_resolve_connect_coro,
				SHARED_OBJ_FROM_THIS,stamp,coro,_1,itr))
				));

			while(err && itr != resolver_iterator())
			{
				//close socket that failed to connect, and open a new one
				socket_impl_->close(err);
				__open(local_edp_,err);
				if (err)
				{
					__retry_async_resolve_connect_coro(stamp);
					return;
				}
				remote_edp_.address(resolver_type::endpoint_type(*itr++).address());
				//yield async connect
				CORO_YIELD(socket_impl_->async_connect(remote_edp_,make_alloc_handler(
					boost::bind(&this_type::__async_resolve_connect_coro,
					SHARED_OBJ_FROM_THIS,stamp,coro,_1,itr)
					)));
			}
			if (err)
			{
				__retry_async_resolve_connect_coro(stamp);
				return;
			}

			//now lowlayer connected

			BOOST_ASSERT(!err);
			
			if (conn_timeout_timer_)
				conn_timeout_timer_->cancel();
			if (conn_retry_timer_)
				conn_retry_timer_->cancel();
			if(resolve_timer_)
				resolve_timer_->cancel();

			resolve_timer_.reset();
			resolver_query_.reset();

			state_=CONNECTED;
			keep_async_receiving();
			if (connection_)
				connection_->dispatch_connected(err);
			return;
		}
	}

	void basic_http_connection_impl::__retry_async_resolve_connect_coro(op_stamp_t stamp)
	{
		if(is_canceled_op(stamp))
			return;

		if (resolve_timer_)
		{
			resolve_timer_->cancel();
			resolve_timer_.reset();
		}

		if (!conn_retry_timer_)
		{
			conn_retry_timer_=timer_type::create(get_io_service());
			conn_retry_timer_->set_obj_desc("basic_http_connection_impl::conn_retry_timer_");
		}
		conn_retry_timer_->cancel();
		conn_retry_timer_->time_signal().clear();
		conn_retry_timer_->time_signal().bind(&this_type::__async_resolve_connect_coro,
			this,stamp,coroutine(),error_code(),resolver_iterator()
			);
		conn_retry_timer_->async_wait(milliseconds(100));
	}

	void basic_http_connection_impl::__async_send_handler(
		error_code ec, std::size_t len,op_stamp_t stamp)
	{
		//all canceled operation will not be invoked
		if(state_!=CLOSING&&(is_canceled_op(stamp)||state_==CLOSED))
			return;

		sending_=false;
		if (!ec)
		{
			BOOST_ASSERT(send_bufs_len_>=len);
			send_bufs_len_-=len;
			if(!send_bufs_.empty())
			{
				asio::async_write(*socket_impl_,
					send_bufs_.front().to_asio_const_buffers_1(),
					asio::transfer_all(),
					make_alloc_handler(
					boost::bind(&this_type::__async_send_handler,
					SHARED_OBJ_FROM_THIS,_1,_2,stamp))
					);
				send_bufs_.pop();
				sending_=true;
			}
			else if(state_==CLOSING)
			{
				__to_close_state(ec,stamp);
			}
			else
			{
				if(!is_canceled_op(stamp)&&connection_)
					connection_->dispatch_sendout();
			}
		}
		else
		{
			while(!send_bufs_.empty())
				send_bufs_.pop();
			send_bufs_len_=0;
			__to_close_state(ec,stamp);
		}
	}

	void basic_http_connection_impl::__async_recv_handler(error_code ec, 
		std::size_t len,op_stamp_t stamp)
	{
		if(is_canceled_op(stamp)||state_==CLOSED||!connection_)
			return;

		if (ec)
		{
			while(!send_bufs_.empty())
				send_bufs_.pop();
			__to_close_state(ec,stamp);
			return;
		}

		recv_state_=RECVED;
		if (is_recv_blocked_)
			return;
		if (!is_header_recvd_)
		{
			http::request  req;
			http::response res;
			http::header* h=(is_passive_?(http::header*)(&req):(http::header*)(&res));
			h->clear();
			asio::streambuf::const_buffers_type bf=recv_buf_.data();
			const char* s=asio::buffer_cast<const char*>(bf);
			int parseRst=h->parse(s,recv_buf_.size());
			if(parseRst>0)
			{
				is_header_recvd_=true;
				safe_buffer buf;
				if (std::size_t(parseRst)<recv_buf_.size())
				{
					safe_buffer_io bio(&buf);
					bio.write(s+parseRst,recv_buf_.size()-parseRst);
				}
				recv_buf_.consume(recv_buf_.size());
				if (connection_)
				{
					const std::string&encoding=h->get(HTTP_ATOM_Content_Encoding);
					if (encoding=="gzip"||encoding=="x-gzip")
						is_gzip_=true;
					else
						is_gzip_=false;
					if(is_passive_)
						connection_->dispatch_request(req);
					else
						connection_->dispatch_response(res);
				}
				if(buf.length()>0&&!(is_canceled_op(stamp)||state_==CLOSED))
				{
					if (is_recv_blocked_)
					{
						memcpy(asio::buffer_cast<char*>(recv_buf_.prepare(buf.length())),
							p2engine::buffer_cast<char*>(buf),buf.size());
						recv_buf_.commit(buf.length());
						return;
					}
					else 
					{
						__dispatch_packet(buf,stamp);
					}
				}
			}
			else if(parseRst==0)
			{
				recv_state_=RECVING;
			}
			else
			{
				recv_buf_.consume(recv_buf_.size());
				__to_close_state(asio::error::message_size,stamp);
				return;
			}

		}
		else
		{
			safe_buffer buf;
			safe_buffer_io bio(&buf);
			const char* s=asio::buffer_cast<const char*>(recv_buf_.data());
			bio.write(s,recv_buf_.size());
			recv_buf_.consume(recv_buf_.size());
			__dispatch_packet(buf,stamp);
		}
		recv_state_=RECVING;
		asio::async_read(*socket_impl_,
			recv_buf_,
			asio::transfer_at_least(1),
			make_alloc_handler(boost::bind(&this_type::__async_recv_handler,
			SHARED_OBJ_FROM_THIS,_1,_2,stamp))
			);
	}


	void basic_http_connection_impl::__allert_connected(error_code ec, 
		op_stamp_t stamp)
	{
		if(is_canceled_op(stamp)||state_==CLOSED)//all canceled operation will not be invoked
			return;

		if (connection_)
			connection_->dispatch_connected(ec);
	}


	void basic_http_connection_impl::__dispatch_packet(safe_buffer& data,op_stamp_t  stamp)
	{
		if (connection_)
			connection_->dispatch_packet(data);
	}


}
}
}
