#!/usr/bin/env python3
"""
Simple texture generator for the raycasting game.
Creates basic BMP textures that can be used by SDL2.
"""

import os
from PIL import Image, ImageDraw
import numpy as np

def create_wall_texture(size, color, pattern_type="brick"):
    """Create a wall texture with a specific pattern."""
    img = Image.new('RGB', (size, size), color)
    draw = ImageDraw.Draw(img)
    
    if pattern_type == "brick":
        # Brick pattern
        for y in range(0, size, size//8):
            offset = (size//16) if (y // (size//8)) % 2 else 0
            for x in range(offset, size, size//4):
                # Draw brick outline
                draw.rectangle([x, y, x + size//4 - 2, y + size//8 - 2], 
                             outline=(color[0]//2, color[1]//2, color[2]//2), width=1)
    elif pattern_type == "stone":
        # Stone pattern
        for y in range(0, size, size//16):
            for x in range(0, size, size//16):
                if (x + y) % 3 == 0:
                    draw.rectangle([x, y, x + size//16, y + size//16], 
                                 fill=(color[0]//2, color[1]//2, color[2]//2))
    elif pattern_type == "wood":
        # Wood grain pattern
        for y in range(0, size, size//32):
            for x in range(0, size, 1):
                if (x + y * 2) % 8 < 2:
                    draw.point((x, y), fill=(color[0]//2, color[1]//2, color[2]//2))
    
    return img

def create_sky_texture(width, height):
    """Create a sky texture with gradient."""
    img = Image.new('RGB', (width, height))
    
    for y in range(height):
        # Create gradient from light blue to darker blue
        intensity = int(255 - (y / height) * 100)
        color = (intensity//3, intensity//2, intensity)
        
        for x in range(width):
            # Add some cloud-like noise
            noise = int(np.random.normal(0, 10))
            r = max(0, min(255, color[0] + noise))
            g = max(0, min(255, color[1] + noise))
            b = max(0, min(255, color[2] + noise))
            img.putpixel((x, y), (r, g, b))
    
    return img

def create_floor_texture(size):
    """Create a floor texture."""
    img = Image.new('RGB', (size, size), (47, 79, 47))  # Dark green
    draw = ImageDraw.Draw(img)
    
    # Add some tile pattern
    for y in range(0, size, size//8):
        for x in range(0, size, size//8):
            if (x + y) % (size//4) == 0:
                draw.rectangle([x, y, x + size//8, y + size//8], 
                             outline=(30, 50, 30), width=1)
    
    return img

def main():
    """Generate all textures."""
    os.makedirs('textures', exist_ok=True)
    
    # Wall textures
    wall_textures = [
        ("wall_grass.bmp", (34, 139, 34), "brick"),      # Green
        ("wall_rock.bmp", (105, 105, 105), "stone"),     # Gray
        ("wall_stone.bmp", (128, 128, 128), "brick"),    # Light gray
        ("wall_wood.bmp", (139, 69, 19), "wood"),        # Brown
        ("wall_dirt.bmp", (160, 82, 45), "stone"),       # Saddle brown
        ("wall_brick.bmp", (178, 34, 34), "brick")       # Red
    ]
    
    print("Creating wall textures...")
    for filename, color, pattern in wall_textures:
        img = create_wall_texture(64, color, pattern)
        img.save(f'textures/{filename}')
        print(f"Created {filename}")
    
    # Sky texture
    print("Creating sky texture...")
    sky_img = create_sky_texture(512, 256)
    sky_img.save('textures/sky.bmp')
    print("Created sky.bmp")
    
    # Floor texture
    print("Creating floor texture...")
    floor_img = create_floor_texture(64)
    floor_img.save('textures/floor.bmp')
    print("Created floor.bmp")
    
    print("\nAll textures created successfully!")
    print("Note: These are basic generated textures. For better quality,")
    print("you can replace them with downloaded textures from the URLs mentioned.")

if __name__ == "__main__":
    main()
