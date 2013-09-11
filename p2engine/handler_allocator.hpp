//
// handler_allocator.hpp
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

#ifndef P2ENGINE_HANDLER_ALLOCATOR_HPP
#define P2ENGINE_HANDLER_ALLOCATOR_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "p2engine/config.hpp"
#include "p2engine/object_allocator.hpp"

namespace p2engine{

	// Wrapper class template for handler objects to allow handler memory
	// allocation to be customised. Calls to operator() are forwarded to the
	// encapsulated handler.
	struct handler_allocator
	{
		typedef object_allocator::memory_pool_type memory_pool_type;

		friend void* asio_handler_allocate(std::size_t size,
			handler_allocator* /*this_handler*/)
		{
			return memory_pool_type::malloc(size);
		}

		friend void asio_handler_deallocate(void* pointer, std::size_t /*size*/,
			handler_allocator* /*this_handler*/)
		{
			return memory_pool_type::free(pointer);
		}
	};


#ifdef P2ENGINE_MEMPOOL_USE_STDMALLOC

	template <typename Handler>
	struct handler_allocator_wrap
		:public handler_allocator
	{
		typedef handler_allocator_wrap<Handler> wrap_type;

#	ifdef BOOST_HAS_RVALUE_REFS
		handler_allocator_wrap(Handler&& handler)
			:handler_(std::forward<Handler>(handler))
		{
		}
#	else
		handler_allocator_wrap(const Handler& handler)
			:handler_(handler)
		{
		}
#	endif

		void operator()()
		{
			handler_();
		}
		void operator()()const
		{
			handler_();
		}

		template <typename Arg1>
		void operator()(const Arg1& arg1)
		{
			handler_(arg1);
		}
		template <typename Arg1>
		void operator()(const Arg1& arg1)const
		{
			handler_(arg1);
		}

		template <typename Arg1, typename Arg2>
		void operator()(const Arg1& arg1, const Arg2& arg2)
		{
			handler_(arg1, arg2);
		}
		template <typename Arg1, typename Arg2>
		void operator()(const Arg1& arg1, const Arg2& arg2)const
		{
			handler_(arg1, arg2);
		}

		template <typename Arg1, typename Arg2, typename Arg3>
		void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3)
		{
			handler_(arg1, arg2, arg3);
		}
		template <typename Arg1, typename Arg2, typename Arg3>
		void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3)const
		{
			handler_(arg1, arg2, arg3);
		}

		template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
		void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3,const Arg4& arg4)
		{
			handler_(arg1, arg2, arg3, arg4);
		}
		template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
		void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3,const Arg4& arg4)const
		{
			handler_(arg1, arg2, arg3, arg4);
		}

		template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
		void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3,const Arg4& arg4,const Arg5& arg5)
		{
			handler_(arg1, arg2, arg3, arg4,arg5);
		}
		template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
		void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3,const Arg4& arg4,const Arg5& arg5)const
		{
			handler_(arg1, arg2, arg3, arg4,arg5);
		}

	private:
		Handler handler_;
	};

#	ifdef BOOST_HAS_RVALUE_REFS
	template <typename Handler>
	inline BOOST_DEDUCED_TYPENAME handler_allocator_wrap<Handler>::wrap_type 
		make_alloc_handler(Handler&& h)
	{
		return handler_allocator_wrap<Handler>(std::forward<Handler>(h));
	}
#	else // defined(BOOST_HAS_RVALUE_REFS)
	template <typename Handler>
	inline BOOST_DEDUCED_TYPENAME handler_allocator_wrap<Handler>::wrap_type 
		make_alloc_handler(const Handler& h)
	{
		return handler_allocator_wrap<Handler>(h);
	}
#	endif // defined(BOOST_HAS_RVALUE_REFS)

#else//P2ENGINE_MEMPOOL_USE_STDMALLOC

	template <typename Handler>
	struct handler_allocator_wrap
	{
		typedef Handler wrap_type;
	};

	template <typename Handler>
	inline BOOST_DEDUCED_TYPENAME const handler_allocator_wrap<Handler>::wrap_type& 
		make_alloc_handler(const Handler& h)
	{
		return h;
	}

#endif//P2ENGINE_MEMPOOL_USE_STDMALLOC


}

#endif//P2ENGINE_HANDLER_ALLOCATOR_HPP

