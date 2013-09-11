#include <p2engine/push_warning_option.hpp>
#include <boost/variant.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/timer.hpp>
#include <iostream>
#include <p2engine/pop_warning_option.hpp>

#include <p2engine/fssignal.hpp>
#include <p2engine/rdp.hpp>

using namespace p2engine;

std::string domain="/p2p/pingpong";

//Define your connection type.
//It must dirived from basic_connection.
class connection_base
	:public basic_connection
{
protected:
	connection_base(io_service& ios,bool realTimeUsage,bool isPassive)
		:basic_connection(ios,realTimeUsage,isPassive){}

public:
	//add your member functions
	void print_local_endpoint()
	{
		error_code ec;
		std::cout<<"local_endpoint:"<<local_endpoint(ec)<<std::endl;
	};
};
//typedef basic_urdp_connection<connection_base> my_connection;//the real connection_type be used
//typedef basic_urdp_acceptor<my_connection,connection_base> my_acceptor;//the acceptor that accepts boost::shared_ptr<basic_connection>
typedef basic_trdp_connection<connection_base> my_connection;//the real connection_type be used
typedef basic_trdp_acceptor<my_connection,connection_base> my_acceptor;//the acceptor that accepts boost::shared_ptr<basic_connection>

//we suppose that the server listen on port of 8888
inline endpoint server_endpoint()
{
	return endpoint(address(address_v4::loopback()),8888);
}

//define message type
enum{
	PING_MSG=1,
	PONG_MSG,
	BYE_MSG
};

//the clinet
class client
	:public fssignal::trackable
{
	typedef client this_type;

public:
	client(io_service& ios):ios_(ios){}
	void run()
	{
		rough_timer_=rough_timer::create(ios_);
		rough_timer_->time_signal().bind(&this_type::connect,this);
		rough_timer_->async_wait(seconds(1));
	}

	void dump()
	{
		std::cout<<socket_->alive_probability()<<std::endl;
	}
private:
	void connect()
	{
		//create a connection that will be used to connect server
		socket_=my_connection::create(ios_,true);

		//bind callback functions 
		socket_->connected_signal().bind(&this_type::on_connected,this,_1);
		socket_->disconnected_signal().bind(&this_type::on_disconnected,this,_1);

		//bind message processing functions
		socket_->received_signal(PONG_MSG).bind(&this_type::on_received_pong,this,_1);//this will be called when PONG_MSG recvd
		socket_->received_signal(BYE_MSG).bind(&this_type::on_received_bye,this,_1);//this will be called when BYE_MSG recvd

		socket_->async_connect(server_endpoint(),domain);
	}
	void on_connected(const error_code& ec)
	{
		std::cout<<"on_connected, error_msg:"<<ec.message()<<std::endl;

		safe_buffer buf;
		safe_buffer_io io(&buf);
		std::string s(1,'a');
		io.write(s.c_str(),s.length());

		//we send 10 ping message to server
		for (int i=0;i<10;i++)
		{
			std::string x=boost::lexical_cast<std::string>(i);
			if (i%3==0)
			{
				socket_->async_send_reliable(buf.clone(),PING_MSG);
			}
			else if(i%3==1)
			{
				std::string x=" unreliable";
				socket_->async_send_unreliable(buf.clone(),PING_MSG);
			}
			else
			{
				std::string x=" semireliable";
				socket_->async_send_semireliable(buf.clone(),PING_MSG);
			}
		}
	}
	void on_disconnected(const error_code& ec)
	{
		std::cout<<"on_disconnected, error_msg:"<<ec.message()<<std::endl;
	}

	void on_received_pong(safe_buffer buf)
	{
		std::string s(buffer_cast<char*>(buf),buf.size());
		std::cout<<"PONG FROM SERVER:"<<s<<std::endl;
	}
	void on_received_bye(safe_buffer buf)
	{
		std::string s(buffer_cast<char*>(buf),buf.size());
		std::cout<<"BYE FROM SERVER:"<<s<<std::endl;
		socket_->close();
	}
private:
	boost::shared_ptr<connection_base> socket_;
	io_service& ios_;
	boost::shared_ptr<rough_timer> rough_timer_;
};

int NMSG=0;
class server
	:public fssignal::trackable
{
	typedef server this_type;
public:
	server(io_service& ios):ios_(ios)
	{
	}
	void run()
	{
		//create acceptor
		acceptor_=my_acceptor::create(ios_,true);

		//bind callback function
		acceptor_->accepted_signal().bind(&this_type::on_accepted,this,_1,_2);

		//listen on server_endpoint 
		error_code ec;
		acceptor_->listen(server_endpoint(),domain,ec);
		if (!ec)
			acceptor_->keep_async_accepting();
	}

private:
	//accept a new connection
	void on_accepted(boost::shared_ptr<basic_connection> socket,const error_code& ec)
	{
		std::cout<<"on_accepted, error_msg:"<<ec.message()<<std::endl;
		sockets_.insert(socket);

		//bind callback functions for this connection
		socket->disconnected_signal().bind(&this_type::on_disconnected,this,socket.get(),_1);
		socket->received_signal(PING_MSG).bind(&this_type::on_received_ping,this,socket.get(),_1);
	}

	void on_disconnected(basic_connection*sock, const error_code& ec)
	{
		std::cout<<"on_disconnected, error_msg:"<<ec.message()<<std::endl;
		sockets_.erase(sock->shared_obj_from_this<basic_connection>());
	}

	void on_received_ping(basic_connection*sock, safe_buffer buf)
	{
		NMSG++;
		std::string s(buffer_cast<char*>(buf),buf.size());
		std::cout<<"ping from clinet:"<<s<<std::endl;

		sock->async_send_reliable(buf,PONG_MSG);

		if (NMSG>=10)
		{
			safe_buffer byeMsg;
			safe_buffer_io io(&byeMsg);
			io.write("bye-bye",strlen("bye-bye"));
			sock->async_send_reliable(byeMsg,BYE_MSG);

			sock->close();
			sockets_.erase(sock->shared_obj_from_this<basic_connection>());
		}
	}
private:
	boost::shared_ptr<my_acceptor> acceptor_;
	io_service& ios_;
	std::set<boost::shared_ptr<basic_connection> > sockets_;
};

void main()
{
	io_service ios;
	{
		client c(ios);
		server s(ios);
		s.run();
		c.run();
		ios.run();
	}

	system("pause");
}
