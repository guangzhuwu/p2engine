//
// safe_buffer_io.hpp
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

#ifndef P2ENGINE_SAFE_BUFFER_IO_HPP
#define P2ENGINE_SAFE_BUFFER_IO_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "p2engine/push_warning_option.hpp"
#include "p2engine/config.hpp"
#include <boost/limits.hpp>
#include <streambuf>
#include <cassert>
#include <cstring>
#include <memory>
#include "p2engine/pop_warning_option.hpp"

#include "p2engine/io.hpp"
#include "p2engine/safe_buffer.hpp"
#include "p2engine/variant_endpoint.hpp"

namespace p2engine {
	class safe_buffer_io
	{
		enum { buffer_delta = 256 };
		enum { usually_buffer_size = 1500 };
	public:
		explicit safe_buffer_io(safe_buffer_base* buffer)
			:buffer_(*buffer)
		{
		}

		void clear()
		{
			buffer_.clear();
			BOOST_ASSERT(buffer_.size()==0);
		}

		/// Return the size of the get area in characters.
		std::size_t size() const
		{
			return buffer_.size();
		}

		//!! NOT SAFE
		const char* pptr() const
		{
			return (const char*)buffer_.pptr();
		}

		//!! NOT SAFE
		const char* gptr() const
		{
			return (const char*)buffer_.gptr();
		}

		safe_buffer_io& operator+=(const safe_buffer_io& other)
		{
			buffer_.realloc(buffer_.size()+other.size());
			memcpy((char*)pptr(),other.gptr(), other.size());
			commit(other.size());
			return *this;
		}

		template<typename T>
		friend safe_buffer_io& operator << (safe_buffer_io&,const T& value);

		template<typename T>
		friend safe_buffer_io& operator >> (safe_buffer_io&, T& value);


		template<class Endpoint>
		void read_v4_endpoint(Endpoint& edp)
		{
			asio::ip::address_v4 addr;
			(*this)>>addr;
			uint16_t port;
			(*this)>>port;
			edp.address(addr);
			edp.port(port);
		}
		template<class Endpoint>
		void read_v6_endpoint(Endpoint& edp)
		{
			asio::ip::address_v6 addr;
			(*this)>>addr;
			uint16_t port;
			(*this)>>port;
			edp.address(addr);
			edp.port(port);
		}

		std::size_t write(const void*buf,std::size_t len)
		{
			if (len==0||!buf)
				return 0;
			buffer_.realloc(buffer_.size()+len);
			BOOST_ASSERT(buffer_.capacity()>=buffer_.size()+len);
			BOOST_ASSERT(buffer_.pptr());
			BOOST_ASSERT(buf);
			std::memcpy(buffer_.pptr(),buf,len);
			commit(len);
			return len;
		}
		std::size_t read(void*buf,std::size_t len)
		{
			len=(std::min)(len,buffer_.size());
			uint8_t* p=(uint8_t*)buffer_.gptr();
			if (len==0)
				return 0;
			memcpy(buf,p,len);
			consume(len);
			return len;
		}

		std::size_t write(const std::string& str,size_t len)
		{
			len=(std::min)(len,str.size());
			return write(str.c_str(),len);
		}
		std::size_t write(const std::string& str)
		{
			return write(str.c_str(),str.length());
		}
		std::size_t read(std::string& str, std::size_t len)
		{
			len=(std::min)(len,buffer_.size());
			char* p=(char*)buffer_.gptr();
			if (len==0)
				return 0;
			str.append(p,len);
			consume(len);
			return len;
		}

		std::size_t write(const safe_buffer& buf,size_t len)
		{
			len=(std::min)(len,buf.size());
			return write(buffer_cast<const char*>(buf),len);
		}
		std::size_t write(const safe_buffer& buf)
		{
			return write(buffer_cast<const char*>(buf),buf.length());
		}
		std::size_t read(safe_buffer& buf, std::size_t len)
		{
			len=(std::min)(len,buffer_.size());
			uint8_t* p=(uint8_t*)buffer_.gptr();
			if (len==0)
				return 0;
			safe_buffer_io dest(&buf);
			dest.write(p,len);
			consume(len);
			return len;
		}

		//Get a list of buffers that represents the put area, with the given size.
		void  prepare(std::size_t n)
		{
			if (buffer_.capacity()<n+buffer_.size())
			{
				if (n<buffer_delta&&(buffer_.size()+buffer_delta<usually_buffer_size))
					n=buffer_delta;
				buffer_.realloc(buffer_.size()+n);
			}
		}

		/// Move the start of the put area by the specified number of characters.
		void commit(std::size_t n)
		{
			buffer_.commit(n);
		}

		/// Move the start of the get area by the specified number of characters.
		void consume(std::size_t n)
		{
			buffer_.consume(n);
		}

	protected:
		template <typename IntType> 
		void _write(IntType v) 
		{
			BOOST_STATIC_ASSERT(boost::is_integral<IntType>::value);
			prepare(sizeof(IntType));
			uint8_t* p=(uint8_t*)buffer_.pptr();
			write_int_hton(v,p);
			commit(sizeof(IntType));
		}

		template <typename IntType>
		IntType _read() 
		{
			BOOST_STATIC_ASSERT(boost::is_integral<IntType>::value);

			BOOST_ASSERT(size()>=sizeof(IntType));
			if (size()<sizeof(IntType))
			{
				consume(size());
				return IntType();
			}
			uint8_t* p=(uint8_t*)buffer_.gptr();
			IntType v=read_int_ntoh<IntType>(p);
			consume(sizeof(IntType));
			return v; 
		}

	private:
		safe_buffer_base& buffer_;
	};

	template<typename T>
	inline safe_buffer_io& operator << (safe_buffer_io&io, const T& value) 
	{ 
		io._write<T>(value); 
		return io; 
	}
	template<typename T>
	inline safe_buffer_io& operator >> (safe_buffer_io&io,T& value) 
	{
		value = io._read<T>(); 
		return io; 
	}

	template<>
	inline safe_buffer_io& operator << (safe_buffer_io&io,const std::string& value) 
	{ 
		io.write(&value[0],value.size());
		return  io; 
	}
	template<>
	inline safe_buffer_io& operator >> (safe_buffer_io&io,std::string& value) 
	{ 
		value.append(io.gptr(),io.size());
		io.consume(io.size());
		return io;
	}

	template<>
	inline safe_buffer_io& operator << (safe_buffer_io&io,const safe_buffer& value) 
	{ 
		io.write(value);
		return  io; 
	}
	template<>
	inline safe_buffer_io& operator >> (safe_buffer_io&io,safe_buffer& value) 
	{ 
		io.read(value,io.size());
		return io;
	}

	template<>
	inline safe_buffer_io& operator << (safe_buffer_io&io,const bool& value) 
	{ 
		io._write<uint8_t>((uint8_t)value);
		return  io; 
	}
	template<>
	inline safe_buffer_io& operator >> (safe_buffer_io&io,bool& value) 
	{ 
		value = io._read<uint8_t>() == (uint8_t)0 ? false : true;
		return io;
	}

	template<>
	inline safe_buffer_io& operator << (safe_buffer_io&io,const asio::ip::address& a) 
	{ 
		if (a.is_v4())
		{
			io<<a.to_v4().to_ulong();
		}
		else if (a.is_v6())
		{
			asio::ip::address_v6::bytes_type bytes
				= a.to_v6().to_bytes();
			io.write(&bytes[0],bytes.size());
		}
		return io;
	}

	template<>
	inline safe_buffer_io& operator >> (safe_buffer_io&io,asio::ip::address_v4& a) 
	{ 
		uint32_t ip;
		io>>ip;
		a=asio::ip::address_v4(ip);
		return io;
	}

	template<>
	inline safe_buffer_io& operator >> (safe_buffer_io&io,asio::ip::address_v6& a) 
	{ 
		typedef asio::ip::address_v6::bytes_type bytes_t;
		bytes_t bytes;
		io.read(&bytes[0],bytes.size());
		a=asio::ip::address_v6(bytes);
		return io;
	}

	template<>
	inline safe_buffer_io& operator << (safe_buffer_io&io,const variant_endpoint& a) 
	{ 
		io<<(a.address());
		io<<a.port();
		return io;
	}

	template <typename InternetProtocol>
	inline safe_buffer_io& operator << (safe_buffer_io&io,
		const boost::asio::ip::basic_endpoint<InternetProtocol>& a) 
	{ 
		io<<(a.address());
		io<<a.port();
		return io;
	}

} // namespace p2engine

#endif // P2ENGINE_SAFE_BUFFER_IO_HPP
