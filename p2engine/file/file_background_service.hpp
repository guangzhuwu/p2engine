//
// file_background_service.hpp
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

#ifndef AIOFILE_AIO_SERVICE_HPP
#define AIOFILE_AIO_SERVICE_HPP

#include "p2engine/file/file_api.hpp"

namespace p2engine{

	template <typename File>
	class file_background_service
		: public boost::asio::detail::service_base< file_background_service<File> >
	{
	public:
		typedef typename File::implementation_type implementation_type;

		explicit file_background_service(boost::asio::io_service &io_service)
			: boost::asio::detail::service_base< file_background_service<File> >(io_service)
			, worker_work(get_work_service())
			, worker_thread(boost::bind(&boost::asio::io_service::run,&get_work_service()))
		{
		}

		template <typename Handler>
		void async_open(File& file, implementation_type& impl,
			const boost::filesystem::path& path, int flags, const Handler& handler)
		{
			worker_service.post(helper_open_file<Handler>(this->get_io_service(), file,
				impl, path, flags, handler));
		}

		boost::asio::io_service& get_work_service()
		{
			return worker_service;
		}

	private:
		void shutdown_service()
		{
			worker_service.stop();
			worker_thread.join();
		}

		template <typename Handler>
		struct helper_open_file {
			boost::asio::io_service& service;
			boost::filesystem::path path;
			int flags;
			File& file;
			implementation_type& impl;
			Handler handler;

			helper_open_file(boost::asio::io_service& service_, File& file_, 
				implementation_type& impl_, const boost::filesystem::path& path_, 
				int flags_, const Handler& handler_)
				: service(service_), path(path_), flags(flags_), file(file_)
				, impl(impl_), handler(handler_)
			{
			}

			void operator()()
			{
				boost::system::error_code res;
				file.open(impl, path, flags, res);
				service.post(boost::bind(handler, res));
			}
		};

		boost::asio::io_service worker_service;//must before worker_work and worker_thread
		boost::asio::io_service::work worker_work;
		boost::thread worker_thread;
	};

}

#endif//AIOFILE_AIO_SERVICE_HPP
