//
// time.hpp
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

#ifndef P2ENGINE_TIME_HPP
#define P2ENGINE_TIME_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "p2engine/push_warning_option.hpp"
#include "p2engine/config.hpp"
#include <memory>
#include <boost/thread/thread.hpp>
#include "p2engine/pop_warning_option.hpp"

#include "p2engine/atomic.hpp"
#include "p2engine/mutex.hpp"

#if defined(_WIN32) || defined(WIN32)
inline int usleep(int usec){Sleep(usec/1000);return 0;}
#endif

namespace p2engine{
	typedef boost::posix_time::time_duration::tick_type tick_type;
	typedef boost::posix_time::ptime ptime;
	typedef boost::posix_time::time_duration time_duration;
	using boost::posix_time::seconds;
	using boost::posix_time::milliseconds;
	using boost::posix_time::microseconds;
	using boost::posix_time::millisec;
	using boost::posix_time::microsec;
	using boost::posix_time::minutes;
	using boost::posix_time::hours;

	inline const ptime& min_time()
	{ static ptime t(boost::posix_time::min_date_time); return t;}
	inline const ptime& max_time()
	{ static ptime t(boost::posix_time::max_date_time); return t; }
	inline const ptime& min_tick()
	{ static ptime t(boost::posix_time::min_date_time); return t; }
	inline const ptime& max_tick()
	{ static ptime t(boost::posix_time::max_date_time); return t; }
	inline int64_t min_tick_count()
	{ return 0; }
	inline int64_t max_tick_count()
	{ return (std::numeric_limits<int64_t>::max)(); }

	uint64_t precise_local_tick_count();
	uint64_t rough_local_tick_count();

	class system_time
	{
		typedef system_time this_type;
	public:
		typedef boost::posix_time::time_duration::tick_type tick_type;
		typedef boost::posix_time::ptime  ptime;
		typedef boost::posix_time::time_duration  time_duration;

	public:
		static ptime local_time()
		{
			return local_time_stamp()+milliseconds(elapse());
		}

		static ptime universal_time()
		{
			return universal_time_stamp()+milliseconds(elapse());
		}

		static tick_type tick_count()
		{
			return elapse();
		}

		static ptime precise_local_time()
		{
			return local_time_stamp()+milliseconds(precise_elapse());
		}

		static ptime precise_universal_time()
		{
			return universal_time_stamp()+milliseconds(precise_elapse());
		}

		static tick_type precise_tick_count()
		{
			return precise_elapse();
		}
		
		//do not throw exception
		static void sleep(const time_duration& duration)throw()
		{
			sleep_microseconds(duration.total_microseconds());
		}
		static void sleep_millisec(tick_type msec)throw()
		{
			sleep_microseconds(msec*1000);
		}
		static void sleep_microseconds(tick_type us)throw()
		{
			if (!us)
			{
				usleep(0);
				return;
			}
			for (;us>0;)
			{
				tick_type t=std::min<tick_type>(us,(std::numeric_limits<int>::max)());
				usleep((int)t);
				us-=t;
			}
		}
	private:
		static ptime& local_time_stamp()
		{
			static ptime local_time_stamp_=boost::posix_time::microsec_clock::local_time();
			return local_time_stamp_;
		}
		static ptime& universal_time_stamp()
		{
			static ptime universal_time_stamp_=boost::posix_time::microsec_clock::universal_time();
			return universal_time_stamp_;
		}
		static int64_t elapse()
		{
			static uint64_t t_=rough_local_tick_count();
			return rough_local_tick_count()-t_;
		}
		static uint64_t precise_elapse()
		{
			static uint64_t t_=precise_local_tick_count();
			return precise_local_tick_count()-t_;
		}
	};

	struct time_traits_base
	{
		/// The time type.
		typedef system_time::ptime time_type;

		/// The duration type.
		typedef system_time::time_duration duration_type;

		/// Add a duration to a time.
		static time_type add(const time_type& t, const duration_type& d)
		{
			return t + d;
		}

		/// Subtract one time from another.
		static duration_type subtract(const time_type& t1, const time_type& t2)
		{
			return t1 - t2;
		}

		/// Test whether one time is less than another.
		static bool less_than(const time_type& t1, const time_type& t2)
		{
			return t1 < t2;
		}

		/// Convert to POSIX duration type.
		static boost::posix_time::time_duration to_posix_duration(
			const duration_type& d)
		{
			return d;
		}
	};

	/// Time traits specialised for posix_time.
	struct precise_tick_time
		:time_traits_base
	{
		static time_type now()
		{
			return system_time::precise_universal_time();
		}
		static tick_type now_tick_count()
		{
			return system_time::precise_tick_count();
		}
	};

	/// Time traits specialised for posix_time.
	struct rough_tick_time
		:time_traits_base
	{
		static time_type now()
		{
			return system_time::universal_time();
		}
		static tick_type now_tick_count()
		{
			return system_time::tick_count();
		}
	};
}

#endif//P2ENGINE_TIME_HPP_INCLUDED

