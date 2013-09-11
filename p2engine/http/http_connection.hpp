//
// http_connection.hpp
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
#ifndef P2ENGINE_HTTP_CONNECTION_HPP
#define P2ENGINE_HTTP_CONNECTION_HPP

#include "p2engine/http/http_connection_base.hpp"
#include "p2engine/timer.hpp"
#include "p2engine/coroutine.hpp"
#include "p2engine/operation_mark.hpp"
#include "p2engine/handler_allocator.hpp"
#include "p2engine/http/http_connection_impl.hpp"

namespace p2engine { namespace http {

	template<typename BaseConnectionType>
	class basic_http_connection
		: public BaseConnectionType
	{
		typedef basic_http_connection<BaseConnectionType> this_type;
		SHARED_ACCESS_DECLARE;
		BOOST_STATIC_ASSERT((boost::is_same<BaseConnectionType,http_connection_base>::value)
			||(boost::is_base_and_derived<http_connection_base,BaseConnectionType>::value));
		typedef detail::basic_http_connection_impl impl_type;

	protected:
		basic_http_connection(io_service& ios,bool enable_ssl,bool isPassive)
			:BaseConnectionType(ios,enable_ssl,isPassive)
		{}
		virtual ~basic_http_connection()
		{
			if (impl_)
				impl_->close();
			impl_.reset();
		};

	public:
		typedef BaseConnectionType connection_base_type; 
		typedef typename BaseConnectionType::lowest_layer_type lowest_layer_type; 

		static shared_ptr create(io_service& ios,bool enable_ssl=false,bool passive=false)
		{
			return shared_ptr(new this_type(ios,enable_ssl,passive),
				shared_access_destroy<this_type>());
		}

	public:
		virtual error_code open(const endpoint& local_edp, error_code& ec,
			const time_duration& life_time=boost::date_time::pos_infin,
			const proxy_settings& ps=proxy_settings()
			)
		{
			BOOST_ASSERT(!impl_);
			if (impl_)
				impl_->close();
			impl_=impl_type::create(*this,this->enable_ssl(),this->is_passive());
			return impl_->open(local_edp,ec,life_time,ps);
		}

		virtual void async_connect(const std::string& remote_host, int port, 
			const time_duration& time_out=boost::date_time::pos_infin
			)
		{
			if (!impl_||!impl_->is_open())
				impl_=impl_type::create(*this,this->enable_ssl(),this->is_passive());
			impl_->async_connect(remote_host,port,time_out);
		}
		virtual void  async_connect(const endpoint& peer_endpoint,
			const time_duration& time_out=boost::date_time::pos_infin
			)
		{
			if (!impl_||!impl_->is_open())
				impl_=impl_type::create(*this,this->enable_ssl(),this->is_passive());
			impl_->async_connect(peer_endpoint,time_out);
		}

		virtual void async_send(const safe_buffer& buf)
		{
			BOOST_ASSERT(impl_);
			if (impl_)
				impl_->async_send(buf);
		}

		virtual void keep_async_receiving()
		{
			BOOST_ASSERT(impl_);
			if (impl_)
				impl_->keep_async_receiving();
		}
		virtual void block_async_receiving()
		{
			BOOST_ASSERT(impl_);
			if (impl_)
				impl_->block_async_receiving();
		}

		virtual void close(bool greaceful=true)
		{
			if (impl_)
				impl_->close();
			impl_.reset();
		}
		virtual bool is_open() const
		{
			if (impl_)
				return impl_->is_open();
			return false;
		}
		virtual bool is_connected() const
		{
			if (impl_)
				return impl_->is_connected();
			return false;
		}

		virtual endpoint local_endpoint(error_code& ec)const
		{
			BOOST_ASSERT(impl_);
			if (impl_)
				return impl_->local_endpoint(ec);
			ec=asio::error::not_socket;
			return endpoint();
		}
		virtual endpoint remote_endpoint(error_code& ec)const
		{
			BOOST_ASSERT(impl_);
			if (impl_)
				return impl_->remote_endpoint(ec);
			ec=asio::error::not_socket;
			return endpoint();
		}

		lowest_layer_type& lowest_layer() 
		{
			BOOST_ASSERT(impl_);
			if (!impl_)
				impl_=impl_type::create(*this,this->enable_ssl(),this->is_passive());
			return impl_->lowest_layer();
		}

		virtual std::size_t overstocked_send_size()const
		{
			BOOST_ASSERT(impl_);
			if (!impl_)
				return 0;
			return impl_->overstocked_send_size();
		}

	protected:
		boost::shared_ptr<impl_type> impl_;

	};

} // namespace http
} // namespace p2engine

#endif // P2ENGINE_HTTP_CONNECTION_HPP

