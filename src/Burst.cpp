#include "Burst.h"
#include "AConfig.h"
#include <map>
#include <Utils.h>
#include <BurstCache.h>
#include <cmath>

Burst::Burst(KDevice& device) :
	instanceLabel(device.GetFormattedName()),
	pktNum(0),uniPktNum(0),
	firstPacketStamp({ 0,0 }), lastPacketStamp({ 0,0 })
{}

Burst::Burst(std::vector<std::shared_ptr<Burst>> burstVec) :
	pktNum(0), uniPktNum(0),
	firstPacketStamp({ 0,0 }), lastPacketStamp({ 0,0 })
{
	if (burstVec.size())
	{
		instanceLabel = burstVec[0]->instanceLabel;
	}

	for (auto& burst : burstVec)
	{
		for (auto& [packetSig, num] : burst->packetDetail)
		{
			for (int i = 0; i < num; i++)
			{
				AddSig(packetSig);
			}
		}
	}
}

Burst::Burst() :
	instanceLabel(""), pktNum(0),
	firstPacketStamp({ 0,0 }), lastPacketStamp({ 0,0 })
{}

float Burst::Distance(const std::shared_ptr<Burst> burst2) const
{

	if ( burst2->pktNum > this->pktNum +120 || this->pktNum > burst2->pktNum +120)
	{
		return 1;
	}

	std::array<bool, 101> arr_blk2_check = { false };
	float distance = 0;
	int cnt1 = 0, cnt2 = 0, cnt2Remain = 0;
	bool match = false;

	for (uint8_t i = 0; i < uniPktNum; i++)
	{
		cnt1 = (packetDetail)[i].second;
		cnt2 = 0;
		const int sig = (packetDetail)[i].first;
		match = false;

		for (uint8_t j = 0; j < burst2->uniPktNum; j++)
		{
			if (arr_blk2_check[j] == false && sig == burst2->packetDetail[j].first)
			{
				cnt2 = (burst2->packetDetail)[j].second;
				arr_blk2_check[j] = true;
				match = true;
				break;
			}
		}

		if (match)
		{
			distance += pow((sqrt(cnt1 / (float)pktNum) - sqrt(cnt2 / (float)burst2->pktNum)), 2);
			//distance += std::abs(cnt1 / (float)pktNum - cnt2 / (float)burst2->pktNum);
		}
		else
		{
			distance += cnt1 / (float)pktNum;
		}
	}

	// check blk2 remainer
	for (uint8_t i = 0; i < burst2->uniPktNum; i++)
	{
		if (!arr_blk2_check[i])
		{
			cnt2Remain += (burst2->packetDetail)[i].second;	
		}
	}

	distance += cnt2Remain / (float)burst2->pktNum;

	if (this->pktNum > burst2->pktNum)
	{
		return sqrt(distance * this->pktNum / burst2->pktNum);
	}
	else
	{
		return sqrt(distance * burst2->pktNum / this->pktNum);
	}
	//return sqrt(distance * this->pktNum / 2.0 * burst2->pktNum);
	//
	//return distance;
}

float Burst::Distance(const Burst& burst2) const
{
	std::array<bool, 101> arr_blk2_check = { false };
	float distance = 0;
	float p1, p2;
	bool match = false;

	if (burst2.pktNum > this->pktNum + 100 || this->pktNum > burst2.pktNum + 100)
	{
		return 1;
	}

	for (uint8_t i = 0; i < uniPktNum; i++)
	{
		p1 = (packetDetail)[i].second / (float)pktNum;
		p2 = 0;
		const int sig = (packetDetail)[i].first;
		match = false;

		for (uint8_t j = 0; j < burst2.uniPktNum; j++)
		{
			if (arr_blk2_check[j] == false && sig == burst2.packetDetail[j].first)
			{
				p2 = (burst2.packetDetail)[j].second / (float)burst2.pktNum;
				arr_blk2_check[j] = true;
				match = true;
				break;
			}
		}

		if (match)
		{
			distance += pow((sqrt(p1) - sqrt(p2)), 2);
		}
		else
		{
			distance += p1;
		}
	}

	// check blk2 remainer
	for (uint8_t i = 0; i < burst2.uniPktNum; i++)
	{
		if (!arr_blk2_check[i])
		{
			distance += (burst2.packetDetail)[i].second / (float)burst2.pktNum;
		}
	}

	return sqrt(distance / 2.0);
}

const std::string Burst::ToString() const
{
	std::stringstream ss;

	ss << "Device=" << instanceLabel
		<< ",Uni=" << unsigned(uniPktNum)
		<< ",PktNum=" << pktNum
		<< ",Detail=";

	for (int i = 0 ; i < uniPktNum; i++)
	{
		ss << "(";
		ss << packetDetail[i].first;
		ss << "X" << packetDetail[i].second;
		ss << ")";
	}
	
	ss << std::endl;
	return ss.str();
}

void Burst::AddPacket( KPacket* packet)
{
	short signature = packet->signedLen;

    AddSig(signature);
}

void Burst::SetFirstPacketStamp(const KPacket& packet)
{
	this->firstPacketStamp = packet.timestamp;
}

void Burst::SetLastPacketStamp(const KPacket& packet)
{
	this->lastPacketStamp = packet.timestamp;
}

bool Burst::EarlierThan(const std::shared_ptr<Burst> burst2) const
{
	if (this->firstPacketStamp.tv_sec < burst2->firstPacketStamp.tv_sec)
	{
		return true;
	}
	else if (this->firstPacketStamp.tv_sec > burst2->firstPacketStamp.tv_sec)
	{
		return false;
	}
	else if (this->firstPacketStamp.tv_nsec < burst2->firstPacketStamp.tv_nsec)
	{
		return true;
	}

	return false;
}

void Burst::AddSig(const short signedLen)
{
	if (!PiggyBack(signedLen))
	{
		packetDetail.push_back({ signedLen , 1 });
		pktNum++;
		uniPktNum++;	
	}
}

bool Burst::PiggyBack(const short sig)
{
	for (uint8_t i = 0; i < uniPktNum; i++)
	{
		if (packetDetail[i].first == sig)
		{
			packetDetail[i].second++;
			pktNum++;
			return true;
		}
	}

	return false;
}

bool Burst::GetDirectionByIP(const std::string& ip, pcpp::RawPacket* rawPacket)
{
	pcpp::Packet parsedPacket(rawPacket);

	if (!parsedPacket.isPacketOfType(pcpp::IPv4)) {
		throw KException(KException::Code::NOT_IPV4_PACKET);
	}

	pcpp::IPv4Layer* ipv4_layer = parsedPacket.getLayerOfType<pcpp::IPv4Layer>();

	std::string ip1 = ipv4_layer->getDstIPv4Address().toString();
	std::string ip2 = ipv4_layer->getSrcIPv4Address().toString();
	if (ip == ipv4_layer->getDstIPv4Address().toString())
	{
		return true;
	}
	else if (ip == ipv4_layer->getSrcIPv4Address().toString())
	{
		return false;
	}

	throw KException(KException::Code::OTHER_DEVICE_PACKET);
}

bool Burst::GetDirectionByMac(const std::string& addressStr, pcpp::RawPacket* rawPacket)
{
	pcpp::Packet parsedPacket(rawPacket);

	if (!parsedPacket.isPacketOfType(pcpp::Ethernet))
	{
		throw KException(KException::Code::NOT_ETHERNET_PACKET);
	}

	pcpp::EthLayer* ethLayer = parsedPacket.getLayerOfType<pcpp::EthLayer>();
	std::string dstMac = ethLayer->getDestMac().toString();
	std::string srcMac = ethLayer->getSourceMac().toString();

	if (addressStr == dstMac)
	{
		return true;
	}
	else if (addressStr == srcMac)
	{
		return false;
	}
	
	throw KException(KException::Code::OTHER_DEVICE_PACKET);
}

void Burst::Validate() const
{
	int count = 0;
	int uniquePakcetNum = 0;

	if (instanceLabel.size() == 0)
	{
		throw KException(KException::Code::UNKNOWN_BURST_DEVICE);
	}

	for (auto& [packet, num] : packetDetail)
	{
		uniquePakcetNum++;
		count += num;
	}

	if (uniquePakcetNum != uniPktNum)
	{
		throw KException(KException::Code::BURST_UNI_NUMBER_MISMATCH);
	}

	if (count != this->pktNum)
	{
		throw KException(KException::Code::BURST_NUMBER_MISMATCH);
	}

	if (uniPktNum > Config::Get().divisionParam.BURST_MAX_UNISIG)
	{
		throw KException(KException::Code::BURST_UNI_NUMBER_OVERFLOW);
	}
}

time_t Burst::FloorDuration()
{
	time_t sec, nsec;
	sec = this->lastPacketStamp.tv_sec - this->firstPacketStamp.tv_sec;

	nsec = this->lastPacketStamp.tv_nsec - this->firstPacketStamp.tv_nsec;
	if (nsec < 0)
	{
		return sec - 1;
	}

	return sec;
}

bool Burst::Similar(std::shared_ptr<Burst> other) const {

	std::unordered_set<int> sigSet;
	if (uniPktNum != other->uniPktNum)
	{
		return false;
	}

	if (pktNum / Config::Get().trainParam.TRAIN_SIMILAR_PACKETNUM !=
		other->pktNum / Config::Get().trainParam.TRAIN_SIMILAR_PACKETNUM)
	{
		return false;
	}

	// 所有的 uniquePacket Num 值相同
	for (auto& [packetSig, num] : packetDetail)
	{
		sigSet.insert(packetSig);
	}

	int sigNum = sigSet.size();

	for (auto& [packetSig, num] : other->packetDetail)
	{
		sigSet.insert(packetSig);
	}

	int sigNumAfter = sigSet.size();

	if (sigNum != sigNumAfter)
	{
		return false;
	}

	if (Distance(other) < Config::Get().trainParam.SIMILAR_DISTANCE)
	{
		return true;
	}
}

