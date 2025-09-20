#include "Player.h"
#include <cmath>

Player::Player() {
    x = 2.0;
    y = 2.0;
    angle = 0.0;
    pitch = 0.0;
    moveSpeed = 0.1;
    rotSpeed = 0.05;
}

void Player::move(double dx, double dy) {
    x += dx;
    y += dy;
}

void Player::rotate(double deltaAngle) {
    angle += deltaAngle;
}

void Player::pitchUp(double deltaPitch) {
    pitch -= deltaPitch;
    // Limit pitch to prevent over-rotation
    if (pitch > M_PI/2) pitch = M_PI/2;
    if (pitch < -M_PI/2) pitch = -M_PI/2;
}

void Player::update() {
    // Player update logic can be added here if needed
}
