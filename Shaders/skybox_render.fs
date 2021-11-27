#version 330 core
out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;  

in vec3 TexCoords;
in vec2 sunTexCoords;

uniform sampler2D sun;
uniform samplerCube skybox;
uniform bool is_sun;

void main()
{    
    FragColor = texture(skybox, TexCoords);
    if (is_sun){
        FragColor = texture(sun, sunTexCoords);
        BrightColor = FragColor;
    }
}