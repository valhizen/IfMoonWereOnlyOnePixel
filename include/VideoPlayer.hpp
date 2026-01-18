#ifndef VIDEOPLAYER_HPP
#define VIDEOPLAYER_HPP

#include <glad/glad.h>
#include <string>

class VideoPlayer {
public:
    VideoPlayer();
    ~VideoPlayer();
    
    bool load(const std::string& path);
    void update(float deltaTime);
    void render();
    
    bool isFinished() const { return m_Finished; }
    void reset();

private:
    struct InternalData;
    InternalData* m_Data;
    
    unsigned int m_TextureID;
    unsigned int m_VAO, m_VBO;
    unsigned int m_ShaderProg;
    
    bool m_Finished;
    float m_TimeAccumulator;
    
    void setupQuad();
    void setupShader();
};

#endif
