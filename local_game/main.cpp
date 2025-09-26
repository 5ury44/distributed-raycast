#include "Raycaster.h"
#include <iostream>

int main(int /*argc*/, char* /*argv*/[]) {
    Raycaster game;
    
    if (!game.initialize()) {
        std::cerr << "Failed to initialize game!" << std::endl;
        return 1;
    }
    
    game.run();
    return 0;
}
