#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <array>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "shader.h"

const unsigned int WIDTH = 1100;
const unsigned int HEIGHT = 760;
const float PI = 3.14159265f;
const glm::vec3 CAMERA_POS(0.0f, -4.6f, 2.6f);

struct Mesh
{
    unsigned int VAO = 0;
    unsigned int VBO = 0;
    int vertexCount = 0;
};

std::string shaderPath(const std::string& name)
{
    std::filesystem::path source = __FILE__;
    return (source.parent_path() / "shaders" / name).string();
}

// Convierte los angulos esfericos en un punto de radio 1.
glm::vec3 sphericalPoint(float phi, float theta)
{
    return {
        cosf(phi) * cosf(theta),
        cosf(phi) * sinf(theta), 
        sinf(phi)
    };
}

// Crea VAO/VBO para vertices intercalados:
// x, y, z, nx, ny, nz.
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

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                          6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                          6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    return mesh;
}

// OpenGL no tiene una primitiva esfera: se aproxima con triangulos.
Mesh createSphere(int stacks, int slices)
{
    std::vector<float> vertices;

    auto addVertex = [&vertices](const glm::vec3& position)
    {
        glm::vec3 normal = glm::normalize(position);
        vertices.insert(vertices.end(), {
            position.x, position.y, position.z,
            normal.x, normal.y, normal.z
        });
    };

    // Cada stack es una franja entre phi0 y phi1.
    for (int stack = 0; stack < stacks; ++stack)
    {
        float phi0 = -PI * 0.5f + PI * stack / stacks;
        float phi1 = -PI * 0.5f + PI * (stack + 1) / stacks;

        // Cada slice es una seccion entre theta0 y theta1.
        for (int slice = 0; slice < slices; ++slice)
        {
            float theta0 = 2.0f * PI * slice / slices;
            float theta1 = 2.0f * PI * (slice + 1) / slices;

            glm::vec3 p0 = sphericalPoint(phi0, theta0);
            glm::vec3 p1 = sphericalPoint(phi0, theta1);
            glm::vec3 p2 = sphericalPoint(phi1, theta1);
            glm::vec3 p3 = sphericalPoint(phi1, theta0);

            // Un cuadrilatero curvo se aproxima con dos triangulos.
            addVertex(p0); addVertex(p1); addVertex(p2);
            addVertex(p2); addVertex(p3); addVertex(p0);
        }
    }

    return createMesh(vertices);
}

void deleteMesh(Mesh& mesh)
{
    glDeleteVertexArrays(1, &mesh.VAO);
    glDeleteBuffers(1, &mesh.VBO);
    mesh = {};
}

bool pressedOnce(GLFWwindow* window, int key,
                 std::array<bool, GLFW_KEY_LAST + 1>& previous)
{
    bool down = glfwGetKey(window, key) == GLFW_PRESS;
    bool result = down && !previous[key];
    previous[key] = down;
    return result;
}

void sendObject(const Shader& shader, const Mesh& mesh,
                const glm::mat4& model,
                const glm::mat4& view,
                const glm::mat4& projection,
                const glm::vec3& color,
                bool isSphere, int stacks, int slices)
{
    glm::mat4 mvp = projection * view * model;

    glUniformMatrix4fv(glGetUniformLocation(shader.ID, "uModel"),
                       1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(shader.ID, "uMVP"),
                       1, GL_FALSE, glm::value_ptr(mvp));
    glUniform3fv(glGetUniformLocation(shader.ID, "uColor"),
                 1, glm::value_ptr(color));
    glUniform1i(glGetUniformLocation(shader.ID, "uIsSphere"),
                isSphere ? 1 : 0);
    glUniform1i(glGetUniformLocation(shader.ID, "uStacks"), stacks);
    glUniform1i(glGetUniformLocation(shader.ID, "uSlices"), slices);

    glBindVertexArray(mesh.VAO);
    glDrawArrays(GL_TRIANGLES, 0, mesh.vertexCount);
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
    glfwWindowHint(GLFW_SAMPLES, 4);

    GLFWwindow* window = glfwCreateWindow(
        WIDTH, HEIGHT, "Practica: coordenadas esfericas", nullptr, nullptr
    );
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
    glEnable(GL_MULTISAMPLE);

    Shader shader(shaderPath("vertex_shader.glsl").c_str(),
                  shaderPath("fragment_shader.glsl").c_str());

    int stacks = 12;
    int slices = 18;
    Mesh sphere = createSphere(stacks, slices);

    float selectedPhi = 0.0f;
    float selectedTheta = PI * 1.5f;
    float rotationAngle = 0.0f;
    bool rotating = true;
    bool wireframe = false;
    std::array<bool, GLFW_KEY_LAST + 1> previousKeys{};

    glm::mat4 view = glm::lookAt(
        CAMERA_POS,
        glm::vec3(0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f)
    );
    glm::mat4 projection = glm::perspective(
        glm::radians(48.0f),
        static_cast<float>(WIDTH) / HEIGHT,
        0.1f,
        100.0f
    );

    std::cout
        << "PRACTICA DE COORDENADAS ESFERICAS\n"
        << "A/D: cambiar theta | W/S: cambiar phi\n"
        << "Z/X: menos/mas stacks | C/V: menos/mas slices\n"
        << "1: solido | 2: malla | SPACE: pausar giro | R: reiniciar\n";

    float previousTime = static_cast<float>(glfwGetTime());
    while (!glfwWindowShouldClose(window))
    {
        float currentTime = static_cast<float>(glfwGetTime());
        float dt = std::min(currentTime - previousTime, 0.033f);
        previousTime = currentTime;

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            selectedTheta -= 1.2f * dt;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            selectedTheta += 1.2f * dt;
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            selectedPhi += 1.0f * dt;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            selectedPhi -= 1.0f * dt;

        selectedPhi = std::clamp(selectedPhi, -PI * 0.5f, PI * 0.5f);
        selectedTheta = fmodf(selectedTheta, 2.0f * PI);
        if (selectedTheta < 0.0f)
            selectedTheta += 2.0f * PI;

        bool rebuild = false;
        if (pressedOnce(window, GLFW_KEY_Z, previousKeys) && stacks > 3)
        {
            --stacks;
            rebuild = true;
        }
        if (pressedOnce(window, GLFW_KEY_X, previousKeys) && stacks < 40)
        {
            ++stacks;
            rebuild = true;
        }
        if (pressedOnce(window, GLFW_KEY_C, previousKeys) && slices > 3)
        {
            --slices;
            rebuild = true;
        }
        if (pressedOnce(window, GLFW_KEY_V, previousKeys) && slices < 60)
        {
            ++slices;
            rebuild = true;
        }

        if (pressedOnce(window, GLFW_KEY_1, previousKeys))
            wireframe = false;
        if (pressedOnce(window, GLFW_KEY_2, previousKeys))
            wireframe = true;
        if (pressedOnce(window, GLFW_KEY_SPACE, previousKeys))
            rotating = !rotating;

        if (pressedOnce(window, GLFW_KEY_R, previousKeys))
        {
            stacks = 12;
            slices = 18;
            selectedPhi = 0.0f;
            selectedTheta = PI * 1.5f;
            rotationAngle = 0.0f;
            rotating = true;
            wireframe = false;
            rebuild = true;
        }

        if (rebuild)
        {
            deleteMesh(sphere);
            sphere = createSphere(stacks, slices);
        }

        if (rotating)
            rotationAngle += 0.45f * dt;

        glm::vec3 selectedPoint = sphericalPoint(selectedPhi, selectedTheta);

        std::ostringstream title;
        title << std::fixed << std::setprecision(2)
              << "Esfera | stacks " << stacks
              << " | slices " << slices
              << " | triangulos " << stacks * slices * 2
              << " | phi " << glm::degrees(selectedPhi)
              << " | theta " << glm::degrees(selectedTheta)
              << " | P=(" << selectedPoint.x << ","
              << selectedPoint.y << "," << selectedPoint.z << ")"
              << (wireframe ? " | MALLA" : " | SOLIDO");
        glfwSetWindowTitle(window, title.str().c_str());

        glClearColor(0.025f, 0.03f, 0.055f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();
        glm::vec3 lightPosition(2.8f, -3.2f, 3.6f);
        glUniform3fv(glGetUniformLocation(shader.ID, "uLightPos"),
                     1, glm::value_ptr(lightPosition));
        glUniform3fv(glGetUniformLocation(shader.ID, "uCameraPos"),
                     1, glm::value_ptr(CAMERA_POS));

        glm::mat4 rotation(1.0f);
        rotation = glm::rotate(rotation, glm::radians(18.0f),
                               glm::vec3(1.0f, 0.0f, 0.0f));
        rotation = glm::rotate(rotation, rotationAngle,
                               glm::vec3(0.0f, 0.0f, 1.0f));

        glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
        sendObject(shader, sphere, rotation, view, projection,
                   {0.12f, 0.58f, 0.95f}, true, stacks, slices);

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glm::mat4 markerModel = rotation;
        markerModel = glm::translate(markerModel, selectedPoint * 1.035f);
        markerModel = glm::scale(markerModel, glm::vec3(0.085f));
        sendObject(shader, sphere, markerModel, view, projection,
                   {1.0f, 0.16f, 0.08f}, false, stacks, slices);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    deleteMesh(sphere);
    shader.destroy();
    glfwTerminate();
    return 0;
}
