#include "Planet.hpp"
#include <glad/glad.h>
#include <cmath>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <iostream>

// Moon diameter as reference: 3,474.8 km
const float MOON_DIAMETER_KM = 3474.8f;
const float PIXELS_PER_MOON = 1.0f; // 1 pixel = moon diameter

Planet::Planet(float diameterKM, std::string planetName, float zPosition, glm::vec3 planetColor, const char* texturePath)
    : name(planetName), color(planetColor), diameterInKM(diameterKM) {

    // Convert diameter to radius in pixels
    // Scale: 1 pixel = Moon diameter
    float diameterInPixels = (diameterKM / MOON_DIAMETER_KM) * PIXELS_PER_MOON;
    radius = diameterInPixels / 2.0f;
    
    // Position: planets go further in -Z (away from camera)
    position = glm::vec3(0.0f, 0.0f, zPosition);
    

    if (name == "SkySphere") {
        shader = new Shader("shader/shader.vert", "shader/shader1.frag");
        inverted = true; // render inside-out
    } else {
        shader = new Shader("shader/shader.vert", "shader/shader.frag");
    }
    if (texturePath != nullptr) {
        textureID = loadTexture(texturePath);
    } else {
        textureID = 0; // No texture
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
    
    // Flatten data
    std::vector<float> data;
    for (unsigned int i = 0; i < positions.size(); ++i) {
        data.push_back(positions[i].x);
        data.push_back(positions[i].y);
        data.push_back(positions[i].z);
if (normals.size() > 0)
            {
                data.push_back(normals[i].x);
                data.push_back(normals[i].y);
                data.push_back(normals[i].z);
            }
            if (uv.size() > 0)
            {
                data.push_back(uv[i].x);
                data.push_back(uv[i].y);
            }
    }

    
    glGenVertexArrays(1, &sphereVAO);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    
    glBindVertexArray(sphereVAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
        unsigned int stride = (3 + 2 + 3) * sizeof(float);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3* sizeof(float), (void*)0);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
    
    glBindVertexArray(0);
}

void Planet::renderSphere(const glm::mat4& view, const glm::mat4& projection, 
                          glm::vec3 campos) {
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
    delete shader;
}


unsigned int Planet::loadTexture(char const * path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

