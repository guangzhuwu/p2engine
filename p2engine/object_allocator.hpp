//
// object_allocator.hpp
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

#ifndef P2ENGINE_OBJECT_ALLOCATOR_HPP
#define P2ENGINE_OBJECT_ALLOCATOR_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "p2engine/basic_object_allocator.hpp"

namespace p2engine {

	typedef basic_object_allocator<> object_allocator;
} // namespace p2engine


#endif // P2ENGINE_MEMORY_POOL_HPP


