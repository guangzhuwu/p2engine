//
// normal_aiofile_service.hpp
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

#ifndef p2engine_file_normal_aiofile_service_hpp__
#define p2engine_file_normal_aiofile_service_hpp__

#ifdef AIOFILE_NORMAL

//#ifdef WIN32
//#	error please use WIN_AIOFILE_SERVICE
//#endif
#include "p2engine/config.hpp"
#include "p2engine/file/file_api.hpp"
#include "p2engine/file/file_background_service.hpp"
#include "p2engine/atomic.hpp"
#include "p2engine/wrappable_integer.hpp"

namespace p2engine{ namespace detail{

	class normal_random_access_handle_service
		:public boost::asio::detail::service_base<normal_random_access_handle_service>
	{
		file_background_service<normal_random_access_handle_service>& get_background_service()
		{
			return boost::asio::use_service<
				file_background_service<normal_random_access_handle_service> 
			>(this->get_io_service());
		}

		void shutdown_service()
		{
		}

	public:
		// The native type of a stream handle.
		typedef int native_handle_type;
		typedef native_handle_type native_type;

		// The implementation type of the stream handle.
		class implementation_type
		{
		public:
			// Default constructor.
			implementation_type()
				: fd(-1), op_count(0)
			{
			}

		//private:
			// Only this get_io_service() will have access to the internal values.
			friend class normal_random_access_handle_service;

			// The native FILE handle representation.
			native_handle_type fd;
			atomic<int32_t> op_count;
		};

		explicit normal_random_access_handle_service(boost::asio::io_service& io_service)
			: boost::asio::detail::service_base<normal_random_access_handle_service>
			(io_service)
		{
		}

		virtual ~normal_random_access_handle_service()
		{
		}

		void destroy(implementation_type& impl)
		{
			boost::system::error_code ec;
			close(impl, ec);
		}

		void construct(implementation_type& impl)
		{
			impl.fd = -1;
			++impl.op_count;
		}

		boost::system::error_code assign(implementation_type& impl,
			const native_handle_type& handle, boost::system::error_code& ec)
		{
			if (is_open(impl))
			{
				ec = boost::asio::error::already_open;
				return ec;
			}
			impl.fd = handle;
			++impl.op_count;
			ec = boost::system::error_code();
			return ec;
		}

		void open(implementation_type& impl,const boost::filesystem::path& path, 
			int mode, boost::system::error_code& err)
		{
			if (is_open(impl))
			{
				err = boost::asio::error::already_open;
				return;
			}
			native_handle_type fd=open_file(path,mode,err);
			if (err) 
				return;
			assign(impl,fd,err);
		}

		template <typename Handler>
		void async_open(implementation_type& impl,const boost::filesystem::path& path,
			int mode, const Handler& handler)
		{
			get_background_service().async_open(*this, impl, path, mode, handler);
		}

		bool is_open(const implementation_type& impl)const
		{
			return impl.fd!=-1;
		}

		void close(implementation_type& impl,boost::system::error_code& err)
		{
			err.clear();
			++impl.op_count;
			if (impl.fd != -1) 
			{
				if(::close(impl.fd))
				{
					err = boost::system::error_code(errno,
						boost::asio::error::get_system_category());
				}
				impl.fd = -1;
				return;
			}
		}

		native_type native_handle(implementation_type& impl)
		{
			return impl.fd;
		}

		template <typename MBS, typename Handler>
		void async_read_some(implementation_type& impl, const MBS& mbs, 
			Handler handler)
		{
			get_background_service().get_work_service().dispatch(
				bind_read_some(get_io_service(), mbs, impl, handler)
				);
		}
		template <typename MBS, typename Handler>
		void async_read_some_at(implementation_type& impl, boost::uint64_t offset, 
			const MBS& mbs, Handler handler)
		{
			get_background_service().get_work_service().dispatch(
				bind_read_some(get_io_service(), mbs, impl, handler, offset)
				);
		}

		template <typename CBS, typename Handler>
		void async_write_some(implementation_type& impl, const CBS& cbs, 
			Handler handler)
		{
			get_background_service().get_work_service().dispatch(
				bind_write_some(get_io_service(), cbs, impl, handler)
				);
		}
		template <typename CBS, typename Handler>
		void async_write_some_at(implementation_type& impl, boost::uint64_t offset,
			const CBS& cbs, Handler handler)
		{
			get_background_service().get_work_service().dispatch(
				bind_write_some(get_io_service(), cbs, impl, handler, offset)
				);
		}

		void seek(implementation_type& impl, int64_t offset, int origin,
			boost::system::error_code& err) 
		{
			if (impl.fd == -1) 
			{
				err = boost::asio::error::bad_descriptor;
				return;
			}
			if(::lseek(impl.fd, offset,origin)<0)
				err = boost::system::error_code(errno,boost::system::get_system_category());
			else
				err.clear();
		}

		boost::system::error_code cancel(implementation_type& impl, 
			boost::system::error_code& ec)
		{
			++impl.op_count;
			ec=boost::system::error_code();
			return ec;
		}

	private:
		template <typename CBS, typename Handler>
		struct helper_write_some {
			boost::asio::io_service& service;
			boost::int64_t offset;
			CBS cbs;
			implementation_type& impl;
			int32_t op_count;
			Handler handler;

			helper_write_some(boost::asio::io_service& service_, 
				const CBS& cbs_, implementation_type& impl_, Handler& handler_,
				boost::int64_t offset_=(std::numeric_limits<boost::int64_t>::max)()
				)
				: service(service_), offset(offset_), cbs(cbs_), impl(impl_)
				, op_count(impl_.op_count)
				, handler(BOOST_ASIO_MOVE_CAST(Handler)(handler_))
			{
			}

			void operator()() {

				using boost::asio::error::get_system_category;

				if(wrappable_greater<int32_t>()((int32_t)impl.op_count,op_count))
				{
					boost::system::error_code ec=boost::asio::error::operation_aborted;
					service.dispatch(boost::bind<void>(handler,ec,00));
					return;
				}
				if (impl.fd == -1) {
					boost::system::error_code ec=boost::asio::error::bad_descriptor;
					service.dispatch(boost::bind<void>(handler,ec,00));
					return;
				}
				if (offset!=(std::numeric_limits<boost::int64_t>::max)())
				{
					if(::lseek(impl.fd,offset,SEEK_SET)<0)
					{
						service.dispatch(boost::bind<void>(handler,
							boost::system::error_code(errno,boost::system::get_system_category()),
							0));
						return;
					}
				}
				size_t totalWriteLen=0;
				size_t totalBufLen=0;
				typename CBS::const_iterator itr = cbs.begin();
				typename CBS::const_iterator end = cbs.end();
				for (;itr != end; ++itr) 
				{
					// at least 1 buffer
					boost::asio::const_buffer buffer(*itr);
					const void* buf = boost::asio::buffer_cast<const void*>(buffer);
					size_t buflen = boost::asio::buffer_size(buffer);

					if (0==buflen)
						continue;

					totalBufLen+=buflen;
					int written = ::write(impl.fd, buf, buflen);
					if (written<0)
						break;
					totalWriteLen+=written;
					if (written!=buflen)
						break;
				}

				if (totalBufLen == 0) {
					// no-op
					service.dispatch(boost::bind<void>(handler, boost::system::error_code(), 0));
					return;
				}
				else if (totalWriteLen!=totalBufLen)
				{
					if (totalWriteLen==0)
					{
						service.dispatch(boost::bind<void>(handler,
							boost::system::error_code(errno,boost::system::get_system_category()),
							0)
							);
					}
					else {
						// this is an error, but we ignore it and hope the next read will return 0
						// this is so we can report this partial success to the handler
						service.dispatch(boost::bind<void>(handler, boost::system::error_code(), totalWriteLen));
					}
				}
				else
				{
					service.dispatch(boost::bind<void>(handler, boost::system::error_code(), totalWriteLen));
				}
			}
		};
		template <typename CBS, typename Handler>
		inline helper_write_some<CBS,Handler> bind_write_some(
			boost::asio::io_service& service_, 
			const CBS& cbs_, implementation_type& impl_, Handler& handler_,
			boost::int64_t offset_=(std::numeric_limits<boost::int64_t>::max)())
		{
			return helper_write_some<CBS,Handler>(service_,cbs_,impl_,handler_,offset_);
		}

		template <typename MBS, typename Handler>
		struct helper_read_some {
			boost::asio::io_service& service;
			boost::int64_t offset;
			MBS mbs;
			implementation_type& impl;
			long op_count;
			Handler handler;

			helper_read_some(boost::asio::io_service& service_, 
				const MBS& mbs_, implementation_type& impl_, Handler& handler_,
				boost::int64_t offset_=(std::numeric_limits<boost::int64_t>::max)()
				)
				: service(service_), offset(offset_),mbs(mbs_), impl(impl_)
				, op_count(impl_.op_count)
				, handler(BOOST_ASIO_MOVE_CAST(Handler)(handler_))
			{
			}
			void operator()() {
				if(wrappable_greater<int32_t>()((int32_t)impl.op_count,op_count))
				{
					service.dispatch(boost::bind<void>(handler,
						boost::system::error_code(boost::asio::error::operation_aborted),
						0)
						);
					return;
				}
				if (impl.fd == -1) {
					service.dispatch(boost::bind<void>(handler,
						boost::system::error_code(boost::asio::error::bad_descriptor),
						0)
						);
					return;
				}
				if (offset!=(std::numeric_limits<boost::int64_t>::max)())
				{
					if(::lseek(impl.fd,offset,SEEK_SET)<0)
					{
						service.dispatch(boost::bind<void>(handler,
							boost::system::error_code(errno,boost::system::get_system_category()),
							0));
						return;
					}
				}
				size_t totalReadLen=0;
				size_t totalBufLen=0;
				typename MBS::const_iterator itr = mbs.begin();
				typename MBS::const_iterator end = mbs.end();
				for (;itr != end; ++itr) {
					// at least 1 buffer
					boost::asio::mutable_buffer buffer(*itr);
					void* buf = boost::asio::buffer_cast<void*>(buffer);
					size_t buflen = boost::asio::buffer_size(buffer);
					if (buflen == 0)
						continue;

					totalBufLen+=buflen;
					int read = ::read(impl.fd, buf,buflen);
					if (read<0)
						break;
					totalReadLen+=read;
					if (read !=buflen)
						break;
				}
				if (totalBufLen == 0) {
					// no-op
					service.dispatch(boost::bind<void>(handler, boost::system::error_code(), 0));
					return;
				}
				else if (totalReadLen!=totalBufLen)
				{
					if (totalReadLen==0)
					{
						service.dispatch(boost::bind<void>(handler,
							boost::system::error_code(errno,boost::system::get_system_category()),
							0)
							);
					}
					else {
						// this is an error, but we ignore it and hope the next read will return 0
						// this is so we can report this partial success to the handler
						service.dispatch(boost::bind<void>(handler, boost::system::error_code(), totalReadLen));
					}
				}
				else
				{
					service.dispatch(boost::bind<void>(handler, boost::system::error_code(), totalReadLen));
				}
			}
		};

		template <typename MBS, typename Handler>
		inline helper_read_some<MBS,Handler> bind_read_some(
			boost::asio::io_service& service_, 
			const MBS& mbs_, implementation_type& impl_, Handler& handler_,
			boost::int64_t offset_=(std::numeric_limits<boost::int64_t>::max)())
		{
			return helper_read_some<MBS,Handler>(service_,mbs_,impl_,handler_,offset_);
		}

	private:
		implementation_type impl;
	};

}
}

#endif //AIOFILE_NORMAL

#endif// p2engine_file_normal_aiofile_service_hpp__
