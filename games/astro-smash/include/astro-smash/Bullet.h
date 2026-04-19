#pragma once
#include <whiskers/core/Entity.h>
#include <glm/glm.hpp>

namespace astro {

class Bullet {
public:
    Bullet(const glm::vec2& position, const glm::vec2& velocity);
    ~Bullet();
    
    void update(float deltaTime);
    bool isExpired() const { return m_expired; }
    
    glm::vec2 getPosition() const { return m_position; }
    glm::vec2 getVelocity() const { return m_velocity; }
    
private:
    glm::vec2 m_position;
    glm::vec2 m_velocity;
    float m_timeAlive;
    bool m_expired;
    
    static const float BULLET_LIFETIME;
    static const float BULLET_SPEED;
};

}  // namespace astro
