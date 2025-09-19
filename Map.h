#pragma once
#include <SDL2/SDL.h>

const int MAP_WIDTH = 16;
const int MAP_HEIGHT = 16;

class Map {
private:
    int map[MAP_HEIGHT][MAP_WIDTH];
    
public:
    Map();
    bool isWall(double x, double y) const;
    int getTile(int x, int y) const;
    void renderMinimap(SDL_Renderer* renderer, int minimapX, int minimapY, int minimapSize) const;
};
