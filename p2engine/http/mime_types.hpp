//
// mime_types.hpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2010 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_P2ENGINE_MIME_TYPES_HPP
#define HTTP_P2ENGINE_MIME_TYPES_HPP

#include "p2engine/config.hpp"

namespace p2engine {
namespace http {
namespace mime_types {

	// Map the file name extension to MIME type.
	const char* ext_to_mime(const char* ext_name);

	// Map the file name extension to MIME type.
	const char* mime_to_ext(const char* mime);

} // namespace mime_types
} // namespace http
} // namespace p2engine

#endif // HTTP_P2ENGINE_MIME_TYPES_HPP
