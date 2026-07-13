#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "shader.h"

// ============================================================================
// ARKANOID 3D SIMPLE
// Conserva los conceptos de la materia y elimina elementos secundarios.
// Ejes del mundo: X = izquierda/derecha, Y = profundidad, Z = altura.
// ============================================================================

const unsigned int WIDTH = 1000;
const unsigned int HEIGHT = 720;

const float LEFT_WALL = -6.0f;
const float RIGHT_WALL = 6.0f;
const float BACK_WALL = 7.0f;
const float LOSE_LINE = -8.0f;
const float FLOOR_Z = -2.5f;
const float CEILING_Z = 4.5f;
const float ROOM_CENTER_Y = (LOSE_LINE + BACK_WALL) * 0.5f;
const float ROOM_DEPTH = BACK_WALL - LOSE_LINE;
const float ROOM_CENTER_Z = (FLOOR_Z + CEILING_Z) * 0.5f;
const float ROOM_HEIGHT = CEILING_Z - FLOOR_Z;

const float PADDLE_Y = -6.0f;
const glm::vec3 PADDLE_HALF_SIZE(1.6f, 0.25f, 0.45f);
const glm::vec3 BLOCK_HALF_SIZE(0.6f, 0.25f, 0.3f);
const glm::vec3 CAMERA_POS(0.0f, -15.8f, 2.5f);

struct Mesh
{
    unsigned int VAO = 0;
    unsigned int VBO = 0;
    int vertexCount = 0;
};

struct Block
{
    glm::vec3 position;
    glm::vec3 color;
    bool alive = true;
    bool disappearing = false;
    float disappearTime = 0.0f;
};

struct Ball
{
    glm::vec3 position = glm::vec3(0.0f, -4.8f, -0.7f);
    glm::vec3 velocity = glm::vec3(3.2f, 5.0f, 1.5f);
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 spin = glm::vec3(2.5f, 3.2f, 1.8f);
    float radius = 0.28f;

    void reset()
    {
        position = glm::vec3(0.0f, -4.8f, -0.7f);
        velocity = glm::vec3(3.2f, 5.0f, 1.5f);
        rotation = glm::vec3(0.0f);
        spin = glm::vec3(2.5f, 3.2f, 1.8f);
    }

    void updateRotation(float dt)
    {
        rotation += spin * dt;
    }

    void registerCollision(float influenceX, float influenceY, float influenceZ)
    {
        spin.x = 2.4f + std::abs(velocity.z) * 0.40f
                 + std::abs(influenceZ);
        spin.y = 2.8f + std::abs(velocity.x) * 0.35f
                 + std::abs(influenceX);
        spin.z = 1.6f + std::abs(velocity.y) * 0.25f
                 + std::abs(influenceY);

        if (influenceX < 0.0f) spin.z *= -1.0f;
        if (influenceY < 0.0f) spin.x *= -1.0f;
        if (influenceZ < 0.0f) spin.y *= -1.0f;
    }
};

std::string shaderPath(const std::string& name)
{
    std::filesystem::path source = __FILE__;
    return (source.parent_path() / "shaders" / name).string();
}

// El VBO guarda grupos de seis floats: posicion (x,y,z) y normal (nx,ny,nz).
Mesh createMesh(const std::vector<float>& vertices)
{
    Mesh mesh;
    mesh.vertexCount = static_cast<int>(vertices.size() / 6);

    glGenVertexArrays(1, &mesh.VAO);
    glGenBuffers(1, &mesh.VBO);
    glBindVertexArray(mesh.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
    glBufferData(GL_ARRAY_BUFFER,
                 vertices.size() * sizeof(float),
                 vertices.data(),
                 GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    return mesh;
}

Mesh createCube()
{
    const std::vector<float> vertices = {
        // Cara posterior
        -0.5f,-0.5f,-0.5f,  0,0,-1,   0.5f, 0.5f,-0.5f,  0,0,-1,   0.5f,-0.5f,-0.5f,  0,0,-1,
         0.5f, 0.5f,-0.5f,  0,0,-1,  -0.5f,-0.5f,-0.5f,  0,0,-1,  -0.5f, 0.5f,-0.5f,  0,0,-1,
        // Cara frontal
        -0.5f,-0.5f, 0.5f,  0,0, 1,   0.5f,-0.5f, 0.5f,  0,0, 1,   0.5f, 0.5f, 0.5f,  0,0, 1,
         0.5f, 0.5f, 0.5f,  0,0, 1,  -0.5f, 0.5f, 0.5f,  0,0, 1,  -0.5f,-0.5f, 0.5f,  0,0, 1,
        // Cara izquierda
        -0.5f, 0.5f, 0.5f, -1,0,0,  -0.5f, 0.5f,-0.5f, -1,0,0,  -0.5f,-0.5f,-0.5f, -1,0,0,
        -0.5f,-0.5f,-0.5f, -1,0,0,  -0.5f,-0.5f, 0.5f, -1,0,0,  -0.5f, 0.5f, 0.5f, -1,0,0,
        // Cara derecha
         0.5f, 0.5f, 0.5f,  1,0,0,   0.5f,-0.5f,-0.5f,  1,0,0,   0.5f, 0.5f,-0.5f,  1,0,0,
         0.5f,-0.5f,-0.5f,  1,0,0,   0.5f, 0.5f, 0.5f,  1,0,0,   0.5f,-0.5f, 0.5f,  1,0,0,
        // Cara inferior
        -0.5f,-0.5f,-0.5f,  0,-1,0,   0.5f,-0.5f,-0.5f,  0,-1,0,   0.5f,-0.5f, 0.5f,  0,-1,0,
         0.5f,-0.5f, 0.5f,  0,-1,0,  -0.5f,-0.5f, 0.5f,  0,-1,0,  -0.5f,-0.5f,-0.5f,  0,-1,0,
        // Cara superior
        -0.5f, 0.5f,-0.5f,  0,1,0,   0.5f, 0.5f, 0.5f,  0,1,0,   0.5f, 0.5f,-0.5f,  0,1,0,
         0.5f, 0.5f, 0.5f,  0,1,0,  -0.5f, 0.5f,-0.5f,  0,1,0,  -0.5f, 0.5f, 0.5f,  0,1,0
    };
    return createMesh(vertices);
}

Mesh createSphere(int stacks = 12, int slices = 18)
{
    std::vector<float> vertices;
    const float pi = 3.14159265f;

    auto addVertex = [&vertices](glm::vec3 p)
    {
        glm::vec3 n = glm::normalize(p);
        vertices.insert(vertices.end(), {p.x, p.y, p.z, n.x, n.y, n.z});
    };

    for (int stack = 0; stack < stacks; ++stack)
    {
        float phi0 = -pi * 0.5f + pi * stack / stacks;
        float phi1 = -pi * 0.5f + pi * (stack + 1) / stacks;

        for (int slice = 0; slice < slices; ++slice)
        {
            float theta0 = 2.0f * pi * slice / slices;
            float theta1 = 2.0f * pi * (slice + 1) / slices;

            glm::vec3 p0(cosf(phi0)*cosf(theta0), cosf(phi0)*sinf(theta0), sinf(phi0));
            glm::vec3 p1(cosf(phi0)*cosf(theta1), cosf(phi0)*sinf(theta1), sinf(phi0));
            glm::vec3 p2(cosf(phi1)*cosf(theta1), cosf(phi1)*sinf(theta1), sinf(phi1));
            glm::vec3 p3(cosf(phi1)*cosf(theta0), cosf(phi1)*sinf(theta0), sinf(phi1));

            addVertex(p0); addVertex(p1); addVertex(p2);
            addVertex(p2); addVertex(p3); addVertex(p0);
        }
    }

    return createMesh(vertices);
}

std::vector<Block> createBlocks()
{
    std::vector<Block> blocks;
    const glm::vec3 colors[] = {
        {0.95f, 0.20f, 0.18f},
        {0.98f, 0.65f, 0.12f},
        {0.20f, 0.75f, 0.35f},
        {0.15f, 0.55f, 0.95f},
        {0.65f, 0.30f, 0.95f}
    };

    for (int row = 0; row < 5; ++row)
        for (int column = 0; column < 8; ++column)
            blocks.push_back({{-4.9f + column * 1.4f, 5.8f,
                               2.6f - row * 0.75f}, colors[row],
                              true, false, 0.0f});

    return blocks;
}

void drawObject(const Shader& shader, const Mesh& mesh,
                const glm::mat4& view, const glm::mat4& projection,
                glm::vec3 position, glm::vec3 scale,
                glm::vec3 color, bool unlit = false,
                glm::vec3 rotation = glm::vec3(0.0f),
                bool isBall = false, float alpha = 1.0f)
{
    // LOCAL -> WORLD: escala la malla local y la coloca en el juego.
    glm::mat4 model(1.0f);
    model = glm::translate(model, position);
    model = glm::rotate(model, rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, scale);

    // LOCAL -> WORLD -> VIEW -> CLIP.
    glm::mat4 mvp = projection * view * model;

    glUniformMatrix4fv(glGetUniformLocation(shader.ID, "uModel"),
                       1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(shader.ID, "uMVP"),
                       1, GL_FALSE, glm::value_ptr(mvp));
    glUniform3fv(glGetUniformLocation(shader.ID, "uColor"),
                 1, glm::value_ptr(color));
    glUniform1i(glGetUniformLocation(shader.ID, "uUnlit"), unlit ? 1 : 0);
    glUniform1i(glGetUniformLocation(shader.ID, "uIsBall"), isBall ? 1 : 0);
    glUniform1f(glGetUniformLocation(shader.ID, "uAlpha"), alpha);

    glBindVertexArray(mesh.VAO);
    glDrawArrays(GL_TRIANGLES, 0, mesh.vertexCount);
}

bool sphereTouchesBox(const glm::vec3& spherePosition, float radius,
                      const glm::vec3& boxCenter, const glm::vec3& boxHalfSize,
                      glm::vec3& collisionNormal)
{
    glm::vec3 closestPoint = glm::clamp(
        spherePosition,
        boxCenter - boxHalfSize,
        boxCenter + boxHalfSize
    );
    glm::vec3 difference = spherePosition - closestPoint;
    float distanceSquared = glm::dot(difference, difference);

    if (distanceSquared > radius * radius)
        return false;

    if (distanceSquared > 0.000001f)
    {
        collisionNormal = glm::normalize(difference);
        return true;
    }

    // Caso poco frecuente: el centro de la bola quedo dentro de la caja.
    glm::vec3 local = spherePosition - boxCenter;
    glm::vec3 penetration = boxHalfSize - glm::abs(local);
    if (penetration.x <= penetration.y && penetration.x <= penetration.z)
        collisionNormal = {local.x < 0.0f ? -1.0f : 1.0f, 0.0f, 0.0f};
    else if (penetration.y <= penetration.z)
        collisionNormal = {0.0f, local.y < 0.0f ? -1.0f : 1.0f, 0.0f};
    else
        collisionNormal = {0.0f, 0.0f, local.z < 0.0f ? -1.0f : 1.0f};

    return true;
}

void deleteMesh(Mesh& mesh)
{
    glDeleteVertexArrays(1, &mesh.VAO);
    glDeleteBuffers(1, &mesh.VBO);
}

void framebufferSizeCallback(GLFWwindow*, int width, int height)
{
    glViewport(0, 0, width, height);
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Arkanoid 3D Simple", nullptr, nullptr);
    if (!window)
    {
        std::cout << "No se pudo crear la ventana." << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "No se pudo iniciar GLAD." << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Shader shader(shaderPath("vertex_shader.glsl").c_str(),
                  shaderPath("fragment_shader.glsl").c_str());
    Mesh cube = createCube();
    Mesh sphere = createSphere();

    std::vector<Block> blocks = createBlocks();
    Ball ball;
    glm::vec3 paddlePosition(0.0f, PADDLE_Y, -0.8f);
    int score = 0;
    int lives = 3;

    // VIEW: posicion de la camara, punto observado y direccion arriba.
    glm::mat4 view = glm::lookAt(
        CAMERA_POS,
        glm::vec3(0.0f, -0.5f, 0.8f),
        glm::vec3(0.0f, 0.0f, 1.0f)
    );

    // PROJECTION: lente perspectiva; su salida es el marco de clip.
    glm::mat4 projection = glm::perspective(
        glm::radians(52.0f),
        static_cast<float>(WIDTH) / HEIGHT,
        0.1f,
        100.0f
    );

    std::cout << "ARKANOID 3D SIMPLE\n"
              << "A/D o izquierda/derecha: eje X\n"
              << "W/S o arriba/abajo: eje Z\n"
              << "R: reiniciar | ESC: salir\n";

    float previousTime = static_cast<float>(glfwGetTime());
    while (!glfwWindowShouldClose(window))
    {
        float currentTime = static_cast<float>(glfwGetTime());
        float dt = std::min(currentTime - previousTime, 0.033f);
        previousTime = currentTime;

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        float directionX = 0.0f;
        float directionZ = 0.0f;
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            directionX -= 1.0f;
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            directionX += 1.0f;
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            directionZ += 1.0f;
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            directionZ -= 1.0f;

        paddlePosition.x += directionX * 7.0f * dt;
        paddlePosition.z += directionZ * 7.0f * dt;
        paddlePosition.x = std::clamp(paddlePosition.x,
                                      LEFT_WALL + PADDLE_HALF_SIZE.x,
                                      RIGHT_WALL - PADDLE_HALF_SIZE.x);
        paddlePosition.z = std::clamp(paddlePosition.z,
                                      FLOOR_Z + PADDLE_HALF_SIZE.z,
                                      CEILING_Z - PADDLE_HALF_SIZE.z);

        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
        {
            ball.reset();
            blocks = createBlocks();
            paddlePosition = glm::vec3(0.0f, PADDLE_Y, -0.8f);
            score = 0;
            lives = 3;
        }

        // Actualizacion del juego en CPU.
        ball.position += ball.velocity * dt;
        ball.updateRotation(dt);

        // Los bloques golpeados se encogen y desaparecen durante 0.72 segundos.
        for (Block& block : blocks)
        {
            if (!block.disappearing)
                continue;

            block.disappearTime += dt;
            if (block.disappearTime >= 0.72f)
            {
                block.alive = false;
                block.disappearing = false;
            }
        }

        if (ball.position.x - ball.radius < LEFT_WALL ||
            ball.position.x + ball.radius > RIGHT_WALL)
        {
            ball.velocity.x *= -1.0f;
            ball.position.x = std::clamp(ball.position.x,
                                         LEFT_WALL + ball.radius,
                                         RIGHT_WALL - ball.radius);
            ball.registerCollision(ball.velocity.x, 0.0f, 0.0f);
        }

        if (ball.position.z - ball.radius < FLOOR_Z ||
            ball.position.z + ball.radius > CEILING_Z)
        {
            ball.velocity.z *= -1.0f;
            ball.position.z = std::clamp(ball.position.z,
                                         FLOOR_Z + ball.radius,
                                         CEILING_Z - ball.radius);
            ball.registerCollision(0.0f, 0.0f, ball.velocity.z);
        }

        if (ball.position.y + ball.radius > BACK_WALL)
        {
            ball.velocity.y = -std::abs(ball.velocity.y);
            ball.position.y = BACK_WALL - ball.radius;
            ball.registerCollision(0.0f, ball.velocity.y, 0.0f);
        }

        glm::vec3 collisionNormal(0.0f);
        if (ball.velocity.y < 0.0f &&
            sphereTouchesBox(ball.position, ball.radius,
                             paddlePosition, PADDLE_HALF_SIZE,
                             collisionNormal))
        {
            ball.position.y = PADDLE_Y + PADDLE_HALF_SIZE.y + ball.radius;
            ball.velocity = glm::reflect(ball.velocity, collisionNormal);
            ball.velocity.y = std::abs(ball.velocity.y);

            // El punto donde golpea el paddle modifica la salida en X y Z.
            ball.velocity.x += (ball.position.x - paddlePosition.x) * 0.8f;
            ball.velocity.z += (ball.position.z - paddlePosition.z) * 0.8f;
            ball.registerCollision(collisionNormal.x,
                                   collisionNormal.y,
                                   collisionNormal.z);
        }

        for (Block& block : blocks)
        {
            if (block.alive && !block.disappearing &&
                sphereTouchesBox(ball.position, ball.radius,
                                 block.position, BLOCK_HALF_SIZE,
                                 collisionNormal))
            {
                block.disappearing = true;
                block.disappearTime = 0.0f;
                if (glm::dot(ball.velocity, collisionNormal) < 0.0f)
                    ball.velocity = glm::reflect(ball.velocity, collisionNormal);
                ball.registerCollision(collisionNormal.x,
                                       collisionNormal.y,
                                       collisionNormal.z);
                score += 10;
                break;
            }
        }

        if (ball.position.y < LOSE_LINE)
        {
            --lives;
            ball.reset();
            if (lives <= 0)
            {
                blocks = createBlocks();
                score = 0;
                lives = 3;
            }
        }

        std::ostringstream title;
        title << "Arkanoid 3D Simple | Puntaje: " << score << " | Vidas: " << lives;
        glfwSetWindowTitle(window, title.str().c_str());

        // Dibujo de un frame.
        glClearColor(0.025f, 0.03f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();
        glUniform3fv(glGetUniformLocation(shader.ID, "uCameraPos"),
                     1, glm::value_ptr(CAMERA_POS));
        // La bola es una fuente puntual de luz y la luz viaja con ella.
        glm::vec3 lightPosition = ball.position;
        glUniform3fv(glGetUniformLocation(shader.ID, "uLightPos"),
                     1, glm::value_ptr(lightPosition));

        // El mismo cubo local se reutiliza para construir todo el cuarto.
        drawObject(shader, cube, view, projection,
                   {0.0f, ROOM_CENTER_Y, FLOOR_Z - 0.1f},
                   {12.4f, ROOM_DEPTH, 0.2f}, {0.10f, 0.14f, 0.22f});
        drawObject(shader, cube, view, projection,
                   {0.0f, ROOM_CENTER_Y, CEILING_Z + 0.1f},
                   {12.4f, ROOM_DEPTH, 0.2f}, {0.08f, 0.12f, 0.20f});
        drawObject(shader, cube, view, projection,
                   {LEFT_WALL - 0.1f, ROOM_CENTER_Y, ROOM_CENTER_Z},
                   {0.2f, ROOM_DEPTH, ROOM_HEIGHT}, {0.12f, 0.25f, 0.48f});
        drawObject(shader, cube, view, projection,
                   {RIGHT_WALL + 0.1f, ROOM_CENTER_Y, ROOM_CENTER_Z},
                   {0.2f, ROOM_DEPTH, ROOM_HEIGHT}, {0.12f, 0.25f, 0.48f});
        drawObject(shader, cube, view, projection,
                   {0.0f, BACK_WALL + 0.1f, ROOM_CENTER_Z},
                   {12.4f, 0.2f, ROOM_HEIGHT}, {0.12f, 0.25f, 0.48f});

        for (const Block& block : blocks)
            if (block.alive && !block.disappearing)
                drawObject(shader, cube, view, projection,
                           block.position,
                           BLOCK_HALF_SIZE * 2.0f, block.color);

        // Los bloques que desaparecen se dibujan transparentes y sin escribir
        // profundidad, para que el parpadeo no oculte objetos posteriores.
        glDepthMask(GL_FALSE);
        for (const Block& block : blocks)
        {
            if (!block.alive || !block.disappearing)
                continue;

            float progress = std::clamp(block.disappearTime / 0.72f, 0.0f, 1.0f);
            float shrink = 1.0f - progress * 0.88f;
            float blink = 0.35f + 0.65f * std::abs(sinf(block.disappearTime * 42.0f));
            float alpha = (1.0f - progress) * blink;
            glm::vec3 flashColor = glm::mix(block.color, glm::vec3(1.0f),
                                            progress * 0.45f);

            drawObject(shader, cube, view, projection,
                       block.position,
                       BLOCK_HALF_SIZE * 2.0f * shrink,
                       flashColor, false, glm::vec3(0.0f), false, alpha);
        }
        glDepthMask(GL_TRUE);

        drawObject(shader, sphere, view, projection,
                   ball.position,
                   {ball.radius, ball.radius, ball.radius},
                   {1.0f, 0.45f, 0.12f}, true,
                   ball.rotation, true);

        // El paddle se dibuja al final para mezclarlo con la escena ya creada.
        glDepthMask(GL_FALSE);
        drawObject(shader, cube, view, projection,
                   paddlePosition,
                   PADDLE_HALF_SIZE * 2.0f, {0.15f, 0.85f, 0.95f},
                   false, glm::vec3(0.0f), false, 0.38f);
        glDepthMask(GL_TRUE);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    deleteMesh(cube);
    deleteMesh(sphere);
    shader.destroy();
    glfwTerminate();
    return 0;
}
