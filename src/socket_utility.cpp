#include "p2engine/socket_utility.hpp"

NAMESPACE_BEGIN(p2engine);

std::pair<address,int> ipport_from_string(const std::string&ipPortStr)
{
	std::string ipStr,portStr;
	const size_t first=ipPortStr.find_first_of(':');
	const size_t last=(first==std::string::npos
		?std::string::npos : ipPortStr.find_last_of(':',first+1)
		);
	bool v6=false;
	if (first!=std::string::npos)
	{
		const size_t posR=ipPortStr.find_first_of(']');
		const size_t posL=ipPortStr.find_first_of('[');
		if (last!=first
			||posR!=std::string::npos
			||posL!=std::string::npos
			)//is ipv6?
		{
			if (posR!=std::string::npos&&posL==std::string::npos
				||posR==std::string::npos&&posL!=std::string::npos
				||posL>posR
				)
			{
				return std::pair<address,int>(address(),0);//error format
			}
			v6=true;
			if (posR!=std::string::npos)//[ip]:port
			{
				BOOST_ASSERT(posL!=std::string::npos&&posR>posL);
				ipStr=ipPortStr.substr(posL+1,posR-posL-1);
				if (last>posR&&last!=std::string::npos&&last+1<ipPortStr.size())
					portStr=ipPortStr.substr(last+1);
			}
			else// only ip
			{
				ipStr=ipPortStr;
			}
		}
		else//is ipv4?
		{
			ipStr=ipPortStr.substr(0,last);
			if (last+1<ipPortStr.size())
				portStr=ipPortStr.substr(last+1);
		}
	}
	else
	{
		const size_t pos=ipPortStr.find_first_of(']');
		if (pos!=std::string::npos)//[]
		{
			v6=true;
			if (ipPortStr[0]=='[')
				ipStr=ipPortStr.substr(1,pos);
			else
				return std::pair<address,int>(address(),0);//error format
			if (pos+1<ipPortStr.size())
				portStr=ipPortStr.substr(pos+1);
			if (boost::iequals(ipStr,"localhost")||boost::iequals(ipStr,"loopback"))
			{
				ipStr="::1";
			}
		}
		else// only ip
		{
			ipStr=ipPortStr;
		}
		ipStr=ipPortStr;
	}

	error_code ec;
	if (ipStr.empty())
	{
		if (v6)
			ipStr=address_v6().to_string(ec);
		else
			ipStr=address_v4().to_string(ec);
	}
	if (boost::iequals(ipStr,"localhost")||boost::iequals(ipStr,"loopback"))
	{
		if (v6)
			ipStr=address_v6::loopback().to_string(ec);
		else
			ipStr=address_v4::loopback().to_string(ec);
	}

	int port=0;
	if (!portStr.empty())
		port=atoi(portStr.c_str());
	if (v6)
		return std::pair<address,int>(address_v6::from_string(ipStr,ec),port);
	else
		return std::pair<address,int>(address_v4::from_string(ipStr,ec),port);
}

std::string ipport_to_string(address const& addr,int port)
{
	error_code ec;
	std::string straddr = addr.to_string(ec);
	std::string ret;
	ret.reserve(straddr.length()+10);
	if (addr.is_v6())
	{
		ret += '[';
		ret += straddr;
		ret += ']';
		ret += ':';
		ret += boost::lexical_cast<std::string>(port);
	}
	else
	{
		ret += straddr;
		ret += ':';
		ret += boost::lexical_cast<std::string>(port);
	}
	return ret;
}

NAMESPACE_END(p2engine);
