CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
SDL2_CFLAGS = $(shell pkg-config --cflags sdl2)
SDL2_LIBS = $(shell pkg-config --libs sdl2)
LIBS = $(SDL2_LIBS)
TARGET = raycast_game
SOURCES = main.cpp Player.cpp Map.cpp TextureManager.cpp Raycaster.cpp

# Default target
all: $(TARGET)

# Build the game
$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(SDL2_CFLAGS) -o $(TARGET) $(SOURCES) $(LIBS)

# Clean build files
clean:
	rm -f $(TARGET)

# Install SDL2 (macOS)
install-deps:
	brew install sdl2

# Run the game
run: $(TARGET)
	./$(TARGET)

# Phony targets
.PHONY: all clean install-deps run
