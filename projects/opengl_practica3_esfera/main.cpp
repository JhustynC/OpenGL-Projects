using namespace std;

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>
#include <cmath>
#include <filesystem>

#include "shader.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);

const unsigned int SCR_WIDTH  = 800;
const unsigned int SCR_HEIGHT = 600;

const float PI = 3.14159265359f;

static std::string getShaderPath(const std::string& filename)
{
    std::filesystem::path srcFile   = __FILE__;
    std::filesystem::path shaderDir = srcFile.parent_path() / "shaders";
    return (shaderDir / filename).string();
}

struct Figura
{
    unsigned int VAO;
    unsigned int VBO;
    int cantidadVertices;
    GLenum primitiva;
};

float radio = 0.7f;
int paralelos = 10;
int meridianos = 20;

vector<float> generarPuntoEsfera(float r, float theta, float phi)
{
    float x = r * sin(theta) * cos(phi);
    float y = r * sin(theta) * sin(phi);
    float z = r * cos(theta);

    return { x, y, z };
}

Figura crearFigura(const vector<float>& vertices, GLenum primitiva)
{
    Figura figura;

    figura.cantidadVertices = vertices.size() / 3;
    figura.primitiva = primitiva;

    glGenVertexArrays(1, &figura.VAO);
    glGenBuffers(1, &figura.VBO);

    glBindVertexArray(figura.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, figura.VBO);
    glBufferData(
        GL_ARRAY_BUFFER,
        vertices.size() * sizeof(float),
        vertices.data(),
        GL_STATIC_DRAW
    );

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return figura;
}

void agregarPunto(vector<float>& vertices, float r, float theta, float phi)
{
    vector<float> p = generarPuntoEsfera(r, theta, phi);

    vertices.push_back(p[0]);
    vertices.push_back(p[1]);
    vertices.push_back(p[2]);
}

int main()
{
    cout << "=== Practica 3: Generacion de una Esfera ===" << endl;
    cout << "Modelo de alambre con paralelos, meridianos y polos." << endl;

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(
        SCR_WIDTH,
        SCR_HEIGHT,
        "Practica 3 - Esfera",
        NULL,
        NULL
    );

    if (window == NULL)
    {
        cout << "Error: no se pudo crear la ventana GLFW" << endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        cout << "Error: no se pudo inicializar GLAD" << endl;
        return -1;
    }

    string vertPath = getShaderPath("vertex_shader.glsl");
    string fragPath = getShaderPath("fragment_shader.glsl");

    Shader shader(vertPath.c_str(), fragPath.c_str());

    vector<Figura> figuras;

    // ============================
    // 1. Paralelos - GL_LINE_LOOP
    // ============================
    for (int i = 1; i < paralelos; i++)
    {
        float theta = PI * i / paralelos;

        vector<float> verticesParalelo;

        for (int j = 0; j < meridianos; j++)
        {
            float phi = 2.0f * PI * j / meridianos;
            agregarPunto(verticesParalelo, radio, theta, phi);
        }

        figuras.push_back(crearFigura(verticesParalelo, GL_LINE_LOOP));
    }

    // ============================
    // 2. Meridianos - GL_LINE_STRIP
    // ============================
    for (int j = 0; j < meridianos; j++)
    {
        float phi = 2.0f * PI * j / meridianos;

        vector<float> verticesMeridiano;

        for (int i = 0; i <= paralelos; i++)
        {
            float theta = PI * i / paralelos;
            agregarPunto(verticesMeridiano, radio, theta, phi);
        }

        figuras.push_back(crearFigura(verticesMeridiano, GL_LINE_STRIP));
    }

    // ============================
    // 3. Polo norte - GL_TRIANGLE_FAN
    // ============================
    vector<float> poloNorte;

    poloNorte.push_back(0.0f);
    poloNorte.push_back(0.0f);
    poloNorte.push_back(radio);

    float thetaNorte = PI / paralelos;

    for (int j = 0; j <= meridianos; j++)
    {
        float phi = 2.0f * PI * j / meridianos;
        agregarPunto(poloNorte, radio, thetaNorte, phi);
    }

    figuras.push_back(crearFigura(poloNorte, GL_TRIANGLE_FAN));

    // ============================
    // 4. Polo sur - GL_TRIANGLE_FAN
    // ============================
    vector<float> poloSur;

    poloSur.push_back(0.0f);
    poloSur.push_back(0.0f);
    poloSur.push_back(-radio);

    float thetaSur = PI * (paralelos - 1) / paralelos;

    for (int j = 0; j <= meridianos; j++)
    {
        float phi = 2.0f * PI * j / meridianos;
        agregarPunto(poloSur, radio, thetaSur, phi);
    }

    figuras.push_back(crearFigura(poloSur, GL_TRIANGLE_FAN));

    glEnable(GL_DEPTH_TEST);

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    while (!glfwWindowShouldClose(window))
    {
        processInput(window);

        glClearColor(0.12f, 0.12f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();

        for (Figura& figura : figuras)
        {
            glBindVertexArray(figura.VAO);
            glDrawArrays(figura.primitiva, 0, figura.cantidadVertices);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    for (Figura& figura : figuras)
    {
        glDeleteVertexArrays(1, &figura.VAO);
        glDeleteBuffers(1, &figura.VBO);
    }

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