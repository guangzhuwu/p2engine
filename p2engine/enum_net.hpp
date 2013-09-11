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

#ifndef P2ENGINE_ENUM_NET_HPP_INCLUDED
#define P2ENGINE_ENUM_NET_HPP_INCLUDED

#include "p2engine/push_warning_option.hpp"
#include "p2engine/config.hpp"
#include <vector>
#include <set>
#include "p2engine/pop_warning_option.hpp"

namespace p2engine{

	struct ip_interface
	{
		address interface_address;
		address netmask;
		char name[64];
		char mac[6];
		int	mtu;

		ip_interface():mtu(1500){name[0]='\0';mac[0]='\0';}
	};

	struct ip_route
	{
		address destination;
		address netmask;
		address gateway;
		char name[64];
		int mtu;

		ip_route():mtu(1500){name[0]='\0';}
	};

	// return (a1 & mask) == (a2 & mask)
	bool match_addr_mask(address const& a1, address const& a2, address const& mask);
	
	// returns a list of the configured IP interfaces
	// on the machine
	std::vector<ip_interface> enum_net_interfaces(io_service& ios
		, error_code& ec);

	std::vector<ip_route> enum_routes(io_service& ios, error_code& ec);

	// returns true if the specified address is on the same
	// local network as the specified interface
	bool in_subnet(address const& addr, ip_interface const& iface);

	// returns true if the specified address is on the same
	// local network as us
	bool in_local_network(io_service& ios, address const& addr
		, error_code& ec);

	address get_default_gateway(io_service& ios, error_code& ec);

	void get_available_address_v4(io_service& ios,std::set<address>& addr);
	
}

#endif//P2ENGINE_ENUM_NET_HPP_INCLUDED



