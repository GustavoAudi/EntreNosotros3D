#pragma once
#ifndef MODEL_H
#define MODEL_H

#include <glad/glad.h> 

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "stb_image.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/gtc/matrix_transform.hpp>
#include "glm\gtc\quaternion.hpp"

#include "mesh.h"
#include "../Shaders/shader.h"
#include "camera.h"
#include "../octree_code/octree.h"


#include <algorithm>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <map>
#include <vector>

using std::string;
using std::shared_ptr;
using std::make_shared;
using std::vector;
using std::pair;
using namespace std;

unsigned int TextureFromFile(const char* path, const string& directory, bool gamma = false);
class Model
{
public:
    static const uint MAX_BONES = 100;
    // model data 
    vector<Texture> textures_loaded;	// stores all the textures loaded so far, optimization to make sure textures aren't loaded more than once.
    vector<Mesh>    meshes;
    string directory;
    bool gammaCorrection;
    //vector<Vertex> vertexCollision;
    vector<Triangle> triangles;
    Octree<vector<int>> tree = Octree<vector<int>>(256); /* Create 4096x4096x4096 octree containing double vectors. */

/*
    Model::~Model()
    {
        importer.FreeScene();
    }*/

    void initBonesForShader(Shader shader)
    {
        for (uint i = 0; i < MAX_BONES; i++) // get location all matrices of bones
        {
            string name = "bones[" + to_string(i) + "]";// name like in shader
            m_bone_location[i] = glGetUniformLocation(shader.ID, name.c_str());
        }

        // rotate head AND AXIS(y_z) about x !!!!!  Not be gimbal lock
        //rotate_head_xz *= glm::quat(cos(glm::radians(-45.0f / 2)), sin(glm::radians(-45.0f / 2)) * glm::vec3(1.0f, 0.0f, 0.0f));
    }
    // constructor, expects a filepath to a 3D model.
    Model(string const& path, bool gamma = false) : gammaCorrection(gamma)
    {
        loadModel(path);
    }

    // draws the model, and thus all its meshes
    void Draw(Shader& shader, bool anim, unsigned int buffer, unsigned int buffer2)
    {
        if (scene->HasAnimations() && anim) {
            vector<aiMatrix4x4> transforms;
            boneTransform((double)SDL_GetTicks() / 1000.0f, transforms);
            for (uint i = 0; i < transforms.size(); i++) // move all matrices for actual model position to shader
            {
                glUniformMatrix4fv(m_bone_location[i], 1, GL_TRUE, (const GLfloat*)&transforms[i]);
            }
        }
        for (unsigned int i = 0; i < meshes.size(); i++) {
            meshes[i].Draw(shader, buffer, buffer2);
        }
    }
    
    bool MeshCollision2(glm::vec3 campos) {
        //bool collided = false;
        int x = round(campos.x);
        int y = round(campos.y);
        int z = round(campos.z);
        vector<int> found = tree.at(abs((x +50)), abs((y +50)), abs((z +50)));
        //vector<int> found = tree.at(1, 1, 1);
        for (int i = 0; i < found.size(); i++) {
            Triangle t = triangles[found[i]];
            glm::vec3 A = meshes[t.mesh_i].vertices[t.A].Position;
            glm::vec3 B = meshes[t.mesh_i].vertices[t.B].Position;
            glm::vec3 C = meshes[t.mesh_i].vertices[t.C].Position;
            if (!is_separated(A, B, C, campos, 0.2)) {
                cout << "Estoy dentro de la malla" << i << endl;
                return true;
            }
        }
        return false;
    }
    
    //esta funci�n est� bien
    //�nico potencial error son los �ndices de almacenamiento
    void GenerateOctree() {
        //Generate triangles
        for (int i = 0; i < meshes.size(); i++) {
            for (int j = 0; j < meshes[i].indices.size(); j = j+3) {
                Triangle t;
                t.mesh_i = i;
                t.A = meshes[i].indices[j];
                t.B = meshes[i].indices[j+1];
                t.C = meshes[i].indices[j+2];
                triangles.push_back(t);
            }
        }
        //generate octree
        //tree = Octree<vector<unsigned int>>(4096);
        for (int i = 0; i < triangles.size(); i++) {
            Triangle t = triangles[i];
            glm::vec3 p_A = meshes[t.mesh_i].vertices[t.A].Position;
            int vax = round(p_A.x);
            int vay = round(p_A.y);
            int vaz = round(p_A.z);
            glm::vec3 p_B = meshes[t.mesh_i].vertices[t.B].Position;
            int vbx = round(p_B.x);
            int vby = round(p_B.y);
            int vbz = round(p_B.z);
            glm::vec3 p_C = meshes[t.mesh_i].vertices[t.C].Position;
            int vcx = round(p_C.x);
            int vcy = round(p_C.y);
            int vcz = round(p_C.z);

            glm::ivec3 aMin;
            aMin.x = min(vax, min(vbx, vcx));
            aMin.y = min(vay, min(vby, vcy));
            aMin.z = min(vaz, min(vbz, vcz));

            glm::ivec3 aMax;
            aMax.x = max(vax, max(vbx, vcx));
            aMax.y = max(vay, max(vby, vcy));
            aMax.z = max(vaz, max(vbz, vcz));

            for (int x = 0; x <= aMax.x - aMin.x; x++) {
                for (int y = 0; y <= aMax.y - aMin.y; y++) {
                    for (int z = 0; z <= aMax.z - aMin.z; z++) {
                        vector<int> ret = tree.at((aMin.x + x + 50), (aMin.y + y + 50), (aMin.z + z + 50));
                        ret.push_back(i);
                        tree.set((aMin.x + x + 50), (aMin.y + y + 50), (aMin.z + z + 50), ret);

                    }
                }
            }
            //las posiciones podr�an escalarse para tener mas granularidad en el espacio,
            //pero creo que a nivel de enteros estamos bien
            // model limits X: -4.38486 Y: 16.575 Z: 13.1727
            //              X: 59.6767 Y: -7.535 Z: -46.1955
            //los inserts pueden evitarse si i ya pertenece al vector
            /*
            vector<int> ret = tree.at((vax +50)/2, (vay+50)/2, (vaz+50)/2);
            ret.push_back(i);
            tree.set((vax + 50)/2, (vay + 50)/2, (vaz + 50)/2, ret);

            vector<int> ret2 = tree.at((vbx + 50)/2, (vby + 50)/2, (vbz + 50)/2);
            ret2.push_back(i);
            tree.set((vbx + 50)/2, (vby + 50)/2, (vbz + 50)/2, ret2);

            vector<int> ret3 = tree.at((vcx + 50)/2, (vcy + 50)/2, (vcz + 50)/2);
            ret3.push_back(i);
            tree.set((vcx + 50)/2, (vcy + 50)/2, (vcz + 50)/2, ret3);*/

            //alternativa
        }


    }
    
    //gamedev.net/forums/topic/701749-swept-sphere-triangle-intersection/
    bool is_separated(glm::vec3 A, glm::vec3 B, glm::vec3 C, glm::vec3 P, double r)
    {
        A = A - P;
        B = B - P;
        C = C - P;

        double rr = r * r;
        glm::vec3 V = cross((B - A), (C - A));
        double d = dot(A, V);
        double e = dot(V, V);
        int sep1 = d * d > rr * e;

        if (sep1)
            return true;

        double aa = dot(A, A);
        double ab = dot(A, B);
        double ac = dot(A, C);
        int sep2 = (aa > rr) & (ab > aa) & (ac > aa);

        if (sep2)
            return true;

        double bb = dot(B, B);
        double bc = dot(B, C);
        int sep3 = (bb > rr) & (ab > bb) & (bc > bb);

        if (sep3)
            return true;

        double cc = dot(C, C);
        int sep4 = (cc > rr) & (ac > cc) & (bc > cc);

        if (sep4)
            return true;

        glm::vec3 AB = B - A;
        glm::vec3 BC = C - B;
        glm::vec3 CA = A - C;

        float d1 = ab - aa;
        float e1 = dot(AB, AB);

        glm::vec3 Q1 = A * e1 - AB * d1;
        glm::vec3 QC = C * e1 - Q1;
        int sep5 = (dot(Q1, Q1) > rr * e1 * e1) & (dot(Q1, QC) > 0);

        if (sep5)
            return true;

        float d2 = bc - bb;
        float e2 = dot(BC, BC);

        glm::vec3 Q2 = B * e2 - BC * d2;
        glm::vec3 QA = A * e2 - Q2;
        int sep6 = (dot(Q2, Q2) > rr * e2 * e2) & (dot(Q2, QA) > 0);

        if (sep6)
            return true;

        float d3 = ac - cc;
        float e3 = dot(CA, CA);

        glm::vec3 Q3 = C * e3 - CA * d3;
        glm::vec3 QB = B * e3 - Q3;
        int sep7 = (dot(Q3, Q3) > rr * e3 * e3) & (dot(Q3, QB) > 0);

        if (sep7)
            return true;

        return false;
    }
   

    void update()
    {
        // making new quaternions for rotate head
       // if (InputHandler::Instance()->isKeyDown(SDL_SCANCODE_1))
      //  {
        rotate_head_xz *= glm::quat(cos(glm::radians(1.0f / 2)), sin(glm::radians(1.0f / 2)) * glm::vec3(1.0f, 0.0f, 0.0f));
        //   }

       //    if (InputHandler::Instance()->isKeyDown(SDL_SCANCODE_2))
        //   {
        rotate_head_xz *= glm::quat(cos(glm::radians(-1.0f / 2)), sin(glm::radians(-1.0f / 2)) * glm::vec3(1.0f, 0.0f, 0.0f));
        //  }

      /*    if (InputHandler::Instance()->isKeyDown(SDL_SCANCODE_3))
          {
              rotate_head_xz *= glm::quat(cos(glm::radians(1.0f / 2)), sin(glm::radians(1.0f / 2)) * glm::vec3(0.0f, 0.0f, 1.0f));
          }

          if (InputHandler::Instance()->isKeyDown(SDL_SCANCODE_4))
          {
              rotate_head_xz *= glm::quat(cos(glm::radians(-1.0f / 2)), sin(glm::radians(-1.0f / 2)) * glm::vec3(0.0f, 0.0f, 1.0f));
          }*/

    }

private:
    const aiScene* scene;
    Assimp::Importer importer;
    map<string, uint> m_bone_mapping; // maps a bone name and their index
    uint m_num_bones = 0;
    vector<BoneMatrix> m_bone_matrices;
    aiMatrix4x4 m_global_inverse_transform;

    GLuint m_bone_location[MAX_BONES];
    float ticks_per_second = 0.0f;
    // loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
    void loadModel(string const& path)
    {
        // read file via ASSIMP
        this->scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace | aiProcess_FlipUVs);
        // check for errors
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
        {
            cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
            return;
        }
        // retrieve the directory path of the filepath
        directory = path.substr(0, path.find_last_of('/'));
        if (scene->HasAnimations()) {

            m_global_inverse_transform = scene->mRootNode->mTransformation;
            m_global_inverse_transform.Inverse();

            if (scene->mAnimations[0]->mTicksPerSecond != 0.0)
            {
                ticks_per_second = scene->mAnimations[0]->mTicksPerSecond;
            }
            else
            {
                ticks_per_second = 25.0f;
            }
            ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// SACAR

            cout << "dir  " << directory << endl;
            cout << "scene->HasAnimations() 1: " << scene->HasAnimations() << endl;
            cout << "scene->mNumMeshes 1: " << scene->mNumMeshes << endl;
            cout << "scene->mAnimations[0]->mNumChannels 1: " << scene->mAnimations[0]->mNumChannels << endl;
            cout << "scene->mAnimations[0]->mDuration 1: " << scene->mAnimations[0]->mDuration << endl;
            cout << "scene->mAnimations[0]->mTicksPerSecond 1: " << scene->mAnimations[0]->mTicksPerSecond << endl << endl;
            cout << "		name nodes animation : " << endl;
           /* for (uint i = 0; i < scene->mAnimations[0]->mNumChannels; i++)
            {
                cout << scene->mAnimations[0]->mChannels[i]->mNodeName.C_Str() << endl;
            }*/
        }
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// SACAR
        // process ASSIMP's root node recursively
        processNode(scene->mRootNode, scene);
    }

    // processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
    void processNode(aiNode* node, const aiScene* scene)
    {
        // process each mesh located at the current node
        for (unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            // the node object only contains indices to index the actual objects in the scene. 
            // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            Mesh meshAux = processMesh(mesh, scene);
            if (meshAux.materials[0].Transparent != 1.0f)
                meshes.push_back(meshAux);
            else
                meshes.insert(meshes.begin(), meshAux);
        }
        // after we've processed all of the meshes (if any) we then recursively process each of the children nodes
        for (unsigned int i = 0; i < node->mNumChildren; i++)
        {
            processNode(node->mChildren[i], scene);
        }
    }

    Material loadMaterial(aiMaterial* mat) {
        Material material;
        aiColor3D color(0.f, 0.f, 0.f);
        float shininess;
        float transparent;

        mat->Get(AI_MATKEY_COLOR_DIFFUSE, color);
        material.Diffuse = glm::vec3(color.r, color.g, color.b);

        mat->Get(AI_MATKEY_COLOR_AMBIENT, color);
        material.Ambient = glm::vec3(color.r, color.g, color.b);

        mat->Get(AI_MATKEY_COLOR_SPECULAR, color);
        material.Specular = glm::vec3(color.r, color.g, color.b);

        mat->Get(AI_MATKEY_COLOR_EMISSIVE, color);
        material.Emissive = glm::vec3(color.r, color.g, color.b);

        mat->Get(AI_MATKEY_OPACITY, transparent);
        material.Transparent = transparent;

        mat->Get(AI_MATKEY_SHININESS, shininess);
        material.Shininess = shininess;

        return material;
    }


  

    Mesh processMesh(aiMesh* mesh, const aiScene* scene)
    {
        // data to fill
        vector<Vertex> vertices;
        vector<unsigned int> indices;
        vector<Texture> textures;
        vector<Material> materials;

        vector<VertexBoneData> bones_id_weights_for_each_vertex;
        bones_id_weights_for_each_vertex.resize(mesh->mNumVertices);


        // walk through each of the mesh's vertices
        for (GLuint i = 0; i < mesh->mNumVertices; i++)
        {
            Vertex vertex;
            glm::vec3 vector; // we declare a placeholder vector since assimp uses its own vector class that doesn't directly convert to glm's vec3 class so we transfer the data to this placeholder glm::vec3 first.
            // positions
            vector.x = mesh->mVertices[i].x;
            vector.y = mesh->mVertices[i].y;
            vector.z = mesh->mVertices[i].z;
            vertex.Position = vector;

            // normals
            if (mesh->HasNormals())
            {
                vector.x = mesh->mNormals[i].x;
                vector.y = mesh->mNormals[i].y;
                vector.z = mesh->mNormals[i].z;
                vertex.Normal = vector;
            }
            else
            {
                vertex.Normal = glm::vec3();
            }

            // texture coordinates
            if (mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
            {
                glm::vec2 vec;
                // a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't 
                // use models where a vertex can have multiple texture coordinates so we always take the first set (0).
                vec.x = mesh->mTextureCoords[0][i].x;
                vec.y = mesh->mTextureCoords[0][i].y;
                vertex.TexCoords = vec;
                // tangent
               /* vector.x = mesh->mTangents[i].x;
                vector.y = mesh->mTangents[i].y;
                vector.z = mesh->mTangents[i].z;
                vertex.Tangent = vector;
                // bitangent
                vector.x = mesh->mBitangents[i].x;
                vector.y = mesh->mBitangents[i].y;
                vector.z = mesh->mBitangents[i].z;
                vertex.Bitangent = vector;*/
            }
            else
            {

                vertex.TexCoords = glm::vec2(0.0f); // vertex.TexCoords = glm::vec2(-1.0f, -1.0f);
            }
            //AABB update
            /*
            if (vertex.Position.x < aabb.aMin.x) {
                aabb.aMin.x = vertex.Position.x;
            }
            if (vertex.Position.y < aabb.aMin.y) {
                aabb.aMin.y = vertex.Position.y;
            }
            if (vertex.Position.z < aabb.aMin.z) {
                aabb.aMin.z = vertex.Position.z;
            }
            if (vertex.Position.x > aabb.aMax.x) {
                aabb.aMax.x = vertex.Position.x;
            }
            if (vertex.Position.y > aabb.aMax.y) {
                aabb.aMax.y = vertex.Position.y;
            }
            if (vertex.Position.z > aabb.aMax.z) {
                aabb.aMax.z = vertex.Position.z;
            }*/

            vertices.push_back(vertex);
        }
        // record mesh's axis-aligned bounding box
        //bounding_meshes.push_back(aabb);

        // now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
        for (unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            // retrieve all indices of the face and store them in the indices vector
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }

        if (mesh->mMaterialIndex >= 0) {
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
            //mesh material
            Material mat = loadMaterial(material);
            materials.push_back(mat);
            // we assume a convention for sampler names in the shaders. Each diffuse texture should be named
            // as 'texture_diffuseN' where N is a sequential number ranging from 1 to MAX_SAMPLER_NUMBER. 
            // Same applies to other texture as the following list summarizes:
            // diffuse: texture_diffuseN
            // specular: texture_specularN
            // normal: texture_normalN

            // 1. diffuse maps
            vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
            textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
            // 2. specular maps
            vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
            textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
            // 3. normal maps
            std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_NORMALS, "texture_normal");
            textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
            // 4. height maps
            std::vector<Texture> ambientMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_ambient");
            textures.insert(textures.end(), ambientMaps.begin(), ambientMaps.end());

            std::vector<Texture> heightMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_height");
            textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());
        }
        // load bones
        for (uint i = 0; i < mesh->mNumBones; i++)
        {
            uint bone_index = 0;
            string bone_name(mesh->mBones[i]->mName.data);

            cout << mesh->mBones[i]->mName.data << endl;

            if (m_bone_mapping.find(bone_name) == m_bone_mapping.end()) // ��������� ��� �� � ������� ��������
            {
                // Allocate an index for a new bone
                bone_index = m_num_bones;
                m_num_bones++;
                BoneMatrix bi;
                m_bone_matrices.push_back(bi);
                m_bone_matrices[bone_index].offset_matrix = mesh->mBones[i]->mOffsetMatrix;
                m_bone_mapping[bone_name] = bone_index;

                cout << "bone_name: " << bone_name << "			 bone_index: " << bone_index << endl;
            }
            else
            {
                bone_index = m_bone_mapping[bone_name];
            }

            for (uint j = 0; j < mesh->mBones[i]->mNumWeights; j++)
            {
                uint vertex_id = mesh->mBones[i]->mWeights[j].mVertexId; // �� ������� �� ������ ����� �������� �����
                float weight = mesh->mBones[i]->mWeights[j].mWeight;
                bones_id_weights_for_each_vertex[vertex_id].addBoneData(bone_index, weight); // � ������ ������� ����� ����� � �� ���

                // ������ ������� vertex_id �� ������ ����� � �������� bone_index  ����� ��� weight
                //cout << " vertex_id: " << vertex_id << "	bone_index: " << bone_index << "		weight: " << weight << endl;
            }
        }
        // return a mesh object created from the extracted mesh data

        return Mesh(vertices, indices, textures, materials, bones_id_weights_for_each_vertex);

    }

    // checks all material textures of a given type and loads the textures if they're not loaded yet.
    // the required info is returned as a Texture struct.
    vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, string typeName)
    {
        vector<Texture> textures;
        for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
        {
            aiString str;
            mat->GetTexture(type, i, &str);
            // check if texture was loaded before and if so, continue to next iteration: skip loading a new texture
            bool skip = false;
            for (unsigned int j = 0; j < textures_loaded.size(); j++)
            {
                if (std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0)
                {
                    textures.push_back(textures_loaded[j]);
                    //skip = true; // a texture with the same filepath has already been loaded, continue to next one. (optimization)
                    break;
                }
            }
            if (!skip)
            {   // if texture hasn't been loaded already, load it
                Texture texture;
                texture.id = TextureFromFile(str.C_Str(), this->directory);
                texture.type = typeName;
                texture.path = str.C_Str();
                textures.push_back(texture);
                textures_loaded.push_back(texture);  // store it as texture loaded for entire model, to ensure we won't unnecesery load duplicate textures.
            }
        }
        return textures;
    }

    uint findPosition(float p_animation_time, const aiNodeAnim* p_node_anim)
    {
        // ����� ���� ������� ����� ����� ����� ������� ���������� ����� ������ ��������
        for (uint i = 0; i < p_node_anim->mNumPositionKeys - 1; i++) // �������� ����� ��������
        {
            if (p_animation_time < (float)p_node_anim->mPositionKeys[i + 1].mTime) // �������� �� �������� ��������� !!!
            {
                return i; // �� ������� ������ �������� !!!!!!!!!!!!!!!!!! ����������������������������
            }
        }

        assert(0);
        return 0;
    }

    uint findRotation(float p_animation_time, const aiNodeAnim* p_node_anim)
    {
        // ����� ���� ������� ����� ����� ����� ������� ���������� ����� ������ ��������
        for (uint i = 0; i < p_node_anim->mNumRotationKeys - 1; i++) // �������� ����� ��������
        {
            if (p_animation_time < (float)p_node_anim->mRotationKeys[i + 1].mTime) // �������� �� �������� ��������� !!!
            {
                return i; // �� ������� ������ �������� !!!!!!!!!!!!!!!!!! ����������������������������
            }
        }

        assert(0);
        return 0;
    }

    uint findScaling(float p_animation_time, const aiNodeAnim* p_node_anim)
    {
        // ����� ���� ������� ����� ����� ����� ������� ���������� ����� ������ ��������
        for (uint i = 0; i < p_node_anim->mNumScalingKeys - 1; i++) // �������� ����� ��������
        {
            if (p_animation_time < (float)p_node_anim->mScalingKeys[i + 1].mTime) // �������� �� �������� ��������� !!!
            {
                return i; // �� ������� ������ �������� !!!!!!!!!!!!!!!!!! ����������������������������
            }
        }

        assert(0);
        return 0;
    }

    aiVector3D calcInterpolatedPosition(float p_animation_time, const aiNodeAnim* p_node_anim)
    {
        if (p_node_anim->mNumPositionKeys == 1) // Keys ��� ������� �����
        {
            return p_node_anim->mPositionKeys[0].mValue;
        }

        uint position_index = findPosition(p_animation_time, p_node_anim); // ������ ������ �������� ����� ������� ������
        uint next_position_index = position_index + 1; // ������ ��������� �������� �����
        assert(next_position_index < p_node_anim->mNumPositionKeys);
        // ���� ����� �������
        float delta_time = (float)(p_node_anim->mPositionKeys[next_position_index].mTime - p_node_anim->mPositionKeys[position_index].mTime);
        float factor = (p_animation_time - (float)p_node_anim->mPositionKeys[position_index].mTime) / delta_time;
        // ������ = (���� ������� ������ �� ������ �������� ��������� �����) / �� ���� ����� �������
        if (factor >= 0.0f && factor <= 1.0f) {return  p_node_anim->mPositionKeys[position_index].mValue;}
        aiVector3D start = p_node_anim->mPositionKeys[position_index].mValue;
        aiVector3D end = p_node_anim->mPositionKeys[next_position_index].mValue;
        aiVector3D delta = end - start;

        return start + factor * delta;
    }

    aiQuaternion calcInterpolatedRotation(float p_animation_time, const aiNodeAnim* p_node_anim)
    {
        if (p_node_anim->mNumRotationKeys == 1) // Keys ��� ������� �����
        {
            return p_node_anim->mRotationKeys[0].mValue;
        }

        uint rotation_index = findRotation(p_animation_time, p_node_anim); // ������ ������ �������� ����� ������� ������
        uint next_rotation_index = rotation_index + 1; // ������ ��������� �������� �����
        assert(next_rotation_index < p_node_anim->mNumRotationKeys);
        // ���� ����� �������
        float delta_time = (float)(p_node_anim->mRotationKeys[next_rotation_index].mTime - p_node_anim->mRotationKeys[rotation_index].mTime);
        // ������ = (���� ������� ������ �� ������ �������� ��������� �����) / �� ���� ����� �������
        float factor = (p_animation_time - (float)p_node_anim->mRotationKeys[rotation_index].mTime) / delta_time;

/*        cout << "p_node_anim->mRotationKeys[rotation_index].mTime: " << p_node_anim->mRotationKeys[rotation_index].mTime << endl;
        cout << "p_node_anim->mRotationKeys[next_rotaion_index].mTime: " << p_node_anim->mRotationKeys[next_rotation_index].mTime << endl;
        cout << "delta_time: " << delta_time << endl;
        cout << "animation_time: " << p_animation_time << endl;
        cout << "animation_time - mRotationKeys[rotation_index].mTime: " << (p_animation_time - (float)p_node_anim->mRotationKeys[rotation_index].mTime) << endl;
        cout << "factor: " << factor << endl << endl << endl;*/

        if (factor >= 0.0f && factor <= 1.0f) { p_node_anim->mRotationKeys[rotation_index].mValue; }
        aiQuaternion start_quat = p_node_anim->mRotationKeys[rotation_index].mValue;
        aiQuaternion end_quat = p_node_anim->mRotationKeys[next_rotation_index].mValue;

        return nlerp(start_quat, end_quat, factor);
    }

    aiVector3D calcInterpolatedScaling(float p_animation_time, const aiNodeAnim* p_node_anim)
    {
        if (p_node_anim->mNumScalingKeys == 1) // Keys ��� ������� �����
        {
            return p_node_anim->mScalingKeys[0].mValue;
        }

        uint scaling_index = findScaling(p_animation_time, p_node_anim); // ������ ������ �������� ����� ������� ������
        uint next_scaling_index = scaling_index + 1; // ������ ��������� �������� �����
        assert(next_scaling_index < p_node_anim->mNumScalingKeys);
        // ���� ����� �������
        float delta_time = (float)(p_node_anim->mScalingKeys[next_scaling_index].mTime - p_node_anim->mScalingKeys[scaling_index].mTime);
        // ������ = (���� ������� ������ �� ������ �������� ��������� �����) / �� ���� ����� �������
        float  factor = (p_animation_time- (float)p_node_anim->mScalingKeys[scaling_index].mTime) / delta_time;
/*
        cout << "p_node_anim->mRotationKeys[scaling_index].mTime: " << p_node_anim->mRotationKeys[scaling_index].mTime << endl;
        cout << "p_node_anim->mRotationKeys[next_scaling_index].mTime: " << p_node_anim->mRotationKeys[next_scaling_index].mTime << endl;
        cout << "delta_time: " << delta_time << endl;
        cout << "animation_time: " << p_animation_time << endl;
        cout << "animation_time - mRotationKeys[scaling_index].mTime: " << (p_animation_time - (float)p_node_anim->mRotationKeys[scaling_index].mTime) << endl;
        cout << "factor: " << factor << endl << endl << endl;
*/

        if (factor >= 0.0f && factor <= 1.0f) { return p_node_anim->mScalingKeys[scaling_index].mValue; }
        aiVector3D start = p_node_anim->mScalingKeys[scaling_index].mValue;
        aiVector3D end = p_node_anim->mScalingKeys[next_scaling_index].mValue;
        aiVector3D delta = end - start;

        return start + factor * delta;
    }

    const aiNodeAnim* findNodeAnim(const aiAnimation* p_animation, const string p_node_name)
    {
        // channel in animation contains aiNodeAnim (aiNodeAnim its transformation for bones)
        // numChannels == numBones
        for (uint i = 0; i < p_animation->mNumChannels; i++)
        {
            const aiNodeAnim* node_anim = p_animation->mChannels[i]; // ��������� ������� ������ node
            if (string(node_anim->mNodeName.data) == p_node_name)
            {
                return node_anim;// ���� ����� �������� �� ������� ����� (� ������� ����������� node) ������������ ���� node_anim
            }
        }

        return nullptr;
    }

    // rotate Head
    glm::quat rotate_head_xz = glm::quat(cos(glm::radians(0.0f)), sin(glm::radians(0.0f)) * glm::vec3(1.0f, 0.0f, 0.0f)); // this quad do nothingggggg!!!!!
    // start from RootNode
    void readNodeHierarchy(float p_animation_time, const aiNode* p_node, const aiMatrix4x4 parent_transform)
    {

        string node_name(p_node->mName.data);

        //������� node, �� ������� ������������ �������, ������������� �������� ���� ������(aiNodeAnim).
        const aiAnimation* animation = scene->mAnimations[0];
        aiMatrix4x4 node_transform = p_node->mTransformation;
        const aiNodeAnim* node_anim = findNodeAnim(animation, node_name); // ����� ������� �� ����� ����
        if (node_anim)
        {

            //scaling
            //aiVector3D scaling_vector = node_anim->mScalingKeys[2].mValue;
            aiVector3D scaling_vector = calcInterpolatedScaling(p_animation_time, node_anim);
            aiMatrix4x4 scaling_matr;
            aiMatrix4x4::Scaling(scaling_vector, scaling_matr);

            //rotation
            //aiQuaternion rotate_quat = node_anim->mRotationKeys[0].mValue;
            aiQuaternion rotate_quat = calcInterpolatedRotation(p_animation_time, node_anim);
            aiMatrix4x4 rotate_matr = aiMatrix4x4(rotate_quat.GetMatrix());

            //translation
            //aiVector3D translate_vector = node_anim->mPositionKeys[2].mValue;
            aiVector3D translate_vector = calcInterpolatedPosition(p_animation_time, node_anim);
            aiMatrix4x4 translate_matr;
            aiMatrix4x4::Translation(translate_vector, translate_matr);

            /*if (string(node_anim->mNodeName.data) == string("Head"))
            {
                aiQuaternion rotate_head = aiQuaternion(rotate_head_xz.w, rotate_head_xz.x, rotate_head_xz.y, rotate_head_xz.z);

                node_transform = translate_matr * (rotate_matr * aiMatrix4x4(rotate_head.GetMatrix())) * scaling_matr;
            }
            else
            {
            */
                node_transform = translate_matr * rotate_matr * scaling_matr;
            //}

        }

        aiMatrix4x4 global_transform = parent_transform * node_transform;

        // ���� � node �� �������� ����������� bone, �� �� node ������ ��������� � ������ bone !!!
        if (m_bone_mapping.find(node_name) != m_bone_mapping.end()) // true if node_name exist in bone_mapping
        {
            uint bone_index = m_bone_mapping[node_name];
            m_bone_matrices[bone_index].final_world_transform = m_global_inverse_transform * global_transform * m_bone_matrices[bone_index].offset_matrix;
        }

        for (uint i = 0; i < p_node->mNumChildren; i++)
        {
            readNodeHierarchy(p_animation_time, p_node->mChildren[i], global_transform);
        }

    }

    void boneTransform(double time_in_sec, vector<aiMatrix4x4>& transforms)
    {
        aiMatrix4x4 identity_matrix; // = mat4(1.0f);
        double time_in_ticks = time_in_sec * ticks_per_second;
            float animation_time =  fmod(time_in_ticks, scene->mAnimations[0]->mDuration); //������� �� ����� (������� �� ������)
        // animation_time - ���� ������� ������ � ���� ������ �� ������ �������� (�� ������� �������� ����� � �������� )
        readNodeHierarchy(animation_time, scene->mRootNode, identity_matrix);

        transforms.resize(m_num_bones);

        for (uint i = 0; i < m_num_bones; i++)
        {
            transforms[i] = m_bone_matrices[i].final_world_transform;
        }
    }

    glm::mat4 aiToGlm(aiMatrix4x4 ai_matr)
    {
        glm::mat4 result;
        result[0].x = ai_matr.a1; result[0].y = ai_matr.b1; result[0].z = ai_matr.c1; result[0].w = ai_matr.d1;
        result[1].x = ai_matr.a2; result[1].y = ai_matr.b2; result[1].z = ai_matr.c2; result[1].w = ai_matr.d2;
        result[2].x = ai_matr.a3; result[2].y = ai_matr.b3; result[2].z = ai_matr.c3; result[2].w = ai_matr.d3;
        result[3].x = ai_matr.a4; result[3].y = ai_matr.b4; result[3].z = ai_matr.c4; result[3].w = ai_matr.d4;
        return result;
    }

    aiQuaternion nlerp(aiQuaternion a, aiQuaternion b, float blend)
    {
        //cout << a.w + a.x + a.y + a.z << endl;
        a.Normalize();
        b.Normalize();

        aiQuaternion result;
        float dot_product = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
        float one_minus_blend = 1.0f - blend;

        if (dot_product < 0.0f)
        {
            result.x = a.x * one_minus_blend + blend * -b.x;
            result.y = a.y * one_minus_blend + blend * -b.y;
            result.z = a.z * one_minus_blend + blend * -b.z;
            result.w = a.w * one_minus_blend + blend * -b.w;
        }
        else
        {
            result.x = a.x * one_minus_blend + blend * b.x;
            result.y = a.y * one_minus_blend + blend * b.y;
            result.z = a.z * one_minus_blend + blend * b.z;
            result.w = a.w * one_minus_blend + blend * b.w;
        }

        return result.Normalize();
    }
    
 
};


unsigned int TextureFromFile(const char* path, const string& directory, bool gamma)
{
    string filename = string(path);
    filename = directory + '/' + filename;

    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nrComponents, STBI_rgb_alpha); // STBI_rgb_alpha para que no crash
    if (data)
    {
        GLenum format = GL_RGB;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}


#endif

