//
// basic_aiofile.hpp
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

#ifndef AIOFILE_BASIC_AIOFILE_HPP
#define AIOFILE_BASIC_AIOFILE_HPP

#include "p2engine/file/file_api.hpp"
#include "p2engine/file/basic_io_handle.hpp"

namespace p2engine{

	template <typename RandomAccessService>
	class basic_aiofile
		:public basic_io_handle<RandomAccessService>
	{
	public:
		typedef typename basic_io_handle<RandomAccessService>::native_handle_type native_handle_type;
		typedef typename basic_io_handle<RandomAccessService>::native_handle_type native_type;
		typedef typename basic_io_handle<RandomAccessService>::lowest_layer_type  lowest_layer_type;

		explicit basic_aiofile(boost::asio::io_service& io_service)
			: basic_io_handle<RandomAccessService>(io_service)
		{
		}

		basic_aiofile(boost::asio::io_service& io_service,
			const native_handle_type& handle)
			: basic_io_handle<RandomAccessService>(io_service, handle)
		{
		}

#if defined(BOOST_ASIO_HAS_MOVE) 
		basic_aiofile(basic_aiofile&& other)
			: basic_io_handle<RandomAccessService>(
			BOOST_ASIO_MOVE_CAST(basic_aiofile)(other))
		{
		}

		basic_aiofile& operator=(basic_aiofile&& other)
		{
			basic_io_handle<RandomAccessService>::operator=(
				BOOST_ASIO_MOVE_CAST(basic_aiofile)(other));
			return *this;
		}
#endif // defined(BOOST_ASIO_HAS_MOVE) 
		void open(const boost::filesystem::path& path, int mode, 
			boost::system::error_code& ec)
		{
			this->service.open(this->implementation, path, mode, ec);
		}

		void open(const boost::filesystem::path& path, int  mode)
		{
			boost::system::error_code ec;
			this->service.open(this->implementation, path, mode, ec);
			if (ec) throw boost::system::system_error(ec);
		}

		void open(const boost::filesystem::path& path, boost::system::error_code& ec)
		{
			this->service.open(this->implementation, path, read_write, ec);
		}

		template <typename Handler>
		void async_open(const boost::filesystem::path& path, int mode,
			const Handler& handler)
		{
			this->service.async_open(this->implementation,path,mode,handler);
		}

		void seek(int64_t offset, int original, boost::system::error_code& ec)
		{
			this->service.seek(this->implementation,offset,original,ec);
		}

		void seek(int64_t offset, int original)
		{
			boost::system::error_code ec;
			this->service.seek(this->implementation,offset,original,ec);
			if (ec) throw boost::system::system_error(ec);
		}

		template <typename ConstBufferSequence>
		std::size_t write_some_at(boost::uint64_t offset,
			const ConstBufferSequence& buffers)
		{
			boost::system::error_code ec;
			std::size_t s = this->service.write_some_at(
				this->implementation, offset, buffers, ec);
			boost::asio::detail::throw_error(ec, "write_some_at");
			return s;
		}

		template <typename ConstBufferSequence>
		std::size_t write_some_at(boost::uint64_t offset,
			const ConstBufferSequence& buffers, boost::system::error_code& ec)
		{
			return this->service.write_some_at(
				this->implementation, offset, buffers, ec);
		}

		template <typename ConstBufferSequence, typename WriteHandler>
		void async_write_some_at(boost::uint64_t offset,
			const ConstBufferSequence& buffers,
			BOOST_ASIO_MOVE_ARG(WriteHandler) handler)
		{
			this->service.async_write_some_at(this->implementation,
				offset, buffers, BOOST_ASIO_MOVE_CAST(WriteHandler)(handler));
		}

		template <typename MutableBufferSequence>
		std::size_t read_some_at(boost::uint64_t offset,
			const MutableBufferSequence& buffers)
		{
			boost::system::error_code ec;
			std::size_t s = this->service.read_some_at(
				this->implementation, offset, buffers, ec);
			boost::asio::detail::throw_error(ec, "read_some_at");
			return s;
		}

		template <typename MutableBufferSequence>
		std::size_t read_some_at(boost::uint64_t offset,
			const MutableBufferSequence& buffers, boost::system::error_code& ec)
		{
			return this->service.read_some_at(
				this->implementation, offset, buffers, ec);
		}

		template <typename MutableBufferSequence, typename ReadHandler>
		void async_read_some_at(boost::uint64_t offset,
			const MutableBufferSequence& buffers,
			BOOST_ASIO_MOVE_ARG(ReadHandler) handler)
		{
			this->service.async_read_some_at(this->implementation,
				offset, buffers, BOOST_ASIO_MOVE_CAST(ReadHandler)(handler));
		}

	};

}// namespace asfio

#endif//AIOFILE_BASIC_AIOFILE_HPP
