//
// config.hpp
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
#ifndef P2ENGINE_CONFIG_HPP
#define P2ENGINE_CONFIG_HPP

#if defined __AMIGA__ || defined __amigaos__ || defined __AROS__
#	define AMIGA_OS
#	ifndef PLATFORM_NAME
#		define PLATFORM_NAME "amigaos"
#	endif


/* Convenience defines for Mac platforms */
#elif defined(macintosh)
/* Mac OS 6.x/7.x/8.x/9.x, with or without CarbonLib - MPW or Metrowerks 68K/PPC compilers */
#	define MAC_OS_CLASSIC  1
#	ifndef PLATFORM_NAME
#		define PLATFORM_NAME "Mac OS"
#	endif
#elif defined(__APPLE__) && defined(__MACH__)
/* Mac OS X (client) 10.x (or server 1.x/10.x) - gcc or Metrowerks MachO compilers */
#   define DARWIN_OS  1
#	if	((defined(TARGET_OS_EMBEDDED) && TARGET_OS_EMBEDDED)  \
	|| (defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE)                   \
	|| (defined(TARGET_IPHONE_SIMULATOR) && TARGET_IPHONE_SIMULATOR))
#		define MAC_IOS 1
#		ifndef PLATFORM_NAME
#			define PLATFORM_NAME "ios"
#		endif
#	else
#		define MAC_OS_X 1
#		ifndef PLATFORM_NAME
#			define PLATFORM_NAME "Mac OS X"
#		endif
#	endif
#	if defined(MAC_OS_CLASSIC) || defined(MAC_OS_X)
/* Any OS on Mac platform */
#		define MAC_OS 1
#		define DARWIN_BASED_OS 1
#		define FILENAMES_CASE_SENSITIVE 0
#		define strcasecmp stricmp
#		ifndef DFLT_REPL_CHARENC
#			define DFLT_REPL_CHARENC MACROMAN
#		endif
#	endif//if defined(MAC_OS_CLASSIC) || defined(MAC_OS_X)


/* Convenience defines for android like platforms */
#elif defined ANDROID || defined __ANDROID__
#	define ANDROID_OS 1
#	ifndef PLATFORM_NAME
#		define PLATFORM_NAME "Android"
#	endif

/* Convenience defines for BSD like platforms */
#elif defined(__FreeBSD__)
#	define BSD_BASED_OS 1
#	ifndef PLATFORM_NAME
#		define PLATFORM_NAME "FreeBSD"
#	endif
#elif defined(__NetBSD__)
#	define BSD_BASED_OS 1
#	ifndef PLATFORM_NAME
#		define PLATFORM_NAME "NetBSD"
#	endif
#elif defined(__OpenBSD__)
#	define BSD_BASED_OS 1
#	ifndef PLATFORM_NAME
#		define PLATFORM_NAME "OpenBSD"
#	endif
#elif defined(__DragonFly__)
#	define BSD_BASED_OS 1
#	ifndef PLATFORM_NAME
#		define PLATFORM_NAME "DragonFly"
#	endif
#elif defined(__MINT__)
#	define BSD_BASED_OS 1
#	ifndef PLATFORM_NAME
#		define PLATFORM_NAME "FreeMiNT"
#	endif
#elif defined(__bsdi__)
#	define BSD_BASED_OS 1
#	ifndef PLATFORM_NAME
#		define PLATFORM_NAME "BSD/OS"
#	endif


/* Convenience defines for Windows platforms */
#elif defined(WINDOWS) || defined(_WIN32) || defined(WIN32) 
#	define WINDOWS_OS 1
#	ifndef PLATFORM_NAME
#		define PLATFORM_NAME "Windows"
#	endif
#	define strcasecmp _stricmp
#elif defined __MINGW32__
#	define MINGW_OS 1
#	define WINDOWS_OS 1
#	ifndef PLATFORM_NAME
#		define PLATFORM_NAME "Mingw32"
#	endif
/* Convenience defines for Cygwin platforms */
#elif defined(__CYGWIN__)
#	define CYGWIN_OS 1
#	define WINDOWS_OS 1
#	ifndef PLATFORM_NAME
#		define PLATFORM_NAME "Cygwin"
#	endif


/* Convenience defines for Linux platforms */
#elif defined(linux) && defined(__alpha__)
/* Linux on Alpha - gcc compiler */
#	define LINUX_OS 1
#	ifndef PLATFORM_NAME
#		define PLATFORM_NAME "Linux/Alpha"
#	endif
#elif defined(linux) && defined(__sparc__)
/* Linux on Sparc - gcc compiler */
#	define LINUX_OS 1
#	ifndef PLATFORM_NAME
#		define PLATFORM_NAME "Linux/Sparc"
#	endif
#elif defined(linux) && (defined(__i386__) || defined(__i486__) || defined(__i586__) || defined(__i686__))
/* Linux on x86 - gcc compiler */
#	define LINUX_OS 1
#	ifndef PLATFORM_NAME
#		define PLATFORM_NAME "Linux/x86"
#	endif
#elif defined(linux) && defined(__powerpc__)
/* Linux on PPC - gcc compiler */
#	define LINUX_OS 1
#	if defined(__linux__) && defined(__powerpc__)
/* #if #system(linux) */
/* MkLinux on PPC  - gcc (egcs) compiler */
/* #define MAC_OS_MKLINUX */
#		ifndef PLATFORM_NAME
#			define PLATFORM_NAME "MkLinux"
#		endif
#	else
#		ifndef PLATFORM_NAME
#			define PLATFORM_NAME "Linux/PPC"
#		endif
#	endif
#elif defined(linux) || defined(__linux__)
/* generic Linux */
#	define LINUX_OS 1
#	ifndef PLATFORM_NAME
#		define PLATFORM_NAME "Linux"
#	endif


/* Convenience defines for Solaris platforms */
#elif defined(sun)
#	define SOLARIS_OS 1
#	ifndef PLATFORM_NAME
#		define PLATFORM_NAME "Solaris"
#	endif

/* Convenience defines for HPUX + gcc platforms */
#elif defined(__hpux)
#	define HPUX_OS 1
#	ifndef PLATFORM_NAME
#		define PLATFORM_NAME "HPUX"
#	endif


/* Convenience defines for RISCOS + gcc platforms */
#elif defined(__riscos__)
#	define RISC_OS 1
#	ifndef PLATFORM_NAME
#		define PLATFORM_NAME "RISC OS"
#	endif


/* Convenience defines for OS/2 + icc/gcc platforms */
#elif defined(__OS2__) || defined(__EMX__)
#	define OS2_OS 1
#	ifndef PLATFORM_NAME
#		define PLATFORM_NAME "OS/2"
#	endif
#	define strcasecmp stricmp


/* Convenience defines for IRIX */
#elif defined(__sgi)
#	define IRIX_OS 1
#	ifndef PLATFORM_NAME
#		define PLATFORM_NAME "SGI IRIX"
#	endif


/* Convenience defines for AIX */
#elif defined(_AIX)
#	define AIX_OS 1
#	ifndef PLATFORM_NAME
#		define PLATFORM_NAME "IBM AIX"
#	endif


/* Convenience defines for BeOS platforms */
#elif defined(__BEOS__)
#	define BE_OS 1
#	ifndef PLATFORM_NAME
#		define PLATFORM_NAME "BeOS"
#	endif


/* Convenience defines for OpenVMS */
#elif defined(__VMS)
#	define OPENVMS_OS 1
#	ifndef PLATFORM_NAME
#		define PLATFORM_NAME "OpenVMS"
#	endif


/* Convenience defines for DEC Alpha OSF + gcc platforms */
#elif defined(__osf__)
#	define OSF_OS 1
#	ifndef PLATFORM_NAME
#		define PLATFORM_NAME "DEC Alpha OSF"
#	endif


/* Convenience defines for ARM platforms */
#elif defined(__arm)
#	define ARM_OS 1
#	if defined(forARM) && defined(__NEWTON_H)
/* Using Newton C++ Tools ARMCpp compiler */
#		define NEWTON_OS
#		ifndef PLATFORM_NAME
#			define PLATFORM_NAME "Newton"
#		endif
#	else
#		ifndef PLATFORM_NAME
#			define PLATFORM_NAME "ARM"
#		endif
#	endif

#endif


#if defined(__BORLANDC__)
#	define strcasecmp stricmp
#endif

#if !defined WINDOWS_OS
#	define POSIX_OS
#endif

//if using msvc10 to build vc9 prj, boost::bind( void(void) ) will cause an error,
//so, we add an dymmy param int to make void(void) to void(int)
#if defined(_MSC_VER) && _MSC_VER==1500
#	define VC9_BIND_BUG_PARAM_DECLARE int dymmy_for_vc9_bind_error=0
#	define VC9_BIND_BUG_PARAM int
#	define VC9_BIND_BUG_PARAM_DUMMY ,0
#else
#	define VC9_BIND_BUG_PARAM_DECLARE
#	define VC9_BIND_BUG_PARAM
#	define VC9_BIND_BUG_PARAM_DUMMY
#endif

//is in debug mode
#if defined(NDEBUG)&&defined(_DEBUG)
#	error  NDEBUG and _DEBUG are both defined, please check it.
#else 
#  if (!defined(NDEBUG)&&!defined(_DEBUG))
#		warning  NDEBUG and _DEBUG are both not defined, you should define one of them. Assume _DEBUG
#		define _DEBUG
#  endif
#  if !defined(NDEBUG)||defined(_DEBUG)
#   define P2ENGINE_DEBUG
#  endif
#endif
#if defined(P2ENGINE_DEBUG)&&!defined(_DEBUG)
#	define _DEBUG
#endif

//msvc
//#if defined(_MSC_VER) && (_MSC_VER != 1500)&& !defined(_DEBUG)//there is a bug in vs90
//#define _SECURE_SCL 0
//#define _HAS_ITERATOR_DEBUGGING 0
//#endif //

#if defined(__CYGWIN__)
#    define __USE_W32_SOCKETS
#endif

#if defined(__CYGWIN__) || defined(_MSC_VER) || defined(__MINGW32__)
# if !defined(_WIN32_WINNT)
#  define _WIN32_WINNT 0x0501
# endif
#endif


#if defined(P2ENGINE_HEADER_ONLY)
# define P2ENGINE_DECL inline
#else // defined(P2ENGINE_HEADER_ONLY)
# if defined(BOOST_HAS_DECLSPEC)
#  if defined(BOOST_ALL_DYN_LINK) || defined(P2ENGINE_DYN_LINK)
#   if !defined(P2ENGINE_DYN_LINK)
#    define P2ENGINE_DYN_LINK
#   endif // !defined(P2ENGINE_DYN_LINK)
// Export if this is our own source, otherwise import.
#   if defined(P2ENGINE_BUILDING_SHARED)
#    define P2ENGINE_DECL __declspec(dllexport)
#   else // defined(P2ENGINE_BUILDING_SHARED)
#    define P2ENGINE_DECL __declspec(dllimport)
#   endif // defined(P2ENGINE_BUILDING_SHARED)
#  endif // defined(BOOST_ALL_DYN_LINK) || defined(P2ENGINE_DYN_LINK)
# endif // defined(BOOST_HAS_DECLSPEC)
#endif // defined(P2ENGINE_HEADER_ONLY)

// If P2ENGINE_DECL isn't defined yet define it now.
#if !defined(P2ENGINE_DECL)
# define P2ENGINE_DECL
#endif // !defined(P2ENGINE_DECL)

// Enable library autolinking for MSVC.

#if !defined(BOOST_ALL_NO_LIB) && !defined(P2ENGINE_NO_LIB) \
	&& !defined(P2ENGINE_SOURCE) && !defined(P2ENGINE_HEADER_ONLY) \
	&& defined(_MSC_VER)

# if !defined(_MT)
#  error "You must use the multithreaded runtime."
# endif

# if (defined(_DLL) || defined(_RTLDLL)) && defined(P2ENGINE_DYN_LINK)
#  define P2ENGINE_LIB_PREFIX
# elif defined(P2ENGINE_DYN_LINK)
#  error "Mixing a dll library with a static runtime is unsupported."
# else
#  define P2ENGINE_LIB_PREFIX "lib"
# endif

# if defined(_DEBUG)
#  if defined(_DLL)
#   define P2ENGINE_LIB_SUFFIX "-gd"
#  else
#   define P2ENGINE_LIB_SUFFIX "-sgd"
#  endif
# else
#  if defined(_DLL)
#   define P2ENGINE_LIB_SUFFIX
#  else
#   define P2ENGINE_LIB_SUFFIX "-s"
#  endif
# endif

# pragma comment(lib, P2ENGINE_LIB_PREFIX "p2engine" P2ENGINE_LIB_SUFFIX ".lib")

#endif // !defined(BOOST_ALL_NO_LIB) && !defined(P2ENGINE_NO_LIB)
//  && !defined(P2ENGINE_SOURCE) && !defined(P2ENGINE_HEADER_ONLY)
//  && defined(_MSC_VER)

#define P2ENGINE_DEBUG_FOR_PACKET_FORMAT_DEF
#include "p2engine/push_warning_option.hpp"
#include <cstddef>
#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <locale>
#include <cwchar>
#include <climits>

#ifndef WCHAR_MAX
#  ifdef __WCHAR_MAX__
#    define WCHAR_MAX     __WCHAR_MAX__
#  elif defined(__WCHAR_MAX)
#    define WCHAR_MAX     __WCHAR_MAX
/* SUNpro has some bugs with casts. wchar_t is size of int there anyway. */
#  elif defined (__SUNPRO_CC) || defined (__DJGPP)
#    define WCHAR_MAX (~0)
#  else
#    define WCHAR_MAX ((wchar_t)~0)
#  endif
#endif
/* WCHAR_MIN should be 0 if wchar_t is an unsigned type and
   (-WCHAR_MAX-1) if wchar_t is a signed type.  Unfortunately,
   it turns out that -fshort-wchar changes the signedness of
   the type. */
#ifndef WCHAR_MIN
#  ifdef __WCHAR_MIN__
#    define WCHAR_MIN     __WCHAR_MIN__
#  elif defined(__WCHAR_MIN)
#    define WCHAR_MIN     __WCHAR_MIN
#  elif WCHAR_MAX == 0xffff
#    define WCHAR_MIN       0
#  else
#    define WCHAR_MIN       (-WCHAR_MAX-1)
#  endif
#endif

#include <boost/config.hpp>
#include <boost/assert.hpp>
#include <boost/foreach.hpp>
#include <boost/typeof/typeof.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/scope_exit.hpp>
#include <boost/bind/protect.hpp>
#include "p2engine/pop_warning_option.hpp"

//net config
// ==== AMIGA ===
#if defined(AMIGA_OS)
#	define P2ENGINE_USE_MLOCK 0
#	define P2ENGINE_USE_WRITEV 0
#	define P2ENGINE_USE_READV 0
#	define P2ENGINE_USE_IPV6 0
#	define P2ENGINE_USE_BOOST_THREAD 0
#	define P2ENGINE_USE_IOSTREAM 0
// set this to 1 to disable all floating point operations
// (disables some float-dependent APIs)
#	define P2ENGINE_NO_FPU 1
#	define P2ENGINE_USE_I2P 0
#	define P2ENGINE_USE_ICONV 0
#endif

// ==== Darwin/BSD ===
#if defined(BSD_BASED_OS) || defined(DARWIN_BASED_OS)
// we don't need iconv on mac, because
// the locale is always utf-8
#	if defined(DARWIN_BASED_OS)
#		ifndef P2ENGINE_USE_ICONV
#			define P2ENGINE_USE_ICONV 0
#		endif
#		ifndef P2ENGINE_USE_LOCALE
#			define P2ENGINE_USE_LOCALE 0
#		endif
#		ifndef P2ENGINE_CLOSE_MAY_BLOCK
#			define P2ENGINE_CLOSE_MAY_BLOCK 0
#		endif
#	endif
#	ifndef P2ENGINE_HAS_FALLOCATE
#		define P2ENGINE_HAS_FALLOCATE 0
#	endif
#	ifndef P2ENGINE_USE_IFADDRS
#		define P2ENGINE_USE_IFADDRS 1
#	endif
#	ifndef P2ENGINE_USE_SYSCTL
#		define P2ENGINE_USE_SYSCTL 1
#	endif
#	ifndef P2ENGINE_USE_IFCONF
#		define P2ENGINE_USE_IFCONF 1
#	endif

#elif defined(ANDROID_OS)
#	ifndef P2ENGINE_USE_IFADDRS
#		define P2ENGINE_USE_IFADDRS 0
#	endif
#	ifndef P2ENGINE_USE_NETLINK
#		define P2ENGINE_USE_NETLINK 1
#	endif
#	ifndef P2ENGINE_USE_IFCONF
#		define P2ENGINE_USE_IFCONF 1
#	endif
#	ifndef P2ENGINE_HAS_SALEN
#		define P2ENGINE_HAS_SALEN 0
#	endif

#elif defined(LINUX_OS)
#	ifndef P2ENGINE_USE_IFADDRS
#		define P2ENGINE_USE_IFADDRS 1
#	endif
#	ifndef P2ENGINE_USE_NETLINK
#		define P2ENGINE_USE_NETLINK 1
#	endif
#	ifndef P2ENGINE_USE_IFCONF
#		define P2ENGINE_USE_IFCONF 1
#	endif
#	ifndef P2ENGINE_HAS_SALEN
#		define P2ENGINE_HAS_SALEN 0
#	endif

#elif defined (MINGW_OS)
#	ifndef P2ENGINE_USE_ICONV
#		define P2ENGINE_USE_ICONV 0
#		define P2ENGINE_USE_LOCALE 1
#	endif
#	define P2ENGINE_ICONV_ARG (const char**)
#	ifndef P2ENGINE_USE_RLIMIT
#		define P2ENGINE_USE_RLIMIT 0
#	endif
#	ifndef P2ENGINE_USE_NETLINK
#		define P2ENGINE_USE_NETLINK 0
#	endif
#	ifndef P2ENGINE_USE_GETADAPTERSADDRESSES
#		define P2ENGINE_USE_GETADAPTERSADDRESSES 1
#	endif
#	ifndef P2ENGINE_HAS_SALEN
#		define P2ENGINE_HAS_SALEN 0
#	endif
#	ifndef P2ENGINE_USE_GETIPFORWARDTABLE
#		define P2ENGINE_USE_GETIPFORWARDTABLE 1
#	endif

#elif defined (WINDOWS_OS)
#	ifndef P2ENGINE_USE_ICONV
#		define P2ENGINE_USE_ICONV 0
#		define P2ENGINE_USE_LOCALE 1
#	endif
#	ifndef P2ENGINE_USE_GETIPFORWARDTABLE
#		define P2ENGINE_USE_GETIPFORWARDTABLE 1
#	endif
#	ifndef P2ENGINE_USE_GETADAPTERSADDRESSES
#		define P2ENGINE_USE_GETADAPTERSADDRESSES 1
#	endif
#	ifndef P2ENGINE_HAS_SALEN
#		define P2ENGINE_HAS_SALEN 0
#	endif
#	ifndef P2ENGINE_USE_RLIMIT
#		define P2ENGINE_USE_RLIMIT 0
#	endif
#	ifndef P2ENGINE_HAS_FALLOCATE
#		define P2ENGINE_HAS_FALLOCATE 0
#	endif

#elif defined SOLARIS_OS
#	ifndef P2ENGINE_SOLARIS
#		define P2ENGINE_SOLARIS 1
#	endif
#	ifndef P2ENGINE_COMPLETE_TYPES_REQUIRED
#		define P2ENGINE_COMPLETE_TYPES_REQUIRED 1
#	endif
#	ifndef P2ENGINE_USE_IFCONF
#		define P2ENGINE_USE_IFCONF 1
#	endif
#	ifndef P2ENGINE_HAS_SALEN
#		define P2ENGINE_HAS_SALEN 0
#	endif

#elif defined BE_OS
#	include <storage/StorageDefs.h> // B_PATH_NAME_LENGTH
#	ifndef P2ENGINE_HAS_FALLOCATE
#		define P2ENGINE_HAS_FALLOCATE 0
#	endif
#	ifndef P2ENGINE_USE_MLOCK
#		define P2ENGINE_USE_MLOCK 0
#	endif
#	ifndef P2ENGINE_USE_ICONV
#		define P2ENGINE_USE_ICONV 0
#	endif
#	if __GNUCC__ == 2
#		if defined(P2ENGINE_BUILDING_SHARED)
#			define P2ENGINE_EXPORT __declspec(dllexport)
#		elif defined(P2ENGINE_LINKING_SHARED)
#		define P2ENGINE_EXPORT __declspec(dllimport)
#	endif
#	endif

#elif defined __GNU__//GNU/Hurd
#	ifndef P2ENGINE_HURD
#		define P2ENGINE_HURD 1
#	endif
#	ifndef P2ENGINE_USE_IFADDRS
#		define P2ENGINE_USE_IFADDRS 1
#	endif
#	ifndef P2ENGINE_USE_IFCONF
#		define P2ENGINE_USE_IFCONF 1
#	endif

#else
#	warning unknown OS, assuming BSD
#	define BSD_BASED_OS
#endif

// windows
#if defined FILENAME_MAX
#	define P2ENGINE_MAX_PATH FILENAME_MAX
// beos
#elif defined B_PATH_NAME_LENGTH
#	define P2ENGINE_MAX_PATH B_PATH_NAME_LENGTH
// solaris
#elif defined MAXPATH
#	define P2ENGINE_MAX_PATH MAXPATH
// posix
#elif defined NAME_MAX
#	define P2ENGINE_MAX_PATH NAME_MAX
// none of the above
#else
// this is the maximum number of characters in a
// path element / filename on windows
#	define P2ENGINE_MAX_PATH 255
#	warning unknown platform, assuming the longest path is 255
#endif


#ifndef P2ENGINE_ICONV_ARG
#define P2ENGINE_ICONV_ARG (char**)
#endif

// libiconv presence, not implemented yet
#ifndef P2ENGINE_USE_ICONV
#define P2ENGINE_USE_ICONV 1
#endif

#ifndef P2ENGINE_HAS_SALEN
#define P2ENGINE_HAS_SALEN 1
#endif

#ifndef P2ENGINE_USE_GETADAPTERSADDRESSES
#define P2ENGINE_USE_GETADAPTERSADDRESSES 0
#endif

#ifndef P2ENGINE_USE_NETLINK
#define P2ENGINE_USE_NETLINK 0
#endif

#ifndef P2ENGINE_USE_SYSCTL
#define P2ENGINE_USE_SYSCTL 0
#endif

#ifndef P2ENGINE_USE_GETIPFORWARDTABLE
#define P2ENGINE_USE_GETIPFORWARDTABLE 0
#endif

#ifndef P2ENGINE_USE_LOCALE
#define P2ENGINE_USE_LOCALE 0
#endif

// set this to true if close() may block on your system
// Mac OS X does this if the file being closed is not fully
// allocated on disk yet for instance. When defined, the disk
// I/O subsytem will use a separate thread for closing files
#ifndef P2ENGINE_CLOSE_MAY_BLOCK
#define P2ENGINE_CLOSE_MAY_BLOCK 0
#endif

#ifndef P2ENGINE_BROKEN_UNIONS
#define P2ENGINE_BROKEN_UNIONS 0
#endif

#if defined UNICODE && !defined BOOST_NO_STD_WSTRING
#define P2ENGINE_USE_WSTRING 1
#else
#define P2ENGINE_USE_WSTRING 0
#endif // UNICODE

#ifndef P2ENGINE_HAS_FALLOCATE
#define P2ENGINE_HAS_FALLOCATE 1
#endif

#ifndef P2ENGINE_EXPORT
# define P2ENGINE_EXPORT
#endif

#ifndef P2ENGINE_DEPRECATED_PREFIX
#define P2ENGINE_DEPRECATED_PREFIX
#endif

#ifndef P2ENGINE_DEPRECATED
#define P2ENGINE_DEPRECATED
#endif

#ifndef P2ENGINE_COMPLETE_TYPES_REQUIRED
#define P2ENGINE_COMPLETE_TYPES_REQUIRED 0
#endif

#ifndef P2ENGINE_USE_RLIMIT
#define P2ENGINE_USE_RLIMIT 1
#endif

#ifndef P2ENGINE_USE_IFADDRS
#define P2ENGINE_USE_IFADDRS 0
#endif

#ifndef P2ENGINE_USE_IPV6
#define P2ENGINE_USE_IPV6 1
#endif

#ifndef P2ENGINE_USE_MLOCK
#define P2ENGINE_USE_MLOCK 1
#endif

#ifndef P2ENGINE_USE_WRITEV
#define P2ENGINE_USE_WRITEV 1
#endif

#ifndef P2ENGINE_USE_READV
#define P2ENGINE_USE_READV 1
#endif

#ifndef P2ENGINE_NO_FPU
#define P2ENGINE_NO_FPU 0
#endif

#if !defined P2ENGINE_USE_IOSTREAM && !defined BOOST_NO_IOSTREAM
#define P2ENGINE_USE_IOSTREAM 1
#else
#define P2ENGINE_USE_IOSTREAM 0
#endif

#ifndef P2ENGINE_USE_I2P
#define P2ENGINE_USE_I2P 1
#endif

#ifndef P2ENGINE_HAS_STRDUP
#define P2ENGINE_HAS_STRDUP 1
#endif

#if !defined P2ENGINE_IOV_MAX
#ifdef IOV_MAX
#define P2ENGINE_IOV_MAX IOV_MAX
#else
#define P2ENGINE_IOV_MAX INT_MAX
#endif
#endif

#if !defined(P2ENGINE_READ_HANDLER_MAX_SIZE)
# define P2ENGINE_READ_HANDLER_MAX_SIZE 256
#endif

#if !defined(P2ENGINE_WRITE_HANDLER_MAX_SIZE)
# define P2ENGINE_WRITE_HANDLER_MAX_SIZE 256
#endif

#if defined _MSC_VER && _MSC_VER <= 1200
#define for if (false) {} else for
#endif

#if P2ENGINE_BROKEN_UNIONS
#define P2ENGINE_UNION struct
#else
#define P2ENGINE_UNION union
#endif

// determine what timer implementation we can use
// if one is already defined, don't pick one
// autmatically. This lets the user control this
// from the Jamfile
#if !defined P2ENGINE_USE_ABSOLUTE_TIME \
	&& !defined P2ENGINE_USE_QUERY_PERFORMANCE_TIMER \
	&& !defined P2ENGINE_USE_CLOCK_GETTIME \
	&& !defined P2ENGINE_USE_BOOST_DATE_TIME \
	&& !defined P2ENGINE_USE_ECLOCK \
	&& !defined P2ENGINE_USE_SYSTEM_TIME

#if defined __APPLE__ && defined __MACH__
#define P2ENGINE_USE_ABSOLUTE_TIME 1
#elif defined(_WIN32) || defined MINGW_OS
#define P2ENGINE_USE_QUERY_PERFORMANCE_TIMER 1
#elif defined(_POSIX_MONOTONIC_CLOCK) && _POSIX_MONOTONIC_CLOCK >= 0
#define P2ENGINE_USE_CLOCK_GETTIME 1
#elif defined(P2ENGINE_AMIGA)
#define P2ENGINE_USE_ECLOCK 1
#elif defined(P2ENGINE_BEOS)
#define P2ENGINE_USE_SYSTEM_TIME 1
#else
#define P2ENGINE_USE_BOOST_DATE_TIME 1
#endif

#endif

#if !P2ENGINE_HAS_STRDUP
inline char* strdup(char const* str)
{
	if (str == 0) return 0;
	size_t len=strlen(str);
	char* tmp = (char*)malloc(len + 1);
	if (tmp == 0) return 0;
	memcpy(tmp,str,len);
	tmp[len]='\0';
	return tmp;
}
#endif

// for non-exception builds
#ifdef BOOST_NO_EXCEPTIONS
#define P2ENGINE_TRY if (true)
#define P2ENGINE_CATCH(x) else if (false)
#define P2ENGINE_DECLARE_DUMMY(x, y) x y
#else
#define P2ENGINE_TRY try
#define P2ENGINE_CATCH(x) catch(x)
#define P2ENGINE_DECLARE_DUMMY(x, y)
#endif // BOOST_NO_EXCEPTIONS

#if defined(WINDOWS_OS)
#define snprintf _snprintf
#define strtoll _strtoi64
#define INT64_F "I64d"
#define UINT64_F "I64u"
#else
#define INT64_F "lld"
#define UINT64_F "llu"
#endif

#if defined(BOOST_NO_STRINGSTREAM) || defined(BOOST_NO_STD_WSTRING)
#   define P2ENGINE_NO_WIDE_FUNCTIONS
#endif

#include "p2engine/macro.hpp"
#include "p2engine/typedef.hpp"

namespace boost{
	void inline assertion_failed(char const * expr, char const * func, char const * file, long line)
	{
		printf("(%s) assert failed, function:%s, file:%s, line:%d\n",expr,func,file,(int)line);
#ifdef _WIN32
		//__asm{int 3};
#else
		//__asm__("int $3");
		//abort();
#endif
	}
}


#endif//P2ENGINE_CONFIG_HPP
