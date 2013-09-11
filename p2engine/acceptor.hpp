//
// acceptor.hpp
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

#ifndef p2engine_acceptor_hpp__
#define p2engine_acceptor_hpp__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "p2engine/basic_dispatcher.hpp"
#include "p2engine/shared_access.hpp"
#include "p2engine/connection.hpp"

namespace p2engine {

	namespace urdp{
		template<typename BaseConnectionType>
		class basic_urdp_connection;
		template<typename ConnectionType,typename ConnectionBaseType>
		class basic_urdp_acceptor;
	}
	namespace trdp{
		template<typename BaseConnectionType>
		class basic_trdp_connection;
		template<typename ConnectionType,typename ConnectionBaseType>
		class basic_trdp_acceptor; 
	}

	template<typename ConnectionBaseType>
	class basic_acceptor
		:public basic_engine_object
		,public basic_acceptor_dispatcher<ConnectionBaseType>
		,public fssignal::trackable
	{
		typedef basic_acceptor<ConnectionBaseType> this_type;
		SHARED_ACCESS_DECLARE;

	public:
		typedef variant_endpoint endpoint_type;
		typedef variant_endpoint endpoint;
		typedef ConnectionBaseType connection_base_type;
		typedef urdp::basic_urdp_connection<connection_base_type> urdp_connection_type;
		typedef trdp::basic_trdp_connection<connection_base_type> trdp_connection_type;
		typedef urdp::basic_urdp_acceptor<urdp_connection_type,connection_base_type> urdp_acceptor_type;
		typedef trdp::basic_trdp_acceptor<trdp_connection_type,connection_base_type> trdp_acceptor_type;

	protected:
		basic_acceptor(io_service&ios,bool realTimeUsage)
			:basic_engine_object(ios)
			,domain_(INVALID_DOMAIN)
			,b_real_time_usage_(realTimeUsage)
		{}
		virtual ~basic_acceptor(){};

	public:
		enum acceptor_t{
			TCP,//tcp message connection acceptor
			UDP,//udp message connection acceptor(urdp)
			MIX//mix tcp and udp
		};
	public:
		//listen on a local_edp to accept connections of domainName
		//listen(localEdp,"bittorrent/p2p",ec)
		virtual error_code listen(const endpoint& local_edp,
			const std::string& domainName,
			error_code& ec)=0;

		virtual error_code listen(const endpoint& local_edp, 
			error_code& ec)=0;

		virtual void keep_async_accepting()=0;
		virtual void block_async_accepting()=0;

		error_code close(){error_code ec; return close(ec);}
		virtual error_code close(error_code& ec)=0;

		virtual endpoint local_endpoint(error_code& ec) const=0;

	public:
		bool is_real_time_usage()const
		{
			return b_real_time_usage_;
		}
		const std::string& domain()const{return domain_;};

	protected:
		std::string domain_;
		bool b_real_time_usage_;
	};

} // namespace p2engine

#endif//basic_urdp_acceptor_h__


