//
// win_aiofile_service.hpp
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
#ifndef AIOFILE_IMPL_WIN_AIOFILE_SERVICE_HPP
#define AIOFILE_IMPL_WIN_AIOFILE_SERVICE_HPP

#ifdef AIOFILE_IOCP

#include "p2engine/file/file_api.hpp"
#include "p2engine/file/file_background_service.hpp"
#include "p2engine/wrappable_integer.hpp"
#include "p2engine/atomic.hpp"

namespace p2engine{  namespace detail{

	class win_random_access_handle_service
		:public boost::asio::windows::random_access_handle_service
 {
	private:
		file_background_service<win_random_access_handle_service>& get_background_service()
		{
			return boost::asio::use_service<
				file_background_service<win_random_access_handle_service> 
			>(this->get_io_service());
		}

	public:
		win_random_access_handle_service(boost::asio::io_service& service)
			: boost::asio::windows::random_access_handle_service(service)
		{
		}

		template <typename Handler>
		void async_open(implementation_type& impl,const boost::filesystem::path& path,
			int mode, const Handler& handler)
		{
			get_background_service().async_open(*this, impl, path, mode, handler);
		}

		void open(implementation_type& impl,const boost::filesystem::path& path, 
			int mode, boost::system::error_code& err)
		{
			HANDLE h=p2engine::create_file(path,mode,err);
			assign(impl, h, err);
		}

		void seek(implementation_type& impl, int64_t offset, int origin,
			boost::system::error_code& err) 
		{
			// setting file pointer here for truncate purposes in async_write_some
			LARGE_INTEGER li;
			li.QuadPart = offset;
			if(HFILE_ERROR==SetFilePointer(native(impl), li.LowPart, &li.HighPart, origin))
			{
				err =  boost::system::error_code(GetLastError(), 
					boost::asio::error::get_system_category());
			}
		}

	};

}
}

#endif //AIOFILE_IOCP

#endif //AIOFILE_IMPL_WIN_AIOFILE_SERVICE_HPP

