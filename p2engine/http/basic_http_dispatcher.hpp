//
// basic_http_dispatcher.hpp
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
#ifndef BASIC_HTTP_DISPATCHER_HPP
#define BASIC_HTTP_DISPATCHER_HPP

#include "p2engine/basic_dispatcher.hpp"
#include "p2engine/http/request.hpp"
#include "p2engine/http/response.hpp"
#include "p2engine/fssignal.hpp"
#include "p2engine/safe_buffer.hpp"
#include "p2engine/logging.hpp"

namespace p2engine { namespace http {

	class http_connection_dispatcher
		:private basic_connection_dispatcher<void_message_extractor>
	{
		typedef http_connection_dispatcher this_type;
		typedef http_connection_dispatcher dispatcher_type;
		typedef basic_connection_dispatcher<void_message_extractor> basic_dispatcher;

		typedef fssignal::signal<void(const request&)>		received_request_header_signal_type;
		typedef fssignal::signal<void(const response&)>		received_response_header_signal_type;

		SHARED_ACCESS_DECLARE;

	protected:
		virtual ~http_connection_dispatcher(){}

	public:
		received_request_header_signal_type& received_request_header_signal() 
		{
			return request_handler_;
		}
		const received_request_header_signal_type& received_request_header_signal() const
		{
			return request_handler_;
		}
		received_response_header_signal_type& received_response_header_signal() 
		{
			return response_handler_;
		}
		const received_response_header_signal_type& received_response_header_signal() const
		{
			return response_handler_;
		}
		static received_request_header_signal_type& global_received_request_header_signal()
		{
			return s_request_handler_;
		}
		static received_response_header_signal_type& global_received_response_header_signal()
		{
			return s_response_handler_;
		}
		received_signal_type& received_data_signal() 
		{
			return this->received_signal();
		}
		const received_signal_type& received_data_signal()const
		{
			return this->received_signal();
		}
		static received_signal_type& global_received_data_signal()
		{
			return s_msg_handler_;
		}
		using basic_dispatcher::connected_signal;
		using basic_dispatcher::disconnected_signal;
		using basic_dispatcher::writable_signal;

		virtual void disconnect_all_slots()
		{
			basic_dispatcher::disconnect_all_slots();

			request_handler_.disconnect_all_slots();
			response_handler_.disconnect_all_slots();
		}
		
		using basic_dispatcher::dispatch_connected;
		using basic_dispatcher::dispatch_disconnected;
		using basic_dispatcher::dispatch_packet;
		using basic_dispatcher::dispatch_sendout;

		bool dispatch_request(request& buf);
		bool dispatch_response(response& buf);

	public:
		received_request_header_signal_type request_handler_;
		received_response_header_signal_type response_handler_;

		static received_request_header_signal_type s_request_handler_;
		static received_response_header_signal_type s_response_handler_;
	};


	template<typename HttpSocket>
	class basic_http_acceptor_dispatcher
	{
		typedef basic_http_acceptor_dispatcher<HttpSocket> this_type;
		SHARED_ACCESS_DECLARE;

		typedef HttpSocket socket_type;
		typedef basic_http_acceptor_dispatcher<socket_type> dispatcher_type;
		typedef typename boost::shared_ptr<socket_type> connection_sptr;
		typedef fssignal::signal<void(connection_sptr,const error_code&)> accepted_signal_type;

	protected:
		virtual ~basic_http_acceptor_dispatcher(){}

	public:
		accepted_signal_type& accepted_signal()
		{
			return accept_handler_;
		}

		const accepted_signal_type& accepted_signal()const
		{
			return accept_handler_;
		}

		virtual void dispatch_accepted(connection_sptr sock, const error_code& ec)
		{
			accepted_signal()(sock,ec);
		}

		void disconnect_all_slots()
		{
			accept_handler_.clear();
		}

	protected:
		accepted_signal_type accept_handler_;
	};

}
}

#endif//BASIC_HTTP_DISPATCHER_HPP
