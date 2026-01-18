#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D cockpitTexture;

void main()
{
    vec4 col = texture(cockpitTexture, TexCoords);
    
    // Green screen keying
    // Check if green is dominant
    float greenness = col.g - max(col.r, col.b);
    
    // Threshold for transparency
    if(greenness > 0.2)
        discard; 
        
    FragColor = col;
}
