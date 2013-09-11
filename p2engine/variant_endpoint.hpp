//
// variant_endpoint.hpp
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

#ifndef p2engine_variant_enpoint_h__
#define p2engine_variant_enpoint_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "p2engine/config.hpp"

namespace p2engine{
	class variant_endpoint{
		typedef boost::asio::ip::udp::endpoint	udp_endpoint_type;
		typedef boost::asio::ip::tcp::endpoint	tcp_endpoint_type;
		typedef boost::asio::ip::icmp::endpoint icmp_endpoint_type;
		
	public:
		/// The type of the endpoint structure. This type is dependent on the
		/// underlying implementation of the socket layer.
#if defined(GENERATING_DOCUMENTATION)
		typedef implementation_defined data_type;
#else
		typedef boost::asio::detail::socket_addr_type data_type;
#endif

		/// Default constructor.
		variant_endpoint()
			: impl_()
		{
		}

		/// Construct an endpoint using a port number, specified in the host's byte
		/// order. The IP address will be the any address (i.e. INADDR_ANY or
		/// in6addr_any). This constructor would typically be used for accepting new
		/// connections.
		/**
		* @par Examples
		* To initialise an IPv4 TCP endpoint for port 1234, use:
		* @code
		* boost::asio::ip::tcp::endpoint ep(boost::asio::ip::tcp::v4(), 1234);
		* @endcode
		*
		* To specify an IPv6 UDP endpoint for port 9876, use:
		* @code
		* boost::asio::ip::udp::endpoint ep(boost::asio::ip::udp::v6(), 9876);
		* @endcode
		*/
		template<typename InternetProtocol>
		variant_endpoint(const InternetProtocol& internet_protocol,
			unsigned short port_num)
			: impl_(internet_protocol.family(), port_num)
		{
		}

		/// Construct an endpoint using a port number and an IP address. This
		/// constructor may be used for accepting connections on a specific interface
		/// or for making a connection to a remote endpoint.
		variant_endpoint(const variant_endpoint& other)
			:impl_(other.address(),other.port())
		{
		}

		variant_endpoint(const boost::asio::ip::address& addr, unsigned short port_num)
			: impl_(addr, port_num)
		{
		}

		template <typename InternetProtocol>
		variant_endpoint(const boost::asio::ip::basic_endpoint<InternetProtocol>& other)
			:impl_(other.address(),other.port())
		{
		}

		/// Assign from another endpoint.
		variant_endpoint& operator=(const variant_endpoint& other)
		{
			impl_.address(other.address());
			impl_.port(other.port());
			return *this;
		}
		template <typename InternetProtocol>
		variant_endpoint& operator=(const boost::asio::ip::basic_endpoint<InternetProtocol>& other)
		{
			impl_.address(other.address());
			impl_.port(other.port());
			return *this;
		}

		/// The protocol associated with the endpoint.
		template<typename ProtocolType>
		ProtocolType protocol() const
		{
			if (impl_.is_v4())
				return ProtocolType::v4();
			return ProtocolType::v6();
		}

		/// Get the underlying endpoint in the native type.
		data_type* data()
		{
			return impl_.data();
		}

		/// Get the underlying endpoint in the native type.
		const data_type* data() const
		{
			return impl_.data();
		}

		/// Get the underlying size of the endpoint in the native type.
		std::size_t size() const
		{
			return impl_.size();
		}

		/// Set the underlying size of the endpoint in the native type.
		void resize(std::size_t new_size)
		{
			impl_.resize(new_size);
		}

		/// Get the capacity of the endpoint in the native type.
		std::size_t capacity() const
		{
			return impl_.capacity();
		}

		/// Get the port associated with the endpoint. The port number is always in
		/// the host's byte order.
		unsigned short port() const
		{
			return impl_.port();
		}

		/// Set the port associated with the endpoint. The port number is always in
		/// the host's byte order.
		void port(unsigned short port_num)
		{
			impl_.port(port_num);
		}

		/// Get the IP address associated with the endpoint.
		boost::asio::ip::address address() const
		{
			return impl_.address();
		}

		/// Set the IP address associated with the endpoint.
		void address(const boost::asio::ip::address& addr)
		{
			impl_.address(addr);
		}

		/// Compare two endpoints for equality.
		friend bool operator==(const variant_endpoint& e1,
			const variant_endpoint& e2)
		{
			return e1.impl_ == e2.impl_;
		}

		/// Compare two endpoints for inequality.
		friend bool operator!=(const variant_endpoint& e1,
			const variant_endpoint& e2)
		{
			return !(e1 == e2);
		}

		/// Compare endpoints for ordering.
		friend bool operator<(const variant_endpoint& e1,
			const variant_endpoint& e2)
		{
			return e1.impl_ < e2.impl_;
		}

		/// Compare endpoints for ordering.
		friend bool operator>(const variant_endpoint& e1,
			const variant_endpoint& e2)
		{
			return e2.impl_ < e1.impl_;
		}

		/// Compare endpoints for ordering.
		friend bool operator<=(const variant_endpoint& e1,
			const variant_endpoint& e2)
		{
			return !(e2 < e1);
		}

		/// Compare endpoints for ordering.
		friend bool operator>=(const variant_endpoint& e1,
			const variant_endpoint& e2)
		{
			return !(e1 < e2);
		}

		template <typename InternetProtocol>
		operator boost::asio::ip::basic_endpoint<InternetProtocol>()const 
		{
			return boost::asio::ip::basic_endpoint<InternetProtocol>(address(),port());
		}

	private:
		// The underlying IP endpoint.
		boost::asio::ip::detail::endpoint impl_;
	};
#if !defined(BOOST_NO_IOSTREAM)
	template <typename Elem, typename Traits>
	std::basic_ostream<Elem, Traits>& operator<<(
		std::basic_ostream<Elem, Traits>& os,
		const variant_endpoint& endpoint)
	{
		boost::asio::ip::detail::endpoint tmp_ep(endpoint.address(), endpoint.port());
		boost::system::error_code ec;
		std::string s = tmp_ep.to_string(ec);
		if (ec)
		{
			if (os.exceptions() & std::basic_ostream<Elem, Traits>::failbit)
				boost::asio::detail::throw_error(ec);
			else
				os.setstate(std::basic_ostream<Elem, Traits>::failbit);
		}
		else
			for (std::string::iterator i = s.begin(); i != s.end(); ++i)
				os << os.widen(*i);
		return os;
	}
#endif//! def BOOST_NO_IOSTREAM
}

#endif // variant_enpoint_h__
