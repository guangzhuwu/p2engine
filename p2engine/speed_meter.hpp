//
// null_mutex.hpp
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

#ifndef P2ENGINE_SPEEDMETER_H__
#define P2ENGINE_SPEEDMETER_H__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "p2engine/push_warning_option.hpp"
#include "p2engine/config.hpp"
#include <deque>
#include <list>
#include <iostream>
#include "p2engine/pop_warning_option.hpp"

#include "p2engine/time.hpp"
#include "p2engine/mutex.hpp"

namespace p2engine{

	template <typename TimeTraitsType,typename MutexType=null_mutex>
	class basic_speed_meter
	{
	public:
		typedef TimeTraitsType time_traits_type;
		typedef MutexType mutex_type;
		typedef typename mutex_type::scoped_lock scoped_lock_type;

	private:
		struct _pair:object_allocator 
		{
			tick_type timestamp;
			int bytes;
			_pair(tick_type t,int b):timestamp(t),bytes(b){}
		};
		typedef std::deque<_pair> history_type;
		typedef typename history_type::iterator iterator;

		BOOST_STATIC_CONSTANT(tick_type,min_time_threshold_usec=100);

	public:
		basic_speed_meter(const time_duration& timewindow)
			:quedBytes_(0)
			,timeWindow_(timewindow.total_milliseconds())
			,timestart_(-1)
			,lastForgetTime_(-1)
			,lastSpeed_(0)
			,totalBytes_(0)
			,totalCnt_(0)
			,needRecalc_(true)
		{
			BOOST_ASSERT(timeWindow_>min_time_threshold_usec);
			BOOST_STATIC_ASSERT(sizeof(tick_type)==8);	
		}

		~basic_speed_meter(){}

		double bytes_per_second()const
		{
			tick_type curTick=time_traits_type::now_tick_count();

			scoped_lock_type lock(mutex_);

			if (-1==timestart_)
			{
				if (history_.empty())
				{
					timestart_=curTick;
					return 0;
				}
				else
				{
					timestart_=history_.front().timestamp;
				}
			}

			__forget_history(curTick);

			if (!needRecalc_)
				return lastSpeed_;

			needRecalc_=false;
			tick_type window=(std::min)(curTick-timestart_,timeWindow_);
			if (min_time_threshold_usec>window)
				window=min_time_threshold_usec;
			lastSpeed_=(quedBytes_*1000.0/window);
			return lastSpeed_;
		}

		void reset(bool releasMemory=true)
		{
			scoped_lock_type lock(mutex_);
			needRecalc_=true;
			lastForgetTime_=-1;
			timestart_=-1;
			totalBytes_=0;
			totalCnt_=0;
			quedBytes_=0;
			lastSpeed_=0;
			if (releasMemory)
				history_=history_type();//to release memory
		}

		void operator+=(std::size_t bytes)const
		{		
			scoped_lock_type lock(mutex_);

			tick_type curTick=time_traits_type::now_tick_count();

			__forget_history(curTick);

			if (-1==timestart_)
				timestart_=curTick;
			
			needRecalc_=true;//que changed, need recalc
			totalBytes_+=bytes;
			totalCnt_+=1;
			quedBytes_+=(int)bytes;
			if (!history_.empty())
			{
				typename history_type::reference elm=history_.back();
				if(elm.timestamp==curTick)//for performance
				{
					elm.bytes+=bytes;
					return;
				}
			}
			history_.push_back(_pair(curTick,bytes));
		}

		int64_t total_bytes() const { return totalBytes_;}
		int64_t total_count() const { return totalCnt_;}

	private:
		bool __forget_history(tick_type now)const
		{
			if (lastForgetTime_==now)
				return false;

			needRecalc_=true;//time changed, need recalc
			lastForgetTime_=now;
			tick_type t=(now-timeWindow_);
			//size_t cnt=history_.size();
			while (!history_.empty())
			{
				typename history_type::reference elm=history_.front();
				if (elm.timestamp<t)
				{
					quedBytes_-=elm.bytes;
					history_.pop_front();
					BOOST_ASSERT(quedBytes_>=0);
				}
				else
					break;
			}
			return true;
		}

	private:
		mutable history_type	history_;  
		mutable int				quedBytes_;
		mutable tick_type       timeWindow_;
		mutable tick_type       timestart_;
		mutable tick_type		lastForgetTime_;
		mutable double			lastSpeed_;
		mutable int64_t			totalBytes_;
		mutable int64_t			totalCnt_;
		mutable mutex_type      mutex_;
		mutable bool			needRecalc_;
	};

	typedef basic_speed_meter<rough_tick_time>   rough_speed_meter;
	typedef basic_speed_meter<precise_tick_time> precise_speed_meter;

	typedef basic_speed_meter<rough_tick_time,fast_mutex>   threadsafe_rough_speed_meter;
	typedef basic_speed_meter<precise_tick_time,fast_mutex> threadsafe_precise_speed_meter;

	PTR_TYPE_DECLARE(rough_speed_meter);
	PTR_TYPE_DECLARE(precise_speed_meter);
}

#endif // P2ENGINE_SPEEDMETER_H__
