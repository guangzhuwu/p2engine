//
// utf8.hpp
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

#ifndef p2engine_utf8_hpp__
#define p2engine_utf8_hpp__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "p2engine/convertutf.h"

namespace p2engine {

#ifndef P2ENGINE_NO_WIDE_FUNCTIONS
	int utf8_wchar(const std::string &utf8, std::wstring &wide);
	int wchar_utf8(const std::wstring &wide, std::string &utf8);
	std::wstring convert_to_wstring(std::string const& s);
	std::string convert_from_wstring(std::wstring const& s);
#endif

	std::string convert_to_native(std::string const& s);
	std::string convert_from_native(std::string const& s);

}

#endif//p2engine_utf8_hpp__

