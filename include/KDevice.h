#pragma once
#include "pcppUsed/pcppCommon.h"
#include "Utils.h"
#include <string>

class KDevice
{
public:

	KDevice(std::string deviceAddressStr, bool mappingIP)
	{
		this->mappingIP = mappingIP;
		static uint16_t count = 0;
		this->devId = count++;

		auto [deviceRawName, addrString] = SplitPairStrByComma(deviceAddressStr);
		this->m_instanceName = MakeFormattedName(deviceRawName);
		this->m_typeLabel = MakeTypeLabel(this->m_instanceName);


		if (mappingIP)
		{
			m_ipaddr = pcpp::IPv4Address(addrString);
		}
		else
		{
			m_mac = pcpp::MacAddress(addrString);
		}
	}
		
	std::string MakeFormattedName(std::string name)
	{
		std::replace(name.begin(), name.end(), '/', '_');
		std::replace(name.begin(), name.end(), ' ', '_');
		std::replace(name.begin(), name.end(), ':', '_');

		return name;
	}

	std::string MakeTypeLabel(std::string fmtName)
	{
		return removeSuffix(fmtName);
	}

	std::string& GetFormattedName()
	{
		return m_instanceName;
	}

	std::string& GetTypeLabel()
	{
		return m_typeLabel;
	}

	std::string GetAddr()
	{
		if (this->mappingIP)
		{
			return m_ipaddr.toString();
		}
		else
		{
			return m_mac.toString();
		}
	}
public:

	bool mappingIP;
	uint16_t devId;
	std::string m_instanceName;
	std::string m_typeLabel;
	pcpp::MacAddress m_mac;
	pcpp::IPv4Address m_ipaddr;
};