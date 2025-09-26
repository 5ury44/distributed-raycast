#include "TextureManager.h"
#include <iostream>

TextureManager::TextureManager() : textureSize(64) {
    wallTextures.resize(6); // 6 different wall textures
    skyTexture = nullptr;
    floorTexture = nullptr;
}

TextureManager::~TextureManager() {
    for (auto texture : wallTextures) {
        if (texture) {
            SDL_DestroyTexture(texture);
        }
    }
    if (skyTexture) {
        SDL_DestroyTexture(skyTexture);
    }
    if (floorTexture) {
        SDL_DestroyTexture(floorTexture);
    }
}

bool TextureManager::loadTextures(SDL_Renderer* renderer) {
    // Try to load external textures first
    bool loadedExternal = true;
    
    // Load wall textures
    std::vector<std::string> wallPaths = {
        "textures/wall_grass.png",
        "textures/wall_rock.png", 
        "textures/wall_stone.png",
        "textures/wall_wood.png",
        "textures/wall_dirt.png",
        "textures/wall_brick.png"
    };
    
    for (size_t i = 0; i < wallTextures.size(); i++) {
        wallTextures[i] = loadTexture(renderer, wallPaths[i]);
        if (!wallTextures[i]) {
            loadedExternal = false;
            break;
        }
    }
    
    // Load sky texture
    skyTexture = loadTexture(renderer, "textures/sky.png");
    if (!skyTexture) {
        loadedExternal = false;
    }
    
    // Load floor texture
    floorTexture = loadTexture(renderer, "textures/floor.png");
    if (!floorTexture) {
        loadedExternal = false;
    }
    
    // If external textures failed, create default ones
    if (!loadedExternal) {
        std::cout << "External textures not found, creating default textures..." << std::endl;
        createDefaultTextures(renderer);
    }
    
    return true;
}

SDL_Texture* TextureManager::loadTexture(SDL_Renderer* renderer, const std::string& path) {
    SDL_Surface* surface = SDL_LoadBMP(path.c_str());
    if (!surface) {
        // Try PNG
        surface = SDL_LoadBMP((path.substr(0, path.length() - 4) + ".bmp").c_str());
    }
    
    if (!surface) {
        return nullptr;
    }
    
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    
    return texture;
}

void TextureManager::createDefaultTextures(SDL_Renderer* renderer) {
    // Create default wall textures with different colors
    for (size_t i = 0; i < wallTextures.size(); i++) {
        SDL_Surface* surface = SDL_CreateRGBSurface(0, textureSize, textureSize, 32, 0, 0, 0, 0);
        if (surface) {
            // Fill with different colors for each texture
            Uint32 colors[] = {
                0xFF4A4A4A, // Gray (stone)
                0xFF8B4513, // Brown (wood)
                0xFF228B22, // Green (grass)
                0xFF696969, // Dark gray (rock)
                0xFF8B4513, // Brown (dirt)
                0xFFB22222  // Red (brick)
            };
            
            SDL_FillRect(surface, nullptr, colors[i]);
            
            // Add some pattern
            for (int y = 0; y < textureSize; y += 8) {
                for (int x = 0; x < textureSize; x += 8) {
                    if ((x + y) % 16 == 0) {
                        SDL_Rect rect = {x, y, 4, 4};
                        SDL_FillRect(surface, &rect, colors[i] + 0x202020);
                    }
                }
            }
            
            wallTextures[i] = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_FreeSurface(surface);
        }
    }
    
    // Create default sky texture
    SDL_Surface* skySurface = SDL_CreateRGBSurface(0, 512, 256, 32, 0, 0, 0, 0);
    if (skySurface) {
        // Create gradient sky
        for (int y = 0; y < 256; y++) {
            Uint8 intensity = (Uint8)(135 - y * 0.3); // Blue gradient
            Uint32 color = SDL_MapRGB(skySurface->format, intensity/3, intensity/2, intensity);
            SDL_Rect rect = {0, y, 512, 1};
            SDL_FillRect(skySurface, &rect, color);
        }
        skyTexture = SDL_CreateTextureFromSurface(renderer, skySurface);
        SDL_FreeSurface(skySurface);
    }
    
    // Create default floor texture
    SDL_Surface* floorSurface = SDL_CreateRGBSurface(0, textureSize, textureSize, 32, 0, 0, 0, 0);
    if (floorSurface) {
        SDL_FillRect(floorSurface, nullptr, 0xFF2F4F2F); // Dark green
        floorTexture = SDL_CreateTextureFromSurface(renderer, floorSurface);
        SDL_FreeSurface(floorSurface);
    }
}

SDL_Texture* TextureManager::getWallTexture(int index) const {
    if (index >= 0 && index < static_cast<int>(wallTextures.size())) {
        return wallTextures[index];
    }
    return wallTextures[0]; // Default to first texture
}

SDL_Texture* TextureManager::getSkyTexture() const {
    return skyTexture;
}

SDL_Texture* TextureManager::getFloorTexture() const {
    return floorTexture;
}

int TextureManager::getTextureSize() const {
    return textureSize;
}
