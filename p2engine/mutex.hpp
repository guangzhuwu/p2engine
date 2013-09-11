//
// mutex.hpp
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
//You should have received a copy of the GNU General Public License auint32_t 
//with this program; if not, contact <guangzhuwu@gmail.com>.
//

#ifndef p2engine_mutex_hpp__
#define p2engine_mutex_hpp__


#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "p2engine/config.hpp"

namespace p2engine{

	template <typename Mutex>
	class scoped_lock
		: private boost::noncopyable
	{
	public:
		scoped_lock(Mutex& m)
			: mutex_(m)
		{
			mutex_.lock();
			locked_ = true;
		}

		~scoped_lock()
		{
			if (locked_)
				mutex_.unlock();
		}

		void lock()
		{
			if (!locked_)
			{
				mutex_.lock();
				locked_ = true;
			}
		}

		void unlock()
		{
			if (locked_)
			{
				mutex_.unlock();
				locked_ = false;
			}
		}

		bool locked() const
		{
			return locked_;
		}

		Mutex& mutex()
		{
			return mutex_;
		}

	private:
		Mutex& mutex_;
		bool locked_;
	};

	template<typename MutexType>
	class deadlock_assert_mutex
		:public MutexType
	{
		DEBUG_SCOPE(;
	public:
		deadlock_assert_mutex():locked_(false){}

		void lock()
		{
			BOOST_ASSERT(locked_==false);
			locked_=true;
			MutexType::lock();
		}
		void unlock()
		{
			DEBUG_SCOPE(
				BOOST_ASSERT(locked_==true);
			locked_=false;
			);
			MutexType::unlock();
		}
		typedef p2engine::scoped_lock<deadlock_assert_mutex<MutexType> > scoped_lock;
	private:
		bool locked_;

		);
	};

	class null_mutex
	{
	public:
		typedef ::p2engine::scoped_lock<null_mutex> scoped_lock;

		// Constructor.
		null_mutex()
		{
		}

		// Destructor.
		~null_mutex()
		{
		}

		// Lock the mutex.
		void lock()
		{
		}

		// Unlock the mutex.
		void unlock()
		{
		}
	};

	typedef boost::asio::detail::mutex fast_mutex;
}

#endif//p2engine_mutex_hpp__

