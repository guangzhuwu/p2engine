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

#ifndef HttpHeader_h__
#define HttpHeader_h__

#include "p2engine/push_warning_option.hpp"
#include "p2engine/config.hpp"
#include <cstring>
#include <string>
#include <map>
#include "p2engine/pop_warning_option.hpp"

#include "p2engine/safe_buffer_io.hpp"

namespace p2engine { namespace http{

#define HTTP_ATOM_DECLARE
#include "p2engine/http/atom.hpp"
#undef HTTP_ATOM_DECLARE

	class basic_http_connection_impl;

	class  header
	{
	public:
		enum status_type
		{
			INVALID_STATUS=0,
			HTTP_CONTINUE                        = 100,
			HTTP_SWITCHING_PROTOCOLS             = 101,

			HTTP_OK                              = 200,
			HTTP_CREATED                         = 201,
			HTTP_ACCEPTED                        = 202,
			HTTP_NONAUTHORITATIVE                = 203,
			HTTP_NO_CONTENT                      = 204,
			HTTP_RESET_CONTENT                   = 205,
			HTTP_PARTIAL_CONTENT                 = 206,

			HTTP_MULTIPLE_CHOICES                = 300,
			HTTP_MOVED_PERMANENTLY               = 301,
			HTTP_FOUND                           = 302,
			HTTP_SEE_OTHER                       = 303,
			HTTP_NOT_MODIFIED                    = 304,
			HTTP_USEPROXY                        = 305,
			// UNUSED: 306
			HTTP_TEMPORARY_REDIRECT              = 307,

			HTTP_BAD_REQUEST                     = 400,
			HTTP_UNAUTHORIZED                    = 401,
			HTTP_PAYMENT_REQUIRED                = 402,
			HTTP_FORBIDDEN                       = 403,
			HTTP_NOT_FOUND                       = 404,
			HTTP_METHOD_NOT_ALLOWED              = 405,
			HTTP_NOT_ACCEPTABLE                  = 406,
			HTTP_PROXY_AUTHENTICATION_REQUIRED   = 407,
			HTTP_REQUEST_TIMEOUT                 = 408,
			HTTP_CONFLICT                        = 409,
			HTTP_GONE                            = 410,
			HTTP_LENGTH_REQUIRED                 = 411,
			HTTP_PRECONDITION_FAILED             = 412,
			HTTP_REQUESTENTITYTOOLARGE           = 413,
			HTTP_REQUESTURITOOLONG               = 414,
			HTTP_UNSUPPORTEDMEDIATYPE            = 415,
			HTTP_REQUESTED_RANGE_NOT_SATISFIABLE = 416,
			HTTP_EXPECTATION_FAILED              = 417,

			HTTP_INTERNAL_SERVER_ERROR           = 500,
			HTTP_NOT_IMPLEMENTED                 = 501,
			HTTP_BAD_GATEWAY                     = 502,
			HTTP_SERVICE_UNAVAILABLE             = 503,
			HTTP_GATEWAY_TIMEOUT                 = 504,
			HTTP_VERSION_NOT_SUPPORTED           = 505
		};

	public:
		//…Ë÷√"HTTP/1.1 or HTTP/1.0"
		void version(const std::string& v);
		const std::string& version() const;

		void content_length(int64_t length);
		int64_t content_length() const;

		void transfer_encoding(const std::string& transferEncoding);
		const std::string& transfer_encoding() const;

		void chunked_transfer_encoding(bool flag);
		bool chunked_transfer_encoding() const;

		void content_type(const std::string& mediaType);
		const std::string& content_type() const;

		void keep_alive(bool keepAlive,int timeout=65,int maxRequest=100);
		bool keep_alive() const;

		virtual int parse(const char* begin,std::size_t len);

		int parse(std::istream& istr);
		int parse(const safe_buffer& buf)
		{
			return parse(buffer_cast<const char*>(buf),buf.length());
		}
		int parse(safe_buffer_io&io)
		{
			int len=parse(io.gptr(),io.size());
			if (len==0)
				io.consume(len);
			else
				io.consume(io.size());
			return len;
		}
		virtual int serialize(std::string& str) const;
		virtual int serialize(std::ostream& ostr) const;
		int serialize(safe_buffer& buf) const
		{
			safe_buffer_io io(&buf);
			return serialize(io);
		}
		int serialize(safe_buffer_io& buf) const;

		void erase(const std::string&name);
		void set(const std::string&name,const std::string& value);
		bool has(const std::string&name)const;
		virtual void clear();

		const std::string& get(const std::string&name ) const;
		const std::string& get(const std::string&name,
			const std::string& defauleValue) const;


	protected:
		explicit header(const std::string& version=HTTP_VERSION_1_1);
		//header& operator= (const header&h);
		virtual ~header();

	protected:
		enum Limits
		{
			MAX_NAME_LENGTH  = 64,
			MAX_VALUE_LENGTH = 4096
		};
		enum HeadersParseState 
		{
			FIRSTLINE_1,
			FIRSTLINE_2,
			FIRSTLINE_3,
			PARSER_KEY,
			PARSER_VALUE
		};
		struct stricmp_less
		{
			bool operator()(const std::string& s1, const std::string& s2)const
			{
#ifndef WINDOWS_OS
				return strcasecmp(s1.c_str(),s2.c_str())<0;
#else
				return _stricmp(s1.c_str(),s2.c_str())<0;
#endif
			}
		};
		typedef std::map<std::string,std::string,stricmp_less>  map_type;
		map_type m_mapNameValue;

		std::string m_version;

		HeadersParseState m_parseState;
		std::string m_key;
		std::string m_value;
		int m_iCRorLFcontinueNum;

		const static std::string m_nullString;
	};

	inline void header::erase(const std::string&name)
	{
		m_mapNameValue.erase(name);
	}

	inline void header::set(const std::string&name,const std::string& v)
	{
		m_mapNameValue[name]=v;
	}

	inline bool header::has(const  std::string&name)const
	{
		return m_mapNameValue.find(name)!=m_mapNameValue.end();
	}

	inline void header::version(const std::string& version)
	{
		m_version=version;
	}

	inline const std::string& header::version() const
	{
		return m_version;
	}

	inline bool header::chunked_transfer_encoding() const
	{
		return transfer_encoding()==CHUNKED_TRANSFER_ENCODING;
	}

	inline const std::string& header::content_type() const
	{
		return get(HTTP_ATOM_Content_Type);
	}

	template<class Ostream>
	inline Ostream& operator <<(Ostream &os,const header&obj)
	{
		obj.serialize(os);
		return os;
	}

	template<class Istream>
	inline Istream& operator >>(Istream &is, header&obj)
	{
		obj.parse(is);
		return is;
	}

}

template<>
inline safe_buffer_io& operator << (safe_buffer_io&io, const http::header& obj) 
{ 
	obj.serialize(io);
	return io;
}

template<>
inline safe_buffer_io& operator >> (safe_buffer_io&io,  http::header& obj) 
{ 
	obj.parse(io);
	return io;
}


}

#endif // HttpHeader_h__
