#pragma once
#include "Burst.h"
#include <mutex>

class BurstGroupPolicy
{
	friend class KBurstClassifier;

public:
	BurstGroupPolicy() {}

	BurstGroupPolicy(BurstGroup testGroup) :testGroup(testGroup) {}
	void AddBurstPrediction(std::pair<float, BurstVec>);
	std::string GenPrediction();

private:
	void AddPredictName(std::set<std::string> predictNames);

	int GetGroupPktNum();

private:
	BurstGroup testGroup;
	std::vector<std::set<std::string>> vPredictNames;
	std::vector<BurstVec> vPredictTrains;
	std::vector<float> vDistance;

	std::unordered_map<std::string, float> predictBucket;
	std::multimap<float, std::string> rankedGroup;
	std::string prediction;
	//Burst mergedBurst;

	static std::mutex numAvgMutex;
	static std::unordered_map<uint16_t, float> addrAvgPktMap;

	static std::mutex levelMutex;
	static std::unordered_map<uint16_t, BurstVec> blockLevelDistribution;


public:
	std::string ToString();
};