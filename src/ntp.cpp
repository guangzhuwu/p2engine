#include "p2engine/ntp.hpp"
#include "p2engine/socket_utility.hpp"
#include "p2engine/io.hpp"

namespace p2engine{

	ntp_client::ntp_client(io_service& ios,const std::string& ntpServer)
		:basic_engine_object(ios)
		,resolver_(ios)
		,socket_(ios)
		,ntp_server_host_(ntpServer)
	{
		if (!timer_)
		{
			timer_=rough_timer::create(get_io_service());
			timer_->set_obj_desc("ntp_client::timer_");
			timer_->time_signal().bind(&this_type::__time_out,this);
		}
	}

	void ntp_client::__time_out()
	{
		resolver_.cancel();
		if (socket_.is_open())
			socket_.close();
		signal_(ptime(),boost::system::error_code(boost::asio::error::timed_out));
	}

	void ntp_client::__gettime(const error_code& ec,  
		udp::resolver::iterator iterator,
		coroutine coro,
		tick_type requestTime
		)
	{
		CORO_REENTER(coro)
		{
			timer_->cancel();
			timer_->async_wait(seconds(1));

			if (!socket_.is_open())
			{
				CORO_YIELD(
					resolver_.async_resolve(
					udp::resolver::query(udp::v4(), ntp_server_host_, "ntp"),
					boost::bind(&this_type::__gettime,SHARED_OBJ_FROM_THIS,_1,_2,coro,-1)
					);	
				);
				if (ec)
				{
					timer_->cancel();
					signal_(ptime(),ec);
					return;
				}
				memset(buf_,0,sizeof(buf_));
				buf_[0]=(char)(0x1B);
				{
					error_code e;
					socket_.open(udp::v4(),e);
					socket_.connect(*iterator,e);
					disable_icmp_unreachable(socket_.native());
				}
			}
			socket_.send(boost::asio::buffer(buf_,48));
			CORO_YIELD(
				socket_.async_receive(boost::asio::buffer(buf_,sizeof(buf_)),
				boost::bind(&this_type::__gettime,SHARED_OBJ_FROM_THIS,_1,iterator,coro,
				system_time::precise_tick_count())
				);
			);

			{
				error_code e;
				socket_.close(e);
			}

			if (ec)
			{
				timer_->cancel();
				signal_(ptime(),ec);
				return;
			}

			struct parse 
			{
				int64_t operator()(const char* p)
				{
					const int64_t iPart=read_uint32_ntoh(p);
					const int64_t fPart=read_uint32_ntoh(p);
					return (int64_t(iPart * 1000LL+ fPart * 1000LL / 0x100000000LL ));
				}
			};

			tick_type t1=requestTime;
			tick_type t4=system_time::precise_tick_count();
			tick_type t2= parse()(buf_+32);
			tick_type t3= parse()(buf_+40);
			tick_type d = (t4 - t1) - (t3 - t2); // roundtrip delay
			tick_type ticks = (t3 + (d >> 1));

			using namespace boost::posix_time;
			ptime pt(boost::gregorian::date(1900,1,1),milliseconds(ticks));

			timer_->cancel();
			signal_(pt,error_code());
		}
	}
}