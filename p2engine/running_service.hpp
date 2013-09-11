//
// static_running_service.hpp
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

#ifndef P2ENGINE_STATIC_running_ASIO_HPP
#define P2ENGINE_STATIC_running_ASIO_HPP

#include "p2engine/push_warning_option.hpp"
#include "p2engine/config.hpp"
#include <boost/thread.hpp>
#include "p2engine/pop_warning_option.hpp"

#include "p2engine/singleton.hpp"

namespace p2engine {

	template<std::size_t THREAD_COUNT=1>
	class running_service{
	public:
		running_service(){
		}
		virtual ~running_service(){
			join();
		}
		io_service& get_running_io_service()
		{
			if (!running_service_ios_.get())
			{
				boost::recursive_mutex::scoped_lock lock(running_service_mutex_);
				if (!running_service_ios_.get())
				{
					running_service_threads_.clear();
					running_service_threads_error_.clear();
					running_service_ios_.reset(new io_service);
					running_service_work_.reset(new io_service::work(*running_service_ios_));
					for (std::size_t i=0;i<THREAD_COUNT;++i)
					{
						running_service_threads_error_.push_back(error_code());
						running_service_threads_.push_back(boost::shared_ptr<boost::thread>(
							new boost::thread(boost::bind(&io_service::run,running_service_ios_.get(),
							running_service_threads_error_[i])
							)));
					}
				}
			}
			return *running_service_ios_;
		}
		void join(int wait_time=-1)
		{
			//printf("---------------running_service joining....\n");
			try
			{
				boost::recursive_mutex::scoped_lock lock(running_service_mutex_);
				if (running_service_work_.get())
				{
					running_service_work_.reset();
					running_service_ios_->stop();
					for (std::size_t i=0;i<THREAD_COUNT;++i)
					{
						if (running_service_threads_[i]->joinable()
							&&running_service_threads_[i]->get_id()!=boost::this_thread::get_id()
							)
						{
							if (wait_time>0)
								running_service_threads_[i]->timed_join(millisec(wait_time));
							else
								running_service_threads_[i]->join();
						}
					}
					running_service_threads_.clear();
				}
			}
			catch (...)
			{
			}
			//printf("---------------running_service--joined!\n");
		}
	protected:
		std::auto_ptr<io_service> running_service_ios_;
		std::auto_ptr<io_service::work> running_service_work_;
		std::vector<boost::shared_ptr<boost::thread> >running_service_threads_;
		std::vector<error_code >running_service_threads_error_;
		boost::recursive_mutex running_service_mutex_;
	};


}

#endif

