//
// utilities.hpp
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


#ifndef P2ENGINE_UTILITIES_HPP
#define P2ENGINE_UTILITIES_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "p2engine/push_warning_option.hpp"
#include "p2engine/config.hpp"

#include "p2engine/byteorder.hpp"
#include "p2engine/random.hpp"

#include <boost/type_traits/make_unsigned.hpp>
#include <boost/type_traits/make_signed.hpp>
#include <boost/functional/hash.hpp>

namespace p2engine {

	template<std::size_t HashBits>
	struct get_str_hash_value
	{
		BOOST_STATIC_ASSERT(HashBits <= 64);
		typedef typename boost::uint_t<HashBits>::least hash_value_type;
		hash_value_type operator()(const std::string& msg_value_name)
		{
			hash_value_type hash_val = boost::hash<std::string>()(msg_value_name);
			return (hash_val & ((1 << HashBits) - 1));
		}
	};

	template<>
	struct get_str_hash_value<0>
	{
		typedef boost::uint_t<0>::least hash_value_type;
		template<typename String>
		hash_value_type operator()(String msg_value_name)
		{
			return 0;
		}
	};

	template <class OStream, std::size_t FieldWidth, typename T, bool IS_INTEGRAL>
	struct stream_then_format_if_integral;

	template <class OStream, std::size_t FieldWidth, typename T>
	struct stream_then_format_if_integral<OStream, FieldWidth, T, false>
	{
		OStream& operator()(OStream& os, const T& val)
		{
			os.width(FieldWidth); os <<val;
			os.width(0); os.fill(' '); os <<std::dec;
			os.flush();
			return os;
		}
	};

	template <class OStream, typename T, std::size_t TypeSize>
	struct stream_integer_to_text
	{
		BOOST_STATIC_ASSERT(boost::is_integral<T>::value);
		OStream& operator()(OStream& os, const T &val)
		{
			os <<val;
			return os;
		}
	};

	template <class OStream, typename T>
	struct stream_integer_to_text<OStream, T, 1>
	{
		BOOST_STATIC_ASSERT(boost::is_integral<T>::value);
		OStream& operator()(OStream& os, const T &val)
		{
			os <<static_cast<uint32_t>(static_cast<typename boost::make_unsigned<T>::type >(val));
			return os;
		}
	};

	template <class OStream, std::size_t FieldWidth, typename T>
	struct stream_then_format_if_integral<OStream, FieldWidth, T, true>
	{
		OStream& operator()(OStream& os, const T &val)
		{
			BOOST_STATIC_ASSERT(boost::is_integral<T>::value);
			os.width(FieldWidth - sizeof(T) * static_cast<std::size_t>(2)); os.fill(' '); os <<' ';
			os.width(sizeof(T) * static_cast<std::size_t>(2)); os.fill('0');
			os <<std::hex<<std::uppercase;
			stream_integer_to_text<OStream, T, sizeof(T)>()(os, val);
			os.width(0); os.fill(' '); os <<std::dec;
			os.flush();
			return os;
		}
	};

	template<class OStream, std::size_t FieldWidth, typename T>
	struct stream_to_text
	{
		OStream& operator()(OStream& os, const T& val)
		{
			stream_then_format_if_integral<OStream, FieldWidth, T, boost::is_integral<T>::value>()(os, val);
			return os;
		}
	};

	//For boost::asio::ip::address_v4::bytes_type and boost::asio::ip::address_v6::bytes_type
	template<class OStream, class T, std::size_t N>
	OStream& operator<<(OStream& os, const boost::array<T, N>& arr)
	{
		os <<"(";
		typename boost::array<T, N>::const_iterator iter;
		for(iter = arr.begin(); iter != arr.end(); ++ iter)
		{
			stream_to_text<OStream, 0, T>() (os, *iter);
			os <<" ";
		}
		os <<")";
		return os;
	}


} // namespace p2engine

#include "p2engine/pop_warning_option.hpp"

#endif // P2ENGINE_UTILITIES_HPP
