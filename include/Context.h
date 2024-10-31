#ifndef _CONTEXT_H_
#define _CONTEXT_H_

#include <stdio.h>
#include <fstream>
#include <vector>
#include <initializer_list>  
#include <filesystem>
#include "AConfig.h"
#include "Burst.h"
#include "boostSerial/boostSerialize.h"
#include "KDevice.h"
#include "Timer.h"
#include "Utils.h"


class Context
{
public:
	std::vector<KDevice>& GetAllDevices();

	std::filesystem::path GetPacketDatasetPath(const std::string& datasetName);

	std::filesystem::path GetPktDatasetStatPath(const std::string& datasetName);
	std::filesystem::path GetBurstTrainStatPath(bool useTimeStamp, const std::string& datasetName);

	std::string GetTimerPath(std::string_view name, const std::string& datasetName);

	std::filesystem::path GetBurstDatasetPath(const std::string& datasetName);

	std::filesystem::path GetDeviceMappings(const std::string& datasetName);

	std::string GetScoreStemName();

	KDevice& GetDevice(std::string deviceName);
	KDevice& GetDeviceByAddr(std::string addr);
	KDevice& GetDeviceById(uint16_t devid);
	void ClearDevices();
	void AddDevices(const std::string& datasetName);
	void UpdateDeviceMap(KDevice device);

	template<class T>
	void Serialize(const T& obj, std::filesystem::path outFile);

	template<class T>
	void LoadBin(T& obj, std::filesystem::path inputFile);
	
	std::vector<std::filesystem::path> GetRawtTrafficPaths(const std::string& datasetName);

	std::vector<std::filesystem::path> GetFilesInSuffix(std::filesystem::path folder,
		std::initializer_list<std::string> args);

	void OutputStringToFile(std::filesystem::path path, std::string str);

private:
	bool IsFileInSuffix(const std::filesystem::directory_entry& dirEntry, std::initializer_list<std::string> suffixes);

	void ValidateDeviceMappingStream(std::ifstream* stream);

public:
	~Context() {}

private:
	std::unordered_map <std::string, KDevice> allDevices;
	std::unordered_map<std::string, KDevice> addressMap;
	std::vector<KDevice> deviceVec;

};

template<class T>
inline void Context::Serialize(const T& obj, std::filesystem::path outFile)
{
	PROFILE_FUNCTION();
	std::filesystem::create_directories(outFile.parent_path());

	std::ofstream ofs(outFile.string(), std::ios::binary | std::ios::out);
	std::ofstream oftext(outFile.string() + ".txt",  std::ios::out);

	if (ofs.is_open() && oftext.is_open())
	{
		typename boost::archive::binary_oarchive oa(ofs);
		typename boost::archive::text_oarchive otext(oftext);

		oa << obj;
		otext << obj;
	}
	else
	{
		throw KException(KException::Code::OUTPUT_ERROR);
	}
}

template<class T>
inline void Context::LoadBin(T& obj, std::filesystem::path inputFile)
{
	PROFILE_FUNCTION();
	std::ifstream ifs(inputFile.string(), std::ios::binary | std::ios::in);

	if (ifs.is_open())
	{
		typename boost::archive::binary_iarchive ia(ifs);
		ia >> obj;
	}
	else
	{
		throw KException(KException::Code::OUTPUT_ERROR);
	}
}

class ContextFactory {
public:
	static Context& Get()
	{
		static Context* instance = new Context();
		return *instance;
	}

private:
	ContextFactory() = default;
	~ContextFactory() = default;
	ContextFactory(const ContextFactory&) = delete;
	ContextFactory& operator=(const ContextFactory&) = delete;
};

#endif