//
// response.cpp
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

#include "p2engine/http/response.hpp"
#include "p2engine/push_warning_option.hpp"
#ifdef BOOST_NO_STRINGSTREAM
#include <strstream>
#else
#include <sstream>
#endif
#include "p2engine/pop_warning_option.hpp"

namespace p2engine { namespace http{

	response::response()
		:m_statusForParser(0)
		,m_status(INVALID_STATUS)
	{
	}

	response::~response()
	{
	}

	void response::clear()
	{
		m_statusForParser=0;
		m_status=INVALID_STATUS;
		m_version.clear();
		m_reason.clear();
		header::clear();
	}

	int response::serialize(std::string& str) const
	{
		std::ostringstream ostr;
		int l=serialize(ostr);
		str=ostr.str();
		return l;
	}

	int response::serialize(std::ostream& ostr) const
	{
		std::ostream::off_type len=ostr.tellp();
		if (len<0)len=0;
		ostr <<version()<<" "<<m_status<<" "<<reason_for_status(m_status)<<"\r\n";
		if(header::serialize(ostr)<0)
			return -1;
		ostr << "\r\n";
		return static_cast<int>(ostr.tellp()-len);
	}

	int response::parse(const char* begin,std::size_t len)
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
					if (m_version.empty())
						break;
					else if (m_version.length()!=MAX_VERSION_LENGTH)
						return -1;
					else
					{
						m_statusForParser=0;
						m_parseState=FIRSTLINE_2;
					}
					break;
				}
				if (*readPtr=='\r'||*readPtr=='\n')
					return -1;
				m_version+=*readPtr;
				if (m_version.length()>MAX_VERSION_LENGTH)
					return -1;
				break;
			case FIRSTLINE_2:
				if (*readPtr==' '||*readPtr=='\t')
				{
					if (m_statusForParser==0)
						break;
					else
					{
						m_parseState=FIRSTLINE_3;
						m_status=(status_type)m_statusForParser;
					}
					break;
				}
				if (*readPtr=='\r'||*readPtr=='\n')
				{
					m_status=(status_type)m_statusForParser;
					m_parseState=PARSER_KEY;
					--readPtr;//使得header::read能正确计算连续的'\r''\n'的个数
					break;
				}
				if (!isdigit(*readPtr))
					return -1;
				else
					m_statusForParser=m_statusForParser*10+((*readPtr)-'0');
				break;
			case FIRSTLINE_3:
				if (*readPtr==' '||*readPtr=='\t')
				{
					if (m_reason.empty())
						break;
					else
						m_reason+=*readPtr;
					break;
				}
				if (*readPtr=='\r'||*readPtr=='\n')
				{
					m_parseState=PARSER_KEY;
					--readPtr;//使得header::read能正确计算连续的'\r''\n'的个数
					break;
				}
				m_reason+=*readPtr;
				if (m_reason.length()>MAX_VALUE_LENGTH)
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

	void response::content_range(int64_t from,int64_t to,int64_t total)
	{
		//Content-Range:  bytes  0-100/2350
		std::ostringstream os;
		os<<"bytes "<<from<<"-"<<to<<"/"<<total;
		set(HTTP_ATOM_Content_Range,os.str());
	}

	std::pair<int64_t,int64_t> response::content_range()const
	{
		//Content-Range:  bytes  0-100/2350
		const std::string& r=get(HTTP_ATOM_Content_Range);
		int64_t from=-1, to=-1;
#ifdef _MSC_VER
		sscanf(r.c_str(),"%*[^0-9]%I64d-%I64d/",&from,&to);
#else
		sscanf(r.c_str(),"%*[^0-9]%lld-%lld/",&from,&to);
#endif
		return std::pair<int64_t,int64_t>(from,to);
	}

	const std::string& response::reason_for_status(status_type s)
	{
		switch(s)
		{
		case INVALID_STATUS:
			return m_nullString;
		case HTTP_CONTINUE:
			return HTTP_REASON_CONTINUE;
		case HTTP_SWITCHING_PROTOCOLS:
			return HTTP_REASON_SWITCHING_PROTOCOLS;
		case HTTP_OK:
			return HTTP_REASON_OK;
		case HTTP_CREATED:
			return HTTP_REASON_CREATED ;
		case HTTP_ACCEPTED:
			return HTTP_REASON_ACCEPTED ;
		case HTTP_NONAUTHORITATIVE:
			return HTTP_REASON_NONAUTHORITATIVE ;
		case HTTP_NO_CONTENT:
			return HTTP_REASON_NO_CONTENT  ;
		case HTTP_RESET_CONTENT:
			return HTTP_REASON_RESET_CONTENT  ;
		case HTTP_PARTIAL_CONTENT:
			return HTTP_REASON_PARTIAL_CONTENT  ;
		case HTTP_MULTIPLE_CHOICES:
			return HTTP_REASON_MULTIPLE_CHOICES  ;
		case HTTP_MOVED_PERMANENTLY:
			return HTTP_REASON_MOVED_PERMANENTLY  ;
		case HTTP_FOUND:
			return HTTP_REASON_FOUND  ;
		case HTTP_SEE_OTHER:
			return HTTP_REASON_SEE_OTHER  ;
		case HTTP_NOT_MODIFIED:
			return HTTP_REASON_NOT_MODIFIED ;
		case HTTP_USEPROXY:
			return HTTP_REASON_USEPROXY;
		case HTTP_TEMPORARY_REDIRECT:
			return HTTP_REASON_TEMPORARY_REDIRECT;
		case HTTP_BAD_REQUEST:
			return HTTP_REASON_BAD_REQUEST;
		case HTTP_PAYMENT_REQUIRED:
			return HTTP_REASON_PAYMENT_REQUIRED;
		case HTTP_FORBIDDEN:
			return HTTP_REASON_FORBIDDEN;
		case HTTP_NOT_FOUND:
			return HTTP_REASON_NOT_FOUND;
		case HTTP_METHOD_NOT_ALLOWED:
			return HTTP_REASON_METHOD_NOT_ALLOWED;
		case HTTP_NOT_ACCEPTABLE:
			return HTTP_REASON_NOT_ACCEPTABLE;
		case HTTP_PROXY_AUTHENTICATION_REQUIRED:
			return HTTP_REASON_PROXY_AUTHENTICATION_REQUIRED;
		case HTTP_REQUEST_TIMEOUT:
			return HTTP_REASON_REQUEST_TIMEOUT;
		case HTTP_CONFLICT:
			return HTTP_REASON_CONFLICT;
		case HTTP_GONE:
			return HTTP_REASON_GONE;
		case HTTP_LENGTH_REQUIRED:
			return HTTP_REASON_LENGTH_REQUIRED;
		case HTTP_PRECONDITION_FAILED:
			return HTTP_REASON_PRECONDITION_FAILED;
		case HTTP_REQUESTENTITYTOOLARGE:
			return HTTP_REASON_REQUESTENTITYTOOLARGE;
		case HTTP_REQUESTURITOOLONG:
			return HTTP_REASON_REQUESTURITOOLONG;
		case HTTP_UNSUPPORTEDMEDIATYPE:
			return HTTP_REASON_UNSUPPORTEDMEDIATYPE;
		case HTTP_REQUESTED_RANGE_NOT_SATISFIABLE:
			return HTTP_REASON_REQUESTED_RANGE_NOT_SATISFIABLE;
		case HTTP_EXPECTATION_FAILED:
			return HTTP_REASON_EXPECTATION_FAILED;
		case HTTP_INTERNAL_SERVER_ERROR:
			return HTTP_REASON_INTERNAL_SERVER_ERROR;
		case HTTP_NOT_IMPLEMENTED:
			return HTTP_REASON_NOT_IMPLEMENTED;
		case HTTP_BAD_GATEWAY:
			return HTTP_REASON_BAD_GATEWAY;
		case HTTP_SERVICE_UNAVAILABLE:
			return HTTP_REASON_SERVICE_UNAVAILABLE;
		case HTTP_GATEWAY_TIMEOUT:
			return HTTP_REASON_GATEWAY_TIMEOUT;
		case HTTP_VERSION_NOT_SUPPORTED:
			return HTTP_REASON_VERSION_NOT_SUPPORTED;
		default:
			return m_nullString;
		}
	}

}
}