#include "VideoPlayer.hpp"
#define PL_MPEG_IMPLEMENTATION
#include "pl_mpeg.h"
#include <iostream>
#include <vector>

struct VideoPlayer::InternalData {
    plm_t* plm = nullptr;
    double videoTime = 0.0;
};

VideoPlayer::VideoPlayer() : m_Data(new InternalData()), m_TextureID(0), m_Finished(false), m_TimeAccumulator(0.0f) {
    setupShader();
    setupQuad();
}

VideoPlayer::~VideoPlayer() {
    if (m_Data->plm) plm_destroy(m_Data->plm);
    delete m_Data;
    glDeleteTextures(1, &m_TextureID);
    glDeleteVertexArrays(1, &m_VAO);
    glDeleteBuffers(1, &m_VBO);
    glDeleteProgram(m_ShaderProg);
}

bool VideoPlayer::load(const std::string& path) {
    std::cout << "VideoPlayer: Loading " << path << "..." << std::endl;
    m_Data->plm = plm_create_with_filename(path.c_str());
    if (!m_Data->plm) {
        std::cerr << "VideoPlayer: Failed to load video: " << path << std::endl;
        return false;
    }
    std::cout << "VideoPlayer: Loaded successfully." << std::endl;
    
    plm_set_audio_enabled(m_Data->plm, FALSE); // Handle audio separately
    
    // Create Texture
    glGenTextures(1, &m_TextureID);
    glBindTexture(GL_TEXTURE_2D, m_TextureID);
    
    int w = plm_get_width(m_Data->plm);
    int h = plm_get_height(m_Data->plm);
    
    // Allocate space
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    return true;
}

void VideoPlayer::update(float deltaTime) {
    if (!m_Data->plm || m_Finished) return;
    
    double fps = plm_get_framerate(m_Data->plm);
    if (fps <= 0.0) fps = 30.0;
    float frameDuration = 1.0f / (float)fps;
    
    m_TimeAccumulator += deltaTime;
    
    plm_frame_t* frame = nullptr;
    bool newFrameDecoded = false;
    
    // Drop frames if we are way behind? Cap iterations to 5
    int iterations = 0;
    while (m_TimeAccumulator >= frameDuration && iterations < 5) {
        frame = plm_decode_video(m_Data->plm);
        m_TimeAccumulator -= frameDuration;
        iterations++;
        newFrameDecoded = true;
        
        if (!frame) {
            std::cout << "VideoPlayer: Video Finished." << std::endl;
            m_Finished = true;
            return;
        }
    }
    
    if (newFrameDecoded && frame) {
        // Upload
        int w = frame->width;
        int h = frame->height;
        std::vector<uint8_t> rgbBuffer(w * h * 4); // RGBA
        
        plm_frame_to_rgba(frame, rgbBuffer.data(), w * 4);
        
        glBindTexture(GL_TEXTURE_2D, m_TextureID);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, rgbBuffer.data());
    }
}

void VideoPlayer::setupQuad() {
    float vertices[] = {
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 0.0f,
        -1.0f, -1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 1.0f,
        
        -1.0f,  1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 1.0f,
         1.0f,  1.0f,  1.0f, 0.0f
    };
    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
}

void VideoPlayer::setupShader() {
    const char* vShaderCode = R"(
        #version 330 core
        layout (location = 0) in vec2 aPos;
        layout (location = 1) in vec2 aTexCoords;
        out vec2 TexCoords;
        void main() {
            gl_Position = vec4(aPos, 0.0, 1.0);
            TexCoords = aTexCoords;
        }
    )";
    
    const char* fShaderCode = R"(
        #version 330 core
        out vec4 FragColor;
        in vec2 TexCoords;
        uniform sampler2D videoTexture;
        void main() {
            FragColor = texture(videoTexture, TexCoords);
        }
    )";
    
    // Compile shaders (simplified, not using Shader class to keep VideoPlayer standalone/simple)
    unsigned int vertex, fragment;
    int success;
    char infoLog[512];
    
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);
    
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    
    m_ShaderProg = glCreateProgram();
    glAttachShader(m_ShaderProg, vertex);
    glAttachShader(m_ShaderProg, fragment);
    glLinkProgram(m_ShaderProg);
    
    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

void VideoPlayer::render() {
    if (m_TextureID == 0) return;
    
    // Save state
    GLboolean depthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
    GLboolean cullFaceEnabled = glIsEnabled(GL_CULL_FACE);
    GLboolean blendEnabled = glIsEnabled(GL_BLEND);
    
    // Force Opaque Overlay State
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    
    glUseProgram(m_ShaderProg);
    
    // Ensure sampler uses texture unit 0
    glUniform1i(glGetUniformLocation(m_ShaderProg, "videoTexture"), 0);
    
    glBindVertexArray(m_VAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_TextureID);
    
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    // Restore state
    if (depthTestEnabled) glEnable(GL_DEPTH_TEST);
    if (cullFaceEnabled) glEnable(GL_CULL_FACE);
    if (blendEnabled) glEnable(GL_BLEND);
}

void VideoPlayer::reset() {
    m_Finished = false;
    m_TimeAccumulator = 0.0f;
    if (m_Data->plm) plm_rewind(m_Data->plm);
}
