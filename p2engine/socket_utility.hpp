//
// socket_utility.hpp
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
/*

Copyright (c) 2007, Arvid Norberg
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

* Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in
the documentation and/or other materials provided with the distribution.
* Neither the name of the author nor the names of its
contributors may be used to endorse or promote products derived
from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

*/
#ifndef P2ENGINE_SOCKET_UTILITY_HPP
#define P2ENGINE_SOCKET_UTILITY_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "p2engine/push_warning_option.hpp"
#include "p2engine/config.hpp"
#include <string>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include "p2engine/pop_warning_option.hpp"

#include "p2engine/typedef.hpp"
#include "p2engine/variant_endpoint.hpp"

namespace p2engine{

	std::pair<address,int> ipport_from_string(const std::string&ipPortStr);

	template<typename EndpointType>
	EndpointType endpoint_from_string(const std::string&ipPortStr)
	{
		std::pair<address,int> ipport=ipport_from_string(ipPortStr);
		return EndpointType(ipport.first,ipport.second);
	}

	inline std::string address_to_string(address const& addr)
	{
		error_code ec;
		return addr.to_string(ec);
	}

	std::string ipport_to_string(address const& addr,int port);

	template<typename Endpoint>
	std::string endpoint_to_string(Endpoint const& ep)
	{
		return ipport_to_string(ep.address(),ep.port());
	}

	namespace detail
	{
		template<class OutIt>
		void write_address(address const& a, OutIt& out)
		{
			if (a.is_v4())
			{
				write_uint32_hton(a.to_v4().to_ulong(), out);
			}
			else if (a.is_v6())
			{
				address_v6::bytes_type bytes = a.to_v6().to_bytes();
				std::copy(bytes.begin(), bytes.end(), out);
			}
		}

		template<class InIt>
		address read_v4_address(InIt& in)
		{
			unsigned long ip = read_uint32_ntoh(in);
			return address_v4(ip);
		}

		template<class InIt>
		address read_v6_address(InIt& in)
		{
			typedef address_v6::bytes_type bytes_t;
			bytes_t bytes;
			for (bytes_t::iterator i = bytes.begin()
				, end(bytes.end()); i != end; ++i)
				*i = read_uint8_ntoh(in);
			return address_v6(bytes);
		}

		template<class Endpoint, class OutIt>
		void write_endpoint(Endpoint const& e, OutIt& out)
		{
			write_address(e.address(), out);
			write_uint16_hton(e.port(), out);
		}

		template<class Endpoint, class InIt>
		Endpoint read_v4_endpoint(InIt& in)
		{
			address addr = read_v4_address(in);
			int port = read_uint16_ntoh(in);
			return Endpoint(addr, port);
		}

		template<class Endpoint, class InIt>
		Endpoint read_v6_endpoint(InIt& in)
		{
			address addr = read_v6_address(in);
			int port = read_uint16_ntoh(in);
			return Endpoint(addr, port);
		}
	}

	struct v6only
	{
		v6only(bool enable): m_value(enable) {}
		template<class Protocol>
		int level(Protocol const&) const { return IPPROTO_IPV6; }
		template<class Protocol>
		int name(Protocol const&) const 
		{
#if !defined(IPV6_V6ONLY)
# define IPV6_V6ONLY 27
#endif
			return IPV6_V6ONLY; 
		}
		template<class Protocol>
		int const* data(Protocol const&) const { return &m_value; }
		template<class Protocol>
		std::size_t size(Protocol const&) const { return sizeof(m_value); }
		int m_value;
	};

#ifdef WINDOWS_OS

#ifndef IPV6_PROTECTION_LEVEL
#define IPV6_PROTECTION_LEVEL 30
#endif
	struct v6_protection_level
	{
		v6_protection_level(int level): m_value(level) {}
		template<class Protocol>
		int level(Protocol const&) const { return IPPROTO_IPV6; }
		template<class Protocol>
		int name(Protocol const&) const { return IPV6_PROTECTION_LEVEL; }
		template<class Protocol>
		int const* data(Protocol const&) const { return &m_value; }
		template<class Protocol>
		std::size_t size(Protocol const&) const { return sizeof(m_value); }
		int m_value;
	};
#endif

	struct type_of_service
	{
		type_of_service(char val): m_value(val) {}
		template<class Protocol>
		int level(Protocol const&) const { return IPPROTO_IP; }
		template<class Protocol>
		int name(Protocol const&) const { return IP_TOS; }
		template<class Protocol>
		char const* data(Protocol const&) const { return &m_value; }
		template<class Protocol>
		std::size_t size(Protocol const&) const { return sizeof(m_value); }
		char m_value;
	};

#ifdef WINDOWS_OS//disable ICMP "port unreachable"(WSAECONNRESET)
	inline void disable_icmp_unreachable(SOCKET socketDescriptor)
	{
		DWORD   dwBytesReturned   =   0;   
		BOOL     bNewBehavior   =   FALSE;   
		WSAIoctl(socketDescriptor,
			SIO_UDP_CONNRESET,   
			&bNewBehavior,   
			sizeof (bNewBehavior),   
			NULL, 0,&dwBytesReturned,   
			NULL, NULL
			);   
	}
#else
	inline void disable_icmp_unreachable(unsigned int)
	{
	}
#endif

}

#endif

