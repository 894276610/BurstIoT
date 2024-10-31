#include "DatasetInfo.h"
#include <unordered_map>

DatasetInfo unsw2016 = { false, true, "UNSW2016", "F:/UNSW2016"};
DatasetInfo unsw201620 = { false, true, "UNSW201620", "F:/UNSW201620" };

DatasetInfo unsw2018 = { true, false, "UNSW2018", "F:/UNSW2018" };

DatasetInfo unb2021 = { true, false, "UNB2021", "F:/UNB2021" };

DatasetInfo yt201803 = { false, true, "YT201803", "F:/YT201803" };
DatasetInfo yt201804 = { false, true, "YT201804", "F:/YT201804" };

//DatasetInfo neus2019 = { false, true, "NEUS2019", "F:/NE_US" };
DatasetInfo neusIdle2019 = { true, false, "NEUSI2019", "F:/NEUSI2019" };
DatasetInfo neukIdle2019 = { true, false, "NEUKI2019", "F:/NEUKI2019" };
DatasetInfo ne2021 = { true, false, "IOTBEHAV2021", "F:/IOTBEHAV2021" };

DatasetInfo homeSnitch2021 = { true, false, "HomeSnitch2021", "F:/HomeSnitch2021"};

DatasetInfo ncsu2020 = { false, true, "NCSU2020", "F:/NCSU2020" };
DatasetInfo ncsu2021 = { false, true, "NCSU2021", "F:/NCSU2021" };

std::vector<DatasetInfo> datasetConfigs = {
    unsw201620, unsw2016, unsw2018,
    yt201803, yt201804,
    neusIdle2019, neukIdle2019,
    ncsu2020, ncsu2021,
    homeSnitch2021, unb2021
};

std::unordered_map<std::string, DatasetInfo> datasetInfoMaps =
{
    {"UNSW2016", unsw2016}, {"UNSW2018", unsw2018}, {"NCSU2020", ncsu2020}, {"NCSU2021", ncsu2021},
    {"NEUKI2019", neukIdle2019}, {"YT201804", yt201804}, {"NEUSI2019", neusIdle2019}, {"IOTBEHAV2021", ne2021},
    {"UNSW201620", unsw201620}
};