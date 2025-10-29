#version 460 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec3 WorldPos;

uniform vec3 viewPos;
uniform float time;

// Hash function for pseudo-random noise
float hash(vec3 p) {
    p = fract(p * 0.3183099 + 0.1);
    p *= 17.0;
    return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}

// Generate a static, sparse starfield
float starfield(vec3 dir) {
    vec3 p = normalize(dir) * 800.0;
    float stars = 0.0;

    // 3 layers of sparse points
    for (int i = 0; i < 3; i++) {
        vec3 q = floor(p * (0.5 + float(i) * 0.6));
        float h = hash(q);

        // Reduce star density (was 0.996)
        if (h > 0.9995) {
            // static brightness — no twinkle
            stars += h * 0.8;
        }
    }

    return clamp(stars, 0.0, 1.0);
}

// Soft nebula effect for color depth
vec3 nebula(vec3 dir) {
    vec3 n = fract(sin(vec3(dot(dir, vec3(12.9898, 78.233, 37.719)),
                             dot(dir, vec3(39.346, 11.135, 83.155)),
                             dot(dir, vec3(73.156, 52.345, 9.353)))) * 43758.5453);
    float cloud = smoothstep(0.65, 0.92, n.x * n.y * n.z);
    vec3 nebulaColor = vec3(0.08, 0.1, 0.25) * cloud;
    return nebulaColor;
}

void main()
{
    vec3 dir = normalize(WorldPos - viewPos);

    // Base dark blue space color
    vec3 baseColor = vec3(0.01, 0.01, 0.025);

    // Stars and nebula
    vec3 stars = vec3(1.0) * starfield(dir);
    vec3 neb = nebula(dir) * 0.8;

    vec3 finalColor = baseColor + stars + neb;

    FragColor = vec4(finalColor, 1.0);
}
