//
// fssignal.hpp
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

#ifndef P2ENGINE_FAST_SIGSLOT_HPP
#define P2ENGINE_FAST_SIGSLOT_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "p2engine/push_warning_option.hpp"
#include "p2engine/config.hpp"
#include <assert.h>
#include <boost/function.hpp>
#include <boost/type_traits.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/preprocessor/arithmetic/inc.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/comparison/equal.hpp>
#include <boost/preprocessor/comparison/not_equal.hpp>
#include <boost/preprocessor/facilities/intercept.hpp>
#include <boost/preprocessor/repetition.hpp>
#include <boost/signals2/detail/signals_common_macros.hpp>
#include "p2engine/pop_warning_option.hpp"

#include "p2engine/type_traits.hpp"
#include "p2engine/intrusive_ptr_base.hpp"
#include "p2engine/shared_access.hpp"
#include "p2engine/basic_object.hpp"

#ifdef _MSC_VER
# pragma warning (push)
# pragma warning(default: 4083)
#endif

#define MAX_PARAM 10

namespace p2engine{namespace fssignal{


	template<typename T>
	struct SURE_BIND_THE_TYPE 
	{
		typedef typename boost::add_const<T>::type const_type;
		typedef typename boost::remove_const<T>::type non_const_type;
		typedef typename boost::add_reference<const_type>::type const_reference_type;
		typedef typename boost::add_reference<non_const_type>::type non_const_reference_type;
		operator const_reference_type()const
		{
			return v_;
		}
		operator non_const_reference_type()
		{
			return const_cast<non_const_reference_type>(v_);
		}
		SURE_BIND_THE_TYPE(const_reference_type v)
			:v_(v)
		{
		}
	private:
		T v_;
	};

	namespace detail{

		class connection_base;
		template<typename Signature> class signal_base;
		template<typename Signature> class signal_impl;

		class trackable;
		class connection;
		template<typename Signature> class signal;

	}

	namespace detail{

#ifdef _MSC_VER
#define	SHOULD_BE_STATIC_ASSERT(msg) BOOST_STATIC_ASSERT(0 && msg);
#else
#define	SHOULD_BE_STATIC_ASSERT(msg) BOOST_ASSERT(0 && msg);
#endif

		template <typename T, typename Pointer = void, typename Trackable=void> 
		struct check_param 
		{
			template<typename Conn>
			bool  operator()(T,Conn&)
			{
				return false;
			}
		};

		template <typename T>
		struct check_param<T,
			typename boost::enable_if<boost::is_reference<T> >::type,
			typename boost::disable_if<boost::is_base_and_derived<trackable,typename boost::remove_reference<T>::type> >::type
		>
		{

			template<typename Conn>
			DECLARE_DEPRECATED_FUNCTION("error: Used a reference of object which is NOT DIRIVED form trackable! This might cause crash! Please use raw_ptr of object that DIRIVED form trackable!",
			bool  operator()(T,Conn&)
			{
				SHOULD_BE_STATIC_ASSERT("Used a reference of object which is NOT DIRIVED form trackable! This might cause crash! Please use raw_ptr of object that DIRIVED form trackable!")
					return false;
			}
			) ;
		};

		template <typename T>
		struct check_param<T,
			typename boost::enable_if<boost::is_reference<T> >::type,
			typename boost::enable_if<boost::is_base_and_derived<trackable,typename boost::remove_reference<T>::type> >::type
		>
		{
			template<typename Conn>
			bool  operator()(T t,Conn& c)
			{
				t.track(c);
				return true;
			}
		};

		template <typename T>
		struct check_param<T, 
			typename boost::enable_if<boost::is_pointer<T> >::type,
			typename boost::disable_if<boost::is_base_and_derived<trackable,typename boost::remove_pointer<T>::type> >::type
		>
		{
			template<typename Conn>
			DECLARE_DEPRECATED_FUNCTION("error: Used a raw_ptr of object which is NOT DIRIVED form trackable! This might cause crash! Please use raw_ptr of object that DIRIVED form trackable!",
			bool  operator()(T,Conn&)
			{
				SHOULD_BE_STATIC_ASSERT("Used a raw_ptr of object which is NOT DIRIVED form trackable! This might cause crash! Please use raw_ptr of object that DIRIVED form trackable!")
					return false;
			}
			);

		};

		template <typename T>
		struct check_param<T, 
			typename boost::enable_if<boost::is_pointer<T> >::type,
			typename boost::enable_if<boost::is_base_and_derived<trackable,typename boost::remove_pointer<T>::type> >::type
		>
		{
			template<typename Conn>
			bool  operator()(T t,Conn& c)
			{
				t->track(c);
				return true;
			}
		};

		template <typename T>
		struct check_param<T, 
			typename boost::enable_if<p2engine::detail::is_weak_ptr<T> >::type,
			typename boost::disable_if<boost::is_base_and_derived<trackable,typename T::element_type> >::type
		>
		{
			template<typename Conn>
			DECLARE_DEPRECATED_FUNCTION("error: Used a weak_ptr<> of object which is NOT DIRIVED form trackable! This might cause crash! Please use raw_ptr or weak_ptr<> of object that DIRIVED form trackable!",
			bool  operator()(T t,Conn& c)
			{
				SHOULD_BE_STATIC_ASSERT("Used a weak_ptr<> of object which is NOT DIRIVED form trackable! This might cause crash! Please use raw_ptr or weak_ptr<> of object that DIRIVED form trackable!")
					return false;
			}
			);
		};

		template <typename T>
		struct check_param<T, 
			typename boost::enable_if<p2engine::detail::is_weak_ptr<T> >::type,
			typename boost::enable_if<boost::is_base_and_derived<trackable,typename T::element_type> >::type
		>
		{
			template<typename Conn>
			bool  operator()(T t,Conn& c)
			{
				if (t.lock())
					t.lock()->track(c);
				return true;
			}
		};

		template <typename T>
		struct check_param<T, 
			typename boost::enable_if<p2engine::detail::is_shared_ptr<T> >::type
		>
		{
			template<typename Conn>
			DECLARE_DEPRECATED_FUNCTION("warning: Used a shared_ptr<> of object! This might cause cycle-reference! Please use raw_ptr or weak_ptr<> of object that DIRIVED form trackable!",
			bool  operator()(T,Conn&)
			{
				SHOULD_BE_STATIC_ASSERT("Used a shared_ptr<> of object! This might cause cycle-reference! Please use raw_ptr or weak_ptr<> of object that DIRIVED form trackable!")
					return false;
			}
			);

		};

		template <typename T>
		struct check_param<T, 
			typename boost::enable_if<p2engine::detail::is_auto_ptr<T> >::type
		>
		{
			template<typename Conn>
			DECLARE_DEPRECATED_FUNCTION("warning: Used an auto_ptr<> of object! This might cause cycle-reference! Please use raw_ptr or weak_ptr<> of object that DIRIVED form trackable!",
			bool  operator()(T,Conn&)
			{
				SHOULD_BE_STATIC_ASSERT("Used an auto_ptr<> of object! This might cause cycle-reference! Please use raw_ptr or weak_ptr<> of object that DIRIVED form trackable!")
					return false;
			}
			);

		};

		template <typename T>
		struct check_param<T, 
			typename boost::enable_if<p2engine::detail::is_scoped_ptr<T> >::type
		>
		{
			template<typename Conn>
			DECLARE_DEPRECATED_FUNCTION("warning: Used an scoped_ptr<> of object! This might cause cycle-reference! Please use raw_ptr or weak_ptr<> of object that DIRIVED form trackable!",
			bool  operator()(T,Conn&)
			{
				SHOULD_BE_STATIC_ASSERT("Used an scoped_ptr<> of object! This might cause cycle-reference! Please use raw_ptr or weak_ptr<> of object that DIRIVED form trackable!")
					return false;
			}
			);

		};

		template <typename T>
		struct check_param<T, 
			typename boost::enable_if<p2engine::detail::is_intrusive_ptr<T> >::type
		>
		{
			template<typename Conn>
			DECLARE_DEPRECATED_FUNCTION("warning: Used an intrusive_ptr<> of object! This might cause cycle-reference! Please use raw_ptr or weak_ptr<> of object that DIRIVED form trackable!",
			bool  operator()(T,Conn&)
			{
				SHOULD_BE_STATIC_ASSERT("Used an intrusive_ptr<> of object! This might cause cycle-reference! Please use raw_ptr or weak_ptr<> of object that DIRIVED form trackable!")
					return false;
			}
			);
		};
	}//namespace detail

	namespace detail{

		typedef std::list<boost::intrusive_ptr<connection_base> > connection_list;
		typedef std::list<connection_base*> connection_ptr_list;
		typedef connection_list::iterator connection_iterator;
		typedef connection_ptr_list::iterator connection_ptr_iterator;

		class signal_base_impl;

		class connection_base
			:public object_allocator
			,public basic_intrusive_ptr<connection_base>
		{
		public:
			friend class connection;
			friend class trackable;
			friend class signal_base_impl;
			template<typename Signature> friend class signal_base;

			typedef std::list<std::pair<trackable*,connection_ptr_iterator> > trackable_list;

		public:
			connection_base(signal_base_impl* sig)
				:signal_(sig)
			{
			}
			virtual ~connection_base()
			{
				BOOST_ASSERT(!signal_);
			}

			void disconnect();
			bool connected()const
			{
				return NULL!=signal_;
			}
		private:
			connection_list::iterator& iterator_in_signal()
			{
				return iterator_in_signal_;
			}
			trackable_list::value_type* track(trackable* t);

		protected:
			virtual void make_slot_null()=0;

		protected:
			trackable_list trackables_;
			connection_list::iterator iterator_in_signal_;
			signal_base_impl* signal_;
		};

		template<typename Signature> 
		class slot
			: public connection_base
		{
			friend class connection;
			friend class trackable;
			template<typename SignatureType> friend class signal_base;
			template<typename SignatureType> friend class signal_impl;
			template<typename SignatureType> friend class signal;

			typedef boost::function<Signature> function_type;
		public:
			slot(signal_base_impl* sig)
				:connection_base(sig)
			{
			}

		protected:
			virtual void make_slot_null()
			{
				function_=NULL;
			}
			function_type& func()
			{
				return function_;
			}

		protected:
			function_type function_;
		};

		class connection
		{
			friend class trackable;
			template <typename T,typename Pointer,typename Trackable> friend struct check_param;
			template<typename Signature> friend class signal_base;

		private:
			connection(boost::intrusive_ptr<connection_base>& s)
				:slot_(s)
			{
			}

		public:
			void disconnect()
			{
				if (slot_)
					slot_->disconnect();
			}
			bool connected()const
			{
				return slot_&&slot_->connected();
			}
		protected:
			boost::intrusive_ptr<connection_base> slot_;
		};


		class  trackable{
			friend class connection_base;
			template<typename Signature> friend class slot;
			template <typename T,typename Pointer,typename Trackable> friend struct check_param;
			template<typename Signature> friend class signal_base;

		public:
			trackable() {}
			virtual ~trackable()
			{
				for(;!connections_.empty();)
					(*connections_.begin())->disconnect();
			}

		private:
			void track(const boost::intrusive_ptr<connection_base>& s)
			{
				connection_base::trackable_list::value_type* rst=s->track(this);
				if (rst)
				{
					connections_.push_back(s.get());
					rst->second=--connections_.end();
					BOOST_ASSERT(*rst->second==s.get());
				}
			}

			void disconnect_without_callback_connection(connection_ptr_iterator& itr)
			{
				connections_.erase(itr);
			}

		private://
			trackable(const trackable&);
			const trackable& operator=(const trackable& );

		private:
			connection_ptr_list connections_;
		};

		class signal_base_impl
			:public object_allocator
			,public basic_intrusive_ptr<signal_base_impl> 
		{
			typedef signal_base_impl this_type;
			SHARED_ACCESS_DECLARE;

			template<typename Signature> friend class signal_base;
			template<typename Signature> friend class signal_impl;
			template<typename Signature> friend class signal;

			typedef connection_list::iterator connection_iterator;
		public:
			signal_base_impl():emitting_cnt_(0),need_clear_pending_(false){}
			virtual ~signal_base_impl();
			bool empty()const
			{
				BOOST_FOREACH(connection_list::const_reference conn, m_connections)
				{
					if (conn&&conn->connected())
						return false;
				}
				return true;
			}
			std::size_t size()const
			{
				std::size_t n=0;
				BOOST_FOREACH(connection_list::const_reference conn, m_connections)
				{
					if (conn&&conn->connected())
						++n;
				}
				return n;
			}
			void clear()
			{
				disconnect_all_slots();
			}
			void disconnect_all_slots();
			void pop_front_slot();
			void pop_back_slot();
			void disconnect_without_callback_connection(connection_base* conn);
			void clear_pending_erase();
			void inc_emiting()
			{
				BOOST_ASSERT(emitting_cnt_>=0);
				++emitting_cnt_;
			}
			void dec_emiting()
			{
				--emitting_cnt_;
				BOOST_ASSERT(emitting_cnt_>=0);
			}
			bool is_emiting()const
			{
				BOOST_ASSERT(emitting_cnt_>=0);
				return emitting_cnt_>0;
			}
		protected:
			connection_list m_connections;
			int emitting_cnt_;
			bool need_clear_pending_;
		};

		template<typename Signature>
		class signal_base
		{
		protected:
			typedef Signature signature;
			typedef slot<signature> slot_type;

			friend  class connection_base;
			friend  class trackable;
			template<class SignatureType> friend class slot;

		public:
			signal_base()
			{
			}
			virtual ~signal_base()
			{
			}

#define CHECK_PARAM(z,paramN,Conn) \
	BOOST_PP_IF(paramN,check_param<BOOST_PP_CAT(T, paramN)>()(BOOST_PP_CAT(arg, paramN),Conn),);

#define BIND(z,paramN,_) \
	template< typename FuncPoint BOOST_PP_COMMA_IF(paramN) BOOST_SIGNALS2_ARGS_TEMPLATE_DECL(paramN)>\
	connection bind(FuncPoint fp BOOST_PP_COMMA_IF(paramN) BOOST_SIGNALS2_SIGNATURE_FULL_ARGS(paramN))\
			{\
			creat_signal_base_impl();\
			boost::intrusive_ptr<slot_type>connImpl(new slot_type(elem_.get()));\
			boost::intrusive_ptr<connection_base>connBase(connImpl.get());\
			elem_->m_connections.push_back(connBase);\
			connImpl->iterator_in_signal()=(--elem_->m_connections.end());\
			connImpl->func()=boost::bind(fp BOOST_PP_COMMA_IF(paramN) BOOST_SIGNALS2_SIGNATURE_ARG_NAMES(paramN));\
			BOOST_PP_REPEAT(BOOST_PP_INC(paramN),CHECK_PARAM,connImpl);\
			return connection(connBase);\
			}

			BOOST_PP_REPEAT(BOOST_PP_INC(MAX_PARAM),BIND,_)

#undef BIND
#undef CHECK_PARAM


		public:
			void swap(signal_base&rhs)
			{
				elem_.swap(rhs.elem_);
			}
			bool empty()const
			{
				return !elem_||elem_->empty();
			}
			std::size_t size()const
			{
				return elem_?elem_->size():0;
			}
			void clear()
			{
				disconnect_all_slots();
			}
			void disconnect_all_slots()
			{
				if (elem_)
					elem_->disconnect_all_slots();
			}
			void pop_front_slot()
			{
				BOOST_ASSERT(elem_);
				elem_->pop_front_slot();
			}
			void pop_back_slot()
			{
				BOOST_ASSERT(elem_);
				elem_->pop_back_slot();
			}
		protected:
			void disconnect_without_callback_connection(slot_type* conn)
			{
				if (elem_)
					elem_->disconnect_without_callback_connection(conn);
			}
			void creat_signal_base_impl()
			{
				if (!elem_)
					elem_.reset(new signal_base_impl);
			}
		protected:
			boost::intrusive_ptr<signal_base_impl> elem_;
		};

#define SIGNAL_IMPL(z,paramN,_) \
	template< BOOST_SIGNALS2_SIGNATURE_TEMPLATE_DECL(paramN) >\
		class signal_impl< BOOST_SIGNALS2_SIGNATURE_FUNCTION_TYPE(paramN) >\
		:public signal_base< BOOST_SIGNALS2_SIGNATURE_FUNCTION_TYPE(paramN) >\
		{\
		typedef slot<BOOST_SIGNALS2_SIGNATURE_FUNCTION_TYPE(paramN)> slot_type;\
		\
		\
		R _emit(BOOST_SIGNALS2_SIGNATURE_FULL_ARGS(paramN) BOOST_PP_COMMA_IF(paramN) bool front=false)\
		{\
		if(!this->elem_) return R();\
		boost::intrusive_ptr<signal_base_impl> protector(this->elem_);\
		protector->inc_emiting();\
		R v;\
		connection_iterator itr(protector->m_connections.begin());\
		for (;itr!=protector->m_connections.end();++itr)\
		{\
		if ((*itr)&&(*itr)->connected())\
		{\
		BOOST_ASSERT(dynamic_cast<slot_type*>((*itr).get()));\
		v=((slot_type*)((*itr).get()))->func()(BOOST_SIGNALS2_SIGNATURE_ARG_NAMES(paramN));\
		if(front) break;\
		}\
		}\
		protector->dec_emiting();\
		if(!protector->is_emiting())\
		protector->clear_pending_erase();\
		return v;\
		}\
		\
		public:\
		R operator()(BOOST_SIGNALS2_SIGNATURE_FULL_ARGS(paramN))\
		{\
		return _emit(BOOST_SIGNALS2_SIGNATURE_ARG_NAMES(paramN));\
		}\
		R emit(BOOST_SIGNALS2_SIGNATURE_FULL_ARGS(paramN))\
		{\
		return _emit(BOOST_SIGNALS2_SIGNATURE_ARG_NAMES(paramN));\
		}\
		R emit_front(BOOST_SIGNALS2_SIGNATURE_FULL_ARGS(paramN))\
		{\
		return _emit(BOOST_SIGNALS2_SIGNATURE_ARG_NAMES(paramN) BOOST_PP_COMMA_IF(paramN) true);\
		}\
		};


#define SIGNAL_IMPL_VOID_RETURN(z,paramN,_) \
	template< BOOST_SIGNALS2_ARGS_TEMPLATE_DECL(paramN) >\
		class signal_impl< void(BOOST_SIGNALS2_ARGS_TEMPLATE_INSTANTIATION(paramN)) >\
		:public signal_base< void(BOOST_SIGNALS2_ARGS_TEMPLATE_INSTANTIATION(paramN)) >\
		{\
		typedef slot<void(BOOST_SIGNALS2_ARGS_TEMPLATE_INSTANTIATION(paramN))> slot_type;\
		\
		\
		void _emit(BOOST_SIGNALS2_SIGNATURE_FULL_ARGS(paramN) BOOST_PP_COMMA_IF(paramN) bool front=false)\
		{\
		if(!this->elem_) return;\
		BOOST_ASSERT(this->elem_->emitting_cnt_<10);\
		boost::intrusive_ptr<signal_base_impl> protector(this->elem_);\
		protector->inc_emiting();\
		connection_iterator itr(protector->m_connections.begin());\
		for (;itr!=protector->m_connections.end();++itr)\
		{\
		if ((*itr)&&(*itr)->connected())\
		{\
		BOOST_ASSERT(dynamic_cast<slot_type*>((*itr).get()));\
		((slot_type*)((*itr).get()))->func()(BOOST_SIGNALS2_SIGNATURE_ARG_NAMES(paramN));\
		if(front) break;\
		}\
		}\
		protector->dec_emiting();\
		if(!protector->is_emiting())\
		protector->clear_pending_erase();\
		}\
		\
		public:\
		void operator()(BOOST_SIGNALS2_SIGNATURE_FULL_ARGS(paramN))\
		{\
		_emit(BOOST_SIGNALS2_SIGNATURE_ARG_NAMES(paramN));\
		}\
		void emit(BOOST_SIGNALS2_SIGNATURE_FULL_ARGS(paramN))\
		{\
		_emit(BOOST_SIGNALS2_SIGNATURE_ARG_NAMES(paramN));\
		}\
		void emit_front(BOOST_SIGNALS2_SIGNATURE_FULL_ARGS(paramN))\
		{\
		_emit(BOOST_SIGNALS2_SIGNATURE_ARG_NAMES(paramN) BOOST_PP_COMMA_IF(paramN) true);\
		}\
		};

		BOOST_PP_REPEAT(BOOST_PP_INC(MAX_PARAM),SIGNAL_IMPL,_)
			BOOST_PP_REPEAT(BOOST_PP_INC(MAX_PARAM),SIGNAL_IMPL_VOID_RETURN,_)
#undef SIGNAL_IMPL

			template<typename Signature> 
		class signal:public signal_impl<Signature>
		{
		public:
			signal(){}

#define BIND(z,paramN,_) \
	template< typename FuncPoint BOOST_PP_COMMA_IF(paramN) BOOST_SIGNALS2_ARGS_TEMPLATE_DECL(paramN)>\
	signal(FuncPoint fp BOOST_PP_COMMA_IF(paramN) BOOST_SIGNALS2_SIGNATURE_FULL_ARGS(paramN))\
			{\
			this->bind(fp BOOST_PP_COMMA_IF(paramN) BOOST_SIGNALS2_SIGNATURE_ARG_NAMES(paramN));\
			}

			BOOST_PP_REPEAT(BOOST_PP_INC(MAX_PARAM),BIND,_)
#undef BIND

		};
	}//namespace detail

	typedef detail::connection connection;
	typedef detail::trackable  trackable;
	using   detail::signal;
}}

#undef MAX_PARAM

#if defined(_MSC_VER)
# pragma warning (pop)
#endif // defined(_MSC_VER)

#endif//

