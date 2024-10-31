#pragma once

#include <map>
#include <set>
#include <algorithm>
#include <filesystem>

#include "Burst.h"
#include "Score.h"
#include "Context.h"
#include "BurstCache.h"

#include "workflow/WFTaskFactory.h"
#include "workflow/WFFacilities.h"

class BurstGroupPolicy;

struct BurstStat
{
	uint16_t deviceId = 0;
	size_t totalUniBurstNum = 0;
	std::array<size_t, 500> counter = {0};

	std::string ToString()
	{
		std::string deviceName = ContextFactory::Get().GetDeviceById(deviceId).m_instanceName;
		
		std::stringstream ss;
		ss << deviceId << "," << deviceName;
		ss << "," << totalUniBurstNum;
		
		for (size_t i = 0; i < 500; i++)
		{
			ss << ",";
			ss << counter[i];
		}
		
		return ss.str();
	}

};

class KBurstClassifier {

friend class BurstGroupPolicy;

public:
	KBurstClassifier(std::string& outputFolder);

	void Train(std::unordered_map<uint16_t, BurstGroups>* trainset);
	Score& Predict(std::unordered_map<uint16_t,BurstGroups>* testset);
	Score& GetScore() { return score; }

private:
	void TrainOneDevice(uint16_t deviceId, BurstGroups burstGroups);
	int BlockAvgPktNum(BurstGroups& burstGroups);
	std::unordered_map<std::shared_ptr<Burst>, int> MergeStrictHash(BurstGroups& burstGroups);
	void MergeIdenticalKeys(std::unordered_map<std::shared_ptr<Burst>, int>* map);
	std::shared_ptr<Burst> FindItenticalKeysInMap(std::unordered_map<std::shared_ptr<Burst>, int>* map, std::shared_ptr<Burst> burst);
	std::shared_ptr<Burst> FindUltraSimilarBurst(std::unordered_map<std::shared_ptr<Burst>, int>* inputBurst, std::shared_ptr<Burst> burst);
	
	void OutputBurstTrainStat();

	void PredictGroups(const BurstGroups deviceGroups, int);
	BurstGroupPolicy PredictBurstGroup(const BurstGroup& burstGroup);
	const std::pair<float, BurstVec> PredictBurst(std::shared_ptr<Burst> testBurst);

public:
	Score score;
private:

	std::string outputFolder;

	std::mutex trainMutex;
	StructuredTrainData train;

	std::mutex statsMutex;
	std::vector<BurstStat> burstStats;

	WFFacilities::WaitGroup* waitPrediction = nullptr;
	WFFacilities::WaitGroup* waitTrain = nullptr;

};