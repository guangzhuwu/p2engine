
#include <p2engine/push_warning_option.hpp>
#include <iostream>
#include <set>
#include <boost/shared_ptr.hpp>
#include <p2engine/pop_warning_option.hpp>

#include <p2engine/p2engine.hpp>
#include <p2engine/http/http.hpp>

using namespace p2engine;

typedef http::basic_http_connection<http::http_connection_base> http_connection;
typedef http::basic_http_acceptor<http_connection,http_connection> http_acceptor;

typedef boost::shared_ptr<http_acceptor> acceptor_ptr;
typedef boost::shared_ptr<http_connection> connection_ptr;

class http_server
	:public fssignal::trackable
{
	typedef http_server this_type;
public:
	http_server(io_service& ios)
		:ios_(ios)
	{

		acceptor_=http_acceptor::create(ios);

		//listen on port of 8080
		http_acceptor::endpoint localEdp(address(),8080);
		error_code ec;
		acceptor_->listen(localEdp,ec);
		if (ec)
			acceptor_->close();

		//bind handler of accept
		acceptor_->accepted_signal().bind(&this_type::on_accept,this,_1,_2);

		//start
		acceptor_->keep_async_accepting();
	}

protected:
	void on_accept(connection_ptr conn, error_code ec)
	{
		if (!ec)
		{
			connections_.insert(conn);

			std::cout<<"accept: "<<conn->remote_endpoint(ec)<<std::endl;

			conn->disconnected_signal().bind(&this_type::on_disconnected,this,conn.get());
			conn->received_request_header_signal().bind(&this_type::on_request,this,_1,conn.get());
			conn->received_data_signal().bind(&this_type::on_data,this,_1);
			conn->writable_signal().bind(&this_type::close_socket,this,conn.get());
		}

	}


	void on_request(const http::request& req,http_connection* conn)
	{
		std::cout<<req;//print req


		//response
		std::string body="<head></head><body>Hello!</body>";

		http::response res;
		res.status(http::header::HTTP_OK);
		res.content_length(body.length());

		safe_buffer buf;
		safe_buffer_io bio(&buf);
		bio<<res;//serielize to send buffer
		bio.write(body.c_str(),body.length());

		std::cout<<"---:"<<std::string(buffer_cast<char*>(buf),buf.length())<<std::endl;
		conn->async_send(buf);
	}

	void on_disconnected(http_connection* conn)
	{
		connections_.erase(conn->shared_obj_from_this<http_connection>());
	}

	void on_data(safe_buffer data)
	{
		//...
	}

	void close_socket(http_connection* conn)
	{
		connections_.erase(conn->shared_obj_from_this<http_connection>());
	}

protected:
	acceptor_ptr acceptor_;
	std::set<connection_ptr> connections_;
	io_service& ios_;
};
void main()
{
	io_service ios;
	http_server server(ios);
	ios.run();
}
