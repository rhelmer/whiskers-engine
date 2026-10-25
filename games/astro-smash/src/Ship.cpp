#include "astro-smash/Ship.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <iostream>

namespace astro {

const float Ship::THRUST_POWER = 200.0f;
const float Ship::ROTATION_SPEED = 180.0f;  // degrees per second
const float Ship::DRAG = 0.98f;
const float Ship::MAX_SPEED = 300.0f;

Ship::Ship() 
    : m_position(400.0f, 300.0f)  // Center of 800x600 screen
    , m_velocity(0.0f, 0.0f)
    , m_rotation(0.0f)
    , m_isThrusting(false)
{
}

Ship::~Ship() = default;

void Ship::update(float deltaTime) {
    // Apply thrust if thrusting
    if (m_isThrusting) {
        float radians = glm::radians(m_rotation);
        glm::vec2 thrustDirection(std::sin(radians), -std::cos(radians));
        m_velocity += thrustDirection * THRUST_POWER * deltaTime;
    }
    
    // Apply drag
    m_velocity *= DRAG;
    
    // Limit maximum speed
    float speed = glm::length(m_velocity);
    if (speed > MAX_SPEED) {
        m_velocity = glm::normalize(m_velocity) * MAX_SPEED;
    }
    
    // Update position
    m_position += m_velocity * deltaTime;
    
    // Wrap around screen boundaries (assuming 800x600 screen)
    if (m_position.x < 0) m_position.x = 800;
    if (m_position.x > 800) m_position.x = 0;
    if (m_position.y < 0) m_position.y = 600;
    if (m_position.y > 600) m_position.y = 0;
}

void Ship::thrust(bool isThrusting) {
    m_isThrusting = isThrusting;
}

void Ship::rotate(float rotationSpeed) {
    m_rotation += rotationSpeed * ROTATION_SPEED;
    
    // Keep rotation in 0-360 range
    while (m_rotation >= 360.0f) m_rotation -= 360.0f;
    while (m_rotation < 0.0f) m_rotation += 360.0f;
}

void Ship::fireBullet() {
    // TODO: Create bullet and add to game world
    std::cout << "Ship firing bullet (not fully implemented)" << std::endl;
}

}  // namespace astro
