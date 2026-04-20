#include "astro-smash/Bullet.h"

namespace astro {

const float Bullet::BULLET_LIFETIME = 2.0f;  // seconds
const float Bullet::BULLET_SPEED = 400.0f;   // units per second

Bullet::Bullet(const glm::vec2& position, const glm::vec2& velocity)
    : m_position(position)
    , m_velocity(velocity)
    , m_timeAlive(0.0f)
    , m_expired(false)
{
    // Normalize and scale velocity
    if (glm::length(velocity) > 0) {
        m_velocity = glm::normalize(velocity) * BULLET_SPEED;
    } else {
        m_velocity = glm::vec2(0.0f, -BULLET_SPEED);  // Default upward
    }
}

Bullet::~Bullet() = default;

void Bullet::update(float deltaTime) {
    if (m_expired) return;
    
    // Update position
    m_position += m_velocity * deltaTime;
    
    // Update lifetime
    m_timeAlive += deltaTime;
    
    // Check if bullet has expired
    if (m_timeAlive >= BULLET_LIFETIME) {
        m_expired = true;
    }
    
    // Check if bullet is off-screen (assuming 800x600 screen)
    if (m_position.x < -10 || m_position.x > 810 || 
        m_position.y < -10 || m_position.y > 610) {
        m_expired = true;
    }
}

}  // namespace astro
