#ifndef PLANET_HPP
#define PLANET_HPP

#include "Camera.hpp"
#include "Shader.hpp"
#include "glm/ext/vector_float3.hpp"
#include <string>
#include <glm/glm.hpp>

class Planet {
private:
    unsigned int sphereVAO;
    unsigned int vbo, ebo;
    unsigned int indexCount;
    unsigned int textureID;
    
    float radius; // In pixels (scaled)
    float diameterInKM; // Store original diameter
    std::string name;
    glm::vec3 color;
    
    void GenerateSphere();
    
public:
    Planet(float diameterKM, std::string planetName, float zPosition, glm::vec3 planetColor = glm::vec3(1.0f, 0.7f, 0.2f), const char* texturePath = nullptr);
    void renderSphere(const glm::mat4& view, const glm::mat4& projection, glm::vec3 campo);
		unsigned int loadTexture(char const * path);
    
    // Getters

    glm::vec3 position;
    glm::vec3 getPosition() const;
    std::string getName() const;
    float getRadius() const;

    float getDiameterKM() const;
		Camera camera;
    Shader* shader;


bool inverted = false;

    ~Planet();
};

#endif
