//
// file_api.hpp
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

#ifndef p2engine_file_api_hpp__
#define p2engine_file_api_hpp__

#include "p2engine/push_warning_option.hpp"
#include "p2engine/config.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <sys/stat.h>
#include <sys/types.h> // For size_t
#include <fcntl.h>
#ifdef WIN32
#	include <io.h>
#endif

#include <string>

#include <boost/filesystem/path.hpp>

#include "p2engine/pop_warning_option.hpp"

namespace p2engine{
	enum mode_t
	{
		read_only = 0,
		write_only = 1,
		read_write = 2,
		_rw_mask = read_only | write_only | read_write,

		sparse = 8,
		no_atime = 16,
		random_access = 32,
		lock_file = 64,

		attribute_hidden = 0x1000,
		attribute_executable = 0x2000,
		_attribute_mask = attribute_hidden | attribute_executable
	};

	int open_file(const boost::filesystem::path& path, int mode, 
		boost::system::error_code& ec);

#ifdef WIN32
	//using CreateFile
	void* create_file(const boost::filesystem::path& path, int mode, 
		boost::system::error_code& ec);
#endif

#if BOOST_FILESYSTEM_VERSION>=3
# define native_path_string(path) (path).native()
#else
# define native_path_string(path) (path).string()
#endif

	boost::filesystem::path get_exe_path();

}// namespace p2engine

#endif//p2engine_file_api_hpp__
