#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout(location = 3) in ivec4 bone_ids;     // INT pointer
layout(location = 4) in vec4 weights;


out vec2 TexCoords;
out vec3 Normal;
out vec3 FragPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 normals_matrix;

const int MAX_BONES = 100;
uniform mat4 bones[MAX_BONES];

uniform bool anim;
uniform bool moove;

void main()
{
TexCoords = aTexCoords; 

if(!anim){
       
    //Normal = aNormal;
    //Esto es por si se llega a escalar el modelo, para que las normales queden coherentes
    Normal = mat3(transpose(inverse(model))) * aNormal; 
    FragPos = aPos;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
else{
	mat4 MVP = projection * view * model;
	mat4 bone_transform;
	if (moove == true){
		bone_transform = bones[bone_ids[0]] * weights[0];
		bone_transform += bones[bone_ids[1]] * weights[1];
		bone_transform += bones[bone_ids[2]] * weights[2];
		bone_transform += bones[bone_ids[3]] * weights[3];
	}
	else  {bone_transform = mat4(1.0f);}

	vec4 boned_position = bone_transform * vec4(aPos, 1.0); // transformed by bones

	Normal = normalize(vec3((normals_matrix * bone_transform * vec4(aNormal, 1.0))));
	FragPos = vec3(model * boned_position);
	gl_Position = MVP * boned_position;


}
}