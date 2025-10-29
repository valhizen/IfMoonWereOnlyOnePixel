#version 460 core
layout (location = 0) in vec3 aPos;

out vec3 FragPos;
out vec3 Normal;
out vec3 WorldPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    WorldPos = FragPos;
    Normal = normalize(aPos); // For sphere at origin, position = normal
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
