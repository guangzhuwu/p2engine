// packet.hpp
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

#ifndef P2ENGINE_PACKET_HPP
#define P2ENGINE_PACKET_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "p2engine/basic_packet.hpp"

namespace p2engine {

	template<typename PacketFormat>
	class packet 
		:public PacketFormat
	{
		typedef packet<PacketFormat> this_type;
		typedef PacketFormat packet_format_base_type;

	public:
		packet()
		{
			safe_buffer buf(this->packet_size());
			this->reset(buf, true);
		};

		explicit packet(const safe_buffer& buf,bool init_format=false)
		{
			this->reset(buf, init_format);
		};

		safe_buffer payload()const
		{
			return this->buffer().buffer_ref(this->packet_size());
		}

		template<class OStream>
		void dump_as_readable_text(OStream& os) const
		{
			this->dump(os);
		}

		template<class OStream>
		void dump_as_hex(OStream& os) const
		{
			this->buffer().dump(os);
		}

		template<class OStream, typename U>
		friend OStream& operator<<(OStream& os, const packet<U>& packet);

	};
	template<class OStream, typename U>
	OStream& operator<<(OStream& os, const packet<U>& pkt)
	{
		pkt.dump_as_readable_text(os);
		return os;
	}
}

#endif // P2ENGINE_PACKET_HPP

