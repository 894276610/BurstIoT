#pragma once

#include <string>
#include <vector>
#include <algorithm>

#include "pcppUsed/pcppCommon.h"
#include "KException.h"

void SplitString(const std::string& s, std::vector<std::string>& tokens, const std::string& delimiters);
std::pair<std::string, std::string> SplitPairStrByComma(const std::string& s);

template<typename T>
std::vector<std::vector<T>> splitVector(const std::vector<T>& vec, size_t chunkSize) {

    std::vector<std::vector<T>> chunks;
    for (size_t i = 0; i < vec.size(); i += chunkSize) {
        // 使用子范围构造函数创建子vector  
        int size = i + chunkSize < vec.size() ? i + chunkSize : vec.size();
        chunks.emplace_back(vec.begin() + i, vec.begin() + size);
    }
    return chunks;
}

std::string removeSuffix(const std::string& str); 
time_t FloorDuration(const timespec& end, const timespec& start);
std::string GetTimeSec(time_t sec);
