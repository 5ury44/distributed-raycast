#pragma once

class Player {
public:
    double x, y;        // Position
    double angle;       // Viewing angle (yaw)
    double pitch;       // Pitch angle (looking up/down)
    double moveSpeed;   // Movement speed
    double rotSpeed;    // Rotation speed
    
    Player();
    void move(double dx, double dy);
    void rotate(double deltaAngle);
    void pitchUp(double deltaPitch);
    void update();
};
