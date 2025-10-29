#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include "imgui_impl_glfw.h"

class Application {
private:
    GLFWwindow* m_Window;
    int m_Width;
    int m_Height;
    
public:
    Application(int width, int height, const char* title);
    void Run();
    ~Application();
};

#endif
