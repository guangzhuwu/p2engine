#include "p2engine/fssignal.hpp"


namespace p2engine{namespace fssignal{ namespace detail{

	//////////////////////////////////////////////////////////////////////////
	//connection_base
	void connection_base::disconnect()
	{
		if (!connected())
			return;
		BOOST_ASSERT(signal_);

		//prevent redisconnect
		signal_base_impl* sig=signal_;
		signal_=NULL;

		make_slot_null();

		BOOST_FOREACH(trackable_list::reference v, trackables_)
		{
			BOOST_ASSERT(*v.second==this);
			v.first->disconnect_without_callback_connection(v.second);
		}
		trackables_.clear();

		sig->disconnect_without_callback_connection(this);//must be called in the last line
	}

	connection_base::trackable_list::value_type* connection_base::track(trackable* t)
	{
		for (trackable_list::iterator itr=trackables_.begin(),end=trackables_.end();
			itr!=end;
			++itr
			)
		{
			if (itr->first==t)
				return NULL;
		}
		trackables_.push_back(std::make_pair(t,connection_ptr_iterator()));
		return &trackables_.back();
	}

	//////////////////////////////////////////////////////////////////////////
	//signal_base_impl
	signal_base_impl::~signal_base_impl()
	{
		clear();
	}

	void signal_base_impl::disconnect_all_slots()
	{
		if (m_connections.empty())
			return;

		if (!is_emiting())
		{
			need_clear_pending_=false;
			for (;!m_connections.empty();)
			{
				connection_list::reference conn=m_connections.front();
				if (!conn||!conn->connected())
					m_connections.pop_front();
				else
					conn->disconnect();
			}
		}
		else
		{
			need_clear_pending_=true;
			BOOST_FOREACH(connection_list::reference conn,m_connections)
			{
				if (conn&&conn->connected()) 
					conn->disconnect();
			}
		}
	}

	void signal_base_impl::pop_front_slot()
	{
		if (m_connections.empty())
			return;
		if (is_emiting())
		{
			BOOST_FOREACH(connection_list::reference conn, m_connections)
			{
				if (conn&&conn->connected())
				{
					conn->disconnect();
					need_clear_pending_=true;
					break;
				}
			}
		}
		else
		{
			for (;!m_connections.empty();)
			{
				connection_list::reference conn=m_connections.front();
				if (conn&&conn->connected())
				{
					conn->disconnect();
					m_connections.pop_front();
					break;
				}
				else
				{
					m_connections.pop_front();
				}
			}
		}
	}
	void signal_base_impl::pop_back_slot()
	{
		if (m_connections.empty())
			return;
		if (is_emiting())
		{
			typedef connection_list::reverse_iterator  riterator;
			for (riterator ritr=m_connections.rbegin(),rend=m_connections.rend();
				ritr!=rend;
				++ritr)
			{
				connection_list::reference conn=*ritr;
				if (conn&&conn->connected())
				{
					need_clear_pending_=true;
					conn->disconnect();
					break;
				}
			}
		}
		else
		{
			for (;!m_connections.empty();)
			{
				connection_list::reference conn=m_connections.back();
				if (conn&&conn->connected())
				{
					conn->disconnect();
					m_connections.pop_back();
					break;
				}
				else
				{
					m_connections.pop_back();
				}
			}
		}
	}
	void signal_base_impl::disconnect_without_callback_connection(connection_base* conn)
	{
		if (is_emiting())
		{
			conn->iterator_in_signal()->reset();
			need_clear_pending_=true;
		}
		else
		{
			m_connections.erase(conn->iterator_in_signal());
		}
	}
	void signal_base_impl::clear_pending_erase()
	{
		if (m_connections.empty())
		{
			BOOST_ASSERT(!need_clear_pending_);
			return;
		}
		if (!need_clear_pending_)
		{
			BOOST_ASSERT(size()==m_connections.size());
			return;
		}

		typedef connection_list::iterator  iterator;
		for (iterator itr=m_connections.begin(), end=m_connections.end();itr!=end;)
		{
			connection_list::reference conn=*itr;
			if (!conn||!conn->connected())
				m_connections.erase(itr++);
			else
				++itr;
		}
		need_clear_pending_=false;
	}

}}}
