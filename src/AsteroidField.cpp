#include "AsteroidField.hpp"
#include <random>
#include <cmath>
#include <iostream>

AsteroidField::AsteroidField(int count, float minRad, float maxRad) {
    // We use a small planet as our base mesh/texture
    // Diameter ~500km (enough to be visible as specks at the 1px=Moon scale)
    m_BaseAsteroid = new Planet(500.0f, "Asteroid", 0.0f, glm::vec3(0.5f, 0.45f, 0.4f), nullptr);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> distRad(minRad, maxRad);
    std::uniform_real_distribution<float> distAngle(0.0f, 6.28318f);
    std::uniform_real_distribution<float> distSpeed(0.005f, 0.015f);
    std::uniform_real_distribution<float> distScale(2.5f, 7.0f);
    std::uniform_real_distribution<float> distY(-3000.0f, 3000.0f); // More thickness to the belt

    for (int i = 0; i < count; ++i) {
        Asteroid a;
        a.orbitRadius = distRad(gen);
        a.angle = distAngle(gen);
        a.orbitSpeed = distSpeed(gen);
        a.rotation = distAngle(gen);
        a.rotationSpeed = distAngle(gen) * 0.1f;
        a.scale = distScale(gen);
        a.yOffset = distY(gen);
        
        // Random axis
        std::uniform_real_distribution<float> distAxis(-1.0f, 1.0f);
        a.axis = glm::normalize(glm::vec3(distAxis(gen), distAxis(gen), distAxis(gen)));
        
        m_Asteroids.push_back(a);
    }
}

AsteroidField::~AsteroidField() {
    delete m_BaseAsteroid;
}

void AsteroidField::update(float deltaTime, float timeScale) {
    for (auto& a : m_Asteroids) {
        a.angle += a.orbitSpeed * 0.01f * deltaTime * timeScale;
        a.rotation += a.rotationSpeed * deltaTime * timeScale;
    }
}

void AsteroidField::render(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& camPos) {
    if (!m_BaseAsteroid) return;

    for (const auto& a : m_Asteroids) {
        // Calculate position in the ring
        float x = cos(a.angle) * a.orbitRadius;
        float z = sin(a.angle) * a.orbitRadius;
        glm::vec3 pos = glm::vec3(x, a.yOffset, z);
        
        // Simple culling: don't render if too far (improves FPS)
        glm::vec3 diff = pos - camPos;
        float distSq = glm::dot(diff, diff);
        if (distSq > 1000000.0f * 1000000.0f) continue; 

        m_BaseAsteroid->setPosition(pos);
        m_BaseAsteroid->setRadiusScale(a.scale);
        
        m_BaseAsteroid->renderSphere(view, projection, camPos);
    }
    
    // Reset scale
    m_BaseAsteroid->setRadiusScale(1.0f);
}
