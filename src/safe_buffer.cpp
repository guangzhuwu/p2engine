#include "p2engine/safe_buffer.hpp"

namespace p2engine{
	//////////////////////////////////////////////////////////////////////////
	//safe_buffer_base
	//////////////////////////////////////////////////////////////////////////
	void safe_buffer_base::clear()
	{
		if (raw_buffer_)
		{
			gptr(raw_buffer_ptr());
			pptr(raw_buffer_ptr());
		}
		else
		{
			gptr(NULL);
			pptr(NULL);
		}
	}

	void safe_buffer_base::reserve(std::size_t len)
	{
		if (len==0)
			return;
		if (capacity()<len)
		{
			raw_buffer_ = raw_buffer::create(len);
			gptr(raw_buffer_ptr());
			pptr(raw_buffer_ptr());
			BOOST_ASSERT(gptr()&&pptr());
		}
	}

	void safe_buffer_base::realloc(std::size_t len)
	{
		if (len==0)
		{
			reset();
			return;
		}
		safe_buffer_base oldBuffer(*this);
		reserve(len);
		if (oldBuffer.raw_buffer_==raw_buffer_)
			return;
		if (oldBuffer.size()>0)
		{
			size_t theLen=std::min(oldBuffer.size(),len);
			memcpy(gptr(),oldBuffer.gptr(),theLen);
			pptr(gptr()+theLen);
		}
		BOOST_ASSERT(capacity()==len);
	}

	safe_buffer_base safe_buffer_base::clone() const
	{
		if (raw_buffer_)
		{
			safe_buffer_base buf(size());
			std::memcpy(buf.gptr(), gptr(), size());
			return buf;
		}
		else
		{
			return safe_buffer_base();
		}
	}

	safe_buffer_base safe_buffer_base::buffer_ref(std::ptrdiff_t byte_offset,
		std::size_t buffer_length )
	{
		if (buffer_length == (std::numeric_limits<std::size_t>::max)())
			buffer_length = length() - byte_offset;
		safe_buffer_base buf(*this);
		buf.g_ptr_+=byte_offset;
		buf.pptr(buf.gptr()+buffer_length);

		BOOST_ASSERT(
			empty()&&byte_offset==0&&buffer_length==0
			||
			!empty()&&buf.pptr()>=buf.gptr()&&
			(raw_buffer_
			||(buf.pptr()<=raw_buffer_ptr()+raw_buffer_->size()
			&&buf.gptr()>=raw_buffer_ptr())
			)
			);

		return buf;
	}

	std::size_t safe_buffer_base::memset(uint8_t byte_val, 
		std::size_t len , 
		std::size_t byte_offset)
	{
		BOOST_ASSERT(raw_buffer_ != NULL);
		BOOST_ASSERT(byte_offset <= length());
		if (len == (std::numeric_limits<std::size_t>::max)())
			len = pptr()-(gptr() + byte_offset);
		BOOST_ASSERT(byte_offset+len<=length());
		std::memset(gptr() + byte_offset, byte_val, len);
		return len;
	}

}//namespace p2engine
