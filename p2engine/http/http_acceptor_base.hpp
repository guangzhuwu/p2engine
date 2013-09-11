#ifndef P2ENGINE_HTTP_ACCEPTOR_BASE_HPP
#define P2ENGINE_HTTP_ACCEPTOR_BASE_HPP

#include "p2engine/config.hpp"
#include "p2engine/time.hpp"
#include "p2engine/basic_dispatcher.hpp"
#include "p2engine/basic_engine_object.hpp"
#include "p2engine/fssignal.hpp"
#include "p2engine/variant_endpoint.hpp"
#include "p2engine/contrib.hpp"

#include "p2engine/http/response.hpp"
#include "p2engine/http/request.hpp"
#include "p2engine/http/basic_http_dispatcher.hpp"

namespace p2engine { namespace http {

	template<typename ConnectionBaseType>
	class http_acceptor_base
		:public basic_engine_object
		,public basic_http_acceptor_dispatcher<ConnectionBaseType>
		,public fssignal::trackable
	{
		typedef http_acceptor_base<ConnectionBaseType> this_type;
		SHARED_ACCESS_DECLARE;

	public:
		typedef variant_endpoint endpoint_type;
		typedef variant_endpoint endpoint;
		typedef ConnectionBaseType connection_base_type;
		typedef tcp::acceptor lowest_layer_type;

	protected:
		http_acceptor_base(io_service&ios)
			:basic_engine_object(ios)
		{}
		virtual ~http_acceptor_base(){};

	public:
		virtual error_code open(const endpoint& local_edp,error_code& ec)=0;

		virtual error_code listen(error_code& ec)=0;
		
		virtual error_code listen(const endpoint& local_edp,error_code& ec)=0;

		template<typename Option>
		void set_option(const Option& option,error_code& ec)
		{
			this->lowest_layer().set_option(option,ec);
		}

		virtual void keep_async_accepting()=0;
		virtual void block_async_accepting()=0;

		error_code close(){error_code ec; return close(ec);}
		virtual error_code close(error_code& ec)=0;
		virtual bool is_open()const=0;

		virtual lowest_layer_type& lowest_layer()=0;

		virtual endpoint local_endpoint(error_code& ec) const=0;

	};


} // namespace http
} // namespace p2engine

#endif // P2ENGINE_HTTP_CONNECTION_BASE_HPP
