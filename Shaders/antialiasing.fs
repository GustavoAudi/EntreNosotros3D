#version 330 core
out vec4 FragColor;
//out vec4 BrightColor;

in vec2 TexCoords;
in vec3 Normal;

uniform sampler2DMS image;
//uniform sampler2DMS bright;
uniform int samples;

void main()
{         
	ivec2 texturePosition = ivec2(gl_FragCoord.x, gl_FragCoord.y);
	vec4 colorSample = vec4(0.0f);
	//vec4 brightSample = vec4(0.0f);
	for(int i = 0; i < samples; i ++){
		colorSample += texelFetch(image, texturePosition, i);
		//brightSample += texelFetch(bright, texturePosition, i);
	}

	FragColor = colorSample;// / samples;
	//BrightColor = brightSample / samples;
}