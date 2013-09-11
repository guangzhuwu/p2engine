//
// keeper.hpp
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

#ifndef id_keeper_h__
#define id_keeper_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "p2engine/push_warning_option.hpp"
#include "p2engine/config.hpp"
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/unordered_set.hpp>
#include <boost/iterator.hpp>

#include "p2engine/time.hpp"
#include "p2engine/fssignal.hpp"

namespace p2engine
{
	namespace multi_index=boost::multi_index;

	template<class _Key, class _Pr = std::less<_Key> >
	class timed_keeper_set
	{
	public:
		typedef _Key	key_type;
		typedef _Pr		key_compare;

	private:
		typedef timed_keeper_set<key_type,key_compare> this_type;

		struct eliment{
			key_type data;
			tick_type outTime;
			tick_type keepTime;
		};

		typedef multi_index::multi_index_container<
			eliment,
			multi_index::indexed_by<
			multi_index::ordered_unique<multi_index::member<eliment,key_type,&eliment::data>,key_compare>,
			multi_index::ordered_non_unique<multi_index::member<eliment,tick_type,&eliment::outTime> >
			> 
		> eliment_set;

		typedef typename multi_index::nth_index_iterator<eliment_set,0>::type index_iterator;
		typedef typename multi_index::nth_index_const_iterator<eliment_set,0>::type index_const_iterator;
	public:

		typedef typename multi_index::nth_index<eliment_set,0>::type id_index_type;
		typedef typename multi_index::nth_index<eliment_set,1>::type outtime_index_type;

		class iterator
			: public boost::iterator_facade<
			iterator,
			key_type,
			boost::bidirectional_traversal_tag
			>
		{
			typedef typename this_type::index_iterator index_iterator;
		public:
			typedef key_type value_type;
			typedef value_type& reference;
			typedef value_type* pointer;

		public:
			iterator(){}

		protected:
			iterator(const index_iterator& itr)
				: index_iterator_(itr) {}

		private:
			template<class _KeyType, class _PrType >friend class timed_keeper_set;
			friend class boost::iterator_core_access;

			void increment() {++index_iterator_; }
			void decrement() {--index_iterator_;}

			bool equal(iterator const& other) const
			{
				return this->index_iterator_ == other.index_iterator_;
			}

			reference dereference() { return index_iterator_->data; }
			const reference dereference() const{ return (const reference)index_iterator_->data; }

			index_iterator index_iterator_;
		};

		class const_iterator
			: public boost::iterator_facade<
			iterator,
			key_type,
			boost::bidirectional_traversal_tag
			>
		{
			typedef typename this_type::index_const_iterator index_const_iterator;
		public:
			typedef key_type value_type;
			typedef value_type& reference;
			typedef value_type* pointer;

		public:
			const_iterator(){}

		protected:
			const_iterator(const index_const_iterator& itr)
				: index_iterator_(itr) {}

		private:
			friend class boost::iterator_core_access;
			template<class _KeyType, class _PrType >friend class timed_keeper_set;

			void increment() {++index_iterator_; }
			void decrement() {--index_iterator_;}

			bool equal(iterator const& other) const
			{
				return this->index_iterator_ == other.index_iterator_;
			}

			const reference dereference() const{ return (const reference)index_iterator_->data; }

			index_const_iterator index_iterator_;
		};

		bool try_keep(const key_type& id, const time_duration& t)
		{
			tick_type now=tick_now();
			clear_timeout(now);
			id_index_type& id_index=multi_index::get<0>(elimentSet_);
			if (id_index.find(id)!=id_index.end())
				return false;
			eliment elm;
			elm.data=id;
			elm.outTime=now+t.total_milliseconds();
			elm.keepTime=now;
			id_index.insert(elm);
			return true;
		}
		bool is_keeped(const key_type& id)
		{
			clear_timeout();
			id_index_type& id_index=multi_index::get<0>(elimentSet_);
			return id_index.find(id)!=id_index.end();
		}

		time_duration expires_time(const key_type& id)
		{
			iterator itr=find(id);
			if (itr!=end())
				return  millisec(tick_now()-itr.index_iterator_->keepTime);
			return boost::posix_time::neg_infin;
		}

		time_duration remain_time(const key_type& id)
		{
			iterator itr=find(id);
			if (itr!=end())
				return  millisec(itr.index_iterator_->outTime-tick_now());
			return boost::posix_time::neg_infin;
		}

		time_duration expires_time(const iterator& itr)
		{
			if (itr!=end())
				return  millisec(tick_now()-itr.index_iterator_->keepTime);
			return boost::posix_time::neg_infin;
		}

		time_duration remain_time(const iterator& itr)
		{
			if (itr!=end())
				return  millisec(itr.index_iterator_->outTime-tick_now());
			return boost::posix_time::neg_infin;
		}

		time_duration max_remain_time()
		{
			outtime_index_type& t_index=multi_index::get<1>(elimentSet_);
			BOOST_AUTO(itr,t_index.rbegin());
			if (itr!=t_index.rend())
				return millisec((*itr).outTime-tick_now());
			return millisec(0);
		}

		time_duration min_remain_time()
		{
			outtime_index_type& t_index=multi_index::get<1>(elimentSet_);
			BOOST_AUTO(itr,t_index.begin());
			if (itr!=t_index.end())
				return millisec((*itr).outTime-tick_now());
			return millisec(0);
		}

		void erase(const key_type& id)
		{
			clear_timeout();
			id_index_type& id_index=multi_index::get<0>(elimentSet_);
			id_index.erase(id);
		}

		void erase(const iterator& itr)
		{
			clear_timeout();
			id_index_type& id_index=multi_index::get<0>(elimentSet_);
			id_index.erase(itr.index_iterator_);
		}

		void erase(const_iterator itr)
		{
			clear_timeout();
			id_index_type& id_index=multi_index::get<0>(elimentSet_);
			id_index.erase(itr.index_iterator_);
		}

		iterator find(const key_type& id) 
		{
			clear_timeout();
			id_index_type& id_index=multi_index::get<0>(elimentSet_);
			return id_index.find(id);
		}
		const_iterator find(const key_type& id) const
		{
			clear_timeout();
			id_index_type& id_index=multi_index::get<0>(elimentSet_);
			return id_index.find(id);
		}
		const_iterator begin()const
		{
			id_index_type& id_index=multi_index::get<0>(elimentSet_);
			return id_index.begin();
		}
		const_iterator end()const
		{
			id_index_type& id_index=multi_index::get<0>(elimentSet_);
			return id_index.end();
		}
		iterator begin()
		{
			id_index_type& id_index=multi_index::get<0>(elimentSet_);
			return id_index.begin();
		}
		iterator end()
		{
			id_index_type& id_index=multi_index::get<0>(elimentSet_);
			return id_index.end();
		}
		std::size_t size_befor_clear()
		{
			id_index_type& id_index=multi_index::get<0>(elimentSet_);
			return id_index.size();
		}
		std::size_t size()
		{
			clear_timeout();
			id_index_type& id_index=multi_index::get<0>(elimentSet_);
			return id_index.size();
		}
		void clear()
		{
			elimentSet_.clear();
		}
		void clear_timeout()
		{
			clear_timeout(tick_now());
		}
		void clear_timeout_before(const iterator& itr)
		{
			clear_timeout(tick_now()+remain_time(itr).total_milliseconds());
		}

		typedef fssignal::signal<void(const _Key&)> expires_signal_type;
		expires_signal_type& expires_signal()
		{
			return expires_signal_;
		}

	protected:
		expires_signal_type expires_signal_;
		eliment_set elimentSet_;
		tick_type tick_now()
		{
			return system_time::tick_count();
		}
		void clear_timeout(tick_type now)
		{
			typedef typename multi_index::nth_index_iterator<eliment_set,1>::type outtime_iterator;
			outtime_index_type& t_index=multi_index::get<1>(elimentSet_);
			outtime_iterator itr=t_index.begin();
			for (;itr!=t_index.end();)
			{
				if ((*itr).outTime<=now)
				{
					expires_signal_(itr->data);
					itr=t_index.erase(itr);
				}
				else
				{
					break;
				}
			}
		}
	};

	template<class _Key,class _Value, class _Pr = std::less<_Key> >
	class timed_keeper_map
	{
	public:
		typedef _Key	key_type;
		typedef _Value  mapped_type;
		typedef _Value  referent_type;
		typedef _Pr		key_compare;

		typedef std::pair<key_type,mapped_type> pair_type;

	private:
		typedef timed_keeper_map<key_type,mapped_type,key_compare> this_type;

		struct eliment{
			pair_type data;
			tick_type outTime;
			tick_type keepTime;
		};

		struct eliment_identity
		{
			typedef key_type result_type;
			const result_type& operator()(const eliment& x)const
			{
				return x.data.first;
			}
			result_type& operator()(eliment& x)const
			{
				return x.data.first;
			}
		};

		typedef multi_index::multi_index_container<
			eliment,
			multi_index::indexed_by<
			multi_index::ordered_unique<eliment_identity,key_compare>,
			multi_index::ordered_non_unique<multi_index::member<eliment,tick_type,&eliment::outTime> >
			> 
		> eliment_set;
		typedef typename multi_index::nth_index_iterator<eliment_set,0>::type index_iterator;
		typedef typename multi_index::nth_index_const_iterator<eliment_set,0>::type index_const_iterator;

	public:
		typedef typename multi_index::nth_index<eliment_set,0>::type id_index_type;
		typedef typename multi_index::nth_index<eliment_set,1>::type outtime_index_type;

		class iterator
			: public boost::iterator_facade<
			iterator, 
			std::pair<key_type,mapped_type>, 
			boost::bidirectional_traversal_tag
			>
		{
			typedef typename this_type::index_iterator index_iterator;
		public:
			typedef std::pair<key_type,mapped_type> value_type;
			typedef value_type& reference;
			typedef value_type* pointer;

		public:
			iterator(){}

		protected:
			iterator(const index_iterator& itr)
				: index_iterator_(itr) {}

		private:
			template<class _KeyType,class _ValueType, class _PrType >friend class  timed_keeper_map;
			friend class boost::iterator_core_access;

			void increment() {++index_iterator_; }
			void decrement() {--index_iterator_;}

			bool equal(iterator const& other) const
			{
				return this->index_iterator_ == other.index_iterator_;
			}

			reference dereference() { return index_iterator_->data; }
			const reference dereference() const{ return (const reference)index_iterator_->data; }

			index_iterator index_iterator_;
		};

		class const_iterator
			: public boost::iterator_facade<
			iterator
			, std::pair<key_type,mapped_type>
			, boost::bidirectional_traversal_tag
			>
		{
			typedef typename this_type::index_const_iterator index_const_iterator;
		public:
			typedef std::pair<key_type,mapped_type> value_type;
			typedef value_type& reference;
			typedef value_type* pointer;

		public:
			const_iterator(){}

		protected:
			const_iterator(const index_const_iterator& itr)
				: index_iterator_(itr) {}

		private:
			template<class _KeyType,class _ValueType, class _PrType >friend class  timed_keeper_map;
			friend class boost::iterator_core_access;

			void increment() {++index_iterator_; }
			void decrement() {--index_iterator_;}

			bool equal(iterator const& other) const
			{
				return this->index_iterator_ == other.index_iterator_;
			}

			const reference dereference() const{ return (const reference)index_iterator_->data; }

			index_const_iterator index_iterator_;
		};

		bool try_keep(const pair_type& id, const time_duration& t)
		{
			tick_type now=tick_now();
			clear_timeout(now);
			id_index_type& id_index=multi_index::get<0>(elimentSet_);
			if (id_index.find(id.first)!=id_index.end())
			{
				return false;
			}
			eliment elm;
			elm.data=id;
			elm.outTime=now+t.total_milliseconds();
			elm.keepTime=now;
			id_index.insert(elm);
			return true;
		}
		bool is_keeped(const key_type& id)
		{
			clear_timeout();
			id_index_type& id_index=multi_index::get<0>(elimentSet_);
			return id_index.find(id)!=id_index.end();
		}

		time_duration expires_time(const key_type& id)
		{
			iterator itr=find(id);
			if (itr!=end())
				return  millisec(tick_now()-itr.index_iterator_->keepTime);
			return boost::posix_time::neg_infin;
		}

		time_duration remain_time(const key_type& id)
		{
			iterator itr=find(id);
			if (itr!=end())
			{
				return  millisec(itr.index_iterator_->outTime-tick_now());
			}
			return boost::posix_time::neg_infin;
		}

		void erase(const key_type& id)
		{
			clear_timeout();
			id_index_type& id_index=multi_index::get<0>(elimentSet_);
			id_index.erase(id);
		}
		void erase(iterator itr)
		{
			clear_timeout();
			id_index_type& id_index=multi_index::get<0>(elimentSet_);
			id_index.erase(itr.index_iterator_);
		}
		iterator find(const key_type& id) 
		{
			clear_timeout();
			id_index_type& id_index=multi_index::get<0>(elimentSet_);
			return iterator(id_index.find(id));
		}
		const_iterator find(const key_type& id) const
		{
			clear_timeout();
			id_index_type& id_index=multi_index::get<0>(elimentSet_);
			return id_index.find(id);
		}
		const_iterator begin()const
		{
			id_index_type& id_index=multi_index::get<0>(elimentSet_);
			return id_index.begin();
		}
		const_iterator end()const
		{
			id_index_type& id_index=multi_index::get<0>(elimentSet_);
			return id_index.end();
		}
		iterator begin()
		{
			id_index_type& id_index=multi_index::get<0>(elimentSet_);
			return id_index.begin();
		}
		iterator end()
		{
			id_index_type& id_index=multi_index::get<0>(elimentSet_);
			return id_index.end();
		}
		std::size_t size_befor_clear()
		{
			id_index_type& id_index=multi_index::get<0>(elimentSet_);
			return id_index.size();
		}
		std::size_t size()
		{
			clear_timeout();
			id_index_type& id_index=multi_index::get<0>(elimentSet_);
			return id_index.size();
		}
		void clear()
		{
			elimentSet_.clear();
		}
		void clear_timeout()
		{
			clear_timeout(tick_now());
		}

		typedef fssignal::signal<void(const std::pair<key_type,mapped_type>&)> expires_signal_type;
		expires_signal_type& expires_signal()
		{
			return expires_signal_;
		}
	protected:
		expires_signal_type expires_signal_;
		eliment_set elimentSet_;
		tick_type tick_now()
		{
			return system_time::tick_count();
		}
		void clear_timeout(tick_type now)
		{
			typedef typename multi_index::nth_index_iterator<eliment_set,1>::type outtime_iterator;
			outtime_index_type& t_index=multi_index::get<1>(elimentSet_);
			outtime_iterator itr=t_index.begin();
			for (;itr!=t_index.end();)
			{
				if ((*itr).outTime<now)
				{
					expires_signal_(itr->data);
					itr=t_index.erase(itr);
				}
				else
				{
					break;
				}
			}
		}
	};

}

#include "p2engine/pop_warning_option.hpp"

#endif // id_keeper_h__
