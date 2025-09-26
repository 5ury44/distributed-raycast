// packages/worker/include/raycast_engine.h
#pragma once
#include "worker_types.h"
#include <cmath>

namespace RaycastWorker {

class RaycastEngine {
private:
    static constexpr double MAX_DISTANCE = 800.0;
    static constexpr int SCREEN_HEIGHT = 768;
    
public:
    static void castRay(double rayAngle, double playerX, double playerY, double playerPitch,
                       const std::vector<std::vector<int>>& map, int mapWidth, int mapHeight,
                       double& distance, int& wallType, double& wallX);
    
    static std::vector<InternalRaycastResult> renderColumns(const InternalRenderRequest& request);
    
    static bool isWall(double x, double y, const std::vector<std::vector<int>>& map,
                      int mapWidth, int mapHeight);
    
    static uint8_t calculateIntensity(double distance);
    static void getWallColor(int wallType, uint8_t intensity, uint8_t& r, uint8_t& g, uint8_t& b);
};

} // namespace RaycastWorker