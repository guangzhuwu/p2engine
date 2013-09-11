//
// http_acceptor.hpp
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
#ifndef P2ENGINE_HTTP_ACCEPTOR_HPP
#define P2ENGINE_HTTP_ACCEPTOR_HPP

#include "p2engine/http/http_acceptor_base.hpp"
#include "p2engine/timer.hpp"
#include "p2engine/coroutine.hpp"
#include "p2engine/operation_mark.hpp"
#include "p2engine/handler_allocator.hpp"

namespace p2engine { namespace http {

	template<typename ConnectionType, typename ConnectionBaseType>
	class basic_http_acceptor 
		:public http_acceptor_base<ConnectionBaseType>
		,public operation_mark
	{
		typedef basic_http_acceptor<ConnectionType,ConnectionBaseType> this_type;
		SHARED_ACCESS_DECLARE;

	public:
		typedef this_type acceptor_type;
		typedef ConnectionType connection_type;
		typedef ConnectionBaseType connection_base_type;
		typedef typename http_acceptor_base<ConnectionBaseType>::lowest_layer_type lowest_layer_type;

	public:
		static shared_ptr create(io_service& ios,bool  enableSsl=false)
		{
			return shared_ptr(new this_type(ios,enableSsl),
				shared_access_destroy<this_type>()
				);
		}

	protected:
		basic_http_acceptor(io_service&ios,bool enableSsl=false)
			:http_acceptor_base<ConnectionBaseType>(ios)
			,is_accepting_(false)
			,enable_ssl_(enableSsl)
			,block_async_accepting_(true)
			,acceptor_(ios)
		{}
		virtual ~basic_http_acceptor()
		{
			set_cancel();
		}

	public:
		virtual error_code open(const endpoint& local_edp,error_code& ec)
		{
			this->op_cancel();
			this->next_op_stamp();
			acceptor_.open(tcp::endpoint(local_edp).protocol(),ec);
			local_endpoint_=local_edp;
			return ec;
		}
		virtual error_code listen(error_code& ec)
		{
			this->next_op_stamp();
			acceptor_.bind(local_endpoint_,ec);
			if (!ec)acceptor_.listen(asio::socket_base::max_connections,ec);
			if (!ec)keep_async_accepting();
			if(ec&&acceptor_.is_open())
			{
				error_code e;
				acceptor_.close(e);
			}
			return ec;
		}

		virtual error_code listen(const endpoint& local_edp,error_code& ec)
		{
			this->op_cancel();
			this->next_op_stamp();
			acceptor_.open(tcp::endpoint(local_edp).protocol(),ec);
			local_endpoint_=local_edp;
			if (!ec)
			{
				acceptor_.set_option(asio::socket_base::linger(true,1),ec);
				acceptor_.set_option(asio::socket_base::reuse_address(true),ec);
				acceptor_.bind(local_edp,ec);
				if (!ec)
					acceptor_.listen(asio::socket_base::max_connections,ec);
			}
			if (!ec)
				keep_async_accepting();
			else
			{
				error_code e;
				acceptor_.close(e);
			}
			return ec;
		}
		virtual void keep_async_accepting()
		{
			if (accepted_)
			{
				boost::shared_ptr<connection_type> conn=accepted_;
				error_code ec=accepted_ec_;
				accepted_.reset();
				accepted_ec_.clear();
				this->get_io_service().post(
					make_alloc_handler(boost::bind(&this_type::__accept_handler,
					SHARED_OBJ_FROM_THIS,ec,conn,this->op_stamp())
					)
					);
			}
			else if (block_async_accepting_&&!is_accepting_)
			{
				is_accepting_=true;
				block_async_accepting_=false;
				boost::shared_ptr<connection_type> conn
					=connection_type::create(this->get_io_service(),enable_ssl_,true);
				error_code ec;
				conn->open(endpoint(),ec);
				acceptor_.async_accept(conn->lowest_layer(),
					make_alloc_handler(boost::bind(&this_type::__accept_handler,
					SHARED_OBJ_FROM_THIS,_1,conn,this->op_stamp()))
					);
			}
		}
		virtual void block_async_accepting()
		{
			block_async_accepting_=true;
		}

		virtual error_code close()
		{
			error_code ec; 
			return __to_close_stat(ec);
		}
		virtual error_code close(error_code& ec)
		{
			return __to_close_stat(ec);
		}

		virtual bool is_open()const
		{
			return acceptor_.is_open();
		}

		virtual endpoint local_endpoint(error_code& ec) const
		{
			return acceptor_.local_endpoint(ec);
		}

		virtual lowest_layer_type& lowest_layer()
		{
			return acceptor_;
		}

	protected:
		void __accept_handler(error_code ec,
			boost::shared_ptr<connection_type> conn, op_stamp_t stamp)
		{
			if (this->is_canceled_op(stamp))
				return;
			is_accepting_=false;
			if (block_async_accepting_)
			{
				accepted_=conn;
				accepted_ec_=ec;
				return;
			}
			if (ec) 
				conn->close();
			else 
				conn->keep_async_receiving();
			dispatch_accepted(conn,ec);
			if (acceptor_.is_open()&&!block_async_accepting_)
			{
				is_accepting_=true;
				boost::shared_ptr<connection_type> conn=
					connection_type::create(this->get_io_service(),enable_ssl_,true);
				error_code ec;
				conn->open(endpoint(),ec);
				acceptor_.async_accept(conn->lowest_layer(),
					make_alloc_handler(boost::bind(&this_type::__accept_handler,
					SHARED_OBJ_FROM_THIS,_1,conn,stamp))
					);
			}
		}
		
		error_code __to_close_stat(error_code& ec)
		{
			ec.clear();
			set_cancel();
			acceptor_.cancel(ec);
			acceptor_.close(ec);
			this->disconnect_all_slots();
			return ec;
		}

	protected:
		bool block_async_accepting_;
		bool is_accepting_;
		bool enable_ssl_;

		boost::shared_ptr<connection_type> accepted_;
		error_code accepted_ec_;
		endpoint local_endpoint_;

		tcp::acceptor acceptor_;
	};


} // namespace http
} // namespace p2engine

#endif // P2ENGINE_HTTP_CONNECTION_HPP

