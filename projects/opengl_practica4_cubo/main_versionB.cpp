using namespace std;

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <filesystem>
#include <algorithm>

#include "shader.h"
#include "topologia.h"

// ============================================================================
// PRACTICA: Cubo Coloreado y Animado - VERSION B
// ----------------------------------------------------------------------
// Todas las transformaciones se calculan EN LA APLICACION usando GLM.
// Se construye una sola matriz MVP (Model * View * Projection) en C++
// y se envia al shader como un unico uniform mat4.
//
// CONTROLES (idénticos a la version A):
// +- TRASLACION (mover el cubo) -----------------------------------------------+
// |  Eje X:     LEFT/RIGHT (izquierda/derecha)                              |
// |  Eje Y:     UP/DOWN (arriba/abajo)                                      |
// |  Eje Z:     Q (acerca) / E (aleja)                                      |
// +- ROTACION (girar el cubo) -------------------------------------------------+
// |  1. Elige eje:  X (eje X) / Y (eje Y) / Z (eje Z)                       |
// |  2. Gira:       R (sentido horario) / F (sentido antihorario)           |
// +- ESCALA (cambiar tamaño) --------------------------------------------------+
// |  Individual por eje:                                                    |
// |    X: 1 (más grande) / 2 (más pequeño)                                 |
// |    Y: 3 (más grande) / 4 (más pequeño)                                 |
// |    Z: 5 (más grande) / 6 (más pequeño)                                 |
// |  Todas las dimensiones a la vez:                                       |
// |    + (crecer) / - (encoger)                                            |
// +- GENERAL -------------------------------------------------------------------+
// |  ESC: salir del programa                                                |
// +-----------------------------------------------------------------------------+
// ============================================================================

const unsigned int SCR_WIDTH  = 800;
const unsigned int SCR_HEIGHT = 800;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);

static std::string getShaderPath(const std::string& filename)
{
    std::filesystem::path srcFile   = __FILE__;
    std::filesystem::path shaderDir = srcFile.parent_path() / "shaders_B";
    return (shaderDir / filename).string();
}

// ----------------------------------------------------------------------
// Estado global de la transformacion (modificado por teclado)
// ----------------------------------------------------------------------
glm::vec3 traslacionVec(0.0f, 0.0f, -4.0f);
glm::vec3 escalaVec(1.0f, 1.0f, 1.0f);
float anguloRotacion = 0.0f; // radianes, acumulado
int   ejeRotacion = 1;       // 0=X, 1=Y, 2=Z
float velocidadRotacion = 0.0f;

const float PASO_TRASLACION = 0.02f;
const float PASO_ESCALA     = 0.02f;
const float PASO_ROTACION   = 1.5f;

// Pose isometrica inicial (vista de esquina, 3 caras visibles)
const float ANGULO_INICIAL_X = glm::radians(30.0f);
const float ANGULO_INICIAL_Y = glm::radians(45.0f);

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT,
                                          "Cubo Coloreado y Animado - Version B (GLM)", NULL, NULL);
    if (!window)
    {
        std::cout << "Error: no se pudo crear la ventana GLFW" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Error: no se pudo inicializar GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    // ------------------------------------------------------------------
    // Shaders
    // ------------------------------------------------------------------
    std::string vertPath = getShaderPath("vertex_shader.glsl");
    std::string fragPath = getShaderPath("fragment_shader.glsl");
    Shader shader(vertPath.c_str(), fragPath.c_str());

    // ------------------------------------------------------------------
    // REPRESENTACION TOPOLOGICA -> GEOMETRICA (igual que Version A)
    // ------------------------------------------------------------------
    Cube cubo;

    // ------------------------------------------------------------------
    // VAO + VBO (posiciones y colores via glBufferSubData) + EBO
    // ------------------------------------------------------------------
    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER,
                  cubo.positionsSizeBytes() + cubo.colorsSizeBytes(),
                  nullptr, GL_STATIC_DRAW);

    glBufferSubData(GL_ARRAY_BUFFER, 0,
                     cubo.positionsSizeBytes(),
                     cubo.positions.data());

    glBufferSubData(GL_ARRAY_BUFFER, cubo.positionsSizeBytes(),
                     cubo.colorsSizeBytes(),
                     cubo.colors.data());

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, cubo.indicesSizeBytes(),
                  cubo.indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                           (void*)(cubo.positionsSizeBytes()));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // ------------------------------------------------------------------
    // Proyeccion (fija) con GLM
    // ------------------------------------------------------------------
    glm::mat4 projection = glm::perspective(
        glm::radians(45.0f),
        (float)SCR_WIDTH / (float)SCR_HEIGHT,
        0.1f, 10.0f
    );

    std::cout << "\n" << std::endl;
    std::cout << "+----- CUBO COLOREADO Y ANIMADO - VERSION B (GLM) -----+" << std::endl;
    std::cout << "|                                                        |" << std::endl;
    std::cout << "| TRASLACION (flechas + Q/E):                           |" << std::endl;
    std::cout << "|   LEFT/RIGHT (izquierda/derecha) = eje X             |" << std::endl;
    std::cout << "|   UP/DOWN (arriba/abajo)           = eje Y             |" << std::endl;
    std::cout << "|   Q (acerca) / E (aleja)           = eje Z             |" << std::endl;
    std::cout << "|                                                        |" << std::endl;
    std::cout << "| ROTACION (X/Y/Z + R/F):                              |" << std::endl;
    std::cout << "|   X / Y / Z = selecciona eje de rotacion              |" << std::endl;
    std::cout << "|   R (horario) / F (antihorario)                       |" << std::endl;
    std::cout << "|                                                        |" << std::endl;
    std::cout << "| ESCALA (numeros + +/-):                               |" << std::endl;
    std::cout << "|   1/2 = eje X   |   3/4 = eje Y   |   5/6 = eje Z     |" << std::endl;
    std::cout << "|   + (crecer) / - (encoger) en todas las dimensiones   |" << std::endl;
    std::cout << "|                                                        |" << std::endl;
    std::cout << "| ESC = salir                                            |" << std::endl;
    std::cout << "+--------------------------------------------------------+" << std::endl;
    std::cout << "" << std::endl;    std::cout << "[EJE ROTACION] Y (eje Y) [INICIAL]" << std::endl;
    std::cout << "" << std::endl;
    float lastTime = (float)glfwGetTime();

    while (!glfwWindowShouldClose(window))
    {
        float currentTime = (float)glfwGetTime();
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        processInput(window);
        anguloRotacion += velocidadRotacion * deltaTime;

        glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // ----------------------------------------------------------
        // Construir la matriz de modelo con GLM:
        //   modelo = Traslacion * Rotacion * Escala
        // ----------------------------------------------------------
        glm::mat4 modelo = glm::mat4(1.0f); // matriz identidad

        modelo = glm::translate(modelo, traslacionVec);

        // Pose isometrica fija: se aplica ANTES de la rotacion interactiva,
        // dando la vista "de esquina" donde se ven 3 caras del cubo.
        modelo = glm::rotate(modelo, ANGULO_INICIAL_X, glm::vec3(1.0f, 0.0f, 0.0f));
        modelo = glm::rotate(modelo, ANGULO_INICIAL_Y, glm::vec3(0.0f, 1.0f, 0.0f));

        glm::vec3 eje(0.0f);
        if (ejeRotacion == 0) eje = glm::vec3(1.0f, 0.0f, 0.0f);
        else if (ejeRotacion == 1) eje = glm::vec3(0.0f, 1.0f, 0.0f);
        else eje = glm::vec3(0.0f, 0.0f, 1.0f);

        modelo = glm::rotate(modelo, anguloRotacion, eje);
        modelo = glm::scale(modelo, escalaVec);

        // MVP = Projection * View * Model  (View = identidad, camara fija en origen)
        glm::mat4 mvp = projection * modelo;

        shader.use();
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "uMVP"),
                           1, GL_FALSE, glm::value_ptr(mvp));

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, cubo.indexCount(), GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    shader.del();

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow* window)
{
    static int ejeRotacionAnterior = 1; // Variable estática para rastrear cambios

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) traslacionVec.x += PASO_TRASLACION;
    if (glfwGetKey(window, GLFW_KEY_LEFT)  == GLFW_PRESS) traslacionVec.x -= PASO_TRASLACION;
    if (glfwGetKey(window, GLFW_KEY_UP)    == GLFW_PRESS) traslacionVec.y += PASO_TRASLACION;
    if (glfwGetKey(window, GLFW_KEY_DOWN)  == GLFW_PRESS) traslacionVec.y -= PASO_TRASLACION;
    if (glfwGetKey(window, GLFW_KEY_E)     == GLFW_PRESS) traslacionVec.z += PASO_TRASLACION;
    if (glfwGetKey(window, GLFW_KEY_Q)     == GLFW_PRESS) traslacionVec.z -= PASO_TRASLACION;

    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) {
        ejeRotacion = 0;
        if (ejeRotacion != ejeRotacionAnterior) {
            std::cout << "[EJE ROTACION] X (eje X)" << std::endl;
            ejeRotacionAnterior = ejeRotacion;
        }
    }
    if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS) {
        ejeRotacion = 1;
        if (ejeRotacion != ejeRotacionAnterior) {
            std::cout << "[EJE ROTACION] Y (eje Y)" << std::endl;
            ejeRotacionAnterior = ejeRotacion;
        }
    }
    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
        ejeRotacion = 2;
        if (ejeRotacion != ejeRotacionAnterior) {
            std::cout << "[EJE ROTACION] Z (eje Z)" << std::endl;
            ejeRotacionAnterior = ejeRotacion;
        }
    }

    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
        velocidadRotacion = PASO_ROTACION;
    else if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS)
        velocidadRotacion = -PASO_ROTACION;
    else
        velocidadRotacion = 0.0f;

    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) escalaVec.x += PASO_ESCALA;
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) escalaVec.x = std::max(0.05f, escalaVec.x - PASO_ESCALA);
    if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) escalaVec.y += PASO_ESCALA;
    if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS) escalaVec.y = std::max(0.05f, escalaVec.y - PASO_ESCALA);
    if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS) escalaVec.z += PASO_ESCALA;
    if (glfwGetKey(window, GLFW_KEY_6) == GLFW_PRESS) escalaVec.z = std::max(0.05f, escalaVec.z - PASO_ESCALA);

    if (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS)
        escalaVec += glm::vec3(PASO_ESCALA);

    if (glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS)
    {
        escalaVec.x = std::max(0.05f, escalaVec.x - PASO_ESCALA);
        escalaVec.y = std::max(0.05f, escalaVec.y - PASO_ESCALA);
        escalaVec.z = std::max(0.05f, escalaVec.z - PASO_ESCALA);
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}