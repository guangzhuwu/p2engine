//
// io.hpp
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

#ifndef P2ENGINE_IO_HPP
#define P2ENGINE_IO_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "p2engine/push_warning_option.hpp"
#include "p2engine/config.hpp"
#include "p2engine/utilities.hpp"

#include <boost/cstdint.hpp>
#include <boost/static_assert.hpp>

#include <string>

namespace p2engine{

	template <typename T, typename InIterator>
	inline T read_value(InIterator& start, bool advance=true,std::size_t bytesize=sizeof(T))
	{
		BOOST_STATIC_ASSERT(boost::is_pointer<InIterator>::value);
		BOOST_STATIC_ASSERT(sizeof(*start)==1);
		BOOST_STATIC_ASSERT(boost::has_trivial_copy<T>::value);

		T rst;
		memcpy(&rst,&*start,bytesize);
		if (advance) start+=bytesize;

		return rst;
	}

	template <typename T, typename OutIterator>
	inline void write_value(T val, OutIterator& start,bool advance=true, std::size_t bytesize=sizeof(T))
	{
		BOOST_STATIC_ASSERT(boost::is_pointer<OutIterator>::value);
		BOOST_STATIC_ASSERT(sizeof(*start)==1);
		BOOST_STATIC_ASSERT(boost::has_trivial_copy<T>::value);

		memcpy(&*start,&val,bytesize);
		if (advance) start+=bytesize;
	}

	template <typename T, typename InIterator>
	inline T read_int(InIterator& start, bool advance=true, std::size_t bytesize=sizeof(T))
	{
		BOOST_STATIC_ASSERT(boost::is_float<T>::value||boost::is_integral<T>::value);
		return read_value<T>(start,advance,bytesize);
	}
	template <typename T, typename InIterator>
	inline T read_int_ntoh(InIterator& start,  bool advance=true, std::size_t bytesize=sizeof(T))
	{
		return ntoh<T>()(read_int<T,InIterator>(start,advance,bytesize));
	}

	template <typename T, typename OutIterator>
	inline void write_int(T val, OutIterator& start,bool advance=true,std::size_t bytesize=sizeof(T))
	{
		BOOST_STATIC_ASSERT(boost::is_float<T>::value||boost::is_integral<T>::value);
		return write_value<T>(val,start,advance,bytesize);
	}

	template <typename T, typename OutIterator>
	inline void write_int_hton(T val, OutIterator& start,bool advance=true,std::size_t bytesize=sizeof(T))
	{
		write_int<T,OutIterator>(hton<T>()(val),start,advance,bytesize);
	}

	// -- adaptors
#ifndef __P2ENGINE_READ_WRITE_INT_FUNT
#	define __P2ENGINE_READ_WRITE_INT_FUNT(x)\
	template <class InIterator>\
	inline boost::x##_t read_##x(InIterator& start,bool advance=true)\
	{ return read_int<boost::x##_t>(start,advance);}\
	\
	template <class InIterator>\
	inline boost::x##_t read_##x##_ntoh(InIterator& start,bool advance=true)\
	{ return read_int_ntoh<boost::x##_t>(start,advance); }\
	\
	template <class OutIterator>\
	inline void write_##x(boost::x##_t val, OutIterator& start,bool advance=true)\
	{ write_int<boost::x##_t,OutIterator>(val, start,advance); }\
	\
	template <class OutIterator>\
	inline void write_##x##_hton(boost::x##_t val, OutIterator& start,bool advance=true)\
	{ write_int_hton<boost::x##_t,OutIterator>(val, start,advance); }

#endif
	__P2ENGINE_READ_WRITE_INT_FUNT(int64)
		__P2ENGINE_READ_WRITE_INT_FUNT(uint64)
		__P2ENGINE_READ_WRITE_INT_FUNT(int32)
		__P2ENGINE_READ_WRITE_INT_FUNT(uint32)
		__P2ENGINE_READ_WRITE_INT_FUNT(int16)
		__P2ENGINE_READ_WRITE_INT_FUNT(uint16)
		__P2ENGINE_READ_WRITE_INT_FUNT(int8)
		__P2ENGINE_READ_WRITE_INT_FUNT(uint8)

#undef __P2ENGINE_READ_WRITE_INT_FUNT
	template <class OutIt>
	void write_string(std::string const& str, OutIt& start,bool advance=true)
	{
		std::copy(str.begin(), str.end(), start);
		if (advance)
			std::advance(start,str.size());
	}
}

#include "p2engine/pop_warning_option.hpp"

#endif // P2ENGINE_IO_HPP
