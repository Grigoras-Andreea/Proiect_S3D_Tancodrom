#pragma once
#include <vector>
#include <iostream>
#include "Texture.h"
#include "Mesh.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

unsigned int TextureFromFile(const char* path, const string& directory, bool gamma = false);

class Model
{
public:
    // model data 
    vector<Texture> textures_loaded;	// stores all the textures loaded so far, optimization to make sure textures aren't loaded more than once.
    vector<Mesh>    meshes;
    string directory;
    bool gammaCorrection;

    // constructor, expects a filepath to a 3D model.
    Model(string const& path, bool bSmoothNormals, bool gamma = false);

    // draws the model, and thus all its meshes
    void Draw(Shader& shader);

    glm::vec3 GetPosition() const;
    void SetPosition(const glm::vec3& newPosition);

    glm::mat4 GetModelMatrix() const {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, position);
        model = glm::rotate(model, glm::radians(rotationAngle), rotationAxis); // Rotates the model around the specified axis

        // alte transformãri dacã sunt necesare
        return model;
    }

    float GetRotationAngle() { return rotationAngle; };
    glm::vec3 GetRotationAxis() { return rotationAxis; };
    void Rotate(float angle, const glm::vec3& axis);
    void SetRotationAxis(const glm::vec3& axis) {
        rotationAxis = axis;
    }
    void SetRotationAngle(float angle) {
        rotationAngle = angle;
    }


private:
    // loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
    void loadModel(string const& path, bool bSmoothNormals);

    // processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
    void processNode(aiNode* node, const aiScene* scene);

    Mesh processMesh(aiMesh* mesh, const aiScene* scene);

    // checks all material textures of a given type and loads the textures if they're not loaded yet.
    // the required info is returned as a Texture struct.
    vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, string typeName);




    glm::vec3 rotationAxis;
    float rotationAngle;

    glm::vec3 position;
};

