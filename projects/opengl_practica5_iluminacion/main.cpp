using namespace std;

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <filesystem>
#include <cmath>
#include <vector>
#include <algorithm>
#include <string>

#include "shader.h"
#include "sphere.h"

// ============================================================================
// PRACTICA: Esfera Iluminada - Modelo de Phong
// ----------------------------------------------------------------------------
// Usa 1/2 o ESPACIO para alternar entre Version A y Version B.
//
// VERSION A (true):  calculos de iluminacion por CARA en C++.
//                    El color final se envia al shader como atributo.
//
// VERSION B (false): calculos de iluminacion en el vertex shader.
//                    Se usan normales de VERTICE = posicion normalizada.
//
// CONTROLES:
//   1 / 2 / ESPACIO Alternar Version A/B
//   N / M           Aumentar / disminuir aproximacion de la esfera
//   Flechas / Q-E   Trasladar la esfera
//   X / Y / Z       Seleccionar eje de rotacion
//   R / F           Rotar en sentido +/-
//   +  / -          Acercar / alejar la fuente de luz
//   ESC             Salir
// ============================================================================

// ---- Configuracion ----
const int  MAX_SUBDIVISIONES = 5;  // 0=tetraedro, 5=muy suave

// ---- Ventana ----
const unsigned int SCR_WIDTH  = 800;
const unsigned int SCR_HEIGHT = 800;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);

static std::string getShaderPath(const std::string& filename, bool versionA)
{
    std::filesystem::path srcFile   = __FILE__;
    std::string carpeta = versionA ? "shaders_A" : "shaders_B";
    return (srcFile.parent_path() / carpeta / filename).string();
}

// ---- Estado global de transformacion ----
float traslacionX = 0.0f, traslacionY = 0.0f, traslacionZ = -3.5f;
float escalaUniforme = 1.0f;
float anguloRotacion = 0.0f;
int   ejeRotacion    = 1;   // 0=X, 1=Y, 2=Z
float velocidadRot   = 0.0f;
bool  usarVersionA   = true;
bool  cambioVersionBloqueado = false;
bool  cambioDetalleBloqueado = false;
int   nivelSubdivision = MAX_SUBDIVISIONES;

// Posicion de la luz (se puede acercar/alejar con + y -)
float luzZ = -2.0f;

const float PASO_TRASLACION = 0.02f;
const float PASO_ROT        = 1.2f;  // rad/seg
const float LUZ_X           = 0.75f;
const float LUZ_Y           = 0.75f;

static std::string getWindowTitle(bool versionA, int subdivision)
{
    std::string version = versionA
        ? "Esfera Iluminada - Version A (Phong por cara en C++)"
        : "Esfera Iluminada - Version B (Gouraud en vertex shader)";
    return version + " - Nivel " + std::to_string(subdivision);
}

// ---- Proyeccion perspectiva (sin GLM) ----
static void buildPerspective(float* m, float fovDeg, float aspect, float near, float far)
{
    float f = 1.0f / tanf(fovDeg * 3.14159265f / 360.0f);
    for (int i = 0; i < 16; i++) m[i] = 0.0f;
    m[0]  = f / aspect;
    m[5]  = f;
    m[10] = (far + near) / (near - far);
    m[11] = -1.0f;
    m[14] = (2.0f * far * near) / (near - far);
}

// ---- Matriz de rotacion (manual, un solo eje) ----
static void buildRotation(float* m, float angle, int eje)
{
    float c = cosf(angle), s = sinf(angle);
    for (int i = 0; i < 16; i++) m[i] = 0.0f;
    m[15] = 1.0f;
    if (eje == 0) { // X
        m[0]=1; m[5]=c; m[6]=s; m[9]=-s; m[10]=c;
    } else if (eje == 1) { // Y
        m[0]=c; m[2]=-s; m[5]=1; m[8]=s; m[10]=c;
    } else { // Z
        m[0]=c; m[1]=s; m[4]=-s; m[5]=c; m[10]=1;
    }
}

// ---- Matriz de traslacion ----
static void buildTranslation(float* m, float tx, float ty, float tz)
{
    for (int i = 0; i < 16; i++) m[i] = 0.0f;
    m[0]=1; m[5]=1; m[10]=1; m[15]=1;
    m[12]=tx; m[13]=ty; m[14]=tz;
}

// ---- Matriz de escala uniforme ----
static void buildScale(float* m, float s)
{
    for (int i = 0; i < 16; i++) m[i] = 0.0f;
    m[0]=s; m[5]=s; m[10]=s; m[15]=1;
}

// ---- Multiplicacion de matrices 4x4 (column-major) ----
static void matMul(float* out, const float* A, const float* B)
{
    float tmp[16] = {};
    for (int col = 0; col < 4; col++)
        for (int row = 0; row < 4; row++)
            for (int k = 0; k < 4; k++)
                tmp[col*4+row] += A[k*4+row] * B[col*4+k];
    for (int i = 0; i < 16; i++) out[i] = tmp[i];
}

static float dot3(const float* a, const float* b)
{
    return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}

static void normalize3(float* v)
{
    float len = sqrtf(dot3(v, v));
    if (len > 0.0f)
    {
        v[0] /= len;
        v[1] /= len;
        v[2] /= len;
    }
}

static void transformPoint(const float* m, const float* p, float* out)
{
    out[0] = m[0]*p[0] + m[4]*p[1] + m[8] *p[2] + m[12];
    out[1] = m[1]*p[0] + m[5]*p[1] + m[9] *p[2] + m[13];
    out[2] = m[2]*p[0] + m[6]*p[1] + m[10]*p[2] + m[14];
}

static void transformDirection(const float* m, const float* v, float* out)
{
    out[0] = m[0]*v[0] + m[4]*v[1] + m[8] *v[2];
    out[1] = m[1]*v[0] + m[5]*v[1] + m[9] *v[2];
    out[2] = m[2]*v[0] + m[6]*v[1] + m[10]*v[2];
    normalize3(out);
}

static void calculateFaceLightingColors(
    std::vector<float>& colors,
    const Sphere& esfera,
    const float* model,
    const float* lightPos,
    const float* lightAmbient,
    const float* lightDiffuse,
    const float* lightSpecular,
    const float* matAmbient,
    const float* matDiffuse,
    const float* matSpecular,
    float matShininess,
    float attC0,
    float attC1,
    float attC2,
    const float* cameraPos)
{
    colors.assign(esfera.vertexCount() * 3, 0.0f);

    for (int base = 0; base < esfera.vertexCount(); base += 3)
    {
        float p[3][3];
        for (int i = 0; i < 3; i++)
        {
            const float* localPos = &esfera.positions[(base + i) * 3];
            transformPoint(model, localPos, p[i]);
        }

        float faceCenter[3] = {
            (p[0][0] + p[1][0] + p[2][0]) / 3.0f,
            (p[0][1] + p[1][1] + p[2][1]) / 3.0f,
            (p[0][2] + p[1][2] + p[2][2]) / 3.0f
        };

        float N[3];
        transformDirection(model, &esfera.normals[base * 3], N);

        float L[3] = {
            lightPos[0] - faceCenter[0],
            lightPos[1] - faceCenter[1],
            lightPos[2] - faceCenter[2]
        };
        float distancia = sqrtf(dot3(L, L));
        normalize3(L);

        float V[3] = {
            cameraPos[0] - faceCenter[0],
            cameraPos[1] - faceCenter[1],
            cameraPos[2] - faceCenter[2]
        };
        normalize3(V);

        float ln = dot3(L, N);
        float R[3] = {
            2.0f * ln * N[0] - L[0],
            2.0f * ln * N[1] - L[1],
            2.0f * ln * N[2] - L[2]
        };
        normalize3(R);

        float atenuacion = 1.0f / (attC0 + attC1 * distancia + attC2 * distancia * distancia);
        float diffFactor = std::max(ln, 0.0f);
        float specFactor = 0.0f;
        if (diffFactor > 0.0f)
            specFactor = powf(std::max(dot3(R, V), 0.0f), matShininess);

        float color[3];
        for (int c = 0; c < 3; c++)
        {
            float ambiental = matAmbient[c] * lightAmbient[c];
            float difusa = matDiffuse[c] * lightDiffuse[c] * diffFactor;
            float especular = matSpecular[c] * lightSpecular[c] * specFactor;
            color[c] = std::clamp(ambiental + atenuacion * (difusa + especular), 0.0f, 1.0f);
        }

        for (int i = 0; i < 3; i++)
        {
            colors[(base + i) * 3 + 0] = color[0];
            colors[(base + i) * 3 + 1] = color[1];
            colors[(base + i) * 3 + 2] = color[2];
        }
    }
}

struct GpuMesh
{
    unsigned int VAO = 0;
    unsigned int VBO = 0;
    int vertexCount = 0;
    size_t colorOffset = 0;
    size_t colorBytes = 0;
    std::vector<float> colors;
};

static void createSphereMesh(GpuMesh& mesh, const Sphere& esfera, bool withColors)
{
    mesh.vertexCount = esfera.vertexCount();
    mesh.colorOffset = esfera.positionsSizeBytes() + esfera.normalsSizeBytes();
    mesh.colorBytes = withColors ? esfera.vertexCount() * 3 * sizeof(float) : 0;

    glGenVertexArrays(1, &mesh.VAO);
    glGenBuffers(1, &mesh.VBO);

    glBindVertexArray(mesh.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);

    size_t totalBytes = esfera.positionsSizeBytes() + esfera.normalsSizeBytes() + mesh.colorBytes;
    glBufferData(GL_ARRAY_BUFFER, totalBytes, nullptr,
                 withColors ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);

    glBufferSubData(GL_ARRAY_BUFFER, 0,
                    esfera.positionsSizeBytes(),
                    esfera.positions.data());

    glBufferSubData(GL_ARRAY_BUFFER, esfera.positionsSizeBytes(),
                    esfera.normalsSizeBytes(),
                    esfera.normals.data());

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float),
                          (void*)(esfera.positionsSizeBytes()));
    glEnableVertexAttribArray(1);

    if (withColors)
    {
        mesh.colors.assign(esfera.vertexCount() * 3, 0.0f);
        glBufferSubData(GL_ARRAY_BUFFER, mesh.colorOffset,
                        mesh.colorBytes,
                        mesh.colors.data());

        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float),
                              (void*)mesh.colorOffset);
        glEnableVertexAttribArray(2);
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

static void createLightMesh(GpuMesh& mesh, const Sphere& esfera, const float* color)
{
    mesh.vertexCount = esfera.vertexCount();
    mesh.colorOffset = esfera.positionsSizeBytes();
    mesh.colorBytes = esfera.vertexCount() * 3 * sizeof(float);
    mesh.colors.assign(esfera.vertexCount() * 3, 0.0f);

    for (int i = 0; i < esfera.vertexCount(); i++)
    {
        mesh.colors[i * 3 + 0] = color[0];
        mesh.colors[i * 3 + 1] = color[1];
        mesh.colors[i * 3 + 2] = color[2];
    }

    glGenVertexArrays(1, &mesh.VAO);
    glGenBuffers(1, &mesh.VBO);

    glBindVertexArray(mesh.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);

    glBufferData(GL_ARRAY_BUFFER,
                 esfera.positionsSizeBytes() + mesh.colorBytes,
                 nullptr,
                 GL_STATIC_DRAW);

    glBufferSubData(GL_ARRAY_BUFFER, 0,
                    esfera.positionsSizeBytes(),
                    esfera.positions.data());

    glBufferSubData(GL_ARRAY_BUFFER, mesh.colorOffset,
                    mesh.colorBytes,
                    mesh.colors.data());

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float),
                          (void*)mesh.colorOffset);
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

static void deleteMesh(GpuMesh& mesh)
{
    if (mesh.VAO) glDeleteVertexArrays(1, &mesh.VAO);
    if (mesh.VBO) glDeleteBuffers(1, &mesh.VBO);
    mesh.VAO = 0;
    mesh.VBO = 0;
}

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

    std::string tituloInicial = getWindowTitle(usarVersionA, nivelSubdivision);
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, tituloInicial.c_str(), NULL, NULL);
    if (!window) { std::cout << "Error GLFW" << std::endl; glfwTerminate(); return -1; }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    { std::cout << "Error GLAD" << std::endl; return -1; }

    glEnable(GL_DEPTH_TEST);

    // ------------------------------------------------------------------
    // 2. Shaders
    // ------------------------------------------------------------------
    std::string vertPathA = getShaderPath("vertex_shader.glsl", true);
    std::string fragPathA = getShaderPath("fragment_shader.glsl", true);
    std::string vertPathB = getShaderPath("vertex_shader.glsl", false);
    std::string fragPathB = getShaderPath("fragment_shader.glsl", false);
    Shader shaderA(vertPathA.c_str(), fragPathA.c_str());
    Shader shaderB(vertPathB.c_str(), fragPathB.c_str());

    // ------------------------------------------------------------------
    // 3. Generar geometria de la esfera
    //    Sphere(subdivisiones, flatNormals)
    //      flatNormals=true  -> Version A: normal de cara
    //      flatNormals=false -> Version B: normal de vertice = posicion
    // ------------------------------------------------------------------
    std::vector<Sphere> esferasA;
    std::vector<Sphere> esferasB;
    esferasA.reserve(MAX_SUBDIVISIONES + 1);
    esferasB.reserve(MAX_SUBDIVISIONES + 1);

    for (int nivel = 0; nivel <= MAX_SUBDIVISIONES; nivel++)
    {
        esferasA.emplace_back(nivel, true);
        esferasB.emplace_back(nivel, false);
    }

    Sphere marcadorLuz(1, false);

    std::cout << "Niveles de aproximacion generados:" << std::endl;
    for (int nivel = 0; nivel <= MAX_SUBDIVISIONES; nivel++)
    {
        std::cout << "  Nivel " << nivel << ": " << esferasA[nivel].vertexCount() << " vertices ("
                  << esferasA[nivel].vertexCount() / 3 << " triangulos)" << std::endl;
    }

    // ------------------------------------------------------------------
    // 4. VAO + VBO (posiciones y normales en regiones separadas
    //    con glBufferSubData, como pide el docente) + sin EBO
    //    (cada triangulo tiene sus 3 vertices propios)
    // ------------------------------------------------------------------
    std::vector<GpuMesh> meshesA(MAX_SUBDIVISIONES + 1);
    std::vector<GpuMesh> meshesB(MAX_SUBDIVISIONES + 1);
    GpuMesh meshLuz;
    float lightMarkerColor[3] = { 1.0f, 0.95f, 0.15f };
    for (int nivel = 0; nivel <= MAX_SUBDIVISIONES; nivel++)
    {
        createSphereMesh(meshesA[nivel], esferasA[nivel], true);
        createSphereMesh(meshesB[nivel], esferasB[nivel], false);
    }
    createLightMesh(meshLuz, marcadorLuz, lightMarkerColor);

    // ------------------------------------------------------------------
    // 5. Parametros de iluminacion (Phong)
    // ------------------------------------------------------------------

    // Fuente de luz: blanca, visible en pantalla y delante de la esfera
    float lightPos[3]      = { LUZ_X, LUZ_Y, luzZ };
    float lightAmbient[3]  = { 0.15f, 0.15f, 0.15f };
    float lightDiffuse[3]  = { 1.0f,  1.0f,  1.0f  };
    float lightSpecular[3] = { 1.0f,  1.0f,  1.0f  };

    // Material: esfera azul con brillo especular blanco
    float matAmbient[3]    = { 0.05f, 0.10f, 0.20f };
    float matDiffuse[3]    = { 0.20f, 0.50f, 0.80f };
    float matSpecular[3]   = { 0.80f, 0.80f, 0.80f };
    float matShininess     = 64.0f;

    // Atenuacion: f = 1 / (c0 + c1*d + c2*d^2)
    float attC0 = 1.0f;  // constante (sin atenuacion base)
    float attC1 = 0.05f; // lineal
    float attC2 = 0.01f; // cuadratico

    // Camara fija en el origen mirando hacia -Z
    float cameraPos[3] = { 0.0f, 0.0f, 0.0f };

    // ------------------------------------------------------------------
    // 6. Matrices
    // ------------------------------------------------------------------
    float mProjection[16], mTranslation[16], mRotation[16], mModel[16];
    float mLightTranslation[16], mLightScale[16], mLightModel[16];
    buildPerspective(mProjection, 45.0f,
                     (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 20.0f);

    std::cout << "Version inicial: A (calculos por cara en C++)" << std::endl;
    std::cout << "Nivel inicial: " << nivelSubdivision << " (esfera mas suave)" << std::endl;
    std::cout << "1/2/Espacio: cambiar A-B | N/M: aumentar-disminuir aproximacion | Flechas/Q-E: mover | X/Y/Z: eje rot | R/F: rotar | +/-: luz | ESC: salir" << std::endl;

    float lastTime = (float)glfwGetTime();
    bool versionAnterior = usarVersionA;
    int nivelAnterior = nivelSubdivision;

    // ------------------------------------------------------------------
    // 7. Render loop
    // ------------------------------------------------------------------
    while (!glfwWindowShouldClose(window))
    {
        float now  = (float)glfwGetTime();
        float dt   = now - lastTime;
        lastTime   = now;

        processInput(window);
        anguloRotacion += velocidadRot * dt;

        if (usarVersionA != versionAnterior || nivelSubdivision != nivelAnterior)
        {
            std::string tituloActual = getWindowTitle(usarVersionA, nivelSubdivision);
            glfwSetWindowTitle(window, tituloActual.c_str());
            std::cout << (usarVersionA
                ? "Version A: calculos por cara en C++"
                : "Version B: calculos en el vertex shader")
                << " | Nivel " << nivelSubdivision << " ("
                << esferasA[nivelSubdivision].vertexCount() / 3 << " triangulos)" << std::endl;
            versionAnterior = usarVersionA;
            nivelAnterior = nivelSubdivision;
        }

        // Actualizar posicion de la luz (en Z)
        lightPos[0] = LUZ_X; lightPos[1] = LUZ_Y; lightPos[2] = luzZ;

        // Construir matriz de modelo: Traslacion * Rotacion
        buildTranslation(mTranslation, traslacionX, traslacionY, traslacionZ);
        buildRotation(mRotation, anguloRotacion, ejeRotacion);
        matMul(mModel, mTranslation, mRotation);

        glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        Shader& shader = usarVersionA ? shaderA : shaderB;
        GpuMesh& mesh = usarVersionA ? meshesA[nivelSubdivision] : meshesB[nivelSubdivision];

        shader.use();

        // Matrices
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "uModel"),
                           1, GL_FALSE, mModel);
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "uProjection"),
                           1, GL_FALSE, mProjection);

        if (usarVersionA)
        {
            GpuMesh& meshA = meshesA[nivelSubdivision];
            Sphere& esferaA = esferasA[nivelSubdivision];

            calculateFaceLightingColors(meshA.colors, esferaA, mModel,
                                        lightPos, lightAmbient, lightDiffuse, lightSpecular,
                                        matAmbient, matDiffuse, matSpecular, matShininess,
                                        attC0, attC1, attC2, cameraPos);

            glBindBuffer(GL_ARRAY_BUFFER, meshA.VBO);
            glBufferSubData(GL_ARRAY_BUFFER, meshA.colorOffset,
                            meshA.colorBytes,
                            meshA.colors.data());
        }
        else
        {
            // Luz
            glUniform3fv(glGetUniformLocation(shader.ID, "uLightPos"),      1, lightPos);
            glUniform3fv(glGetUniformLocation(shader.ID, "uLightAmbient"),  1, lightAmbient);
            glUniform3fv(glGetUniformLocation(shader.ID, "uLightDiffuse"),  1, lightDiffuse);
            glUniform3fv(glGetUniformLocation(shader.ID, "uLightSpecular"), 1, lightSpecular);

            // Material
            glUniform3fv(glGetUniformLocation(shader.ID, "uMatAmbient"),   1, matAmbient);
            glUniform3fv(glGetUniformLocation(shader.ID, "uMatDiffuse"),   1, matDiffuse);
            glUniform3fv(glGetUniformLocation(shader.ID, "uMatSpecular"),  1, matSpecular);
            glUniform1f (glGetUniformLocation(shader.ID, "uMatShininess"),   matShininess);

            // Atenuacion
            glUniform1f(glGetUniformLocation(shader.ID, "uAttC0"), attC0);
            glUniform1f(glGetUniformLocation(shader.ID, "uAttC1"), attC1);
            glUniform1f(glGetUniformLocation(shader.ID, "uAttC2"), attC2);

            // Camara
            glUniform3fv(glGetUniformLocation(shader.ID, "uCameraPos"), 1, cameraPos);
        }

        glBindVertexArray(mesh.VAO);
        glDrawArrays(GL_TRIANGLES, 0, mesh.vertexCount);

        buildTranslation(mLightTranslation, lightPos[0], lightPos[1], lightPos[2]);
        buildScale(mLightScale, 0.08f);
        matMul(mLightModel, mLightTranslation, mLightScale);

        shaderA.use();
        glUniformMatrix4fv(glGetUniformLocation(shaderA.ID, "uModel"),
                           1, GL_FALSE, mLightModel);
        glUniformMatrix4fv(glGetUniformLocation(shaderA.ID, "uProjection"),
                           1, GL_FALSE, mProjection);
        glBindVertexArray(meshLuz.VAO);
        glDrawArrays(GL_TRIANGLES, 0, meshLuz.vertexCount);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // ------------------------------------------------------------------
    // 8. Liberar recursos
    // ------------------------------------------------------------------
    for (int nivel = 0; nivel <= MAX_SUBDIVISIONES; nivel++)
    {
        deleteMesh(meshesA[nivel]);
        deleteMesh(meshesB[nivel]);
    }
    deleteMesh(meshLuz);
    shaderA.del();
    shaderB.del();
    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) usarVersionA = true;
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) usarVersionA = false;

    bool espacioPresionado = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
    if (espacioPresionado && !cambioVersionBloqueado)
    {
        usarVersionA = !usarVersionA;
        cambioVersionBloqueado = true;
    }
    if (!espacioPresionado)
        cambioVersionBloqueado = false;

    bool subirDetalle = glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS;
    bool bajarDetalle = glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS;
    if (!cambioDetalleBloqueado)
    {
        if (subirDetalle && nivelSubdivision < MAX_SUBDIVISIONES)
        {
            nivelSubdivision++;
            cambioDetalleBloqueado = true;
        }
        else if (bajarDetalle && nivelSubdivision > 0)
        {
            nivelSubdivision--;
            cambioDetalleBloqueado = true;
        }
    }
    if (!subirDetalle && !bajarDetalle)
        cambioDetalleBloqueado = false;

    // Traslacion
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) traslacionX += PASO_TRASLACION;
    if (glfwGetKey(window, GLFW_KEY_LEFT)  == GLFW_PRESS) traslacionX -= PASO_TRASLACION;
    if (glfwGetKey(window, GLFW_KEY_UP)    == GLFW_PRESS) traslacionY += PASO_TRASLACION;
    if (glfwGetKey(window, GLFW_KEY_DOWN)  == GLFW_PRESS) traslacionY -= PASO_TRASLACION;
    if (glfwGetKey(window, GLFW_KEY_E)     == GLFW_PRESS) traslacionZ += PASO_TRASLACION;
    if (glfwGetKey(window, GLFW_KEY_Q)     == GLFW_PRESS) traslacionZ -= PASO_TRASLACION;

    // Eje de rotacion
    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) ejeRotacion = 0;
    if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS) ejeRotacion = 1;
    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) ejeRotacion = 2;

    // Rotacion
    if      (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) velocidadRot =  PASO_ROT;
    else if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) velocidadRot = -PASO_ROT;
    else                                                     velocidadRot =  0.0f;

    // Acercar/alejar fuente de luz
    if (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS) luzZ += 0.02f;
    if (glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS) luzZ -= 0.02f;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}
