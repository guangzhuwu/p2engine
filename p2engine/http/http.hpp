//
// http.hpp
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

#ifndef P2ENGINE_Http_h__
#define P2ENGINE_Http_h__

#include "p2engine/http/request.hpp"
#include "p2engine/http/response.hpp"
#include "p2engine/http/mime_types.hpp"
#include "p2engine/http/http_connection.hpp"
#include "p2engine/http/http_acceptor.hpp"
#include "p2engine/gzip.hpp"

namespace p2engine{ namespace http{
	
#define HTTP_DECLARE(connectionName,acceptorName)\
	typedef basic_http_connection<connectionName> http_##connectionName;\
	typedef basic_http_acceptor<http_##connectionName,http_##connectionName::connection_base_type> http_##acceptorName;\
	PTR_TYPE_DECLARE(connectionName);\
	PTR_TYPE_DECLARE(acceptorName);\
	PTR_TYPE_DECLARE(http_##connectionName);\
	PTR_TYPE_DECLARE(http_##acceptorName);

	typedef basic_http_connection<http_connection_base> http_connection;
	typedef basic_http_acceptor<http_connection,http_connection_base> http_acceptor;
}
}

#endif//P2ENGINE_Http_h__

