#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <algorithm>
#include <iostream>
#include <vector>

#include "config.h"
#include "game.h"
#include "math_utils.h"
#include "rendering.h"
#include "shader.h"
#include "ui.h"

// ============================================================================
// PROYECTO FINAL: Arkanoid 3D
// ----------------------------------------------------------------------------
// main.cpp queda como coordinador general:
//   1. Inicializa ventana/OpenGL/shaders/mallas.
//   2. Lee input global y cambia estados.
//   3. Actualiza la simulacion.
//   4. Llama al render 3D y a la UI.
//
// Para modificar el juego:
//   config.h      constantes generales/camara/cuarto
//   game.h        reglas, entidades, colisiones y estados
//   mesh.h        geometria base: cubos, esfera, cuadricula
//   rendering.h   dibujo de escena 3D
//   ui.h          HUD, menus y texto pixel-art
// ============================================================================

void framebuffer_size_callback(GLFWwindow* window, int width, int height);

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
    GameMeshes meshes = createGameMeshes();

    Paddle paddle;
    Ball ball;
    ball.reset();
    std::vector<TrailPoint> ballTrail;
    std::vector<Block> blocks = createBlocks();

    int score = 0;
    int lives = 3;
    GameState state = GameState::Start;
    bool spaceBlocked = false;
    bool enterBlocked = false;
    bool resetBlocked = false;
    bool trailBlocked = false;
    bool trailEnabled = true;
    float uiGlow = 1.0f;

    auto resetGame = [&]() {
        paddle = Paddle();
        ball.reset();
        ballTrail.clear();
        blocks = createBlocks();
        score = 0;
        lives = 3;
    };

    std::cout << "+------------------ ARKANOID 3D ------------------+" << std::endl;
    std::cout << "| ENTER: iniciar | WASD/Flechas: mover paddle     |" << std::endl;
    std::cout << "| SPACE: pausa | R: reiniciar | T: estela        |" << std::endl;
    std::cout << "| +/-: brillo UI | ESC: salir                     |" << std::endl;
    std::cout << "+--------------------------------------------------+" << std::endl;

    float projection[16], view[16], viewProjection[16], ortho[16];
    buildPerspective(projection, 63.0f, (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    buildLookAt(view,
                { CAMERA_POS[0], CAMERA_POS[1], CAMERA_POS[2] },
                { CAMERA_LOOK[0], CAMERA_LOOK[1], CAMERA_LOOK[2] },
                { 0.0f, 0.0f, 1.0f });
    matMul(viewProjection, projection, view);
    buildOrtho(ortho, 0.0f, (float)SCR_WIDTH, (float)SCR_HEIGHT, 0.0f, -1.0f, 1.0f);

    float lastTime = (float)glfwGetTime();

    while (!glfwWindowShouldClose(window))
    {
        float now = (float)glfwGetTime();
        float dt = std::min(now - lastTime, 0.033f);
        lastTime = now;

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        bool enterPressed = glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS;
        if (enterPressed && !enterBlocked)
        {
            if (state == GameState::Start)
            {
                resetGame();
                state = GameState::Playing;
            }
            else if (state == GameState::Paused)
            {
                state = GameState::Playing;
            }
            enterBlocked = true;
        }
        if (!enterPressed)
            enterBlocked = false;

        bool spacePressed = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
        if (spacePressed && !spaceBlocked)
        {
            if (state == GameState::Playing)
                state = GameState::Paused;
            else if (state == GameState::Paused)
                state = GameState::Playing;
            spaceBlocked = true;
        }
        if (!spacePressed)
            spaceBlocked = false;

        bool resetPressed = glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS;
        if (resetPressed && !resetBlocked)
        {
            resetGame();
            state = GameState::Playing;
            resetBlocked = true;
        }
        if (!resetPressed)
            resetBlocked = false;

        bool trailPressed = glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS;
        if (trailPressed && !trailBlocked)
        {
            trailEnabled = !trailEnabled;
            if (!trailEnabled)
                ballTrail.clear();
            trailBlocked = true;
        }
        if (!trailPressed)
            trailBlocked = false;

        if (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS)
            uiGlow = std::min(1.4f, uiGlow + dt);
        if (glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS)
            uiGlow = std::max(0.55f, uiGlow - dt);

        if (state != GameState::Playing)
            ball.updateRotation(dt);

        if (state == GameState::Playing)
        {
            paddle.update(window, dt);
            ball.update(dt);
            score += handleCollisions(ball, paddle, blocks);
            updateBlocks(blocks, dt);
            updateBallTrail(ballTrail, ball, dt, trailEnabled);

            if (ball.y < LOSE_Y)
            {
                lives--;
                ball.reset();
                ballTrail.clear();
            }

            bool won = std::all_of(blocks.begin(), blocks.end(),
                                   [](const Block& b) { return !b.alive; });
            if (lives <= 0)
                state = GameState::GameOver;
            else if (won)
                state = GameState::Win;
        }

        updateWindowTitle(window, score, lives, state != GameState::Playing, state == GameState::Win);

        glClearColor(0.015f, 0.015f, 0.025f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();
        glUniform3f(glGetUniformLocation(shader.ID, "uLightPos"), ball.x, ball.y, ball.z);
        glUniform3f(glGetUniformLocation(shader.ID, "uCameraPos"), CAMERA_POS[0], CAMERA_POS[1], CAMERA_POS[2]);

        drawScene3D(shader, meshes, viewProjection, paddle, ball, blocks, ballTrail);

        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        drawUiOverlay(shader, meshes.cubeSolid, ortho,
                      state, score, lives, visibleBlocksCount(blocks),
                      trailEnabled, uiGlow, now);
        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    deleteGameMeshes(meshes);
    shader.del();
    glfwTerminate();
    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}
