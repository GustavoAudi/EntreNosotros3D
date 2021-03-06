  #version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D scene;
uniform sampler2D bloomBlur;
uniform bool bloom;
uniform float exposure;

void main()
{             
    const float gamma = 2.2;
    vec3 hdrColor = texture2D(scene, vec2(gl_FragCoord.x/1024, gl_FragCoord.y/720)).rgb;
    vec3 bloomColor = texture2D(bloomBlur, vec2(gl_FragCoord.x/1024, gl_FragCoord.y/720)).rgb;
    hdrColor += bloomColor; // additive blending
    // tone mapping
    vec3 result =  exp(hdrColor * exposure);
    // also gamma correct while we're at it
    result = (2*hdrColor)/2 //pow(hdrColor, vec3(1.0 / gamma));
    FragColor = vec4(result, 1.0);//vec4(bloomColor, 1.0);
}