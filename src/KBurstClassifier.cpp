#include "KBurstClassifier.h"
#include  "Timer.h"
#include "BurstGroupPolicy.h"
#include "workflow/WFTaskFactory.h"
#include "workflow/WFFacilities.h"

KBurstClassifier::KBurstClassifier(std::string& outputFolder): outputFolder(outputFolder), score(outputFolder)
{
	std::vector<BurstVec> temp(Config::Get().divisionParam.BURST_MAX_PACKET_INDEX);
	train.resize(Config::Get().divisionParam.BURST_MAX_UNISIG, temp);
}

void KBurstClassifier::Train(std::unordered_map<uint16_t, BurstGroups>* trainset)
{
	PROFILE_SCOPE("Training Time");
	this->waitTrain = new WFFacilities::WaitGroup(trainset->size());

	this->burstStats.resize(trainset->size());

	for (const auto& [deviceId, burstGroups] : *trainset)
	{
		WFGoTask* processTask = WFTaskFactory::create_go_task("Train",
				&KBurstClassifier::TrainOneDevice, this, deviceId, burstGroups);
		processTask->start();
	}

	this->waitTrain->wait();

	delete this->waitTrain;
	this->waitTrain = nullptr;

	//OutputBurstTrainStat();
}

void KBurstClassifier::TrainOneDevice(uint16_t deviceId, BurstGroups burstGroups)
{
	std::array<size_t, 500> pktNumCount = { 0 };
	size_t uniBurstCount = 0;
	std::unordered_map<std::shared_ptr<Burst>, int> trainMap;
	{
		float middlePktNum = BlockAvgPktNum(burstGroups);
		std::unique_lock<std::mutex> lock(BurstGroupPolicy::numAvgMutex);
		BurstGroupPolicy::addrAvgPktMap.insert({ deviceId, middlePktNum });
	}

	for (const auto& burstGroup : burstGroups)
	{
		std::shared_ptr<Burst> mergedBurst = std::make_shared<Burst>(burstGroup);

		std::unique_lock<std::mutex> lock(BurstGroupPolicy::levelMutex);
		BurstGroupPolicy::blockLevelDistribution[deviceId].push_back(mergedBurst);
	}

	trainMap = MergeStrictHash(burstGroups);

	if (Config::Get().trainParam.ENABLE_MERGE_IDENTICAL)
	{
		MergeIdenticalKeys(&trainMap);
	}

	int packetNumIndex = 0;
	for (auto& [burst, num] : trainMap)
	{
		burst->Validate();

		packetNumIndex = std::min(burst->pktNum, 500);
		pktNumCount[packetNumIndex - 1]++;
		uniBurstCount++;

		packetNumIndex = std::min(burst->pktNum, Config::Get().divisionParam.BURST_MAX_PACKET_INDEX);
		std::unique_lock<std::mutex> lock(trainMutex);
		train[burst->uniPktNum -1 ][packetNumIndex -1].push_back(burst);
	}

	{
		std::unique_lock<std::mutex> lock(this->statsMutex);
		this->burstStats[deviceId] = { deviceId, uniBurstCount, pktNumCount };
	}

	waitTrain->done();
}

int KBurstClassifier::BlockAvgPktNum(BurstGroups& burstGroups)
{
	std::vector<int> numVec;
	int groupSize = burstGroups.size();

	for (auto& block : burstGroups)
	{
		int pktNum = 0;

		for (auto& burst : block)
		{
			pktNum += burst->pktNum;
		}

		numVec.push_back(pktNum);
	}

	std::stable_sort(numVec.begin(), numVec.end());


	if (groupSize % 2 == 1) {
		return numVec[groupSize / 2];
	}
	else {
		return (numVec[groupSize / 2 - 1] + numVec[groupSize / 2]) / 2.0;
	}
}

std::unordered_map<std::shared_ptr<Burst>, int> KBurstClassifier::MergeStrictHash(BurstGroups& burstGroups)
{
	std::unordered_map<std::shared_ptr<Burst>, int> trainMap;
	std::unordered_map<std::size_t, std::shared_ptr<Burst>> hashMap;
	std::size_t hash = 0;

	for (const auto& burstGroup : burstGroups)
	{
		for (const auto& burst : burstGroup)
		{
			hash = std::hash<Burst>{}(*burst);
			if (hashMap[hash] == 0)
			{
				trainMap.insert({ burst, 1 });
				hashMap[hash] = burst;
			}
			else
			{
				std::shared_ptr<Burst> existingBurst = hashMap[hash];
				trainMap[existingBurst]++;
			}
		}
	}

	return trainMap;
}

void KBurstClassifier::MergeIdenticalKeys(std::unordered_map<std::shared_ptr<Burst>, int>* map)
{

	std::vector<std::pair<std::shared_ptr<Burst>, int>> vec(map->begin(), map->end());
	map->clear();

	std::stable_sort(vec.begin(), vec.end(), [](const std::pair<std::shared_ptr<Burst>, int>& a, 
		const std::pair<std::shared_ptr<Burst>, int>& b) {
		return a.second > b.second;
	});

	for (auto& [burst, num] : vec)
	{
		if (map->size() > 1000)
		{
			break;
		}

		if (burst->uniPktNum > 10 || burst->pktNum > 6)
		{
			std::shared_ptr<Burst> bIdentical = FindItenticalKeysInMap(map, burst);
			std::shared_ptr<Burst> bUSimilar = FindUltraSimilarBurst(map, burst);

			if (bIdentical == nullptr && bUSimilar == nullptr)
			{
				map->insert({ burst, num });
				continue;
			}
			
			if (bUSimilar != nullptr)
			{
				(*map)[bUSimilar] += num;
				continue;
			}

			if (bIdentical != nullptr && bIdentical->Distance(burst) < 0.7)
			{
				(*map)[bIdentical] += num;
				continue;
			}	
		}

		map->insert({ burst, num });
		continue;
		
	}
}

// TODO identical keys 或许不止一个
std::shared_ptr<Burst> KBurstClassifier::FindItenticalKeysInMap(std::unordered_map<std::shared_ptr<Burst>, int>* map, std::shared_ptr<Burst> burst)
{
	bool equal = true;
	for (auto& [b,num] : *map)
	{
		if (b->packetDetail.size() != burst->packetDetail.size())
		{
			continue;
		}
		
		equal = true;
		for (size_t i = 0; i < b->packetDetail.size(); i++)
		{
			if (burst->packetDetail[i].first != b->packetDetail[i].first)
			{
				equal = false;
				break;
			}
		}

		if (equal)
		{
			return b;
		}
	}

	return nullptr;
}


std::shared_ptr<Burst> KBurstClassifier::FindUltraSimilarBurst(std::unordered_map<std::shared_ptr<Burst>, int>* map, std::shared_ptr<Burst> burst)
{
	for (auto& [b,num] : *map)
	{
		if (b->Distance(burst) < 0.2)
		{
			return b;
		}	
	}

	return nullptr;
}


void KBurstClassifier::OutputBurstTrainStat()
{
	std::stringstream ss;
	size_t totaluniBurstNum = 0;
	size_t ge500totaluniBurstNum = 0;
	ss << "deviceId, deviceName, uniBurstNum";

	for (size_t i = 0; i < 500; i++)
	{
		ss << ",burstPktNum" << i + 1;
	}
	ss << std::endl;

	for (size_t i = 0; i < burstStats.size(); i++)
	{
		ss << burstStats[i].ToString() << std::endl;
		totaluniBurstNum += burstStats[i].totalUniBurstNum;
		ge500totaluniBurstNum += burstStats[i].counter[499];
	}

	ss << "totaluniBurstNum, " << totaluniBurstNum << std::endl;
	ss << "ge500totaluniBurstNum, " << ge500totaluniBurstNum; 

	try {
		ContextFactory::Get().OutputStringToFile(ContextFactory::Get().GetBurstTrainStatPath(false, outputFolder), ss.str());
	}
	catch (KException e)
	{
		ContextFactory::Get().OutputStringToFile(ContextFactory::Get().GetBurstTrainStatPath(true, outputFolder), ss.str());
	}
	
	burstStats.clear();
}



Score& KBurstClassifier::Predict(std::unordered_map<uint16_t,BurstGroups>* testset)
{
	PROFILE_FUNCTION();
	int taskNum = 0;
	int epochSlotNum = EPOCH_TIMESLOT * 3600 / Config::Get().divisionParam.TIMESLOT;

	for (auto& [deviceName, burstGroups] : *testset)
	{
		int remain = burstGroups.size() % epochSlotNum;
		taskNum += burstGroups.size() / epochSlotNum;
		taskNum = remain > 0 ? taskNum + 1 : taskNum;
	}
	
	waitPrediction = new WFFacilities::WaitGroup(taskNum);

	for (auto& [deviceId, burstGroups] : *testset)
	{
		int index = 0;

		score.AddPktAvgNum(deviceId, BurstGroupPolicy::addrAvgPktMap[deviceId]);

		for (const auto& smallGroups : splitVector(burstGroups, epochSlotNum))
		{
			WFGoTask* processTask = WFTaskFactory::create_go_task("PedictBurstGroups",
				&KBurstClassifier::PredictGroups, this, smallGroups, index);
			processTask->start();
			index += epochSlotNum;
		}
	}

	waitPrediction->wait();
	delete waitPrediction;
	waitPrediction = nullptr;

	return score;
}

void KBurstClassifier::PredictGroups(const BurstGroups deviceGroups, int index)
{
	std::string name = deviceGroups.at(0).at(0)->instanceLabel + std::to_string(deviceGroups.at(0).size());
	InstrumentationTimer timer = InstrumentationTimer(name.c_str());
	//PROFILE_SCOPE(name);
	const std::string trueDevice = deviceGroups.at(0).at(0)->instanceLabel;

	for (const BurstGroup& burstGroup : deviceGroups)
	{
		int burstGroupPktNum = 0;

		for (const auto& burst : burstGroup)
		{
			burstGroupPktNum += burst->pktNum;
		}
		// burstGroup.size() < Config::Get().filterParam.MIN_BURST_NUM ||
		if ( burstGroupPktNum < Config::Get().filterParam.MIN_BLOCK_PKTNUM)
		{
			//std::cout << "filter one burstSlot" << trueDevice << std::endl;
			continue;
		}

		BurstGroupPolicy predictDevice = PredictBurstGroup(burstGroup);

		score.AddPrediction(trueDevice, predictDevice, index++);	
	}

	waitPrediction->done();
}

BurstGroupPolicy KBurstClassifier::PredictBurstGroup(const BurstGroup& burstGroup)
{
	BurstGroupPolicy debugBurstGroup(burstGroup);

	for (auto& pBurst : burstGroup)
	{
		debugBurstGroup.AddBurstPrediction(PredictBurst(pBurst));
	}

	return debugBurstGroup;
}

const std::pair<float,BurstVec> KBurstClassifier::PredictBurst(std::shared_ptr<Burst> testBurst)
{
	float min_distance = 100, distance;
	std::unique_lock<std::mutex> lock(BurstCacheFactory::Get().GetMutex(testBurst));
	auto& [nearestDistance, nearestVec] = BurstCacheFactory::Get().Query(testBurst);
	
  	if (nearestVec.size() != 0)
	{
		return { nearestDistance, nearestVec };
	}

	int iMin = std::max((uint8_t)testBurst->uniPktNum - Config::Get().classifyParam.UNISIG_RANGE, 0);
	int iMax = std::min((uint8_t)(testBurst->uniPktNum + Config::Get().classifyParam.UNISIG_RANGE), Config::Get().divisionParam.BURST_MAX_UNISIG);
	int jMin = std::max(testBurst->pktNum - Config::Get().classifyParam.PACKETNUM_RANGE, 0);
	jMin = std::min(jMin, Config::Get().divisionParam.BURST_MAX_PACKET_INDEX - 1);
	int jMax = std::min(testBurst->pktNum + Config::Get().classifyParam.PACKETNUM_RANGE, Config::Get().divisionParam.BURST_MAX_PACKET_INDEX);

	for (int i = iMin; i < iMax; i++)
	{
		for (int j = jMin; j < jMax; j++)
		{
			for (const auto trainBurst : train[i][j])
			{
 				distance = testBurst->Distance(trainBurst);

				if (distance < min_distance)
				{
					nearestVec.clear();
					nearestVec.push_back(trainBurst);
					min_distance = distance;
				}
				else if (distance == min_distance)
				{
					nearestVec.push_back(trainBurst);
				}
			}	
		}
	}

	nearestDistance = min_distance;
	return { min_distance, nearestVec };
}