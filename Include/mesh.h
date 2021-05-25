#ifndef MESH_H
#define MESH_H
#pragma warning(disable : 5208)

#include <glad/glad.h> // holds all OpenGL type declarations

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../Shaders/shader.h"
#include <iostream>
#include <string>
#include <vector>
typedef unsigned int uint;
using namespace std;
#define NUM_BONES_PER_VEREX 4

struct Vertex {
    // position
    glm::vec3 Position;
    // normal
    glm::vec3 Normal;
    // texCoords
    glm::vec2 TexCoords;
    // tangent
    glm::vec3 Tangent;
    // bitangent
    glm::vec3 Bitangent;

};

typedef struct{
    int mesh_i =-1;
    int A = -1;
    int B = -1;
    int C = -1;
} Triangle;

struct Texture {
    unsigned int id;
    string type;
    string path;
};

struct Material {
    glm::vec3 Diffuse;
    glm::vec3 Specular;
    glm::vec3 Ambient;
    glm::vec3 Emissive;
    float Transparent;
    float Shininess;
};

struct BoneMatrix
{
    aiMatrix4x4 offset_matrix;
    aiMatrix4x4 final_world_transform;

};
struct VertexBoneData
{
    uint ids[NUM_BONES_PER_VEREX];   // we have 4 bone ids for EACH vertex & 4 weights for EACH vertex
    float weights[NUM_BONES_PER_VEREX];

    VertexBoneData()
    {
        memset(ids, 0, sizeof(ids));    // init all values in array = 0
        memset(weights, 0, sizeof(weights));
    }

    void addBoneData(uint bone_id, float weight)
    {
        for (uint i = 0; i < NUM_BONES_PER_VEREX; i++)
        {
            if (weights[i] == 0.0)
            {
                ids[i] = bone_id;
                weights[i] = weight;
                return;
            }
        }
    }
};

class Mesh {
public:
    // mesh Data
    vector<Vertex>       vertices;
    vector<unsigned int> indices;
    vector<Texture>      textures;
    vector <Material>    materials;
    vector<VertexBoneData> bones_id_weights_for_each_vertex;
    //unsigned int VAO;

    // constructor
    Mesh(vector<Vertex> vertices, vector<unsigned int> indices, vector<Texture> textures, vector<Material> materials, vector<VertexBoneData> bone_id_weights)
    {
        this->vertices = vertices;
        this->indices = indices;
        this->textures = textures;
        this->materials = materials;
        bones_id_weights_for_each_vertex = bone_id_weights;
        // now that we have all the required data, set the vertex buffers and its attribute pointers.
        setupMesh();
    }

    // render the mesh
    void Draw(Shader& shader, unsigned int buffer, unsigned int buffer2)
    {
        // bind appropriate textures
        unsigned int diffuseNr = 1;
        unsigned int specularNr = 1;
        unsigned int normalNr = 1;
        unsigned int heightNr = 1;
        unsigned int ambientNr = 1;
        if (buffer == 0 && buffer2 == 0) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, 0);
            for (unsigned int i = 0; i < textures.size(); i++)
            {
                glActiveTexture(GL_TEXTURE0 + i); // active proper texture unit before binding
                // retrieve texture number (the N in diffuse_textureN)
                string number;
                string name = textures[i].type;
                if (name == "texture_diffuse")
                    number = std::to_string(diffuseNr++);
                else if (name == "texture_specular")
                    number = std::to_string(specularNr++); // transfer unsigned int to stream
                else if (name == "texture_normal")
                    number = std::to_string(normalNr++); // transfer unsigned int to stream
                else if (name == "texture_ambient")
                    number = std::to_string(ambientNr++); // transfer unsigned int to stream
                else if (name == "texture_height")
                    number = std::to_string(heightNr++); // transfer unsigned int to stream
                // now set the sampler to the correct texture unit
                //glUniform1i(glGetUniformLocation(shader.ID, (name + number).c_str()), i);
                // and finally bind the texture
                glBindTexture(GL_TEXTURE_2D, textures[i].id);
            }
            //Material uniform
            glUniform3f(glGetUniformLocation(shader.ID, "material.Diffuse"), materials[0].Diffuse.x, materials[0].Diffuse.y, materials[0].Diffuse.z);
            glUniform3f(glGetUniformLocation(shader.ID, "material.Specular"), materials[0].Specular.x, materials[0].Specular.y, materials[0].Specular.z);
            glUniform3f(glGetUniformLocation(shader.ID, "material.Ambient"), materials[0].Ambient.x, materials[0].Ambient.y, materials[0].Ambient.z);
            glUniform3f(glGetUniformLocation(shader.ID, "material.Emissive"), materials[0].Emissive.x, materials[0].Emissive.y, materials[0].Emissive.z);
            glUniform1f(glGetUniformLocation(shader.ID, "material.Shininess"), materials[0].Shininess);
            glUniform1f(glGetUniformLocation(shader.ID, "material.Transparent"), materials[0].Transparent);
        }
        /*
        if (GL_FRAMEBUFFER_UNDEFINED == glCheckFramebufferStatus(GL_FRAMEBUFFER)) {
            cout << " framebuffer is the default read or draw framebuffer, but the default framebuffer does not exist." << endl;
        }
        if (GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT == glCheckFramebufferStatus(GL_FRAMEBUFFER)) {
            cout << " any of the framebuffer attachment points are framebuffer incomplete" << endl;
        }
        if (GL_FRAMEBUFFER_COMPLETE == glCheckFramebufferStatus(GL_FRAMEBUFFER)) {
            cout << "es todo ok" << endl;
        }*/

        // draw mesh
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, GLsizei(indices.size()), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

/*
        for (GLuint i = 0; i < this->textures.size(); i++)
        {
            glActiveTexture(GL_TEXTURE0 + i);
            //glBindTexture(GL_TEXTURE_2D, 0);
        }*/

        

    }

private:
    // render data 
    unsigned int VAO;
    unsigned int VBO_vertices, EBO_indices, VBO_bones;

    // initializes all the buffer objects/arrays
    void setupMesh()
    {
        //vertices data
        glGenBuffers(1, &VBO_vertices);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_vertices);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertices[0]), &vertices[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        //bones data
        glGenBuffers(1, &VBO_bones);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_bones);
        glBufferData(GL_ARRAY_BUFFER, bones_id_weights_for_each_vertex.size() * sizeof(bones_id_weights_for_each_vertex[0]), &bones_id_weights_for_each_vertex[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        //numbers for sequence indices
        glGenBuffers(1, &EBO_indices);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_indices);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), &indices[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        // create VAO and binding data from buffers to shaders
        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_vertices);
        //vertex position
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)0);
        glEnableVertexAttribArray(1); // offsetof(Vertex, normal) = returns the byte offset of that variable from the start of the struct
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, Normal));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, TexCoords));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        //bones
        glBindBuffer(GL_ARRAY_BUFFER, VBO_bones);
        glEnableVertexAttribArray(3);
        glVertexAttribIPointer(3, 4, GL_INT, sizeof(VertexBoneData), (GLvoid*)0); // for INT Ipointer
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(VertexBoneData), (GLvoid*)offsetof(VertexBoneData, weights));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        //indices
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_indices);
        glBindVertexArray(0);

    }
};
#endif