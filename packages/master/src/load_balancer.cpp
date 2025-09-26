#include "load_balancer.h"
#include <algorithm>
#include <numeric>

namespace RaycastMaster {

LoadBalancer::LoadBalancer(WorkerPool* worker_pool, LoadBalancingStrategy strategy)
    : worker_pool_(worker_pool),
      strategy_(strategy),
      random_generator_(std::random_device{}()),
      random_distribution_(0, 0) { // Will be updated in GetNextWorker
}

WorkerConnection* LoadBalancer::GetNextWorker() {
    auto workers = worker_pool_->GetHealthyWorkers();
    if (workers.empty()) {
        return nullptr;
    }
    
    // Update random distribution range
    random_distribution_ = std::uniform_int_distribution<size_t>(0, workers.size() - 1);
    
    switch (strategy_) {
        case LoadBalancingStrategy::ROUND_ROBIN:
            return SelectRoundRobin();
        case LoadBalancingStrategy::LEAST_LOADED:
            return SelectLeastLoaded();
        case LoadBalancingStrategy::RANDOM:
            return SelectRandom();
        case LoadBalancingStrategy::WEIGHTED_ROUND_ROBIN:
            return SelectWeightedRoundRobin();
        default:
            return SelectRoundRobin();
    }
}

WorkerConnection* LoadBalancer::GetWorkerForRequest(const std::string& request_id) {
    // For now, just use the same logic as GetNextWorker
    // In the future, you could implement request affinity based on request_id
    return GetNextWorker();
}

void LoadBalancer::SetStrategy(LoadBalancingStrategy strategy) {
    strategy_ = strategy;
}

std::vector<WorkerConnection*> LoadBalancer::GetAllWorkers() {
    return worker_pool_->GetHealthyWorkers();
}

int LoadBalancer::GetAvailableWorkerCount() const {
    return worker_pool_->GetActiveWorkers();
}

WorkerConnection* LoadBalancer::SelectRoundRobin() {
    auto workers = worker_pool_->GetHealthyWorkers();
    if (workers.empty()) {
        return nullptr;
    }
    
    size_t index = round_robin_index_.fetch_add(1) % workers.size();
    return workers[index];
}

WorkerConnection* LoadBalancer::SelectLeastLoaded() {
    auto workers = worker_pool_->GetHealthyWorkers();
    if (workers.empty()) {
        return nullptr;
    }
    
    // Find worker with least active jobs
    auto min_worker = std::min_element(workers.begin(), workers.end(),
        [](WorkerConnection* a, WorkerConnection* b) {
            return a->GetActiveJobs() < b->GetActiveJobs();
        });
    
    return *min_worker;
}

WorkerConnection* LoadBalancer::SelectRandom() {
    auto workers = worker_pool_->GetHealthyWorkers();
    if (workers.empty()) {
        return nullptr;
    }
    
    size_t index = random_distribution_(random_generator_);
    return workers[index];
}

WorkerConnection* LoadBalancer::SelectWeightedRoundRobin() {
    auto workers = worker_pool_->GetHealthyWorkers();
    if (workers.empty()) {
        return nullptr;
    }
    
    // Calculate weights for all workers
    std::vector<double> weights;
    weights.reserve(workers.size());
    
    for (auto* worker : workers) {
        weights.push_back(CalculateWorkerWeight(worker));
    }
    
    // Find worker with highest weight
    auto max_weight_it = std::max_element(weights.begin(), weights.end());
    size_t max_weight_index = std::distance(weights.begin(), max_weight_it);
    
    return workers[max_weight_index];
}

double LoadBalancer::CalculateWorkerWeight(WorkerConnection* worker) const {
    if (!worker) {
        return 0.0;
    }
    
    // Weight calculation based on:
    // 1. Inverse of active jobs (more jobs = lower weight)
    // 2. Processing speed (faster = higher weight)
    // 3. Health status (healthy = higher weight)
    
    double active_jobs_weight = 1.0 / (1.0 + worker->GetActiveJobs());
    double processing_speed_weight = 1.0 / (1.0 + worker->GetAverageProcessingTimeMs() / 1000.0);
    double health_weight = worker->IsHealthy() ? 1.0 : 0.0;
    
    // Combine weights (you can adjust these coefficients)
    return active_jobs_weight * 0.5 + processing_speed_weight * 0.3 + health_weight * 0.2;
}

} // namespace RaycastMaster
