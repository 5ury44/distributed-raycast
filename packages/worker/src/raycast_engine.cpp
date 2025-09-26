// packages/worker/src/raycast_engine.cpp
#include "raycast_engine.h"
#include <algorithm>

namespace RaycastWorker {

void RaycastEngine::castRay(double rayAngle, double playerX, double playerY, double playerPitch,
                           const std::vector<std::vector<int>>& map, int mapWidth, int mapHeight,
                           double& distance, int& wallType, double& wallX) {
    double rayX = playerX;
    double rayY = playerY;
    double rayDirX = cos(rayAngle) * cos(playerPitch);
    double rayDirY = sin(rayAngle) * cos(playerPitch);
    
    double deltaDistX = (rayDirX == 0) ? 1e30 : std::abs(1.0 / rayDirX);
    double deltaDistY = (rayDirY == 0) ? 1e30 : std::abs(1.0 / rayDirY);
    
    int mapX = (int)rayX;
    int mapY = (int)rayY;
    
    double sideDistX, sideDistY;
    int stepX, stepY;
    int side;
    
    if (rayDirX < 0) {
        stepX = -1;
        sideDistX = (rayX - mapX) * deltaDistX;
    } else {
        stepX = 1;
        sideDistX = (mapX + 1.0 - rayX) * deltaDistX;
    }
    
    if (rayDirY < 0) {
        stepY = -1;
        sideDistY = (rayY - mapY) * deltaDistY;
    } else {
        stepY = 1;
        sideDistY = (mapY + 1.0 - rayY) * deltaDistY;
    }
    
    // DDA algorithm
    while (true) {
        if (sideDistX < sideDistY) {
            sideDistX += deltaDistX;
            mapX += stepX;
            side = 0;
        } else {
            sideDistY += deltaDistY;
            mapY += stepY;
            side = 1;
        }
        
        if (mapX < 0 || mapX >= mapWidth || mapY < 0 || mapY >= mapHeight || 
            map[mapY][mapX] == 1) {
            break;
        }
    }
    
    if (side == 0) {
        distance = (sideDistX - deltaDistX);
        wallX = playerY + distance * rayDirY;
    } else {
        distance = (sideDistY - deltaDistY);
        wallX = playerX + distance * rayDirX;
    }
    
    // Prevent fisheye effect
    distance = distance * cos(playerPitch);
    
    // Determine wall type
    wallType = (mapX + mapY) % 6;
}

std::vector<InternalRaycastResult> RaycastEngine::renderColumns(const InternalRenderRequest& request) {
    std::vector<InternalRaycastResult> results;
    results.reserve(request.endColumn - request.startColumn);
    
    for (int x = request.startColumn; x < request.endColumn; x++) {
        double rayAngle = request.player.angle - request.fov/2 + 
                         (x * request.fov / request.screenWidth);
        
        double distance, wallX;
        int wallType;
        
        castRay(rayAngle, request.player.x, request.player.y, request.player.pitch,
                request.map, request.mapWidth, request.mapHeight,
                distance, wallType, wallX);
        
        // Calculate wall height
        int wallHeight = static_cast<int>(SCREEN_HEIGHT / distance);
        int wallTop = (SCREEN_HEIGHT - wallHeight) / 2;
        int wallBottom = wallTop + wallHeight;
        
        // Calculate color
        uint8_t intensity = calculateIntensity(distance);
        uint8_t r, g, b;
        getWallColor(wallType, intensity, r, g, b);
        
        results.push_back({
            x, distance, wallType, wallX, wallTop, wallBottom, r, g, b
        });
    }
    
    return results;
}

bool RaycastEngine::isWall(double x, double y, const std::vector<std::vector<int>>& map,
                          int mapWidth, int mapHeight) {
    int mapX = (int)x;
    int mapY = (int)y;
    
    if (mapX < 0 || mapX >= mapWidth || mapY < 0 || mapY >= mapHeight) {
        return true;
    }
    
    return map[mapY][mapX] == 1;
}

uint8_t RaycastEngine::calculateIntensity(double distance) {
    uint8_t intensity = static_cast<uint8_t>(255 * (1.0 - distance / MAX_DISTANCE));
    return std::max(intensity, static_cast<uint8_t>(50));
}

void RaycastEngine::getWallColor(int wallType, uint8_t intensity, uint8_t& r, uint8_t& g, uint8_t& b) {
    // Wall type colors (scaled by intensity)
    uint8_t baseColors[6][3] = {
        {34, 139, 34},   // Green (grass)
        {105, 105, 105}, // Gray (rock)
        {128, 128, 128}, // Light gray (stone)
        {139, 69, 19},   // Brown (wood)
        {160, 82, 45},   // Saddle brown (dirt)
        {178, 34, 34}    // Red (brick)
    };
    
    if (wallType >= 0 && wallType < 6) {
        r = (baseColors[wallType][0] * intensity) / 255;
        g = (baseColors[wallType][1] * intensity) / 255;
        b = (baseColors[wallType][2] * intensity) / 255;
    } else {
        r = g = b = intensity;
    }
}

} // namespace RaycastWorker