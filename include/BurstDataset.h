#ifndef _BURST_DATASET_H
#define _BURST_DATASET_H
#include <fstream>
#include <filesystem>
#include <random>
#include "Context.h"

class PacketDataset
{
public:
	std::string datasetName;
	std::mutex pktDatasetMutex;
	std::vector<KPacket> dataset;
	DatasetStat stat;

public:
	PacketDataset()
	{
		dataset.reserve(1000000);
	}

	bool Valid();
	void RegistDevice(const std::string& name);
	void AddPacket(short deviceId, short signedLen, uint32_t hash5tuple, timespec timestamp);
	void Sort();
	void OutputStat();
	void Serialize();
	void Load();
	void Clear();
};

class BurstDataset
{
public:

	BurstDataset(std::unordered_map<uint16_t, BurstGroups> rawMap, bool shuffle) :rawMap(rawMap) {
		TrainTestSplit(shuffle);
		Serialize();
	}

	BurstDataset(const std::string& datasetName) : seed(0), name(datasetName)
	{
		Load();
		TrainTestSplit();
	}

	BurstDataset(unsigned seed) : seed(seed) {	
		Load();
		TrainTestSplit();
	}

	BurstDataset(std::filesystem::path datasetPath) {
		ContextFactory::Get().LoadBin(rawMap, datasetPath);
		TrainTestSplit();
	}

	void Serialize();
	void Load();
	void Load(const std::string&);
	void Clear();

protected:		
	void TrainTestSplit(bool shuffle = false);
	std::filesystem::path GetBurstDatasetPath(const std::string& datasetName);

public:
	unsigned seed;
	std::string name;

	std::unordered_map<uint16_t, BurstGroups> rawMap;
	std::unordered_map<uint16_t, BurstGroups> trainset;
	std::unordered_map<uint16_t, BurstGroups> testset;
};

#endif