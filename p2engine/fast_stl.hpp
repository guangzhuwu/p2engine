//
// fast_stl.hpp
// ~~~~~~~~~~~~
//
// Copyright (c) 2008-2010 GuangZhu Wu (GuangZhuWu@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef P2ENGINE_FAST_STL_HPP
#define P2ENGINE_FAST_STL_HPP

#include <p2engine/push_warning_option.hpp>
#include <map>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <p2engine/pop_warning_option.hpp>

namespace p2engine{

	template<typename Key, typename Compare=std::less<Key> >
	class fast_set
		:public boost::multi_index::multi_index_container<
		Key,
		boost::multi_index::indexed_by< boost::multi_index::ordered_unique<
		boost::multi_index::identity<Key>,Compare 
		> >
		>
	{
	public:
		typedef Key key_type;
		typedef Compare key_compare;
	};

}

#endif//P2ENGINE_FAST_STL_HPP

