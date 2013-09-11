//
// post_in_constructor.hpp
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

#ifndef post_in_constructor_h__
#define post_in_constructor_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "p2engine/intrusive_ptr_base.hpp"
#include "p2engine/fssignal.hpp"

namespace p2engine{

	class post_in_constructor
	{
		struct post_in_constructor_impl
			:public basic_intrusive_ptr<post_in_constructor_impl>
		{
			void emit(fssignal::signal<void(void)> exec)
			{
				exec();
			}
		};
	public:
		post_in_constructor(io_service& ios)
			:ios_(ios),impl_(new post_in_constructor_impl)
		{
		}
		void post(fssignal::signal<void(void)> exec)
		{
			ios_.post(make_alloc_handler(boost::bind(&post_in_constructor_impl::emit,impl_,exec)));
		}
	private:
		io_service&  ios_;
		boost::intrusive_ptr<post_in_constructor_impl> impl_;
	};
}


#endif // post_in_constructor_h__


