//
// safe_buffer_base.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2009-2010  GuangZhu Wu
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
//
// THANKS  Meng Zhang <albert.meng.zhang@gmail.com>
//

#ifndef P2ENGINE_SAFE_BUFFER_HPP
#define P2ENGINE_SAFE_BUFFER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "p2engine/raw_buffer.hpp"
#include "p2engine/io.hpp"

namespace p2engine {

	struct buffer_cast_helper;
	class safe_buffer;
	class safe_buffer_io;
	class asio_const_buffer;
	class asio_mutable_buffer;
	class asio_const_buffers_1;
	class asio_mutable_buffers_1;

	class safe_buffer_base
	{
		friend class asio_const_buffer;
		friend class asio_mutable_buffer;
		friend class asio_const_buffers_1;
		friend class asio_mutable_buffers_1;
		friend class safe_buffer_io;
		friend struct buffer_cast_helper;

		typedef safe_buffer_base this_type;
		typedef boost::intrusive_ptr<raw_buffer> raw_buffer_type;

		BOOST_STATIC_CONSTANT(std::size_t,BUFFER_DELTA=128);
	public:
		safe_buffer_base():p_ptr_(NULL),g_ptr_(NULL)
		{}
		explicit safe_buffer_base(std::size_t len)
			: raw_buffer_(raw_buffer::create(((std::max))(len,(std::size_t)BUFFER_DELTA)))
		{
			gptr(raw_buffer_ptr());
			pptr(raw_buffer_ptr()+len);
		}
		safe_buffer_base(const safe_buffer_base &buf,
			std::size_t trucated_len = (std::numeric_limits<std::size_t>::max)())
		{
			this->reset(buf, trucated_len);
		}
		safe_buffer_base(const void* data,std::size_t data_len)
			: raw_buffer_(raw_buffer::create(((std::max))(data_len,(std::size_t)BUFFER_DELTA)))
		{
			gptr(raw_buffer_ptr());
			memcpy(raw_buffer_ptr(),data,data_len);
			pptr(raw_buffer_ptr()+data_len);
		}
		virtual ~safe_buffer_base(){}

		safe_buffer_base& operator=(const safe_buffer_base& buf)
		{
			this->reset(buf);
			return *this;
		}

		safe_buffer_base clone() const;

		safe_buffer_base buffer_ref(std::ptrdiff_t byte_offset,
			std::size_t buffer_length = (std::numeric_limits<std::size_t>::max)());

		const safe_buffer_base buffer_ref(std::ptrdiff_t byte_offset,
			std::size_t buffer_length = (std::numeric_limits<std::size_t>::max)()) const
		{
			return const_cast<safe_buffer_base*>(this)->buffer_ref(byte_offset,buffer_length);
		}

		void reset(const safe_buffer_base &buf, std::size_t trucated_len = (std::numeric_limits<std::size_t>::max)())
		{
			if (this!=&buf)
			{
				raw_buffer_ = buf.raw_buffer_;
				g_ptr_ = buf.g_ptr_;
			}
			p_ptr_ = g_ptr_+(std::min)(buf.length(), trucated_len);
		}

		void reset()
		{
			raw_buffer_.reset();
			gptr(NULL);
			pptr(NULL);
		}

		void swap(safe_buffer_base&buf)
		{
			if (this!=&buf)
			{
				raw_buffer_.swap(buf.raw_buffer_);
				std::swap(p_ptr_, buf.p_ptr_);
				std::swap(g_ptr_, buf.g_ptr_);
			}
		}

		void clear();
		
		//just alloc buffer of len. size()==0
		void reserve(std::size_t len);
		
		//alloc buffer and copy old data to the new buffer
		//size()==min(old.size(),len)
		void realloc(std::size_t len);

		//alloc buffer and copy old data to the new buffer
		//size()==len
		void resize(std::size_t len)
		{
			realloc(len);
			pptr(gptr()+len);
		}

		void recreate(std::size_t len)
		{
			raw_buffer_ = raw_buffer::create(len);
			gptr(raw_buffer_ptr());
			pptr(gptr()+len);
		}

		std::size_t capacity()const
		{
			if (raw_buffer_)
			{
				return raw_buffer_len()-(gptr()-raw_buffer_ptr());
			}
			return 0;
		}

		std::size_t size() const 
		{
			return length();
		};

		std::size_t length() const 
		{
			if (!raw_buffer_)
				return 0;
			BOOST_ASSERT(raw_buffer_len() >=(std::size_t)(p_ptr_ - g_ptr_));
			return p_ptr_ - g_ptr_;
		};

		bool empty()const
		{
			return !length();
		}

		operator bool() const
		{
			return this->raw_buffer_!=NULL;
		}

		template <typename T>
		T get(std::size_t byte_offset = 0) const
		{
			BOOST_ASSERT(this->raw_buffer_ != NULL);
			BOOST_ASSERT(gptr()+byte_offset + sizeof(T) <= pptr());
			const char* pos=gptr()+byte_offset;
			return read_value<T>(pos);
		}
		template <typename T>
		void set(T val, std::size_t byte_offset = 0)
		{
			BOOST_ASSERT(this->raw_buffer_ != NULL);
			BOOST_ASSERT(gptr() + byte_offset + sizeof(T) <= pptr());
			char* pos=gptr()+byte_offset;
			write_value<T>(val,pos);
		}

		template <typename T>
		T get_nth_elem(std::size_t n, std::size_t byte_offset = 0) const
		{
			return get<T>(byte_offset+n*sizeof(T));
		}
		template <typename T>
		void set_nth_elem(std::size_t n, const T &val, std::size_t byte_offset = 0)
		{
			return set<T>(val,byte_offset+n*sizeof(T));
		}
		std::size_t get_data(void * buf, std::size_t buf_size, std::size_t len, std::size_t byte_offset = 0) const
		{
			BOOST_ASSERT(this->raw_buffer_ != NULL);
			BOOST_ASSERT(buf_size >= len);
			BOOST_ASSERT(length()>=byte_offset);
			(void)(buf_size);
			std::size_t true_len = (std::min)(len, length()-byte_offset);
			std::memcpy(buf, gptr()+byte_offset, true_len);
			return true_len;
		}
		void set_data(const void * buf, std::size_t len, std::size_t byte_offset = 0)
		{
			BOOST_ASSERT(this->raw_buffer_ != NULL);
			BOOST_ASSERT(byte_offset+len<=length());
			std::memcpy(gptr()+ byte_offset, buf, len);
		}
		std::size_t get_data(safe_buffer_base &buf, std::size_t len, std::size_t byte_offset = 0) const
		{
			return get_data(buf.gptr(),buf.length(),len,byte_offset);
		}
		void set_data(const safe_buffer_base& buf, std::size_t len, std::size_t byte_offset = 0)
		{
			BOOST_ASSERT(buf.raw_buffer_ != NULL);
			BOOST_ASSERT(buf.length() >= len);
			set_data(buf.gptr(),len,byte_offset);
		}

		std::size_t memset(uint8_t byte_val, std::size_t len = (std::numeric_limits<std::size_t>::max)(),
			std::size_t byte_offset = 0);

		template <class OStream>
		OStream& dump(OStream& os, bool dump_as_char = false,
			std::size_t trucated_len = (std::numeric_limits<std::size_t>::max)(),
			std::size_t bytes_per_line = static_cast<std::size_t>(0x10)) const
		{
			if (!raw_buffer_)
			{
				os <<"<null>\n";
			}
			else
			{
				const char* p=gptr();
				os <<"<<"<<(ptrdiff_t)p<<">>\n";
				BOOST_ASSERT(trucated_len == (std::numeric_limits<std::size_t>::max)() ||
					trucated_len <= this->length());
				if (this->length() == 0)
				{
					os <<"<null>"<<std::endl;
				}
				else
				{
					std::size_t offset;
					for(offset = 0; offset < (std::min)(trucated_len, this->length());
						++ offset)
					{
						if (offset % bytes_per_line == 0)
						{
							os.fill('0');
							os.width(sizeof(uint32_t) * 2);
							os <<std::hex<<std::uppercase<<static_cast<uint32_t>(offset)
								<<"h:";
						}
						uint8_t byte_val = static_cast<uint8_t>(p[offset]);
						os <<" ";
						os.fill('0');
						os.width(sizeof(char) * 2);
						os <<std::hex<<std::uppercase<<static_cast<unsigned long>(static_cast<unsigned char>(byte_val));
						if (offset % bytes_per_line == bytes_per_line - 1)
						{
							if (dump_as_char)
							{
								os <<" ; ";
								for(std::size_t i = offset + 1 - bytes_per_line; i <= offset; ++ i)
									os <<static_cast<char>(p[i]);
							}
							os <<std::endl;
						}
					}
					if (offset % bytes_per_line != 0)
					{
						os <<std::endl;
					}
				}
			}
			os.width(0); os.fill(' '); os <<std::dec;
			os.flush();
			return os;
		}

	protected:
		void consume(std::size_t n)
		{
			BOOST_ASSERT(size()>=n);
			g_ptr_+=n;
		}
		void commit(std::size_t n)
		{
			BOOST_ASSERT(capacity()>=n+size());
			p_ptr_+=n;
		}
		
		char* gptr()const
		{
			BOOST_ASSERT(raw_buffer_||!g_ptr_);
			return g_ptr_; 
		}
		void gptr(char* p)
		{
			BOOST_ASSERT(!p
				||(p>=raw_buffer_ptr()&&p<(raw_buffer_ptr()+raw_buffer_len()))
				);
			g_ptr_=p;
		}
		char* pptr()const
		{
			BOOST_ASSERT(raw_buffer_||!p_ptr_);
			return p_ptr_; 
		}
		void pptr(char* p)
		{
			BOOST_ASSERT(!p
				||(p>=raw_buffer_ptr()&&p<=(raw_buffer_ptr()+raw_buffer_len()))//<=
				);
			p_ptr_=p;
		}
		
		const char* raw_buffer_ptr()const
		{
			BOOST_ASSERT(raw_buffer_);
			return raw_buffer_->buffer();
		}
		char* raw_buffer_ptr()
		{
			BOOST_ASSERT(raw_buffer_);
			return raw_buffer_->buffer();
		}
		std::size_t raw_buffer_len()const
		{
			BOOST_ASSERT(raw_buffer_);
			return raw_buffer_->length();
		}

	protected:
		raw_buffer_type raw_buffer_;
		char* p_ptr_;
		char* g_ptr_;
	};

	template<class OStream>
	inline OStream& operator<<(OStream& os, const safe_buffer_base& buf)
	{
		return buf.dump(os);
	}

	class asio_const_buffer : public boost::asio::const_buffer
	{
	public:
		asio_const_buffer(const safe_buffer_base &buf, std::size_t trucated_len)
			: boost::asio::const_buffer(buf.gptr(), trucated_len), safe_buffer_base_(buf)
		{
			BOOST_ASSERT(trucated_len <= buf.length());
		}
		asio_const_buffer(const safe_buffer_base &buf, std::size_t offset,
			std::size_t trucated_len) 
			: boost::asio::const_buffer((char*)buf.gptr()+offset, trucated_len)
			, safe_buffer_base_(buf)
		{
			BOOST_ASSERT(trucated_len+offset <= buf.length());
		}
		asio_const_buffer(const asio_const_buffer &buf)
			: boost::asio::const_buffer(static_cast<const asio::const_buffer&>(buf)),
			safe_buffer_base_(buf.safe_buffer_base_)
		{};
	private:
		safe_buffer_base safe_buffer_base_;
	};

	class asio_mutable_buffer : public boost::asio::mutable_buffer
	{
	public:
		asio_mutable_buffer(const safe_buffer_base &buf, std::size_t trucated_len)
			: boost::asio::mutable_buffer(buf.gptr(), trucated_len), safe_buffer_base_(buf)
		{
			BOOST_ASSERT(trucated_len <= buf.length());
		}
		asio_mutable_buffer(const safe_buffer_base &buf, std::size_t offset,
			std::size_t trucated_len) 
			: boost::asio::mutable_buffer((char*)buf.gptr()+offset, trucated_len), safe_buffer_base_(buf)
		{
			BOOST_ASSERT(trucated_len+offset <= buf.capacity());
		}
		asio_mutable_buffer(const asio_mutable_buffer &buf)
			: boost::asio::mutable_buffer(static_cast<const asio::mutable_buffer&>(buf)),
			safe_buffer_base_(buf.safe_buffer_base_)
		{};
	private:
		safe_buffer_base safe_buffer_base_;
	};

	class asio_const_buffers_1 : public boost::asio::const_buffers_1
	{
	public:
		asio_const_buffers_1(const safe_buffer_base &buf, std::size_t trucated_len)
			: boost::asio::const_buffers_1(buf.gptr(), trucated_len), safe_buffer_base_(buf)
		{
			BOOST_ASSERT(trucated_len <= buf.length());
		}
		asio_const_buffers_1(const asio_const_buffers_1 &buf)
			: boost::asio::const_buffers_1(static_cast<const asio::const_buffers_1&>(buf)),
			safe_buffer_base_(buf.safe_buffer_base_)
		{};
	private:
		safe_buffer_base safe_buffer_base_;
	};

	class asio_mutable_buffers_1 : public boost::asio::mutable_buffers_1
	{
	public:
		asio_mutable_buffers_1(const safe_buffer_base &buf, std::size_t trucated_len)
			: boost::asio::mutable_buffers_1(buf.gptr(), trucated_len), safe_buffer_base_(buf)
		{
			BOOST_ASSERT(trucated_len <= buf.length());
		}
		asio_mutable_buffers_1(const asio_mutable_buffers_1 &buf)
			: boost::asio::mutable_buffers_1(static_cast<const asio::mutable_buffers_1&>(buf)),
			safe_buffer_base_(buf.safe_buffer_base_)
		{};
	private:
		safe_buffer_base safe_buffer_base_;
	};

	class safe_buffer : public safe_buffer_base
	{
		friend class const_asio_safe_buffer_base;
		friend class mutable_asio_safe_buffer_base;
		friend class safe_buffer_io;
	public:
		safe_buffer() : safe_buffer_base() {};
		safe_buffer(std::size_t len) : safe_buffer_base(len) {};
		safe_buffer(const safe_buffer_base &buf,
			std::size_t trucated_len = (std::numeric_limits<std::size_t>::max)())
			: safe_buffer_base(buf, trucated_len) {};
		safe_buffer(const void* data, std::size_t data_len )
			: safe_buffer_base(data, data_len) {};

		safe_buffer buffer_ref(std::ptrdiff_t byte_offset,
			std::size_t buffer_length = (std::numeric_limits<std::size_t>::max)()) const
		{
			return safe_buffer_base::buffer_ref(byte_offset,buffer_length);
		}

		asio_const_buffer to_asio_const_buffer(std::size_t trucated_len = (std::numeric_limits<std::size_t>::max)()) const
		{
			BOOST_ASSERT(raw_buffer_);
			BOOST_ASSERT(trucated_len == (std::numeric_limits<std::size_t>::max)() || trucated_len <= length());
			return asio_const_buffer(*this, (trucated_len == (std::numeric_limits<std::size_t>::max)()) ? length() : trucated_len);
		};
		asio_mutable_buffer to_asio_mutable_buffer(std::size_t trucated_len = (std::numeric_limits<std::size_t>::max)()) const
		{
			BOOST_ASSERT(raw_buffer_);
			BOOST_ASSERT(trucated_len == (std::numeric_limits<std::size_t>::max)() || trucated_len <= length());
			return asio_mutable_buffer(*this, (trucated_len == (std::numeric_limits<std::size_t>::max)()) ? length() : trucated_len);
		};
		asio_const_buffers_1 to_asio_const_buffers_1(std::size_t trucated_len = (std::numeric_limits<std::size_t>::max)()) const
		{
			BOOST_ASSERT(raw_buffer_);
			BOOST_ASSERT(trucated_len == (std::numeric_limits<std::size_t>::max)() || trucated_len <= length());
			return asio_const_buffers_1(*this, (trucated_len == (std::numeric_limits<std::size_t>::max)()) ? length() : trucated_len);
		};
		asio_mutable_buffers_1 to_asio_mutable_buffers_1(std::size_t trucated_len = (std::numeric_limits<std::size_t>::max)()) const
		{
			BOOST_ASSERT(raw_buffer_);
			BOOST_ASSERT(trucated_len == (std::numeric_limits<std::size_t>::max)() || trucated_len <= length());
			return asio_mutable_buffers_1(*this, (trucated_len == (std::numeric_limits<std::size_t>::max)()) ? length() : trucated_len);
		};
	};

	struct buffer_cast_helper
	{
		void* operator()(const safe_buffer_base&buf){return buf.gptr();}
	};

	template <typename PointerToPodType>
	inline PointerToPodType buffer_cast(const boost::asio::const_buffer& b)
	{
		return boost::asio::buffer_cast<PointerToPodType>(b);
	}
	inline std::size_t buffer_size(const boost::asio::const_buffer& b)
	{
		return boost::asio::buffer_size(b);
	}
	template <typename PointerToPodType>
	inline PointerToPodType buffer_cast(const safe_buffer_base& b)
	{
		return static_cast<PointerToPodType>(buffer_cast_helper()(b));
	}
	inline std::size_t buffer_size(const safe_buffer_base& b)
	{
		return b.size();
	}

	template <class SafeBufferSequence, class ConstAsioBufferSequence>
	void to_asio_const_buffers(const SafeBufferSequence& safe_buffer_bases, 
		ConstAsioBufferSequence& asio_buffers)
	{
		typedef typename SafeBufferSequence::value_type buffer_type;
		BOOST_STATIC_ASSERT((boost::is_base_of<safe_buffer_base, buffer_type>::value));
		asio_buffers.reserve(asio_buffers.size()+safe_buffer_bases.size());
		BOOST_FOREACH(const buffer_type& safebuf,safe_buffer_bases)
		{
			asio_buffers.push_back(safebuf.to_asio_const_buffer());
		}
	}

	template <class SafeBufferSequence, class MutableAsioBufferSequence>
	void to_asio_mutable_buffers(const SafeBufferSequence& safe_buffer_bases, 
		MutableAsioBufferSequence& asio_buffers)
	{
		typedef typename SafeBufferSequence::value_type buffer_type;
		BOOST_STATIC_ASSERT((boost::is_base_of<safe_buffer_base,buffer_type>::value));
		asio_buffers.reserve(asio_buffers.size()+safe_buffer_bases.size());
		BOOST_FOREACH(const buffer_type& safebuf,safe_buffer_bases)
		{
			asio_buffers.push_back(safebuf.to_asio_mutable_buffer());
		}
	}

} // namespace p2engine

#endif // P2ENGINE_SAFE_BUFFER_HPP

