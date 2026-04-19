#include "astro-smash/AstroSmashApp.h"
#include "astro-smash/Ship.h"
#include "astro-smash/GameRenderer.h"
#include <whiskers/core/EntityManager.h>
#include <whiskers/core/Entity.h>
#include <iostream>

namespace astro {

AstroSmashApp::AstroSmashApp() 
    : m_shipEntityId(0)
    , m_thrusting(false)
{
}

AstroSmashApp::~AstroSmashApp() = default;

bool AstroSmashApp::onInitialize() {
    // Create game components
    m_ship = std::make_unique<Ship>();
    m_gameRenderer = std::make_unique<GameRenderer>(800, 600);
    m_physicsSystem = std::make_unique<whiskers::PhysicsSystem>();
    
    if (!m_gameRenderer->initialize()) {
        std::cerr << "Failed to initialize game renderer" << std::endl;
        return false;
    }
    
    // Create ship entity
    Entity shipEntity;
    shipEntity.position = m_ship->getPosition();
    shipEntity.rotation = m_ship->getRotation();
    m_shipEntityId = m_entityManager->createEntity(shipEntity);
    
    std::cout << "AstroSmash game initialized successfully" << std::endl;
    return true;
}

void AstroSmashApp::onUpdate(float deltaTime) {
    handleInput(deltaTime);
    
    // Update ship
    m_ship->update(deltaTime);
    
    // Update ship entity
    auto& entities = m_entityManager->getEntities();
    if (m_shipEntityId < entities.size()) {
        entities[m_shipEntityId].position = m_ship->getPosition();
        entities[m_shipEntityId].rotation = m_ship->getRotation();
    }
    
    // Update physics
    m_physicsSystem->update(entities, deltaTime);
}

void AstroSmashApp::onRender() {
    m_gameRenderer->clear();
    
    if (m_ship) {
        m_gameRenderer->renderShip(*m_ship);
    }
    
    m_gameRenderer->present();
}

void AstroSmashApp::onEvent(const whiskers::Event& event) {
    // Handle quit event
    if (event.type == whiskers::Event::Quit) {
        // Application base class will handle this
        return;
    }
    
    // Handle key events for ship controls
    if (event.type == whiskers::Event::KeyPress) {
        switch (event.key) {
            case SDLK_UP:
            case SDLK_w:
                m_thrusting = true;
                break;
            case SDLK_SPACE:
                fireBullet();
                break;
        }
    } else if (event.type == whiskers::Event::KeyRelease) {
        switch (event.key) {
            case SDLK_UP:
            case SDLK_w:
                m_thrusting = false;
                break;
        }
    }
}

void AstroSmashApp::onShutdown() {
    std::cout << "Shutting down AstroSmash game" << std::endl;
}

void AstroSmashApp::handleInput(float deltaTime) {
    if (!m_ship) return;
    
    // Handle continuous input (rotation)
    // This is a simplified approach - in a real game you might use 
    // the Input system for more sophisticated input handling
    
    // Update ship thrust state
    m_ship->thrust(m_thrusting);
}

void AstroSmashApp::fireBullet() {
    if (m_ship) {
        m_ship->fireBullet();
    }
}

void AstroSmashApp::spawnAsteroid() {
    // TODO: Implement asteroid spawning
    std::cout << "Spawning asteroid (not implemented yet)" << std::endl;
}

}  // namespace astro
