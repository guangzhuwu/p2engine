//
// request.cpp
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

#include "p2engine/uri.hpp"
#include "p2engine/http/request.hpp"

#include "p2engine/push_warning_option.hpp"
#include <cctype>
#include <iostream>
#include <boost/lexical_cast.hpp>
#include "p2engine/pop_warning_option.hpp"

namespace p2engine { namespace http{

	request::request()
		:m_method(HTTP_METHORD_GET)
		,m_strUri("/")
	{
	}

	request::request(const std::string& version)
		:header(version)
		,m_method(HTTP_METHORD_GET)
		,m_strUri("/")
	{
	}

	request::request(const std::string& method, const std::string& u)
		:m_method(method)
	{
		url(u);
	}

	request::request(const std::string& method, const std::string& u, const std::string& version)
		:header(version)
		,m_method(method)
	{
		url(u);
	}

	request::~request()
	{
	}

	void request::range(int64_t from,int64_t to)
	{
		if (from>=0)
		{
			char buf[256]={0};
			if (to>=from)
				snprintf(buf,sizeof(buf),"bytes=%"INT64_F"-%"INT64_F,from,to);
			else
				snprintf(buf,sizeof(buf),"bytes=%"INT64_F"-",from);
			set(HTTP_ATOM_Range,buf);
		}
		else
		{
			erase(HTTP_ATOM_Range);
		}
	}

	std::pair<int64_t,int64_t> request::range()const
	{
		const std::string& r=get(HTTP_ATOM_Range);
		int64_t from=-1, to=-1;
		sscanf(r.c_str(),"%*[^0-9]%"INT64_F"-%"INT64_F,&from,&to);
		return std::pair<int64_t,int64_t>(from,to);
	}

	void request::clear()
	{
		m_method.clear();
		m_strUri.clear();
		header::clear();
	}

	void request::url(const std::string& s)
	{
		boost::system::error_code ec;
		p2engine::uri u=p2engine::uri::from_string(s,ec);
		if (!ec)
		{
			m_strUri = u.to_string(p2engine::uri::all_components
				&~p2engine::uri::protocol_component
				&~p2engine::uri::user_info_component
				&~p2engine::uri::port_component
				&~p2engine::uri::host_component
				);
			if (!u.host().empty())
				host(u.host(),u.port());
		}
		else
		{
			m_strUri = s;
		}
	}

	void request::host(const std::string& s, unsigned short port)
	{
		if (port !=80)
		{
			char buf[32]={0};
			snprintf(buf,sizeof(buf),"%u",port);
			host(s+buf);
		}
		else
		{
			host(s);
		}
	}

	unsigned short request::port()const
	{
		const std::string& h=host();
		if (std::string::size_type pos=h.find(":")!=std::string::npos)
			return atoi(h.c_str()+pos+1);
		return 80;
	}

	void request::get_credentials(std::string& scheme, std::string& authInfo) const
	{
		scheme.clear();
		authInfo.clear();
		const std::string& auth = get(HTTP_ATOM_Authorization);
		if (!auth.empty())
		{
			const std::string& auth = get(HTTP_ATOM_Authorization);
			std::string::const_iterator it  = auth.begin();
			std::string::const_iterator end = auth.end();
			while (it != end && std::isspace(*it)) 
				++it;
			while (it != end && !std::isspace(*it)) 
				scheme += *it++;
			while (it != end && std::isspace(*it)) 
				++it;
			while (it != end) 
				authInfo += *it++;
		}
	}

	void request::set_credentials(const std::string& scheme, const std::string& authInfo)
	{
		std::string auth;
		auth.reserve(scheme.length()+authInfo.length()+1);
		auth+=scheme;
		auth+=' ';
		auth+=authInfo;
		set(HTTP_ATOM_Authorization, auth);
	}

	int request::serialize(std::string& str) const
	{
		std::ostringstream ostr;
		int l=serialize(ostr);
		str=ostr.str();
		return l;
	}

	int request::serialize(std::ostream& ostr) const
	{
		std::ostream::off_type len=ostr.tellp();
		if (len<0)len=0;
		BOOST_ASSERT(!host().empty());
		ostr << m_method << " " <<m_strUri<< " " << version() << "\r\n";
		if(header::serialize(ostr)<0)
			return -1;
		ostr << "\r\n";
		return static_cast<int>(ostr.tellp()-len);
	}

	int request::parse(const char* begin,std::size_t len)
	{
		const char* readPtr=begin;
		const char* readEndPtr=begin+len;
		for (;readPtr!=readEndPtr;++readPtr)
		{
			switch (m_parseState)
			{
			case FIRSTLINE_1:
				if (*readPtr==' '||*readPtr=='\t')
				{
					if (m_method.empty())
						break;
					else if (m_method.length()>MAX_METHOD_LENGTH)
						return -1;
					else
					{
						m_parseState=FIRSTLINE_2;
						break;
					}
				}
				else if (!isupper(*readPtr))
				{
					return -1;
				}
				m_method+=*readPtr;
				break;
			case FIRSTLINE_2:
				if (*readPtr==' '||*readPtr=='\t')
				{
					if (m_strUri.empty())
						break;
					else 
					{
						m_parseState=FIRSTLINE_3;
						break;
					}
				}
				m_strUri+=*readPtr;
				break;
			case FIRSTLINE_3:
				if (m_strUri.empty())
				{
					return -1;
				}
				if (*readPtr==' '||*readPtr=='\t')
				{
					if (m_version.empty())
						break;
					else
						m_parseState=PARSER_KEY;
					break;
				}
				if (*readPtr=='\r'||*readPtr=='\n')
				{
					m_parseState=PARSER_KEY;
					--readPtr;//make header::read find '\r''\n'
					break;
				}
				m_version+=*readPtr;
				if (m_version.length()>MAX_VERSION_LENGTH)
					return -1;
				break;
			default:
				{
					int havePasered=int(readPtr-begin);
					int parselen= header::parse(readPtr,len-havePasered);
					if (parselen<=0)
						return parselen;
					return havePasered+parselen;
				}
			}
		}
		return 0;
	}

}
}