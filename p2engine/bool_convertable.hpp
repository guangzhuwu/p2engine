//
// bool_convertable.hpp
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

#ifndef P2ENGINE_BOOL_CONVERTABLE_HPP
#define P2ENGINE_BOOL_CONVERTABLE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "p2engine/get_derived_this.hpp"

namespace p2engine{

	template<class ConvertableType>
	class bool_convertable
	{
		typedef bool_convertable this_type;
		typedef const ConvertableType* ConstConvertablePtr;

	public:
#if (defined(__SUNPRO_CC) && BOOST_WORKAROUND(__SUNPRO_CC, < 0x570)) || defined(__CINT_)
		typedef bool unspecified_bool_type;
		static unspecified_bool_type to_unspecified_bool(const bool x, ConstConvertablePtr)
		{
			return x;
		}
#elif defined(_MANAGED)
		static void unspecified_bool(this_type***)
		{
		}
		typedef void(*unspecified_bool_type)(this_type***);
		static unspecified_bool_type to_unspecified_bool(const bool x, ConstConvertablePtr)
		{
			return x ? unspecified_bool : 0;
		}
#elif \
	( defined(__MWERKS__) && BOOST_WORKAROUND(__MWERKS__, < 0x3200) ) || \
	( defined(__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__ < 304) ) || \
	( defined(__SUNPRO_CC) && BOOST_WORKAROUND(__SUNPRO_CC, <= 0x590) )

		typedef bool (this_type::*unspecified_bool_type)() const;

		static unspecified_bool_type to_unspecified_bool(const bool x, const ConstConvertablePtr)
		{
			return x ? &this_type::detail_bool_convertable_member_fn : 0;
		}
	private:
		bool detail_bool_convertable_member_fn() const { return false; }
#else
		typedef ConstConvertablePtr unspecified_bool_type;
		static unspecified_bool_type to_unspecified_bool(const bool x, const ConstConvertablePtr p)
		{
			return x ? p : 0;
		}
#endif


	private:
		bool_convertable();
		bool_convertable(const bool_convertable&);
		void operator=(const bool_convertable&);
		~bool_convertable();
	};

}

#endif // P2ENGINE_BOOL_CONVERTABLE_HPP


