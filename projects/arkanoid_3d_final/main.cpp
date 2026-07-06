using namespace std;

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "shader.h"

// ============================================================================
// PROYECTO FINAL: Arkanoid 3D
// ----------------------------------------------------------------------------
// Migracion del prototipo Python/PyOpenGL al formato de las practicas:
// GLFW + GLAD + OpenGL 3.3 core + VAO/VBO + shaders externos.
//
// CONTROLES:
//   WASD / Flechas  Mover paddle en X/Z
//   ESPACIO         Pausa
//   R               Reiniciar
//   ESC             Salir
// ============================================================================

const unsigned int SCR_WIDTH  = 1100;
const unsigned int SCR_HEIGHT = 750;

const float PI = 3.14159265f;

// Coordenadas del mundo:
// X = izquierda/derecha
// Y = profundidad: camara/paddle abajo -> bloques arriba/fondo
// Z = altura visual en pantalla
const float CAMERA_POS[3]  = { 0.0f, -15.8f, 2.4f };
const float CAMERA_LOOK[3] = { 0.0f,  -1.5f, 0.85f };

const float PADDLE_Y    = -9.0f;
const float BALL_RADIUS = 0.25f;

const float ROOM_X_LIMIT = 6.0f;
const float ROOM_Z_MIN   = -2.5f;
const float ROOM_Z_MAX   = 4.5f;
const float BACK_WALL_Y  = 7.2f;
const float LOSE_Y       = -11.0f;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);

static std::string getShaderPath(const std::string& filename)
{
    std::filesystem::path srcFile = __FILE__;
    return (srcFile.parent_path() / "shaders" / filename).string();
}

struct Vec3
{
    float x, y, z;
};

static Vec3 sub3(Vec3 a, Vec3 b)
{
    return { a.x - b.x, a.y - b.y, a.z - b.z };
}

static float dot3(Vec3 a, Vec3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static Vec3 cross3(Vec3 a, Vec3 b)
{
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

static Vec3 normalize3(Vec3 v)
{
    float len = sqrtf(dot3(v, v));
    if (len <= 0.00001f) return { 0.0f, 0.0f, 0.0f };
    return { v.x / len, v.y / len, v.z / len };
}

static void identity(float* m)
{
    for (int i = 0; i < 16; i++) m[i] = 0.0f;
    m[0] = 1.0f;
    m[5] = 1.0f;
    m[10] = 1.0f;
    m[15] = 1.0f;
}

static void matMul(float* out, const float* A, const float* B)
{
    float tmp[16] = {};
    for (int col = 0; col < 4; col++)
        for (int row = 0; row < 4; row++)
            for (int k = 0; k < 4; k++)
                tmp[col * 4 + row] += A[k * 4 + row] * B[col * 4 + k];

    for (int i = 0; i < 16; i++) out[i] = tmp[i];
}

static void buildPerspective(float* m, float fovDeg, float aspect, float nearP, float farP)
{
    float f = 1.0f / tanf(fovDeg * PI / 360.0f);
    for (int i = 0; i < 16; i++) m[i] = 0.0f;
    m[0]  = f / aspect;
    m[5]  = f;
    m[10] = (farP + nearP) / (nearP - farP);
    m[11] = -1.0f;
    m[14] = (2.0f * farP * nearP) / (nearP - farP);
}

static void buildLookAt(float* m, Vec3 eye, Vec3 center, Vec3 up)
{
    Vec3 f = normalize3(sub3(center, eye));
    Vec3 s = normalize3(cross3(f, up));
    Vec3 u = cross3(s, f);

    identity(m);
    m[0] = s.x;  m[4] = s.y;  m[8]  = s.z;
    m[1] = u.x;  m[5] = u.y;  m[9]  = u.z;
    m[2] = -f.x; m[6] = -f.y; m[10] = -f.z;

    m[12] = -dot3(s, eye);
    m[13] = -dot3(u, eye);
    m[14] =  dot3(f, eye);
}

static void buildTranslation(float* m, float tx, float ty, float tz)
{
    identity(m);
    m[12] = tx;
    m[13] = ty;
    m[14] = tz;
}

static void buildScale(float* m, float sx, float sy, float sz)
{
    identity(m);
    m[0] = sx;
    m[5] = sy;
    m[10] = sz;
}

static void buildRotationX(float* m, float angle)
{
    identity(m);
    float c = cosf(angle), s = sinf(angle);
    m[5] = c;
    m[6] = s;
    m[9] = -s;
    m[10] = c;
}

static void buildRotationY(float* m, float angle)
{
    identity(m);
    float c = cosf(angle), s = sinf(angle);
    m[0] = c;
    m[2] = -s;
    m[8] = s;
    m[10] = c;
}

static void buildRotationZ(float* m, float angle)
{
    identity(m);
    float c = cosf(angle), s = sinf(angle);
    m[0] = c;
    m[1] = s;
    m[4] = -s;
    m[5] = c;
}

static void buildModel(float* m,
                       float tx, float ty, float tz,
                       float sx, float sy, float sz,
                       float rx = 0.0f, float ry = 0.0f, float rz = 0.0f)
{
    float T[16], S[16], RX[16], RY[16], RZ[16], tmp1[16], tmp2[16], tmp3[16];

    buildTranslation(T, tx, ty, tz);
    buildScale(S, sx, sy, sz);
    buildRotationX(RX, rx);
    buildRotationY(RY, ry);
    buildRotationZ(RZ, rz);

    matMul(tmp1, RX, S);
    matMul(tmp2, RY, tmp1);
    matMul(tmp3, RZ, tmp2);
    matMul(m, T, tmp3);
}

struct Mesh
{
    unsigned int VAO = 0;
    unsigned int VBO = 0;
    int vertexCount = 0;
    GLenum primitive = GL_TRIANGLES;
};

static std::vector<float> makeDefaultNormals(int vertexCount)
{
    std::vector<float> normals;
    normals.reserve(vertexCount * 3);
    for (int i = 0; i < vertexCount; i++)
        normals.insert(normals.end(), { 0.0f, 0.0f, 1.0f });
    return normals;
}

static Mesh createMesh(const std::vector<float>& positions,
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

static void deleteMesh(Mesh& mesh)
{
    if (mesh.VAO) glDeleteVertexArrays(1, &mesh.VAO);
    if (mesh.VBO) glDeleteBuffers(1, &mesh.VBO);
    mesh.VAO = 0;
    mesh.VBO = 0;
}

static std::vector<float> createCubeSolidPositions()
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

static std::vector<float> createCubeSolidNormals()
{
    std::vector<float> normals;
    Vec3 faceNormals[6] = {
        {  0.0f,  0.0f, -1.0f },
        {  0.0f,  0.0f,  1.0f },
        { -1.0f,  0.0f,  0.0f },
        {  1.0f,  0.0f,  0.0f },
        {  0.0f, -1.0f,  0.0f },
        {  0.0f,  1.0f,  0.0f }
    };

    for (Vec3 n : faceNormals)
        for (int i = 0; i < 6; i++)
            normals.insert(normals.end(), { n.x, n.y, n.z });

    return normals;
}

static std::vector<float> createCubeWirePositions()
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

static std::vector<float> createGridPositions()
{
    std::vector<float> vertices;

    // Piso
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

    // Pared izquierda y derecha: las lineas van apenas hacia adentro del cuarto
    // para que no queden ocultas por la profundidad de las paredes solidas.
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

    // Pared del fondo
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

    // Pared superior / techo
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

static std::vector<float> createSpherePositions(int stacks, int slices)
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

static std::vector<float> createSphereNormals(const std::vector<float>& positions)
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
        x = 0.0f;
        y = -7.2f;
        z = -0.9f;
        vx = 2.0f;
        vy = 6.0f;
        vz = 1.2f;
        rotX = 0.0f;
        rotY = 0.0f;
        rotZ = 0.0f;
        spinX = 2.8f;
        spinY = 3.4f;
        spinZ = 1.7f;
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

static std::vector<Block> createBlocks()
{
    std::vector<Block> blocks;
    float colors[6][4] = {
        { 1.0f, 0.15f, 0.12f, 0.95f },
        { 1.0f, 0.55f, 0.05f, 0.95f },
        { 1.0f, 0.85f, 0.05f, 0.95f },
        { 0.1f, 0.85f, 0.25f, 0.95f },
        { 0.1f, 0.55f, 1.0f, 0.95f },
        { 0.6f, 0.15f, 1.0f, 0.95f }
    };

    int rows = 6;
    int cols = 8;
    float startX = -4.9f;
    float startZ = 3.7f;
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

static bool sphereBoxCollision(const Ball& ball, const Block& box)
{
    float closestX = std::clamp(ball.x, box.x - box.w * 0.5f, box.x + box.w * 0.5f);
    float closestY = std::clamp(ball.y, box.y - box.d * 0.5f, box.y + box.d * 0.5f);
    float closestZ = std::clamp(ball.z, box.z - box.h * 0.5f, box.z + box.h * 0.5f);

    float dx = ball.x - closestX;
    float dy = ball.y - closestY;
    float dz = ball.z - closestZ;

    return dx * dx + dy * dy + dz * dz <= BALL_RADIUS * BALL_RADIUS;
}

static int handleCollisions(Ball& ball, const Paddle& paddle, std::vector<Block>& blocks)
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

static void updateBlocks(std::vector<Block>& blocks, float dt)
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

static void updateBallTrail(std::vector<TrailPoint>& trail, const Ball& ball, float dt, bool emit)
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

static void drawMesh(Shader& shader, const Mesh& mesh, const float* viewProjection,
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

static void updateWindowTitle(GLFWwindow* window, int score, int lives, bool paused, bool won)
{
    std::ostringstream title;
    title << "Arkanoid 3D | Score: " << score << " | Lives: " << lives;

    if (won)
        title << " | GANASTE - R para reiniciar";
    else if (lives <= 0)
        title << " | GAME OVER - R para reiniciar";
    else if (paused)
        title << " | PAUSA";

    glfwSetWindowTitle(window, title.str().c_str());
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Arkanoid 3D", NULL, NULL);
    if (!window)
    {
        std::cout << "Error: no se pudo crear la ventana GLFW" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Error: no se pudo inicializar GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Shader shader(getShaderPath("vertex_shader.glsl").c_str(),
                  getShaderPath("fragment_shader.glsl").c_str());

    std::vector<float> cubeSolidPositions = createCubeSolidPositions();
    std::vector<float> cubeWirePositions = createCubeWirePositions();
    std::vector<float> gridPositions = createGridPositions();
    std::vector<float> spherePositions = createSpherePositions(18, 28);

    Mesh cubeSolid = createMesh(cubeSolidPositions, createCubeSolidNormals(), GL_TRIANGLES);
    Mesh cubeWire  = createMesh(cubeWirePositions, makeDefaultNormals((int)cubeWirePositions.size() / 3), GL_LINES);
    Mesh grid      = createMesh(gridPositions, makeDefaultNormals((int)gridPositions.size() / 3), GL_LINES);
    Mesh sphere    = createMesh(spherePositions, createSphereNormals(spherePositions), GL_TRIANGLES);

    Paddle paddle;
    Ball ball;
    ball.reset();
    std::vector<TrailPoint> ballTrail;
    std::vector<Block> blocks = createBlocks();

    int score = 0;
    int lives = 3;
    bool paused = false;
    bool spaceBlocked = false;
    bool resetBlocked = false;

    std::cout << "+------------------ ARKANOID 3D ------------------+" << std::endl;
    std::cout << "| WASD/Flechas: mover paddle en X/Z               |" << std::endl;
    std::cout << "| ESPACIO: pausa | R: reiniciar | ESC: salir      |" << std::endl;
    std::cout << "+--------------------------------------------------+" << std::endl;

    float projection[16], view[16], viewProjection[16];
    buildPerspective(projection, 63.0f, (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    buildLookAt(view,
                { CAMERA_POS[0], CAMERA_POS[1], CAMERA_POS[2] },
                { CAMERA_LOOK[0], CAMERA_LOOK[1], CAMERA_LOOK[2] },
                { 0.0f, 0.0f, 1.0f });
    matMul(viewProjection, projection, view);

    float lastTime = (float)glfwGetTime();

    while (!glfwWindowShouldClose(window))
    {
        float now = (float)glfwGetTime();
        float dt = now - lastTime;
        lastTime = now;
        dt = std::min(dt, 0.033f);

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        bool spacePressed = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
        if (spacePressed && !spaceBlocked)
        {
            paused = !paused;
            spaceBlocked = true;
        }
        if (!spacePressed)
            spaceBlocked = false;

        bool resetPressed = glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS;
        if (resetPressed && !resetBlocked)
        {
            paddle = Paddle();
            ball.reset();
            ballTrail.clear();
            blocks = createBlocks();
            score = 0;
            lives = 3;
            paused = false;
            resetBlocked = true;
        }
        if (!resetPressed)
            resetBlocked = false;

        if (paused)
            ball.updateRotation(dt);

        bool won = std::all_of(blocks.begin(), blocks.end(),
                               [](const Block& b) { return !b.alive; });

        if (!paused && lives > 0 && !won)
        {
            paddle.update(window, dt);
            ball.update(dt);
            score += handleCollisions(ball, paddle, blocks);
            updateBlocks(blocks, dt);
            updateBallTrail(ballTrail, ball, dt, true);

            if (ball.y < LOSE_Y)
            {
                lives--;
                ball.reset();
                ballTrail.clear();
            }
        }

        won = std::all_of(blocks.begin(), blocks.end(),
                          [](const Block& b) { return !b.alive; });
        if (lives <= 0 || won)
            paused = true;

        updateWindowTitle(window, score, lives, paused, won);

        glClearColor(0.015f, 0.015f, 0.025f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();
        glUniform3f(glGetUniformLocation(shader.ID, "uLightPos"), ball.x, ball.y, ball.z);
        glUniform3f(glGetUniformLocation(shader.ID, "uCameraPos"), CAMERA_POS[0], CAMERA_POS[1], CAMERA_POS[2]);

        // Piso y paredes del cuarto
        drawMesh(shader, cubeSolid, viewProjection,
                 0.0f, -2.15f, ROOM_Z_MIN - 0.03f,
                 ROOM_X_LIMIT * 2.0f, 18.7f, 0.06f,
                 0.13f, 0.17f, 0.23f, 1.0f);

        drawMesh(shader, cubeSolid, viewProjection,
                 -ROOM_X_LIMIT - 0.08f, -2.0f, 1.0f,
                 0.15f, 19.0f, 7.0f,
                 0.15f, 0.20f, 0.30f, 1.0f);

        drawMesh(shader, cubeSolid, viewProjection,
                 ROOM_X_LIMIT + 0.08f, -2.0f, 1.0f,
                 0.15f, 19.0f, 7.0f,
                 0.15f, 0.20f, 0.30f, 1.0f);

        drawMesh(shader, cubeSolid, viewProjection,
                 0.0f, BACK_WALL_Y + 0.15f, 1.0f,
                 12.5f, 0.25f, 7.0f,
                 0.12f, 0.17f, 0.25f, 1.0f);

        drawMesh(shader, grid, viewProjection,
                 0.0f, 0.0f, 0.0f,
                 1.0f, 1.0f, 1.0f,
                 0.30f, 0.55f, 0.78f, 0.85f,
                 false);

        // Bloques
        for (const Block& block : blocks)
        {
            if (!block.alive) continue;

            float progress = block.disappearing ? std::clamp(block.disappearTime / 0.72f, 0.0f, 1.0f) : 0.0f;
            float shrink = 1.0f - progress * 0.88f;
            float blink = block.disappearing
                ? (0.35f + 0.65f * fabsf(sinf(block.disappearTime * 42.0f)))
                : 1.0f;
            float alpha = block.color[3] * (1.0f - progress) * blink;

            drawMesh(shader, cubeSolid, viewProjection,
                     block.x, block.y, block.z,
                     block.w * shrink, block.d * shrink, block.h * shrink,
                     std::min(1.0f, block.color[0] + progress * 0.45f),
                     std::min(1.0f, block.color[1] + progress * 0.45f),
                     std::min(1.0f, block.color[2] + progress * 0.45f),
                     alpha);

            drawMesh(shader, cubeWire, viewProjection,
                     block.x, block.y, block.z,
                     block.w * shrink * 1.01f, block.d * shrink * 1.01f, block.h * shrink * 1.01f,
                     1.0f, 1.0f, 1.0f, 0.75f * (1.0f - progress),
                     false);
        }

        // Estela de plasma
        glDepthMask(GL_FALSE);
        for (const TrailPoint& point : ballTrail)
        {
            float life = std::clamp(1.0f - point.age / 0.85f, 0.0f, 1.0f);
            float size = BALL_RADIUS * (0.25f + 0.85f * life);
            drawMesh(shader, sphere, viewProjection,
                     point.x, point.y, point.z,
                     size, size, size,
                     0.15f + 0.25f * life,
                     0.65f + 0.30f * life,
                     1.0f,
                     0.08f + 0.42f * life,
                     false, true, false);
        }
        glDepthMask(GL_TRUE);

        // Bola
        float ballScale = BALL_RADIUS * (1.0f + ball.hitFlash * 0.18f);
        drawMesh(shader, sphere, viewProjection,
                 ball.x, ball.y, ball.z,
                 ballScale, ballScale, ballScale,
                 0.50f, 0.90f, 1.0f, 1.0f,
                 false, true, true,
                 ball.rotX, ball.rotY, ball.rotZ);

        // Paddle translucido
        glDepthMask(GL_FALSE);
        drawMesh(shader, cubeSolid, viewProjection,
                 paddle.x, PADDLE_Y, paddle.z,
                 paddle.w, paddle.d, paddle.h,
                 0.15f, 0.80f, 1.0f, 0.35f);

        drawMesh(shader, cubeWire, viewProjection,
                 paddle.x, PADDLE_Y, paddle.z,
                 paddle.w * 1.01f, paddle.d * 1.01f, paddle.h * 1.01f,
                 0.55f, 0.95f, 1.0f, 1.0f,
                 false);
        glDepthMask(GL_TRUE);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    deleteMesh(cubeSolid);
    deleteMesh(cubeWire);
    deleteMesh(grid);
    deleteMesh(sphere);
    shader.del();

    glfwTerminate();
    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}
