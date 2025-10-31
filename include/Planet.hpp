#ifndef PLANET_HPP
#define PLANET_HPP

#include <glm/glm.hpp>
#include <string>
#include "Shader.hpp"

class Planet {
public:
    Planet(float diameterKM, std::string planetName, float zPosition, 
           glm::vec3 planetColor, const char* texturePath = nullptr);
    ~Planet();

    void renderSphere(const glm::mat4& view, const glm::mat4& projection, 
                     glm::vec3 campos);
    void update(float deltaTime);
    
    // Orbital mechanics
    void setOrbitParent(Planet* parent, float orbitRadius, float orbitSpeed = 0.0f);
    
    // Getters
    glm::vec3 getPosition() const;
    std::string getName() const;
    float getRadius() const;
    float getDiameterKM() const;
    float getMass() const { return mass; }
    void setPosition(const glm::vec3& pos) { position = pos; }

    
    bool inverted;

private:
    void GenerateSphere();
    unsigned int loadTexture(const char* path);
    
    // Rendering
    unsigned int sphereVAO, vbo, ebo;
    unsigned int indexCount;
    unsigned int textureID;
    Shader* shader;
    
    // Physical properties
    std::string name;
    glm::vec3 color;
    float radius;
    float diameterInKM;
    float mass; // For gravitational calculations
    glm::vec3 position;
    
    // Orbital mechanics
    Planet* orbitParent = nullptr;        // Body this planet orbits around
    float orbitRadius;          // Distance from parent
    float orbitSpeed;           // Angular velocity (radians/second)
    float orbitAngle;           // Current angle in orbit
    glm::vec3 orbitPlaneNormal; // Normal vector of orbital plane (for tilted orbits)
};

#endif
