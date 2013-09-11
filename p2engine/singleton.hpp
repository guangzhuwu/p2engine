//
// singleton.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2009-2010, GuangZhu Wu 
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

#ifndef P2ENGINE_SINGLETON_HPP
#define P2ENGINE_SINGLETON_HPP
#include "p2engine/push_warning_option.hpp"
#include <boost/serialization/singleton.hpp>
#include "p2engine/pop_warning_option.hpp"

namespace p2engine{
	namespace detail{
		template<typename Type>
		struct singleton_variable
			:public Type
		{
			typedef Type type;
			static bool m_is_destroyed;
			~singleton_variable(){
				m_is_destroyed = true;
			}
		};
		template <typename Type>
		bool singleton_variable<Type>::m_is_destroyed = false;
	}

#define SINGLETON_ACCESS_DECLARE \
	template <typename _This_Type_> friend class ::p2engine::singleton;\
	template <typename _This_Type_> friend struct ::p2engine::detail::singleton_variable;


	// T must be: no-throw default constructible and no-throw destructible
	template <typename T>
	class singleton
	{
		singleton();
		typedef detail::singleton_variable<T> singleton_variable_type;
	public:
		typedef T object_type; 

		// If, at any point (in user code), singleton_default<T>::instance()
		//  is called, then the following function is instantiated.
		P2ENGINE_DECL static object_type & instance()
		{
			BOOST_ASSERT(!singleton_variable_type::m_is_destroyed);
			__do_nothing(m_instance);
			return (object_type&)(m_the_instance);
		}
		P2ENGINE_DECL static bool is_destroyed(){
			return singleton_variable_type::m_is_destroyed;
		}
	private:
		// include this to provoke instantiation at pre-execution time
		P2ENGINE_DECL static void __do_nothing(T&){}
		static singleton_variable_type m_the_instance;
		static object_type & m_instance;
	};
	template<class T>
	typename singleton<T>::singleton_variable_type singleton<T>::m_the_instance;
	template<class T>
	T & singleton<T>::m_instance = singleton<T>::instance();

}//namespace p2engine

#endif

