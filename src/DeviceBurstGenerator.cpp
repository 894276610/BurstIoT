#include "DeviceBurstGenerator.h"

void DeviceBurstGenerator::LongShortSplit(std::vector<KPacket*>* pktVec)
{
	std::vector<KPacket*> rawBurst;

	for (auto packet : *pktVec)
	{
		time_t floorDiff = 0;

		if (rawBurst.size() > 0)
		{
			floorDiff = FloorDuration(packet->timestamp, rawBurst.back()->timestamp);
		}

		if (floorDiff >= Config::Get().divisionParam.BURST_MAX_INTERVAL)
		{
			m_shortBurstVec.push_back(rawBurst);
			rawBurst = std::vector<KPacket*>();
		}
		else if (rawBurst.size() > 0 && FloorDuration(rawBurst.back()->timestamp,
								rawBurst.front()->timestamp) >
								Config::Get().divisionParam.BURST_MAX_DURATION)
		{
			m_longburstVec.push_back(rawBurst);
			rawBurst = std::vector<KPacket*>();
		}

		rawBurst.push_back(packet);
	}

	m_shortBurstVec.push_back(rawBurst);
}

void DeviceBurstGenerator::LongBurstSplit()
{
	SeparateBy5Tuple();
	MakeNormalBurstVec();
	MakeLongBurstVec();
}

void DeviceBurstGenerator::SeparateBy5Tuple()
{
	for (auto& burst : this->m_longburstVec)
	{
		for (auto& packet : burst)
		{
			this->m_flowMap[packet->hash5tuple].push_back(packet);
		}
	}
}

void DeviceBurstGenerator::MakeNormalBurstVec()
{	
	for (auto& raw : m_shortBurstVec)
	{	
		BurstVec burstVec = StrictToBursts(raw);
		this->m_burstVec.insert(this->m_burstVec.end(), burstVec.begin(), burstVec.end());		
	}
}

void DeviceBurstGenerator::MakeLongBurstVec()
{
	for (auto& [hash, rawPackets] : this->m_flowMap)
	{
		BurstVec burstVec = StrictToBursts(rawPackets);
		this->m_burstVec.insert(this->m_burstVec.end(), burstVec.begin(), burstVec.end());
	}
}

BurstVec DeviceBurstGenerator::StrictToBursts(std::vector<KPacket*> packetVec)
{
	BurstVec burstVec;
	time_t nearDiff = 0, farDiff = 0;
	uint8_t uniqueSize = 0;

	std::shared_ptr<Burst> burst = std::make_shared<Burst>(device);

	for (KPacket* packet : packetVec)
	{
		if (burst->firstPacketStamp.tv_sec == 0)
		{
			burst->SetFirstPacketStamp(*packet);
		}

		uniqueSize = burst->uniPktNum;

		if (burst->pktNum > 0)
		{
			nearDiff = FloorDuration(packet->timestamp, burst->lastPacketStamp);
			farDiff = FloorDuration(packet->timestamp, burst->firstPacketStamp);
		}

		if (uniqueSize >= Config::Get().divisionParam.BURST_MAX_UNISIG || 
			nearDiff >= Config::Get().divisionParam.BURST_MAX_INTERVAL ||
			farDiff >= Config::Get().divisionParam.BURST_MAX_DURATION)
		{
			//std::cout << (unsigned)uniqueSize << std::endl;
			burstVec.push_back(burst);
			burst = std::make_shared<Burst>(device);
		}

		burst->AddPacket(packet);
		burst->SetLastPacketStamp(*packet);
	}

	burstVec.push_back(burst);

	return burstVec;
}

void DeviceBurstGenerator::SortInOrder()
{
	std::sort(m_burstVec.begin(), m_burstVec.end(), [](const std::shared_ptr<Burst>& a, const std::shared_ptr<Burst>& b) {
		if (a->firstPacketStamp.tv_sec < b->firstPacketStamp.tv_sec) return true;
		if (a->firstPacketStamp.tv_sec > b->firstPacketStamp.tv_sec) return false;
		return a->firstPacketStamp.tv_nsec < b->firstPacketStamp.tv_nsec;
	});

	for (auto& burst : m_burstVec)
	{
		std::sort(burst->packetDetail.begin(), burst->packetDetail.end(),
			[](const std::pair<int, int>& a, const std::pair<int, int>& b) {
				return a.first < b.first;
			});
	}
}
