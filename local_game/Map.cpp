#include "Map.h"
#include <SDL2/SDL.h>

Map::Map() {
    // Initialize map data (1 = wall, 0 = empty space)
    int mapData[MAP_HEIGHT][MAP_WIDTH] = {
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,0,1,1,1,0,0,0,0,1,1,1,0,0,1},
        {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
        {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,0,0,0,0,0,1,1,0,0,0,0,0,0,1},
        {1,0,0,0,0,0,0,1,1,0,0,0,0,0,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,0,1,0,0,0,0,0,0,0,0,1,0,0,1},
        {1,0,0,1,0,0,0,0,0,0,0,0,1,0,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
    };
    
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            map[y][x] = mapData[y][x];
        }
    }
}

bool Map::isWall(double x, double y) const {
    int mapX = (int)x;
    int mapY = (int)y;
    
    if (mapX < 0 || mapX >= MAP_WIDTH || mapY < 0 || mapY >= MAP_HEIGHT) {
        return true;
    }
    
    return map[mapY][mapX] == 1;
}

int Map::getTile(int x, int y) const {
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT) {
        return 1; // Treat out of bounds as wall
    }
    return map[y][x];
}

void Map::renderMinimap(SDL_Renderer* renderer, int minimapX, int minimapY, int minimapSize) const {
    // Draw minimap background
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_Rect minimapRect = {minimapX, minimapY, minimapSize, minimapSize};
    SDL_RenderFillRect(renderer, &minimapRect);
    
    // Draw map
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            if (map[y][x] == 1) {
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            } else {
                SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
            }
            
            int rectX = minimapX + x * minimapSize / MAP_WIDTH;
            int rectY = minimapY + y * minimapSize / MAP_HEIGHT;
            int rectW = minimapSize / MAP_WIDTH;
            int rectH = minimapSize / MAP_HEIGHT;
            
            SDL_Rect rect = {rectX, rectY, rectW, rectH};
            SDL_RenderFillRect(renderer, &rect);
        }
    }
}
