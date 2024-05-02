#pragma once
#include <GLM.hpp>
#include <gtc/matrix_transform.hpp>

#include "Shader.h"
#include "Texture.h"
#include "Vertex.h"

#include <string>
#include <vector>

using namespace std;

class Mesh
{
public:
    // mesh Data
    unsigned int numVertices;
    std::vector <Vertex> vertices;

    unsigned int numIndexes;
    std::vector <unsigned int> indices;
    vector<Texture>      textures;
    unsigned int VAO;

    Mesh(const vector<Vertex>& vertices, const vector<unsigned int>& indices, const vector<Texture>& textures);
    Mesh(unsigned int numVertices, std::vector <Vertex> vertices, unsigned int numIndexes, std::vector <unsigned int> indices, const vector<Texture>& textures);
    void Draw(Shader& shader);


private:
    // render data 
    unsigned int VBO, EBO;
    void setupMesh();
};

