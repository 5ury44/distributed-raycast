#pragma once
#include <SDL2/SDL.h>
#include <string>
#include <vector>

class TextureManager {
private:
    std::vector<SDL_Texture*> wallTextures;
    SDL_Texture* skyTexture;
    SDL_Texture* floorTexture;
    int textureSize;
    
public:
    TextureManager();
    ~TextureManager();
    
    bool loadTextures(SDL_Renderer* renderer);
    SDL_Texture* getWallTexture(int index) const;
    SDL_Texture* getSkyTexture() const;
    SDL_Texture* getFloorTexture() const;
    int getTextureSize() const;
    
private:
    SDL_Texture* loadTexture(SDL_Renderer* renderer, const std::string& path);
    void createDefaultTextures(SDL_Renderer* renderer);
};
