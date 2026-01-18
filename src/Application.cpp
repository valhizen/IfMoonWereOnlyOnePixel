#include <iostream>
#include "Application.hpp"
#include "Camera.hpp"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include "imgui_impl_opengl3.h"
#include "ImGuiStyle.hpp"
#include "Cockpit.hpp"
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
float baseZoom = 45.0f;
float warpZoomEffect = 0.0f; // 0.0 (normal) to 1.0 (warping)

// Camera speed modes
const float MOON_DIAMETER_KM = 3474.8f;
float cameraSpeedMultiplier = 1.0f;
const float NORMAL_SPEED = 500.0f;
const float FAST_SPEED = 5000.0f;
const float LIGHT_SPEED = 299792.458f / MOON_DIAMETER_KM;
bool useLightSpeed = false;
bool useFastSpeed = false;

// Mouse control
bool cursorEnabled = false;
bool showExitDialog = false;

// Camera tracking
Planet* trackedPlanet = nullptr;
bool cameraTrackingEnabled = false;
float cameraTrackDistance = 1000.0f;
float cameraOrbitAngle = 0.0f;
float cameraOrbitSpeed = 0.5f;
bool cameraOrbitMode = false;

// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Planet search
bool showSearchDialog = false;
char searchBuffer[256] = "";
bool searchJustOpened = false;

// Light speed travel animation
bool lightTravelActive = false;
Planet* lightTravelFrom = nullptr;
Planet* lightTravelTo = nullptr;
glm::vec3 lightTravelStartPos;
glm::vec3 lightTravelEndPos;
float lightTravelProgress = 0.0f;
float lightTravelElapsedTime = 0.0f;
float lightTravelTotalDistance = 0.0f;

// View State
bool showCockpit = true;
void ApplyCockpitTheme();
void ApplySimpleTheme();

// Audio System
#include "AudioSystem.hpp"
AudioSystem* audioSystem = nullptr;

// Video Player
#include "VideoPlayer.hpp"
VideoPlayer* videoPlayer = nullptr;
bool showLaunchSequence = true;
bool playingLaunch = false;
#include "AsteroidField.hpp"
AsteroidField* asteroidField = nullptr;

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  glViewport(0, 0, width, height);
}

void processInput(GLFWwindow *window) {
  // Escape key handling
  static bool escKeyPressed = false;
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS && !escKeyPressed) {
    if (showSearchDialog) {
      showSearchDialog = false; // Close search if open
    } else if (showExitDialog) {
        showExitDialog = false; // Close exit dialog if open
    } else {
      showExitDialog = true;    // Show exit dialog
    }
    escKeyPressed = true;
  }
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_RELEASE) {
    escKeyPressed = false;
  }

  // Toggle search dialog with / key
  static bool slashKeyPressed = false;
  if (glfwGetKey(window, GLFW_KEY_SLASH) == GLFW_PRESS && !slashKeyPressed) {
    if (!showSearchDialog) {
        // Only open if not already open
        showSearchDialog = true;
        searchBuffer[0] = '\0';
        searchJustOpened = true;
    }
    // If already open, do nothing here (allow typing / in the box)
    slashKeyPressed = true;
  }
  if (glfwGetKey(window, GLFW_KEY_SLASH) == GLFW_RELEASE) {
    slashKeyPressed = false;
  }

  // Toggle light speed with C key
  static bool cKeyPressed = false;
  if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS && !cKeyPressed) {
    if (!lightTravelActive) {
         useLightSpeed = !useLightSpeed;
         if (useLightSpeed) {
           cameraSpeedMultiplier = LIGHT_SPEED;
         } else {
           cameraSpeedMultiplier = useFastSpeed ? FAST_SPEED : NORMAL_SPEED;
         }
    }
    cKeyPressed = true;
  }
  if (glfwGetKey(window, GLFW_KEY_C) == GLFW_RELEASE) {
    cKeyPressed = false;
  }

  // Toggle Cockpit View with V key
  static bool vKeyPressed = false;
  if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS && !vKeyPressed) {
      showCockpit = !showCockpit;
      if (showCockpit) ApplyCockpitTheme();
      else ApplySimpleTheme();
      vKeyPressed = true;
  }
  if (glfwGetKey(window, GLFW_KEY_V) == GLFW_RELEASE) {
      vKeyPressed = false;
  }

  // Toggle camera tracking with T key
  static bool tKeyPressed = false;
  if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS && !tKeyPressed) {
    cameraTrackingEnabled = !cameraTrackingEnabled;
    if (!cameraTrackingEnabled) {
      trackedPlanet = nullptr;
    }
    tKeyPressed = true;
  }
  if (glfwGetKey(window, GLFW_KEY_T) == GLFW_RELEASE) {
    tKeyPressed = false;
  }
  
  // Toggle camera orbit mode with F key
  static bool fKeyPressed = false;
  if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS && !fKeyPressed) {
    cameraOrbitMode = !cameraOrbitMode;
    fKeyPressed = true;
  }
  if (glfwGetKey(window, GLFW_KEY_F) == GLFW_RELEASE) {
    fKeyPressed = false;
  }
  
  // Toggle fullscreen with F11 key
  static bool f11KeyPressed = false;
  if (glfwGetKey(window, GLFW_KEY_F11) == GLFW_PRESS && !f11KeyPressed) {
    static bool isFullscreen = false;
    static int windowedX = 100, windowedY = 100;
    static int windowedWidth = 1920, windowedHeight = 1080;
    
    isFullscreen = !isFullscreen;
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    
    if (isFullscreen) {
      // Save windowed position and size
      glfwGetWindowPos(window, &windowedX, &windowedY);
      glfwGetWindowSize(window, &windowedWidth, &windowedHeight);
      // Switch to fullscreen
      glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
    } else {
      // Restore windowed mode
      glfwSetWindowMonitor(window, nullptr, windowedX, windowedY, windowedWidth, windowedHeight, 0);
    }
    
    f11KeyPressed = true;
  }
  if (glfwGetKey(window, GLFW_KEY_F11) == GLFW_RELEASE) {
    f11KeyPressed = false;
  }

  // Toggle cursor mode with TAB key
  static bool tabKeyPressed = false;
  if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS && !tabKeyPressed) {
    cursorEnabled = !cursorEnabled;
    if (cursorEnabled) {
      glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    } else {
      glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
      firstMouse = true;  // Reset to avoid camera jump
    }
    tabKeyPressed = true;
  }
  if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_RELEASE) {
    tabKeyPressed = false;
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
  // Only process camera movement when cursor is disabled (flight mode)
  if (cursorEnabled) {
    return;  // Cursor is free for UI interaction
  }

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
  baseZoom -= (float)yoffset;
  if (baseZoom < 1.0f) baseZoom = 1.0f;
  if (baseZoom > 90.0f) baseZoom = 90.0f;
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

  // Start in FULLSCREEN mode
  GLFWmonitor* monitor = glfwGetPrimaryMonitor();
  const GLFWvidmode* mode = glfwGetVideoMode(monitor);
  m_Window = glfwCreateWindow(mode->width, mode->height, title, monitor, nullptr);
  if (m_Window == nullptr)
    throw std::runtime_error("Failed to create window");

  glfwMakeContextCurrent(m_Window);
  glfwSwapInterval(1);  // Enable vsync

  glfwSetCursorPosCallback(m_Window, mouse_callback);
  glfwSetScrollCallback(m_Window, scroll_callback);
  
  // Start in UI mode with cursor enabled (press TAB to toggle to flight mode)
  glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
  cursorEnabled = true;

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

  // Apply vintage squared theme from separate style file
  VintageStyle::ApplyVintageSquaredTheme();

  ImGui_ImplGlfw_InitForOpenGL(m_Window, true);
  ImGui_ImplOpenGL3_Init("#version 460");

  cameraSpeedMultiplier = NORMAL_SPEED;
  camera.MovementSpeed = NORMAL_SPEED;
  
  // Initialize Cockpit
  m_Cockpit = new Cockpit();
  m_Cockpit->init();
  
  // Initialize Audio
  audioSystem = new AudioSystem();
  if (audioSystem->init()) {
      audioSystem->loadSounds();
  }
  
  // Initialize Video
  videoPlayer = new VideoPlayer();
  if (videoPlayer->load("assets/launch_video.mpg")) {
      if (showLaunchSequence) {
          playingLaunch = true;
          // Defer audio play until loop starts or play here?
          // Play here is fine if context active
      }
  }

  // Sci-Fi Theme (Default)
  ApplyCockpitTheme();
}

void ApplyCockpitTheme() {
  ImGuiStyle& style = ImGui::GetStyle();
  style.Colors[ImGuiCol_WindowBg] = ImVec4(0.02f, 0.05f, 0.1f, 0.5f);
  style.Colors[ImGuiCol_Border] = ImVec4(0.0f, 0.8f, 1.0f, 0.6f);
  style.Colors[ImGuiCol_TitleBg] = ImVec4(0.02f, 0.05f, 0.1f, 0.9f);
  style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.0f, 0.4f, 0.6f, 0.8f);
  style.Colors[ImGuiCol_Text] = ImVec4(0.8f, 0.95f, 1.0f, 1.0f);
  style.Colors[ImGuiCol_Button] = ImVec4(0.0f, 0.4f, 0.6f, 0.4f);
  style.Colors[ImGuiCol_Header] = ImVec4(0.0f, 0.4f, 0.6f, 0.4f);
  style.WindowRounding = 2.0f;
  style.FrameRounding = 2.0f;
}

void ApplySimpleTheme() {
  VintageStyle::ApplyVintageSquaredTheme();
  // Ensure alpha is opaque enough for simple mode
  ImGui::GetStyle().Colors[ImGuiCol_WindowBg].w = 0.9f; 
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

  // =============================================================================
  // SUN - Center of solar system with emissive glow
  // =============================================================================
  Planet* sun = new Planet(1392000.0f, "Sun", 0.0f, 
                           glm::vec3(1.0f, 0.9f, 0.2f),
                           "assets/sunmap.jpg");
  sun->setPosition(glm::vec3(0.0f, 0.0f, 0.0f));
  
  Planet::VisualParams sunVisual;
  sunVisual.radiusScale = 1.0f;
  sunVisual.emissiveStrength = 1.0f;
  sunVisual.emissiveColor = glm::vec3(1.0f, 0.9f, 0.2f);
  sunVisual.showOrbitPath = false;
  sun->setVisualParams(sunVisual);
  
  Planet::RotationParams sunRot;
  sunRot.rotationPeriodHours = 25.0f * 24.0f;
  sunRot.axialTiltDeg = 7.25f;
  sunRot.initialRotationDeg = 0.0f;
  sun->setRotation(sunRot);
  
  planets.push_back(sun);

  // =============================================================================
  // PLANETS - All with realistic Keplerian orbits
  // =============================================================================
  
  // MERCURY
  Planet* mercury = new Planet(4879.0f, "Mercury", 0.0f, 
                               glm::vec3(0.7f, 0.7f, 0.7f),
                               "assets/8k_mercury.jpg");
  mercury->setOrbit(sun, PlanetPresets::getMercuryOrbit());
  mercury->setRotation(PlanetPresets::getMercuryRotation());
  Planet::VisualParams mercuryVisual;
  mercuryVisual.radiusScale = 8.0f;
  mercuryVisual.showOrbitPath = true;
  mercuryVisual.orbitPathColor = glm::vec3(0.7f, 0.7f, 0.7f);
  mercuryVisual.orbitSegments = 256;
  mercury->setVisualParams(mercuryVisual);
  planets.push_back(mercury);

  // VENUS
  Planet* venus = new Planet(12104.0f, "Venus", 0.0f, 
                            glm::vec3(0.9f, 0.7f, 0.5f),
                            "assets/8k_venus_surface.jpg");
  venus->setOrbit(sun, PlanetPresets::getVenusOrbit());
  venus->setRotation(PlanetPresets::getVenusRotation());
  Planet::VisualParams venusVisual;
  venusVisual.radiusScale = 8.0f;
  venusVisual.showOrbitPath = true;
  venusVisual.orbitPathColor = glm::vec3(0.9f, 0.7f, 0.5f);
  venusVisual.orbitSegments = 256;
  venus->setVisualParams(venusVisual);
  planets.push_back(venus);

  // EARTH
  Planet* earth = new Planet(12742.0f, "Earth", 0.0f, 
                            glm::vec3(0.2f, 0.5f, 1.0f),
                            "assets/Earth-Color-Map-8k.png");
  earth->setOrbit(sun, PlanetPresets::getEarthOrbit());
  earth->setRotation(PlanetPresets::getEarthRotation());
  Planet::VisualParams earthVisual;
  earthVisual.radiusScale = 8.0f;
  earthVisual.showOrbitPath = true;
  earthVisual.orbitPathColor = glm::vec3(0.2f, 0.5f, 1.0f);
  earthVisual.orbitSegments = 360;
  earth->setVisualParams(earthVisual);
  planets.push_back(earth);

  // MARS
  Planet* mars = new Planet(6779.0f, "Mars", 0.0f, 
                           glm::vec3(0.8f, 0.3f, 0.2f),
                           "assets/8k_mars.jpg");
  mars->setOrbit(sun, PlanetPresets::getMarsOrbit());
  mars->setRotation(PlanetPresets::getMarsRotation());
  Planet::VisualParams marsVisual;
  marsVisual.radiusScale = 8.0f;
  marsVisual.showOrbitPath = true;
  marsVisual.orbitPathColor = glm::vec3(0.8f, 0.3f, 0.2f);
  marsVisual.orbitSegments = 256;
  mars->setVisualParams(marsVisual);
  planets.push_back(mars);

  // JUPITER
  Planet* jupiter = new Planet(139820.0f, "Jupiter", 0.0f,
                              glm::vec3(0.8f, 0.6f, 0.4f),
                              "assets/8k_jupiter.jpg");
  jupiter->setOrbit(sun, PlanetPresets::getJupiterOrbit());
  jupiter->setRotation(PlanetPresets::getJupiterRotation());
  Planet::VisualParams jupiterVisual;
  jupiterVisual.radiusScale = 3.0f;
  jupiterVisual.showOrbitPath = true;
  jupiterVisual.orbitPathColor = glm::vec3(0.8f, 0.6f, 0.4f);
  jupiterVisual.orbitSegments = 360;
  jupiter->setVisualParams(jupiterVisual);
  planets.push_back(jupiter);

  // SATURN
  Planet* saturn = new Planet(116460.0f, "Saturn", 0.0f, 
                             glm::vec3(0.9f, 0.8f, 0.6f),
                             "assets/8k_saturn.jpg");
  saturn->setOrbit(sun, PlanetPresets::getSaturnOrbit());
  saturn->setRotation(PlanetPresets::getSaturnRotation());
  Planet::VisualParams saturnVisual;
  saturnVisual.radiusScale = 3.0f;
  saturnVisual.showOrbitPath = true;
  saturnVisual.orbitPathColor = glm::vec3(0.9f, 0.8f, 0.6f);
  saturnVisual.orbitSegments = 360;
  saturn->setVisualParams(saturnVisual);
  planets.push_back(saturn);

  // URANUS
  Planet* uranus = new Planet(50724.0f, "Uranus", 0.0f, 
                             glm::vec3(0.5f, 0.8f, 0.9f),
                             "assets/2k_uranus.jpg");
  uranus->setOrbit(sun, PlanetPresets::getUranusOrbit());
  uranus->setRotation(PlanetPresets::getUranusRotation());
  Planet::VisualParams uranusVisual;
  uranusVisual.radiusScale = 4.0f;
  uranusVisual.showOrbitPath = true;
  uranusVisual.orbitPathColor = glm::vec3(0.5f, 0.8f, 0.9f);
  uranusVisual.orbitSegments = 360;
  uranus->setVisualParams(uranusVisual);
  planets.push_back(uranus);

  // NEPTUNE
  Planet* neptune = new Planet(49244.0f, "Neptune", 0.0f,
                              glm::vec3(0.2f, 0.3f, 0.8f),
                              "assets/2k_neptune.jpg");
  neptune->setOrbit(sun, PlanetPresets::getNeptuneOrbit());
  neptune->setRotation(PlanetPresets::getNeptuneRotation());
  Planet::VisualParams neptuneVisual;
  neptuneVisual.radiusScale = 4.0f;
  neptuneVisual.showOrbitPath = true;
  neptuneVisual.orbitPathColor = glm::vec3(0.2f, 0.3f, 0.8f);
  neptuneVisual.orbitSegments = 360;
  neptune->setVisualParams(neptuneVisual);
  planets.push_back(neptune);

  // =============================================================================
  // TIME CONTROL & SIMULATION SETTINGS
  // =============================================================================
  float globalTimeScale = 10000.0f;  // Default: 10,000x real time
  bool showTimeControl = true;
  bool pauseSimulation = false;
  
  Planet::SimulationParams simSettings;
  simSettings.timeScale = globalTimeScale;
  simSettings.usePhysicalOrbits = true;
  simSettings.pauseOrbit = false;
  simSettings.pauseRotation = false;
  
  // Apply simulation settings to all planets
  for (auto* planet : planets) {
      planet->setSimulationParams(simSettings);
  }

  // =============================================================================
  // SKY SPHERE (background starfield)
  // =============================================================================
  Planet *skySphere = new Planet(1e8f, "SkySphere", 0.0f, 
                                 glm::vec3(0.02f, 0.02f, 0.08f),
                                nullptr );
  skySphere->inverted = true;

  // UI state
  bool showPlanetLabels = true;
  bool showCompass = false;  // Disabled by default to prevent overlap
  bool showPerformance = false;  // FPS shown in HUD
  bool showInfo = true;

  // Main game loop
  // Start at Earth position
  if (earth) {
     float earthRad = earth->getRadius();
     camera.Position = earth->getPosition() + glm::vec3(0, 0, earthRad * 3.0f); // offset
  }

  if (playingLaunch && audioSystem) {
      std::cout << "Application: Starting Launch Audio." << std::endl;
      audioSystem->playLaunch();
  }
  
  // Reset timer to avoid huge delta from loading time
  lastFrame = glfwGetTime();

  // Initialize Asteroid Field (between Mars and Jupiter: 2.1 to 3.3 AU)
  // 1 AU ≈ 43053 pixels in this simulation
  asteroidField = new AsteroidField(1000, 2.1f * 43053.0f, 3.3f * 43053.0f);

  while (!glfwWindowShouldClose(m_Window)) {
    // Audio Update
    if (audioSystem) {
        bool isMoving = (glfwGetKey(m_Window, GLFW_KEY_W) == GLFW_PRESS ||
                         glfwGetKey(m_Window, GLFW_KEY_S) == GLFW_PRESS ||
                         glfwGetKey(m_Window, GLFW_KEY_A) == GLFW_PRESS ||
                         glfwGetKey(m_Window, GLFW_KEY_D) == GLFW_PRESS);
        if (lightTravelActive) isMoving = true;
        
        audioSystem->setEngineActive(isMoving);
        
        // Mute Toggle (M)
        static bool mKeyPressed = false;
        if (glfwGetKey(m_Window, GLFW_KEY_M) == GLFW_PRESS && !mKeyPressed) {
            audioSystem->toggleMute();
            mKeyPressed = true;
        }
        if (glfwGetKey(m_Window, GLFW_KEY_M) == GLFW_RELEASE) mKeyPressed = false;
    }
    float currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    processInput(m_Window);
    glfwPollEvents();
    
    // Warp Effect logic - trigger during autopilot OR manual light speed flight
    bool isWarping = lightTravelActive || (useLightSpeed && (glfwGetKey(m_Window, GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(m_Window, GLFW_KEY_S) == GLFW_PRESS));
    
    if (isWarping) warpZoomEffect = glm::mix(warpZoomEffect, 1.0f, 2.0f * deltaTime);
    else          warpZoomEffect = glm::mix(warpZoomEffect, 0.0f, 4.0f * deltaTime);
    
    camera.Zoom = baseZoom + (warpZoomEffect * 25.0f);
    if (camera.Zoom > 115.0f) camera.Zoom = 115.0f; // Safety cap

    // UPDATE ALL PLANETS FIRST (before rendering)
    for (auto *planet : planets) {
      planet->update(deltaTime);
    }
    if (asteroidField) asteroidField->update(deltaTime, simSettings.timeScale);
    
    // Camera tracking logic
    if (cameraTrackingEnabled && trackedPlanet != nullptr) {
        glm::vec3 planetPos = trackedPlanet->getPosition();
        float planetRadius = trackedPlanet->getRadius();
        
        if (cameraOrbitMode) {
            // Orbital camera mode - rotate around planet
            cameraOrbitAngle += cameraOrbitSpeed * deltaTime;
            if (cameraOrbitAngle > 360.0f) cameraOrbitAngle -= 360.0f;
            
            float angleRad = glm::radians(cameraOrbitAngle);
            glm::vec3 orbitOffset = glm::vec3(
                cos(angleRad) * cameraTrackDistance,
                cameraTrackDistance * 0.3f,  // Slight elevation
                sin(angleRad) * cameraTrackDistance
            );
            
            camera.Position = planetPos + orbitOffset;
            
            // Look at planet
            glm::vec3 direction = glm::normalize(planetPos - camera.Position);
            camera.Front = direction;
            
            // Recalculate Right and Up vectors
            camera.Right = glm::normalize(glm::cross(camera.Front, glm::vec3(0.0f, 1.0f, 0.0f)));
            camera.Up = glm::normalize(glm::cross(camera.Right, camera.Front));
        } else {
            // Follow mode - maintain relative position
            glm::vec3 targetPos = planetPos - camera.Front * cameraTrackDistance;
            
            // Smooth camera movement
            float smoothFactor = 5.0f * deltaTime;
            camera.Position = glm::mix(camera.Position, targetPos, smoothFactor);
            
            // Look at planet
            glm::vec3 direction = glm::normalize(planetPos - camera.Position);
            camera.Front = direction;
            
            // Recalculate Right and Up vectors
            camera.Right = glm::normalize(glm::cross(camera.Front, glm::vec3(0.0f, 1.0f, 0.0f)));
            camera.Up = glm::normalize(glm::cross(camera.Right, camera.Front));
        }
    }
    
    // Light speed travel animation
    if (lightTravelActive && lightTravelFrom && lightTravelTo) {
        // Target current position (planets move!)
        glm::vec3 currentTargetPos = lightTravelTo->getPosition();
        lightTravelEndPos = currentTargetPos; // Update for HUD accuracy
        float targetRadius = lightTravelTo->getRadius();
        
        // Direction to target center
        glm::vec3 toTarget = currentTargetPos - camera.Position;
        float distToTarget = glm::length(toTarget);
        glm::vec3 direction = glm::normalize(toTarget);
        
        // Stop at a comfortable viewing distance (e.g., 2x radius + minimal buffer)
        // We want to stop slightly before the center to avoid clipping/crashing into it
        float stopDistance = targetRadius * 2.0f;
        if (stopDistance < 500.0f) stopDistance = 500.0f; // Minimum distance
        
        // Move camera
        float moveStep = LIGHT_SPEED * deltaTime;
        
        lightTravelElapsedTime += deltaTime;
        
        // Check if we arrived or overshot
        if (distToTarget - moveStep <= stopDistance) {
            // Travel complete
            lightTravelProgress = 1.0f;
            lightTravelActive = false;
            
            // Snap to final position
            camera.Position = currentTargetPos - (direction * stopDistance);
            
            // Look at the destination planet
            camera.Front = direction; // Look forward (which is towards planet)
            camera.Right = glm::normalize(glm::cross(camera.Front, glm::vec3(0.0f, 1.0f, 0.0f)));
            camera.Up = glm::normalize(glm::cross(camera.Right, camera.Front));
            
            // Enable tracking on arrival for smooth transition
            cameraTrackingEnabled = true;
            trackedPlanet = lightTravelTo;
            cameraTrackDistance = stopDistance;
            cameraOrbitMode = false;
            
        } else {
            // Move towards planet
            camera.Position += direction * moveStep;
            
            // Look in travel direction
            camera.Front = direction;
            camera.Right = glm::normalize(glm::cross(camera.Front, glm::vec3(0.0f, 1.0f, 0.0f)));
            camera.Up = glm::normalize(glm::cross(camera.Right, camera.Front));
            
            // Update progress for HUD
            // We use the initial total distance to estimate percentage
            float distTraveled = lightTravelTotalDistance - distToTarget; // Approx
            lightTravelProgress = distTraveled / lightTravelTotalDistance;
            if (lightTravelProgress < 0.0f) lightTravelProgress = 0.0f;
            if (lightTravelProgress > 1.0f) lightTravelProgress = 1.0f;
        }
        
        // Disable normal camera controls during travel
        // (Ensure these stay disabled even if user tries to override during travel)
        cameraTrackingEnabled = false;
        trackedPlanet = nullptr;
    }
    
    int displayWidth, displayHeight;
    glfwGetFramebufferSize(m_Window, &displayWidth, &displayHeight);

    glClearColor(0.02f, 0.02f, 0.05f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float aspectRatio = (float)displayWidth / (float)displayHeight;
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
                                            aspectRatio, 0.1f, 10000000.0f);
    glm::mat4 view = camera.GetViewMatrix();

    // Render sky sphere
    skySphere->setPosition(camera.Position);
    glDepthMask(GL_FALSE);
    skySphere->renderSphere(view, projection, camera.Position);
    glDepthMask(GL_TRUE);

    // Render orbit paths
    for (auto *planet : planets) {
        planet->renderOrbitPath(view, projection);
    }

    // Render all planets
    for (auto *planet : planets) {
      planet->renderSphere(view, projection, camera.Position);
    }
    if (asteroidField) asteroidField->render(view, projection, camera.Position);

    // ImGui
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Get Earth's current position for distance calculation
    glm::vec3 earthPos = earth->getPosition();
    float distToEarth = glm::length(camera.Position - earthPos);

    // =============================================================================
    // ADVANCED UI PANELS
    // =============================================================================
    
    // Only show panels if NOT playing video
    if (!playingLaunch) {
    
    // TIME CONTROL PANEL
    if (showTimeControl) {
        ImGuiWindowFlags flags = showCockpit 
            ? (ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove) 
            : ImGuiWindowFlags_AlwaysAutoResize;
            
        if (showCockpit) {
            ImGui::SetNextWindowPos(ImVec2(displayWidth - 320, displayHeight - 200), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(300, 180), ImGuiCond_Always);
        } else {
            ImGui::SetNextWindowPos(ImVec2(displayWidth - 320, 100), ImGuiCond_FirstUseEver);
        }
        ImGui::Begin("Time Control", &showTimeControl, flags);
        
        ImGui::TextColored(ImVec4(0.8f, 0.9f, 1.0f, 1.0f), "Simulation Speed");
        ImGui::Separator();
        ImGui::Spacing();
        
        // Quick preset buttons
        if (ImGui::Button("Real Time", ImVec2(95, 0))) globalTimeScale = 1.0f;
        ImGui::SameLine();
        if (ImGui::Button("1 Day/s", ImVec2(95, 0))) globalTimeScale = 86400.0f;
        ImGui::SameLine();
        if (ImGui::Button("1 Week/s", ImVec2(95, 0))) globalTimeScale = 604800.0f;
        
        if (ImGui::Button("10k x", ImVec2(95, 0))) globalTimeScale = 10000.0f;
        ImGui::SameLine();
        if (ImGui::Button("100k x", ImVec2(95, 0))) globalTimeScale = 100000.0f;
        ImGui::SameLine();
        if (ImGui::Button("1M x", ImVec2(95, 0))) globalTimeScale = 1000000.0f;
        
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.7f, 0.8f, 0.9f, 1.0f), "Custom Speed:");
        ImGui::SliderFloat("##TimeScale", &globalTimeScale, 1.0f, 100000000.0f, 
                          "%.0f x", ImGuiSliderFlags_Logarithmic);
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // Pause controls
        if (ImGui::Checkbox("Pause Simulation", &pauseSimulation)) {
            simSettings.pauseOrbit = pauseSimulation;
            simSettings.pauseRotation = pauseSimulation;
        }
        ImGui::SameLine();
        ImGui::Checkbox("Physical Orbits", &simSettings.usePhysicalOrbits);
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // Current status
        ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Current: %.0f x real time", globalTimeScale);
        float earthYearSeconds = 365.256f * 86400.0f / globalTimeScale;
        ImGui::Text("Earth's year: %.1f seconds", earthYearSeconds);
        
        // Apply time scale
        simSettings.timeScale = globalTimeScale;
        for (auto* planet : planets) {
            planet->setSimulationParams(simSettings);
        }
        
        ImGui::End();
    }
    
    // CAMERA & PLANET NAVIGATOR PANEL
    {
        ImGuiWindowFlags flags = showCockpit 
            ? (ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove) 
            : ImGuiWindowFlags_AlwaysAutoResize;

        if (showCockpit) {
            ImGui::SetNextWindowPos(ImVec2(displayWidth/2 - 200, displayHeight - 220), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(400, 200), ImGuiCond_Always);
        } else {
             ImGui::SetNextWindowPos(ImVec2(10, 50), ImGuiCond_FirstUseEver);
        }
        ImGui::Begin("Navigation & Camera", nullptr, flags);
        
        // CAMERA CONTROLS SECTION
        ImGui::TextColored(ImVec4(0.8f, 0.9f, 1.0f, 1.0f), "Camera Controls");
        ImGui::Separator();
        ImGui::Spacing();
        
        // Speed display
        ImGui::Text("Movement Speed:");
        if (useLightSpeed) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "LIGHT SPEED");
        } else if (useFastSpeed) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "FAST");
        } else {
            ImGui::SameLine();
            ImGui::Text("Normal");
        }
        
        ImGui::Spacing();
        
        // Camera tracking
        ImGui::TextColored(ImVec4(0.7f, 0.8f, 0.9f, 1.0f), "Tracking:");
        if (ImGui::Checkbox("Enable Tracking (T)", &cameraTrackingEnabled)) {
            if (!cameraTrackingEnabled) {
                trackedPlanet = nullptr;
            }
        }
        
        if (cameraTrackingEnabled) {
            ImGui::Checkbox("Orbit Mode (F)", &cameraOrbitMode);
            ImGui::SliderFloat("Distance", &cameraTrackDistance, 100.0f, 50000.0f, "%.0f px");
            if (cameraOrbitMode) {
                ImGui::SliderFloat("Orbit Speed", &cameraOrbitSpeed, 0.1f, 5.0f, "%.1f");
            }
            
            if (trackedPlanet) {
                ImGui::Text("Tracking: %s", trackedPlanet->getName().c_str());
            }
        }
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // PLANET NAVIGATOR SECTION
        ImGui::TextColored(ImVec4(0.8f, 0.9f, 1.0f, 1.0f), "Planet Navigator");
        ImGui::Separator();
        ImGui::Spacing();
        
        static Planet* selectedPlanet = earth;
        
        // Planet selection buttons
        for (auto* planet : planets) {
            if (planet->getName() == "SkySphere") continue;
            
            bool isSelected = (selectedPlanet == planet);
            bool isTracked = (trackedPlanet == planet);
            
            // Highlight tracked/selected planets
            if (isTracked) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.3f, 0.5f, 1.0f));
            } else if (isSelected) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.4f, 0.6f, 1.0f));
            }
            
            std::string buttonLabel = planet->getName();
            if (isTracked) buttonLabel += " [TRACKED]";
            
            if (ImGui::Button(buttonLabel.c_str(), ImVec2(160, 0))) {
                selectedPlanet = planet;
            }
            
            if (isTracked || isSelected) {
                ImGui::PopStyleColor();
            }
            
            // Show distance on same line
            ImGui::SameLine();
            float dist = glm::length(camera.Position - planet->getPosition());
            ImGui::TextDisabled("%.0f px", dist);
        }
        
        // Selected planet info
        if (selectedPlanet) {
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            
            ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.3f, 1.0f), "%s", selectedPlanet->getName().c_str());
            ImGui::Text("Diameter: %.0f km", selectedPlanet->getDiameterKM());
            ImGui::Text("Orbital Period: %.1f days", selectedPlanet->getOrbitalPeriodDays());
            
            float distToPlanet = glm::length(camera.Position - selectedPlanet->getPosition());
            ImGui::Text("Distance: %.0f px", distToPlanet);
            
            float lightTravelTime = distToPlanet / LIGHT_SPEED;
            if (lightTravelTime < 60.0f) {
                ImGui::TextDisabled("Light travel: %.1f sec", lightTravelTime);
            } else {
                ImGui::TextDisabled("Light travel: %.1f min", lightTravelTime / 60.0f);
            }
            
            ImGui::Spacing();
            
            // Action buttons
            if (ImGui::Button("Go to Planet", ImVec2(160, 30))) {
                glm::vec3 targetPos = selectedPlanet->getPosition();
                float planetRadius = selectedPlanet->getRadius();
                cameraTrackDistance = planetRadius * 15.0f;
                if (cameraTrackDistance < 500.0f) cameraTrackDistance = 500.0f;
                camera.Position = targetPos + glm::vec3(0, 0, -cameraTrackDistance);
            }
            
            ImGui::SameLine();
            
            if (cameraTrackingEnabled && trackedPlanet == selectedPlanet) {
                if (ImGui::Button("Untrack", ImVec2(160, 30))) {
                    trackedPlanet = nullptr;
                    cameraTrackingEnabled = false;
                }
            } else {
                if (ImGui::Button("Track This Planet", ImVec2(160, 30))) {
                    trackedPlanet = selectedPlanet;
                    cameraTrackingEnabled = true;
                    float planetRadius = selectedPlanet->getRadius();
                    cameraTrackDistance = planetRadius * 15.0f;
                    if (cameraTrackDistance < 500.0f) cameraTrackDistance = 500.0f;
                }
            }
        }
        
        ImGui::End();
    }
    
    // EXIT CONFIRMATION DIALOG (Modal Popup)
    if (showExitDialog) {
        ImGui::OpenPopup("Exit Confirmation");
        ImVec2 center = ImVec2(displayWidth * 0.5f, displayHeight * 0.5f);
        ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        
        if (ImGui::BeginPopupModal("Exit Confirmation", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Spacing();
            
            // Valhizen branding header
            ImGui::TextColored(ImVec4(0.6f, 0.8f, 0.5f, 1.0f), "If the Moon Were Only One Pixel");
            ImGui::TextColored(ImVec4(0.5f, 0.6f, 0.5f, 0.8f), "by valhizen");
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Leave the solar system?");
            ImGui::Spacing();
            ImGui::TextWrapped("Your journey through space will end.");
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            
            float buttonWidth = 140.0f;
            float totalWidth = buttonWidth * 2 + 10.0f;
            float windowWidth = ImGui::GetWindowContentRegionMax().x;
            ImGui::SetCursorPosX((windowWidth - totalWidth) * 0.5f);
            
            if (ImGui::Button("Exit", ImVec2(buttonWidth, 30))) {
                glfwSetWindowShouldClose(m_Window, true);
            }
            
            ImGui::SameLine();
            
            if (ImGui::Button("Continue Exploring", ImVec2(buttonWidth, 30))) {
                showExitDialog = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SetItemDefaultFocus();
            
            ImGui::Spacing();
            ImGui::EndPopup();
        }
    }
    
    // HUD STATUS BAR - Top of screen
    {
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(displayWidth, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 6));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 4));
        ImGui::SetNextWindowBgAlpha(0.85f);
        ImGui::Begin("##HUD", nullptr, 
                    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
                    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                    ImGuiWindowFlags_NoSavedSettings);
        
        // Left side - System title
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "SOLAR SYSTEM EXPLORER");
        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();
        ImGui::Text("1px = 3,474.8 km");
        
        // Center-right - Speed and tracking
        ImGui::SameLine(displayWidth - 650);
        
        // Speed indicator
        if (useLightSpeed) {
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "LIGHT SPEED");
        } else if (useFastSpeed) {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "FAST MODE");
        } else {
            ImGui::TextColored(ImVec4(0.5f, 0.8f, 0.5f, 1.0f), "Normal Speed");
        }
        
        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();
        
        // Tracking status
        if (cameraTrackingEnabled && trackedPlanet) {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.8f, 1.0f), "Tracking: %s", 
                             trackedPlanet->getName().c_str());
            if (cameraOrbitMode) {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.3f, 1.0f), "(Orbit)");
            }
        } else {
            ImGui::TextDisabled("Free Camera");
        }
        
        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();
        
        // Cursor state
        if (cursorEnabled) {
            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "UI Mode");
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), "Flight Mode");
        }
        
        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();
        ImGui::Text("FPS: %.0f", 1.0f / deltaTime);
        
        ImGui::End();
        ImGui::PopStyleVar(2);
    }
    
    
    // LIGHT TRAVEL HUD
    if (lightTravelActive) {
        ImGui::SetNextWindowPos(ImVec2(displayWidth / 2 - 200, 80));
        ImGui::SetNextWindowSize(ImVec2(400, 0));
        ImGui::SetNextWindowBgAlpha(0.6f);
        
        ImGui::Begin("##TravelHUD", nullptr, 
                    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
                    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoInputs);
        
        ImGui::SetWindowFontScale(1.2f);
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "TRAVELING AT LIGHT SPEED");
        ImGui::SetWindowFontScale(1.0f);
        
        ImGui::Separator();
        
        ImGui::Text("%s -> %s", lightTravelFrom->getName().c_str(), lightTravelTo->getName().c_str());
        
        // Time display
        int minutes = (int)lightTravelElapsedTime / 60;
        float seconds = fmod(lightTravelElapsedTime, 60.0f);
        ImGui::Text("Time Elapsed: %02d:%05.2f", minutes, seconds);
        
        // Distance remaining
        float distRemaining = glm::length(lightTravelEndPos - camera.Position);
        float distKm = distRemaining * MOON_DIAMETER_KM; // Convert pixels to km
        
        if (distKm > 1000000.0f)
            ImGui::Text("Distance Remaining: %.1f million km", distKm / 1000000.0f);
        else
            ImGui::Text("Distance Remaining: %.0f km", distKm);
            
        // Progress bar
        ImGui::ProgressBar(lightTravelProgress, ImVec2(-1, 0), "");
        
        ImGui::Spacing();
        ImGui::TextDisabled("Press 'C' to stop");
        
        if (glfwGetKey(m_Window, GLFW_KEY_C) == GLFW_PRESS) {
            lightTravelActive = false;
        }
        
        ImGui::End();
    }
    
    // HELP & CONTROLS PANEL
    if (showInfo) {
        ImGuiWindowFlags flags = showCockpit 
            ? (ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove) 
            : ImGuiWindowFlags_AlwaysAutoResize;

        if (showCockpit) {
            ImGui::SetNextWindowPos(ImVec2(20, displayHeight - 350), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(300, 330), ImGuiCond_Always);
            ImGui::SetNextWindowBgAlpha(0.0f); 
        } else {
            ImGui::SetNextWindowPos(ImVec2(10, displayHeight - 10), ImGuiCond_FirstUseEver, ImVec2(0,1));
            ImGui::SetNextWindowBgAlpha(0.9f);
        }
        ImGui::Begin("Controls & Help", &showInfo, flags);
        
        if (ImGui::CollapsingHeader("Keyboard Controls", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "Mode Toggle:");
            ImGui::BulletText("TAB - Toggle UI/Flight mode");
            ImGui::TextDisabled("  (UI mode: free cursor, Flight mode: locked cursor)");
            
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Movement (Flight Mode):");
            ImGui::BulletText("W/A/S/D - Move camera");
            ImGui::BulletText("Mouse - Look around");
            ImGui::BulletText("Scroll - Zoom FOV");
            
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.5f, 1.0f), "Speed:");
            ImGui::BulletText("Shift - Fast mode (hold)");
            ImGui::BulletText("C - Light speed toggle");
            
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.8f, 1.0f), "Camera:");
            ImGui::BulletText("T - Toggle tracking");
            ImGui::BulletText("F - Toggle orbit mode");
            
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.8f, 1.0f, 0.7f, 1.0f), "UI:");
            ImGui::BulletText("/ - Quick search");
            ImGui::BulletText("ESC - Exit dialog");
            
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "Audio:");
            ImGui::BulletText("M - Toggle Mute");
            
            ImGui::Spacing();
            ImGui::Checkbox("Play Launch Sequence", &showLaunchSequence);
        }
        
        if (ImGui::CollapsingHeader("Status")) {
            ImGui::Spacing();
            ImGui::Text("Camera: (%.0f, %.0f, %.0f)", 
                       camera.Position.x, camera.Position.y, camera.Position.z);
            
            float distToSun = glm::length(camera.Position - sun->getPosition());
            ImGui::Text("Sun distance: %.0f px", distToSun);
            ImGui::Text("Earth distance: %.0f px", distToEarth);
            
            float lightTime = distToEarth / LIGHT_SPEED;
            if (lightTime < 60.0f) {
                ImGui::TextDisabled("Light: %.1f sec", lightTime);
            } else {
                ImGui::TextDisabled("Light: %.1f min", lightTime / 60.0f);
            }
        }
        
        if (ImGui::CollapsingHeader("Settings")) {
            ImGui::Spacing();
            ImGui::Checkbox("Planet Labels", &showPlanetLabels);
            ImGui::Checkbox("Compass", &showCompass);
            ImGui::Checkbox("Performance", &showPerformance);
        }
        
        // Valhizen branding footer
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.5f, 0.6f, 0.5f, 0.7f), "Created by valhizen");
        
        ImGui::End();
    }

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
      
      // Check for /From/To travel syntax (e.g., "/Sun/Earth")
      std::string fromPlanetName, toPlanetName;
      bool isTravelCommand = false;
      
      if (searchStr.length() > 2 && searchStr.find('/') != std::string::npos) {
          // Check if it starts with / or just contains it (allow Sun/Earth or /Sun/Earth)
          size_t firstSlash = searchStr.find('/');
          size_t secondSlash = searchStr.find('/', firstSlash + 1);
          
          if (secondSlash != std::string::npos) {
             // Case: /Sun/Earth
             fromPlanetName = searchStr.substr(firstSlash + 1, secondSlash - (firstSlash + 1));
             toPlanetName = searchStr.substr(secondSlash + 1);
             isTravelCommand = true;
          } else if (firstSlash > 0) {
             // Case: Sun/Earth
             fromPlanetName = searchStr.substr(0, firstSlash);
             toPlanetName = searchStr.substr(firstSlash + 1);
             isTravelCommand = true;
          }
      }

      // Handle planet selection
      if (selectedPlanet || enterPressed) {
        if (isTravelCommand && enterPressed) {
            // Initiate light speed travel
            Planet* fromP = nullptr;
            Planet* toP = nullptr;
            
            // Find planets
            std::string fromLower = fromPlanetName;
            std::string toLower = toPlanetName;
            std::transform(fromLower.begin(), fromLower.end(), fromLower.begin(), ::tolower);
            std::transform(toLower.begin(), toLower.end(), toLower.begin(), ::tolower);
            
            for (auto* p : planets) {
                std::string pName = p->getName();
                std::transform(pName.begin(), pName.end(), pName.begin(), ::tolower);
                if (pName == fromLower) fromP = p;
                if (pName == toLower) toP = p;
            }
            
            if (fromP && toP) {
                // Setup travel
                lightTravelActive = true;
                lightTravelFrom = fromP;
                lightTravelTo = toP;
                
                // Position camera near 'from' planet
                lightTravelStartPos = fromP->getPosition() + glm::vec3(0, 0, fromP->getRadius() * 2.0f);
                camera.Position = lightTravelStartPos;
                
                // Destination position
                lightTravelEndPos = toP->getPosition() + glm::vec3(0, 0, toP->getRadius() * 2.0f);
                
                lightTravelProgress = 0.0f;
                lightTravelElapsedTime = 0.0f;
                lightTravelTotalDistance = glm::length(lightTravelEndPos - lightTravelStartPos);
                
                showSearchDialog = false;
                searchBuffer[0] = '\0';
                
                // Force flight mode (hide cursor) for immersion
                cursorEnabled = false;
                glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }
        } else if (!selectedPlanet && !searchStr.empty()) {
          // Normal single planet search
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
        
        if (selectedPlanet && !isTravelCommand) {
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

    // Draw labels for visible planets (only in flight mode OR during light speed travel)
    if (showPlanetLabels && (!cursorEnabled || lightTravelActive)) {
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
    }

    // Compass - Bottom right
    if (showCompass) {
      ImGui::SetNextWindowPos(ImVec2(displayWidth - 10, displayHeight - 10), 
                             ImGuiCond_Always, ImVec2(1.0f, 1.0f));
      ImGui::SetNextWindowBgAlpha(0.88f);
      ImGui::Begin("Planet Compass", &showCompass, ImGuiWindowFlags_AlwaysAutoResize);
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
    }

    // Performance window (optional - FPS also in HUD)
    if (showPerformance) {
      ImGui::SetNextWindowPos(ImVec2(displayWidth / 2 - 75, displayHeight - 10), 
                             ImGuiCond_Always, ImVec2(0.5f, 1.0f));
      ImGui::SetNextWindowBgAlpha(0.88f);
      ImGui::Begin("Performance", &showPerformance, ImGuiWindowFlags_AlwaysAutoResize);
      ImGui::Text("FPS: %.1f", 1.0f / deltaTime);
      ImGui::Text("Frame Time: %.3f ms", deltaTime * 1000.0f);
      ImGui::End();
    }

    // Render Cockpit Overlay
    if (showCockpit && m_Cockpit) m_Cockpit->render();
    
    } // End of !playingLaunch UI block
    
    // Video Playback Overlay
    if (playingLaunch && videoPlayer) {
        // Render video on top of everything (conceptually, actually replaces scene)
        // But since we already rendered scene, we just draw over it.
        // For efficiency we could skip scene render, but simple overlay is safer code-wise.
        videoPlayer->update(deltaTime);
        videoPlayer->render();
        
        if (videoPlayer->isFinished() || glfwGetKey(m_Window, GLFW_KEY_SPACE) == GLFW_PRESS) {
            playingLaunch = false;
        }
        
        // Show Skip Text
        ImGui::SetNextWindowPos(ImVec2(displayWidth - 200, displayHeight - 50));
        ImGui::Begin("Skip", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBackground);
        ImGui::Text("SPACE to Skip");
        ImGui::End();
    }

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
    if (m_Cockpit) delete m_Cockpit;
    if (audioSystem) delete audioSystem;
    if (videoPlayer) delete videoPlayer;
    if (asteroidField) delete asteroidField;
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  glfwDestroyWindow(m_Window);
  glfwTerminate();
}
