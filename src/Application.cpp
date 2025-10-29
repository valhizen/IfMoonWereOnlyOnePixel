#include "Application.hpp"
#include "Planet.hpp"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdexcept>
#include <vector>
#include "Camera.hpp"

// Camera - position it far back to see all planets
Camera camera(glm::vec3(0.0f, 200.0f, 800.0f));
float lastX = 800.0f / 2.0;
float lastY = 600.0 / 2.0;
bool firstMouse = true;

// Camera speed modes
// NOTE: For calculation with distance and nothing else
const float MOON_DIAMETER_KM = 3474.8f; // Moon diameter reference
float cameraSpeedMultiplier = 1.0f;
const float NORMAL_SPEED = 20.0f; // Normal speed 
const float LIGHT_SPEED = 299792.458f / MOON_DIAMETER_KM; // Speed of light in pixels/s
bool useLightSpeed = false;

// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    
    // Toggle speed mode with C key
    static bool cKeyPressed = false;
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS && !cKeyPressed) {
        useLightSpeed = !useLightSpeed;
        cameraSpeedMultiplier = useLightSpeed ? LIGHT_SPEED : NORMAL_SPEED;
        cKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_RELEASE) {
        cKeyPressed = false;
    }
    
    // Movement with current speed
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime * cameraSpeedMultiplier);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime * cameraSpeedMultiplier);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime * cameraSpeedMultiplier);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime * cameraSpeedMultiplier);
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);
    
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;
    
    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

Application::Application(int width, int height, const char *title) 
    : m_Width(width), m_Height(height) {
    
    if (!glfwInit()) {
        throw std::runtime_error("Error initializing GLFW");
    }
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    // Primary Monitor will be used
    // GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    // const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    
    m_Window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (m_Window == nullptr)
        throw std::runtime_error("Failed to create window");
    
    glfwMakeContextCurrent(m_Window);
    glfwSwapInterval(1);
    
    glfwSetCursorPosCallback(m_Window, mouse_callback);
    glfwSetScrollCallback(m_Window, scroll_callback);
    glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        throw std::runtime_error("Error initializing GLAD");
    }
    
    glfwSetFramebufferSizeCallback(m_Window, framebuffer_size_callback);
    glEnable(GL_DEPTH_TEST);
    
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    
    ImGui_ImplGlfw_InitForOpenGL(m_Window, true);
    ImGui_ImplOpenGL3_Init("#version 460");
    
    // Initialize camera speed
    cameraSpeedMultiplier = NORMAL_SPEED;
}

// Helper function to project 3D position to screen space
ImVec2 WorldToScreen(const glm::vec3& worldPos, const glm::mat4& view, 
                     const glm::mat4& projection, int screenWidth, int screenHeight) {
    glm::vec4 clipSpace = projection * view * glm::vec4(worldPos, 1.0f);
    
    if (clipSpace.w <= 0.0f) {
        return ImVec2(-1000, -1000); // Behind camera
    }
    
    glm::vec3 ndc = glm::vec3(clipSpace) / clipSpace.w;
    
    float screenX = (ndc.x + 1.0f) * 0.5f * screenWidth;
    float screenY = (1.0f - ndc.y) * 0.5f * screenHeight;
    
    return ImVec2(screenX, screenY);
}

void Application::Run() {
    // Real planet diameters in kilometers and orbital distances
    // Scale: 1 pixel = Moon diameter (3,474.8 km)
    // Actual distances from Sun in million km:
    // Mercury: 57.9, Venus: 108.2, Earth: 149.6, Mars: 227.9, Jupiter: 778.5, Saturn: 1434, Uranus: 2871, Neptune: 4495
    
    const float MOON_DIAMETER_KM = 3474.8f;
    const float PIXELS_PER_MOON = 1.0f;
    
    std::vector<Planet*> planets;
    
    // Sun at origin
    planets.push_back(new Planet(1392000.0f, "Sun", 0.0f, glm::vec3(1.0f, 0.9f, 0.2f)));
    
    // Mercury - 57.9 million km from Sun
    float mercuryDist = (57900000.0f / MOON_DIAMETER_KM) * PIXELS_PER_MOON;
    planets.push_back(new Planet(4879.0f, "Mercury", mercuryDist, glm::vec3(0.7f, 0.7f, 0.7f)));
    
    // Venus - 108.2 million km from Sun
    float venusDist = (108200000.0f / MOON_DIAMETER_KM) * PIXELS_PER_MOON;
    planets.push_back(new Planet(12104.0f, "Venus", venusDist, glm::vec3(0.9f, 0.7f, 0.5f)));
    
    // Earth - 149.6 million km from Sun (1 AU)
    float earthDist = (149600000.0f / MOON_DIAMETER_KM) * PIXELS_PER_MOON;
    planets.push_back(new Planet(12742.0f, "Earth", earthDist, glm::vec3(0.2f, 0.5f, 1.0f)));
    
    // Mars - 227.9 million km from Sun
    float marsDist = (227900000.0f / MOON_DIAMETER_KM) * PIXELS_PER_MOON;
    planets.push_back(new Planet(6779.0f, "Mars", marsDist, glm::vec3(0.8f, 0.3f, 0.2f)));
    
    // Jupiter - 778.5 million km from Sun
    float jupiterDist = (778500000.0f / MOON_DIAMETER_KM) * PIXELS_PER_MOON;
    planets.push_back(new Planet(139820.0f, "Jupiter", jupiterDist, glm::vec3(0.8f, 0.6f, 0.4f)));
    
    // Saturn - 1434 million km from Sun
    float saturnDist = (1434000000.0f / MOON_DIAMETER_KM) * PIXELS_PER_MOON;
    planets.push_back(new Planet(116460.0f, "Saturn", saturnDist, glm::vec3(0.9f, 0.8f, 0.6f)));
    
    // Uranus - 2871 million km from Sun
    float uranusDist = (2871000000.0f / MOON_DIAMETER_KM) * PIXELS_PER_MOON;
    planets.push_back(new Planet(50724.0f, "Uranus", uranusDist, glm::vec3(0.5f, 0.8f, 0.9f)));
    
    // Neptune - 4495 million km from Sun
    float neptuneDist = (4495000000.0f / MOON_DIAMETER_KM) * PIXELS_PER_MOON;
    planets.push_back(new Planet(49244.0f, "Neptune", neptuneDist, glm::vec3(0.2f, 0.3f, 0.8f)));


Planet* skySphere = new Planet(1e8f, "SkySphere", 0.0f, glm::vec3(0.02f, 0.02f, 0.08f));
skySphere->inverted = true;

    
    while (!glfwWindowShouldClose(m_Window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        
        processInput(m_Window);
        glfwPollEvents();
        
        int displayWidth, displayHeight;
        glfwGetFramebufferSize(m_Window, &displayWidth, &displayHeight);
        
        glClearColor(0.02f, 0.02f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        float aspectRatio = (float)displayWidth / (float)displayHeight;

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), 
                                                aspectRatio, 0.1f, 10000000.0f); // Increased far plane
        glm::mat4 view = camera.GetViewMatrix();

glDepthMask(GL_FALSE);
skySphere->position = camera.Position;
skySphere->renderSphere(view, projection, camera.Position);
glDepthMask(GL_TRUE);



        
        // Render all planets
        for (auto* planet : planets) {
            planet->renderSphere(view, projection, camera.Position);
        }
        
        // ImGui labels
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        // Draw labels for each planet
        for (auto* planet : planets) {
            glm::vec3 planetPos = planet->getPosition();
            float planetRadius = planet->getRadius();
            
            // Position label above the planet
            glm::vec3 labelPos = planetPos + glm::vec3(0.0f, planetRadius * 1.2f, 0.0f);
            
            ImVec2 screenPos = WorldToScreen(labelPos, view, projection, 
                                            displayWidth, displayHeight);
            
            // Calculate distance from camera to planet
            float distanceToPlanet = glm::length(camera.Position - planetPos);
            
            // Only draw if on screen
            if (screenPos.x >= 0 && screenPos.x <= displayWidth &&
                screenPos.y >= 0 && screenPos.y <= displayHeight) {
                
                ImGui::SetNextWindowPos(ImVec2(screenPos.x - 50, screenPos.y), ImGuiCond_Always);
                ImGui::SetNextWindowBgAlpha(0.7f);
                ImGui::Begin(planet->getName().c_str(), nullptr, 
                            ImGuiWindowFlags_NoTitleBar | 
                            ImGuiWindowFlags_NoResize |
                            ImGuiWindowFlags_AlwaysAutoResize |
                            ImGuiWindowFlags_NoMove |
                            ImGuiWindowFlags_NoSavedSettings);
                
                ImGui::Text("%s", planet->getName().c_str());
                ImGui::Text("Diameter: %.0f km", planet->getDiameterKM());
                ImGui::Text("Distance: %.0f pixels", distanceToPlanet);
                ImGui::End();
            }
        }
        
        // Planet Direction Indicators (compass on edges)
        ImGui::SetNextWindowPos(ImVec2(displayWidth - 310, 10), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.8f);
        ImGui::Begin("Planet Compass", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("Planet Directions:");
        ImGui::Separator();
        
        for (auto* planet : planets) {
            glm::vec3 planetPos = planet->getPosition();
            glm::vec3 directionToPlanet = glm::normalize(planetPos - camera.Position);
            glm::vec3 cameraForward = camera.Front;
            glm::vec3 cameraRight = camera.Right;
            glm::vec3 cameraUp = camera.Up;
            
            // Calculate angles
            float forwardDot = glm::dot(directionToPlanet, cameraForward);
            float rightDot = glm::dot(directionToPlanet, cameraRight);
            float upDot = glm::dot(directionToPlanet, cameraUp);
            
            float distanceToPlanet = glm::length(camera.Position - planetPos);
            
            // Direction indicators
            const char* forwardDir = (forwardDot > 0.3f) ? "AHEAD" : (forwardDot < -0.3f) ? "BEHIND" : "";
            const char* rightDir = (rightDot > 0.3f) ? "RIGHT" : (rightDot < -0.3f) ? "LEFT" : "";
            const char* upDir = (upDot > 0.3f) ? "UP" : (upDot < -0.3f) ? "DOWN" : "";
            
            ImGui::Text("%s: %.0f px", planet->getName().c_str(), distanceToPlanet);
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "%s %s %s", forwardDir, rightDir, upDir);
        }
        ImGui::End();
        
        // Info panel
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowBgAlpha(0.8f);
        ImGui::Begin("Info", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("Scale: 1 pixel = Moon diameter (3,475 km)");
        ImGui::Text("True-to-scale distances!");
        ImGui::Text("Camera: (%.1f, %.1f, %.1f)", camera.Position.x, camera.Position.y, camera.Position.z);
        ImGui::Text("Speed: %s (%.1f pixels/s)", useLightSpeed ? "LIGHT SPEED" : "Normal", cameraSpeedMultiplier);
        ImGui::Text("WASD: Move | Mouse: Look | Scroll: Zoom");
        ImGui::Text("C: Toggle Speed Mode");
        ImGui::End();
        
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        glfwSwapBuffers(m_Window);
    }
    
    for (auto* planet : planets) {
        delete planet;
    }
}

Application::~Application() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(m_Window);
    glfwTerminate();
}
