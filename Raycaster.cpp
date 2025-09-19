#include "Raycaster.h"
#include <cmath>
#include <iostream>

Raycaster::Raycaster() : running(true), mouseCaptured(false), lastMouseX(0), mouseSensitivity(0.002) {
    window = nullptr;
    renderer = nullptr;
}

Raycaster::~Raycaster() {
    // Wait for any running threads to finish
    for (auto& thread : renderThreads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    cleanup();
}

bool Raycaster::initialize() {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }
    
    window = SDL_CreateWindow("Raycaster Game with Textures",
                            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                            SCREEN_WIDTH, SCREEN_HEIGHT,
                            SDL_WINDOW_SHOWN);
    
    if (!window) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }
    
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }
    
    // Load textures
    if (!textureManager.loadTextures(renderer)) {
        std::cerr << "Failed to load textures!" << std::endl;
        return false;
    }
    
    // Initialize mouse capture
    captureMouse();
    
    return true;
}

void Raycaster::handleInput() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            running = false;
        }
        else if (e.type == SDL_KEYDOWN) {
            if (e.key.keysym.sym == SDLK_ESCAPE) {
                if (mouseCaptured) {
                    releaseMouse();
                } else {
                    running = false;
                }
            }
        }
        else if (e.type == SDL_MOUSEBUTTONDOWN) {
            if (e.button.button == SDL_BUTTON_LEFT && !mouseCaptured) {
                captureMouse();
            }
        }
    }
    
    // Handle mouse movement
    if (mouseCaptured) {
        handleMouseInput();
    }
    
    const Uint8* keystate = SDL_GetKeyboardState(NULL);
    
    // Movement
    double newX = player.x;
    double newY = player.y;
    
    if (keystate[SDL_SCANCODE_W]) {
        newX += cos(player.angle) * player.moveSpeed;
        newY += sin(player.angle) * player.moveSpeed;
    }
    if (keystate[SDL_SCANCODE_S]) {
        newX -= cos(player.angle) * player.moveSpeed;
        newY -= sin(player.angle) * player.moveSpeed;
    }
    if (keystate[SDL_SCANCODE_A]) {
        newX += cos(player.angle - M_PI/2) * player.moveSpeed;
        newY += sin(player.angle - M_PI/2) * player.moveSpeed;
    }
    if (keystate[SDL_SCANCODE_D]) {
        newX += cos(player.angle + M_PI/2) * player.moveSpeed;
        newY += sin(player.angle + M_PI/2) * player.moveSpeed;
    }
    
    // Check collision
    if (!map.isWall(newX, player.y)) {
        player.x = newX;
    }
    if (!map.isWall(player.x, newY)) {
        player.y = newY;
    }
    
    // Rotation
    if (keystate[SDL_SCANCODE_LEFT]) {
        player.rotate(-player.rotSpeed);
    }
    if (keystate[SDL_SCANCODE_RIGHT]) {
        player.rotate(player.rotSpeed);
    }
}

void Raycaster::castRay(double rayAngle, int /*column*/, double& distance, int& wallType, double& wallX) {
    double rayX = player.x;
    double rayY = player.y;
    double rayDirX = cos(rayAngle);
    double rayDirY = sin(rayAngle);
    
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
        
        if (mapX < 0 || mapX >= MAP_WIDTH || mapY < 0 || mapY >= MAP_HEIGHT || map.getTile(mapX, mapY) == 1) {
            break;
        }
    }
    
    if (side == 0) {
        distance = (sideDistX - deltaDistX);
        wallX = player.y + distance * rayDirY;
    } else {
        distance = (sideDistY - deltaDistY);
        wallX = player.x + distance * rayDirX;
    }
    
    // Prevent fisheye effect
    distance = distance * cos(player.angle - rayAngle);
    
    // Determine wall type (for now, use map position for variety)
    wallType = (mapX + mapY) % 6;
}

void Raycaster::render() {
    // Clear screen
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    
    renderSky();
    renderWalls();
    drawMinimap();
    
    SDL_RenderPresent(renderer);
}

void Raycaster::renderSky() {
    SDL_Texture* skyTex = textureManager.getSkyTexture();
    if (skyTex) {
        // Calculate sky offset based on player angle
        double skyOffset = (player.angle / (2 * M_PI)) * 512.0;
        int skyX = (int)skyOffset % 512;
        
        SDL_Rect srcRect = {skyX, 0, SCREEN_WIDTH, 256};
        SDL_Rect dstRect = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT / 2};
        SDL_RenderCopy(renderer, skyTex, &srcRect, &dstRect);
    } else {
        // Fallback: solid color sky
        SDL_SetRenderDrawColor(renderer, 135, 206, 235, 255);
        SDL_Rect skyRect = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT / 2};
        SDL_RenderFillRect(renderer, &skyRect);
    }
}

void Raycaster::renderWalls() {
    renderWallsThreaded();
}

void Raycaster::renderWallsThreaded() {
    // Pre-calculate all ray data
    std::vector<double> distances(SCREEN_WIDTH);
    std::vector<int> wallTypes(SCREEN_WIDTH);
    std::vector<double> wallXs(SCREEN_WIDTH);
    std::vector<int> wallTops(SCREEN_WIDTH);
    std::vector<int> wallBottoms(SCREEN_WIDTH);
    
    // Calculate ray data for all columns
    for (int x = 0; x < SCREEN_WIDTH; x++) {
        double rayAngle = player.angle - FOV/2 + (x * FOV / SCREEN_WIDTH);
        castRay(rayAngle, x, distances[x], wallTypes[x], wallXs[x]);
        
        // Calculate wall height
        int wallHeight = (int)(SCREEN_HEIGHT / distances[x]);
        wallTops[x] = (SCREEN_HEIGHT - wallHeight) / 2;
        wallBottoms[x] = wallTops[x] + wallHeight;
    }
    
    // Draw ceiling (single-threaded, fast)
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    for (int x = 0; x < SCREEN_WIDTH; x++) {
        SDL_RenderDrawLine(renderer, x, 0, x, wallTops[x]);
    }
    
    // Render walls and floors in parallel
    renderThreads.clear();
    int columnsPerThread = SCREEN_WIDTH / NUM_THREADS;
    
    for (int i = 0; i < NUM_THREADS; i++) {
        int startX = i * columnsPerThread;
        int endX = (i == NUM_THREADS - 1) ? SCREEN_WIDTH : (i + 1) * columnsPerThread;
        
        renderThreads.emplace_back([this, startX, endX, &distances, &wallTypes, &wallXs, &wallTops, &wallBottoms]() {
            for (int x = startX; x < endX; x++) {
                renderWallColumn(x, distances[x], wallTypes[x], wallXs[x], wallTops[x], wallBottoms[x]);
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : renderThreads) {
        thread.join();
    }
}

void Raycaster::renderWallColumn(int x, double distance, int wallType, double wallX, int wallTop, int wallBottom) {
    // Get wall texture
    SDL_Texture* wallTex = textureManager.getWallTexture(wallType);
    if (wallTex) {
        // Calculate texture X coordinate
        wallX -= floor(wallX);
        int texX = (int)(wallX * textureManager.getTextureSize());
        
        // Render textured wall column
        SDL_Rect srcRect = {texX, 0, 1, textureManager.getTextureSize()};
        SDL_Rect dstRect = {x, wallTop, 1, wallBottom - wallTop};
        
        // Apply distance-based shading
        Uint8 intensity = (Uint8)(255 * (1.0 - distance / MAX_DISTANCE));
        intensity = std::max(intensity, (Uint8)50);
        SDL_SetTextureColorMod(wallTex, intensity, intensity, intensity);
        
        // Thread-safe rendering
        std::lock_guard<std::mutex> lock(renderMutex);
        SDL_RenderCopy(renderer, wallTex, &srcRect, &dstRect);
    } else {
        // Fallback: solid color wall
        Uint8 intensity = (Uint8)(255 * (1.0 - distance / MAX_DISTANCE));
        intensity = std::max(intensity, (Uint8)50);
        SDL_SetRenderDrawColor(renderer, intensity, intensity, intensity, 255);
        
        std::lock_guard<std::mutex> lock(renderMutex);
        SDL_RenderDrawLine(renderer, x, wallTop, x, wallBottom);
    }
    
    // Draw floor (below walls) - render each pixel individually for proper perspective
    int floorHeight = SCREEN_HEIGHT - wallBottom;
    if (floorHeight > 0) {
        SDL_Texture* floorTex = textureManager.getFloorTexture();
        double rayAngle = player.angle - FOV/2 + (x * FOV / SCREEN_WIDTH);
        
        for (int y = wallBottom; y < SCREEN_HEIGHT; y++) {
            // Calculate the distance to the floor at this screen Y coordinate
            double floorDistance = (SCREEN_HEIGHT / 2.0) / (y - SCREEN_HEIGHT / 2.0);
            
            // Calculate world position of floor point
            double floorX = player.x + cos(rayAngle) * floorDistance;
            double floorY = player.y + sin(rayAngle) * floorDistance;
            
            // Calculate texture coordinates
            int texX = (int)(floorX * textureManager.getTextureSize()) % textureManager.getTextureSize();
            int texY = (int)(floorY * textureManager.getTextureSize()) % textureManager.getTextureSize();
            
            if (texX < 0) texX += textureManager.getTextureSize();
            if (texY < 0) texY += textureManager.getTextureSize();
            
            if (floorTex) {
                // Get pixel from texture
                SDL_Rect srcRect = {texX, texY, 1, 1};
                SDL_Rect dstRect = {x, y, 1, 1};
                
                // Apply distance-based shading
                Uint8 intensity = (Uint8)(255 * (1.0 - floorDistance / MAX_DISTANCE));
                intensity = std::max(intensity, (Uint8)30);
                SDL_SetTextureColorMod(floorTex, intensity, intensity, intensity);
                
                std::lock_guard<std::mutex> lock(renderMutex);
                SDL_RenderCopy(renderer, floorTex, &srcRect, &dstRect);
            } else {
                // Fallback: solid color floor
                Uint8 intensity = (Uint8)(255 * (1.0 - floorDistance / MAX_DISTANCE));
                intensity = std::max(intensity, (Uint8)30);
                SDL_SetRenderDrawColor(renderer, intensity/2, intensity, intensity/2, 255);
                
                std::lock_guard<std::mutex> lock(renderMutex);
                SDL_RenderDrawPoint(renderer, x, y);
            }
        }
    }
}


void Raycaster::drawMinimap() {
    int minimapSize = 200;
    int minimapX = SCREEN_WIDTH - minimapSize - 10;
    int minimapY = 10;
    
    map.renderMinimap(renderer, minimapX, minimapY, minimapSize);
    
    // Draw player
    int playerMinimapX = minimapX + (int)(player.x * minimapSize / MAP_WIDTH);
    int playerMinimapY = minimapY + (int)(player.y * minimapSize / MAP_HEIGHT);
    
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_Rect playerRect = {playerMinimapX - 2, playerMinimapY - 2, 4, 4};
    SDL_RenderFillRect(renderer, &playerRect);
    
    // Draw player direction
    int dirX = playerMinimapX + (int)(cos(player.angle) * 10);
    int dirY = playerMinimapY + (int)(sin(player.angle) * 10);
    SDL_RenderDrawLine(renderer, playerMinimapX, playerMinimapY, dirX, dirY);
}

void Raycaster::run() {
    while (running) {
        handleInput();
        render();
        SDL_Delay(16); // ~60 FPS
    }
}

void Raycaster::handleMouseInput() {
    int mouseX, mouseY;
    SDL_GetMouseState(&mouseX, &mouseY);
    
    if (lastMouseX != 0) {
        int deltaX = mouseX - lastMouseX;
        
        // Horizontal rotation (yaw) - mouse X movement only
        player.rotate(deltaX * mouseSensitivity);
        
        // Reset mouse to center to prevent cursor from leaving window
        SDL_WarpMouseInWindow(window, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
        lastMouseX = SCREEN_WIDTH / 2;
    } else {
        lastMouseX = mouseX;
    }
}

void Raycaster::captureMouse() {
    SDL_SetRelativeMouseMode(SDL_TRUE);
    SDL_WarpMouseInWindow(window, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
    lastMouseX = SCREEN_WIDTH / 2;
    mouseCaptured = true;
    std::cout << "Mouse captured! Move mouse left/right to look around. ESC to release." << std::endl;
}

void Raycaster::releaseMouse() {
    SDL_SetRelativeMouseMode(SDL_FALSE);
    mouseCaptured = false;
    std::cout << "Mouse released! Click to capture again." << std::endl;
}

void Raycaster::cleanup() {
    if (renderer) {
        SDL_DestroyRenderer(renderer);
        renderer = nullptr;
    }
    if (window) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }
    SDL_Quit();
}
