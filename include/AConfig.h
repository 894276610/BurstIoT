#pragma once
#include "DatasetInfo.h"
#include <unordered_map>
#include <sstream>

constexpr char SUFFIX_ABNORMAL_DETAIL[] = ".detail";
constexpr char SUFFIX_TABLE_RESULT[] = ".csv";
constexpr char SUFFIX_BURST_DATASET[] = ".burstDataset";
constexpr char SUFFIX_PACKET_DATASET[] = ".pktDataset";
constexpr char SUFFIX_BENCHMARK[] = ".json";
constexpr uint8_t EPOCH_TIMESLOT = 4; // 4 Hour
constexpr int STORE_MULTITHREAD = 10;

// 突发划分设置
struct DivisionParam {
	uint8_t BURST_MAX_INTERVAL = 2;			    // 突发的时间间隔在X秒钟之内
	uint8_t BURST_MAX_DURATION = 15;			// 一段突发在X秒钟之后触发截断
	uint8_t BURST_MAX_UNISIG = 50;			    // 一个突发中最多X个独特报文
	bool divideSmallBurstInFlow = false;
	int BURST_MAX_PACKET_INDEX = 60;
	uint16_t TIMESLOT = 1800;

	std::string ToString()
	{
		std::stringstream ss;
		ss << "Div"
			<< "(Slot=" << TIMESLOT << "s)"
			<< "(Intv=" << unsigned(BURST_MAX_INTERVAL) << "s)"
			<< "(Unq=" << unsigned(BURST_MAX_UNISIG) << ")"
			<< "(Dur=" << unsigned(BURST_MAX_DURATION) << "s)"
			<< "(PktIndex=" << BURST_MAX_PACKET_INDEX << ")"
			<< "(dvidshort=" << divideSmallBurstInFlow << ")";
		return ss.str();
	}
};

// 过滤无效时间片
struct FilterParam
{
	uint8_t MIN_BLOCK_PKTNUM = 1;
	uint8_t MIN_BURST_NUM = 1;
	
	std::string ToString()
	{
		std::stringstream ss;
		ss << "Filter"
			<< "(pktN=" << unsigned(MIN_BLOCK_PKTNUM) << ")"
			<< "(burstN=" << unsigned(MIN_BURST_NUM) << ")";
		return ss.str();
	}
};

// 训练过程参数
struct TrainParam {
	uint8_t TRAIN_PERCENT = 50;

	uint8_t TRAIN_SIMILAR_PACKETNUM = 0;
	uint8_t TRAIN_SIMILAR_UNIQUE = 0;
	bool ENABLE_SIMILAR = false;
	bool ENABLE_MERGE_IDENTICAL = true;
	bool POSITIVE_ONLY = false;
	
	float SIMILAR_DISTANCE = 0;

	std::string ToString()
	{
		std::stringstream ss;
		ss << "Train"
			<< "(Pcnt=" << unsigned(TRAIN_PERCENT) << ")"
			<< "(Mer=" << ENABLE_MERGE_IDENTICAL << ")"
			<< "(Sim=" << ENABLE_SIMILAR << ")";
			//<< "(SimPktNum=" << TRAIN_SIMILAR_PACKETNUM << "s)"
			//<< "(SimUnique=" << TRAIN_SIMILAR_UNIQUE << ")"
			//<< "(SimDistance=" << SIMILAR_DISTANCE << ")";

		return ss.str();
	}
};

struct TestParam
{
	uint8_t UNISIG_RANGE = 20;
	int PACKETNUM_RANGE = 60;
	float MIN_OUTPERFORM = 1.01;
	// max distance trigger = 100
	std::string ToString()
	{
		std::stringstream ss;
		ss << "Test"
			<< "(RngUni=" << unsigned(UNISIG_RANGE) << ")"
			<< "(RngPkt=" << PACKETNUM_RANGE << "s)"
			<< "(Otpfm=" << MIN_OUTPERFORM << ")";
		return ss.str();
	}
};

struct Config {

	DatasetInfo currentDataset;
	DivisionParam divisionParam;
	FilterParam filterParam;
	TrainParam trainParam;
	TestParam classifyParam;
	std::unordered_map<std::string, DatasetInfo> datasets;

    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    static Config& Get() {
        static Config instance; 
        return instance;
    }

private:
    Config() {
		for (auto& dataset : datasetConfigs)
		{
			datasets.insert({std::string(dataset.datasetName), dataset});
		}	

		//currentDataset = datasets["NCSU_21"];
		currentDataset = datasets["IOTBEHAV2021"];
	}
};