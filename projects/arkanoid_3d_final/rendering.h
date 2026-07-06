#ifndef ARKANOID_RENDERING_H
#define ARKANOID_RENDERING_H

#include <glad/glad.h>

#include <algorithm>
#include <cmath>
#include <vector>

#include "config.h"
#include "game.h"
#include "math_utils.h"
#include "mesh.h"
#include "shader.h"

struct GameMeshes
{
    Mesh cubeSolid;
    Mesh cubeWire;
    Mesh grid;
    Mesh sphere;
};

inline GameMeshes createGameMeshes()
{
    std::vector<float> cubeSolidPositions = createCubeSolidPositions();
    std::vector<float> cubeWirePositions = createCubeWirePositions();
    std::vector<float> gridPositions = createGridPositions();
    std::vector<float> spherePositions = createSpherePositions(18, 28);

    GameMeshes meshes;
    meshes.cubeSolid = createMesh(cubeSolidPositions, createCubeSolidNormals(), GL_TRIANGLES);
    meshes.cubeWire  = createMesh(cubeWirePositions, makeDefaultNormals((int)cubeWirePositions.size() / 3), GL_LINES);
    meshes.grid      = createMesh(gridPositions, makeDefaultNormals((int)gridPositions.size() / 3), GL_LINES);
    meshes.sphere    = createMesh(spherePositions, createSphereNormals(spherePositions), GL_TRIANGLES);
    return meshes;
}

inline void deleteGameMeshes(GameMeshes& meshes)
{
    deleteMesh(meshes.cubeSolid);
    deleteMesh(meshes.cubeWire);
    deleteMesh(meshes.grid);
    deleteMesh(meshes.sphere);
}

inline void drawMesh(Shader& shader, const Mesh& mesh, const float* viewProjection,
                     float x, float y, float z, float sx, float sy, float sz,
                     float r, float g, float b, float a,
                     bool useLighting = true,
                     bool isLight = false,
                     bool isBall = false,
                     float rx = 0.0f, float ry = 0.0f, float rz = 0.0f)
{
    float model[16], mvp[16];
    buildModel(model, x, y, z, sx, sy, sz, rx, ry, rz);
    matMul(mvp, viewProjection, model);

    glUniformMatrix4fv(glGetUniformLocation(shader.ID, "uModel"), 1, GL_FALSE, model);
    glUniformMatrix4fv(glGetUniformLocation(shader.ID, "uMVP"), 1, GL_FALSE, mvp);
    glUniform4f(glGetUniformLocation(shader.ID, "uColor"), r, g, b, a);
    glUniform1i(glGetUniformLocation(shader.ID, "uUseLighting"), useLighting ? 1 : 0);
    glUniform1i(glGetUniformLocation(shader.ID, "uIsLight"), isLight ? 1 : 0);
    glUniform1i(glGetUniformLocation(shader.ID, "uIsBall"), isBall ? 1 : 0);

    glBindVertexArray(mesh.VAO);
    glDrawArrays(mesh.primitive, 0, mesh.vertexCount);
}

inline void drawScene3D(Shader& shader, const GameMeshes& meshes, const float* viewProjection,
                        const Paddle& paddle, const Ball& ball,
                        const std::vector<Block>& blocks,
                        const std::vector<TrailPoint>& ballTrail)
{
    drawMesh(shader, meshes.cubeSolid, viewProjection,
             0.0f, -2.15f, ROOM_Z_MIN - 0.03f,
             ROOM_X_LIMIT * 2.0f, 18.7f, 0.06f,
             0.13f, 0.17f, 0.23f, 1.0f);

    drawMesh(shader, meshes.cubeSolid, viewProjection,
             -ROOM_X_LIMIT - 0.08f, -2.0f, 1.0f,
             0.15f, 19.0f, 7.0f,
             0.15f, 0.20f, 0.30f, 1.0f);

    drawMesh(shader, meshes.cubeSolid, viewProjection,
             ROOM_X_LIMIT + 0.08f, -2.0f, 1.0f,
             0.15f, 19.0f, 7.0f,
             0.15f, 0.20f, 0.30f, 1.0f);

    drawMesh(shader, meshes.cubeSolid, viewProjection,
             0.0f, BACK_WALL_Y + 0.15f, 1.0f,
             12.5f, 0.25f, 7.0f,
             0.12f, 0.17f, 0.25f, 1.0f);

    drawMesh(shader, meshes.grid, viewProjection,
             0.0f, 0.0f, 0.0f,
             1.0f, 1.0f, 1.0f,
             0.30f, 0.55f, 0.78f, 0.85f,
             false);

    for (const Block& block : blocks)
    {
        if (!block.alive) continue;

        float progress = block.disappearing ? std::clamp(block.disappearTime / 0.72f, 0.0f, 1.0f) : 0.0f;
        float shrink = 1.0f - progress * 0.88f;
        float blink = block.disappearing
            ? (0.35f + 0.65f * fabsf(sinf(block.disappearTime * 42.0f)))
            : 1.0f;
        float alpha = block.color[3] * (1.0f - progress) * blink;

        drawMesh(shader, meshes.cubeSolid, viewProjection,
                 block.x, block.y, block.z,
                 block.w * shrink, block.d * shrink, block.h * shrink,
                 std::min(1.0f, block.color[0] + progress * 0.45f),
                 std::min(1.0f, block.color[1] + progress * 0.45f),
                 std::min(1.0f, block.color[2] + progress * 0.45f),
                 alpha);

        drawMesh(shader, meshes.cubeWire, viewProjection,
                 block.x, block.y, block.z,
                 block.w * shrink * 1.01f, block.d * shrink * 1.01f, block.h * shrink * 1.01f,
                 1.0f, 1.0f, 1.0f, 0.75f * (1.0f - progress),
                 false);
    }

    glDepthMask(GL_FALSE);
    for (const TrailPoint& point : ballTrail)
    {
        float life = std::clamp(1.0f - point.age / 0.85f, 0.0f, 1.0f);
        float size = BALL_RADIUS * (0.25f + 0.85f * life);
        drawMesh(shader, meshes.sphere, viewProjection,
                 point.x, point.y, point.z,
                 size, size, size,
                 0.15f + 0.25f * life,
                 0.65f + 0.30f * life,
                 1.0f,
                 0.08f + 0.42f * life,
                 false, true, false);
    }
    glDepthMask(GL_TRUE);

    float ballScale = BALL_RADIUS * (1.0f + ball.hitFlash * 0.18f);
    drawMesh(shader, meshes.sphere, viewProjection,
             ball.x, ball.y, ball.z,
             ballScale, ballScale, ballScale,
             0.50f, 0.90f, 1.0f, 1.0f,
             false, true, true,
             ball.rotX, ball.rotY, ball.rotZ);

    glDepthMask(GL_FALSE);
    drawMesh(shader, meshes.cubeSolid, viewProjection,
             paddle.x, PADDLE_Y, paddle.z,
             paddle.w, paddle.d, paddle.h,
             0.15f, 0.80f, 1.0f, 0.35f);

    drawMesh(shader, meshes.cubeWire, viewProjection,
             paddle.x, PADDLE_Y, paddle.z,
             paddle.w * 1.01f, paddle.d * 1.01f, paddle.h * 1.01f,
             0.55f, 0.95f, 1.0f, 1.0f,
             false);
    glDepthMask(GL_TRUE);
}

#endif
