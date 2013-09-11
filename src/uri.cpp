//
// url.ipp
// ~~~~~~~
//
// Copyright (c) 2009 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#ifdef _MSC_VER
#	define _SCL_SECURE_NO_WARNINGS//disable WARNINGS
#endif

#include "p2engine/uri.hpp"

#include <cstring>
#include <cctype>
#include <cstdlib>

#include <vector>

#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/system/system_error.hpp>
#include <boost/throw_exception.hpp>

namespace p2engine {

	void tokenize(const std::string& src, const std::string& tok,
		std::vector<std::string>& v,
		bool trim=false, const std::string& null_subst="")  
	{  
		typedef std::string::size_type size_type; 
		if( src.empty() || tok.empty() ) 
			return;
		size_type pre_index = 0, index = 0, len = 0;  
		while( (index = src.find_first_of(tok, pre_index)) !=std::string::npos)  
		{  
			if( (len = index-pre_index)!=0 )  
				v.push_back(src.substr(pre_index, len));  
			else if(trim==false&&null_subst.size())  
				v.push_back(null_subst);  
			pre_index = index+1;  
		}  
		std::string endstr = src.substr(pre_index);  
		if( trim==false ) 
			v.push_back( endstr.empty()?null_subst:endstr );  
		else if( !endstr.empty() ) 
			v.push_back(endstr);  
	}

	unsigned short uri::port() const
	{
		if (!port_.empty())
			return (unsigned short)std::atoi(port_.c_str());
		if (protocol_ == "http")
			return 80;
		if (protocol_ == "https")
			return 443;
		if (protocol_ == "ftp")
			return 21;
		if (protocol_ == "ftpes")
			return 21;
		if (protocol_ == "ftps")
			return 990;
		if (protocol_ == "tftp")
			return 69;
		return 0;
	}

	std::string uri::to_string(int components) const
	{
		std::string s;

		if ((components & protocol_component) != 0 && !protocol_.empty())
		{
			s = protocol_;
			s += "://";
		}

		if ((components & user_info_component) != 0 && !user_info_.empty())
		{
			s += user_info_;
			s += "@";
		}

		if ((components & host_component) != 0)
		{
			if (ipv6_host_)
				s += "[";
			s += host_;
			if (ipv6_host_)
				s += "]";
		}

		if ((components & port_component) != 0 && !port_.empty())
		{
			s += ":";
			s += port_;
		}

		if ((components & path_component) != 0 && !path_.empty())
		{
			s += path_;
		}

		if ((components & query_component) != 0 && !query_.empty())
		{
			s += "?";
			s += query_;
		}

		if ((components & fragment_component) != 0 && !fragment_.empty())
		{
			s += "#";
			s += fragment_;
		}

		return s;
	}

	uri uri::from_string(const char* p, boost::system::error_code& ec)
	{
		uri new_url;
		from_string(new_url,p,ec);
		if (ec)
			return uri();
		return new_url;
	}

	void uri::from_string(uri& new_url,const char* p, boost::system::error_code& ec)
	{
		std::string str=normalize(p);
		const char* s=str.c_str();

		// Protocol.
		std::size_t length = std::strcspn(s, ":");
		if (length+2<=str.length()&&
			(s[length]==':')&&(s[length+1]=='/')&&(s[length+2]=='/'))
		{
			new_url.protocol_.assign(s, s + length);
			boost::trim_if(new_url.protocol_, ::isspace);

			//boost::to_lower(new_url.protocol_);
			//std::local has a bug in msvc stl.It will caush crash. so,...
			for (size_t i=0;i<new_url.protocol_.size();++i)
				new_url.protocol_[i]=::tolower(new_url.protocol_[i]);
			
			s += length;

			// "://".
			if (*s++ != ':')
			{
				ec = make_error_code(boost::system::errc::invalid_argument);
				return;
			}
			if (*s++ != '/')
			{
				ec = make_error_code(boost::system::errc::invalid_argument);
				return;
			}
			if (*s++ != '/')
			{
				ec = make_error_code(boost::system::errc::invalid_argument);
				return;
			}
		}
		else
		{
			new_url.protocol_="";
		}
		// UserInfo.
		length = std::strcspn(s, "@:[/?#");
		if (s[length] == '@')
		{
			new_url.user_info_.assign(s, s + length);
			s += length + 1;
		}
		else if (s[length] == ':')
		{
			std::size_t length2 = std::strcspn(s + length, "@/?#");
			if (s[length + length2] == '@')
			{
				new_url.user_info_.assign(s, s + length + length2);
				s += length + length2 + 1;
			}
		}
		if (!new_url.user_info_.empty())
		{
			std::string::size_type n=new_url.user_info_.find(':');
			if (n!=std::string::npos)
			{
				new_url.user_name_=new_url.user_info_.substr(0,n);
				if (n<new_url.user_info_.length())
					new_url.user_password_=new_url.user_info_.substr(n+1,new_url.user_info_.length()-n);
			}
		}


		// Host.
		if (*s == '[')
		{
			length = std::strcspn(++s, "]");
			if (s[length] != ']')
			{
				ec = make_error_code(boost::system::errc::invalid_argument);
				return;
			}
			new_url.host_.assign(s, s + length);
			new_url.ipv6_host_ = true;
			s += length + 1;
			if (std::strcspn(s, ":/?#") != 0)
			{
				ec = make_error_code(boost::system::errc::invalid_argument);
				return;
			}
		}
		else
		{
			length = std::strcspn(s, ":/?#");
			new_url.host_.assign(s, s + length);
			s += length;
		}

		// Port.
		if (*s == ':')
		{
			length = std::strcspn(++s, "/?#");
			if (length == 0)
			{
				ec = make_error_code(boost::system::errc::invalid_argument);
				return;
			}
			new_url.port_.assign(s, s + length);
			for (std::size_t i = 0; i < new_url.port_.length(); ++i)
			{
				if (!::isdigit(new_url.port_[i]))
				{
					ec = make_error_code(boost::system::errc::invalid_argument);
					return;
				}
			}
			s += length;
		}

		// Path.
		if (*s == '/')
		{
			length = std::strcspn(s, "?#");
			new_url.path_.assign(s, s + length);
			std::string tmp_path;
			if (!unescape_path(new_url.path_, tmp_path))
			{
				ec = make_error_code(boost::system::errc::invalid_argument);
				return;
			}
			s += length;
		}
		else
			new_url.path_ = "/";

		// Query.
		if (*s == '?')
		{
			length = std::strcspn(++s, "#");
			new_url.query_.assign(s, s + length);
			s += length;

			const char* p=new_url.query_.c_str();
			std::vector<std::string> result;
			std::vector<std::string> result2;
			boost::split(result,p,boost::is_any_of("&"));
			for(std::size_t i=0;i<result.size();++i)
			{
				p=result[i].c_str();
				result2.clear();
				boost::split(result2,p,boost::is_any_of("="));
				if (result2.size()>0)
				{
					if(result2.size()>=2)
					{
						std::string tem_res;
						if (!unescape_path(result2[1], tem_res))
						{
							ec = make_error_code(boost::system::errc::invalid_argument);
							return;
						}
						new_url.query_map_[result2[0] ]=tem_res;
					}
					else
						new_url.query_map_[result2[0] ]="";
				}
			}
		}

		// Fragment.
		if (*s == '#')
			new_url.fragment_.assign(++s);

		ec = boost::system::error_code();
	}

	uri uri::from_string(const char* s)
	{
		boost::system::error_code ec;
		uri new_url(from_string(s, ec));
		if (ec)
		{
			boost::system::system_error ex(ec);
			boost::throw_exception(ex);
		}
		return new_url;
	}

	bool uri::unescape_path(const std::string& in, std::string& out)
	{
		out.clear();
		out.reserve(in.size());
		for (std::size_t i = 0; i < in.size(); ++i)
		{
			switch (in[i])
			{
			case '%':
				if (i + 3 <= in.size())
				{
					unsigned int value = 0;
					for (std::size_t j = i + 1; j < i + 3; ++j)
					{
						char c=in[j];
						if (c >= '0' && c <= '9')
							value += c - '0';
						else if (c >= 'a' && c <= 'f')
							value += c - 'a' + 10;
						else if (c >= 'A' && c <= 'F')
							value += c - 'A' + 10;
						else
							return false;
						if (j == i + 1)
							value <<= 4;
					}
					out += static_cast<char>(value);
					i += 2;
				}
				else
					return false;
				break;

			default:
				if (!is_uri_char(in[i]))
					return false;
				out += in[i];
				break;
			}
		}
		return true;
	}

	bool operator==(const uri& a, const uri& b)
	{
		return a.protocol_ == b.protocol_
			&& a.user_info_ == b.user_info_
			&& a.host_ == b.host_
			&& a.port_ == b.port_
			&& a.path_ == b.path_
			&& a.query_ == b.query_
			&& a.fragment_ == b.fragment_;
	}

	bool operator<(const uri& a, const uri& b)
	{
		if (a.protocol_ < b.protocol_)
			return true;
		if (b.protocol_ < a.protocol_)
			return false;

		if (a.user_info_ < b.user_info_)
			return true;
		if (b.user_info_ < a.user_info_)
			return false;

		if (a.host_ < b.host_)
			return true;
		if (b.host_ < a.host_)
			return false;

		if (a.port_ < b.port_)
			return true;
		if (b.port_ < a.port_)
			return false;

		if (a.path_ < b.path_)
			return true;
		if (b.path_ < a.path_)
			return false;

		if (a.query_ < b.query_)
			return true;
		if (b.query_ < a.query_)
			return false;

		return a.fragment_ < b.fragment_;
	}

	std::string uri::normalize(const char* in)
	{
		std::string inpath(in);

		//trim space
		boost::trim_if(inpath, ::isspace);

		//replace"\\" with "/"
		boost::replace_all(inpath, "\\", "/");

		//replace"//" with "/", but "://" will not be replaced
		boost::replace_first(inpath,":/",":\\");//prevent "://" be replaced
		std::size_t len=inpath.length();
		for (;;)
		{
			boost::replace_all(inpath, "//", "/");
			if (inpath.length()==len)
				break;
			else
				len=inpath.length();
		}
		boost::replace_first(inpath,":\\",":/");

		//is there a "/" at the end?
		bool append_slash = (inpath.size() > 1 && inpath[inpath.size()-1] == '/') ? 1 : 0;

		// this loop removes leading '/..' and trailing '/.' elements.
		typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
		boost::char_separator<char> sep("/");
		tokenizer tokens(inpath, sep);
		std::list<tokenizer::iterator> remainder;
		for (tokenizer::iterator iter = tokens.begin();
			iter != tokens.end(); ++iter)
		{
			if ( ".."== (*iter) )
			{
				if(!remainder.empty())
					remainder.pop_back();
				else
					remainder.push_back(iter);
			}
			else if ( "." != (*iter) )
			{
				remainder.push_back(iter);
			}
		}

		if (append_slash&&(inpath.empty()||inpath[inpath.length()-1]!='/'))
			return inpath+'/';
		return inpath;
	}

	/*
	* escape URL
	*
	* Puts escape codes in URLs.  (More complete than it used to be;
	* GN Jan 1997.  We escape all that isn't alphanumeric, "safe" or "extra"
	* as spec'd in RFCs 1738, 1808 and 2068.)
	*/
	bool uri::is_uri_char( int c )
	{
		static const char unreserved_chars[] =
			// when determining if a url needs encoding
			// % should be ok
			"%+"
			// reserved
			";?:@=&,$/"
			// unreserved (special characters) ' excluded,
			// since some buggy trackers fail with those
			"-_!.~*()";

		unsigned int uc=(unsigned int)c;
		if ( uc > 128 )
			return false;
		if ( std::isalnum ( uc ) )
			return true;
		if ( strchr ( unreserved_chars, uc ) )
			return true;
		return false;
	}

	bool uri::is_escaped(const std::string& s)
	{
		//const char* unsafe = " <>#{}|\\^~[]`";
		bool has_only_url_chars = true;
		bool has_percent = false;
		for( size_t n = 0; n < s.length(); ++n )
		{
			char c = s[n];
			if( c == '%' )
				has_percent = true;
			else if( !is_uri_char(c) )
				has_only_url_chars = false;
		}
		return has_percent && has_only_url_chars;
	}

	std::string uri::escape(const char *src, bool space_to_plus )
	{
		static const char *hex = "0123456789ABCDEF";

		std::string buffer;
		buffer.reserve(2*strlen(src));

		for ( const char *cp = src; *cp; cp++ )
		{
			if ( *cp == ' ' && space_to_plus )
				buffer+= ( '+' );
			else if ( *cp == '\\')
				buffer+= ( '/' );
			else if ( is_uri_char ( (unsigned char) *cp ) || ( *cp == '+' && !space_to_plus ) )
			{
				buffer+= ( *cp );
			}
			else
			{
				buffer+= ( '%' );
				buffer+= ( hex [ (unsigned char) *cp / 16 ] );
				buffer+= ( hex [ (unsigned char) *cp % 16 ] );
			}
		}

		return buffer;
	}

} // namespace url

