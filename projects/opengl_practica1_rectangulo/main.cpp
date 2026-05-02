using namespace std;

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <filesystem>  // Para construir la ruta a los shaders de forma portátil

#include "shader.h"  // Nuestra clase Shader que lee los archivos .glsl

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);

// Configuración de la ventana
const unsigned int SCR_WIDTH  = 800;
const unsigned int SCR_HEIGHT = 600;

// __FILE__ es la ruta absoluta de este archivo en tiempo de compilación.
// Así encontramos la carpeta "shaders/" sin importar desde dónde se ejecute el programa.
static std::string getShaderPath(const std::string& filename)
{
    std::filesystem::path srcFile   = __FILE__;
    std::filesystem::path shaderDir = srcFile.parent_path() / "shaders";
    return (shaderDir / filename).string();
}

int main()
{
    // ------------------------------------------------------------------
    // 1. Pedir al usuario las coordenadas y dimensiones del rectángulo
    // ------------------------------------------------------------------
    float x, y, width, height;

    std::cout << "=== Practica 1: Rectangulo Coloreado ===" << std::endl;
    std::cout << "Las coordenadas van de -1.0 a 1.0 (espacio NDC de OpenGL)" << std::endl;
    std::cout << std::endl;

    std::cout << "Ingrese la coordenada X del vertice inferior izquierdo: ";
    std::cin >> x;

    std::cout << "Ingrese la coordenada Y del vertice inferior izquierdo: ";
    std::cin >> y;

    std::cout << "Ingrese el ancho del rectangulo: ";
    std::cin >> width;

    std::cout << "Ingrese la altura del rectangulo: ";
    std::cin >> height;

    // Calcular los 4 vértices a partir de la esquina inferior izquierda + dimensiones
    //
    //  3 (sup-izq) ----------- 2 (sup-der)
    //       |                       |
    //  0 (inf-izq) ----------- 1 (inf-der)
    //
    // Cada vértice tiene: posición (x, y, z) + color (r, g, b)
    float vertices[] = {
        // Posición (x, y, z)          Color (r, g, b)
        x,         y,          0.0f,   1.0f, 0.0f, 0.0f,  // 0: inf-izq  -> ROJO
        x + width, y,          0.0f,   0.0f, 1.0f, 0.0f,  // 1: inf-der  -> VERDE
        x + width, y + height, 0.0f,   0.0f, 0.0f, 1.0f,  // 2: sup-der  -> AZUL
        x,         y + height, 0.0f,   1.0f, 1.0f, 0.0f   // 3: sup-izq  -> AMARILLO
    };

    // EBO: dos triángulos que forman el rectángulo
    unsigned int indices[] = {
        0, 1, 2,  // primer triángulo  (inf-izq, inf-der, sup-der)
        0, 2, 3   // segundo triángulo (inf-izq, sup-der, sup-izq)
    };

    // ------------------------------------------------------------------
    // 2. Inicializar GLFW
    // ------------------------------------------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Practica 1 - Rectangulo Coloreado", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Error: no se pudo crear la ventana GLFW" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // ------------------------------------------------------------------
    // 3. Inicializar GLAD
    // ------------------------------------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Error: no se pudo inicializar GLAD" << std::endl;
        return -1;
    }

    // ------------------------------------------------------------------
    // 4. Cargar y compilar los shaders desde archivos externos.
    //    getShaderPath() construye la ruta absoluta usando la ubicación
    //    de main.cpp en tiempo de compilacion, asi funciona sin importar
    //    desde donde se ejecute el binario.
    // ------------------------------------------------------------------
    std::string vertPath = getShaderPath("vertex_shader.glsl");
    std::string fragPath = getShaderPath("fragment_shader.glsl");

    Shader shader(vertPath.c_str(), fragPath.c_str());

    // ------------------------------------------------------------------
    // 5. Crear VAO, VBO y EBO
    // ------------------------------------------------------------------
    unsigned int VAO, VBO, EBO;

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    // Subir vértices al VBO
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Subir índices al EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Atributo 0: posición  (3 floats, stride=6, offset=0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Atributo 1: color     (3 floats, stride=6, offset=3)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // ------------------------------------------------------------------
    // 6. Bucle de renderizado
    // ------------------------------------------------------------------
    while (!glfwWindowShouldClose(window))
    {
        processInput(window);

        glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        shader.use();
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // ------------------------------------------------------------------
    // 7. Liberar recursos
    // ------------------------------------------------------------------
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
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