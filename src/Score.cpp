#include "Score.h"
#include "Context.h"
#include <fstream>
#include <filesystem>
#include "BurstGroupPolicy.h"

std::unordered_map<std::string, ScoreSheet> sheet;

std::string ScoreSheet::ToCsvString()
{
	std::stringstream ss;

	ss << TP << ","
		<< FN << ","
		<< FP << ","
		<< Recall() << ","
		<< Precision() << ","
		<< F1() << ","
		<< missNum << ","
		<< pktAvgNum;

	return ss.str();
}

std::string ScoreSheet::MissIndexString()
{
	std::stringstream ss;
	for (auto index : missIndexVector)
	{
		ss << "," << index;
	}

	return ss.str();
}

std::string ScoreSheet::ToResultIndex()
{
	int len = this->wrongVector.size() + this->missIndexVector.size() + this->correctVector.size();
	this->resultVector.resize(len);
	for (auto& missIndex : missIndexVector)
	{
		resultVector[missIndex] = "◻";
	}

	for (auto& correctIndex : correctVector)
	{
		resultVector[correctIndex] = "✔";
	}

	for (auto& wrongIndex : wrongVector)
	{
		resultVector[wrongIndex] = "✖";
	}

	std::stringstream ss;
	for (int i = 0; i < resultVector.size(); i++)
	{
		ss << resultVector[i];
		if (i % 100 == 0)
		{
			ss << std::endl;
		}
	}
	return ss.str();
}

void Score::Clear()
{
	this->sheet.clear();
	this->m_ErrBurstGroups.clear();
}

void Score::AddPrediction(std::string trueLabel, BurstGroupPolicy predict, int index)
{
	std::string trueType = removeSuffix(trueLabel);
	std::string predictType = removeSuffix(predict.GenPrediction());

	if (predictType == "unreliable")
	{
		AddMiss(trueLabel, index);
		m_ErrBurstGroups.push_back(predict);
		return;
	}

	std::unique_lock<std::mutex> lock(mutex);

	if (predictType == trueType)
	{
		sheet[trueType].TP++;
		sheet[trueType].correctVector.push_back(index);
	}
	else
	{
		sheet[trueType].FN++;
		sheet[predictType].FP++;
		sheet[trueType].wrongVector.push_back(index);

		std::unique_lock<std::mutex> lock(debugMutex);
		m_ErrBurstGroups.push_back(predict);

	}
}

void Score::AddPktAvgNum(uint16_t deviceId, float num)
{
	std::string deviceName = ContextFactory::Get().GetDeviceById(deviceId).GetFormattedName();
	std::string trueLableClean = removeSuffix(deviceName);
	std::unique_lock<std::mutex> lock(mutex);
	sheet[trueLableClean].pktAvgNum = num;
}

void Score::AddMiss(std::string trueLabel, int missIndex)
{
	std::string trueLableClean = removeSuffix(trueLabel);
	std::unique_lock<std::mutex> lock(mutex);
	sheet[trueLableClean].missNum++;
	sheet[trueLableClean].missIndexVector.push_back(missIndex);
}

void Score::OutputToFile()
{
	std::filesystem::path path = GetScoreOutpath(outputFolder) + SUFFIX_TABLE_RESULT;
	try {
		OutputStringToFile(path, IndexTable());
	}
	catch (KException e)
	{
		path = GetScoreOutpath(outputFolder) + GetTimeSec(std::time(nullptr)) + SUFFIX_TABLE_RESULT;
		OutputStringToFile(path, IndexTable());
	}
	
	path = GetScoreOutpath(outputFolder) + SUFFIX_ABNORMAL_DETAIL;
	try {
		OutputStringToFile(path, WrongBurstGroupDetails());
	}
	catch (KException e)
	{
		path = GetScoreOutpath(outputFolder) + GetTimeSec(std::time(nullptr)) + SUFFIX_ABNORMAL_DETAIL;
		OutputStringToFile(path, WrongBurstGroupDetails());
	}
}

void Score::OutputStringToFile(std::filesystem::path path, const std::string& str)
{

	std::filesystem::create_directories(path.parent_path());

	std::ofstream ofs(path);

	if (!ofs.is_open())
	{
		throw KException(KException::Code::OPEN_ERROR);
	}

	ofs << str;
	ofs.close();
}

std::string Score::IndexTable()
{
	std::stringstream ss;

	ss << "device,TP,FN,FP,recall,precision,f1,miss,avgPktNum" << std::endl;

	for (auto& [deviceName, scoreSheet] : sheet)
	{
		ss << deviceName << ","
			<< scoreSheet.ToCsvString()
			<< std::endl;
	}

	ss << GetSummaryIndex();
	return ss.str();
}

std::string Score::WrongBurstGroupDetails()
{
	std::stringstream ss;

	for (auto& debug : m_ErrBurstGroups)
	{
		ss << debug.ToString();
	}

	return ss.str();
}

std::string Score::GetSummaryIndex()
{
	std::stringstream ss;

	int TP = 0, FN = 0, FP = 0, missNum = 0;
	float precision = 0, recall = 0, f1 = 0;


	int sheetSize = sheet.size();
	for (auto& [name, scoresheet] : sheet)
	{
		TP += scoresheet.TP;
		FN += scoresheet.FN;
		FP += scoresheet.FP;

		missNum += scoresheet.missNum;
		precision += scoresheet.Precision();
		recall += scoresheet.Recall();
		f1 += scoresheet.F1();
	}

	ss << "Total," << TP << "," << FN << "," << FP << ","
		<< recall / sheetSize << ","
		<< precision / sheetSize << ","
		<< f1 / sheetSize << ","
		<< missNum << std::endl;

	ss << "Total Accuracy," << (TP+0.0f)  / (TP  + FP );

	return  ss.str();

}

std::string Score::TrueFalseMissEnum()
{
	std::stringstream ss;

	for (auto& [deviceName, scoreSheet] : sheet)
	{
		ss << deviceName
			<< scoreSheet.ToResultIndex()
			<< std::endl;
	}
	return ss.str();
}

std::string Score::GetScoreOutpath(const std::string& outFolder)
{
	std::stringstream ss;
	ss << outFolder << "/res/" << GetScoreStem();
	return ss.str();
}

std::string Score::GetScoreOutpathByDataset(const std::string& datasetName)
{
	std::stringstream ss;
	ss << datasetInfoMaps[datasetName].datasetFolder << "/res/" << GetScoreStem();
	return ss.str();
}

std::string Score::GetScoreStem()
{
	std::stringstream ss;
	
	ss << Config::Get().divisionParam.ToString()
		<< Config::Get().trainParam.ToString()
		<< Config::Get().classifyParam.ToString()
		<< Config::Get().filterParam.ToString();
	
	return ss.str();
}