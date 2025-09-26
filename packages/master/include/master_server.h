#pragma once

#include <grpcpp/grpcpp.h>
#include <memory>
#include <string>
#include <atomic>
#include <chrono>

#include "master_service.pb.h"
#include "master_service.grpc.pb.h"
#include "worker_pool.h"
#include "load_balancer.h"

namespace RaycastMaster {

class MasterServiceImpl final : public MasterService::Service {
private:
    std::unique_ptr<WorkerPool> worker_pool_;
    std::unique_ptr<LoadBalancer> load_balancer_;
    std::atomic<int> total_requests_processed_{0};
    std::atomic<double> total_response_time_ms_{0.0};
    
public:
    MasterServiceImpl();
    ~MasterServiceImpl() = default;
    
    // gRPC service methods
    grpc::Status ProcessRaycastRequest(grpc::ServerContext* context,
                                      const RaycastRequest* request,
                                      RaycastResponse* response) override;
    
    grpc::Status GetMasterStatus(grpc::ServerContext* context,
                                const StatusRequest* request,
                                MasterStatus* response) override;
    
private:
    void ConvertRequest(const RaycastRequest* master_request, 
                       RaycastWorker::RenderRequest* worker_request);
    
    void ConvertResponse(const RaycastWorker::RenderResponse* worker_response,
                        RaycastResponse* master_response);
    
    void UpdateStats(int64_t response_time_ms);
};

class MasterServer {
private:
    std::unique_ptr<grpc::Server> server_;
    std::unique_ptr<MasterServiceImpl> service_;
    std::string server_address_;
    int port_;
    
public:
    MasterServer(const std::string& address = "0.0.0.0", int port = 50052);
    ~MasterServer() = default;
    
    bool Start();
    void Stop();
    void Wait();
    
    bool IsRunning() const;
    std::string GetAddress() const;
    int GetPort() const;
};

} // namespace RaycastMaster
