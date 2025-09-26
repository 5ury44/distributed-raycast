#include "worker_pool.h"
#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>
#include <algorithm>

namespace RaycastMaster {

// WorkerConnection implementation
WorkerConnection::WorkerConnection(const std::string& endpoint) 
    : endpoint_(endpoint),
      last_health_check_(std::chrono::steady_clock::now()) {
    Connect();
}

bool WorkerConnection::Connect() {
    try {
        auto channel = grpc::CreateChannel(endpoint_, grpc::InsecureChannelCredentials());
        stub_ = RaycastWorker::WorkerService::NewStub(channel);
        
        // Test connection with a health check
        return PerformHealthCheck();
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to connect to worker " << endpoint_ << ": " << e.what() << std::endl;
        is_healthy_.store(false);
        return false;
    }
}

void WorkerConnection::Disconnect() {
    stub_.reset();
    is_healthy_.store(false);
}

bool WorkerConnection::IsHealthy() {
    std::lock_guard<std::mutex> lock(health_check_mutex_);
    
    // Check if we need to perform a health check
    auto now = std::chrono::steady_clock::now();
    auto time_since_last_check = std::chrono::duration_cast<std::chrono::seconds>(
        now - last_health_check_);
    
    const char* healthCheckInterval = std::getenv("HEALTH_CHECK_INTERVAL_SECONDS");
    int intervalSeconds = healthCheckInterval ? std::atoi(healthCheckInterval) : 30;
    
    if (time_since_last_check.count() > intervalSeconds) {
        return PerformHealthCheck();
    }
    
    return is_healthy_.load();
}

void WorkerConnection::MarkUnhealthy() {
    is_healthy_.store(false);
}

bool WorkerConnection::PerformHealthCheck() {
    if (!stub_) {
        is_healthy_.store(false);
        return false;
    }
    
    try {
        grpc::ClientContext context;
        const char* healthCheckTimeout = std::getenv("HEALTH_CHECK_TIMEOUT_SECONDS");
        int timeoutSeconds = healthCheckTimeout ? std::atoi(healthCheckTimeout) : 5;
        context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(timeoutSeconds));
        
        RaycastWorker::StatusRequest request;
        RaycastWorker::WorkerStatus response;
        
        auto status = stub_->GetWorkerStatus(&context, request, &response);
        bool healthy = status.ok();
        
        is_healthy_.store(healthy);
        last_health_check_ = std::chrono::steady_clock::now();
        
        if (!healthy) {
            std::cerr << "Health check failed for worker " << endpoint_ 
                      << ": " << status.error_message() << std::endl;
        }
        
        return healthy;
        
    } catch (const std::exception& e) {
        std::cerr << "Health check exception for worker " << endpoint_ 
                  << ": " << e.what() << std::endl;
        is_healthy_.store(false);
        return false;
    }
}

void WorkerConnection::UpdateLastHealthCheck() {
    std::lock_guard<std::mutex> lock(health_check_mutex_);
    last_health_check_ = std::chrono::steady_clock::now();
}

void WorkerConnection::IncrementActiveJobs() {
    active_jobs_.fetch_add(1);
}

void WorkerConnection::DecrementActiveJobs() {
    active_jobs_.fetch_sub(1);
}

void WorkerConnection::UpdateJobStats(int64_t processing_time_ms) {
    total_jobs_processed_.fetch_add(1);
    double current_total = total_processing_time_ms_.load();
    total_processing_time_ms_.store(current_total + static_cast<double>(processing_time_ms));
}

double WorkerConnection::GetAverageProcessingTimeMs() const {
    int total_jobs = total_jobs_processed_.load();
    if (total_jobs == 0) return 0.0;
    return total_processing_time_ms_.load() / total_jobs;
}

grpc::Status WorkerConnection::ProcessRenderRequest(const RaycastWorker::RenderRequest* request,
                                                   RaycastWorker::RenderResponse* response) {
    if (!stub_) {
        return grpc::Status(grpc::StatusCode::UNAVAILABLE, "Worker not connected");
    }
    
    try {
        grpc::ClientContext context;
        const char* requestTimeout = std::getenv("REQUEST_TIMEOUT_SECONDS");
        int timeoutSeconds = requestTimeout ? std::atoi(requestTimeout) : 30;
        context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(timeoutSeconds));
        
        auto start_time = std::chrono::high_resolution_clock::now();
        IncrementActiveJobs();
        
        auto status = stub_->ProcessRenderRequest(&context, *request, response);
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        UpdateJobStats(duration.count());
        
        DecrementActiveJobs();
        
        if (status.ok()) {
            UpdateLastHealthCheck();
        } else {
            MarkUnhealthy();
        }
        
        return status;
        
    } catch (const std::exception& e) {
        DecrementActiveJobs();
        MarkUnhealthy();
        return grpc::Status(grpc::StatusCode::INTERNAL, 
                           std::string("Exception: ") + e.what());
    }
}

grpc::Status WorkerConnection::GetWorkerStatus(const RaycastWorker::StatusRequest* request,
                                              RaycastWorker::WorkerStatus* response) {
    if (!stub_) {
        return grpc::Status(grpc::StatusCode::UNAVAILABLE, "Worker not connected");
    }
    
    try {
        grpc::ClientContext context;
        const char* healthCheckTimeout = std::getenv("HEALTH_CHECK_TIMEOUT_SECONDS");
        int timeoutSeconds = healthCheckTimeout ? std::atoi(healthCheckTimeout) : 5;
        context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(timeoutSeconds));
        
        auto status = stub_->GetWorkerStatus(&context, *request, response);
        
        if (status.ok()) {
            UpdateLastHealthCheck();
        } else {
            MarkUnhealthy();
        }
        
        return status;
        
    } catch (const std::exception& e) {
        MarkUnhealthy();
        return grpc::Status(grpc::StatusCode::INTERNAL, 
                           std::string("Exception: ") + e.what());
    }
}

// WorkerPool implementation
WorkerPool::WorkerPool(const std::string& service_name, const std::string& namespace_name)
    : worker_service_name_(service_name),
      worker_namespace_(namespace_name),
      last_discovery_(std::chrono::steady_clock::now()) {
}

void WorkerPool::DiscoverWorkers() {
    std::lock_guard<std::mutex> lock(workers_mutex_);
    
    try {
        auto endpoints = GetWorkerEndpoints();
        
        // Remove old workers that are no longer in the endpoint list
        workers_.erase(
            std::remove_if(workers_.begin(), workers_.end(),
                [&endpoints](const std::unique_ptr<WorkerConnection>& worker) {
                    return std::find(endpoints.begin(), endpoints.end(), worker->GetEndpoint()) == endpoints.end();
                }),
            workers_.end()
        );
        
        // Add new workers
        for (const auto& endpoint : endpoints) {
            bool exists = std::any_of(workers_.begin(), workers_.end(),
                [&endpoint](const std::unique_ptr<WorkerConnection>& worker) {
                    return worker->GetEndpoint() == endpoint;
                });
            
            if (!exists) {
                auto worker = std::make_unique<WorkerConnection>(endpoint);
                if (worker->IsHealthy()) {
                    workers_.push_back(std::move(worker));
                    std::cout << "Added worker: " << endpoint << std::endl;
                }
            }
        }
        
        last_discovery_ = std::chrono::steady_clock::now();
        
    } catch (const std::exception& e) {
        std::cerr << "Exception during worker discovery: " << e.what() << std::endl;
    }
}

void WorkerPool::RefreshWorkers() {
    if (ShouldRefreshWorkers()) {
        DiscoverWorkers();
    }
}

std::vector<WorkerConnection*> WorkerPool::GetHealthyWorkers() {
    std::lock_guard<std::mutex> lock(workers_mutex_);
    std::vector<WorkerConnection*> healthy;
    
    for (auto& worker : workers_) {
        if (worker->IsHealthy()) {
            healthy.push_back(worker.get());
        }
    }
    return healthy;
}

std::vector<WorkerConnection*> WorkerPool::GetAllWorkers() {
    std::lock_guard<std::mutex> lock(workers_mutex_);
    std::vector<WorkerConnection*> all;
    
    for (auto& worker : workers_) {
        all.push_back(worker.get());
    }
    return all;
}

void WorkerPool::AddWorker(const std::string& endpoint) {
    std::lock_guard<std::mutex> lock(workers_mutex_);
    
    // Check if worker already exists
    bool exists = std::any_of(workers_.begin(), workers_.end(),
        [&endpoint](const std::unique_ptr<WorkerConnection>& worker) {
            return worker->GetEndpoint() == endpoint;
        });
    
    if (!exists) {
        auto worker = std::make_unique<WorkerConnection>(endpoint);
        if (worker->IsHealthy()) {
            workers_.push_back(std::move(worker));
            std::cout << "Added worker: " << endpoint << std::endl;
        }
    }
}

void WorkerPool::RemoveWorker(const std::string& endpoint) {
    std::lock_guard<std::mutex> lock(workers_mutex_);
    
    workers_.erase(
        std::remove_if(workers_.begin(), workers_.end(),
            [&endpoint](const std::unique_ptr<WorkerConnection>& worker) {
                return worker->GetEndpoint() == endpoint;
            }),
        workers_.end()
    );
    
    std::cout << "Removed worker: " << endpoint << std::endl;
}

WorkerConnection* WorkerPool::FindWorker(const std::string& endpoint) {
    std::lock_guard<std::mutex> lock(workers_mutex_);
    
    auto it = std::find_if(workers_.begin(), workers_.end(),
        [&endpoint](const std::unique_ptr<WorkerConnection>& worker) {
            return worker->GetEndpoint() == endpoint;
        });
    
    return (it != workers_.end()) ? it->get() : nullptr;
}

int WorkerPool::GetTotalWorkers() {
    std::lock_guard<std::mutex> lock(workers_mutex_);
    return static_cast<int>(workers_.size());
}

int WorkerPool::GetActiveWorkers() {
    std::lock_guard<std::mutex> lock(workers_mutex_);
    return static_cast<int>(std::count_if(workers_.begin(), workers_.end(),
        [](const std::unique_ptr<WorkerConnection>& worker) {
            return worker->IsHealthy();
        }));
}

std::vector<InternalWorkerInfo> WorkerPool::GetWorkerInfo() {
    std::lock_guard<std::mutex> lock(workers_mutex_);
    std::vector<InternalWorkerInfo> info;
    
    for (const auto& worker : workers_) {
        InternalWorkerInfo worker_info;
        worker_info.endpoint = worker->GetEndpoint();
        worker_info.worker_id = 0; // Will be updated from worker status
        worker_info.status = worker->IsHealthy() ? "healthy" : "unhealthy";
        worker_info.active_jobs = worker->GetActiveJobs();
        worker_info.total_jobs_processed = worker->GetTotalJobsProcessed();
        worker_info.average_processing_time_ms = worker->GetAverageProcessingTimeMs();
        worker_info.last_heartbeat = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        
        info.push_back(worker_info);
    }
    
    return info;
}

void WorkerPool::SetDiscoveryInterval(std::chrono::seconds interval) {
    discovery_interval_ = interval;
}

void WorkerPool::SetWorkerServiceName(const std::string& name) {
    worker_service_name_ = name;
}

void WorkerPool::SetWorkerNamespace(const std::string& namespace_name) {
    worker_namespace_ = namespace_name;
}

std::vector<std::string> WorkerPool::GetWorkerEndpoints() {
    std::vector<std::string> endpoints;
    
    // For now, use hardcoded endpoints based on the service name
    // In a real implementation, you would use Kubernetes API or DNS discovery
    std::stringstream ss;
    const char* port = std::getenv("WORKER_PORT");
    std::string workerPort = port ? port : "50051";
    ss << worker_service_name_ << "." << worker_namespace_ << ".svc.cluster.local:" << workerPort;
    endpoints.push_back(ss.str());
    
    // You could also add individual pod endpoints for more granular load balancing
    // This would require Kubernetes API integration
    
    return endpoints;
}

bool WorkerPool::ShouldRefreshWorkers() const {
    auto now = std::chrono::steady_clock::now();
    auto time_since_last_discovery = std::chrono::duration_cast<std::chrono::seconds>(
        now - last_discovery_);
    
    return time_since_last_discovery >= discovery_interval_;
}

} // namespace RaycastMaster
