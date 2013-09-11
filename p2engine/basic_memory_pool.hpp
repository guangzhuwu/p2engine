//
// basic_memory_pool.hpp
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

#ifndef P2ENGINE_BASIC_MEMORY_POOL_HPP
#define P2ENGINE_BASIC_MEMORY_POOL_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "p2engine/push_warning_option.hpp"
#include "p2engine/config.hpp"

#if !defined(P2ENGINE_MEMPOOL_USE_STDMALLOC)\
	&&!defined(P2ENGINE_MEMPOOL_USE_NEDMALLOC)\
	&&!defined(P2ENGINE_MEMPOOL_USE_BOOST_POOL)\
	&&!defined(P2ENGINE_MEMPOOL_USE_RESUABLE)
# define P2ENGINE_MEMPOOL_USE_STDMALLOC
#endif

#ifdef P2ENGINE_MEMPOOL_USE_NEDMALLOC
#	include "p2engine/nedmalloc/nedmalloc.h"
#elif defined P2ENGINE_MEMPOOL_BOOST_POOL
#	include <boost/pool/pool.hpp>
#endif

#include <boost/thread/tss.hpp>
#include "p2engine/pop_warning_option.hpp"

#include "p2engine/singleton.hpp"
#include "p2engine/utilities.hpp"

namespace p2engine {

	//////////////////////////////////////////////////////////////////////////
	//default_user_allocator_malloc_free
	struct std_malloc_free
	{
		typedef std::size_t size_type;
		typedef std::ptrdiff_t difference_type;

		static char * malloc(const size_type bytes)
		{ return reinterpret_cast<char *>(::std::malloc(bytes)); }

		static void free(void * const block)
		{::std::free(block); }

		static char * realloc(void * const block, const size_type bytes)
		{return (char*)::std::realloc(block,bytes); }
	};

	//////////////////////////////////////////////////////////////////////////
	//nedmalloc_malloc_free
#if defined P2ENGINE_MEMPOOL_USE_STDMALLOC
	typedef std_malloc_free default_allocator;

#elif defined P2ENGINE_MEMPOOL_USE_NEDMALLOC
	struct nedmalloc_malloc_free
	{
		typedef std::size_t size_type;
		typedef std::ptrdiff_t difference_type;

		static char * malloc(const size_type bytes)
		{ return reinterpret_cast<char *>(nedalloc::nedmalloc(bytes)); }

		static void free(void * const block)
		{ nedalloc::nedfree(block); }

		static char * realloc(void * const block, size_type bytes)
		{ return reinterpret_cast<char *>(nedalloc::nedrealloc(block,bytes)); }
	};
	typedef nedmalloc_malloc_free default_allocator;

#elif defined P2ENGINE_MEMPOOL_USE_BOOST_POOL
	//////////////////////////////////////////////////////////////////////////
	//basic_boost_pool_memory_allocator
	template <typename UserAllocator=std_malloc_free>
	class basic_boost_pool_memory_allocator
	{
		typedef basic_boost_pool_memory_allocator<UserAllocator> this_type;
		SINGLETON_ACCESS_DECLARE;
	protected:
		enum{HEADER_SIZE=8};
		enum{BLOCK_SIZE_BIT=3};
		enum{BLOCK_SIZE=(1<<BLOCK_SIZE_BIT)};
		enum{MAX_BLOCK_CNT=0xff};

		typedef UserAllocator user_allocator_type;
		typedef boost::pool<user_allocator_type> pool_type;
		struct pools
		{
			typedef pool_type* pool_ptr_type;
			pool_ptr_type& operator pool_type[](size_t i){return pools_[i];}
			const pool_ptr_type& operator pool_type[]const (size_t i){return pools_[i];}
			pools()
			{
				for(int32_t i=0;i<MAX_BLOCK_CNT;++i)
				{
					std::size_t bytes=HEADER_SIZE+(i+1)*BLOCK_SIZE;
					pools_[i]=new pool_type(bytes);
				}
			}
			~pools(){
				lock_type lock(this);
				for(int32_t i=0;i<MAX_BLOCK_CNT;++i)
				{
					delete pools_[i];
				}
			}
		private:
			pool_ptr_type pools_[MAX_BLOCK_CNT];
		};
		typedef boost::thread_specific_ptr<pools> thread_specific_pool;
	public:
		static void* malloc(std::size_t bytes)
		{
			if (!pools_.get())
				pools_.reset(new pools);

			BOOST_ASSERT(bytes>0);
			int32_t index=int32_t(int32_t(bytes-1)>>BLOCK_SIZE_BIT);
			void* p=NULL;
			if (index>=MAX_BLOCK_CNT)
			{
				p=(void*)(user_allocator_type::malloc(bytes+HEADER_SIZE));
				index=MAX_BLOCK_CNT;
			}
			else
			{
				BOOST_ASSERT((*pools_)[index]->get_requested_size()>=bytes+HEADER_SIZE);
				p=(void*)((*pools_)[index]->malloc());
			}
			if (!p)
				return p;
			DEBUG_SCOPE(*reinterpret_cast<uint64_t*>(p)=0);
			(*reinterpret_cast<uint8_t*>(p))=(uint8_t)(index);
			return (reinterpret_cast<int8_t*>(p) + HEADER_SIZE);
		}
		static void free(void *pointer)
		{
			if (!pools_.get())
				pools_.reset(new pools);

			BOOST_ASSERT(pointer);
			uint8_t* p = reinterpret_cast<uint8_t*>(pointer);
			p -= (std::ptrdiff_t)HEADER_SIZE;
			uint8_t index = *(reinterpret_cast<uint8_t*>(p));
			DEBUG_SCOPE(*(reinterpret_cast<uint8_t*>(p))=0;BOOST_ASSERT(*reinterpret_cast<uint64_t*>(p)==0));
			if (index==MAX_BLOCK_CNT)
				user_allocator_type::free((char*)p);
			else
			{
				BOOST_ASSERT((*pools_)[index]->is_from(p));
				(*pools_)[index]->free(p);
			}
		}
	private:
		static thread_specific_pool pools_;
	};
	template<typename UserAllocator>
	typename basic_boost_pool_memory_allocator<UserAllocator>::thread_specific_pool
		basic_boost_pool_memory_allocator<UserAllocator>::pools_;
	typedef basic_boost_pool_memory_allocator<> boost_pool_memory_allocator;

	typedef boost_pool_memory_allocator default_allocator;

#elif defined(P2ENGINE_MEMPOOL_USE_RESUABLE)
	//////////////////////////////////////////////////////////////////////////
	//basic_reusable_memory_allocator
#	if defined(_MSC_VER)
#		pragma warning (push)
#		pragma warning(disable : 4267)
#	endif
	template <typename UserAllocator=std_malloc_free>
	class  basic_reusable_memory_allocator
	{
		typedef basic_reusable_memory_allocator<UserAllocator> this_type;

	protected:
		typedef uint8_t mark_type;
		enum{MAX_INDEX_CNT=0xFF};
		enum{BLOCK_SIZE=8};
		enum{MAX_SIZE=BLOCK_SIZE*MAX_INDEX_CNT};

		class  PtrPtrVec
		{
		public:
			PtrPtrVec()
			{
				BOOST_ASSERT((std::numeric_limits<mark_type>::max)()>=MAX_INDEX_CNT);
				memset(buf_,0,sizeof(buf_));
				memset(size_map_,0,sizeof(size_map_));
				memset(capacity_map_,0,sizeof(capacity_map_));
				for (int32_t i=0;i<MAX_INDEX_CNT+1;i++)
				{
					max_capacity_map_[i]=(32*1024*1024)/(BLOCK_SIZE*(i+1));
				}
			}
			void push_back(int32_t which,void* p)
			{
				if( capacity_map_[which]>=max_capacity_map_[which] )
				{
					UserAllocator::free((char*)((int8_t*)p-BLOCK_SIZE));
					return;
				}
				if (capacity_map_[which]<=size_map_[which])
				{
					if (capacity_map_[which]==0)
					{
						capacity_map_[which]=8;
						buf_[which]=(void**)UserAllocator::malloc(capacity_map_[which]*sizeof(void**));
					}
					else
					{
						if(capacity_map_[which]<16*1024)
							capacity_map_[which]<<=1;
						else
							capacity_map_[which]+=1024;
						buf_[which]= (void**)UserAllocator::realloc(buf_[which],capacity_map_[which]*sizeof(void**));
					}
				}
				buf_[which][size_map_[which]]=p;
				++size_map_[which];
			}
			void* pop_back(int32_t which)
			{
				if (which>MAX_INDEX_CNT||size_map_[which]==0)
					return NULL;
				--size_map_[which];
				return buf_[which][size_map_[which]];
			}
			void clear()
			{
				for (std::size_t i=0;i<sizeof(buf_)/sizeof(buf_[0]);++i)
				{
					if (buf_[i]==NULL)
						continue;
					for(int32_t j=0;j<size_map_[i];j++)
					{
						int8_t* ptr=reinterpret_cast<int8_t*>(buf_[i][j]);
						ptr-=BLOCK_SIZE;
						UserAllocator::free(ptr);
					}
					UserAllocator::free(buf_[i]);
				}
			}
			int32_t size(std::size_t which)
			{
				return size_map_[which];
			}
			int32_t capacity(std::size_t which)
			{
				return capacity_map_[which];
			}
		private:
			void** buf_[MAX_INDEX_CNT+1]; 
			int32_t size_map_[MAX_INDEX_CNT+1];
			int32_t capacity_map_[MAX_INDEX_CNT+1];
			int32_t max_capacity_map_[MAX_INDEX_CNT+1];
		};

		typedef boost::thread_specific_ptr<PtrPtrVec> thread_specific_pool;

	public:
		static void* malloc(std::size_t n)
		{
			if (!pools_.get())
				pools_.reset(new PtrPtrVec);

			if (n==0)
				n=1;
			BOOST_STATIC_ASSERT(BLOCK_SIZE==8);
			std::size_t n8=((n+7)>>3);//8bytes a unit
			void * p=NULL;
			if(n8<=MAX_INDEX_CNT)
			{
				p=pools_->pop_back((int32_t)n8);
				if (p==NULL)
				{
					//BLOCK_SIZE is used to contain length
					void* pl =UserAllocator::malloc((n8<<3)+ BLOCK_SIZE);
					if(!pl)
						return NULL;
					BOOST_ASSERT(mark_type(n8)==n8);
					*(mark_type*)(pl)=(mark_type)n8;
					p = ((int8_t*)pl)+BLOCK_SIZE;

				}
				else 
				{
				}
			}
			else
			{
				//BLOCK_SIZE is used to contain length
				void* pl= UserAllocator::malloc(n+BLOCK_SIZE);
				if(!pl)
					return NULL;

				//0 means not reuseable
				*(mark_type*)(pl)=(mark_type)0;
				p = ((int8_t*)pl)+BLOCK_SIZE;
			}
			return p;
		}
		static void free(void* p)
		{
			if (!pools_.get())
				pools_.reset(new PtrPtrVec);

			if(!p)
				return;
			int8_t* real_p=((int8_t*)p-BLOCK_SIZE);
			mark_type len_8 = *(mark_type*)real_p; // 8bytes a unite
			if(!len_8)
			{
				UserAllocator::free((char*)real_p);
			}
			else
			{
				pools_->push_back((int32_t)len_8,p);
			}
		}

	private:
		static thread_specific_pool pools_;
	};
	template<typename UserAllocator>
	typename basic_reusable_memory_allocator<UserAllocator>::thread_specific_pool
		basic_reusable_memory_allocator<UserAllocator>::pools_;

	typedef basic_reusable_memory_allocator<> reusable_memory_allocator;

	typedef reusable_memory_allocator default_allocator;

#	if defined(_MSC_VER)
#		pragma warning (pop)
#	endif

#endif

	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	//basic_memory_pool
	template < typename UserAllocator=default_allocator>
	class basic_memory_pool
	{
		typedef basic_memory_pool<UserAllocator> this_type;
		SINGLETON_ACCESS_DECLARE;
	public:
		static void* malloc(std::size_t bytes)
		{
			if (bytes==0)
				bytes=1;
			return UserAllocator::malloc(bytes);
		}
		static void free(void * const pointer)
		{
			if (!pointer)
				return;
			UserAllocator::free(pointer);
		}
	};

	typedef basic_memory_pool<> memory_pool;

} // namespace p2engine

#endif // p2engine_BASIC_MEMORY_POOL_HPP

