//
// get_derived_this.hpp
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

#ifndef P2ENGINE_GET_DERIVED_THIS_HPP
#define P2ENGINE_GET_DERIVED_THIS_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "p2engine/push_warning_option.hpp"
#include "p2engine/config.hpp"
#include "p2engine/utilities.hpp"

namespace p2engine{

	template<typename Base,typename Derived>
	struct get_derived_this
	{
		BOOST_STATIC_ASSERT((boost::is_base_and_derived<Base, Derived>::value));
		Derived* operator()(Base* ptr)const
		{
			Derived*p=(Derived*)(ptr);//!!DO NOT USING reinterpret_cast!!
			BOOST_ASSERT((uintptr_t)dynamic_cast<Derived*>(ptr)==(uintptr_t)(p));
			return p;
		}
	};

}

#endif // P2ENGINE_GET_DERIVED_THIS_HPP
