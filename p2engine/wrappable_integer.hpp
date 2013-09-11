//
// wrappable_integer.hpp
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
//
// THANKS  Meng Zhang <albert.meng.zhang@gmail.com>
//

#ifndef P2ENGINE_WRAPPABLE_INTEGER_HPP
#define P2ENGINE_WRAPPABLE_INTEGER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "p2engine/push_warning_option.hpp"
#include "p2engine/config.hpp"

#include <boost/operators.hpp>
#include <boost/type_traits.hpp>

namespace p2engine {

	template<class T>
	inline int wrappable_cmp( T val1,  T val2)
	{
		typedef typename boost::make_signed<T>::type signed_type;
		typedef typename boost::make_unsigned<T>::type unsigned_type;

		signed_type dist=static_cast<signed_type>((unsigned_type)val1 - (unsigned_type)val2);
		unsigned_type dist_down=static_cast<unsigned_type>(dist);
		unsigned_type dist_up=static_cast<unsigned_type>(-dist);

		if (dist_up<dist_down)
			return -1;
		else if (dist_up==dist_down)
			return 0;
		else
			return 1;
	}


	template<class T>
	inline  typename boost::make_signed<T>::type wrappable_sub( T val1,  T val2)
	{
		typedef typename boost::make_signed<T>::type signed_type;
		typedef typename boost::make_unsigned<T>::type unsigned_type;
		return signed_type((unsigned_type)val1 - (unsigned_type)val2);
	}

	template<class T>
	inline  T wrappable_average(T val1, T val2)
	{
		typedef typename boost::make_signed<T>::type signed_type;
		typedef typename boost::make_unsigned<T>::type unsigned_type;
		return val1+wrappable_sub<T>(val2,val1)/2;
	}

	template<class T>
	struct  wrappable_minus
		: public std::binary_function<T, T, typename boost::make_signed<T>::type>
	{	
		typedef typename boost::make_signed<T>::type signed_type;
		typedef typename boost::make_unsigned<T>::type unsigned_type;

		signed_type operator()( T _Left,  T _Right) const
		{	
			return signed_type((unsigned_type)_Left - (unsigned_type)_Right);
		}
	};

	template<class T>
	struct wrappable_less
		: public std::binary_function<T, T, bool>
	{
		typedef typename boost::make_signed<T>::type signed_type;
		typedef typename boost::make_unsigned<T>::type unsigned_type;

		bool operator()( T _Left,  T _Right) const
		{
			signed_type dist=static_cast<signed_type>((unsigned_type)_Left - (unsigned_type)_Right);
			return (unsigned_type)(-dist)<(unsigned_type)(dist);
		}
	};

	template<class T>
	struct wrappable_greater
		: public std::binary_function<T, T, bool>
	{
		typedef typename boost::make_signed<T>::type signed_type;
		typedef typename boost::make_unsigned<T>::type unsigned_type;

		bool operator()( T _Left,  T _Right) const
		{	
			return wrappable_less<T>()(_Right,_Left);
		}
		bool operator()( T _Left,T _Middle, T _Right) const
		{	
			return operator()(_Left,_Middle)&&operator()(_Middle,_Right);
		}
		bool operator()( T v1,T v2, T v3,T v4) const
		{	
			return operator()(v1,v2,v3)&&operator()(v3,v4);
		}
		bool operator()( T v1,T v2, T v3,T v4,T v5) const
		{	
			return operator()(v1,v2,v3,v4)&&operator()(v4,v5);
		}
	};

	template<class T>
	struct wrappable_less_equal
		: public std::binary_function<T, T, bool>
	{
		typedef typename boost::make_signed<T>::type signed_type;
		typedef typename boost::make_unsigned<T>::type unsigned_type;

		bool operator()( T _Left,  T _Right) const
		{	
			return !wrappable_less<T>()(_Right,_Left);
		}
		bool operator()( T _Left,T _Middle, T _Right) const
		{	
			return operator()(_Left,_Middle)&&operator()(_Middle,_Right);
		}
		bool operator()( T v1,T v2, T v3,T v4) const
		{	
			return operator()(v1,v2,v3)&&operator()(v3,v4);
		}
		bool operator()( T v1,T v2, T v3,T v4,T v5) const
		{	
			return operator()(v1,v2,v3,v4)&&operator()(v4,v5);
		}
	};

	template<class T>
	struct wrappable_greater_equal
		: public std::binary_function<T, T, bool>
	{
		typedef typename boost::make_signed<T>::type signed_type;
		typedef typename boost::make_unsigned<T>::type unsigned_type;

		bool operator()( T _Left,  T _Right) const
		{	
			return !wrappable_less<T>()(_Left,_Right);
		}
		bool operator()( T _Left,T _Middle, T _Right) const
		{	
			return operator()(_Left,_Middle)&&operator()(_Middle,_Right);
		}
		bool operator()( T v1,T v2, T v3,T v4) const
		{	
			return operator()(v1,v2,v3)&&operator()(v3,v4);
		}
		bool operator()( T v1,T v2, T v3,T v4,T v5) const
		{	
			return operator()(v1,v2,v3,v4)&&operator()(v4,v5);
		}
	};

	template<typename T> 
	class wrappable_integer
		: public boost::operators<wrappable_integer<T> >
		, public boost::shiftable<wrappable_integer<T> >
	{
		BOOST_STATIC_ASSERT((boost::is_integral<T>::value));
		typedef typename boost::make_signed<T>::type signed_type;
		typedef typename boost::make_unsigned<T>::type unsigned_type;

	public:
		typedef T int_type;
		wrappable_integer(int_type init_value) : value_(init_value) {}
		wrappable_integer() : value_(static_cast<int_type>(0)) {}
		wrappable_integer(const wrappable_integer& v) : value_(v.value_) {}

		template<class U>
		operator U()const
		{
			return (U)value_;
		}

		bool operator<(wrappable_integer val) const
		{
			return wrappable_less<int_type>()(value_,val.value_);
		}

		bool operator==(wrappable_integer val) const
		{
			return value_==val.value_;
		}

		bool operator==(int_type val) const 
		{
			return value_ == val;
		}

		wrappable_integer& operator+=(int_type val) 
		{
			value_+=val;return *this;
		}

		wrappable_integer& operator-=(int_type val) 
		{
			value_-=val; return *this;
		}

		wrappable_integer& operator*=( wrappable_integer val)
		{
			value_*=val.value_; return *this;
		}

		wrappable_integer& operator/=( wrappable_integer val)   
		{
			value_/=val.value_; return *this;
		}

		wrappable_integer& operator%=( wrappable_integer val)
		{
			value_%=val.value_; return *this;
		}

		wrappable_integer& operator|=( wrappable_integer val)
		{
			value_|=val.value_; return *this;
		}

		wrappable_integer& operator&=( wrappable_integer val)
		{
			value_&=val.value_; return *this;
		}

		wrappable_integer& operator^=( wrappable_integer val)
		{
			value_^=val.value_; return *this;
		}

		wrappable_integer& operator<<=( std::size_t val)
		{
			value_<<=val; return *this;
		}

		wrappable_integer& operator>>=( std::size_t val)
		{
			value_>>=val; return *this;
		}

		wrappable_integer& operator++() 
		{
			++value_; return *this;
		}
		wrappable_integer& operator--()
		{
			--value_; return *this;
		}

		wrappable_integer& operator=(const wrappable_integer& val) 
		{
			value_ = val.value_; return *this;
		}
		wrappable_integer& operator=(int_type val) 
		{
			value_ = val; return *this;
		}

		static unsigned_type max_distance()
		{
			return (std::numeric_limits<signed_type>::max)();
		}

	private:
		int_type value_;
	};

	template<class OStream, typename U>
	inline OStream& operator<<(OStream& os, const wrappable_integer<U>&val)
	{
		os << (U)val;
		return os;
	}

}//namespace p2engine

#include "p2engine/pop_warning_option.hpp"

#endif // P2ENGINE_WRAPPABLE_INTEGER_HPP

