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

const float MOON_DIAMETER_KM = 3474.8f;
const float PIXELS_PER_MOON = 1.0f;
const float PI = 3.14159265359f;
const float TWO_PI = 6.28318530718f;
const float DEG_TO_RAD = PI / 180.0f;
const float EARTH_DAYS_TO_SECONDS = 86400.0f;

Planet::Planet(float diameterKM, std::string planetName, float zPosition, 
               glm::vec3 planetColor, const char* texturePath)
    : name(planetName), 
      color(planetColor), 
      diameterInKM(diameterKM), 
      shader(nullptr), 
      inverted(false),
      orbitParent(nullptr),
      orbitAngle(0.0f),
      trueAnomaly(0.0f),
      currentRotation(0.0f),
      accumulatedTime(0.0f),
      orbitVAO(0),
      orbitVBO(0) {

    float diameterInPixels = (diameterKM / MOON_DIAMETER_KM) * PIXELS_PER_MOON;
    radius = diameterInPixels / 2.0f;
    mass = (diameterKM * diameterKM * diameterKM) / 1000.0f;
    position = glm::vec3(0.0f, 0.0f, zPosition);
    
    // Initialize default parameters
    orbitalParams = {0, 0, 0, 0, 0, 0, 0};
    
    rotationParams.rotationPeriodHours = 24.0f;
    rotationParams.axialTiltDeg = 0.0f;
    rotationParams.initialRotationDeg = 0.0f;
    
    visualParams.radiusScale = 1.0f;
    visualParams.emissiveStrength = 0.0f;
    visualParams.emissiveColor = glm::vec3(0.0f);
    visualParams.showOrbitPath = false;
    visualParams.orbitPathColor = glm::vec3(0.5f, 0.5f, 0.5f);
    visualParams.orbitSegments = 128;
    
    simParams.timeScale = 1.0f;
    simParams.usePhysicalOrbits = true;
    simParams.pauseOrbit = false;
    simParams.pauseRotation = false;
    
    try {
        if (name == "SkySphere") {
            shader = new Shader("shader/shader.vert", "shader/shader1.frag");
            inverted = true;
        } else {
            shader = new Shader("shader/shader.vert", "shader/shader.frag");
        }
    } catch (const std::exception& e) {
        std::cerr << "Error creating shader for " << name << ": " << e.what() << std::endl;
        throw;
    }
    
    textureID = (texturePath != nullptr) ? loadTexture(texturePath) : 0;
    GenerateSphere();
}

void Planet::setOrbit(Planet* parent, const OrbitalParams& params) {
    orbitParent = parent;
    orbitalParams = params;
    
    // Convert semi-major axis from km to pixels
    float orbitRadiusPixels = (params.semiMajorAxisKM / MOON_DIAMETER_KM) * PIXELS_PER_MOON;
    
    // Initialize starting position based on mean anomaly
    orbitAngle = params.meanAnomalyDeg * DEG_TO_RAD;
    
    // CRITICAL: Calculate and set initial orbital position
    position = calculateOrbitalPosition();
    
    if (visualParams.showOrbitPath) {
        GenerateOrbitPath();
    }
}

void Planet::setOrbitParent(Planet* parent, float orbitRad, float orbitSpd) {
    orbitParent = parent;
    
    // Create simple circular orbit parameters
    orbitalParams.semiMajorAxisKM = orbitRad * MOON_DIAMETER_KM;
    orbitalParams.eccentricity = 0.0f;
    orbitalParams.inclinationDeg = 0.0f;
    orbitalParams.longitudeAscNodeDeg = 0.0f;
    orbitalParams.argPeriapsisDeg = 0.0f;
    orbitalParams.meanAnomalyDeg = 0.0f;
    
    // Calculate period from speed or use Kepler's 3rd law
    if (orbitSpd > 0.0f) {
        float angularVelocityRadPerSec = orbitSpd;
        float periodSeconds = TWO_PI / angularVelocityRadPerSec;
        orbitalParams.orbitalPeriodDays = periodSeconds / EARTH_DAYS_TO_SECONDS;
    } else if (parent != nullptr) {
        // Use simplified Kepler's 3rd law: T² ∝ a³
        const float EARTH_PERIOD_DAYS = 365.256f;
        const float EARTH_DISTANCE_KM = 149600000.0f;
        float ratio = orbitalParams.semiMajorAxisKM / EARTH_DISTANCE_KM;
        orbitalParams.orbitalPeriodDays = EARTH_PERIOD_DAYS * std::pow(ratio, 1.5f);
    }
    
    // Calculate and set initial orbital position
    orbitAngle = 0.0f;
    position = calculateOrbitalPosition();
}

void Planet::setRotation(const RotationParams& params) {
    rotationParams = params;
    currentRotation = params.initialRotationDeg * DEG_TO_RAD;
}

void Planet::setVisualParams(const VisualParams& params) {
    visualParams = params;
    if (params.showOrbitPath && orbitParent != nullptr) {
        GenerateOrbitPath();
    }
}

void Planet::setSimulationParams(const SimulationParams& params) {
    simParams = params;
}

// Solve Kepler's equation: M = E - e*sin(E) for eccentric anomaly E
float Planet::solveKeplerEquation(float M, float e, int maxIter) {
    float E = M; // Initial guess
    for (int i = 0; i < maxIter; i++) {
        float dE = (E - e * std::sin(E) - M) / (1.0f - e * std::cos(E));
        E -= dE;
        if (std::abs(dE) < 1e-6f) break;
    }
    return E;
}

glm::vec3 Planet::calculateOrbitalPosition() {
    if (orbitParent == nullptr) {
        return position;
    }
    
    float a = (orbitalParams.semiMajorAxisKM / MOON_DIAMETER_KM) * PIXELS_PER_MOON;
    float e = orbitalParams.eccentricity;
    
    if (simParams.usePhysicalOrbits && e > 0.001f) {
        // Elliptical orbit using Kepler's laws
        float E = solveKeplerEquation(orbitAngle, e);
        
        // Calculate true anomaly from eccentric anomaly
        trueAnomaly = 2.0f * std::atan2(
            std::sqrt(1.0f + e) * std::sin(E / 2.0f),
            std::sqrt(1.0f - e) * std::cos(E / 2.0f)
        );
        
        // Distance from focus (parent body)
        float r = a * (1.0f - e * std::cos(E));
        
        // Position in orbital plane
        float x = r * std::cos(trueAnomaly);
        float y = r * std::sin(trueAnomaly);
        
        // Apply orbital element rotations
        glm::vec3 orbitalPos(x, 0.0f, y);
        
        // Rotate by argument of periapsis
        float w = orbitalParams.argPeriapsisDeg * DEG_TO_RAD;
        glm::mat4 rotW = glm::rotate(glm::mat4(1.0f), w, glm::vec3(0, 1, 0));
        
        // Rotate by inclination
        float i = orbitalParams.inclinationDeg * DEG_TO_RAD;
        glm::mat4 rotI = glm::rotate(glm::mat4(1.0f), i, glm::vec3(1, 0, 0));
        
        // Rotate by longitude of ascending node
        float omega = orbitalParams.longitudeAscNodeDeg * DEG_TO_RAD;
        glm::mat4 rotOmega = glm::rotate(glm::mat4(1.0f), omega, glm::vec3(0, 1, 0));
        
        glm::vec4 finalPos = rotOmega * rotI * rotW * glm::vec4(orbitalPos, 1.0f);
        return orbitParent->getPosition() + glm::vec3(finalPos);
        
    } else {
        // Simple circular orbit
        float x = a * std::cos(orbitAngle);
        float z = a * std::sin(orbitAngle);
        
        // Apply inclination for circular orbits
        float i = orbitalParams.inclinationDeg * DEG_TO_RAD;
        float y = z * std::sin(i);
        z = z * std::cos(i);
        
        return orbitParent->getPosition() + glm::vec3(x, y, z);
    }
}

void Planet::update(float deltaTime) {
    if (orbitParent == nullptr) return;
    
    float scaledDelta = deltaTime * simParams.timeScale;
    accumulatedTime += scaledDelta;
    
    // Update orbital position
    if (!simParams.pauseOrbit && orbitalParams.orbitalPeriodDays > 0.0f) {
        float periodSeconds = orbitalParams.orbitalPeriodDays * EARTH_DAYS_TO_SECONDS;
        float angularVelocity = TWO_PI / periodSeconds;
        orbitAngle += angularVelocity * scaledDelta;
        
        // Keep in range [0, 2π]
        while (orbitAngle > TWO_PI) orbitAngle -= TWO_PI;
        while (orbitAngle < 0) orbitAngle += TWO_PI;
        
        position = calculateOrbitalPosition();
    }
    
    // Update axial rotation
    if (!simParams.pauseRotation && rotationParams.rotationPeriodHours > 0.0f) {
        float rotPeriodSeconds = rotationParams.rotationPeriodHours * 3600.0f;
        float rotAngularVelocity = TWO_PI / rotPeriodSeconds;
        currentRotation += rotAngularVelocity * scaledDelta;
        
        while (currentRotation > TWO_PI) currentRotation -= TWO_PI;
    }
}

glm::mat4 Planet::getRotationMatrix() {
    glm::mat4 rot = glm::mat4(1.0f);
    
    // Apply axial tilt
    float tilt = rotationParams.axialTiltDeg * DEG_TO_RAD;
    rot = glm::rotate(rot, tilt, glm::vec3(0, 0, 1));
    
    // Apply rotation
    rot = glm::rotate(rot, currentRotation, glm::vec3(0, 1, 0));
    
    return rot;
}

void Planet::GenerateSphere() {
    std::vector<glm::vec3> positions;
    std::vector<unsigned int> indices;
    std::vector<glm::vec2> uv;
    std::vector<glm::vec3> normals;
    
    const unsigned int X_SEGMENTS = 64;
    const unsigned int Y_SEGMENTS = 64;
    
    for (unsigned int y = 0; y <= Y_SEGMENTS; ++y) {
        for (unsigned int x = 0; x <= X_SEGMENTS; ++x) {
            float xSegment = (float)x / (float)X_SEGMENTS;
            float ySegment = (float)y / (float)Y_SEGMENTS;
            
            float xPos = cos(xSegment * TWO_PI) * sin(ySegment * PI);
            float yPos = cos(ySegment * PI);
            float zPos = sin(xSegment * TWO_PI) * sin(ySegment * PI);
            
            positions.push_back(glm::vec3(xPos, yPos, zPos));
            normals.push_back(glm::vec3(xPos, yPos, zPos));
            uv.push_back(glm::vec2(xSegment, ySegment));
        }
    }
    
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
    
    unsigned int stride = 8 * sizeof(float);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
    
    glBindVertexArray(0);
}

void Planet::GenerateOrbitPath() {
    if (orbitParent == nullptr) return;
    
    std::vector<glm::vec3> orbitPoints;
    int segments = visualParams.orbitSegments;
    float a = (orbitalParams.semiMajorAxisKM / MOON_DIAMETER_KM) * PIXELS_PER_MOON;
    float e = orbitalParams.eccentricity;
    
    // Prepare rotation matrices (same as in calculateOrbitalPosition)
    float w = orbitalParams.argPeriapsisDeg * DEG_TO_RAD;
    float inc = orbitalParams.inclinationDeg * DEG_TO_RAD;
    float omega = orbitalParams.longitudeAscNodeDeg * DEG_TO_RAD;
    
    glm::mat4 rotW = glm::rotate(glm::mat4(1.0f), w, glm::vec3(0, 1, 0));
    glm::mat4 rotI = glm::rotate(glm::mat4(1.0f), inc, glm::vec3(1, 0, 0));
    glm::mat4 rotOmega = glm::rotate(glm::mat4(1.0f), omega, glm::vec3(0, 1, 0));
    glm::mat4 fullRotation = rotOmega * rotI * rotW;
    
    for (int i = 0; i <= segments; i++) {
        float angle = (float)i / segments * TWO_PI;
        glm::vec3 orbitalPos;
        
        if (e > 0.001f) {
            // Elliptical path
            float E = solveKeplerEquation(angle, e);
            float r = a * (1.0f - e * std::cos(E));
            float nu = 2.0f * std::atan2(std::sqrt(1.0f + e) * std::sin(E / 2.0f),
                                         std::sqrt(1.0f - e) * std::cos(E / 2.0f));
            
            float x = r * std::cos(nu);
            float y = r * std::sin(nu);
            orbitalPos = glm::vec3(x, 0.0f, y);
        } else {
            // Circular path
            float x = a * std::cos(angle);
            float z = a * std::sin(angle);
            orbitalPos = glm::vec3(x, 0.0f, z);
        }
        
        // Apply the same rotations as calculateOrbitalPosition
        glm::vec4 rotatedPos = fullRotation * glm::vec4(orbitalPos, 1.0f);
        orbitPoints.push_back(glm::vec3(rotatedPos));
    }
    
    glGenVertexArrays(1, &orbitVAO);
    glGenBuffers(1, &orbitVBO);
    glBindVertexArray(orbitVAO);
    glBindBuffer(GL_ARRAY_BUFFER, orbitVBO);
    glBufferData(GL_ARRAY_BUFFER, orbitPoints.size() * sizeof(glm::vec3), 
                 &orbitPoints[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glBindVertexArray(0);
}

void Planet::renderOrbitPath(const glm::mat4& view, const glm::mat4& projection) {
    if (!visualParams.showOrbitPath || orbitVAO == 0 || orbitParent == nullptr) return;
    
    shader->use();
    glm::mat4 model = glm::translate(glm::mat4(1.0f), orbitParent->getPosition());
    shader->setMat4("model", model);
    shader->setMat4("view", view);
    shader->setMat4("projection", projection);
    shader->setVec3("objectColor", visualParams.orbitPathColor);
    shader->setBool("useTexture", false);
    
    glBindVertexArray(orbitVAO);
    glDrawArrays(GL_LINE_LOOP, 0, visualParams.orbitSegments + 1);
    glBindVertexArray(0);
}

void Planet::renderSphere(const glm::mat4& view, const glm::mat4& projection, 
                          glm::vec3 campos) {
    if (shader == nullptr) return;
    
    shader->use();
    
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
    model = model * getRotationMatrix();
    model = glm::scale(model, glm::vec3(radius * visualParams.radiusScale));
    
    shader->setMat4("model", model);
    shader->setMat4("view", view);
    shader->setMat4("projection", projection);
    shader->setVec3("viewPos", campos);
    shader->setFloat("time", accumulatedTime);
    shader->setVec3("objectColor", color);
    
    if (inverted) glFrontFace(GL_CW);
    else glFrontFace(GL_CCW);
        
    glBindVertexArray(sphereVAO);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    glFrontFace(GL_CCW);
}

// Getters implementation
glm::vec3 Planet::getPosition() const { return position; }
std::string Planet::getName() const { return name; }
float Planet::getRadius() const { return radius * visualParams.radiusScale; }
float Planet::getDiameterKM() const { return diameterInKM; }
float Planet::getTrueAnomaly() const { return trueAnomaly; }
float Planet::getMeanAnomaly() const { return orbitAngle; }

float Planet::getDistanceFromParent() const {
    if (orbitParent == nullptr) return 0.0f;
    return glm::length(position - orbitParent->getPosition());
}

float Planet::getOrbitalVelocity() const {
    if (orbitalParams.orbitalPeriodDays <= 0.0f) return 0.0f;
    float a = (orbitalParams.semiMajorAxisKM / MOON_DIAMETER_KM) * PIXELS_PER_MOON;
    float periodSeconds = orbitalParams.orbitalPeriodDays * EARTH_DAYS_TO_SECONDS;
    return TWO_PI * a / periodSeconds;
}

void Planet::setPosition(const glm::vec3& pos) { position = pos; }
void Planet::setTimeScale(float scale) { simParams.timeScale = scale; }
void Planet::setRadiusScale(float scale) { visualParams.radiusScale = scale; }

void Planet::resetOrbit() {
    orbitAngle = orbitalParams.meanAnomalyDeg * DEG_TO_RAD;
    currentRotation = rotationParams.initialRotationDeg * DEG_TO_RAD;
    accumulatedTime = 0.0f;
    position = calculateOrbitalPosition();
}

unsigned int Planet::loadTexture(const char* path) {
    // [Same as before - texture loading code]
    std::string filePath = path;
    unsigned int textureID;
    glGenTextures(1, &textureID);
    
    std::string ext;
    size_t dotPos = filePath.find_last_of('.');
    if (dotPos != std::string::npos)
        ext = filePath.substr(dotPos + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    if (ext == "tif" || ext == "tiff") {
        readTiffImage(const_cast<char*>(filePath.c_str()), &textureID);
        return textureID;
    }
    
    int width, height, nrComponents;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data) {
        GLenum format = (nrComponents == 1) ? GL_RED : 
                       (nrComponents == 3) ? GL_RGB : GL_RGBA;
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format,
                     GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        stbi_image_free(data);
    } else {
        std::cerr << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }
    return textureID;
}

Planet::~Planet() {
    glDeleteVertexArrays(1, &sphereVAO);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    if (orbitVAO) {
        glDeleteVertexArrays(1, &orbitVAO);
        glDeleteBuffers(1, &orbitVBO);
    }
    if (shader != nullptr) delete shader;
}

// REAL PLANET PRESETS
namespace PlanetPresets {
    Planet::OrbitalParams getEarthOrbit() {
        return {149598023.0f, 0.0167f, 365.256f, 0.0f, 0.0f, 102.9f, 0.0f};
    }
    
    Planet::OrbitalParams getMercuryOrbit() {
        return {57909050.0f, 0.2056f, 87.97f, 7.0f, 48.3f, 29.1f, 0.0f};
    }
    
    Planet::OrbitalParams getVenusOrbit() {
        return {108208000.0f, 0.0068f, 224.7f, 3.4f, 76.7f, 54.9f, 0.0f};
    }
    
    Planet::OrbitalParams getMarsOrbit() {
        return {227939200.0f, 0.0934f, 686.98f, 1.85f, 49.6f, 286.5f, 0.0f};
    }
    
    Planet::OrbitalParams getJupiterOrbit() {
        return {778570000.0f, 0.0489f, 4332.59f, 1.3f, 100.5f, 273.9f, 0.0f};
    }
    
    Planet::OrbitalParams getSaturnOrbit() {
        return {1433530000.0f, 0.0565f, 10759.22f, 2.5f, 113.7f, 339.4f, 0.0f};
    }
    
    Planet::OrbitalParams getUranusOrbit() {
        return {2875040000.0f, 0.0457f, 30688.5f, 0.77f, 74.0f, 96.6f, 0.0f};
    }
    
    Planet::OrbitalParams getNeptuneOrbit() {
        return {4504450000.0f, 0.0113f, 60182.0f, 1.77f, 131.8f, 276.3f, 0.0f};
    }
    
    Planet::RotationParams getEarthRotation() {
        return {24.0f, 23.44f, 0.0f};
    }
    
    Planet::RotationParams getMercuryRotation() {
        return {1407.6f, 0.03f, 0.0f};
    }
    
    Planet::RotationParams getVenusRotation() {
        return {-5832.5f, 177.4f, 0.0f}; // Negative = retrograde
    }
    
    Planet::RotationParams getMarsRotation() {
        return {24.6f, 25.19f, 0.0f};
    }
    
    Planet::RotationParams getJupiterRotation() {
        return {9.9f, 3.13f, 0.0f};
    }
    
    Planet::RotationParams getSaturnRotation() {
        return {10.7f, 26.73f, 0.0f};
    }
    
    Planet::RotationParams getUranusRotation() {
        return {-17.2f, 97.77f, 0.0f}; // Retrograde, extreme tilt
    }
    
    Planet::RotationParams getNeptuneRotation() {
        return {16.1f, 28.32f, 0.0f};
    }
}
