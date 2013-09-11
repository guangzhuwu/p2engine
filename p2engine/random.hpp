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


#ifndef P2ENGINE_RANDOM_HPP
#define P2ENGINE_RANDOM_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "p2engine/push_warning_option.hpp"
#include "p2engine/config.hpp"

#include <ctime>
#include <cfloat>

#include <boost/type_traits/make_unsigned.hpp>
#include <boost/type_traits/make_signed.hpp>
#include <boost/utility/enable_if.hpp>

namespace p2engine {

	//[0,2^32-1]
	inline uint32_t random()
	{
		uint32_t t;
		static uint32_t x=(uint32_t)(std::clock()+(int64_t)::getpid()+::time(NULL));
		static uint32_t y=362436069,z=521288629,w=88675123,v=5783321,d=6615241;
		t=(x^(x>>2)); x=y; y=z; z=w; w=v; v=(v^(v<<4))^(t^(t<<1)); 
		return (d+=362437)+v;
	}

	//[0,1)
	inline double random01()
	{
//#ifndef DBL_EPSILON
//#	define DBL_EPSILON     2.2204460492503131e-016 /* smallest such that 1.0+DBL_EPSILON != 1.0 */
//#endif
		static double m=1.0/
			static_cast<double>((std::numeric_limits<BOOST_TYPEOF(random())>::max)());
		return ((std::max<double>)(0.0,(double)random()-DBL_EPSILON))*m;
	}

	namespace detail{

		template<typename T>
		struct is_less_64bit_int{
			typedef T type;
			enum{value=((sizeof(T)<8)&&!boost::is_float<T>::value)};
		};

		template<typename T,typename Enable=void>
		struct range_random
		{
			T operator()(T from,T to)const
			{
				return static_cast<T>(from+(to-from)*random01());
			}
		};

		template<typename T>
		struct range_random<T,
			typename boost::enable_if<is_less_64bit_int<T> >::type
		>
		{
			T operator()(T from,T to)const
			{
				if (from==to)
					return from;
				return from+random()%(to-from);
			}
		};

	}

	//[from,to)
	template<typename T>
	inline T random(T from,T to)
	{
		return static_cast<T>(detail::range_random<T>()(from,to));
	}

	inline bool in_probability(double x)
	{
		return random01()<=x;
	}

	template<typename Iterator>
	inline Iterator random_select(Iterator begin,std::size_t n)
	{
		if((int)n>1)
			std::advance(begin,random<int>(0,(int)n)%(int)n);
		return begin;
	}

} // namespace p2engine

#include "p2engine/pop_warning_option.hpp"

#endif // P2ENGINE_RANDOM_HPP
