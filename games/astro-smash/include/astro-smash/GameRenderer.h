#pragma once
#include <whiskers/rendering/Renderer.h>
#include <memory>
#include <vector>

namespace astro {

class Ship;
class Bullet;

class GameRenderer {
public:
    GameRenderer(int width, int height);
    ~GameRenderer();
    
    bool initialize();
    void clear();
    void present();
    
    void renderShip(const Ship& ship);
    void renderBullet(const Bullet& bullet);
    void renderAsteroid(const glm::vec2& position, float size);
    
private:
    std::unique_ptr<whiskers::Renderer> m_renderer;
    int m_width;
    int m_height;
};

}  // namespace astro
