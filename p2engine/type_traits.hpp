//
// type_traits.hpp
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

# ifndef P2ENGINE_DETAIL_IS_XXX__HPP
# define P2ENGINE_DETAIL_IS_XXX__HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "p2engine/push_warning_option.hpp"

#include <boost/detail/is_xxx.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/intrusive_ptr.hpp>

#include <memory>

namespace p2engine { namespace detail{

		namespace mpl=boost::mpl;

		BOOST_DETAIL_IS_XXX_DEF(shared_ptr,boost::shared_ptr,1);
		BOOST_DETAIL_IS_XXX_DEF(weak_ptr,boost::weak_ptr,1);
		BOOST_DETAIL_IS_XXX_DEF(intrusive_ptr,boost::intrusive_ptr,1);
		BOOST_DETAIL_IS_XXX_DEF(scoped_ptr,boost::scoped_ptr,1);

		//auto_ptr
#ifndef BOOST_NO_AUTO_PTR
		BOOST_DETAIL_IS_XXX_DEF(auto_ptr,std::auto_ptr,1);
#endif

		//tr1
#ifdef BOOST_HAS_TR1_SHARED_PTR
		BOOST_DETAIL_IS_XXX_DEF(shared_ptr,std::tr1::shared_ptr,1);
		BOOST_DETAIL_IS_XXX_DEF(weak_ptr,std::tr1::weak_ptr,1);
#endif
	
}
}

#include "p2engine/pop_warning_option.hpp"

#endif // P2ENGINE_DETAIL_IS_XXX__HPP
