#ifndef COCKPIT_HPP
#define COCKPIT_HPP

#include "Shader.hpp"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

class Cockpit {
public:
    Cockpit();
    ~Cockpit();
    
    void init();
    void render();
    
private:
    unsigned int quadVAO, quadVBO;
    unsigned int textureID;
    Shader* shader;
    
    void setupQuad();
};

#endif
