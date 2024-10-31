#pragma once
#include <vector>
#include <sstream>
#include <unordered_map>
struct DatasetInfo
{
	bool mappingMac = false;
	bool mappingIP = true;
	char datasetName[18] = "UNSW";
	char datasetFolder[80] = "F:/UNSW";
};

extern std::vector<DatasetInfo> datasetConfigs;
extern std::unordered_map<std::string, DatasetInfo> datasetInfoMaps;

struct DeviceStat
{
	std::string instanceName;
	long totalSend = 0;
	long totalRecv = 0;

	DeviceStat(std::string name): 
		instanceName(name){ }

	std::string ToString()
	{
		std::stringstream ss;
		ss << instanceName << ","
			<< totalSend << ","
			<< totalRecv;
		return ss.str();
	}

};

struct DatasetStat
{
	long totalPktNum;
	long usefulPktNum;
	std::vector<DeviceStat> devStat;

	std::string ToString()
	{
		std::stringstream ss;
		ss << "name,totalSend,totalRecv" << std::endl;

		for (size_t i = 0; i < devStat.size(); i++)
		{
			ss << devStat[i].ToString() << std::endl;
		}

		ss << "totalPktNum, usefulPktNum" << std::endl;
		ss << totalPktNum << "," << usefulPktNum << std::endl;
		return ss.str();
	}
};