#pragma once
#include <unordered_map>
#include <time.h>
#include <sstream>
#include <mutex>
#include "Burst.h"
#include "BurstGroupPolicy.h"

//class BurstGroupPolicy;

struct ScoreSheet {
	
	int missNum = 0;
	size_t TP, FN, FP;
	float recall, precision, f1, pktAvgNum = 0;
	std::vector<int> missIndexVector;
	std::vector<int> correctVector;
	std::vector<int> wrongVector;
	std::vector<std::string> resultVector;

	ScoreSheet() : TP(0), FN(0), FP(0), recall(0), precision(0), f1(0) {};

	float Precision() { precision = (float)TP / (TP + FP); return precision; }
	float Recall() { recall = (float)TP / (TP + FN); return recall; }
	float F1() { f1 = 2 * Precision() * Recall() / (float)(Precision() + Recall()); return f1; }
	
	std::string ToCsvString();
	std::string MissIndexString();
	std::string ToResultIndex();

};

class Score {

public:
	Score(const std::string& outputFolder) : outputFolder(outputFolder) {}

	void Clear();
	void AddPrediction(std::string trueLabel, BurstGroupPolicy predict, int index);
	void AddPktAvgNum(uint16_t deviceAddr, float num);
	std::string TrueFalseMissEnum();
	static std::string GetScoreOutpath(const std::string& datasetName);
	std::string GetScoreOutpathByDataset(const std::string& datasetName);
	std::string IndexTable();
	std::string WrongBurstGroupDetails();
	
	void OutputToFile();
	void OutputStringToFile(std::filesystem::path path, const std::string& str);


	
private:
	void AddMiss(std::string trueLabel, int missIndex);

	static std::string GetScoreStem();
	std::string GetSummaryIndex();
private:

	std::string outputFolder;

	std::mutex mutex;
	std::unordered_map<std::string, ScoreSheet> sheet;
		
	std::mutex debugMutex;
	std::vector<BurstGroupPolicy> m_ErrBurstGroups;
};