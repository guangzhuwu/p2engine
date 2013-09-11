#ifndef SAFE_ASIO_BASE_HPP
#define SAFE_ASIO_BASE_HPP

#include "p2engine/config.hpp"
#include "p2engine/atomic.hpp"
#include "p2engine/wrappable_integer.hpp"

namespace p2engine{

	class operation_mark{
	protected:
		operation_mark()
			:op_stamp_(0),op_cancel_(0)
		{
		}
		virtual ~operation_mark()
		{
		}

	public:
		typedef int32_t op_stamp_t;

		op_stamp_t next_op_stamp()
		{
			return ++op_stamp_;
		}
		op_stamp_t op_stamp()const
		{
			return op_stamp_;
		}
		op_stamp_t op_cancel()const
		{
			return op_cancel_;
		}

		void set_cancel()
		{
			op_cancel_=(op_stamp_t)op_stamp_;
		}

		bool is_canceled_op(op_stamp_t stamp)const
		{
			return wrappable_less_equal<op_stamp_t>()(stamp,(op_stamp_t)op_cancel_);
		}

		template<typename Handler>
		void dispatch_handler(Handler handler,op_stamp_t stamp)const
		{
			if (!is_canceled_op(stamp))
				handler();
		}

	private:
		atomic<op_stamp_t> op_stamp_, op_cancel_;
	};

	template<typename Handler,typename Object>
	void operation_mark_post(const Object& obj,const Handler& handler,
		operation_mark::op_stamp_t* stamp=NULL)
	{
		typedef BOOST_TYPEOF(boost::protect(handler)) handler_type;
		obj->get_io_service().post(make_alloc_handler(
			boost::bind(&operation_mark::dispatch_handler<handler_type>,obj,
			boost::protect(handler),(stamp?(*stamp):obj->op_stamp()))
			));
	}

	template<typename Handler,typename Object>
	void operation_mark_dispatch(const Object& obj,const Handler& handler,
		operation_mark::op_stamp_t* stamp=NULL)
	{
		typedef BOOST_TYPEOF(boost::protect(handler)) handler_type;

		obj->get_io_service().dispatch(make_alloc_handler(
			boost::bind(&operation_mark::dispatch_handler<handler_type>,obj,
			boost::protect(handler),(stamp?(*stamp):obj->op_stamp()))
			));
	}

}

#endif//SAFE_SOCKET_BASE_HPP



