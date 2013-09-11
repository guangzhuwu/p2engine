//
// ntp.hpp
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

#ifndef P2ENGINE_NTPCLIENT_HPP
#define P2ENGINE_NTPCLIENT_HPP

#include "p2engine/config.hpp"
#include "p2engine/time.hpp"
#include "p2engine/fssignal.hpp"
#include "p2engine/basic_engine_object.hpp"
#include "p2engine/coroutine.hpp"
#include "p2engine/timer.hpp"

#include "p2engine/push_warning_option.hpp"
#include <string>
#include "p2engine/pop_warning_option.hpp"

namespace p2engine{

	class ntp_client
		:public basic_engine_object
		,public fssignal::trackable
	{
		typedef ntp_client this_type;
		SHARED_ACCESS_DECLARE;

	public:
		typedef fssignal::signal<void(const ptime&,const error_code& ec)> on_gottime_signal_type;

		static shared_ptr create(io_service& engine_svc,const std::string& ntpServer="pool.ntp.org")
		{
			return shared_ptr(new this_type(engine_svc,ntpServer),
				shared_access_destroy<this_type>());
		}

		void gettime(){__gettime();}
		on_gottime_signal_type& gottime_signal(){return signal_;}
		const on_gottime_signal_type& gottime_signal()const{return signal_;}

	private:
		ntp_client(io_service& ios,const std::string& ntpServer="pool.ntp.org");

	private:
		void __gettime(const error_code& ec=error_code(),  
			udp::resolver::iterator iterator=udp::resolver::iterator(),
			coroutine coro=coroutine(),
			tick_type requestTime=-1
			);
		void __time_out();

	private:
		std::string ntp_server_host_;
		udp::resolver resolver_;
		udp::socket socket_;
		on_gottime_signal_type signal_;
		char buf_[48];
		rough_timer::shared_ptr timer_;
	};
}
#endif