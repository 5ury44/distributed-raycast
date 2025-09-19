#pragma once
#include <SDL2/SDL.h>
#include <thread>
#include <vector>
#include <mutex>
#include "Player.h"
#include "Map.h"
#include "TextureManager.h"

const int SCREEN_WIDTH = 1024;
const int SCREEN_HEIGHT = 768;
const double FOV = M_PI / 3; // 60 degrees
const double MAX_DISTANCE = 800.0;
const int NUM_THREADS = 4; // Number of rendering threads

class Raycaster {
private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    Player player;
    Map map;
    TextureManager textureManager;
    bool running;
    
    // Multithreading
    std::vector<std::thread> renderThreads;
    std::mutex renderMutex;
    
    // Mouse control
    bool mouseCaptured;
    int lastMouseX;
    double mouseSensitivity;
    
    // Thread-safe rendering data
    struct RenderData {
        int startX, endX;
        double* distances;
        int* wallTypes;
        double* wallXs;
        int* wallTops;
        int* wallBottoms;
    };
    
public:
    Raycaster();
    ~Raycaster();
    
    bool initialize();
    void run();
    
private:
    void handleInput();
    void castRay(double rayAngle, int column, double& distance, int& wallType, double& wallX);
    void render();
    void renderSky();
    void renderWalls();
    void renderWallsThreaded();
    void renderWallColumn(int x, double distance, int wallType, double wallX, int wallTop, int wallBottom);
    void handleMouseInput();
    void captureMouse();
    void releaseMouse();
    void drawMinimap();
    void cleanup();
};
