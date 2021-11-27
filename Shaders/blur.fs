#version 330 core
out vec4 FragColor;

uniform sampler2D image;
uniform bool horizontal;
uniform float weight[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main()
{            
     vec2 tex_offset = 1.0 / textureSize(image, 0); // gets size of single texel
     vec3 result = texture2D(image, vec2(gl_FragCoord.x/1024, gl_FragCoord.y/720)).rgb * weight[0];
     if(horizontal)
     {
         for(int i = 1; i < 5; ++i)
         {
            result += texture2D(image, vec2(gl_FragCoord.x/1024, gl_FragCoord.y/720) + vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
            result += texture2D(image, vec2(gl_FragCoord.x/1024, gl_FragCoord.y/720) - vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
         }
     }
     else
     {
         for(int i = 1; i < 5; ++i)
         {
             result += texture2D(image, vec2(gl_FragCoord.x/1024, gl_FragCoord.y/720) + vec2(0.0, tex_offset.y * i)).rgb * weight[i];
             result += texture2D(image, vec2(gl_FragCoord.x/1024, gl_FragCoord.y/720) - vec2(0.0, tex_offset.y * i)).rgb * weight[i];
         }
     }
   
     FragColor = vec4(result, 1);
}
