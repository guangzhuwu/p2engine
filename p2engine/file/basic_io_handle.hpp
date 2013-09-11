//
// basic_io_handle.hpp
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

#ifndef P2ENGINE_BASIC_IO_HANDLE_HPP
#define P2ENGINE_BASIC_IO_HANDLE_HPP

#include "p2engine/push_warning_option.hpp"
#include "p2engine/config.hpp"

#include <boost/asio/basic_io_object.hpp>

#include "p2engine/pop_warning_option.hpp"

namespace p2engine{

	template<typename HandleService>
	class basic_io_handle
		: public boost::asio::basic_io_object<HandleService>
	{
	public:
		typedef typename HandleService::native_handle_type native_type;
		typedef typename HandleService::native_handle_type native_handle_type;
		typedef basic_io_handle<HandleService> lowest_layer_type;

		explicit basic_io_handle(boost::asio::io_service& io_service)
			: boost::asio::basic_io_object<HandleService>(io_service)
		{
		}

		basic_io_handle(boost::asio::io_service& io_service,
			const native_handle_type& handle)
			: boost::asio::basic_io_object<HandleService>(io_service)
		{
			boost::system::error_code ec;
			this->service.assign(this->implementation, handle, ec);
			boost::asio::detail::throw_error(ec, "assign");
		}

#if defined(BOOST_ASIO_HAS_MOVE)
		basic_io_handle(basic_io_handle&& other)
			:boost::asio::basic_io_object<HandleService>(
			BOOST_ASIO_MOVE_CAST(basic_io_handle)(other))
		{
		}

		basic_io_handle& operator=(basic_io_handle&& other)
		{
			boost::asio::basic_io_object<HandleService>::operator=
				(BOOST_ASIO_MOVE_CAST(basic_io_handle)(other));
			return *this;
		}

#endif 
		lowest_layer_type& lowest_layer()
		{
			return *this;
		}
		const lowest_layer_type& lowest_layer() const
		{
			return *this;
		}

		void assign(const native_handle_type& handle)
		{
			boost::system::error_code ec;
			this->service.assign(this->implementation, handle, ec);
			boost::asio::detail::throw_error(ec, "assign");
		}

		boost::system::error_code assign(const native_handle_type& handle,
			boost::system::error_code& ec)
		{
			return this->service.assign(this->implementation, handle, ec);
		}

		bool is_open() const
		{
			return this->service.is_open(this->implementation);
		}

		void close()
		{
			boost::system::error_code ec;
			this->service.close(this->implementation, ec);
			boost::asio::detail::throw_error(ec, "close");
		}

		boost::system::error_code close(boost::system::error_code& ec)
		{
			this->service.close(this->implementation, ec);
			return ec;
		}

		native_type native()
		{
			return this->service.native_handle(this->implementation);
		}

		native_handle_type native_handle()
		{
			return this->service.native_handle(this->implementation);
		}

		void cancel()
		{
			boost::system::error_code ec;
			this->service.cancel(this->implementation, ec);
			boost::asio::detail::throw_error(ec, "cancel");
		}

		boost::system::error_code cancel(boost::system::error_code& ec)
		{
			return this->service.cancel(this->implementation, ec);
		}

	protected:
		/// Protected destructor to prevent deletion through this type.
		virtual ~basic_io_handle()
		{
		}
	};

}

#endif//P2ENGINE_BASIC_IO_HANDLE_HPP
