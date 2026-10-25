#pragma once
#include <whiskers/core/Entity.h>
#include <glm/glm.hpp>

namespace astro {

class Ship {
public:
    Ship();
    ~Ship();
    
    void update(float deltaTime);
    void thrust(bool isThrusting);
    void rotate(float rotationSpeed);
    void fireBullet();
    
    glm::vec2 getPosition() const { return m_position; }
    glm::vec2 getVelocity() const { return m_velocity; }
    float getRotation() const { return m_rotation; }
    bool isThrusting() const { return m_isThrusting; }
    
    void setPosition(const glm::vec2& position) { m_position = position; }
    
private:
    glm::vec2 m_position;
    glm::vec2 m_velocity;
    float m_rotation;
    bool m_isThrusting;
    
    static const float THRUST_POWER;
    static const float ROTATION_SPEED;
    static const float DRAG;
    static const float MAX_SPEED;
};

}  // namespace astro
