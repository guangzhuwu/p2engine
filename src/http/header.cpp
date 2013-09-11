//
// header.hpp
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
#include "p2engine/http/header.hpp"
#include "p2engine/push_warning_option.hpp"
#include <iostream>
#include <cctype>
#include <exception>
#include <sstream>
#include <streambuf>
#include "p2engine/pop_warning_option.hpp"

namespace p2engine { namespace http{
	//
#define HTTP_ATOM_DEFINE
#include "p2engine/http/atom.hpp"
#undef HTTP_ATOM_DEFINE


	const std::string header::m_nullString;

	header::header(const std::string& version)
		:m_version(version)
		,m_parseState(FIRSTLINE_1)
		,m_iCRorLFcontinueNum(0)
	{
	}

	header::~header()
	{
	}

	const std::string& header::get(const std::string&name ) const
	{
		map_type::const_iterator itr=m_mapNameValue.find(name);
		if (itr!=m_mapNameValue.end())
			return itr->second;
		return m_nullString;
	}


	const std::string& header::get(const std::string&name,const std::string& defauleValue) const
	{
		map_type::const_iterator itr=m_mapNameValue.find(name);
		if (itr!=m_mapNameValue.end())
			return itr->second;
		return defauleValue;
	}

	void header::clear()
	{
		m_mapNameValue.clear();
		m_version.clear();
		m_key.clear();
		m_value.clear();
		m_iCRorLFcontinueNum=0;
		m_parseState=FIRSTLINE_1;
	}

	int header::serialize(std::ostream& ostr) const
	{
		std::ostream::pos_type len=ostr.tellp();
		if (len<0)len=0;
		map_type::const_iterator itr =m_mapNameValue.begin();
		for(;itr != m_mapNameValue.end();++itr)
		{ 
			ostr <<itr->first << ": " << itr->second<<"\r\n";
		}
		return  static_cast<int>(ostr.tellp()-len);
	}

	int header::serialize(std::string& str) const
	{
		std::size_t orgLen=str.length();
		map_type::const_iterator itr =m_mapNameValue.begin();
		for(;itr != m_mapNameValue.end();++itr)
		{
			str+=itr->first;
			str+=": ";
			str+=itr->second;
			str+="\r\n";
		}
		return static_cast<int>(str.length()-orgLen);
	}

	int header::serialize(safe_buffer_io& buf) const
	{
		std::string s;
		serialize(s);
		buf<<s;
		return static_cast<int>(s.size());
	}

	int header::parse(const char* begin,std::size_t len)
	{
		if (len==0)
			return 0;
		const char* readPtr=begin;
		const char* readEndPtr=begin+len;
		for (;readPtr!=readEndPtr;++readPtr)
		{
			switch (m_parseState)
			{
			case PARSER_KEY:
				if (*readPtr==' '||*readPtr=='\t')
				{
					if (m_key.empty())
						break;
					else 
						return -1;
				}
				//»›¥Ì
				if (*readPtr=='\n'||*readPtr=='\r')
				{
					m_iCRorLFcontinueNum++;
					if (m_iCRorLFcontinueNum==4)
					{
						++readPtr;
						return int(readPtr-begin);
					}
					if(!m_key.empty())
						m_key.clear();
					break;
				}
				//∂¡»°m_key
				if(*readPtr != ':' 
					&&m_key.length() < MAX_NAME_LENGTH
					) 
				{ 
					if (m_iCRorLFcontinueNum==3)
						return int(readPtr-begin);
					m_iCRorLFcontinueNum=0;
					m_key += *readPtr; 
					break;
				}
				else if (*readPtr== ':')
				{
					if (m_iCRorLFcontinueNum==3)
						return -1;
					m_iCRorLFcontinueNum=0;
					m_parseState=PARSER_VALUE;
					break;
				}
				else
				{
					m_iCRorLFcontinueNum=0;
					//OutputError("header field name too long; no colon found!");
					return -1;
				}
			case PARSER_VALUE:
				if (*readPtr==' '||*readPtr=='\t')
				{
					if (m_value.empty())
						break;
					else 
						m_value += *readPtr; 
					break;
				}
				//∂¡»°value
				if(*readPtr!= '\r' 
					&&*readPtr!= '\n' 
					&& m_key.length() < MAX_NAME_LENGTH
					) 
				{ 
					if (m_iCRorLFcontinueNum==3)
						return int(readPtr-begin);
					m_iCRorLFcontinueNum=0;
					m_value += *readPtr; 
					break;
				}
				else if (
					*readPtr=='\r'
					||*readPtr=='\n'
					)
				{
					m_iCRorLFcontinueNum++;
					m_parseState=PARSER_KEY;
					if (!m_key.empty()&&!m_value.empty())
					{
						m_mapNameValue.insert(make_pair(m_key, m_value));
						m_key.clear();
						m_value.clear();
					}
					break;
				}
				else
				{
					////OutputError("header field name too long; no colon found!");
					return -1;
				}
			default:
				assert(0);
				return -1;
			}
		}
		return 0;
	}

	int header::parse(std::istream& istr)
	{
		std::istreambuf_iterator<char> itr(istr);
		std::istreambuf_iterator<char> end;

		char buf[1024];
		int rnCnt=0;
		int i=0;
		while (itr!=end && i<int(sizeof(buf)-1))
		{
			char ch=*itr;
			buf[i++]=ch;
			if(ch == '\r'||ch == '\n')
			{
				if (++rnCnt==4)
					break;
			}
			else
			{
				rnCnt=0;
			}
			ch=(char)istr.get();
		}
		return parse(buf,i);
	}

	void header::content_length(int64_t length)
	{
		if (length >0)
		{
			char buf[32]={0};
			snprintf(buf,sizeof(buf),"%"INT64_F,length);
			this->set(HTTP_ATOM_Content_Length,buf);
		}
		else
			erase(HTTP_ATOM_Content_Length);
	}

	int64_t header::content_length() const
	{
		map_type::const_iterator itr=m_mapNameValue.find(HTTP_ATOM_Content_Length);
		if (itr!=m_mapNameValue.end())
		{
			const char* p=itr->second.c_str();
			if (!isdigit(*p))
				return -1;
			return _atoi64(p);
		}
		return -1;
	}

	void header::transfer_encoding(const std::string& transferEncoding)
	{
		if (transferEncoding==IDENTITY_TRANSFER_ENCODING)
			erase(HTTP_ATOM_Transfer_Encoding);
		else
			set(HTTP_ATOM_Transfer_Encoding, transferEncoding);
	}

	const std::string& header::transfer_encoding() const
	{
		map_type::const_iterator itr=m_mapNameValue.find(HTTP_ATOM_Transfer_Encoding);
		if (itr!=m_mapNameValue.end())
			return itr->second;
		else
			return IDENTITY_TRANSFER_ENCODING;
	}

	void header::chunked_transfer_encoding(bool flag)
	{
		if (flag)
			set(HTTP_ATOM_Transfer_Encoding, CHUNKED_TRANSFER_ENCODING);
		else
			erase(HTTP_ATOM_Transfer_Encoding);
	}

	void header::content_type(const std::string& mediaType)
	{
		if (mediaType.empty())
			erase(HTTP_ATOM_Content_Type);
		else
			set(HTTP_ATOM_Content_Type, mediaType);
	}

	void header::keep_alive(bool keepAlive,int timeout,int maxRequest)
	{
		if (keepAlive)
		{
			char buf[256]={0};
			snprintf(buf,sizeof(buf)-1,"timeout=%d,max=%d",timeout,maxRequest);
			set(HTTP_ATOM_Connection, CONNECTION_KEEP_ALIVE);
			set(CONNECTION_KEEP_ALIVE,buf);
		}
		else
		{
			set(HTTP_ATOM_Connection, CONNECTION_CLOSE);
			erase(CONNECTION_KEEP_ALIVE);
		}
	}

	bool header::keep_alive() const
	{
		const std::string& keeyAlive=get(HTTP_ATOM_Connection);
		if (keeyAlive.empty())
			return  version() == HTTP_VERSION_1_1;
		else
			return !strcasecmp(keeyAlive.c_str(),CONNECTION_KEEP_ALIVE.c_str());
	}

}
}
