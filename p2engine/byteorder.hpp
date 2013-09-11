//
// byteorder.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2009  GuangZhu Wu 
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


#ifndef P2ENGINE_BYTEORDER_HPP
#define P2ENGINE_BYTEORDER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "p2engine/push_warning_option.hpp"
#include "p2engine/config.hpp"

#include <boost/detail/endian.hpp>

#if defined(BOOST_BIG_ENDIAN)&&defined(BOOST_LITTLE_ENDIAN)
#	error BOOST_BIG_ENDIAN and BOOST_LITTLE_ENDIAN are both defined! ONLY ONE can be defined!
#endif

#if defined(__linux__) || defined (__GLIBC__) || defined(__GNU__)
#	include <endian.h>
#	include <byteswap.h>
#elif defined(__APPLE__)
#	include <machine/endian.h>
#	include <libkern/OSByteOrder.h>
#	define bswap_16 OSSwapInt16
#	define bswap_32 OSSwapInt32
#	define bswap_64 OSSwapInt64
#elif defined(__FreeBSD__) || defined(__DragonFlyBSD__) || defined(__NetBSD__)
#	include <sys/endian.h>
#	define bswap_16 bswap16
#	define bswap_32 bswap32
#	define bswap_64 bswap64
#elif defined(__OpenBSD__)
#	include <machine/endian.h>
#	define bswap_16 __swap16
#	define bswap_32 __swap32
#	define bswap_64 __swap64
#elif defined(__sun)
#	include <sys/byteorder.h>
#	define bswap_16 BSWAP_16
#	define bswap_32 BSWAP_32
#	define bswap_64 BSWAP_64
#elif defined(_MSC_VER)
#	include <stdlib.h>
#	define bswap_16 _byteswap_ushort
#	define bswap_32 _byteswap_ulong
#	define bswap_64 _byteswap_uint64
#else
#	include "p2engine/macro.hpp"
WARNING(No bswap available for your platform, using ntos);
inline uint64_t bswap_64(uint64_t val)
{
	return (
		((val) >> 56) | 
		((val & 0x00ff000000000000ULL) >> 40) |
		((val & 0x0000ff0000000000ULL) >> 24) | 
		((val & 0x000000ff00000000ULL) >> 8) | 
		((val & 0x00000000ff000000ULL) << 8) | 
		((val & 0x0000000000ff0000ULL) << 24) | 
		((val & 0x000000000000ff00ULL) << 40) | 
		((val) << 56)
		);
}
#endif

namespace p2engine {

	inline bool is_host_order_big_endian()
	{
		static const long host_one= 1;
		return !(*reinterpret_cast<const boost::uint8_t*>(&host_one));
	}

	inline bool is_host_order_little_endian()
	{
		return !is_host_order_big_endian();
	}

	namespace detail{
		static struct endian_assert 
		{
			endian_assert()
			{
#ifdef BOOST_BIG_ENDIAN
				if(!is_host_order_big_endian())
					abort();
#elif defined(BOOST_LITTLE_ENDIAN)
				if(is_host_order_big_endian())
					abort();
#endif
			}
		}endian_assert;
	}

	inline uint64_t htonll(uint64_t hostlonglong)
	{
#ifdef BOOST_BIG_ENDIAN
		return hostlonglong;
#elif defined(BOOST_LITTLE_ENDIAN)
		return bswap_64(hostlonglong);
#else
		return (is_host_order_big_endian()) ? hostlonglong : bswap_64(hostlonglong);
#endif
	}

	inline uint64_t ntohll(uint64_t netlonglong)
	{
		return htonll(netlonglong);
	}

	template<typename T, std::size_t N>
	struct hton_with_n_bytes
	{
	public:
		T operator()( T h) const{return h;}
	};

	template<typename T>
	struct hton_with_n_bytes<T, static_cast<std::size_t>(2)>
	{
	public:
		T operator()(T h)const {return T(htons((uint16_t)h));}
	};

	template<typename T>
	struct hton_with_n_bytes<T, static_cast<std::size_t>(4)>
	{
	public:
		T operator()(T h)const {return T(htonl((uint32_t)h));}
	};

	template<typename T>
	struct hton_with_n_bytes<T, static_cast<std::size_t>(8)>
	{
	public:
		T operator()(T h)const {return T(htonll((uint64_t)h));}
	};

	template<typename T, std::size_t N>
	struct bswap_with_n_bytes
	{
	public:
		T operator()( T h) const{return h;}
	};

	template<typename T>
	struct bswap_with_n_bytes<T, static_cast<std::size_t>(2)>
	{
	public:
		T operator()(T h)const {return T(bswap_16((uint16_t)h));}
	};

	template<typename T>
	struct bswap_with_n_bytes<T, static_cast<std::size_t>(4)>
	{
	public:
		T operator()(T h)const {return T(bswap_32((uint32_t)h));}
	};

	template<typename T>
	struct bswap_with_n_bytes<T, static_cast<std::size_t>(8)>
	{
	public:
		T operator()(T h)const {return T(bswap_64((uint64_t)h));}
	};


	template<typename T>
	struct hton
	{
	public:
		T operator()(T h)const {return hton_with_n_bytes<T, sizeof(T)>()(h);}
	};
	template<typename T>
	struct ntoh
	{
	public:
		T operator()(T n)const {return hton_with_n_bytes<T, sizeof(T)>()(n);}
	};
	template<typename T>
	struct bswap
	{
	public:
		T operator()(T n)const {return bswap_with_n_bytes<T, sizeof(T)>()(n);}
	};
}

#include "p2engine/pop_warning_option.hpp"

#endif //P2ENGINE_BYTEORDER_HPP

