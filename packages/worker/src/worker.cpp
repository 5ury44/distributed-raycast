// packages/worker/src/worker.cpp
#include "raycast_engine.h"
#include "worker_types.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <random>
#include <signal.h>
#include <unistd.h>
#include <functional>
#include <cstdlib>

// gRPC includes
#include <grpcpp/grpcpp.h>
#include "worker_service.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

class RaycastWorkerServiceImpl final : public RaycastWorker::WorkerService::Service {
private:
    int workerId_;
    RaycastWorker::InternalWorkerStatus status_;
    std::atomic<int> activeJobs_;
    std::atomic<int> totalJobsProcessed_;
    std::atomic<double> totalProcessingTime_;
    
public:
    RaycastWorkerServiceImpl(int workerId) 
        : workerId_(workerId), activeJobs_(0), totalJobsProcessed_(0), totalProcessingTime_(0.0) {
        status_.workerId = workerId_;
        status_.status = "idle";
        status_.activeJobs.store(0);
        status_.totalJobsProcessed.store(0);
        status_.averageProcessingTimeMs = 0.0;
        status_.lastHeartbeat = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }
    
    Status ProcessRenderRequest(ServerContext* context, 
                               const RaycastWorker::RenderRequest* request,
                               RaycastWorker::RenderResponse* response) override {
        
        auto startTime = std::chrono::high_resolution_clock::now();
        activeJobs_++;
        status_.activeJobs.store(activeJobs_.load());
        status_.status = "busy";
        
        try {
            // Convert protobuf request to internal format
            RaycastWorker::InternalRenderRequest internalRequest;
            internalRequest.requestId = request->request_id();
            internalRequest.playerId = request->player_id();
            internalRequest.player.x = request->player().x();
            internalRequest.player.y = request->player().y();
            internalRequest.player.angle = request->player().angle();
            internalRequest.player.pitch = request->player().pitch();
            internalRequest.screenWidth = request->screen_width();
            internalRequest.screenHeight = request->screen_height();
            internalRequest.fov = request->fov();
            internalRequest.startColumn = request->start_column();
            internalRequest.endColumn = request->end_column();
            internalRequest.mapWidth = request->map_width();
            internalRequest.mapHeight = request->map_height();
            
            // Convert map data
            for (int y = 0; y < request->map_height(); y++) {
                std::vector<int> row;
                for (int x = 0; x < request->map_width(); x++) {
                    row.push_back(request->map(y * request->map_width() + x));
                }
                internalRequest.map.push_back(row);
            }
            
            // Process raycasting
            auto results = RaycastWorker::RaycastEngine::renderColumns(internalRequest);
            
            // Convert results back to protobuf
            for (const auto& result : results) {
                auto* protoResult = response->add_results();
                protoResult->set_column(result.column);
                protoResult->set_distance(result.distance);
                protoResult->set_wall_type(result.wallType);
                protoResult->set_wall_x(result.wallX);
                protoResult->set_wall_top(result.wallTop);
                protoResult->set_wall_bottom(result.wallBottom);
                protoResult->set_r(result.r);
                protoResult->set_g(result.g);
                protoResult->set_b(result.b);
            }
            
            response->set_request_id(internalRequest.requestId);
            response->set_player_id(internalRequest.playerId);
            response->set_worker_id(workerId_);
            response->set_timestamp(std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count());
            
            auto endTime = std::chrono::high_resolution_clock::now();
            auto processingTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
            response->set_processing_time_ms(processingTime.count());
            
            // Update statistics
            totalJobsProcessed_++;
            totalProcessingTime_ = totalProcessingTime_.load() + static_cast<double>(processingTime.count());
            status_.totalJobsProcessed.store(totalJobsProcessed_.load());
            status_.averageProcessingTimeMs = totalProcessingTime_.load() / totalJobsProcessed_.load();
            
        } catch (const std::exception& e) {
            std::cerr << "Error processing request: " << e.what() << std::endl;
            return Status(grpc::StatusCode::INTERNAL, "Internal processing error");
        }
        
        activeJobs_--;
        status_.activeJobs.store(activeJobs_.load());
        status_.status = activeJobs_ > 0 ? "busy" : "idle";
        status_.lastHeartbeat = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        
        return Status::OK;
    }
    
    Status GetWorkerStatus(ServerContext* context, 
                          const RaycastWorker::StatusRequest* request,
                          RaycastWorker::WorkerStatus* response) override {
        
        response->set_worker_id(status_.workerId);
        response->set_status(status_.status);
        response->set_active_jobs(status_.activeJobs.load());
        response->set_total_jobs_processed(status_.totalJobsProcessed.load());
        response->set_average_processing_time_ms(status_.averageProcessingTimeMs);
        response->set_last_heartbeat(status_.lastHeartbeat);
        
        return Status::OK;
    }
};

void RunWorker(int workerId, const std::string& serverAddress) {
    RaycastWorkerServiceImpl service(workerId);
    
    ServerBuilder builder;
    builder.AddListeningPort(serverAddress, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Worker " << workerId << " listening on " << serverAddress << std::endl;
    
    // Keep the server running
    server->Wait();
}

int main(int argc, char** argv) {
    int workerId = 1;
    std::string serverAddress = "0.0.0.0:50051";
    
    // Try to get worker ID from environment variable first
    const char* envWorkerId = std::getenv("WORKER_ID");
    if (envWorkerId != nullptr) {
        // Extract numeric part from pod name (e.g., "raycast-worker-59859594c9-2d5v8" -> "2d5v8")
        std::string workerIdStr = envWorkerId;
        // Use hash of the pod name to get a unique numeric ID
        std::hash<std::string> hasher;
        workerId = static_cast<int>(hasher(workerIdStr) % 1000) + 1;
    }
    
    // Get server address from environment variable
    const char* envServerAddress = std::getenv("WORKER_SERVER_ADDRESS");
    if (envServerAddress != nullptr) {
        serverAddress = envServerAddress;
    }
    
    if (argc > 1) {
        workerId = std::atoi(argv[1]);
    }
    if (argc > 2) {
        serverAddress = argv[2];
    }
    
    std::cout << "Starting Raycast Worker " << workerId << " on " << serverAddress << std::endl;
    
    RunWorker(workerId, serverAddress);
    return 0;
}