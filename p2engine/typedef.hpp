//
// typedef.hpp
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
#ifndef P2ENGINE_TYPEDEF_HPP
#define P2ENGINE_TYPEDEF_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "p2engine/push_warning_option.hpp"
#include "p2engine/config.hpp"
// if building as Objective C++, asio's template
// parameters Protocol has to be renamed to avoid
// colliding with keywords
#ifdef __OBJC__
#define Protocol Protocol_
#endif
#include <boost/version.hpp>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#ifdef __OBJC__
#undef Protocol
#endif
#include "p2engine/pop_warning_option.hpp"

#include "p2engine/variant_endpoint.hpp"

#define _INT_T_2_INT(bit)\
	typedef int##bit##_t   int##bit;\
	typedef uint##bit##_t  uint##bit;

#ifdef BOOST_HAS_STDINT_H
#	define DEF_INTTYPE 

#else
#	define _USING_BOOST_INT(bit)\
	typedef boost::int##bit##_t   int##bit##_t;\
	typedef boost::uint##bit##_t  uint##bit##_t;\
	_INT_T_2_INT(bit);

#	define DEF_INTTYPE\
	_USING_BOOST_INT(64);\
	_USING_BOOST_INT(32);\
	_USING_BOOST_INT(16);\
	_USING_BOOST_INT(8);
#endif
namespace p2engine
{

	//asio
#if BOOST_VERSION >= 103500
	namespace asio = boost::asio;
	typedef boost::system::error_code error_code;
#else
	typedef asio::error_code error_code;
#endif

	using asio::async_write;
	using asio::async_read;

	typedef asio::ip::tcp tcp;
	typedef asio::ip::udp udp;
	typedef variant_endpoint endpoint;
	typedef asio::ip::address address;
	typedef asio::ip::address_v4 address_v4;
	typedef asio::ip::address_v6 address_v6;
	typedef asio::io_service io_service;


	//inttype
	DEF_INTTYPE;


	//type used by p2engine
	typedef	uint16_t message_type;

}//namespace p2engine

#endif//P2ENGINE_TYPEDEF_HPP

