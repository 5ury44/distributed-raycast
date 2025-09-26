#include "master_server.h"
#include <iostream>
#include <sstream>

namespace RaycastMaster {

MasterServiceImpl::MasterServiceImpl() 
    : worker_pool_(std::make_unique<WorkerPool>()),
      load_balancer_(std::make_unique<LoadBalancer>(worker_pool_.get())) {
    
    // Discover workers on startup
    worker_pool_->DiscoverWorkers();
    
    std::cout << "Master server initialized with " 
              << worker_pool_->GetActiveWorkers() 
              << " active workers" << std::endl;
}

grpc::Status MasterServiceImpl::ProcessRaycastRequest(grpc::ServerContext* context,
                                                     const RaycastRequest* request,
                                                     RaycastResponse* response) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    try {
        // Refresh workers if needed
        worker_pool_->RefreshWorkers();
        
        // Get available worker
        auto worker = load_balancer_->GetNextWorker();
        if (!worker) {
            response->set_success(false);
            response->set_error_message("No workers available");
            return grpc::Status(grpc::StatusCode::UNAVAILABLE, "No workers available");
        }
        
        // Convert master request to worker request
        RaycastWorker::RenderRequest worker_request;
        ConvertRequest(request, &worker_request);
        
        // Send to worker
        RaycastWorker::RenderResponse worker_response;
        auto status = worker->ProcessRenderRequest(&worker_request, &worker_response);
        
        if (status.ok()) {
            // Convert worker response to master response
            ConvertResponse(&worker_response, response);
            response->set_worker_endpoint(worker->GetEndpoint());
            response->set_success(true);
            
            // Update statistics
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
            UpdateStats(duration.count());
            
            std::cout << "Request " << request->request_id() 
                      << " processed by worker " << worker->GetEndpoint()
                      << " in " << duration.count() << "ms" << std::endl;
        } else {
            response->set_success(false);
            response->set_error_message(status.error_message());
            std::cerr << "Worker request failed: " << status.error_message() << std::endl;
        }
        
        return status;
        
    } catch (const std::exception& e) {
        response->set_success(false);
        response->set_error_message(std::string("Internal error: ") + e.what());
        std::cerr << "Exception in ProcessRaycastRequest: " << e.what() << std::endl;
        return grpc::Status(grpc::StatusCode::INTERNAL, "Internal server error");
    }
}

grpc::Status MasterServiceImpl::GetMasterStatus(grpc::ServerContext* context,
                                               const StatusRequest* request,
                                               MasterStatus* response) {
    try {
        // Refresh workers
        worker_pool_->RefreshWorkers();
        
        // Get worker information
        auto worker_info = worker_pool_->GetWorkerInfo();
        
        response->set_total_workers(worker_pool_->GetTotalWorkers());
        response->set_active_workers(worker_pool_->GetActiveWorkers());
        response->set_total_requests_processed(total_requests_processed_.load());
        response->set_average_response_time_ms(
            total_requests_processed_.load() > 0 ? 
            total_response_time_ms_.load() / total_requests_processed_.load() : 0.0);
        response->set_timestamp(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
        
        // Copy worker info
        for (const auto& info : worker_info) {
            auto* worker = response->add_workers();
            worker->set_endpoint(info.endpoint);
            worker->set_worker_id(info.worker_id);
            worker->set_status(info.status);
            worker->set_active_jobs(info.active_jobs);
            worker->set_total_jobs_processed(info.total_jobs_processed);
            worker->set_average_processing_time_ms(info.average_processing_time_ms);
            worker->set_last_heartbeat(info.last_heartbeat);
        }
        
        return grpc::Status::OK;
        
    } catch (const std::exception& e) {
        std::cerr << "Exception in GetMasterStatus: " << e.what() << std::endl;
        return grpc::Status(grpc::StatusCode::INTERNAL, "Internal server error");
    }
}

void MasterServiceImpl::ConvertRequest(const RaycastRequest* master_request, 
                                      RaycastWorker::RenderRequest* worker_request) {
    worker_request->set_request_id(master_request->request_id());
    worker_request->set_player_id(master_request->client_id());
    worker_request->set_timestamp(master_request->timestamp());
    
    // Convert player
    auto* player = worker_request->mutable_player();
    player->set_x(master_request->player().x());
    player->set_y(master_request->player().y());
    player->set_angle(master_request->player().angle());
    player->set_pitch(master_request->player().pitch());
    player->set_id(master_request->player().id());
    player->set_timestamp(master_request->player().timestamp());
    
    // Convert render parameters
    worker_request->set_screen_width(master_request->screen_width());
    worker_request->set_screen_height(master_request->screen_height());
    worker_request->set_fov(master_request->fov());
    worker_request->set_start_column(master_request->start_column());
    worker_request->set_end_column(master_request->end_column());
    
    // Convert map
    for (int i = 0; i < master_request->map_size(); ++i) {
        worker_request->add_map(master_request->map(i));
    }
    worker_request->set_map_width(master_request->map_width());
    worker_request->set_map_height(master_request->map_height());
}

void MasterServiceImpl::ConvertResponse(const RaycastWorker::RenderResponse* worker_response,
                                       RaycastResponse* master_response) {
    master_response->set_request_id(worker_response->request_id());
    master_response->set_client_id(worker_response->player_id());
    master_response->set_worker_id(worker_response->worker_id());
    master_response->set_timestamp(worker_response->timestamp());
    master_response->set_processing_time_ms(worker_response->processing_time_ms());
    
    // Convert results
    for (int i = 0; i < worker_response->results_size(); ++i) {
        const auto& worker_result = worker_response->results(i);
        auto* master_result = master_response->add_results();
        
        master_result->set_column(worker_result.column());
        master_result->set_distance(worker_result.distance());
        master_result->set_wall_type(worker_result.wall_type());
        master_result->set_wall_x(worker_result.wall_x());
        master_result->set_wall_top(worker_result.wall_top());
        master_result->set_wall_bottom(worker_result.wall_bottom());
        master_result->set_r(worker_result.r());
        master_result->set_g(worker_result.g());
        master_result->set_b(worker_result.b());
    }
}

void MasterServiceImpl::UpdateStats(int64_t response_time_ms) {
    total_requests_processed_.fetch_add(1);
    double current_total = total_response_time_ms_.load();
    total_response_time_ms_.store(current_total + static_cast<double>(response_time_ms));
}

// MasterServer implementation
MasterServer::MasterServer(const std::string& address, int port) 
    : service_(std::make_unique<MasterServiceImpl>()),
      server_address_(address),
      port_(port) {
}

bool MasterServer::Start() {
    try {
        std::stringstream ss;
        ss << server_address_ << ":" << port_;
        std::string server_address = ss.str();
        
        grpc::ServerBuilder builder;
        builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
        builder.RegisterService(service_.get());
        
        server_ = builder.BuildAndStart();
        if (!server_) {
            std::cerr << "Failed to start master server on " << server_address << std::endl;
            return false;
        }
        
        std::cout << "Master server listening on " << server_address << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Exception starting master server: " << e.what() << std::endl;
        return false;
    }
}

void MasterServer::Stop() {
    if (server_) {
        server_->Shutdown();
        std::cout << "Master server stopped" << std::endl;
    }
}

void MasterServer::Wait() {
    if (server_) {
        server_->Wait();
    }
}

bool MasterServer::IsRunning() const {
    return server_ != nullptr;
}

std::string MasterServer::GetAddress() const {
    return server_address_;
}

int MasterServer::GetPort() const {
    return port_;
}

} // namespace RaycastMaster
