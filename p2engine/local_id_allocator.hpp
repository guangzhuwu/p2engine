//
// basic_local_id_allocator.hpp
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
/***  -- basic_local_id_allocator.h --                                      */
/* Authors: Meng ZHANG <albert.meng.zhang@gmail.com>                  */
/*                                                                    */
/* Copyright (C) 2007, Meng ZHANG,                                    */
/* in Dept. of Computer Sci. & Tech.,                                 */
/* Tsinghua Univ., Beijing 100084, P.R.China                          */
/* MSN: Albert.Meng.ZHANG@hotmail.com                                 */
/* Skype: Albert.Meng.ZHANG                                           */
/* Homepage: http://media.cs.tsinghua.edu.cn/~zhangm                  */
/* All rights reserved.                                               */
/******/

#ifndef P2ENGINE_BASIC_LOCAL_ID_ALLOCATOR_HPP
#define P2ENGINE_BASIC_LOCAL_ID_ALLOCATOR_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "p2engine/push_warning_option.hpp"
#include "p2engine/config.hpp"
#include <boost/unordered_set.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/identity.hpp>

#include "p2engine/basic_object.hpp"

namespace p2engine {

	template<typename IdType>
	class basic_local_id_allocator 
		: public basic_object
	{
		typedef basic_local_id_allocator<IdType> this_type;
		SHARED_ACCESS_DECLARE;
	public:
		static shared_ptr create(bool reuse_id=false, int  low_water_mark=0)
		{
			return shared_ptr(new this_type(reuse_id),
				shared_access_destroy<this_type>());
		}

		basic_local_id_allocator(bool reuse_id = false, int  low_water_mark=0) 
			:tail_id_(IdType()), reuse_id_(reuse_id),low_water_mark_(low_water_mark)
		{
			this->set_obj_desc("basic_local_id_allocator");
		};

		virtual ~basic_local_id_allocator()
		{
		};

	public:
		void set_start_id(const IdType& start_id)
		{
			tail_id_=start_id;
		}
		void add_reserved_id(const IdType& reserved_id)
		{
			reserved_id_set_.insert(reserved_id);
		};

		IdType alloc_id()
		{
			if(reuse_id_)
			{
				while (avail_id_set_.size() > (std::size_t)low_water_mark_)
				{
					BOOST_AUTO(&orderedUniqueIndex,boost::multi_index::get<1>(avail_id_set_));
					BOOST_AUTO(begin,orderedUniqueIndex.begin());
					IdType ret = *begin;
					orderedUniqueIndex.erase(begin);
					if(reserved_id_set_.find(ret) == reserved_id_set_.end())
						return ret;
				}
			}
			while(reserved_id_set_.find(tail_id_) != reserved_id_set_.end())
			{
				++tail_id_;
			}
			return tail_id_++;
		}

		void release_id(const IdType& released_id)
		{
			if (!reuse_id_) return;
			if (reserved_id_set_.find(released_id) != reserved_id_set_.end()) return;
			if (released_id >= 0 && released_id <tail_id_)
			{
				boost::multi_index::get<0>(avail_id_set_).insert(released_id);
			}
		}

		void reset()
		{
			tail_id_=IdType();
			avail_id_set_.clear();
		}
		//bool is_allocated(IdType id)
		//{
		//	return id >= IdType() && id <= tail_id_ 
		//		&& avail_id_set_.find(id) == avail_id_set_.end();
		//}

	protected:
		boost::multi_index::multi_index_container<
			IdType,
			boost::multi_index::indexed_by<
			boost::multi_index::ordered_unique<boost::multi_index::identity<IdType> >,
			boost::multi_index::sequenced<>
			>
		>avail_id_set_;
		boost::unordered_set<IdType> reserved_id_set_;
		IdType tail_id_;
		bool reuse_id_;
		int low_water_mark_;
	};

} // namespace p2engine

#include "p2engine/pop_warning_option.hpp"

#endif //P2ENGINE_BASIC_LOCAL_ID_ALLOCATOR_HPP
