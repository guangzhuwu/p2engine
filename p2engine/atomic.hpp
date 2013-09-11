//
// atomic.hpp
// ~~~~~~~~~~~
//
// Copyright (c) 2008-2010 GuangZhu Wu (GuangZhuWu@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef BOOST_NETWORK_ATOMIC_HPP
#define BOOST_NETWORK_ATOMIC_HPP

#if defined( __GNUC__ )
#	if( __GNUC__ * 100 + __GNUC_MINOR__ >= 401 )
#		define USING_BOOST_PROCESS_ATOMIC 1
#	elif defined(__GLIBCPP__) || defined(__GLIBCXX__)
#		define USEING_GNU_ATOMIC
#	endif
#else
#	define USING_BOOST_PROCESS_ATOMIC 1
#endif

#include "p2engine/push_warning_option.hpp"

#ifdef USING_BOOST_PROCESS_ATOMIC
#	include <boost/interprocess/detail/atomic.hpp>
#elif defined(USEING_GNU_ATOMIC)
#	include <bits/atomicity.h>
#else
#	error atomic is unavailable!
#endif

#include "p2engine/pop_warning_option.hpp"

namespace boost{namespace interprocess{
		namespace ipcdetail{}
		namespace detail{}
}};
namespace p2engine{
	namespace detail
	{
#ifdef USING_BOOST_PROCESS_ATOMIC
		typedef boost::uint32_t atomic_t;
		using namespace  boost::interprocess::detail;
		using namespace  boost::interprocess::ipcdetail;
		//using boost::interprocess::detail::atomic_write32;
		//using boost::interprocess::detail::atomic_inc32;
		//using boost::interprocess::detail::atomic_dec32;
		//using boost::interprocess::detail::atomic_read32;
#else //USING_GLIBC_ATOMIC
		typedef _Atomic_word atomic_t;
#	if defined(__GLIBCXX__)
		using __gnu_cxx::__atomic_add;
		using __gnu_cxx::__exchange_and_add;
#	endif
		inline void atomic_write32(volatile atomic_t *mem, atomic_t val)
		{
			*mem = val;
		}
		inline atomic_t atomic_inc32(volatile atomic_t *mem)
		{
			return __exchange_and_add(mem,+1);
		}
		inline atomic_t atomic_dec32(volatile atomic_t *mem)
		{
			return __exchange_and_add(mem,-1);
		}
		inline atomic_t atomic_read32(volatile atomic_t *mem)
		{
			return __exchange_and_add(mem, 0);
		}
#endif
	}

	template <typename Integer32Type>
	class atomic
	{
	public:
		typedef Integer32Type integer_type;
		BOOST_STATIC_ASSERT(sizeof(integer_type)==4);

		explicit atomic(integer_type v=integer_type(0))
		{
			detail::atomic_write32(&value_,v);
		}

		integer_type operator++()
		{
			return (integer_type)detail::atomic_inc32(&value_)+integer_type(1);
		}

		integer_type operator--()
		{
			return (integer_type)detail::atomic_dec32(&value_)-integer_type(1);
		}
		integer_type operator++(int)
		{
			return (integer_type)detail::atomic_inc32(&value_);
		}

		integer_type operator--(int)
		{
			return (integer_type)detail::atomic_dec32(&value_);
		}
		operator integer_type() const
		{
			return detail::atomic_read32(&value_);
		}

		atomic & operator=(integer_type v)
		{
			detail::atomic_write32(&value_,v);
			return *this;
		}

	private:
		atomic & operator=(const atomic& v);

	private:
		mutable detail::atomic_t value_;
	};

}

#endif//p2engine_atomic_hpp__
