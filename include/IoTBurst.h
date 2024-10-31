#ifndef _IOTBURST_H_
#define _IOTBURST_H_

#include "AConfig.h"
#include "BurstDataset.h"
#include "Context.h"
#include "Combiner.h"
#include "KBurstClassifier.h"
#include "Reorganizer.h"
#include "Timer.h"
#include "tqdm/tqdm.h"

class IoTBurst {

public:
	IoTBurst();

	~IoTBurst() {
		delete converter;
	}

	void LabelRateExperiment();
	void NCSULongTermExperiment();
	void SingleExperiment(std::vector<int> cfg, const std::string& datasetName);
private:

	

private:
	BurstDataset* GetBurstFullDataset(const std::string& datsetName);
	BurstDataset* FromBurstDataset(const std::string& datasetName);
	BurstDataset* FromPktDataset(const std::string& datasetName);
	BurstDataset* FromRawPackets(const std::string& datasetName);
private:

	std::vector<int> GetSampleRates();
	std::vector<int> GetSlotTimes();
	std::vector<int> GetIntervals();
	std::vector<int> GetMaxUniques();
	std::vector<int> GetMaxDurations();

	std::vector<std::string> GetDatasetNames();

	BurstDataset* GetBurstDataset(const std::string& datasetName);


private:
	Converter* converter = nullptr;
};

#endif
