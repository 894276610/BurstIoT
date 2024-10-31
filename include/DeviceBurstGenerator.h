#pragma once
#include <vector>
#include <memory>
#include <unordered_map>
#include "Burst.h"
#include <functional>
#include "Utils.h"

class DeviceBurstGenerator {

public:

	// 由 pcpp::RawPacketVector传入，省去自己析构
	DeviceBurstGenerator(std::vector<KPacket*>* pktVec, KDevice& device) :
		pktVec(pktVec), device(device)
	{
		LongShortSplit(pktVec);
		LongBurstSplit();
		SortInOrder();
	}

public:
	BurstVec GetBurstVec(){return m_burstVec;}
	
private:
	void LongShortSplit(std::vector<KPacket*>* pktVec);
	void LongBurstSplit();

	void SeparateBy5Tuple();
	void MakeNormalBurstVec();
	void MakeLongBurstVec();
	BurstVec StrictToBursts(std::vector<KPacket*> packetVec);

	void SortInOrder();
private:
	KDevice device;

	//pcpp::RawPacketVector* rawPackets;
	std::vector<KPacket*>* pktVec;

	std::vector<std::vector<KPacket*>>  m_longburstVec;
	std::vector<std::vector<KPacket*>>  m_shortBurstVec;

	std::unordered_map<std::size_t, std::vector<KPacket*>> m_flowMap;
	BurstVec m_burstVec;
};
