#include "BurstDataset.h"

void BurstDataset::TrainTestSplit(bool shuffle)
{
	PROFILE_FUNCTION();
	std::default_random_engine e(seed);

	if (Config::Get().trainParam.POSITIVE_ONLY)
	{
		//TODO
	}

	for (auto& [deviceId, burstGroup] : rawMap)
	{
		if(shuffle)
		{ 
			std::cout << "shuffle" << std::endl;
			std::shuffle(burstGroup.begin(), burstGroup.end(), e);
		}
		else
		{
			std::stable_sort(burstGroup.begin(), burstGroup.end(), [](const BurstGroup& a, const BurstGroup& b) {
				return (*a.front()) < (*b.front());
			});
		}
		
		size_t trainNum = std::ceil(burstGroup.size() * Config::Get().trainParam.TRAIN_PERCENT / 100.0);

		BurstGroups trainGroups(burstGroup.begin(), burstGroup.begin() + trainNum);
		BurstGroups testGroups(burstGroup.begin() + trainNum, burstGroup.end());

		trainset.insert({ deviceId, trainGroups });
		testset.insert({ deviceId, testGroups });
	}
}

std::filesystem::path BurstDataset::GetBurstDatasetPath(const std::string& datasetName)
{
	return ContextFactory::Get().GetBurstDatasetPath(datasetName);
}

void BurstDataset::Serialize()
{
	PROFILE_FUNCTION();
	
	std::filesystem::path datasetPath = GetBurstDatasetPath(this->name);
	ContextFactory::Get().Serialize(rawMap, datasetPath);
}

void BurstDataset::Load(const std::string& datasetName)
{
	PROFILE_FUNCTION();
	std::filesystem::path datasetPath = GetBurstDatasetPath(datasetName);

	if (!std::filesystem::exists(datasetPath))
	{
		throw KException(KException::Code::DATASET_PATH_NOT_EXIST);
	}

	ContextFactory::Get().LoadBin(rawMap, datasetPath);
}

void BurstDataset::Load()
{
	PROFILE_FUNCTION();
	std::filesystem::path datasetPath = GetBurstDatasetPath(this->name);
	// check the path
	if (!std::filesystem::exists(datasetPath))
	{
		throw KException(KException::Code::DATASET_PATH_NOT_EXIST);
	}

	ContextFactory::Get().LoadBin(rawMap, datasetPath);
}

void BurstDataset::Clear()
{
	if (this->rawMap.size() != 0)
	{
		this->rawMap.clear();
	}
	else
	{
		this->trainset.clear();
		this->testset.clear();
	}
}

bool PacketDataset::Valid()
{
	for (size_t i = 0; i < dataset.size(); i++)
	{
		if (dataset[i].deviceId >= stat.devStat.size())
		{
			std::cout << dataset[i].deviceId << "invalid id" << stat.devStat.size();
			__debugbreak();
			return false;
		}

		if (dataset[i].signedLen > 3000 || dataset[i].signedLen < -3000)
		{
			std::cout << dataset[i].deviceId << "invalid signedLen" << dataset[i].signedLen;
			__debugbreak();
			return false;
		}

		if (dataset[i].timestamp.tv_nsec == 0 && dataset[i].timestamp.tv_sec == 0)
		{
			std::cout << "invalid time" << i << dataset[i].timestamp.tv_nsec << dataset[i].timestamp.tv_sec;
			__debugbreak();
			return false;
		}
		
	}

	return true;
}

void PacketDataset::RegistDevice(const std::string& name)
{
	stat.devStat.emplace_back(name);
}

void PacketDataset::AddPacket(short deviceId, short signedLen, uint32_t hash5tuple, timespec timestamp)
{
	std::unique_lock<std::mutex> lock(pktDatasetMutex);
	dataset.emplace_back(deviceId, signedLen, hash5tuple, timestamp);
}

void PacketDataset::Sort()
{

	if (Valid())
	{
		std::unique_lock<std::mutex> lock(pktDatasetMutex);
		std::sort(dataset.begin(), dataset.end());
	}
	else
	{
		throw KException(KException::Code::INVALID_PKT);
	}
}

void PacketDataset::OutputStat()
{
	ContextFactory::Get().OutputStringToFile(ContextFactory::Get().GetPktDatasetStatPath(this->datasetName), stat.ToString());
}

void PacketDataset::Serialize()
{
	std::unique_lock<std::mutex> lock(pktDatasetMutex);
	ContextFactory::Get().Serialize(dataset, ContextFactory::Get().GetPacketDatasetPath(this->datasetName));
}

void PacketDataset::Load()
{

}

void PacketDataset::Clear()
{
	dataset.clear();
}
