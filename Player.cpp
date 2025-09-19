#include "Player.h"
#include <cmath>

Player::Player() {
    x = 2.0;
    y = 2.0;
    angle = 0.0;
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

void Player::update() {
    // Player update logic can be added here if needed
}
