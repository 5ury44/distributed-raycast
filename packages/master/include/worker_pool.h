#pragma once

#include <vector>
#include <memory>
#include <mutex>
#include <string>
#include <chrono>
#include <atomic>

#include <grpcpp/grpcpp.h>
#include "worker_service.pb.h"
#include "worker_service.grpc.pb.h"
#include "master_service.pb.h"

namespace RaycastMaster {

struct InternalWorkerInfo {
    std::string endpoint;
    int worker_id;
    std::string status;
    int active_jobs;
    int total_jobs_processed;
    double average_processing_time_ms;
    int64_t last_heartbeat;
};

class WorkerConnection {
private:
    std::string endpoint_;
    std::unique_ptr<RaycastWorker::WorkerService::Stub> stub_;
    std::atomic<bool> is_healthy_{true};
    std::atomic<int> active_jobs_{0};
    std::atomic<int> total_jobs_processed_{0};
    std::atomic<double> total_processing_time_ms_{0.0};
    std::chrono::steady_clock::time_point last_health_check_;
    std::mutex health_check_mutex_;
    
public:
    explicit WorkerConnection(const std::string& endpoint);
    ~WorkerConnection() = default;
    
    // Connection management
    bool Connect();
    void Disconnect();
    bool IsHealthy();
    void MarkUnhealthy();
    
    // Health checking
    bool PerformHealthCheck();
    void UpdateLastHealthCheck();
    
    // Job management
    void IncrementActiveJobs();
    void DecrementActiveJobs();
    void UpdateJobStats(int64_t processing_time_ms);
    
    // Getters
    const std::string& GetEndpoint() const { return endpoint_; }
    int GetActiveJobs() const { return active_jobs_.load(); }
    int GetTotalJobsProcessed() const { return total_jobs_processed_.load(); }
    double GetAverageProcessingTimeMs() const;
    
    // gRPC calls
    grpc::Status ProcessRenderRequest(const RaycastWorker::RenderRequest* request,
                                     RaycastWorker::RenderResponse* response);
    
    grpc::Status GetWorkerStatus(const RaycastWorker::StatusRequest* request,
                                RaycastWorker::WorkerStatus* response);
};

class WorkerPool {
private:
    std::vector<std::unique_ptr<WorkerConnection>> workers_;
    std::mutex workers_mutex_;
    std::string worker_service_name_;
    std::string worker_namespace_;
    std::chrono::steady_clock::time_point last_discovery_;
    std::chrono::seconds discovery_interval_{30}; // 30 seconds
    
public:
    explicit WorkerPool(const std::string& service_name = "raycast-worker-service",
                       const std::string& namespace_name = "default");
    ~WorkerPool() = default;
    
    // Worker discovery and management
    void DiscoverWorkers();
    void RefreshWorkers();
    std::vector<WorkerConnection*> GetHealthyWorkers();
    std::vector<WorkerConnection*> GetAllWorkers();
    
    // Worker operations
    void AddWorker(const std::string& endpoint);
    void RemoveWorker(const std::string& endpoint);
    WorkerConnection* FindWorker(const std::string& endpoint);
    
    // Statistics
    int GetTotalWorkers();
    int GetActiveWorkers();
    std::vector<InternalWorkerInfo> GetWorkerInfo();
    
    // Configuration
    void SetDiscoveryInterval(std::chrono::seconds interval);
    void SetWorkerServiceName(const std::string& name);
    void SetWorkerNamespace(const std::string& namespace_name);
    
private:
    std::vector<std::string> GetWorkerEndpoints();
    bool ShouldRefreshWorkers() const;
};

} // namespace RaycastMaster
