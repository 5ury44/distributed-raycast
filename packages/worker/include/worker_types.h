// packages/worker/include/worker_types.h
#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <chrono>
#include <atomic>

namespace RaycastWorker {

struct InternalPlayer {
    double x, y;
    double angle;
    double pitch;
    std::string id;
    uint64_t timestamp;
};

struct InternalRaycastResult {
    int column;
    double distance;
    int wallType;
    double wallX;
    int wallTop;
    int wallBottom;
    uint8_t r, g, b; // Color for this column
};

struct InternalRenderRequest {
    std::string requestId;
    std::string playerId;
    InternalPlayer player;
    int screenWidth;
    int screenHeight;
    double fov;
    int startColumn;
    int endColumn;
    std::vector<std::vector<int>> map;
    int mapWidth;
    int mapHeight;
    uint64_t timestamp;
};

struct InternalRenderResponse {
    std::string requestId;
    std::string playerId;
    std::vector<InternalRaycastResult> results;
    int workerId;
    uint64_t timestamp;
    uint64_t processingTimeMs;
};

struct InternalWorkerStatus {
    int workerId;
    std::string status; // "idle", "busy", "error"
    std::atomic<int> activeJobs;
    std::atomic<int> totalJobsProcessed;
    double averageProcessingTimeMs;
    uint64_t lastHeartbeat;
};

} // namespace RaycastWorker