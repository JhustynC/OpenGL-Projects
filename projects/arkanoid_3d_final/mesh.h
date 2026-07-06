#ifndef ARKANOID_MESH_H
#define ARKANOID_MESH_H

#include <glad/glad.h>

#include <vector>

#include "config.h"
#include "math_utils.h"

struct Mesh
{
    unsigned int VAO = 0;
    unsigned int VBO = 0;
    int vertexCount = 0;
    GLenum primitive = GL_TRIANGLES;
};

inline std::vector<float> makeDefaultNormals(int vertexCount)
{
    std::vector<float> normals;
    normals.reserve(vertexCount * 3);
    for (int i = 0; i < vertexCount; i++)
        normals.insert(normals.end(), { 0.0f, 0.0f, 1.0f });
    return normals;
}

inline Mesh createMesh(const std::vector<float>& positions,
                       const std::vector<float>& normals,
                       GLenum primitive)
{
    Mesh mesh;
    mesh.vertexCount = (int)positions.size() / 3;
    mesh.primitive = primitive;

    glGenVertexArrays(1, &mesh.VAO);
    glGenBuffers(1, &mesh.VBO);

    glBindVertexArray(mesh.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
    glBufferData(GL_ARRAY_BUFFER,
                 (positions.size() + normals.size()) * sizeof(float),
                 nullptr,
                 GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
                    positions.size() * sizeof(float),
                    positions.data());
    glBufferSubData(GL_ARRAY_BUFFER, positions.size() * sizeof(float),
                    normals.size() * sizeof(float),
                    normals.data());

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float),
                          (void*)(positions.size() * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return mesh;
}

inline void deleteMesh(Mesh& mesh)
{
    if (mesh.VAO) glDeleteVertexArrays(1, &mesh.VAO);
    if (mesh.VBO) glDeleteBuffers(1, &mesh.VBO);
    mesh.VAO = 0;
    mesh.VBO = 0;
}

inline std::vector<float> createCubeSolidPositions()
{
    return {
        -0.5f,-0.5f,-0.5f,  0.5f,-0.5f,-0.5f,  0.5f, 0.5f,-0.5f,
         0.5f, 0.5f,-0.5f, -0.5f, 0.5f,-0.5f, -0.5f,-0.5f,-0.5f,
        -0.5f,-0.5f, 0.5f,  0.5f, 0.5f, 0.5f,  0.5f,-0.5f, 0.5f,
         0.5f, 0.5f, 0.5f, -0.5f,-0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
        -0.5f, 0.5f, 0.5f, -0.5f,-0.5f,-0.5f, -0.5f, 0.5f,-0.5f,
        -0.5f,-0.5f,-0.5f, -0.5f, 0.5f, 0.5f, -0.5f,-0.5f, 0.5f,
         0.5f, 0.5f, 0.5f,  0.5f, 0.5f,-0.5f,  0.5f,-0.5f,-0.5f,
         0.5f,-0.5f,-0.5f,  0.5f,-0.5f, 0.5f,  0.5f, 0.5f, 0.5f,
        -0.5f,-0.5f,-0.5f,  0.5f,-0.5f, 0.5f,  0.5f,-0.5f,-0.5f,
         0.5f,-0.5f, 0.5f, -0.5f,-0.5f,-0.5f, -0.5f,-0.5f, 0.5f,
        -0.5f, 0.5f,-0.5f,  0.5f, 0.5f,-0.5f,  0.5f, 0.5f, 0.5f,
         0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f,-0.5f
    };
}

inline std::vector<float> createCubeSolidNormals()
{
    std::vector<float> normals;
    Vec3 faceNormals[6] = {
        {  0.0f,  0.0f, -1.0f }, {  0.0f,  0.0f,  1.0f },
        { -1.0f,  0.0f,  0.0f }, {  1.0f,  0.0f,  0.0f },
        {  0.0f, -1.0f,  0.0f }, {  0.0f,  1.0f,  0.0f }
    };

    for (Vec3 n : faceNormals)
        for (int i = 0; i < 6; i++)
            normals.insert(normals.end(), { n.x, n.y, n.z });

    return normals;
}

inline std::vector<float> createCubeWirePositions()
{
    float x0 = -0.5f, x1 = 0.5f;
    float y0 = -0.5f, y1 = 0.5f;
    float z0 = -0.5f, z1 = 0.5f;

    return {
        x0,y0,z0, x1,y0,z0,  x1,y0,z0, x1,y1,z0,
        x1,y1,z0, x0,y1,z0,  x0,y1,z0, x0,y0,z0,
        x0,y0,z1, x1,y0,z1,  x1,y0,z1, x1,y1,z1,
        x1,y1,z1, x0,y1,z1,  x0,y1,z1, x0,y0,z1,
        x0,y0,z0, x0,y0,z1,  x1,y0,z0, x1,y0,z1,
        x1,y1,z0, x1,y1,z1,  x0,y1,z0, x0,y1,z1
    };
}

inline std::vector<float> createGridPositions()
{
    std::vector<float> vertices;

    for (int xi = -6; xi <= 6; xi++)
    {
        float x = (float)xi;
        vertices.insert(vertices.end(), { x, -11.5f, ROOM_Z_MIN + 0.02f, x, BACK_WALL_Y, ROOM_Z_MIN + 0.02f });
    }
    for (int yi = -11; yi <= 7; yi++)
    {
        float y = (float)yi;
        vertices.insert(vertices.end(), { -ROOM_X_LIMIT, y, ROOM_Z_MIN + 0.02f, ROOM_X_LIMIT, y, ROOM_Z_MIN + 0.02f });
    }

    float gridLeftX = -ROOM_X_LIMIT + 0.035f;
    float gridRightX = ROOM_X_LIMIT - 0.035f;
    for (int yi = -11; yi <= 7; yi++)
    {
        float y = (float)yi;
        vertices.insert(vertices.end(), { gridLeftX, y, ROOM_Z_MIN, gridLeftX, y, ROOM_Z_MAX });
        vertices.insert(vertices.end(), { gridRightX, y, ROOM_Z_MIN, gridRightX, y, ROOM_Z_MAX });
    }
    for (int zi = -2; zi <= 4; zi++)
    {
        float z = (float)zi;
        vertices.insert(vertices.end(), { gridLeftX, -11.5f, z, gridLeftX, BACK_WALL_Y, z });
        vertices.insert(vertices.end(), { gridRightX, -11.5f, z, gridRightX, BACK_WALL_Y, z });
    }

    for (int xi = -6; xi <= 6; xi++)
    {
        float x = (float)xi;
        vertices.insert(vertices.end(), { x, BACK_WALL_Y + 0.03f, ROOM_Z_MIN, x, BACK_WALL_Y + 0.03f, ROOM_Z_MAX });
    }
    for (int zi = -2; zi <= 4; zi++)
    {
        float z = (float)zi;
        vertices.insert(vertices.end(), { -ROOM_X_LIMIT, BACK_WALL_Y + 0.03f, z, ROOM_X_LIMIT, BACK_WALL_Y + 0.03f, z });
    }

    for (int xi = -6; xi <= 6; xi++)
    {
        float x = (float)xi;
        vertices.insert(vertices.end(), { x, -11.5f, ROOM_Z_MAX + 0.01f, x, BACK_WALL_Y, ROOM_Z_MAX + 0.01f });
    }
    for (int yi = -11; yi <= 7; yi++)
    {
        float y = (float)yi;
        vertices.insert(vertices.end(), { -ROOM_X_LIMIT, y, ROOM_Z_MAX + 0.01f, ROOM_X_LIMIT, y, ROOM_Z_MAX + 0.01f });
    }

    return vertices;
}

inline std::vector<float> createSpherePositions(int stacks, int slices)
{
    std::vector<float> vertices;

    for (int stack = 0; stack < stacks; stack++)
    {
        float phi0 = -PI * 0.5f + PI * (float)stack / (float)stacks;
        float phi1 = -PI * 0.5f + PI * (float)(stack + 1) / (float)stacks;

        for (int slice = 0; slice < slices; slice++)
        {
            float theta0 = 2.0f * PI * (float)slice / (float)slices;
            float theta1 = 2.0f * PI * (float)(slice + 1) / (float)slices;

            Vec3 p0 = { cosf(phi0) * cosf(theta0), cosf(phi0) * sinf(theta0), sinf(phi0) };
            Vec3 p1 = { cosf(phi0) * cosf(theta1), cosf(phi0) * sinf(theta1), sinf(phi0) };
            Vec3 p2 = { cosf(phi1) * cosf(theta1), cosf(phi1) * sinf(theta1), sinf(phi1) };
            Vec3 p3 = { cosf(phi1) * cosf(theta0), cosf(phi1) * sinf(theta0), sinf(phi1) };

            vertices.insert(vertices.end(), { p0.x,p0.y,p0.z, p1.x,p1.y,p1.z, p2.x,p2.y,p2.z });
            vertices.insert(vertices.end(), { p2.x,p2.y,p2.z, p3.x,p3.y,p3.z, p0.x,p0.y,p0.z });
        }
    }

    return vertices;
}

inline std::vector<float> createSphereNormals(const std::vector<float>& positions)
{
    std::vector<float> normals;
    normals.reserve(positions.size());

    for (int i = 0; i < (int)positions.size(); i += 3)
    {
        Vec3 n = normalize3({ positions[i], positions[i + 1], positions[i + 2] });
        normals.insert(normals.end(), { n.x, n.y, n.z });
    }

    return normals;
}

#endif
