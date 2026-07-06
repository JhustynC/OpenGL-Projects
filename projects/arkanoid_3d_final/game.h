#ifndef ARKANOID_GAME_H
#define ARKANOID_GAME_H

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>
#include <vector>

#include "config.h"

struct Paddle
{
    float x = 0.0f;
    float z = -1.25f;
    float w = 4.6f;
    float h = 0.72f;
    float d = 0.45f;
    float speed = 8.0f;

    void update(GLFWwindow* window, float dt)
    {
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            x -= speed * dt;
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            x += speed * dt;
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            z += speed * dt;
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            z -= speed * dt;

        x = std::clamp(x, -ROOM_X_LIMIT + w * 0.5f, ROOM_X_LIMIT - w * 0.5f);
        z = std::clamp(z, ROOM_Z_MIN + h * 0.5f, ROOM_Z_MAX - h * 0.5f);
    }
};

struct Ball
{
    float x, y, z;
    float vx, vy, vz;
    float rotX, rotY, rotZ;
    float spinX, spinY, spinZ;
    float hitFlash;

    void reset()
    {
        x = 0.0f; y = -7.2f; z = -0.9f;
        vx = 2.0f; vy = 6.0f; vz = 1.2f;
        rotX = 0.0f; rotY = 0.0f; rotZ = 0.0f;
        spinX = 2.8f; spinY = 3.4f; spinZ = 1.7f;
        hitFlash = 0.0f;
    }

    void update(float dt)
    {
        x += vx * dt;
        y += vy * dt;
        z += vz * dt;
        updateRotation(dt);
    }

    void updateRotation(float dt)
    {
        rotX += spinX * dt;
        rotY += spinY * dt;
        rotZ += spinZ * dt;
        hitFlash = std::max(0.0f, hitFlash - dt * 3.0f);
    }

    void registerHit(float impulseX, float impulseZ)
    {
        spinX = 3.0f + fabsf(vz) * 0.65f + fabsf(impulseZ) * 1.8f;
        spinY = 4.0f + fabsf(vx) * 0.45f + fabsf(vy) * 0.12f;
        spinZ = 2.0f + fabsf(impulseX) * 2.2f;
        hitFlash = 1.0f;
    }
};

struct TrailPoint
{
    float x, y, z;
    float age;
};

enum class GameState
{
    Start,
    Playing,
    Paused,
    GameOver,
    Win
};

struct Block
{
    float x, y, z;
    float w = 1.25f;
    float h = 0.38f;
    float d = 0.55f;
    float color[4];
    bool alive = true;
    bool disappearing = false;
    float disappearTime = 0.0f;
};

inline std::vector<Block> createBlocks()
{
    std::vector<Block> blocks;
    float colors[6][4] = {
        { 1.0f, 0.15f, 0.12f, 0.95f }, { 1.0f, 0.55f, 0.05f, 0.95f },
        { 1.0f, 0.85f, 0.05f, 0.95f }, { 0.1f, 0.85f, 0.25f, 0.95f },
        { 0.1f, 0.55f, 1.0f, 0.95f }, { 0.6f, 0.15f, 1.0f, 0.95f }
    };

    int rows = 6;
    int cols = 8;
    float startX = -4.9f;
    float startZ = 2.55f;
    float y = 6.6f;

    for (int r = 0; r < rows; r++)
    {
        for (int c = 0; c < cols; c++)
        {
            Block block;
            block.x = startX + c * 1.4f;
            block.y = y;
            block.z = startZ - r * 0.55f;
            for (int i = 0; i < 4; i++) block.color[i] = colors[r % 6][i];
            blocks.push_back(block);
        }
    }

    return blocks;
}

inline bool sphereBoxCollision(const Ball& ball, const Block& box)
{
    float closestX = std::clamp(ball.x, box.x - box.w * 0.5f, box.x + box.w * 0.5f);
    float closestY = std::clamp(ball.y, box.y - box.d * 0.5f, box.y + box.d * 0.5f);
    float closestZ = std::clamp(ball.z, box.z - box.h * 0.5f, box.z + box.h * 0.5f);

    float dx = ball.x - closestX;
    float dy = ball.y - closestY;
    float dz = ball.z - closestZ;

    return dx * dx + dy * dy + dz * dz <= BALL_RADIUS * BALL_RADIUS;
}

inline int handleCollisions(Ball& ball, const Paddle& paddle, std::vector<Block>& blocks)
{
    if (ball.x - BALL_RADIUS < -ROOM_X_LIMIT)
    {
        ball.x = -ROOM_X_LIMIT + BALL_RADIUS;
        ball.vx *= -1.0f;
        ball.registerHit(1.0f, 0.0f);
    }
    if (ball.x + BALL_RADIUS > ROOM_X_LIMIT)
    {
        ball.x = ROOM_X_LIMIT - BALL_RADIUS;
        ball.vx *= -1.0f;
        ball.registerHit(1.0f, 0.0f);
    }
    if (ball.z - BALL_RADIUS < ROOM_Z_MIN)
    {
        ball.z = ROOM_Z_MIN + BALL_RADIUS;
        ball.vz *= -1.0f;
        ball.registerHit(0.0f, 1.0f);
    }
    if (ball.z + BALL_RADIUS > ROOM_Z_MAX)
    {
        ball.z = ROOM_Z_MAX - BALL_RADIUS;
        ball.vz *= -1.0f;
        ball.registerHit(0.0f, 1.0f);
    }
    if (ball.y + BALL_RADIUS > BACK_WALL_Y)
    {
        ball.y = BACK_WALL_Y - BALL_RADIUS;
        ball.vy *= -1.0f;
        ball.registerHit(0.6f, 0.6f);
    }

    Block paddleBox;
    paddleBox.x = paddle.x;
    paddleBox.y = PADDLE_Y;
    paddleBox.z = paddle.z;
    paddleBox.w = paddle.w;
    paddleBox.h = paddle.h;
    paddleBox.d = paddle.d;

    if (ball.vy < 0.0f && sphereBoxCollision(ball, paddleBox))
    {
        ball.y = PADDLE_Y + paddle.d * 0.5f + BALL_RADIUS;
        ball.vy = fabsf(ball.vy);

        float offsetX = (ball.x - paddle.x) / (paddle.w * 0.5f);
        float offsetZ = (ball.z - paddle.z) / (paddle.h * 0.5f);

        ball.vx += offsetX * 2.5f;
        ball.vz += offsetZ * 1.5f;
        ball.registerHit(offsetX, offsetZ);
    }

    for (Block& block : blocks)
    {
        if (block.alive && !block.disappearing && sphereBoxCollision(ball, block))
        {
            block.disappearing = true;
            block.disappearTime = 0.0f;
            ball.vy *= -1.0f;
            ball.registerHit(0.8f, 0.8f);
            return 10;
        }
    }

    return 0;
}

inline void updateBlocks(std::vector<Block>& blocks, float dt)
{
    const float duration = 0.72f;
    for (Block& block : blocks)
    {
        if (!block.disappearing) continue;
        block.disappearTime += dt;
        if (block.disappearTime >= duration)
        {
            block.alive = false;
            block.disappearing = false;
        }
    }
}

inline void updateBallTrail(std::vector<TrailPoint>& trail, const Ball& ball, float dt, bool emit)
{
    const float maxAge = 0.85f;
    for (TrailPoint& point : trail)
        point.age += dt;

    trail.erase(std::remove_if(trail.begin(), trail.end(),
                               [maxAge](const TrailPoint& p) { return p.age > maxAge; }),
                trail.end());

    if (!emit) return;

    bool shouldAdd = trail.empty();
    if (!trail.empty())
    {
        const TrailPoint& last = trail.back();
        float dx = ball.x - last.x;
        float dy = ball.y - last.y;
        float dz = ball.z - last.z;
        shouldAdd = dx * dx + dy * dy + dz * dz > 0.035f;
    }

    if (shouldAdd)
    {
        trail.push_back({ ball.x, ball.y, ball.z, 0.0f });
        if (trail.size() > 36)
            trail.erase(trail.begin());
    }
}

inline int visibleBlocksCount(const std::vector<Block>& blocks)
{
    int count = 0;
    for (const Block& block : blocks)
        if (block.alive)
            count++;
    return count;
}

#endif
