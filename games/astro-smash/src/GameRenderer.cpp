#include "astro-smash/GameRenderer.h"
#include "astro-smash/Ship.h"
#include "astro-smash/Bullet.h"
#include <whiskers/core/Entity.h>
#include <iostream>

namespace astro {

GameRenderer::GameRenderer(int width, int height)
    : m_width(width)
    , m_height(height)
{
    m_renderer = std::make_unique<whiskers::Renderer>(width, height);
}

GameRenderer::~GameRenderer() = default;

bool GameRenderer::initialize() {
    if (!m_renderer->init()) {
        std::cerr << "Failed to initialize base renderer" << std::endl;
        return false;
    }
    
    std::cout << "GameRenderer initialized successfully" << std::endl;
    return true;
}

void GameRenderer::clear() {
    m_renderer->clear();
}

void GameRenderer::present() {
    m_renderer->present();
}

void GameRenderer::renderShip(const Ship& ship) {
    // Create a temporary entity for rendering
    Entity shipEntity;
    shipEntity.position = ship.getPosition();
    shipEntity.rotation = ship.getRotation();
    
    m_renderer->renderShip(shipEntity, ship.isThrusting());
}

void GameRenderer::renderBullet(const Bullet& bullet) {
    // Create a temporary entity for rendering
    Entity bulletEntity;
    bulletEntity.position = bullet.getPosition();
    bulletEntity.rotation = 0.0f;  // Bullets don't rotate
    
    m_renderer->renderBullet(bulletEntity);
}

void GameRenderer::renderAsteroid(const glm::vec2& position, float size) {
    // Create a temporary entity for rendering
    Entity asteroidEntity;
    asteroidEntity.position = position;
    asteroidEntity.rotation = 0.0f;
    
    m_renderer->renderAsteroid(asteroidEntity);
}

}  // namespace astro
