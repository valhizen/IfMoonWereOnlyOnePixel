#include "Planet.hpp"
#include <glad/glad.h>
#include <cmath>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>
#include "readTexture.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <iostream>
#include <algorithm>

// Moon diameter as reference: 3,474.8 km
const float MOON_DIAMETER_KM = 3474.8f;
const float PIXELS_PER_MOON = 1.0f; // 1 pixel = moon diameter

// In Planet.cpp, replace your constructor with this:

Planet::Planet(float diameterKM, std::string planetName, float zPosition, 
               glm::vec3 planetColor, const char* texturePath)
    : name(planetName), 
      color(planetColor), 
      diameterInKM(diameterKM), 
      shader(nullptr), 
      inverted(false),
      orbitParent(nullptr),    // CRITICAL: Initialize to nullptr
      orbitRadius(0.0f),       // Initialize orbit parameters
      orbitSpeed(0.0f),
      orbitAngle(0.0f) {

    // Convert diameter to radius in pixels
    float diameterInPixels = (diameterKM / MOON_DIAMETER_KM) * PIXELS_PER_MOON;
    radius = diameterInPixels / 2.0f;

    mass = (diameterKM * diameterKM * diameterKM) / 1000.0f;
    
    // Position: planets go further in -Z (away from camera)
    position = glm::vec3(0.0f, 0.0f, zPosition);
    
    try {
        if (name == "SkySphere") {
            shader = new Shader("shader/shader.vert", "shader/shader1.frag");
            inverted = true;
        } else {
            shader = new Shader("shader/shader.vert", "shader/shader.frag");
        }
        
        if (shader == nullptr) {
            throw std::runtime_error("Failed to create shader for " + name);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error creating shader for " << name << ": " << e.what() << std::endl;
        throw;
    }
    
    if (texturePath != nullptr) {
        textureID = loadTexture(texturePath);
    } else {
        textureID = 0;
    }
    
    GenerateSphere();
}

void Planet::GenerateSphere() {
    std::vector<glm::vec3> positions;
    std::vector<unsigned int> indices;
    std::vector<glm::vec2> uv;
    std::vector<glm::vec3> normals;
    
    const unsigned int X_SEGMENTS = 64;
    const unsigned int Y_SEGMENTS = 64;
    const float PI = 3.14159265359f;
    
    // Generate vertices
    for (unsigned int y = 0; y <= Y_SEGMENTS; ++y) {
        for (unsigned int x = 0; x <= X_SEGMENTS; ++x) {
            float xSegment = (float)x / (float)X_SEGMENTS;
            float ySegment = (float)y / (float)Y_SEGMENTS;
            
            float xPos = cos(xSegment * 2.0f * PI) * sin(ySegment * PI);
            float yPos = cos(ySegment * PI);
            float zPos = sin(xSegment * 2.0f * PI) * sin(ySegment * PI);
            
            positions.push_back(glm::vec3(xPos, yPos, zPos));
            normals.push_back(glm::vec3(xPos, yPos, zPos)); // Normal = normalized position for sphere
            uv.push_back(glm::vec2(xSegment, ySegment)); // Generate UV coordinates
        }
    }
    
    // Generate indices
    for (unsigned int y = 0; y < Y_SEGMENTS; ++y) {
        for (unsigned int x = 0; x < X_SEGMENTS; ++x) {
            indices.push_back(y * (X_SEGMENTS + 1) + x);
            indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
            indices.push_back((y + 1) * (X_SEGMENTS + 1) + x + 1);
            
            indices.push_back(y * (X_SEGMENTS + 1) + x);
            indices.push_back((y + 1) * (X_SEGMENTS + 1) + x + 1);
            indices.push_back(y * (X_SEGMENTS + 1) + x + 1);
        }
    }
    
    indexCount = static_cast<unsigned int>(indices.size());
    
    // Flatten data: position (3) + normal (3) + uv (2)
    std::vector<float> data;
    for (unsigned int i = 0; i < positions.size(); ++i) {
        data.push_back(positions[i].x);
        data.push_back(positions[i].y);
        data.push_back(positions[i].z);
        
        data.push_back(normals[i].x);
        data.push_back(normals[i].y);
        data.push_back(normals[i].z);
        
        data.push_back(uv[i].x);
        data.push_back(uv[i].y);
    }
    
    glGenVertexArrays(1, &sphereVAO);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    
    glBindVertexArray(sphereVAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
    
    unsigned int stride = 8 * sizeof(float); // 3 pos + 3 normal + 2 uv
    
    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    
    // Normal attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    
    // UV attribute
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
    
    glBindVertexArray(0);
}

void Planet::renderSphere(const glm::mat4& view, const glm::mat4& projection, 
                          glm::vec3 campos) {
    // Add null check for safety
    if (shader == nullptr) {
        std::cerr << "Error: Shader is null for planet " << name << std::endl;
        return;
    }
    
    shader->use();
    
    // Bind texture if available
    if (textureID != 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);
        shader->setInt("texture1", 0);
        shader->setBool("useTexture", true);
    } else {
        shader->setBool("useTexture", false);
    }
    
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::scale(model, glm::vec3(radius));
    
    shader->setMat4("model", model);
    shader->setMat4("view", view);
    shader->setMat4("projection", projection);
    shader->setVec3("viewPos", campos);
    shader->setFloat("time", (float)glfwGetTime());
    shader->setVec3("objectColor", color);
    
    if (inverted)
        glFrontFace(GL_CW);
    else
        glFrontFace(GL_CCW);
        
    glBindVertexArray(sphereVAO);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    glFrontFace(GL_CCW);
}

glm::vec3 Planet::getPosition() const {
    return position;
}

std::string Planet::getName() const {
    return name;
}

float Planet::getRadius() const {
    return radius;
}

float Planet::getDiameterKM() const {
    return diameterInKM;
}

Planet::~Planet() {
    glDeleteVertexArrays(1, &sphereVAO);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    if (shader != nullptr) {
        delete shader;
    }
}

unsigned int Planet::loadTexture(const char* path)
{
    std::string filePath = path;
    unsigned int textureID;
    glGenTextures(1, &textureID);

    // Detect file extension
    std::string ext;
    size_t dotPos = filePath.find_last_of('.');
    if (dotPos != std::string::npos)
        ext = filePath.substr(dotPos + 1);

    // Convert to lowercase for safety
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == "tif" || ext == "tiff") {
        // --- Load TIFF using libtiff ---
        std::cout << "Loading TIFF texture: " << filePath << std::endl;
        readTiffImage(const_cast<char*>(filePath.c_str()), &textureID);
        return textureID;
    }

    // --- Default: use stb_image ---
    int width, height, nrComponents;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;
        else
            format = GL_RGB;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format,
                     GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cerr << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

// Set orbital parameters
void Planet::setOrbitParent(Planet* parent, float orbitRad, float orbitSpd) {
    orbitParent = parent;
    orbitRadius = orbitRad;
    
    // If orbit speed not specified, calculate from gravitational physics
    if (orbitSpd == 0.0f && parent != nullptr) {
        // Simplified orbital velocity: v = sqrt(G*M/r)
        // We use a scaled G constant for our simulation
        const float G_SCALED = 100.0f; // Tuned for visual effect
        float v = std::sqrt(G_SCALED * parent->getMass() / orbitRadius);
        
        // Convert linear velocity to angular velocity: ω = v/r
        orbitSpeed = v / orbitRadius;
    } else {
        orbitSpeed = orbitSpd;
    }
    
    // Start at a random angle for variety
    orbitAngle = 0.0f;
}
// Update planet position based on orbit
// Update planet position based on orbit
void Planet::update(float deltaTime) {
    // CRITICAL: Only update if this planet has an orbit parent
    if (orbitParent == nullptr) {
        return; // Static body (like the Sun) - don't move
    }
    
    // Increment orbital angle
    orbitAngle += orbitSpeed * deltaTime;
    
    // Keep angle in [0, 2π] range for numerical stability
    const float TWO_PI = 6.28318530718f;
    if (orbitAngle > TWO_PI) {
        orbitAngle -= TWO_PI;
    }
    
    // Calculate position in orbital plane (XZ plane by default)
    float x = orbitRadius * std::cos(orbitAngle);
    float z = orbitRadius * std::sin(orbitAngle);
    
    // Position in orbit relative to parent
    glm::vec3 orbitPos(x, 0.0f, z);
    
    // Add parent's position to get world position
    // This allows nested orbits (Moon follows Earth, Earth follows Sun)
    position = orbitParent->getPosition() + orbitPos;
}
