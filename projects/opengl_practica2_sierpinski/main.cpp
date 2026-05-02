using namespace std;

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>
#include <cstdlib>   // rand(), srand()
#include <ctime>     // time()
#include <filesystem>

#include "shader.h"

// -----------------------------------------------------------------------
// Configuración — cambia estos valores para ajustar la animación
// -----------------------------------------------------------------------
const unsigned int SCR_WIDTH      = 600;
const unsigned int SCR_HEIGHT     = 600;   // cuadrada para que el triángulo no se deforme
const int          MAX_PUNTOS     = 500000; // total de puntos a generar
const int          PUNTOS_X_FRAME = 60;   // cuántos puntos nuevos se añaden por frame

// -----------------------------------------------------------------------
// Vértices del triángulo principal (en NDC)
//   v2 arriba al centro
//   v1 abajo a la izquierda
//   v3 abajo a la derecha
// -----------------------------------------------------------------------
const float V1[2] = { -0.9f, -0.8f };  // inferior izquierdo
const float V2[2] = {  0.0f,  0.9f };  // superior central
const float V3[2] = {  0.9f, -0.8f };  // inferior derecho

const float* VERTICES_TRIANGULO[3] = { V1, V2, V3 };

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);

static std::string getShaderPath(const std::string& filename)
{
    std::filesystem::path srcFile   = __FILE__;
    std::filesystem::path shaderDir = srcFile.parent_path() / "shaders";
    return (shaderDir / filename).string();
}

int main()
{
    srand((unsigned int)time(nullptr));

    // ------------------------------------------------------------------
    // 1. Inicializar GLFW
    // ------------------------------------------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4); // anti-aliasing

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT,
                                          "Practica 2 - Triangulo de Sierpinski", NULL, NULL);
    if (!window)
    {
        std::cout << "Error: no se pudo crear la ventana GLFW" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // ------------------------------------------------------------------
    // 2. Inicializar GLAD
    // ------------------------------------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Error: no se pudo inicializar GLAD" << std::endl;
        return -1;
    }

    // Habilitar gl_PointSize desde el vertex shader
    glEnable(GL_PROGRAM_POINT_SIZE);

    // ------------------------------------------------------------------
    // 3. Cargar shaders
    // ------------------------------------------------------------------
    std::string vertPath = getShaderPath("vertex_shader.glsl");
    std::string fragPath = getShaderPath("fragment_shader.glsl");
    Shader shader(vertPath.c_str(), fragPath.c_str());

    // ------------------------------------------------------------------
    // 4. Crear VAO y VBO
    //    Reservamos espacio para MAX_PUNTOS desde el inicio (GL_DYNAMIC_DRAW)
    //    y vamos llenando cada frame con glBufferSubData
    // ------------------------------------------------------------------
    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    // Reservar el buffer completo desde el principio (2 floats por punto: x, y)
    glBufferData(GL_ARRAY_BUFFER, MAX_PUNTOS * 2 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);

    // Atributo 0: posición (x, y)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // ------------------------------------------------------------------
    // 5. Estado del chaos game
    // ------------------------------------------------------------------
    std::vector<float> puntos;          // lista plana: x0, y0, x1, y1, ...
    puntos.reserve(MAX_PUNTOS * 2);

    // Punto inicial aleatorio dentro del triángulo (cualquier punto sirve)
    float px = ((float)rand() / RAND_MAX) * 1.8f - 0.9f;
    float py = ((float)rand() / RAND_MAX) * 1.7f - 0.8f;

    int puntosSubidos = 0; // cuántos puntos ya subimos al VBO

    std::cout << "Generando triangulo de Sierpinski..." << std::endl;
    std::cout << "Presiona ESC para salir." << std::endl;

    // ------------------------------------------------------------------
    // 6. Bucle de renderizado / animación
    // ------------------------------------------------------------------
    while (!glfwWindowShouldClose(window))
    {
        processInput(window);

        // --- Generar nuevos puntos este frame (chaos game) ---
        if ((int)puntos.size() / 2 < MAX_PUNTOS)
        {
            int generadosEstFrame = 0;
            while (generadosEstFrame < PUNTOS_X_FRAME &&
                   (int)puntos.size() / 2 < MAX_PUNTOS)
            {
                // Elegir un vértice al azar (0, 1 o 2)
                int v = rand() % 3;

                // Mover al punto medio entre la posición actual y el vértice elegido
                px = (px + VERTICES_TRIANGULO[v][0]) / 2.0f;
                py = (py + VERTICES_TRIANGULO[v][1]) / 2.0f;

                puntos.push_back(px);
                puntos.push_back(py);
                generadosEstFrame++;
            }

            // Subir solo los puntos nuevos al GPU con glBufferSubData
            int totalPuntos   = (int)puntos.size() / 2;
            int puntosNuevos  = totalPuntos - puntosSubidos;

            if (puntosNuevos > 0)
            {
                glBindBuffer(GL_ARRAY_BUFFER, VBO);
                glBufferSubData(
                    GL_ARRAY_BUFFER,
                    puntosSubidos * 2 * sizeof(float),           // offset en bytes
                    puntosNuevos  * 2 * sizeof(float),           // tamaño en bytes
                    puntos.data() + puntosSubidos * 2            // puntero al inicio de los nuevos
                );
                glBindBuffer(GL_ARRAY_BUFFER, 0);
                puntosSubidos = totalPuntos;
            }
        }

        // --- Renderizar ---
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        shader.use();
        glBindVertexArray(VAO);

        // Dibujar solo los puntos que ya existen
        if (puntosSubidos > 0)
            glDrawArrays(GL_POINTS, 0, puntosSubidos);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // ------------------------------------------------------------------
    // 7. Liberar recursos
    // ------------------------------------------------------------------
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    shader.del();

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}