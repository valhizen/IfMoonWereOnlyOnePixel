#ifndef PLANET_HPP
#define PLANET_HPP

#include <glm/glm.hpp>
#include <string>
#include "Shader.hpp"

class Planet {
public:
    // Enhanced constructor with more controls
    Planet(float diameterKM, std::string planetName, float zPosition, 
           glm::vec3 planetColor, const char* texturePath);
    
    ~Planet();
    
    // Core rendering
    void renderSphere(const glm::mat4& view, const glm::mat4& projection, 
                      glm::vec3 campos);
    void update(float deltaTime);
    
    // Orbit configuration - Enhanced with real astronomical data
    struct OrbitalParams {
        float semiMajorAxisKM;      // Average distance from parent (km)
        float eccentricity;          // Orbital eccentricity (0 = circle, <1 = ellipse)
        float orbitalPeriodDays;     // Real orbital period in Earth days
        float inclinationDeg;        // Orbital plane tilt in degrees
        float longitudeAscNodeDeg;   // Longitude of ascending node
        float argPeriapsisDeg;       // Argument of periapsis
        float meanAnomalyDeg;        // Starting position in orbit
    };
    
    // Set orbit with full Keplerian elements
    void setOrbit(Planet* parent, const OrbitalParams& params);
    
    // Simple orbit setter (backwards compatible)
    void setOrbitParent(Planet* parent, float orbitRad, float orbitSpd = 0.0f);
    
    // Rotation controls
    struct RotationParams {
        float rotationPeriodHours;   // Axial rotation period (hours)
        float axialTiltDeg;          // Axial tilt in degrees
        float initialRotationDeg;    // Starting rotation angle
    };
    
    void setRotation(const RotationParams& params);
    
    // Visual controls
    struct VisualParams {
        float radiusScale;           // Visual scale multiplier (for visibility)
        float emissiveStrength;      // Self-illumination (for stars)
        glm::vec3 emissiveColor;     // Glow color
        bool showOrbitPath;          // Draw orbital path
        glm::vec3 orbitPathColor;    // Orbit line color
        int orbitSegments;           // Orbit path detail
    };
    
    void setVisualParams(const VisualParams& params);
    
    // Simulation controls
    struct SimulationParams {
        float timeScale;             // Time multiplier (1.0 = real time)
        bool usePhysicalOrbits;      // Use Kepler's laws vs simple circular
        bool pauseOrbit;             // Freeze orbital motion
        bool pauseRotation;          // Freeze axial rotation
    };
    
    void setSimulationParams(const SimulationParams& params);
    
    // Getters
    glm::vec3 getPosition() const;
    std::string getName() const;
    float getRadius() const;
    float getDiameterKM() const;
    float getMass() const { return mass; }
    float getOrbitalPeriodDays() const { return orbitalParams.orbitalPeriodDays; }
    float getDistanceFromParent() const;
    float getOrbitalVelocity() const;
    glm::vec3 getVelocityVector() const;
    
    // Advanced getters
    float getTrueAnomaly() const;        // Current angle in orbit
    float getEccentricAnomaly() const;   // Eccentric anomaly
    float getMeanAnomaly() const;        // Mean anomaly
    float getDistanceToParent() const;   // Current distance (varies for ellipse)
    
    // Setters for manual control
    void setPosition(const glm::vec3& pos);
    void setTimeScale(float scale);
    void setRadiusScale(float scale);
    
    // Utility
    void resetOrbit();                   // Reset to starting position
    void renderOrbitPath(const glm::mat4& view, const glm::mat4& projection);
    
    bool inverted;

private:
    // Core properties
    std::string name;
    glm::vec3 color;
    glm::vec3 position;
    float radius;
    float diameterInKM;
    float mass;
    unsigned int textureID;
    
    // Rendering
    Shader* shader;
    unsigned int sphereVAO, vbo, ebo;
    unsigned int indexCount;
    unsigned int orbitVAO, orbitVBO;
    
    // Orbital mechanics
    Planet* orbitParent;
    OrbitalParams orbitalParams;
    float orbitAngle;                    // Current mean anomaly (radians)
    float trueAnomaly;                   // True anomaly (radians)
    glm::vec3 orbitCenter;              // Center of elliptical orbit
    
    // Rotation
    RotationParams rotationParams;
    float currentRotation;               // Current axial rotation angle
    
    // Visual settings
    VisualParams visualParams;
    
    // Simulation settings
    SimulationParams simParams;
    
    // Time tracking
    float accumulatedTime;
    
    // Helper methods
    void GenerateSphere();
    void GenerateOrbitPath();
    unsigned int loadTexture(const char* path);
    float solveKeplerEquation(float M, float e, int maxIter = 100);
    glm::vec3 calculateOrbitalPosition();
    glm::mat4 getRotationMatrix();
};

// Preset configurations for real planets
namespace PlanetPresets {
    Planet::OrbitalParams getMercuryOrbit();
    Planet::OrbitalParams getVenusOrbit();
    Planet::OrbitalParams getEarthOrbit();
    Planet::OrbitalParams getMarsOrbit();
    Planet::OrbitalParams getJupiterOrbit();
    Planet::OrbitalParams getSaturnOrbit();
    Planet::OrbitalParams getUranusOrbit();
    Planet::OrbitalParams getNeptuneOrbit();
    
    Planet::RotationParams getMercuryRotation();
    Planet::RotationParams getVenusRotation();
    Planet::RotationParams getEarthRotation();
    Planet::RotationParams getMarsRotation();
    Planet::RotationParams getJupiterRotation();
    Planet::RotationParams getSaturnRotation();
    Planet::RotationParams getUranusRotation();
    Planet::RotationParams getNeptuneRotation();
}

#endif // PLANET_HPP
