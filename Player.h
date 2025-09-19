#pragma once

class Player {
public:
    double x, y;        // Position
    double angle;       // Viewing angle
    double moveSpeed;   // Movement speed
    double rotSpeed;    // Rotation speed
    
    Player();
    void move(double dx, double dy);
    void rotate(double deltaAngle);
    void update();
};
