//
// macro.hpp
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

#ifndef P2ENGINE_MACRO_HPP
#define P2ENGINE_MACRO_HPP



//disable warning for unused paramer
#define UNUSED_PARAMETER(x) {(void)(x);}

//namespace define
#define  P2ENGINE_NAMESPACE_BEGIN namespace p2engine {
#define  P2ENGINE_NAMESPACE_END }

#define  NAMESPACE_BEGIN(x) namespace x {
#define  NAMESPACE_END(x) }

//debug scope
#if defined(_DEBUG) || defined(_DEBUG_SCOPE)
#	define DEBUG_SCOPE(x) x
#	define DEBUG_SCOPE_OPENED
#else
#	define DEBUG_SCOPE(x)
#endif

/** Declare _TODO(message)  */
#if defined(_MSC_VER)
#define _DO_PRAGMA(x) __pragma(x)
#define __STR__(x) #x
#define __STR(x) __STR__(x)
#define __FILE__LINE__ __FILE__ "(" __STR(__LINE__) ") : "
#define _MSG_PRAGMA(_msg) _DO_PRAGMA(message (__FILE__LINE__ _msg))
#elif defined(__GNUC__)
#define _DO_PRAGMA(x) _Pragma (#x)
#define _MSG_PRAGMA(_msg) _DO_PRAGMA(message (_msg))
#else
#define _DO_PRAGMA(x)
#define _MSG_PRAGMA(_msg) 
#endif
#define WARNING(x) _MSG_PRAGMA("Warning : " #x)
#define TODO(x)	_MSG_PRAGMA("TODO : " #x)
#define TODO_ASSERT(x) TODO(x) BOOST_ASSERT(0&&#x)
#define NOTICE(x)	_MSG_PRAGMA("Notice : " #x)


// A cross-compiler definition for "deprecated"-warnings
#if defined(__GNUC__) && (__GNUC__ - 0 > 3 || (__GNUC__ - 0 == 3 && __GNUC_MINOR__ - 0 >= 2))
/* gcc >= 3.2 */
#   define _DEPRECATED_PRE(_MSG)
// The "message" is not supported yet in GCC 
//#   define _DEPRECATED_POST(_MSG) __attribute__ ((deprecated(_MSG)))
#   define _DEPRECATED_POST(_MSG) __attribute__ ((deprecated))
# elif defined(_MSC_VER) && (_MSC_VER >= 1300)
/* msvc >= 7 */
#   define _DEPRECATED_PRE(_MSG)  __declspec(deprecated (_MSG))
#   define _DEPRECATED_POST(_MSG)
# else
#  define _DEPRECATED_PRE(_MSG)
#  define _DEPRECATED_POST(_MSG)
# endif
/** Usage: _DECLARE_DEPRECATED_FUNCTION("Use XX instead", void myFunc(double)); */
#define DECLARE_DEPRECATED_FUNCTION(__MSG, __FUNC) _DEPRECATED_PRE(__MSG) __FUNC _DEPRECATED_POST(__MSG)
#define DECLARE_DEPRECATED(__MSG) _DEPRECATED_PRE(__MSG) _DEPRECATED_POST(__MSG)


//auto static member
#define STATIC_DATA_MEMBER(Type,Name,ConstructParm)\
	inline static Type& s_##Name(){static Type Name__##ConstructParm; return Name__;}

//alignment
namespace detail{
#if defined(_MSC_VER)
# pragma warning (push)
# pragma warning(disable : 4624)
#endif
	template<class _Ty>
	struct __get_align
	{	// struct used to determine alignemt of _Ty
		_Ty _Elt0;
		char _Elt1;
		_Ty _Elt2;
	};
#if defined(_MSC_VER)
# pragma warning (pop)
#endif
}

//
#ifndef ALIGN_OF
#	define ALIGN_OF(type) (sizeof(::detail::__get_align<type>) - 2 * sizeof(type))
#endif
#ifndef IS_ALIGNED
#	define IS_ALIGNED(ptr,type) (( (uintptr_t)(ptr) & ( ALIGN_OF(type) - 1 ) ) == 0)
#endif // IS_ALIGNED
#define ALIGN_UP(ptr, mp_align)\
	((void*)( (static_cast<const void*>(ptr) + ((mp_align) - 1)) & ~(((mp_align) - 1)) ))

#ifndef ROUND_UP
#	define ROUND_UP(a, b)   (((a)+((b)-1))&~((b)-1))
#endif
#ifndef SIZEOF_ARRAY
#	define SIZEOF_ARRAY(A) (sizeof(A)/sizeof((A)[0]))
#endif // SIZEOF_ARRAY





#endif//P2ENGINE_MACRO_HPP
