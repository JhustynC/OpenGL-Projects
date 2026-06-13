using namespace std;

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <filesystem>
#include <cmath>

#include "shader.h"
#include "cube.h"

// ============================================================================
// PRACTICA: Cubo Coloreado y Animado - VERSION A
// ----------------------------------------------------------------------
// Todas las transformaciones (traslacion, rotacion, escala) se calculan
// DENTRO del vertex shader usando matrices mat4. La aplicacion solo envia
// los parametros (uniforms): angulo, eje de rotacion, desplazamiento,
// factores de escala.
//
// CONTROLES DE TECLADO:
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
    std::filesystem::path shaderDir = srcFile.parent_path() / "shaders_A";
    return (shaderDir / filename).string();
}

// ----------------------------------------------------------------------
// Estado global de la transformacion (modificado por teclado)
// ----------------------------------------------------------------------
float traslacionX = 0.0f, traslacionY = 0.0f, traslacionZ = 0.0f;
float escalaX = 1.0f, escalaY = 1.0f, escalaZ = 1.0f;
float anguloRotacion = 0.0f; // radianes, acumulado
int   ejeRotacion = 1;       // 0=X, 1=Y, 2=Z  (empieza rotando en Y)
float velocidadRotacion = 0.0f; // rad/seg, controlado con R / F

const float PASO_TRASLACION = 0.02f;
const float PASO_ESCALA     = 0.02f;
const float PASO_ROTACION   = 1.5f; // rad/seg al presionar R o F

// Pose isometrica inicial: para ver 3 caras del cubo desde una esquina,
// como en la imagen de referencia (cara superior, frontal y lateral
// derecha visibles simultaneamente).
const float ANGULO_INICIAL_X = 30.0f * 3.14159265f / 180.0f; // inclinar hacia abajo
const float ANGULO_INICIAL_Y = 45.0f * 3.14159265f / 180.0f; // girar para ver la cara lateral

int main()
{
    // ------------------------------------------------------------------
    // 1. GLFW + GLAD
    // ------------------------------------------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT,
                                          "Cubo Coloreado y Animado - Version A (shader)", NULL, NULL);
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

    glEnable(GL_DEPTH_TEST); // necesario para que el cubo se vea correctamente en 3D

    // ------------------------------------------------------------------
    // 2. Shaders
    // ------------------------------------------------------------------
    std::string vertPath = getShaderPath("vertex_shader.glsl");
    std::string fragPath = getShaderPath("fragment_shader.glsl");
    Shader shader(vertPath.c_str(), fragPath.c_str());

    // ------------------------------------------------------------------
    // 3. REPRESENTACION TOPOLOGICA -> GEOMETRICA
    //    El objeto Cube ya construyo positions[], colors[] e indices[]
    //    a partir de su topologia (caras -> vertices -> lista de vertices)
    // ------------------------------------------------------------------
    Cube cubo;

    // ------------------------------------------------------------------
    // 4. VAO + VBO (posiciones y colores en el MISMO buffer, regiones
    //    separadas, cargadas con glBufferSubData) + EBO
    // ------------------------------------------------------------------
    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    // --- VBO: reservar espacio total = posiciones + colores ---
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER,
                  cubo.positionsSizeBytes() + cubo.colorsSizeBytes(),
                  nullptr, GL_STATIC_DRAW);

    // Subir posiciones en el rango [0, positionsSizeBytes)
    glBufferSubData(GL_ARRAY_BUFFER, 0,
                     cubo.positionsSizeBytes(),
                     cubo.positions.data());

    // Subir colores en el rango [positionsSizeBytes, positionsSizeBytes + colorsSizeBytes)
    glBufferSubData(GL_ARRAY_BUFFER, cubo.positionsSizeBytes(),
                     cubo.colorsSizeBytes(),
                     cubo.colors.data());

    // --- EBO ---
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, cubo.indicesSizeBytes(),
                  cubo.indices.data(), GL_STATIC_DRAW);

    // --- Atributo 0: posicion (vec3), apunta al inicio del buffer ---
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // --- Atributo 1: color (vec4), apunta despues de todas las posiciones ---
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                           (void*)(cubo.positionsSizeBytes()));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // ------------------------------------------------------------------
    // 5. Matriz de proyeccion (perspectiva simple, fija)
    //    fov=45 grados, aspect=1, near=0.1, far=10
    // ------------------------------------------------------------------
    float fov    = 45.0f * 3.14159265f / 180.0f;
    float aspect = (float)SCR_WIDTH / (float)SCR_HEIGHT;
    float nearP  = 0.1f, farP = 10.0f;
    float f = 1.0f / tanf(fov / 2.0f);

    float projection[16] = {
        f / aspect, 0,  0,                                 0,
        0,          f,  0,                                 0,
        0,          0, (farP + nearP) / (nearP - farP),   -1,
        0,          0, (2 * farP * nearP) / (nearP - farP), 0
    };

    // Empujar el cubo hacia -Z para que quede frente a la "camara" (en origen)
    traslacionZ = -4.0f;

    std::cout << "\n" << std::endl;
    std::cout << "+----- CUBO COLOREADO Y ANIMADO - VERSION A (Shader) -----+" << std::endl;
    std::cout << "|                                                          |" << std::endl;
    std::cout << "| TRASLACION (flechas + Q/E):                             |" << std::endl;
    std::cout << "|   LEFT/RIGHT (izquierda/derecha) = eje X               |" << std::endl;
    std::cout << "|   UP/DOWN (arriba/abajo)           = eje Y               |" << std::endl;
    std::cout << "|   Q (acerca) / E (aleja)           = eje Z               |" << std::endl;
    std::cout << "|                                                          |" << std::endl;
    std::cout << "| ROTACION (X/Y/Z + R/F):                                |" << std::endl;
    std::cout << "|   X / Y / Z = selecciona eje de rotacion                |" << std::endl;
    std::cout << "|   R (horario) / F (antihorario)                         |" << std::endl;
    std::cout << "|                                                          |" << std::endl;
    std::cout << "| ESCALA (numeros + +/-):                                 |" << std::endl;
    std::cout << "|   1/2 = eje X   |   3/4 = eje Y   |   5/6 = eje Z       |" << std::endl;
    std::cout << "|   + (crecer) / - (encoger) en todas las dimensiones     |" << std::endl;
    std::cout << "|                                                          |" << std::endl;
    std::cout << "| ESC = salir                                              |" << std::endl;
    std::cout << "+----------------------------------------------------------+" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "[EJE ROTACION] Y (eje Y) [INICIAL]" << std::endl;
    std::cout << "" << std::endl;

    float lastTime = (float)glfwGetTime();

    // ------------------------------------------------------------------
    // 6. Render loop
    // ------------------------------------------------------------------
    while (!glfwWindowShouldClose(window))
    {
        float currentTime = (float)glfwGetTime();
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        processInput(window);

        // Acumular rotacion segun velocidad actual
        anguloRotacion += velocidadRotacion * deltaTime;

        glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();

        // Enviar los parametros de transformacion como uniforms
        glUniform3f(glGetUniformLocation(shader.ID, "uTraslacion"),
                    traslacionX, traslacionY, traslacionZ);
        glUniform3f(glGetUniformLocation(shader.ID, "uEscala"),
                    escalaX, escalaY, escalaZ);
        glUniform1f(glGetUniformLocation(shader.ID, "uAngulo"), anguloRotacion);
        glUniform1i(glGetUniformLocation(shader.ID, "uEjeRotacion"), ejeRotacion);
        glUniform1f(glGetUniformLocation(shader.ID, "uAnguloInicialX"), ANGULO_INICIAL_X);
        glUniform1f(glGetUniformLocation(shader.ID, "uAnguloInicialY"), ANGULO_INICIAL_Y);
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "uProjection"),
                           1, GL_FALSE, projection);

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

    // --- Traslacion ---
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) traslacionX += PASO_TRASLACION;
    if (glfwGetKey(window, GLFW_KEY_LEFT)  == GLFW_PRESS) traslacionX -= PASO_TRASLACION;
    if (glfwGetKey(window, GLFW_KEY_UP)    == GLFW_PRESS) traslacionY += PASO_TRASLACION;
    if (glfwGetKey(window, GLFW_KEY_DOWN)  == GLFW_PRESS) traslacionY -= PASO_TRASLACION;
    if (glfwGetKey(window, GLFW_KEY_E)     == GLFW_PRESS) traslacionZ += PASO_TRASLACION;
    if (glfwGetKey(window, GLFW_KEY_Q)     == GLFW_PRESS) traslacionZ -= PASO_TRASLACION;

    // --- Seleccion de eje de rotacion ---
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

    // --- Rotacion en ambos sentidos ---
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
        velocidadRotacion = PASO_ROTACION;
    else if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS)
        velocidadRotacion = -PASO_ROTACION;
    else
        velocidadRotacion = 0.0f;

    // --- Escala por eje ---
    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) escalaX += PASO_ESCALA;
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) escalaX = std::max(0.05f, escalaX - PASO_ESCALA);
    if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) escalaY += PASO_ESCALA;
    if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS) escalaY = std::max(0.05f, escalaY - PASO_ESCALA);
    if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS) escalaZ += PASO_ESCALA;
    if (glfwGetKey(window, GLFW_KEY_6) == GLFW_PRESS) escalaZ = std::max(0.05f, escalaZ - PASO_ESCALA);

    // --- Escala uniforme (las 3 dimensiones a la vez) ---
    if (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS) // tecla '+'
    {
        escalaX += PASO_ESCALA; escalaY += PASO_ESCALA; escalaZ += PASO_ESCALA;
    }
    if (glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS) // tecla '-'
    {
        escalaX = std::max(0.05f, escalaX - PASO_ESCALA);
        escalaY = std::max(0.05f, escalaY - PASO_ESCALA);
        escalaZ = std::max(0.05f, escalaZ - PASO_ESCALA);
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}