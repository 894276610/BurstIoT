#pragma once
#include "Burst.h"
#include <unordered_map>
#include <mutex>
#include <set>

struct PredictionCache {

    std::mutex cacheMutex;
    std::unordered_map<Burst, std::pair<float,BurstVec>> cache;
    std::unordered_map<Burst, std::mutex> lockMap;
    std::mutex mutex;

    void Clear()
    {
        std::lock_guard<std::mutex> lock(cacheMutex);
        cache.clear();
    }

    std::pair<float, BurstVec>& Query(const std::shared_ptr<Burst> burst) {
        
        std::lock_guard<std::mutex> lock(cacheMutex);
        return cache[*burst];
    }

    std::mutex& GetMutex(const std::shared_ptr<Burst> burst)
    {
        std::unique_lock<std::mutex> lock(mutex);
        return lockMap[*burst];
    }
};

class BurstCacheFactory {
public:
 
    static PredictionCache& Get() {
        static PredictionCache* instance = new PredictionCache();
        return *instance;
    }

private:
 
    BurstCacheFactory() = default;
    ~BurstCacheFactory() = default;
    BurstCacheFactory(const BurstCacheFactory&) = delete;
    BurstCacheFactory& operator=(const BurstCacheFactory&) = delete;
};

