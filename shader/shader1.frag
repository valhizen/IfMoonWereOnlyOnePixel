#version 460 core
out vec4 FragColor;
in vec3 FragPos;
in vec3 Normal;
in vec3 WorldPos;

uniform vec3 viewPos;
uniform float time;

// Improved hash function with better distribution
float hash(vec3 p) {
    p = fract(p * vec3(0.1031, 0.1030, 0.0973));
    p += dot(p, p.yxz + 33.33);
    return fract((p.x + p.y) * p.z);
}

// 3D noise function for nebula
float noise(vec3 x) {
    vec3 p = floor(x);
    vec3 f = fract(x);
    f = f * f * (3.0 - 2.0 * f);
    
    float n = p.x + p.y * 157.0 + 113.0 * p.z;
    return mix(
        mix(mix(hash(p + vec3(0, 0, 0)), hash(p + vec3(1, 0, 0)), f.x),
            mix(hash(p + vec3(0, 1, 0)), hash(p + vec3(1, 1, 0)), f.x), f.y),
        mix(mix(hash(p + vec3(0, 0, 1)), hash(p + vec3(1, 0, 1)), f.x),
            mix(hash(p + vec3(0, 1, 1)), hash(p + vec3(1, 1, 1)), f.x), f.y), f.z);
}

// Fractal Brownian Motion for complex patterns
float fbm(vec3 p) {
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;
    
    for (int i = 0; i < 5; i++) {
        value += amplitude * noise(p * frequency);
        frequency *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}

// Multi-layered starfield with varying sizes and brightness
float starfield(vec3 dir) {
    vec3 p = normalize(dir) * 1000.0;
    float stars = 0.0;
    
    // Layer 1: Bright stars (rare)
    vec3 q1 = floor(p * 0.5);
    float h1 = hash(q1);
    if (h1 > 0.9998) {
        float brightness = pow(h1, 3.0);
        stars += brightness * 1.5;
    }
    
    // Layer 2: Medium stars
    vec3 q2 = floor(p * 1.2);
    float h2 = hash(q2);
    if (h2 > 0.9995) {
        float brightness = pow(h2, 2.0);
        stars += brightness * 0.9;
    }
    
    // Layer 3: Distant dim stars (more frequent)
    vec3 q3 = floor(p * 2.5);
    float h3 = hash(q3);
    if (h3 > 0.999) {
        stars += h3 * 0.3;
    }
    
    return clamp(stars, 0.0, 1.0);
}

// Star color variation (blue, white, yellow, red giants)
vec3 starColor(vec3 dir) {
    vec3 p = normalize(dir) * 1000.0;
    vec3 q = floor(p * 0.5);
    float h = hash(q + vec3(7.123));
    
    // Color temperature based on hash
    if (h < 0.3) return vec3(0.7, 0.8, 1.0);      // Blue-white (hot)
    else if (h < 0.7) return vec3(1.0, 1.0, 0.95); // White
    else if (h < 0.9) return vec3(1.0, 0.9, 0.7);  // Yellow
    else return vec3(1.0, 0.6, 0.4);               // Red (cool giants)
}

// Enhanced nebula with multiple colors and layers
vec3 nebula(vec3 dir) {
    vec3 p = dir * 3.0;
    
    // Multi-octave noise for cloud structure
    float density = fbm(p);
    float density2 = fbm(p * 2.3 + vec3(5.2, 1.3, 9.2));
    
    // Color layers - purple, blue, pink nebula regions
    vec3 color1 = vec3(0.3, 0.1, 0.5) * pow(density, 2.0);        // Purple
    vec3 color2 = vec3(0.1, 0.2, 0.6) * pow(density2, 3.0);       // Blue
    vec3 color3 = vec3(0.5, 0.15, 0.3) * pow(density * density2, 4.0); // Pink
    
    // Bright emission regions
    float emission = pow(fbm(p * 5.0), 4.0) * 0.15;
    vec3 emissionColor = vec3(0.8, 0.4, 0.6) * emission;
    
    return (color1 + color2 + color3 + emissionColor) * 0.4;
}

// Milky Way band across sky
float milkyWay(vec3 dir) {
    // Create a band across the sky
    float band = abs(dir.y);
    band = smoothstep(0.7, 0.2, band);
    
    // Add texture to the band
    float detail = fbm(dir * 20.0);
    band *= detail;
    
    return band * 0.3;
}

// Subtle color gradient based on direction
vec3 ambientGradient(vec3 dir) {
    // Dark blue at "zenith", slightly warmer at "horizon"
    float upness = dir.y * 0.5 + 0.5;
    vec3 zenith = vec3(0.005, 0.008, 0.015);
    vec3 horizon = vec3(0.015, 0.012, 0.025);
    return mix(horizon, zenith, upness);
}

void main()
{
    vec3 dir = normalize(WorldPos - viewPos);
    
    // Base gradient color
    vec3 baseColor = ambientGradient(dir);
    
    // Add Milky Way band
    float milky = milkyWay(dir);
    vec3 milkyColor = vec3(0.6, 0.5, 0.4) * milky;
    
    // Add nebula clouds
    vec3 nebulaColor = nebula(dir);
    
    // Add stars with color variation
    float starMask = starfield(dir);
    vec3 stars = starColor(dir) * starMask;
    
    // Add subtle star glow
    float starGlow = pow(starMask, 0.5) * 0.1;
    stars += vec3(starGlow);
    
    // Composite everything
    vec3 finalColor = baseColor + milkyColor + nebulaColor + stars;
    
    // Subtle tone mapping for better contrast
    finalColor = finalColor / (finalColor + vec3(1.0));
    finalColor = pow(finalColor, vec3(0.9)); // Slight gamma adjustment
    
    FragColor = vec4(finalColor, 1.0);
}
