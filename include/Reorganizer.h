#pragma once

#include <unordered_map>
#include <filesystem>
#include <ctime>

#include "Context.h"
#include "pcppUsed/pcppCommon.h"
#include "Timer.h"
#include "Burst.h"
#include "Utils.h"

#include "DeviceBurstGenerator.h"
#include "BurstDataset.h"

#include "workflow/WFTaskFactory.h"
#include "workflow/WFFacilities.h"

class Reorganizer
{
protected:
	
	void OpenReader(pcpp::IFileReaderDevice* device);
	void CloseReader(pcpp::IFileReaderDevice* reader);
};

class Converter : public Reorganizer
{
public:
	Converter(const std::string& datasetName);
	std::unordered_map<uint16_t, BurstBlocks> GenBurstDatasetContent(const std::string& datasetName);


public:
	std::unordered_map<uint16_t, BurstBlocks> PktDatasetCvtoBursts(std::vector<KPacket>* packetDataset);

private:
	void RawCvtoPktDataset();
	void StoreKPackets(int index, std::vector<pcpp::RawPacket*> rawVec, const std::string& datasetName);
	bool AddPacket(const std::string& addr, bool isSource, pcpp::RawPacket* pRawPacket);
	void AddPacket(uint16_t deviceId, time_t slotId, KPacket* packet);
	bool IsIoTAddr(const std::string& ipStr);


	void PktDatasetCvtoBursts();
	BurstVec CreateBurstVec(std::vector<KPacket*>* packetVectors, KDevice& device);
	void ReleasePktDataset(std::vector<KPacket>* packetDataset);

	void SplitSlots();
	void SplitSlots(size_t start, size_t end);
	void SplitSlot(uint16_t deviceId, time_t slotId);

	void GarbageCollection(size_t fileNum);

private:

	std::string datasetName;

	// raw to pktDataset
	std::vector<pcpp::RawPacketVector*> rawPacketVec;
	PacketDataset* pkDataset = nullptr;

	std::vector<WFFacilities::WaitGroup*> storeCounter;
	WFFacilities::WaitGroup* gcTask = nullptr;

	// pktDataset toburst
	std::mutex mapMutex;
	std::unordered_map<short, std::map<time_t, std::vector<KPacket*>*>> deviceTimeRawVecMap;

	std::unordered_map<uint16_t, std::mutex> blockMutexs;
	std::unordered_map<uint16_t, BurstBlocks> burstDatasetContent;

	std::mutex addressMapMutex;
	std::unordered_map<std::string, KDevice> addressMap;	
};