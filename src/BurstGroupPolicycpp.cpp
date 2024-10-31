#include "BurstGroupPolicy.h"
#include "Context.h"

std::unordered_map<uint16_t, float> BurstGroupPolicy::addrAvgPktMap;
std::unordered_map<uint16_t, BurstVec> BurstGroupPolicy::blockLevelDistribution;

std::mutex BurstGroupPolicy::numAvgMutex;
std::mutex BurstGroupPolicy::levelMutex;

void BurstGroupPolicy::AddBurstPrediction(std::pair<float,BurstVec> predictTrains)
{
	int index = vPredictTrains.size();
	int burstPacketNum = testGroup[index]->pktNum;
	int uniPktNum = testGroup[index]->uniPktNum;

	auto& [minDistance, predictVec] = predictTrains;

	this->vPredictTrains.push_back(predictVec);
	this->vDistance.push_back(minDistance);

	
	std::set<std::string> predictNames;

	for (auto predict : predictVec)
	{
		predictNames.insert(predict->instanceLabel);
	}

	AddPredictName(predictNames);

	if (minDistance > 0.5)
	{
		return;
	}

	for (auto item : predictNames)
	{
		this->predictBucket[item] += 1.0 * log(uniPktNum + 1) /
			(predictNames.size() * (sqrt(minDistance) + 1));
	}
	// 修改结果未准
	/*
	for (auto item : predictTrains)
	{
		this->predictBucket[item->instanceLabel] += 1.0 / predictTrains.size();
	}
	*/
}

void BurstGroupPolicy::AddPredictName(std::set<std::string> predictNames)
{
	this->vPredictNames.push_back(predictNames);
}

int BurstGroupPolicy::GetGroupPktNum()
{
	int groupPktNum = 0;
	for (auto& group : testGroup)
	{
		groupPktNum += group->pktNum;
	}

	return groupPktNum;
}

std::string BurstGroupPolicy::GenPrediction()
{
	for (const auto& [label, number] : predictBucket)
	{
		if (label.compare("unreliable") == 0)
		{
			continue;
		}

		rankedGroup.insert({ number, label });
	}

	if (rankedGroup.size() == 0)
	{
		prediction = "unreliable";

		// 将所有设置为1 再次选择
		for (auto& device : ContextFactory::Get().GetAllDevices())
		{
			rankedGroup.insert({ 1, removeSuffix(device.m_instanceName) });
		}
		
	}

	if (rankedGroup.size() > 1 && rankedGroup.rbegin()->first - (++rankedGroup.rbegin())->first <= Config::Get().classifyParam.MIN_OUTPERFORM)
	{
		float maxScore = rankedGroup.rbegin()->first - Config::Get().classifyParam.MIN_OUTPERFORM;

		std::string currPrediction = "";
		//int groupPacketNum = this->GetGroupPktNum();
		float minDistance = INT_MAX;
		Burst mergedBurst = Burst(testGroup);

		for (auto iter = rankedGroup.rbegin(); iter != rankedGroup.rend(); iter++)
		{
			auto [devScore, devName] = *iter;

			if (devScore < maxScore)
			{
				break;
			}

			uint16_t devid = ContextFactory::Get().GetDevice(devName).devId;
			
			/*float distance = abs(mergedBurst.pktNum - this->addrAvgPktMap[addr]);

			if (distance < minDistance)
			{
				prediction = devName;
				minDistance = distance;
			}*/
		
			for (auto& trainMergedBurst : this->blockLevelDistribution[devid])
			{
				
				int distance = std::abs(mergedBurst.pktNum - trainMergedBurst->pktNum);
				//float distance = mergedBurst.BigBurstDistance(*trainMergedBurst);

				if (distance < minDistance)
				{
					prediction = devName;
					minDistance = distance;
				}
			}
			

		}
		return prediction;
	}

	prediction = rankedGroup.rbegin()->second;
	return prediction;
}

std::string BurstGroupPolicy::ToString()
{
	std::stringstream ss;
	int i = 0;
	
	for (; i < testGroup.size();  i++)
	{
		bool contains = false;
		for (auto& name : vPredictNames[i])
		{
			if (removeSuffix(name) == removeSuffix(testGroup[i]->instanceLabel))
			{
				contains = true;
				break;
			}
		}

		if (contains)
		{	
			continue;
		}

		ss << "index=" << i << std::endl
			<< "test" << testGroup[i]->ToString();

		ss << "prediction names";

		for (auto& name : vPredictNames[i])
		{
			ss << "," << name;
		}

		ss << std::endl;
		ss << "Attention prediction bursts:" << std::endl;
		ss << "Distance:" << vDistance[i] << std::endl;

		for (auto& burst : vPredictTrains[i])
		{
			ss << burst->ToString();
		}
		ss << std::endl;
	}

	//ss << "total packet num:" << mergedBurst.pktNum << std::endl;
	ss << "total burst num:" << testGroup.size() << std::endl;
	ss << "prediction rank:" << std::endl;
	
	for (auto& bucket : rankedGroup)
	{
		ss << bucket.second << bucket.first << std::endl;
	}
	ss << "final prediction:" << prediction << std::endl;

	if (prediction != "unreliable")
	{
		uint16_t id;
		try {
			id = ContextFactory::Get().GetDevice(prediction).devId;
		}
		catch (KException e)
		{
			Klog::exception(e);
			exit(-1);
		}

		ss << "prediction pktNum:" << this->addrAvgPktMap[id] << std::endl;
	}
	
	ss << "--------------" << std::endl;

	return ss.str();
}
