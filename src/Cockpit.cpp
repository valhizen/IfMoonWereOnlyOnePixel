#include "Cockpit.hpp"
#include "readTexture.hpp" 
#include <iostream>

Cockpit::Cockpit() : shader(nullptr), textureID(0), quadVAO(0), quadVBO(0) {}

Cockpit::~Cockpit() {
    if (shader) delete shader;
    if (quadVAO) glDeleteVertexArrays(1, &quadVAO);
    if (quadVBO) glDeleteBuffers(1, &quadVBO);
}

void Cockpit::init() {
    // Load shader
    try {
        shader = new Shader("shader/cockpit.vert", "shader/cockpit.frag");
    } catch (const std::exception& e) {
        std::cerr << "Failed to init cockpit shader: " << e.what() << std::endl;
        return;
    }
    
    // Load texture
    textureID = loadTexture("assets/cockpit_overlay.png");
    if (textureID == 0) {
        std::cerr << "Failed to load cockpit texture!" << std::endl;
    }
    
    setupQuad();
}

void Cockpit::setupQuad() {
    // Standard full screen quad
    float quadVertices[] = {
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
        
        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };
    
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
}

void Cockpit::render() {
    if (!shader || textureID == 0) return;
    
    // Disable depth test so it draws on top (or just ensure it's drawn last)
    // Actually, we want it to be an overlay, so disable depth test
    GLboolean depthEnabled;
    glGetBooleanv(GL_DEPTH_TEST, &depthEnabled);
    glDisable(GL_DEPTH_TEST);
    
    shader->use();
    shader->setInt("cockpitTexture", 0);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    
    glDisable(GL_BLEND);
    if (depthEnabled) glEnable(GL_DEPTH_TEST);
}
