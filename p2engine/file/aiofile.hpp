//
// aiofile.hpp
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

#ifndef AIOFILE_AIOFILE_HPP
#define AIOFILE_AIOFILE_HPP

#include "p2engine/file/file_api.hpp"
#include "p2engine/file/basic_aiofile.hpp"

#ifndef AIOFILE_NORMAL

# ifdef WIN32
#  if !defined(BOOST_ASIO_DISABLE_IOCP)
#	define AIOFILE_IOCP
#  else
#	define AIOFILE_NORMAL
#  endif//!defined(BOOST_ASIO_DISABLE_IOCP)
# endif//ifdef WIN32

# if !defined(AIOFILE_IOCP)&&!defined(AIOFILE_POSIX_AIO)
#  define AIOFILE_NORMAL
# endif//if !defined(AIOFILE_IOCP)&&!defined(AIOFILE_POSIX_AIO)

#endif


#ifdef AIOFILE_IOCP
#	include "p2engine/file/win_aiofile_service.hpp"
namespace p2engine{
	typedef basic_aiofile<detail::win_random_access_handle_service> aiofile;
}

#elif defined AIOFILE_POSIX_AIO
#  error AIOFILE_ENABLE_POSIX_AIO NOT IMP


#else
#	include "p2engine/file/normal_aiofile_service.hpp"
namespace p2engine{
	typedef basic_aiofile<detail::normal_random_access_handle_service> aiofile;
}

#endif



#endif//AIOFILE_AIOFILE_HPP

