#include "Context.h"
#include "Utils.h"
#include "DatasetInfo.h"

std::vector<std::filesystem::path> Context::GetRawtTrafficPaths(const std::string& datasetName)
{
	std::filesystem::path rawTrafficFolder = std::string(datasetInfoMaps[datasetName].datasetFolder) + "/RAW";
	std::filesystem::path rawFolder(rawTrafficFolder);

	return GetFilesInSuffix(rawFolder, { ".pcap", ".pcapng" });
}

std::vector<std::filesystem::path> Context::GetFilesInSuffix(std::filesystem::path folder,
	std::initializer_list<std::string> args)
{
	std::vector<std::filesystem::path> files;

	if (!std::filesystem::exists(folder) || !std::filesystem::is_directory(folder))
	{
		return std::vector<std::filesystem::path>();
	}

	for (const std::filesystem::directory_entry& dirEntry : std::filesystem::recursive_directory_iterator(folder))
	{
		if (!IsFileInSuffix(dirEntry, args))
		{
			continue;
		}

		files.push_back(dirEntry.path());
	}

	return files;
}

bool Context::IsFileInSuffix(const std::filesystem::directory_entry& dirEntry, std::initializer_list<std::string> suffixes)
{
	if (!dirEntry.is_regular_file())
		return false;

	std::filesystem::path path = dirEntry.path();
	std::string extension = path.extension().string();

	for (auto& suffix : suffixes)
	{
		if (extension == suffix)
			return true;
	}
	
	return false;
}

std::string Context::GetScoreStemName()
{
	std::stringstream ss;

	ss << Config::Get().divisionParam.ToString()
		<< Config::Get().trainParam.ToString()
		<< Config::Get().classifyParam.ToString()
		<< Config::Get().filterParam.ToString();
	
	return ss.str();
}

KDevice& Context::GetDevice(std::string deviceName)
{
	auto deviceIter = allDevices.find(deviceName);
	if (deviceIter == allDevices.end())
	{
		std::cout << "wanna find" << deviceName;
		std::cout << "all devices size:" << allDevices.size() << std::endl;
		for (auto [deviceName, kdevice] : allDevices)
		{
			std::cout << deviceName << std::endl;
		}
		std::cout << "in get device" << std::endl;
		throw KException(KException::Code::DEVICE_NOT_FOUND);
	}

	return deviceIter->second;
}

KDevice& Context::GetDeviceByAddr(std::string addr)
{
	auto deviceIter = addressMap.find(addr);
	if (deviceIter == addressMap.end())
	{
		std::cout << "in get device by addr" << std::endl;
		throw KException(KException::Code::DEVICE_NOT_FOUND);
	}

	return deviceIter->second;
}

KDevice& Context::GetDeviceById(uint16_t devid)
{
	return this->deviceVec[devid];
}

void Context::ClearDevices()
{
	this->allDevices.clear();
	this->addressMap.clear();
	this->deviceVec.clear();
}
void Context::AddDevices(const std::string& datasetName)
{
	std::string nameIPString;
	std::ifstream stream(GetDeviceMappings(datasetName), std::ios_base::in);

	try {
		ValidateDeviceMappingStream(&stream);
	}
	catch (KException exception) {
		Klog::exception(exception);
		exit(-1);
	}

	while (getline(stream, nameIPString))
	{
		KDevice device = KDevice(nameIPString, datasetInfoMaps[datasetName].mappingIP);
		
		UpdateDeviceMap(device);
	}

	stream.close();
}

void Context::ValidateDeviceMappingStream(std::ifstream* stream)
{
	std::string firstLine;

	if (!stream->is_open())
	{
		goto openFail;
	}

	getline(*stream, firstLine);

	if (!firstLine.find("IP"))
	{
		goto typeFail;
	}

	return;

openFail:
	throw KException(KException::Code::DEVICE_MAPPING_OPEN_ERROR);

typeFail:
	throw KException(KException::Code::DEVICE_MAPPING_TYPE_ERROR);
}

void Context::UpdateDeviceMap(KDevice device)
{
	this->allDevices.insert({ device.GetFormattedName(), device });
	this->addressMap.insert({ device.GetAddr(), device });
	this->deviceVec.push_back(device);
}

std::vector<KDevice>& Context::GetAllDevices()
{
	return deviceVec;
}

std::filesystem::path Context::GetPacketDatasetPath(const std::string& datasetName)
{
	std::stringstream ss;
	std::string folder = datasetInfoMaps[datasetName].datasetFolder;
	ss << folder << "/" << datasetName << SUFFIX_PACKET_DATASET;

	return ss.str();
}

std::filesystem::path Context::GetPktDatasetStatPath(const std::string& datasetName)
{
	std::stringstream ss;
	std::string folder = datasetInfoMaps[datasetName].datasetFolder;
	ss << folder << "/PktStat.csv";
	return ss.str();
}

std::filesystem::path Context::GetBurstTrainStatPath(bool useTimeStamp, const std::string& outputFolder)
{
	std::stringstream ss;

	if (useTimeStamp)
	{
		ss << outputFolder << "/BurstStat"
			<< GetTimeSec(std::time(nullptr)) << ".csv";
	}
	else
	{
		ss << outputFolder << "/BurstStat.csv";
	}
	return ss.str();
}

std::string Context::GetTimerPath(std::string_view name, const std::string& datasetName)
{
	std::stringstream ss;
	std::string folder = datasetInfoMaps[datasetName].datasetFolder;

	ss << folder << "/" << name;
	return ss.str();
}

std::filesystem::path Context::GetBurstDatasetPath(const std::string& datasetName)
{
	std::stringstream ss;
	std::string folder = datasetInfoMaps[datasetName].datasetFolder;
	ss << folder << "/SlottedBurstDataset/"
		<< "【TIME_INTERVAL=" << Config::Get().divisionParam.TIMESLOT << "s】/"
		<< Config::Get().divisionParam.ToString() << SUFFIX_BURST_DATASET;

	return ss.str();
}

std::filesystem::path Context::GetDeviceMappings(const std::string& datasetName)
{
	std::string folder = datasetInfoMaps[datasetName].datasetFolder;
	return folder + "/mapping/device_mappings.csv";
}

void Context::OutputStringToFile(std::filesystem::path path, std::string str)
{
	PROFILE_FUNCTION();
	std::filesystem::create_directories(path.parent_path());

	std::ofstream ofs(path);
	
	if (!ofs.is_open())
	{
		throw KException(KException::Code::OPEN_ERROR);
	}
	
	ofs << str;
	ofs.close();
}