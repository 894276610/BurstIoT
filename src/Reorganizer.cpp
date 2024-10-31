#include "Reorganizer.h"

Converter::Converter(const std::string& datasetName)
{
	this->datasetName = datasetName;
	pkDataset = new PacketDataset();

	for (auto& device : ContextFactory::Get().GetAllDevices())
	{
		if (datasetInfoMaps[datasetName].mappingIP)
		{
			this->addressMap.insert({ device.m_ipaddr.toString(), device });
			this->blockMutexs[device.devId];
		}
		else if (datasetInfoMaps[datasetName].mappingMac)
		{
			this->addressMap.insert({ device.m_mac.toString(), device });
			this->blockMutexs[device.devId];
		}
		pkDataset->RegistDevice(device.m_instanceName);
	}
}

std::unordered_map<uint16_t, BurstBlocks> Converter::GenBurstDatasetContent(const std::string& datasetName)
{

	RawCvtoPktDataset();
	PktDatasetCvtoBursts();
	return this->burstDatasetContent;
}

void Converter::GarbageCollection(size_t fileNum)
{
	gcTask = new WFFacilities::WaitGroup(1);

	for (size_t i = 0; i < fileNum; i++)
	{
		storeCounter.push_back(new WFFacilities::WaitGroup(STORE_MULTITHREAD));
	}

	for (size_t i = 0; i < fileNum; i++)
	{
		storeCounter[i]->wait();

		rawPacketVec[i]->clear();
		delete rawPacketVec[i];
		rawPacketVec[i] = nullptr;

	}

	for (size_t i = 0; i < fileNum; i++)
	{
		delete storeCounter[i];
		storeCounter[i] = nullptr;
	}

	storeCounter.clear();
	gcTask->done();
}


void Converter::RawCvtoPktDataset()
{
	PROFILE_FUNCTION();
	
	std::vector<std::filesystem::path> paths = ContextFactory::Get().GetRawtTrafficPaths(this->datasetName);
	
	rawPacketVec.resize(paths.size(), nullptr);
	
	{
		PROFILE_SCOPE("Reading Raw Packets");

		pcpp::IFileReaderDevice* reader = nullptr;
		pcpp::RawPacketVector* rawVec = nullptr;

		WFGoTask* gcTask = WFTaskFactory::create_go_task("store",
			&Converter::GarbageCollection, this, paths.size());
		gcTask->start();

		for (int i = 0; i < paths.size(); i++)
		{
			reader = pcpp::IFileReaderDevice::getReader(paths[i].string());
			OpenReader(reader);

			rawPacketVec[i] = new pcpp::RawPacketVector();		
			rawVec = rawPacketVec[i];

			if(reader->getNextPackets(*rawVec, -1))
			{
				std::cout << "complete one read" << std::endl;
				pkDataset->stat.totalPktNum += rawVec->size();
				std::vector<std::vector<pcpp::RawPacket*>> splittedVec = splitVector(rawVec->m_Vector, rawVec->size()/ (STORE_MULTITHREAD - 1));

				for (auto smallrawVec : splittedVec)
				{
					WFGoTask* processTask = WFTaskFactory::create_go_task("store",
						&Converter::StoreKPackets, this, i, smallrawVec, this->datasetName);
					processTask->start();
				}

				for (size_t j = 0; j < STORE_MULTITHREAD - splittedVec.size(); j++)
				{
					storeCounter[i]->done();
				}
			}
			else
			{
				for (size_t j = 0; j < STORE_MULTITHREAD; j++)
				{
					storeCounter[i]->done();
				}	
			}

			CloseReader(reader);	
			rawVec = nullptr;
		}
	}

	gcTask->wait();
	pkDataset->Sort();

	pkDataset->OutputStat();
	pkDataset->Serialize();
	
}

void Converter::StoreKPackets(int index, std::vector<pcpp::RawPacket*> rawVec, const std::string& datasetName)
{
	PROFILE_SCOPE("StoreKPackets ");
	
	const bool mappingIP = datasetInfoMaps[datasetName].mappingIP;
	const bool mappingMac = datasetInfoMaps[datasetName].mappingMac;
	bool srcSucc, dstSucc;
	pcpp::IPv4Layer* layerV4 = nullptr;
	pcpp::EthLayer* layerEth = nullptr;
	std::string srcAddr, dstAddr;
	pcpp::Packet parsedPacket;

	for (pcpp::RawPacket* pRawPacket : rawVec)
	{	
		parsedPacket = pcpp::Packet(pRawPacket);

		if ( mappingIP && !parsedPacket.isPacketOfType(pcpp::IPv4) )
		{
			continue;
		}
		else if ( mappingMac && !parsedPacket.isPacketOfType(pcpp::Ethernet) )
		{
			continue;
		}

		if (mappingIP)
		{
			layerV4 = parsedPacket.getLayerOfType<pcpp::IPv4Layer>();

			srcAddr = layerV4->getSrcIPv4Address().toString();
			dstAddr = layerV4->getDstIPv4Address().toString();
		}
		else if (mappingMac)
		{
			layerEth = parsedPacket.getLayerOfType<pcpp::EthLayer>();

			srcAddr = layerEth->getSourceMac().toString();
			dstAddr = layerEth->getDestMac().toString();
		}
		
		srcSucc = AddPacket(srcAddr, true, pRawPacket);
		dstSucc = AddPacket(dstAddr, false, pRawPacket);

		if (srcSucc || dstSucc)
		{
			pkDataset->stat.usefulPktNum++;
		}
	}

	storeCounter[index]->done();
}

bool Converter::AddPacket(const std::string& addr, bool isSource, pcpp::RawPacket* pRawPacket)
{
	if (!IsIoTAddr(addr))
	{
		return false;
	}

	uint16_t devId = ContextFactory::Get().GetDeviceByAddr(addr).devId;
	int len = pRawPacket->getFrameLength();
	short signedLen = len > 3000 ? 3000 : len;
	uint32_t hash = pcpp::hash5Tuple(&pcpp::Packet(pRawPacket));
	timespec timestamp = pRawPacket->getPacketTimeStamp();

	if (!isSource)
	{
		signedLen = -signedLen;
		pkDataset->stat.devStat[devId].totalRecv++;
	}
	else
	{
		pkDataset->stat.devStat[devId].totalSend++;
	}

	
	pkDataset->AddPacket( devId, signedLen, hash, timestamp);
 
	return true;
}

bool Converter::IsIoTAddr(const std::string& ipStr)
{
	std::unique_lock<std::mutex> lock(addressMapMutex);
	if (this->addressMap.find(ipStr) == addressMap.end())
	{
		return false;
	}
	return true;
}

void Converter::PktDatasetCvtoBursts()
{
	PROFILE_FUNCTION();
	for (KPacket& packet : pkDataset->dataset)
	{
		time_t slotId = packet.timestamp.tv_sec / Config::Get().divisionParam.TIMESLOT;
		
		AddPacket(packet.deviceId, slotId, &packet);
	}
	
	SplitSlots();
	pkDataset->Clear();
	delete pkDataset;
	pkDataset = nullptr;
}

std::unordered_map<uint16_t, BurstBlocks> Converter::PktDatasetCvtoBursts(std::vector<KPacket>* packetDataset)
{
	PROFILE_SCOPE("PktDatasetCvtoBursts");
	for (KPacket& packet : *packetDataset)
	{
		time_t slotId = packet.timestamp.tv_sec / Config::Get().divisionParam.TIMESLOT;

		AddPacket(packet.deviceId, slotId, &packet);
	}

	SplitSlots();

	ReleasePktDataset(packetDataset);

	return this->burstDatasetContent;
}

void Converter::AddPacket(uint16_t deviceId, time_t slotId, KPacket* packet)
{
	std::unique_lock<std::mutex> lock(mapMutex);
	if (deviceTimeRawVecMap[deviceId][slotId] == nullptr)
	{
		deviceTimeRawVecMap[deviceId][slotId] = new std::vector<KPacket*>();
	}

	deviceTimeRawVecMap[deviceId][slotId]->push_back(packet);
}

void Converter::SplitSlots()
{
	PROFILE_SCOPE("SplitSlots");

	std::set<size_t> timeSlots;

	for (auto& [deviceAddr, timeRawVec] : this->deviceTimeRawVecMap)
	{
		for (auto& [timeSlot, rawVec] : timeRawVec)
		{
			timeSlots.insert(timeSlot);
		}
	}	

	SplitSlots(*timeSlots.begin(), *timeSlots.rbegin() + 1);
	this->deviceTimeRawVecMap.clear();
}

void Converter::SplitSlots(size_t start, size_t end)
{
	int waitCount = end - start;

	for (auto [deviceId, timeRawVec] : this->deviceTimeRawVecMap)
	{
		for (size_t slotId = start; slotId < end; slotId++)
		{
			SplitSlot(deviceId, slotId);
		}
	}

}

void Converter::SplitSlot(uint16_t deviceId, time_t slotId)
{
	std::vector<KPacket*>* pktVec = this->deviceTimeRawVecMap[deviceId][slotId];
	
	if (pktVec == nullptr)
	{
		return;
	}

	BurstBlock burstBlock = CreateBurstVec(pktVec, ContextFactory::Get().GetDeviceById(deviceId));

	std::unique_lock<std::mutex> lock(this->blockMutexs[deviceId]);
	burstDatasetContent[deviceId].reserve(100);
	burstDatasetContent[deviceId].push_back(burstBlock);
	lock.unlock();

	this->deviceTimeRawVecMap[deviceId][slotId] = nullptr;
	this->deviceTimeRawVecMap[deviceId].erase(slotId);
}

BurstVec Converter::CreateBurstVec(std::vector<KPacket*>* pktVec, KDevice& device)
{
	DeviceBurstGenerator deviceBurstGenerator(pktVec, device);
	return deviceBurstGenerator.GetBurstVec();
}

void Converter::ReleasePktDataset(std::vector<KPacket>* packetDataset)
{
	PROFILE_FUNCTION();
	packetDataset->clear();
	delete packetDataset;
	packetDataset = nullptr;
}

void Reorganizer::CloseReader(pcpp::IFileReaderDevice* reader)
{
	if (reader)
	{
		reader->close();
		reader = nullptr;
	}
}

void Reorganizer::OpenReader(pcpp::IFileReaderDevice* reader)
{
	if (reader == NULL)
	{
		throw KException(KException::Code::PCAP_FORMAT_ERROR);
	}

	if (!reader->open())
	{
		throw KException(KException::Code::PCAP_CANNOT_OPEN);
	}
}