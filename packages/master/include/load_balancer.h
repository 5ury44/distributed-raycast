#pragma once

#include <vector>
#include <memory>
#include <atomic>
#include <random>
#include <algorithm>

#include "worker_pool.h"

namespace RaycastMaster {

enum class LoadBalancingStrategy {
    ROUND_ROBIN,
    LEAST_LOADED,
    RANDOM,
    WEIGHTED_ROUND_ROBIN
};

class LoadBalancer {
private:
    WorkerPool* worker_pool_;
    LoadBalancingStrategy strategy_;
    std::atomic<size_t> round_robin_index_{0};
    std::mt19937 random_generator_;
    std::uniform_int_distribution<size_t> random_distribution_;
    
public:
    explicit LoadBalancer(WorkerPool* worker_pool, 
                         LoadBalancingStrategy strategy = LoadBalancingStrategy::ROUND_ROBIN);
    ~LoadBalancer() = default;
    
    // Worker selection
    WorkerConnection* GetNextWorker();
    WorkerConnection* GetWorkerForRequest(const std::string& request_id = "");
    
    // Strategy management
    void SetStrategy(LoadBalancingStrategy strategy);
    LoadBalancingStrategy GetStrategy() const { return strategy_; }
    
    // Statistics
    std::vector<WorkerConnection*> GetAllWorkers();
    int GetAvailableWorkerCount() const;
    
private:
    WorkerConnection* SelectRoundRobin();
    WorkerConnection* SelectLeastLoaded();
    WorkerConnection* SelectRandom();
    WorkerConnection* SelectWeightedRoundRobin();
    
    double CalculateWorkerWeight(WorkerConnection* worker) const;
};

} // namespace RaycastMaster
