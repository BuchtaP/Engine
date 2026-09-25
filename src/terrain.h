#pragma once
#include <glad/glad.h>
#include <vector>
#include <string>
#include <iostream>
#include <glm/glm.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

class Terrain
{
public:
    unsigned int VAO, VBO, EBO;
    int width, height;
    std::vector<float> vertices; // pozice (3) + normala (3) = 6 floatu na vrchol
    std::vector<unsigned int> indices;

    void GenerateFromHeightmap(const std::string& path, float maxHeight, float vertexSpacing)
    {
        int channels;
        unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 1);

        if (!data)
        {
            std::cerr << "Nepodarilo se nacist heightmapu: " << path << "\n";
            return;
        }

        // Pomocna funkce na vypocet vysky (pro sousedy, kvuli normalam)
        auto getHeight = [&](int x, int z) -> float {
            x = glm::clamp(x, 0, width - 1);
            z = glm::clamp(z, 0, height - 1);
            return data[z * width + x] / 255.0f * maxHeight;
        };

        // Vygenerovani vertexu s normalami
        for (int z = 0; z < height; z++)
        {
            for (int x = 0; x < width; x++)
            {
                float heightValue = getHeight(x, z);

                // pozice
                vertices.push_back(x * vertexSpacing);
                vertices.push_back(heightValue);
                vertices.push_back(z * vertexSpacing);

                // normala - spocitana ze sousednich vysek (finite difference)
                float hL = getHeight(x - 1, z);
                float hR = getHeight(x + 1, z);
                float hD = getHeight(x, z - 1);
                float hU = getHeight(x, z + 1);

                glm::vec3 normal = glm::normalize(glm::vec3(hL - hR, 2.0f * vertexSpacing, hD - hU));

                vertices.push_back(normal.x);
                vertices.push_back(normal.y);
                vertices.push_back(normal.z);
            }
        }

        stbi_image_free(data);

        // Indexy zustavaji stejne
        for (int z = 0; z < height - 1; z++)
        {
            for (int x = 0; x < width - 1; x++)
            {
                int topLeft = z * width + x;
                int topRight = topLeft + 1;
                int bottomLeft = (z + 1) * width + x;
                int bottomRight = bottomLeft + 1;

                indices.push_back(topLeft);
                indices.push_back(bottomLeft);
                indices.push_back(topRight);

                indices.push_back(topRight);
                indices.push_back(bottomLeft);
                indices.push_back(bottomRight);
            }
        }

        SetupMesh();
    }

    void SetupMesh()
    {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

        // pozice (0) - 3 floaty, krok 6 floatu (pozice+normala), offset 0
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // normala (1) - 3 floaty, krok 6 floatu, offset 3 floaty
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
    }

    void Draw()
    {
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, (unsigned int)indices.size(), GL_UNSIGNED_INT, 0);
    }

     float GetHeightAt(float worldX, float worldZ)
    {
        int x = (int)worldX;
        int z = (int)worldZ;

        if (x < 0 || x >= width || z < 0 || z >= height)
            return 0.0f; // mimo mapu

        // najdeme index vrcholu v poli 'vertices' - kazdy vrchol ma 6 hodnot (pozice+normala)
        int index = (z * width + x) * 6;
        return vertices[index + 1]; // Y souradnice (vyska)
    }
};