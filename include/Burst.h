#pragma once

#include <sstream>
#include <string>
#include <time.h>
#include <vector>
#include <memory>
#include <unordered_map>
#include <algorithm>
#include <array>

#include <unordered_set>
#include <map>
#include <set>

#include "AConfig.h"
#include "pcppUsed/pcppCommon.h"

#include "Klog.h"
#include "KException.h"
#include "KDevice.h"

struct KPacket
{
	uint16_t deviceId;
	short signedLen;
	uint32_t hash5tuple;
	timespec timestamp;

	KPacket() {}
	KPacket(short deviceId, short signedLen, uint32_t hash5tuple, timespec timestamp) :
		deviceId(deviceId), signedLen(signedLen), hash5tuple(hash5tuple), timestamp(timestamp) {}

	bool operator<(const KPacket& k2) {
		if (timestamp.tv_sec < k2.timestamp.tv_sec)
		{
			return true;
		}
		else if (timestamp.tv_sec > k2.timestamp.tv_sec)
		{
			return false;
		}
		else if (timestamp.tv_nsec < k2.timestamp.tv_nsec)
		{
			return true;
		}

		return false;
	}
};




class Burst {
public:
	uint8_t uniPktNum = 0;
	int pktNum = 0;
	std::string instanceLabel;
	timespec firstPacketStamp;
	timespec lastPacketStamp;

	std::vector<std::pair<short, int>> packetDetail;

public:
	Burst();
	Burst(KDevice& device);
	Burst(std::vector<std::shared_ptr<Burst>> burstVec);
	float Distance(const std::shared_ptr<Burst> burst2) const;
	float Distance(const Burst& burst2) const;
	const std::string ToString() const;

	void AddPacket(KPacket* packet);
	
	bool EarlierThan(const std::shared_ptr<Burst> burst2) const;

	time_t FloorDuration();

public:

	void SetFirstPacketStamp(const KPacket& packet);

	void SetLastPacketStamp(const KPacket& packet);

private:
	bool GetDirectionByIP(const std::string& ip, pcpp::RawPacket* rawPacket);
	bool GetDirectionByMac(const std::string& macStr, pcpp::RawPacket* rawPacket);
	void AddSig(const short sig);
	bool PiggyBack(const short sig);
	
public:
	void Validate() const;
	bool Similar(std::shared_ptr<Burst> other) const;

	bool operator<(const Burst& other) const
	{
		if (firstPacketStamp.tv_sec < other.firstPacketStamp.tv_sec)
		{
			return true;
		}
		else if (firstPacketStamp.tv_sec > other.firstPacketStamp.tv_sec)
		{
			return false;
		}
		else if (firstPacketStamp.tv_nsec < other.firstPacketStamp.tv_nsec)
		{
			return true;
		}

		return false;
	}

	bool operator==(const Burst& other) const {
		
		if (uniPktNum != other.uniPktNum)
		{
			return false;
		}
		
		if (pktNum != other.pktNum)
		{
			return false;
		}

		int sum1 = 0, sum2 = 0;
		int dirsum1 = 0, dirsum2 = 0;
		for (int i = 0; i < uniPktNum; i++)
		{
			if (packetDetail[i].first != other.packetDetail[i].first)
			{
				return false;
			}
			
			if (packetDetail[i].second != other.packetDetail[i].second)
			{
				return false;
			}
		}

		return true;
	}

	template<class Archive>
	void serialize(Archive& ar, const unsigned int version) {
		ar& instanceLabel;
		ar& pktNum;
		ar& uniPktNum;
		ar& firstPacketStamp;
		ar& lastPacketStamp;
		ar& packetDetail;
	}
};

template<>
struct std::hash<Burst>
{
	std::size_t operator()(const Burst& p) const {

		std::size_t seed = std::hash<int>{}(p.uniPktNum);
		int sum = 0;
		for (int i = 0; i < p.uniPktNum; i++)
		{
			//sum += p.packetDetail[i].first.len;
			seed ^= std::hash<int>{}(p.packetDetail[i].first) << (p.packetDetail[i].second%30);
		}

		seed ^= std::hash<uint32_t>{}(p.pktNum);
		//seed ^= std::hash<int>{}(sum);

		return seed;
	}
};

namespace boost {
	namespace serialization {

		template<class Archive>
		void serialize(Archive& ar, timespec& t, const unsigned int version) {
			ar& t.tv_sec;
			ar& t.tv_nsec;
		}

		template<class Archive>
		void serialize(Archive& ar, KPacket& t, const unsigned int version) {
			ar& t.deviceId;
			ar& t.signedLen;
			ar& t.hash5tuple;
			ar& t.timestamp;
		}

	} // namespace serialization  
} // namespace boost 

typedef std::vector<std::shared_ptr<Burst>> BurstVec;
typedef std::vector<std::shared_ptr<Burst>> BurstGroup;
typedef std::vector<BurstGroup> BurstGroups;

typedef std::vector<std::shared_ptr<Burst>> BurstBlock; // 描述固定时间内的 若干突发
typedef std::vector<BurstBlock> BurstBlocks; 

typedef std::vector<std::vector<BurstVec>> StructuredTrainData;
//BURST_MAX_PACKET_INDEX>, BURST_MAX_UNISIG