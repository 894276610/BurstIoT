#include "IoTBurst.h"

IoTBurst::IoTBurst()
{
	pcpp::Logger::getInstance().suppressLogs();
}

// 实验1: 验证本系统在不同的标注率下的准确度
// 准备测试的和流划分相关的自变量，测试他们的利用程度
// 自变量1: 采样率 1 2 3 4 ... 80
// 自变量2: 时间片 100 200 300 600 900 1200 1500 1800
// 自变量3: 时间间隔 1 2 3 
// 自变量4: 最大独特报文数量 50
// 自变量5: 最大时长 15s

void IoTBurst::LabelRateExperiment()
{	
	//std::vector<int> sampleRateVector = { 1,2,3,4,5,8,10,20,30,40,50 };
	std::vector<int> sampleRateVector = { 20 };
	std::vector<int> slotTimes = { 1800 };
	std::vector<int> intervals = { 2 };
	std::vector<int> maxUniques = GetMaxUniques();
	std::vector<int> maxDuration = GetMaxDurations();
	std::vector<std::string> datasetNames = { "UNSW201620" };

	std::vector<std::vector<int>> argArrays = { sampleRateVector, slotTimes, intervals ,maxUniques, maxDuration };
	Combiner combiner(argArrays);

	while (!combiner.isFinished()) {
		std::vector<int> cfg = combiner.getNext();
		for (auto& datasetName : datasetNames)
		{
			SingleExperiment(cfg, datasetName);
		}	
	}	
}

void IoTBurst::NCSULongTermExperiment()
{
	Config::Get().divisionParam.TIMESLOT = 1800;
	ContextFactory::Get().AddDevices("NCSU2020");
	ContextFactory::Get().AddDevices("NCSU2021");

	BurstDataset* ncsu2020 = GetBurstDataset("NCSU2020");
	BurstDataset* ncsu2021 = GetBurstDataset("NCSU2021");

	std::string outputFolder = "F:/NCSULong";
	KBurstClassifier* iotBurstClassifier = new KBurstClassifier(outputFolder);

	Instrumentor::Get().BeginSession("Train and Prediction", Score::GetScoreOutpath(outputFolder) + SUFFIX_BENCHMARK);
	iotBurstClassifier->Train(&ncsu2020->rawMap);
	Score& score = iotBurstClassifier->Predict(&ncsu2021->rawMap);
	score.OutputToFile();
	Instrumentor::Get().EndSession();

	ncsu2020->Clear();
	ncsu2021->Clear();
	delete ncsu2020;
	delete ncsu2021;
	ncsu2020 = nullptr;
	ncsu2021 = nullptr;

	delete iotBurstClassifier;
	iotBurstClassifier = nullptr;
	BurstCacheFactory::Get().Clear();
}

void IoTBurst::SingleExperiment(std::vector<int> cfg, const std::string& datasetName)
{	
	ContextFactory::Get().AddDevices(datasetName);

	Config::Get().trainParam.TRAIN_PERCENT = cfg[0];
	Config::Get().divisionParam.TIMESLOT = cfg[1];
	Config::Get().divisionParam.BURST_MAX_INTERVAL = cfg[2];
	Config::Get().divisionParam.BURST_MAX_UNISIG = cfg[3];
	Config::Get().divisionParam.BURST_MAX_DURATION = cfg[4];
	
	BurstDataset* burstDataset = GetBurstDataset(datasetName);
	std::string outputFolder = datasetInfoMaps[datasetName].datasetFolder;
	KBurstClassifier* iotBurstClassifier = new KBurstClassifier(outputFolder);
	
	Instrumentor::Get().BeginSession("Train and Prediction", Score::GetScoreOutpath(outputFolder) + SUFFIX_BENCHMARK);
	iotBurstClassifier->Train(&burstDataset->trainset);	
	Score& score = iotBurstClassifier->Predict(&burstDataset->testset);
	score.OutputToFile();
	Instrumentor::Get().EndSession();
	
	burstDataset->Clear();
	delete burstDataset;
	burstDataset = nullptr;

	delete iotBurstClassifier;
	iotBurstClassifier = nullptr;
	BurstCacheFactory::Get().Clear();
}

std::vector<int> IoTBurst::GetSampleRates()
{
	std::vector<int> sampleRates;
	for (int i = 20; i < 22; i+=10)
	{
		sampleRates.emplace_back(i);
	}
	return sampleRates;
}

std::vector<int> IoTBurst::GetSlotTimes()
{
	return { 300, 900, 1800 };

	//return { 100, 200, 300, 600, 900, 1800 };
}

std::vector<int> IoTBurst::GetIntervals()
{
	return {2};
	//return { 1, 2, 3 };

}

std::vector<int> IoTBurst::GetMaxUniques()
{
	return { 50 };

	//return { 20, 30, 40, 50, 60, 70 };
}

std::vector<int> IoTBurst::GetMaxDurations()
{
	return { 15 };

	//return { 5, 8, 10, 13, 15, 20, 30, 60 };
}

std::vector<std::string> IoTBurst::GetDatasetNames()
{
	return { "UNSW2016", "UNSW2018", "NEUSI2019", "NEUKI2019", "IOTBEHAV2021" };
}

BurstDataset* IoTBurst::GetBurstDataset(const std::string& datasetName)
{
	BurstDataset* burstDataset = nullptr;

	if (std::filesystem::exists(ContextFactory::Get().GetBurstDatasetPath(datasetName)))
	{
		Instrumentor::Get().BeginSession("GetBurstDataset", ContextFactory::Get().GetTimerPath("Load_burstDataset.json", datasetName));
		burstDataset = FromBurstDataset(datasetName);
		Instrumentor::Get().EndSession();

		return burstDataset;
	}
	
	converter = new Converter(datasetName);

	if (std::filesystem::exists(ContextFactory::Get().GetPacketDatasetPath(datasetName)))
	{
		Instrumentor::Get().BeginSession("GetBurstDataset", ContextFactory::Get().GetTimerPath("Load_pktDataset.json", datasetName));
		burstDataset = FromPktDataset(datasetName);
		Instrumentor::Get().EndSession();

		return burstDataset;
	}

	try {	
		Instrumentor::Get().BeginSession("GetBurstDataset", ContextFactory::Get().GetTimerPath("Load_rawPackets.json", datasetName));
		burstDataset = FromRawPackets(datasetName);
		Instrumentor::Get().EndSession();
		
		return burstDataset;
	}
	catch (KException e)
	{
		Klog::exception(e);
	}
	catch (std::exception e)
	{
		std::cout << e.what() << std::endl;
		exit(-1);
	}	
}

BurstDataset* IoTBurst::GetBurstFullDataset(const std::string& datsetName)
{
	BurstDataset* burstDataset = nullptr;

}
BurstDataset* IoTBurst::FromBurstDataset(const std::string& datasetName)
{
	PROFILE_SCOPE("Load from burstDataset");
	return new BurstDataset(datasetName);
}

BurstDataset* IoTBurst::FromPktDataset(const std::string& datasetName)
{
	PROFILE_SCOPE("Load from pktDataset");

	std::vector<KPacket>* packetDataset = new std::vector<KPacket>();
	ContextFactory::Get().LoadBin(*packetDataset, ContextFactory::Get().GetPacketDatasetPath(datasetName));
	return new BurstDataset(converter->PktDatasetCvtoBursts(packetDataset), false);
}

BurstDataset* IoTBurst::FromRawPackets(const std::string& datasetName)
{
	PROFILE_SCOPE("Load from Raw Packets");
	return new BurstDataset(converter->GenBurstDatasetContent(datasetName), false);
}
