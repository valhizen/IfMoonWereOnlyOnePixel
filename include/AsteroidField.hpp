#ifndef ASTEROIDFIELD_HPP
#define ASTEROIDFIELD_HPP

#include <vector>
#include <glm/glm.hpp>
#include "Planet.hpp"

class AsteroidField {
public:
    AsteroidField(int count, float minRad, float maxRad);
    ~AsteroidField();

    void update(float deltaTime, float timeScale);
    void render(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& camPos);

private:
    struct Asteroid {
        float orbitRadius;
        float angle;
        float orbitSpeed;
        float rotation;
        float rotationSpeed;
        float scale;
        glm::vec3 axis;
        float yOffset;
    };

    std::vector<Asteroid> m_Asteroids;
    Planet* m_BaseAsteroid;
};

#endif
