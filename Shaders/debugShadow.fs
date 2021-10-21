#version 330 core
out vec4 FragColor;
  
in vec2 TexCoords;

uniform sampler2D shadowMap;
uniform sampler2D game;

void main()
{   
	vec2 tex_offset = 1.0 / textureSize(shadowMap, 0); // gets size of single texel
    float depthValue = texture2D(shadowMap, vec2(gl_FragCoord.x/1024, gl_FragCoord.y/1024)).r;      
    //float depthValue = texture(shadowMap, TexCoords).r;
    FragColor = texture(game, TexCoords);//vec4(1.0,0.0,1.0,0.5); //vec4(vec3(depthValue), 1.0);
}
