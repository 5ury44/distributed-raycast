# Raycasting Game with Textures

A 3D-style raycasting game built in C++ using SDL2, similar to classic games like Wolfenstein 3D. Now with full texture support and modular class architecture!

## Features

- **First-person perspective** with raycasting rendering
- **Textured walls** with 6 different wall types (grass, rock, stone, wood, dirt, brick)
- **Textured skybox** with gradient and cloud effects
- **Textured floor** with tile patterns
- **Player movement** (WASD keys)
- **Camera rotation** (Left/Right arrow keys)
- **Minimap** showing player position and direction
- **Collision detection** with walls
- **Real-time 3D-style rendering** at 60 FPS
- **Modular architecture** with separate classes for Player, Map, TextureManager, and Raycaster

## Controls

- **W** - Move forward
- **S** - Move backward
- **A** - Strafe left
- **D** - Strafe right
- **Left Arrow** - Turn left
- **Right Arrow** - Turn right
- **ESC** - Quit game

## Building and Running

### Prerequisites

You need SDL2 installed on your system:

**macOS:**

```bash
brew install sdl2
```

**Ubuntu/Debian:**

```bash
sudo apt-get install libsdl2-dev
```

**Windows:**
Download SDL2 development libraries from https://www.libsdl.org/

### Build

```bash
make
```

### Run

```bash
make run
```

Or directly:

```bash
./raycast_game
```

### Clean

```bash
make clean
```

## How It Works

The game uses raycasting to create a 3D-like effect:

1. **Raycasting**: For each column of pixels on screen, a ray is cast from the player's position in the viewing direction
2. **DDA Algorithm**: The Digital Differential Analyzer algorithm is used to efficiently find where each ray intersects with walls
3. **Wall Rendering**: Based on the distance to the wall, the height of the wall column is calculated and drawn
4. **Perspective**: The distance is adjusted to prevent fisheye distortion

## Code Structure

### Classes

- **`Player`** - Handles player position, movement, and rotation
- **`Map`** - Manages the 2D map data and collision detection
- **`TextureManager`** - Loads and manages all textures (walls, sky, floor)
- **`Raycaster`** - Main game engine handling rendering, input, and game loop

### Files

- `main.cpp` - Entry point and game initialization
- `Player.h/cpp` - Player class implementation
- `Map.h/cpp` - Map class implementation
- `TextureManager.h/cpp` - Texture management system
- `Raycaster.h/cpp` - Main game engine
- `Makefile` - Build configuration
- `textures/` - Directory containing all texture files
- `create_textures.py` - Script to generate basic textures

### Texture System

- **Wall Textures**: 6 different wall types with procedural patterns
- **Sky Texture**: Gradient sky with cloud effects
- **Floor Texture**: Tiled floor pattern
- **Fallback System**: Creates default textures if external ones aren't found

## Customization

You can modify the game by:

- **Changing the map** in the `Map.cpp` constructor
- **Adjusting screen resolution** by modifying `SCREEN_WIDTH` and `SCREEN_HEIGHT` in `Raycaster.h`
- **Modifying `FOV`** for different field of view angles
- **Changing movement speeds** in the `Player` class constructor
- **Adding new textures** by placing them in the `textures/` directory
- **Creating custom textures** using the `create_textures.py` script
- **Adding new wall types** by extending the `TextureManager` class

## Texture Sources

The game includes basic generated textures, but you can replace them with higher quality ones:

- **Sky textures**: http://www.texturex.com
- **Night sky textures**: https://opengameart.org/content/night-sky-stars-and-galaxies
- **Wall textures**: https://opengameart.org/content/wall-grass-rock-stone-wood-and-dirt-480

Simply place the downloaded textures in the `textures/` directory with the appropriate names.

Enjoy exploring the textured 3D world!
