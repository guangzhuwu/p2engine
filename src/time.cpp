#include "p2engine/time.hpp"

#define TIME_INACCURACY_TEST //*1.90

NAMESPACE_BEGIN(p2engine);

#if (!defined (__MACH__) && !defined (_WIN32)\
	&&(!defined(_POSIX_MONOTONIC_CLOCK) || _POSIX_MONOTONIC_CLOCK < 0))

#	include <boost/date_time/posix_time/posix_time_types.hpp>
uint64_t precise_local_tick_count()
{
	return (boost::posix_time::microsec_clock::universal_time()-min_time()).total_milliseconds()TIME_INACCURACY_TEST;
}
#else

#	if defined(__MACH__)
#		include <mach/mach_time.h>
#		include <boost/cstdint.hpp>
// high precision timer for darwin intel and ppc
uint64_t precise_local_tick_count()
{
	static mach_timebase_info_data_t timebase_info = {0,0};
	if (timebase_info.denom == 0)
		mach_timebase_info(&timebase_info);
	tick_type at = mach_absolute_time();
	// make sure we don't overflow
	BOOST_ASSERT((at >= 0 && at >= at / 1000 * timebase_info.numer / timebase_info.denom)
		|| (at < 0 && at < at / 1000 * timebase_info.numer / timebase_info.denom));
	return ((at / 1000 * timebase_info.numer / timebase_info.denom)/1000)TIME_INACCURACY_TEST;
}
#	elif defined(_WIN32) || defined(WIN32)
#		ifndef WIN32_LEAN_AND_MEAN
#			define WIN32_LEAN_AND_MEAN
#		endif
#		include <windows.h>
uint64_t precise_local_tick_count()
{
	static LARGE_INTEGER performace_counter_frequency = {0,0};
	static BOOL dummy_bool=QueryPerformanceFrequency(&performace_counter_frequency);
	LARGE_INTEGER now;
	QueryPerformanceCounter(&now);
	return (now.QuadPart * 1000ULL / performace_counter_frequency.QuadPart)TIME_INACCURACY_TEST;
}
#	elif defined(_POSIX_MONOTONIC_CLOCK) && _POSIX_MONOTONIC_CLOCK >= 0
#		include <time.h>
uint64_t precise_local_tick_count()
{
	timespec ts;
	::clock_gettime(CLOCK_MONOTONIC, &ts);
	return ((uint64_t(ts.tv_sec) * 1000ULL + uint64_t(ts.tv_nsec)/1000000ULL))TIME_INACCURACY_TEST;
}
#	endif
#endif


#if (defined(_WIN32) || defined(WIN32))
#	include <MMSystem.h>
#	pragma comment(lib, "winmm.lib")

inline uint32_t time_get_time()
{
	return timeGetTime()TIME_INACCURACY_TEST;
}

static uint64_t s_diff_precise__=precise_local_tick_count()-time_get_time();
uint64_t rough_local_tick_count()
{
	static atomic<uint32_t> h_time(0);
	static atomic<uint32_t> l_time(time_get_time());

	uint32_t now =time_get_time();
	uint32_t ltime=l_time;
	uint32_t htime=h_time;
	uint64_t lastTime64= ((((uint64_t)htime)<<32)|ltime);
	uint32_t diff=now-ltime;
	lastTime64+=diff;
	if (ltime>now||diff>0x7fffffffU)
	{
		h_time=(lastTime64>>32);
		l_time=now;
		ltime=l_time;
		htime=h_time;
	}
	if (ltime!=l_time||htime!=h_time)
		return rough_local_tick_count();
	else
		return lastTime64+s_diff_precise__;
}

#elif defined(_POSIX_MONOTONIC_CLOCK) && _POSIX_MONOTONIC_CLOCK >= 0
#	include <time.h>
inline uint64_t time_get_time()
{
	struct timespec ts;
	::clock_gettime(CLOCK_MONOTONIC, &ts);
	return ((uint64_t(ts.tv_sec) * 1000ULL + uint64_t(ts.tv_nsec)/1000000ULL))TIME_INACCURACY_TEST;
}
static uint64_t s_diff_precise__=precise_local_tick_count()-time_get_time();
uint64_t rough_local_tick_count()
{
	return s_diff_precise__+time_get_time();
}

#elif defined(BOOST_HAS_GETTIMEOFDAY)
#	include <sys/timeb.h>
#	include <stdio.h>
#	include <time.h>
inline uint64_t time_get_time()
{
	struct timeval ts;
	::gettimeofday(&ts,0);
	return (ts.tv_sec * 1000ULL + (ts.tv_usec / 1000ULL))TIME_INACCURACY_TEST;
}
static uint64_t s_diff_precise__=precise_local_tick_count()-time_get_time();
uint64_t rough_local_tick_count()
{
	return s_diff_precise__+time_get_time();
}
#else
# error NO proper tick functions.
#endif

NAMESPACE_END(p2engine);

