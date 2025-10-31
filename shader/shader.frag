#version 460 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec3 WorldPos;
in vec2 TexCoord;

uniform vec3 objectColor;
uniform vec3 viewPos;
uniform float time;
uniform bool useTexture;
uniform sampler2D texture1;

// Hash function for pseudo-random numbers
float hash(vec3 p) {
    p = fract(p * 0.3183099 + 0.1);
    p *= 17.0;
    return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}

// Generate starfield
float starfield(vec3 dir) {
    vec3 p = dir * 1000.0;
    float stars = 0.0;
    
    for(int i = 0; i < 3; i++) {
        vec3 q = floor(p * (1.0 + float(i) * 0.5));
        float h = hash(q);
        
        if(h > 0.995) {
            vec3 center = q + 0.5;
            float dist = length(p * (1.0 + float(i) * 0.5) - center);
            float twinkle = sin(time * 2.0 + h * 100.0) * 0.5 + 0.5;
            stars += (1.0 - smoothstep(0.0, 0.3, dist)) * (0.3 + twinkle * 0.7) * h;
        }
    }
    
    return stars;
}

// Nebula effect
vec3 nebula(vec3 dir) {
    float n1 = hash(floor(dir * 2.0 + time * 0.01));
    float n2 = hash(floor(dir * 3.0 - time * 0.015));
    
    vec3 nebulaColor1 = vec3(0.3, 0.1, 0.5) * n1 * 0.1;
    vec3 nebulaColor2 = vec3(0.1, 0.2, 0.4) * n2 * 0.08;
    
    return nebulaColor1 + nebulaColor2;
}

void main()
{
    vec3 viewDir = normalize(WorldPos - viewPos);
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
    vec3 norm = normalize(Normal);
    
    // Get base color from texture or uniform
    vec3 baseColor;
    if (useTexture) {
        baseColor = texture(texture1, TexCoord).rgb;
    } else {
        baseColor = objectColor;
    }
    
    // Ambient
    float ambient = 0.25;
    
    // Diffuse
    float diff = max(dot(norm, lightDir), 0.0);
    
    // Specular highlight
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0) * 0.3;
    
    // Planet color with lighting
    vec3 planetColor = (ambient + diff) * baseColor + spec;
    
    // Add subtle rim lighting
    float rimFactor = 1.0 - max(dot(viewDir, norm), 0.0);
    rimFactor = pow(rimFactor, 3.0);
    vec3 rimColor = baseColor * 0.5 * rimFactor;
    
    // Combine planet lighting
    vec3 result = planetColor + rimColor;
    
    // Add stars in the background
    float starIntensity = starfield(viewDir);
    vec3 stars = vec3(1.0) * starIntensity * (1.0 - diff * 0.5);
    
    // Add nebula effect
    vec3 nebulaEffect = nebula(viewDir) * (1.0 - diff);
    
    // Final composition
    result +=nebulaEffect;
    
    FragColor = vec4(result, 1.0);
}
