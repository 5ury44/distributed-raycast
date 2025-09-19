#!/bin/bash

# Build and run the raycasting game
echo "Building raycasting game..."
make clean && make

if [ $? -eq 0 ]; then
    echo "Build successful! Starting game..."
    echo "Controls:"
    echo "  WASD - Move"
    echo "  Left/Right arrows - Turn"
    echo "  Close window to quit"
    echo ""
    ./raycast_game
else
    echo "Build failed!"
    exit 1
fi
