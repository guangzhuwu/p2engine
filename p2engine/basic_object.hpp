//
// basic_object.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2009-2010  GuangZhu Wu
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

#ifndef P2ENGINE_BASIC_OBJECT_HPP
#define P2ENGINE_BASIC_OBJECT_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "p2engine/push_warning_option.hpp"
#include "p2engine/config.hpp"
#include <boost/assert.hpp>
#include <boost/cstdint.hpp>
#include <boost/enable_shared_from_this.hpp>
#include "p2engine/pop_warning_option.hpp"

#include "p2engine/typedef.hpp"
#include "p2engine/object_allocator.hpp"
#include "p2engine/shared_access.hpp"
#include "p2engine/handler_allocator.hpp"

namespace p2engine {

	class basic_object
		:public object_allocator
		,public handler_allocator
		,public boost::enable_shared_from_this<basic_object>
	{
		typedef basic_object this_type;
		SHARED_ACCESS_DECLARE;

	public:
		typedef int64_t object_count_type;
		typedef int64_t object_id_type;

	protected:
		basic_object() 
			:obj_desc_("")
		{
			DEBUG_SCOPE(
				destruction_mark_=0x1927438376243739LL;
			);
		}
		virtual ~basic_object()
		{
			DEBUG_SCOPE(
				BOOST_ASSERT(destruction_mark_==0x1927438376243739LL);
			destruction_mark_=0;
			);
		}

	private:  // make this noncopyable
		basic_object(basic_object&);
		const basic_object& operator=( const basic_object& );

	public:
		template<typename NetObject>
		inline boost::shared_ptr<NetObject> shared_obj_from_this()
		{
			assert(boost::dynamic_pointer_cast<NetObject>(this->shared_from_this()));
			return boost::static_pointer_cast<NetObject>(this->shared_from_this());
		}
		template<typename NetObject>
		inline boost::shared_ptr<const NetObject> shared_obj_from_this()const
		{
			assert(boost::dynamic_pointer_cast<const NetObject>(this->shared_from_this()));
			return boost::static_pointer_cast<const NetObject>(this->shared_from_this());
		}
		const char* get_obj_desc() const{return obj_desc_;}
		void set_obj_desc(const char* desc)
		{
			BOOST_ASSERT(desc != NULL);
			obj_desc_ = desc;
		}
	private:
		const char* obj_desc_;
		DEBUG_SCOPE(
			int64_t destruction_mark_;
		);
	};

	PTR_TYPE_DECLARE(basic_object);

#define SHARED_OBJ_FROM_THIS basic_object::shared_obj_from_this<this_type>()
#define	OBJ_PROTECTOR(protector) boost::shared_ptr<this_type>protector=SHARED_OBJ_FROM_THIS

} // namespace p2engine


#endif // P2ENGINE_BASIC_OBJECT_HPP
