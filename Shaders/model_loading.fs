#version 330 core
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;  

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;
in vec4 gl_FragCoord; 

struct Material {
    vec3 Diffuse;
    vec3 Specular;
    vec3 Ambient;
    vec3 Emissive;
    float Transparent;
    float Shininess;
};

struct DirLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {
    vec3 position;
    float constant;
    float linear;
    float quadratic;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct SpotLight {
    vec3 position;
    vec3 direction;     
};

struct SpotLightCharacter {
    vec3 position;
    vec3 direction;    
    float cutOff;
    float outerCutOff;
    float constant;
    float linear;
    float quadratic;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;  
};

#define NR_POINT_LIGHTS 17
#define NR_SPOT_LIGHTS 72

uniform bool apagon;
uniform bool linterna;
uniform bool specular_map;
uniform vec3 viewPos;

uniform PointLight pointLights[NR_POINT_LIGHTS];

uniform SpotLightCharacter characterLight;

uniform SpotLight spotLights[NR_SPOT_LIGHTS];
uniform float cutOffSpot;
uniform float outerCutOffSpot;
uniform float constantSpot;
uniform float linearSpot;
uniform float quadraticSpot;
uniform vec3 ambientSpot;
uniform vec3 diffuseSpot;
uniform vec3 specularSpot;  

uniform Material material;
uniform sampler2D texture_diffuse1;

// function prototypes
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
vec3 CalcSpotLightCharacter(SpotLightCharacter light, vec3 normal, vec3 fragPos, vec3 viewDir);
float LinearizeDepth(float depth);

float near = 0.1; 
float far  = 100.0; 

void main()
{   
      vec3 norm = normalize(Normal);
      vec3 viewDir = normalize(viewPos - FragPos);
      vec4 tex = texture(texture_diffuse1, TexCoords);
      float depth = LinearizeDepth(gl_FragCoord.z); // divide by far for demonstration      float ndc = depth * 2.0 - 1.0; 

      vec3 result = vec3(0.0f);
   
      if(!apagon){   
           for(int i = 0; i < NR_POINT_LIGHTS; i++){
                result += CalcPointLight(pointLights[i], norm, FragPos, viewDir);
           }   
      }

      for(int i = 0; i < NR_SPOT_LIGHTS; i++){
         float distance = length(spotLights[i].position - viewPos);
         if(distance < 7f){
              result += CalcSpotLight(spotLights[i], norm, FragPos, viewDir);
         }
      }

      
      if(linterna)
      {
           if (result.x + result.y + result.z < 0.f){
                result = CalcSpotLightCharacter(characterLight, norm, FragPos, viewDir);    
           }else{
                result += CalcSpotLightCharacter(characterLight, norm, FragPos, viewDir);    
           }
      }

      //Emission
      vec3 emission;
      if(tex.rgb == vec3(0.f)){
        emission = material.Emissive;
      }else{
        emission = material.Emissive * tex.rgb;
      }
      if(material.Diffuse == vec3(0.360784, 0.683922, 0.690196)){
         emission = diffuseSpot;
      }

      if (result.x + result.y + result.z < 0.f){
         result = emission;
      }else{
         result += emission;
      }
      //Fadeout
      float fadeout;
      float maxDepth = 20.0f;
      if(depth>maxDepth){
        fadeout = 0;
      } else{
        fadeout = (maxDepth-depth)/maxDepth;
      }
     
      float transparent = material.Transparent;
      if(vec3(tex) != vec3(0.0f))
           transparent = tex.w;

      //Alpha lod
      float alphaLOD;
      float maxLODdist= 60.0f;
      if((depth>maxLODdist) && (vec3(tex) != vec3(0.f))){
        alphaLOD = 0;
      } else{
        alphaLOD = (maxLODdist-depth)/maxLODdist;
      }

      if(material.Emissive != vec3(0.0))
          BrightColor = vec4(result, transparent);
      else
          BrightColor = vec4(0.0, 0.0, 0.0, transparent);

      FragColor = vec4(result,transparent);   
      
}

float LinearizeDepth(float depth) 
{
    float z = depth * 2.0 - 1.0; // back to NDC 
    return (2.0 * near * far) / (far + near - z * (far - near));	
}


vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 tex =  vec3(texture(texture_diffuse1, TexCoords));

    vec3 lightDir = normalize(light.position - fragPos);
   
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
   
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.Shininess);
   
    // attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));    
    
    // combine results
    vec3 newDiffuse = material.Diffuse;
    vec3 newSpecular = material.Specular;
    if(material.Diffuse == vec3(0.360784, 0.683922, 0.690196)){
           newDiffuse = diffuseSpot;
           newSpecular = specularSpot;
    }
    vec3 ambient = light.ambient * newDiffuse;
    if(tex != vec3(0.f))
            ambient = light.ambient * tex;

    vec3 diffuse = light.diffuse * diff * newDiffuse;
    if(tex != vec3(0.f))
            diffuse =  light.diffuse * diff *  tex;
    
    vec3 specular = light.specular * spec * newSpecular;
    if(specular_map){
        if(tex != vec3(0.f)){
            float tex_depth = (tex.x + tex.y + tex.z)/3;
            if (tex_depth < 0.3f){
                tex_depth = 0.05;
            }
            if (tex_depth > 0.7){
                tex_depth = 0.95;
            }
            vec3 tex_specular = vec3(tex_depth*tex_depth);
            specular = light.specular * spec * tex_specular;
        }
    }

    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
 
    return (ambient + diffuse + specular);
}

vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 tex =  vec3(texture(texture_diffuse1, TexCoords));

    vec3 lightDir = normalize(light.position - fragPos);

    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);

    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.Shininess);

    // attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (constantSpot + linearSpot * distance + quadraticSpot * (distance * distance));   
    
    // spotlight intensity
    float theta = dot(lightDir, normalize(-light.direction)); 
    float epsilon = cutOffSpot - outerCutOffSpot;
    float intensity = clamp((theta - outerCutOffSpot) / epsilon, 0.0, 1.0);

    // combine results
    vec3 newDiffuse = material.Diffuse;
    vec3 newSpecular = material.Specular;
    if(material.Diffuse == vec3(0.360784, 0.683922, 0.690196)){
           newDiffuse = diffuseSpot;
           newSpecular = specularSpot;
    }
    vec3 ambient = ambientSpot * newDiffuse;
     if(tex != vec3(0.f))
            ambient = ambientSpot * tex;

    vec3 diffuse = diffuseSpot * diff * newDiffuse;
     if(tex != vec3(0.f))
            diffuse =  diffuseSpot * diff *  tex;

    //vec3 specular = light.specular * spec * material.Specular;
    vec3 specular = specularSpot * spec * newSpecular;
    if(specular_map){
       if(tex != vec3(0.f)){
            float tex_depth = (tex.x + tex.y + tex.z)/3;
            if (tex_depth < 0.3f){
                tex_depth = 0.05;
            }
            if (tex_depth > 0.7){
                tex_depth = 0.95;
            }
            vec3 tex_specular = vec3(tex_depth*tex_depth);
            specular = specularSpot * spec * tex_specular;
        }
    }


    ambient *= attenuation * intensity;
    diffuse *= attenuation * intensity;
    specular *= attenuation * intensity;
 
    return (ambient + diffuse + specular);
}

vec3 CalcSpotLightCharacter(SpotLightCharacter light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 tex =  vec3(texture(texture_diffuse1, TexCoords));

    vec3 lightDir = normalize(light.position - fragPos);

    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);

    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.Shininess);

    // attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));   
    
    // spotlight intensity
    float theta = dot(lightDir, normalize(-light.direction)); 
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);

    // combine results
    vec3 newDiffuse = material.Diffuse;
    vec3 newSpecular = material.Specular;
    if(material.Diffuse == vec3(0.360784, 0.683922, 0.690196)){
           newDiffuse = diffuseSpot;
           newSpecular = specularSpot;
    }
    vec3 ambient = light.ambient * newDiffuse;
     if(tex != vec3(0.f))
            ambient = light.ambient * tex;

    vec3 diffuse = light.diffuse * diff * newDiffuse;
     if(tex != vec3(0.f))
            diffuse =  light.diffuse * diff *  tex;

    vec3 specular = light.specular * spec * newSpecular;
    if(specular_map){
       if(tex != vec3(0.f)){
            float tex_depth = (tex.x + tex.y + tex.z)/3;
            if (tex_depth < 0.3f){
                tex_depth = 0.05;
            }
            if (tex_depth > 0.7){
                tex_depth = 0.95;
            }
            vec3 tex_specular = vec3(tex_depth*tex_depth);
            specular = light.specular * spec * tex_specular;
        }
    }

    ambient *= attenuation * intensity;
    diffuse *= attenuation * intensity;
    specular *= attenuation * intensity;
 
    return (ambient + diffuse + specular);
}


