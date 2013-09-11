#ifndef p2engine_ssl_stream_wrapper_h__
#define p2engine_ssl_stream_wrapper_h__
#include "p2engine/push_warning_option.hpp"
#include "p2engine/config.hpp"
#ifdef P2ENGINE_ENABLE_SSL
#include <boost/asio/ssl.hpp>
#endif
#include "p2engine/pop_warning_option.hpp"

namespace p2engine{

	template<typename Socket>
	class ssl_stream_wrapper{
		typedef variant_endpoint endpoint;

		typedef Socket	 socket_type;
		typedef typename socket_type::lowest_layer_type	lowest_layer_type;

#ifdef P2ENGINE_ENABLE_SSL
		typedef asio::ssl::verify_context	verify_context;
		typedef asio::ssl::stream<Socket>	ssl_socket_type;
		typedef boost::function<void(const error_code&)> handler_type;
#else
		typedef struct{}verify_context;
#endif
		typedef ssl_stream_wrapper<Socket>	this_type;
	public:
		ssl_stream_wrapper(io_service& ios, bool enableSsl=false)
			:enable_ssl_(enableSsl)
		{
#ifdef P2ENGINE_ENABLE_SSL
			if (enableSsl)
			{
				boost::asio::ssl::context ctx(boost::asio::ssl::context::sslv23);
				new(&mem_[0]) ssl_socket_type(ios, ctx);
            }
			else 
#endif
			{
				new(&mem_[0]) socket_type(ios);
			}
		}

		virtual ~ssl_stream_wrapper()
		{
#ifdef P2ENGINE_ENABLE_SSL
			if (enable_ssl_)
			{
				ssl_socket_type& sock=*reinterpret_cast<ssl_socket_type*>(&mem_[0]);
				sock.~ssl_socket_type();
			}
			else
#endif
			{
				socket_type& sock=*reinterpret_cast<socket_type*>(&mem_[0]);
				sock.~socket_type();
			}
		}

		template<typename EndPoint>
		void open(const EndPoint& edp, error_code& ec)
		{
#ifdef P2ENGINE_ENABLE_SSL
			if (enable_ssl_)
			{
				ssl_socket_type& sock=*reinterpret_cast<ssl_socket_type*>(&mem_[0]);
				sock.next_layer().open(edp, ec);
			}
			else
#endif
			{
				Socket& sock=*reinterpret_cast<socket_type*>(&mem_[0]);
				sock.open(edp, ec);
			}
		}

		template<typename Handler>
		void async_connect(const endpoint& edp, const Handler& h)
		{
#ifdef P2ENGINE_ENABLE_SSL
			if (enable_ssl_)
			{
				ssl_socket_type& sock=*reinterpret_cast<ssl_socket_type*>(&mem_[0]);
				//sock.set_verify_mode(boost::asio::ssl::verify_peer);
				error_code ec;
				sock.set_verify_callback(
					boost::bind(&this_type::verify_certificate, this, _1, _2), ec);

				boost::shared_ptr<handler_type> handler(new handler_type(h));
				sock.next_layer().async_connect(edp, 
					boost::bind(&this_type::connected, this, _1, handler));
			}
			else
#endif
			{
				Socket& sock=*reinterpret_cast<socket_type*>(&mem_[0]);
				sock.async_connect(edp,h);
			}
		}

		template<typename BufferType, typename ObjType>
		void async_write_some(BufferType buf, ObjType obj)
		{
#ifdef P2ENGINE_ENABLE_SSL
			if (enable_ssl_)
			{
				ssl_socket_type& sock=*reinterpret_cast<ssl_socket_type*>(&mem_[0]);
				sock.async_write_some(buf,obj);

			}
			else
#endif
			{
				Socket& sock=*reinterpret_cast<socket_type*>(&mem_[0]);
				sock.async_write_some(buf,obj);
			}
		}

		template<typename BufferType, typename ObjType>
		void async_read_some(BufferType buf, ObjType obj)
		{
#ifdef P2ENGINE_ENABLE_SSL
			if (enable_ssl_)
			{
				ssl_socket_type& sock=*reinterpret_cast<ssl_socket_type*>(&mem_[0]);
				sock.async_read_some(buf,obj);

			}
			else
#endif
			{
				Socket& sock=*reinterpret_cast<socket_type*>(&mem_[0]);
				sock.async_read_some(buf,obj);
			}
		}

		bool is_open() const
		{
#ifdef P2ENGINE_ENABLE_SSL
			if (enable_ssl_)
			{
				ssl_socket_type& sock=*(ssl_socket_type*)(&mem_[0]);
				return sock.next_layer().is_open();
			}
			else
#endif
			{
				Socket& sock=*(Socket*)(&mem_[0]);
				return sock.is_open();
			}
		}

		endpoint local_endpoint(error_code& ec) const
		{
#ifdef P2ENGINE_ENABLE_SSL
			if (enable_ssl_)
			{
				ssl_socket_type& sock=*(ssl_socket_type*)(&mem_[0]);
				return sock.next_layer().local_endpoint(ec);
			}
			else
#endif
			{
				Socket& sock=*(Socket*)(&mem_[0]);
				return sock.local_endpoint(ec);
			}
		}

		endpoint remote_endpoint(error_code& ec) const 
		{
#ifdef P2ENGINE_ENABLE_SSL
			if (enable_ssl_)
			{
				ssl_socket_type& sock=*(ssl_socket_type*)(&mem_[0]);
				return sock.next_layer().remote_endpoint(ec);
			}
			else
#endif
			{
				Socket& sock=*(Socket*)(&mem_[0]);
				return sock.remote_endpoint(ec);
			}
		}

		lowest_layer_type& lowest_layer()
		{
#ifdef P2ENGINE_ENABLE_SSL
			if (enable_ssl_)
			{
			    ssl_socket_type& sock=*reinterpret_cast<ssl_socket_type*>(&mem_[0]);
				return sock.lowest_layer();
			}
			else
#endif
			{
				Socket& sock=*reinterpret_cast<socket_type*>(&mem_[0]);
				return sock.lowest_layer();
			}
		}

		void shutdown(boost::asio::socket_base::shutdown_type what, error_code& ec)
		{
#ifdef P2ENGINE_ENABLE_SSL
			if (enable_ssl_)
			{
				ssl_socket_type& sock=*reinterpret_cast<ssl_socket_type*>(&mem_[0]);
				sock.shutdown(ec);
			}
			else
#endif
			{
				Socket& sock=*reinterpret_cast<socket_type*>(&mem_[0]);
				sock.shutdown(what,ec);
			}
		}

		template <typename Handler>
		void async_shutdown(boost::asio::socket_base::shutdown_type what,
			Handler handler=boost::bind(&this_type::dymmy,_1))
		{
#ifdef P2ENGINE_ENABLE_SSL
			if (enable_ssl_)
			{
				ssl_socket_type& sock=*reinterpret_cast<ssl_socket_type*>(&mem_[0]);
				sock.async_shutdown(handler);
			}
			else
#endif
			{
				Socket& sock=*reinterpret_cast<socket_type*>(&mem_[0]);
				io_service& ios=sock.get_io_service();
				error_code ec;
				sock.shutdown(what,ec);
				ios.post(boost::bind(handler,ec));
			}
		}

		void close(error_code& ec)
		{
#ifdef P2ENGINE_ENABLE_SSL
			if (enable_ssl_)
			{
				ssl_socket_type& sock=*reinterpret_cast<ssl_socket_type*>(&mem_[0]);
				sock.shutdown(ec);
			}
			else
#endif
			{
				Socket& sock=*reinterpret_cast<socket_type*>(&mem_[0]);
				sock.close(ec);
			}
		}

		template <typename Handler>
		void async_close( Handler handler=boost::bind(&this_type::dymmy,_1))
		{
#ifdef P2ENGINE_ENABLE_SSL
			if (enable_ssl_)
			{
				ssl_socket_type& sock=*reinterpret_cast<ssl_socket_type*>(&mem_[0]);
				sock.async_shutdown(handler);
			}
			else
#endif
			{
				Socket& sock=*reinterpret_cast<socket_type*>(&mem_[0]);
				io_service& ios=sock.get_io_service();
				error_code ec;
				sock.shutdown(asio::socket_base::shutdown_both,ec);
				sock.close(ec);
				ios.post(boost::bind(handler,ec));
			}
		}

		template<typename EndPoint>
		void bind(const EndPoint& edp, error_code& ec)
		{
#ifdef P2ENGINE_ENABLE_SSL
			if (enable_ssl_)
			{
				ssl_socket_type& sock=*reinterpret_cast<ssl_socket_type*>(&mem_[0]);
				sock.next_layer().bind(asio::ip::tcp::endpoint(edp), ec);
			}
			else
#endif
			{
				Socket& sock=*reinterpret_cast<socket_type*>(&mem_[0]);
				sock.bind(asio::ip::tcp::endpoint(edp), ec);
			}
		}

		template<typename OptionType>
		void set_option(const OptionType& SettableSocketOption, error_code& ec)
		{
#ifdef P2ENGINE_ENABLE_SSL
			if (enable_ssl_)
			{
				ssl_socket_type& sock=*reinterpret_cast<ssl_socket_type*>(&mem_[0]);
				sock.next_layer().set_option(SettableSocketOption, ec);
			}
			else
#endif
			{
				Socket& sock=*reinterpret_cast<socket_type*>(&mem_[0]);
				sock.set_option(SettableSocketOption, ec);
			}
		}
	private:

#ifdef P2ENGINE_ENABLE_SSL
		bool verify_certificate(bool preverified,verify_context& ctx)
		{
			DEBUG_SCOPE(
				char subject_name[256];
			X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
			X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
			std::cout << "Verifying " << subject_name << "\n";
			return true;
			);
			return preverified;
		}

		void connected(error_code const& e, boost::shared_ptr<handler_type> h)
		{
			if (enable_ssl_)
			{
				if (e)
				{
					(*h)(e);
					return;
				}

				ssl_socket_type& sock=*reinterpret_cast<ssl_socket_type*>(&mem_[0]);
				sock.async_handshake(asio::ssl::stream_base::client
					, boost::bind(&this_type::handshake, this, _1, h));
			}
		}

		void handshake(error_code const& e, boost::shared_ptr<handler_type> h)
		{
			(*h)(e);
		}
#else
		bool verify_certificate(bool,verify_context&)
		{
			return true;
		}
#endif
			

	private:
		static void dymmy(const error_code& ec)
		{
		}
#ifdef P2ENGINE_ENABLE_SSL
		template<size_t v1,size_t v2>
		struct extractor
		{
			enum{max=v1>v2?v1:v2};
		};
		enum{
			socket_size=(extractor<sizeof(ssl_socket_type),sizeof(Socket)>::max+8-1)/8*8
		};
#else
		enum{
			socket_size=(sizeof(Socket)+8-1)/8*8
		};
#endif
		char mem_[socket_size];
		bool enable_ssl_;
	};
};
#endif // p2engine_ssl_stream_wrapper_h__
