#ifndef P2ENGINE_HTTP_CONNECTION_BASE_HPP
#define P2ENGINE_HTTP_CONNECTION_BASE_HPP

#include "p2engine/push_warning_option.hpp"
#include "p2engine/config.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/date_time.hpp>
#include <string>
#include "p2engine/pop_warning_option.hpp"

#include "p2engine/time.hpp"
#include "p2engine/contrib.hpp"
#include "p2engine/fssignal.hpp"
#include "p2engine/variant_endpoint.hpp"
#include "p2engine/basic_dispatcher.hpp"
#include "p2engine/basic_engine_object.hpp"

#include "p2engine/http/response.hpp"
#include "p2engine/http/request.hpp"
#include "p2engine/http/basic_http_dispatcher.hpp"

namespace p2engine { namespace http {

	namespace detail{

		class basic_http_connection_base
		{
			typedef basic_http_connection_base this_type;
			SHARED_ACCESS_DECLARE;

		public:
			typedef variant_endpoint endpoint_type;
			typedef variant_endpoint endpoint;
			typedef this_type connection_base_type;
			typedef tcp::socket::lowest_layer_type lowest_layer_type;

		protected:
			basic_http_connection_base(bool enable_ssl,bool isPassive)
				:is_passive_(isPassive),enable_ssl_(enable_ssl)
			{}
			virtual ~basic_http_connection_base(){};

		public:
			virtual error_code open(const endpoint& local_edp, error_code& ec,
				const time_duration& life_time=boost::date_time::pos_infin,
				const proxy_settings& ps=proxy_settings()
				)=0;

			template<typename Option>
			void set_option(const Option& option,error_code& ec)
			{
				this->lowest_layer().set_option(option,ec);
			}

			virtual void async_connect(const std::string& remote_host, int port, 
				const time_duration& time_out=boost::date_time::pos_infin
				)=0;
			virtual void  async_connect(const endpoint& peer_endpoint,
				const time_duration& time_out=boost::date_time::pos_infin
				)=0;

			//reliable send
			virtual void async_send(const safe_buffer& buf)=0;

			virtual void keep_async_receiving()=0;
			virtual void block_async_receiving()=0;

			virtual lowest_layer_type& lowest_layer()=0;

			virtual void close(bool greaceful=true)=0;
			virtual bool is_open() const=0;
			virtual bool is_connected()const=0;

			virtual endpoint local_endpoint(error_code& ec)const=0;
			virtual endpoint remote_endpoint(error_code& ec)const=0;

			bool is_passive()const {return is_passive_;}
			bool enable_ssl()const {return enable_ssl_;}

			virtual std::size_t overstocked_send_size()const=0;

		protected:
			bool is_passive_;
			bool enable_ssl_;
		};
	}


	class http_connection_base
		:public basic_engine_object
		,public detail::basic_http_connection_base
		,public http_connection_dispatcher
		,public fssignal::trackable
	{
		typedef http_connection_base this_type;
		SHARED_ACCESS_DECLARE;

	public:
		typedef variant_endpoint endpoint_type;
		typedef variant_endpoint endpoint;
		typedef this_type connection_base_type;

	protected:
		http_connection_base(io_service& ios,bool enable_ssl,bool isPassive)
			:basic_engine_object(ios)
			,detail::basic_http_connection_base(enable_ssl,isPassive) 
		{}

	};

} // namespace http
} // namespace p2engine

#endif // P2ENGINE_HTTP_CONNECTION_BASE_HPP
