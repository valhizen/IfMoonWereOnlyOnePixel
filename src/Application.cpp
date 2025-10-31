#include "Application.hpp"
#include "Camera.hpp"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include "Planet.hpp"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdexcept>
#include <vector>
#include <string>
#include <algorithm>

Camera camera(glm::vec3(0.0f, 500.0f, -3000.0f));

float lastX = 800.0f / 2.0;
float lastY = 600.0 / 2.0;
bool firstMouse = true;

// Camera speed modes
const float MOON_DIAMETER_KM = 3474.8f;
float cameraSpeedMultiplier = 1.0f;
const float NORMAL_SPEED = 500.0f;
const float FAST_SPEED = 5000.0f;
const float LIGHT_SPEED = 299792.458f / MOON_DIAMETER_KM;
bool useLightSpeed = false;
bool useFastSpeed = false;

// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Planet search
bool showSearchDialog = false;
char searchBuffer[256] = "";
bool searchJustOpened = false;

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  glViewport(0, 0, width, height);
}

void processInput(GLFWwindow *window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);

  // Toggle search dialog with / key
  static bool slashKeyPressed = false;
  if (glfwGetKey(window, GLFW_KEY_SLASH) == GLFW_PRESS && !slashKeyPressed) {
    showSearchDialog = !showSearchDialog;
    if (showSearchDialog) {
      searchBuffer[0] = '\0';
      searchJustOpened = true;
    }
    slashKeyPressed = true;
  }
  if (glfwGetKey(window, GLFW_KEY_SLASH) == GLFW_RELEASE) {
    slashKeyPressed = false;
  }

  // Toggle light speed with C key
  static bool cKeyPressed = false;
  if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS && !cKeyPressed) {
    useLightSpeed = !useLightSpeed;
    if (useLightSpeed) {
      cameraSpeedMultiplier = LIGHT_SPEED;
    } else {
      cameraSpeedMultiplier = useFastSpeed ? FAST_SPEED : NORMAL_SPEED;
    }
    cKeyPressed = true;
  }
  if (glfwGetKey(window, GLFW_KEY_C) == GLFW_RELEASE) {
    cKeyPressed = false;
  }

  // Fast speed with Shift
  if (!useLightSpeed) {
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
      useFastSpeed = true;
      cameraSpeedMultiplier = FAST_SPEED;
    } else {
      useFastSpeed = false;
      cameraSpeedMultiplier = NORMAL_SPEED;
    }
  }

  camera.MovementSpeed = cameraSpeedMultiplier;

  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    camera.ProcessKeyboard(FORWARD, deltaTime);
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    camera.ProcessKeyboard(BACKWARD, deltaTime);
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    camera.ProcessKeyboard(LEFT, deltaTime);
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    camera.ProcessKeyboard(RIGHT, deltaTime);
}

void mouse_callback(GLFWwindow *window, double xposIn, double yposIn) {
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

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
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

  m_Window = glfwCreateWindow(width, height, title, nullptr, nullptr);
  if (m_Window == nullptr)
    throw std::runtime_error("Failed to create window");

  glfwMakeContextCurrent(m_Window);

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

  cameraSpeedMultiplier = NORMAL_SPEED;
  camera.MovementSpeed = NORMAL_SPEED;
}

ImVec2 WorldToScreen(const glm::vec3 &worldPos, const glm::mat4 &view,
                     const glm::mat4 &projection, int screenWidth,
                     int screenHeight) {
  glm::vec4 clipSpace = projection * view * glm::vec4(worldPos, 1.0f);

  if (clipSpace.w <= 0.0f) {
    return ImVec2(-1000, -1000);
  }

  glm::vec3 ndc = glm::vec3(clipSpace) / clipSpace.w;

  float screenX = (ndc.x + 1.0f) * 0.5f * screenWidth;
  float screenY = (1.0f - ndc.y) * 0.5f * screenHeight;

  return ImVec2(screenX, screenY);
}

void Application::Run() {
  const float MOON_DIAMETER_KM = 3474.8f;

  std::vector<Planet *> planets;

  // Sun at origin
  Planet* sun = new Planet(1392000.0f, "Sun", 0.0f, 
                           glm::vec3(1.0f, 0.9f, 0.2f),
                           "assets/sunmap.jpg");
  planets.push_back(sun);

  // Mercury orbiting Sun
  float mercuryDist = 57900000.0f / MOON_DIAMETER_KM;
  Planet* mercury = new Planet(4879.0f, "Mercury", 0.0f, 
                               glm::vec3(0.7f, 0.7f, 0.7f),
                               "assets/8k_mercury.jpg");
  mercury->setOrbitParent(sun, mercuryDist, 0.04f);
  planets.push_back(mercury);

  // Venus orbiting Sun
  float venusDist = 108200000.0f / MOON_DIAMETER_KM;
  Planet* venus = new Planet(12104.0f, "Venus", 0.0f, 
                            glm::vec3(0.9f, 0.7f, 0.5f),
                            "assets/8k_venus_surface.jpg");
  venus->setOrbitParent(sun, venusDist, 0.03f);
  planets.push_back(venus);

  // Earth orbiting Sun
  float earthDist = 149600000.0f / MOON_DIAMETER_KM;
  Planet* earth = new Planet(12742.0f, "Earth", 0.0f, 
                            glm::vec3(0.2f, 0.5f, 1.0f),
                            "assets/Earth-Color-Map-8k.png");
  earth->setOrbitParent(sun, earthDist, 0.02f);
  planets.push_back(earth);

  // // Moon orbiting Earth
  // float moonDistFromEarth = 384400.0f / MOON_DIAMETER_KM;
  // Planet* moon = new Planet(3474.8f, "Moon", 0.0f,
  //                          glm::vec3(0.6f, 0.6f, 0.6f),
  //                          nullptr);
  // moon->setOrbitParent(earth, moonDistFromEarth);
  // planets.push_back(moon);

  // Mars orbiting Sun
  float marsDist = 227900000.0f / MOON_DIAMETER_KM;
  Planet* mars = new Planet(6779.0f, "Mars", 0.0f, 
                           glm::vec3(0.8f, 0.3f, 0.2f),
                           "assets/8k_mars.jpg");
  mars->setOrbitParent(sun, marsDist, 0.015f);
  planets.push_back(mars);

  // Jupiter orbiting Sun
  float jupiterDist = 778500000.0f / MOON_DIAMETER_KM;
  Planet* jupiter = new Planet(139820.0f, "Jupiter", 0.0f,
                              glm::vec3(0.8f, 0.6f, 0.4f),
                              "assets/8k_jupiter.jpg");
  jupiter->setOrbitParent(sun, jupiterDist, 0.008f);
  planets.push_back(jupiter);

  // Saturn orbiting Sun
  float saturnDist = 1434000000.0f / MOON_DIAMETER_KM;
  Planet* saturn = new Planet(116460.0f, "Saturn", 0.0f, 
                             glm::vec3(0.9f, 0.8f, 0.6f),
                             "assets/8k_saturn.jpg");
  saturn->setOrbitParent(sun, saturnDist, 0.005f);
  planets.push_back(saturn);

  // Uranus orbiting Sun
  float uranusDist = 2871000000.0f / MOON_DIAMETER_KM;
  Planet* uranus = new Planet(50724.0f, "Uranus", 0.0f, 
                             glm::vec3(0.5f, 0.8f, 0.9f),
                             "assets/2k_uranus.jpg");
  uranus->setOrbitParent(sun, uranusDist, 0.003f);
  planets.push_back(uranus);

  // Neptune orbiting Sun
  float neptuneDist = 4495000000.0f / MOON_DIAMETER_KM;
  Planet* neptune = new Planet(49244.0f, "Neptune", 0.0f,
                              glm::vec3(0.2f, 0.3f, 0.8f),
                              "assets/2k_neptune.jpg");
  neptune->setOrbitParent(sun, neptuneDist, 0.002f);
  planets.push_back(neptune);

  // Sky sphere (no orbit)
  Planet *skySphere = new Planet(1e8f, "SkySphere", 0.0f, 
                                 glm::vec3(0.02f, 0.02f, 0.08f),
                                 nullptr);
  skySphere->inverted = true;
	float value;

  // Main game loop
  while (!glfwWindowShouldClose(m_Window)) {
    float currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    processInput(m_Window);
    glfwPollEvents();

    // UPDATE ALL PLANETS FIRST (before rendering)
    for (auto *planet : planets) {
      planet->update(deltaTime);
    }
    
    int displayWidth, displayHeight;
    glfwGetFramebufferSize(m_Window, &displayWidth, &displayHeight);

    glClearColor(0.02f, 0.02f, 0.05f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float aspectRatio = (float)displayWidth / (float)displayHeight;
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
                                            aspectRatio, 0.1f, 10000000.0f);
    glm::mat4 view = camera.GetViewMatrix();
// Before rendering the sky sphere
skySphere->setPosition(camera.Position);
skySphere->renderSphere(view, projection, camera.Position);


    // Render sky sphere
    glDepthMask(GL_FALSE);
    skySphere->renderSphere(view, projection, camera.Position);
    glDepthMask(GL_TRUE);

    // Render all planets
    for (auto *planet : planets) {
      planet->renderSphere(view, projection, camera.Position);
    }

    // ImGui
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Get Earth's current position for distance calculation
    glm::vec3 earthPos = earth->getPosition();
    float distToEarth = glm::length(camera.Position - earthPos);

    // Planet search dialog
    if (showSearchDialog) {
      ImGui::SetNextWindowPos(ImVec2(displayWidth / 2 - 200, 100), ImGuiCond_Always);
      ImGui::SetNextWindowSize(ImVec2(400, 0), ImGuiCond_Always);
      ImGui::SetNextWindowBgAlpha(0.95f);
      ImGui::Begin("Go to Planet", &showSearchDialog, 
                   ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
      
      if (searchJustOpened) {
        ImGui::SetKeyboardFocusHere();
        searchJustOpened = false;
      }
      
      ImGui::Text("Type planet name:");
      bool enterPressed = ImGui::InputText("##search", searchBuffer, 
                                           sizeof(searchBuffer), 
                                           ImGuiInputTextFlags_EnterReturnsTrue);
      
      std::string searchStr(searchBuffer);
      std::transform(searchStr.begin(), searchStr.end(), searchStr.begin(), ::tolower);
      
      ImGui::Separator();
      ImGui::Text("Available planets:");
      
      Planet* selectedPlanet = nullptr;
      
      for (auto *planet : planets) {
        std::string planetName = planet->getName();
        std::string planetNameLower = planetName;
        std::transform(planetNameLower.begin(), planetNameLower.end(), 
                      planetNameLower.begin(), ::tolower);
        
        if (searchStr.empty() || planetNameLower.find(searchStr) != std::string::npos) {
          if (ImGui::Button(planetName.c_str(), ImVec2(-1, 0))) {
            selectedPlanet = planet;
          }
        }
      }
      
      // Handle planet selection
      if (selectedPlanet || enterPressed) {
        if (!selectedPlanet && !searchStr.empty()) {
          // Find first matching planet on Enter
          for (auto *planet : planets) {
            std::string planetNameLower = planet->getName();
            std::transform(planetNameLower.begin(), planetNameLower.end(), 
                          planetNameLower.begin(), ::tolower);
            if (planetNameLower.find(searchStr) != std::string::npos) {
              selectedPlanet = planet;
              break;
            }
          }
        }
        
        if (selectedPlanet) {
          glm::vec3 targetPos = selectedPlanet->getPosition();
          float planetRadius = selectedPlanet->getRadius();
          
          // Position camera 500px in front of the planet
          float viewDistance = planetRadius + 500.0f;
          
          camera.Position = targetPos + glm::vec3(0, 0, -viewDistance);
          
          showSearchDialog = false;
          searchBuffer[0] = '\0';
        }
      }
      
      ImGui::End();
    }

    // Draw labels for visible planets
    for (auto *planet : planets) {
      glm::vec3 planetPos = planet->getPosition();
      float planetRadius = planet->getRadius();
      glm::vec3 labelPos = planetPos + glm::vec3(0.0f, planetRadius * 1.2f, 0.0f);

      ImVec2 screenPos = WorldToScreen(labelPos, view, projection, 
                                       displayWidth, displayHeight);

      float distanceToPlanet = glm::length(camera.Position - planetPos);

      if (screenPos.x >= 0 && screenPos.x <= displayWidth && 
          screenPos.y >= 0 && screenPos.y <= displayHeight) {

        ImGui::SetNextWindowPos(ImVec2(screenPos.x - 50, screenPos.y),
                                ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.7f);
        ImGui::Begin(planet->getName().c_str(), nullptr,
                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_AlwaysAutoResize |
                     ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoSavedSettings);

        ImGui::Text("%s", planet->getName().c_str());
        ImGui::Text("Diameter: %.0f km", planet->getDiameterKM());
        ImGui::Text("Distance: %.0f px", distanceToPlanet);
        ImGui::End();
      }
    }

    // Compass
    ImGui::SetNextWindowPos(ImVec2(displayWidth - 310, 10), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.8f);
    ImGui::Begin("Planet Compass", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("Planet Directions:");
    ImGui::Separator();

    for (auto *planet : planets) {
      if (planet->getName() == "SkySphere")
        continue;

      glm::vec3 planetPos = planet->getPosition();
      glm::vec3 directionToPlanet = glm::normalize(planetPos - camera.Position);

      float forwardDot = glm::dot(directionToPlanet, camera.Front);
      float rightDot = glm::dot(directionToPlanet, camera.Right);
      float upDot = glm::dot(directionToPlanet, camera.Up);

      float distanceToPlanet = glm::length(camera.Position - planetPos);

      const char *forwardDir = (forwardDot > 0.3f) ? "AHEAD"
                               : (forwardDot < -0.3f) ? "BEHIND" : "";
      const char *rightDir = (rightDot > 0.3f) ? "RIGHT"
                             : (rightDot < -0.3f) ? "LEFT" : "";
      const char *upDir = (upDot > 0.3f) ? "UP" : (upDot < -0.3f) ? "DOWN" : "";

      ImGui::Text("%s: %.0f px", planet->getName().c_str(), distanceToPlanet);
      ImGui::SameLine();
      ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "%s %s %s", 
                         forwardDir, rightDir, upDir);
    }
    ImGui::End();

    // Performance window
    ImGui::SetNextWindowPos(ImVec2(10, displayHeight - 120), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.8f);
    ImGui::Begin("Performance", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("FPS: %.1f", 1.0f / deltaTime);
    ImGui::Text("Frame Time: %.3f ms", deltaTime * 1000.0f);
    ImGui::End();

    // Info panel
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(0.8f);
    ImGui::Begin("Info", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("Scale: 1 pixel = Moon diameter (3,474.8 km)");
    ImGui::Text("TRUE astronomical distances with REAL textures!");
    ImGui::Separator();

    ImGui::Text("Camera: (%.0f, %.0f, %.0f)", camera.Position.x,
                camera.Position.y, camera.Position.z);

    if (useLightSpeed) {
      ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f),
                         "Speed: LIGHT SPEED (%.2f px/s)", LIGHT_SPEED);
    } else if (useFastSpeed) {
      ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f),
                         "Speed: FAST (%.0f px/s)", FAST_SPEED);
    } else {
      ImGui::Text("Speed: Normal (%.0f px/s)", NORMAL_SPEED);
    }

    ImGui::Separator();
    ImGui::Text("Real Distances from Sun:");
    ImGui::Text("  Mercury: 16,666 px");
    ImGui::Text("  Earth: 43,051 px");
    ImGui::Text("  Jupiter: 224,050 px");
    ImGui::Text("  Neptune: 1,293,488 px");

    ImGui::Separator();
    float distToSun = glm::length(camera.Position - sun->getPosition());
    ImGui::Text("Your distance from Sun: %.0f px", distToSun);

    ImGui::Separator();
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f),
                       "Distance to Earth: %.0f px", distToEarth);
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f),
                       "Travel time at light speed: %.1f seconds (%.1f min)",
                       distToEarth / LIGHT_SPEED,
                       (distToEarth / LIGHT_SPEED) / 60.0f);

    ImGui::Separator();
    ImGui::Text("Controls:");
    ImGui::Text("  W/A/S/D: Move");
    ImGui::Text("  Shift: Fast speed");
    ImGui::Text("  C: Light speed toggle");
    ImGui::Text("  /: Search planets");
    ImGui::Text("  Mouse: Look around");

    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(m_Window);
  }

  for (auto *planet : planets) {
    delete planet;
  }
  delete skySphere;
}

Application::~Application() {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  glfwDestroyWindow(m_Window);
  glfwTerminate();
}
