// GLAD carga las funciones de OpenGL; GLFW crea la ventana y lee el teclado.
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// GLM aporta vectores, matrices y funciones de transformacion.
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Herramientas normales de C++ usadas por la logica y los archivos.
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// Clase pequena que lee, compila, activa y elimina los shaders.
#include "shader.h"

// ============================================================================
// ARKANOID 3D SIMPLE
// Conserva los conceptos de la materia y elimina elementos secundarios.
// Ejes del mundo: X = izquierda/derecha, Y = profundidad, Z = altura.
// ============================================================================

// Tamano de la ventana en pixeles.
const unsigned int WIDTH = 1000;
const unsigned int HEIGHT = 720;

// Limites del cuarto en coordenadas del mundo.
// No hay pared frontal: si la bola pasa LOSE_LINE, se pierde una vida.
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

// El paddle conserva Y fija y solo se mueve en X y Z.
// Los tamanos son mitades porque tambien se usan para las colisiones.
const float PADDLE_Y = -6.0f;
const glm::vec3 PADDLE_HALF_SIZE(1.6f, 0.25f, 0.45f);
const glm::vec3 BLOCK_HALF_SIZE(0.6f, 0.25f, 0.3f);
const glm::vec3 CAMERA_POS(0.0f, -15.8f, 2.5f);

// Una malla guarda los identificadores de OpenGL y cuantos vertices dibujar.
struct Mesh
{
    // El VAO recuerda como leer los datos; el VBO guarda los vertices.
    unsigned int VAO = 0;
    unsigned int VBO = 0;
    int vertexCount = 0;
};

// Estado de un bloque del nivel. Todos reutilizan la misma malla de cubo.
struct Block
{
    // Posicion en el mundo y color RGB.
    glm::vec3 position;
    glm::vec3 color;
    // Estados: normal, desapareciendo o eliminado.
    bool alive = true;
    bool disappearing = false;
    // Tiempo transcurrido desde que la bola lo golpeo.
    float disappearTime = 0.0f;
};

// Estado completo de la pelota.
struct Ball
{
    // position indica donde esta; velocity indica cuanto avanza por segundo.
    glm::vec3 position = glm::vec3(0.0f, -4.8f, -0.7f);
    glm::vec3 velocity = glm::vec3(3.2f, 5.0f, 1.5f);
    // rotation es el giro acumulado y spin es la rapidez de ese giro.
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 spin = glm::vec3(2.5f, 3.2f, 1.8f);
    float radius = 0.28f;

    // Devuelve la pelota a su estado inicial despues de perder o reiniciar.
    void reset()
    {
        position = glm::vec3(0.0f, -4.8f, -0.7f);
        velocity = glm::vec3(3.2f, 5.0f, 1.5f);
        rotation = glm::vec3(0.0f);
        spin = glm::vec3(2.5f, 3.2f, 1.8f);
    }

    void updateRotation(float dt)
    {
        // Angulo nuevo = angulo anterior + rapidez angular * tiempo.
        rotation += spin * dt;
    }

    // Cambia visualmente el giro segun la direccion del ultimo choque.
    // Es una aproximacion sencilla, no una simulacion fisica completa.
    void registerCollision(float influenceX, float influenceY, float influenceZ)
    {
        spin.x = 2.4f + std::abs(velocity.z) * 0.40f
                 + std::abs(influenceZ);
        spin.y = 2.8f + std::abs(velocity.x) * 0.35f
                 + std::abs(influenceX);
        spin.z = 1.6f + std::abs(velocity.y) * 0.25f
                 + std::abs(influenceY);

        // Un valor negativo cambia el sentido del giro correspondiente.
        if (influenceX < 0.0f) spin.z *= -1.0f;
        if (influenceY < 0.0f) spin.x *= -1.0f;
        if (influenceZ < 0.0f) spin.y *= -1.0f;
    }
};

std::string shaderPath(const std::string& name)
{
    // Parte desde la carpeta de main.cpp y entra a la carpeta shaders.
    std::filesystem::path source = __FILE__;
    return (source.parent_path() / "shaders" / name).string();
}

// El VBO guarda grupos de seis floats: posicion (x,y,z) y normal (nx,ny,nz).
Mesh createMesh(const std::vector<float>& vertices)
{
    Mesh mesh;
    // Cada vertice ocupa seis floats, por eso dividimos el total entre seis.
    mesh.vertexCount = static_cast<int>(vertices.size() / 6);

    // Creamos un VAO para recordar el formato y un VBO para guardar los datos.
    glGenVertexArrays(1, &mesh.VAO);
    glGenBuffers(1, &mesh.VBO);
    glBindVertexArray(mesh.VAO);

    // Copiamos el vector de la CPU al VBO administrado por OpenGL.
    glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
    glBufferData(GL_ARRAY_BUFFER,
                 vertices.size() * sizeof(float),
                 vertices.data(),
                 GL_STATIC_DRAW);

    // Atributo 0: posicion. Son tres floats y comienza en el primer dato.
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Atributo 1: normal. Saltamos primero los tres floats de posicion.
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Dejamos de editar el VAO y devolvemos la malla preparada.
    glBindVertexArray(0);
    return mesh;
}

Mesh createCube()
{
    // Cubo unitario centrado en el origen local. Cada cara usa dos triangulos.
    // Formato de cada vertice: posicion (x,y,z), normal (nx,ny,nz).
    const std::vector<float> vertices = {
        // Cara inferior en Z = -0.5
        -0.5f,-0.5f,-0.5f,  0,0,-1,   0.5f, 0.5f,-0.5f,  0,0,-1,   0.5f,-0.5f,-0.5f,  0,0,-1,
         0.5f, 0.5f,-0.5f,  0,0,-1,  -0.5f,-0.5f,-0.5f,  0,0,-1,  -0.5f, 0.5f,-0.5f,  0,0,-1,
        // Cara superior en Z = 0.5
        -0.5f,-0.5f, 0.5f,  0,0, 1,   0.5f,-0.5f, 0.5f,  0,0, 1,   0.5f, 0.5f, 0.5f,  0,0, 1,
         0.5f, 0.5f, 0.5f,  0,0, 1,  -0.5f, 0.5f, 0.5f,  0,0, 1,  -0.5f,-0.5f, 0.5f,  0,0, 1,
        // Cara izquierda
        -0.5f, 0.5f, 0.5f, -1,0,0,  -0.5f, 0.5f,-0.5f, -1,0,0,  -0.5f,-0.5f,-0.5f, -1,0,0,
        -0.5f,-0.5f,-0.5f, -1,0,0,  -0.5f,-0.5f, 0.5f, -1,0,0,  -0.5f, 0.5f, 0.5f, -1,0,0,
        // Cara derecha
         0.5f, 0.5f, 0.5f,  1,0,0,   0.5f,-0.5f,-0.5f,  1,0,0,   0.5f, 0.5f,-0.5f,  1,0,0,
         0.5f,-0.5f,-0.5f,  1,0,0,   0.5f, 0.5f, 0.5f,  1,0,0,   0.5f,-0.5f, 0.5f,  1,0,0,
        // Cara frontal en Y = -0.5
        -0.5f,-0.5f,-0.5f,  0,-1,0,   0.5f,-0.5f,-0.5f,  0,-1,0,   0.5f,-0.5f, 0.5f,  0,-1,0,
         0.5f,-0.5f, 0.5f,  0,-1,0,  -0.5f,-0.5f, 0.5f,  0,-1,0,  -0.5f,-0.5f,-0.5f,  0,-1,0,
        // Cara posterior en Y = 0.5
        -0.5f, 0.5f,-0.5f,  0,1,0,   0.5f, 0.5f, 0.5f,  0,1,0,   0.5f, 0.5f,-0.5f,  0,1,0,
         0.5f, 0.5f, 0.5f,  0,1,0,  -0.5f, 0.5f,-0.5f,  0,1,0,  -0.5f, 0.5f, 0.5f,  0,1,0
    };
    // createMesh copia estos vertices al VBO y configura su VAO.
    return createMesh(vertices);
}

Mesh createSphere(int stacks = 12, int slices = 18)
{
    // La esfera no se escribe a mano: sus vertices se generan con angulos.
    // stacks son franjas verticales y slices son divisiones alrededor.
    std::vector<float> vertices;
    const float pi = 3.14159265f;

    // Pequena funcion local que agrega posicion y normal al vector.
    auto addVertex = [&vertices](glm::vec3 p)
    {
        // En una esfera centrada en el origen, la normal apunta como p.
        glm::vec3 n = glm::normalize(p);
        vertices.insert(vertices.end(), {p.x, p.y, p.z, n.x, n.y, n.z});
    };

    // phi recorre del polo inferior al polo superior.
    for (int stack = 0; stack < stacks; ++stack)
    {
        float phi0 = -pi * 0.5f + pi * stack / stacks;
        float phi1 = -pi * 0.5f + pi * (stack + 1) / stacks;

        // theta da una vuelta completa alrededor del eje Z.
        for (int slice = 0; slice < slices; ++slice)
        {
            float theta0 = 2.0f * pi * slice / slices;
            float theta1 = 2.0f * pi * (slice + 1) / slices;

            // Cuatro esquinas de una pequena seccion de la esfera.
            // La formula produce una esfera local de radio uno.
            glm::vec3 p0(cosf(phi0)*cosf(theta0), cosf(phi0)*sinf(theta0), sinf(phi0));
            glm::vec3 p1(cosf(phi0)*cosf(theta1), cosf(phi0)*sinf(theta1), sinf(phi0));
            glm::vec3 p2(cosf(phi1)*cosf(theta1), cosf(phi1)*sinf(theta1), sinf(phi1));
            glm::vec3 p3(cosf(phi1)*cosf(theta0), cosf(phi1)*sinf(theta0), sinf(phi1));

            // La seccion se divide en dos triangulos.
            addVertex(p0); addVertex(p1); addVertex(p2);
            addVertex(p2); addVertex(p3); addVertex(p0);
        }
    }

    // Enviamos a la GPU la lista que acabamos de calcular.
    return createMesh(vertices);
}

std::vector<Block> createBlocks()
{
    // Aqui se crea el estado del nivel, no nuevas mallas de cubo.
    std::vector<Block> blocks;
    // Un color RGB para cada una de las cinco filas.
    const glm::vec3 colors[] = {
        {0.95f, 0.20f, 0.18f},
        {0.98f, 0.65f, 0.12f},
        {0.20f, 0.75f, 0.35f},
        {0.15f, 0.55f, 0.95f},
        {0.65f, 0.30f, 0.95f}
    };

    // Cinco filas por ocho columnas producen cuarenta bloques.
    for (int row = 0; row < 5; ++row)
        for (int column = 0; column < 8; ++column)
            // X cambia con la columna, Y queda al fondo y Z cambia por fila.
            blocks.push_back({{-4.9f + column * 1.4f, 5.8f,
                               2.6f - row * 0.75f}, colors[row],
                              // Empieza vivo, sin animacion y con tiempo cero.
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
    // Empezamos con una matriz identidad, que todavia no cambia el objeto.
    glm::mat4 model(1.0f);
    // LOCAL -> WORLD: tamano, giro y posicion propia de este objeto.
    // Aunque se escriben en este orden, al vertice se aplican de abajo arriba:
    // primero escala, despues rota y al final traslada.
    model = glm::translate(model, position);
    model = glm::rotate(model, rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, scale);

    // Recorrido completo que estudiamos: LOCAL -> WORLD -> VIEW -> CLIP.
    glm::mat4 mvp = projection * view * model;

    // uModel se usa para obtener posiciones y normales en el mundo.
    glUniformMatrix4fv(glGetUniformLocation(shader.ID, "uModel"),
                       1, GL_FALSE, glm::value_ptr(model));
    // uMVP lleva directamente los vertices locales hasta el marco de clip.
    glUniformMatrix4fv(glGetUniformLocation(shader.ID, "uMVP"),
                       1, GL_FALSE, glm::value_ptr(mvp));
    // Datos que el fragment shader usa para color, luz y transparencia.
    glUniform3fv(glGetUniformLocation(shader.ID, "uColor"),
                 1, glm::value_ptr(color));
    glUniform1i(glGetUniformLocation(shader.ID, "uUnlit"), unlit ? 1 : 0);
    glUniform1i(glGetUniformLocation(shader.ID, "uIsBall"), isBall ? 1 : 0);
    glUniform1f(glGetUniformLocation(shader.ID, "uAlpha"), alpha);

    // Seleccionamos la malla y dibujamos cada grupo de tres como triangulo.
    glBindVertexArray(mesh.VAO);
    glDrawArrays(GL_TRIANGLES, 0, mesh.vertexCount);
}

bool sphereTouchesBox(const glm::vec3& spherePosition, float radius,
                      const glm::vec3& boxCenter, const glm::vec3& boxHalfSize,
                      glm::vec3& collisionNormal)
{
    // Limitamos el centro de la esfera a los bordes de la caja.
    // El resultado es el punto de la caja mas cercano a la esfera.
    glm::vec3 closestPoint = glm::clamp(
        spherePosition,
        boxCenter - boxHalfSize,
        boxCenter + boxHalfSize
    );
    // Vector que va desde ese punto cercano hasta el centro de la esfera.
    glm::vec3 difference = spherePosition - closestPoint;
    // Usamos distancia al cuadrado para evitar una raiz cuadrada innecesaria.
    float distanceSquared = glm::dot(difference, difference);

    // Si la distancia es mayor que el radio, no se estan tocando.
    if (distanceSquared > radius * radius)
        return false;

    // En el caso normal, esta direccion sera la normal del rebote.
    if (distanceSquared > 0.000001f)
    {
        collisionNormal = glm::normalize(difference);
        return true;
    }

    // Caso poco frecuente: el centro de la bola quedo dentro de la caja.
    // Buscamos la cara mas cercana para decidir por donde debe salir.
    glm::vec3 local = spherePosition - boxCenter;
    glm::vec3 penetration = boxHalfSize - glm::abs(local);
    // La componente mas pequena indica la salida mas corta.
    if (penetration.x <= penetration.y && penetration.x <= penetration.z)
        collisionNormal = {local.x < 0.0f ? -1.0f : 1.0f, 0.0f, 0.0f};
    else if (penetration.y <= penetration.z)
        collisionNormal = {0.0f, local.y < 0.0f ? -1.0f : 1.0f, 0.0f};
    else
        collisionNormal = {0.0f, 0.0f, local.z < 0.0f ? -1.0f : 1.0f};

    // Estaba dentro de la caja, por lo tanto si existe una colision.
    return true;
}

void deleteMesh(Mesh& mesh)
{
    // Libera los recursos creados en OpenGL cuando ya no se necesitan.
    glDeleteVertexArrays(1, &mesh.VAO);
    glDeleteBuffers(1, &mesh.VBO);
}

void framebufferSizeCallback(GLFWwindow*, int width, int height)
{
    // Hace que OpenGL use toda el area disponible del framebuffer.
    // Aqui NDC finalmente se adapta a los pixeles de la ventana.
    glViewport(0, 0, width, height);
}

int main()
{
    // ---------------------------------------------------------------------
    // 1. CREAR LA VENTANA Y PREPARAR OPENGL
    // ---------------------------------------------------------------------
    // GLFW administra la ventana, el teclado y el contexto de OpenGL.
    glfwInit();
    // Pedimos OpenGL 3.3 con el perfil moderno usado por nuestros shaders.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    // nullptr indica que no usamos pantalla completa ni compartimos otra ventana.
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Arkanoid 3D Simple", nullptr, nullptr);
    if (!window)
    {
        // Si la ventana falla, cerramos GLFW y detenemos el programa.
        std::cout << "No se pudo crear la ventana." << std::endl;
        glfwTerminate();
        return -1;
    }

    // Este contexto sera el que reciba todas las instrucciones de OpenGL.
    glfwMakeContextCurrent(window);
    // Conectamos el cambio de framebuffer con nuestro glViewport.
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    // VSync: presenta la imagen al ritmo de actualizacion de la pantalla.
    glfwSwapInterval(1);

    // GLAD carga las funciones modernas de OpenGL para este equipo.
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "No se pudo iniciar GLAD." << std::endl;
        return -1;
    }

    // La profundidad hace que lo cercano oculte correctamente lo lejano.
    glEnable(GL_DEPTH_TEST);
    // El blending permite objetos transparentes, como el paddle.
    glEnable(GL_BLEND);
    // Color final = color nuevo * alpha + fondo * (1 - alpha).
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // ---------------------------------------------------------------------
    // 2. CREAR RECURSOS Y ESTADO INICIAL DEL JUEGO
    // ---------------------------------------------------------------------
    // Lee, compila y une el vertex shader y el fragment shader.
    Shader shader(shaderPath("vertex_shader.glsl").c_str(),
                  shaderPath("fragment_shader.glsl").c_str());
    // Estas dos mallas se crean una vez y luego se reutilizan en cada frame.
    Mesh cube = createCube();
    Mesh sphere = createSphere();

    // Estado inicial: cuarenta bloques, una pelota y el paddle centrado.
    std::vector<Block> blocks = createBlocks();
    Ball ball;
    glm::vec3 paddlePosition(0.0f, PADDLE_Y, -0.8f);
    // Variables sencillas de la logica del juego.
    int score = 0;
    int lives = 3;

    // VIEW: transforma el mundo para verlo desde la camara.
    // Parametros: posicion de camara, punto observado y direccion arriba.
    glm::mat4 view = glm::lookAt(
        CAMERA_POS,
        glm::vec3(0.0f, -5.0f, 0.8f),
        glm::vec3(0.0f, 0.0f, 1.0f)
    );

    // PROJECTION: lente perspectiva; su salida es el marco de clip.
    // Usa campo de vision, proporcion de ventana, plano cercano y lejano.
    glm::mat4 projection = glm::perspective(
        glm::radians(52.0f),
        static_cast<float>(WIDTH) / HEIGHT,
        0.1f,
        100.0f
    );

    // Mostramos los controles en la consola.
    std::cout << "ARKANOID 3D SIMPLE\n"
              << "A/D o izquierda/derecha: eje X\n"
              << "W/S o arriba/abajo: eje Z\n"
              << "R: reiniciar | ESC: salir\n";

    // ---------------------------------------------------------------------
    // 3. CICLO PRINCIPAL: UN RECORRIDO PRODUCE UN FOTOGRAMA
    // ---------------------------------------------------------------------
    float previousTime = static_cast<float>(glfwGetTime());
    while (!glfwWindowShouldClose(window))
    {
        // dt es el tiempo desde el frame anterior. Se limita para evitar
        // saltos enormes si el programa se pausa por un momento.
        float currentTime = static_cast<float>(glfwGetTime());
        float dt = std::min(currentTime - previousTime, 0.033f);
        previousTime = currentTime;

        // -----------------------------------------------------------------
        // 3A. LEER EL TECLADO Y MOVER EL PADDLE
        // -----------------------------------------------------------------
        // Escape marca la ventana para salir del ciclo.
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        // Direccion deseada en este frame: -1, 0 o 1 en cada eje.
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

        // Posicion nueva = posicion anterior + direccion * rapidez * tiempo.
        paddlePosition.x += directionX * 7.0f * dt;
        paddlePosition.z += directionZ * 7.0f * dt;
        // Limitamos el centro teniendo en cuenta el tamano completo del paddle.
        paddlePosition.x = std::clamp(paddlePosition.x,
                                      LEFT_WALL + PADDLE_HALF_SIZE.x,
                                      RIGHT_WALL - PADDLE_HALF_SIZE.x);
        paddlePosition.z = std::clamp(paddlePosition.z,
                                      FLOOR_Z + PADDLE_HALF_SIZE.z,
                                      CEILING_Z - PADDLE_HALF_SIZE.z);

        // R devuelve todo el juego a su estado inicial.
        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
        {
            ball.reset();
            blocks = createBlocks();
            paddlePosition = glm::vec3(0.0f, PADDLE_Y, -0.8f);
            score = 0;
            lives = 3;
        }

        // -----------------------------------------------------------------
        // 3B. ACTUALIZAR EL JUEGO EN LA CPU
        // -----------------------------------------------------------------
        // Movimiento lineal y giro, ambos independientes de los FPS gracias a dt.
        ball.position += ball.velocity * dt;
        ball.updateRotation(dt);

        // Los bloques golpeados permanecen 0.72 segundos para mostrar animacion.
        for (Block& block : blocks)
        {
            // continue pasa directamente al siguiente bloque.
            if (!block.disappearing)
                continue;

            // Cada bloque conserva su propio reloj de desaparicion.
            block.disappearTime += dt;
            if (block.disappearTime >= 0.72f)
            {
                // Al terminar el tiempo, deja de dibujarse y de colisionar.
                block.alive = false;
                block.disappearing = false;
            }
        }

        // -----------------------------------------------------------------
        // 3C. RESOLVER COLISIONES DE LA PELOTA
        // -----------------------------------------------------------------
        // Paredes laterales: revisamos los extremos centro -/+ radio.
        if (ball.position.x - ball.radius < LEFT_WALL ||
            ball.position.x + ball.radius > RIGHT_WALL)
        {
            // Invertir X produce el rebote horizontal.
            ball.velocity.x *= -1.0f;
            // Corregimos la posicion para que no quede dentro de la pared.
            ball.position.x = std::clamp(ball.position.x,
                                         LEFT_WALL + ball.radius,
                                         RIGHT_WALL - ball.radius);
            // El choque tambien modifica el giro visible de la pelota.
            ball.registerCollision(ball.velocity.x, 0.0f, 0.0f);
        }

        // Piso y techo: el mismo proceso, pero sobre el eje Z.
        if (ball.position.z - ball.radius < FLOOR_Z ||
            ball.position.z + ball.radius > CEILING_Z)
        {
            ball.velocity.z *= -1.0f;
            ball.position.z = std::clamp(ball.position.z,
                                         FLOOR_Z + ball.radius,
                                         CEILING_Z - ball.radius);
            ball.registerCollision(0.0f, 0.0f, ball.velocity.z);
        }

        // Pared del fondo: fuerza a la pelota a regresar hacia Y negativo.
        if (ball.position.y + ball.radius > BACK_WALL)
        {
            ball.velocity.y = -std::abs(ball.velocity.y);
            ball.position.y = BACK_WALL - ball.radius;
            ball.registerCollision(0.0f, ball.velocity.y, 0.0f);
        }

        // sphereTouchesBox escribira aqui la direccion del contacto.
        glm::vec3 collisionNormal(0.0f);
        // Solo probamos el paddle si la pelota viaja hacia el, en Y negativo.
        if (ball.velocity.y < 0.0f &&
            sphereTouchesBox(ball.position, ball.radius,
                             paddlePosition, PADDLE_HALF_SIZE,
                             collisionNormal))
        {
            // Primero la sacamos del paddle para evitar choques repetidos.
            ball.position.y = PADDLE_Y + PADDLE_HALF_SIZE.y + ball.radius;
            // reflect cambia la velocidad usando la normal encontrada.
            ball.velocity = glm::reflect(ball.velocity, collisionNormal);
            // Garantizamos que salga hacia el fondo, en Y positivo.
            ball.velocity.y = std::abs(ball.velocity.y);

            // El punto donde golpea el paddle modifica la salida en X y Z.
            // Golpear a un lado agrega velocidad hacia ese mismo lado.
            ball.velocity.x += (ball.position.x - paddlePosition.x) * 0.8f;
            ball.velocity.z += (ball.position.z - paddlePosition.z) * 0.8f;
            ball.registerCollision(collisionNormal.x,
                                   collisionNormal.y,
                                   collisionNormal.z);
        }

        // Buscamos el primer bloque normal que este tocando la pelota.
        for (Block& block : blocks)
        {
            if (block.alive && !block.disappearing &&
                sphereTouchesBox(ball.position, ball.radius,
                                 block.position, BLOCK_HALF_SIZE,
                                 collisionNormal))
            {
                // El golpe inicia la animacion, pero no lo borra de inmediato.
                block.disappearing = true;
                block.disappearTime = 0.0f;
                // Solo reflejamos si la pelota realmente entra contra la cara.
                if (glm::dot(ball.velocity, collisionNormal) < 0.0f)
                    ball.velocity = glm::reflect(ball.velocity, collisionNormal);
                ball.registerCollision(collisionNormal.x,
                                       collisionNormal.y,
                                       collisionNormal.z);
                score += 10;
                // Procesamos un solo bloque por frame para evitar dobles rebotes.
                break;
            }
        }

        // No hay pared frontal: cruzar esta linea significa perder una vida.
        if (ball.position.y < LOSE_LINE)
        {
            --lives;
            ball.reset();
            // Sin vidas se reconstruye el nivel y se restaura el marcador.
            if (lives <= 0)
            {
                blocks = createBlocks();
                score = 0;
                lives = 3;
            }
        }

        // Construimos el titulo actualizado con puntaje y vidas.
        std::ostringstream title;
        title << "Arkanoid 3D Simple | Puntaje: " << score << " | Vidas: " << lives;
        glfwSetWindowTitle(window, title.str().c_str());

        // -----------------------------------------------------------------
        // 3D. DIBUJAR EL FOTOGRAMA EN LA GPU
        // -----------------------------------------------------------------
        // Color de fondo y limpieza de color y profundidad del frame anterior.
        glClearColor(0.025f, 0.03f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Activamos el programa shader antes de enviarle sus uniformes.
        shader.use();
        // La posicion mundial de la camara se usa para el brillo especular.
        glUniform3fv(glGetUniformLocation(shader.ID, "uCameraPos"),
                     1, glm::value_ptr(CAMERA_POS));
        // La bola es una fuente puntual de luz y la luz viaja con ella.
        glm::vec3 lightPosition = ball.position;
        glUniform3fv(glGetUniformLocation(shader.ID, "uLightPos"),
                     1, glm::value_ptr(lightPosition));

        // El mismo cubo local se reutiliza para construir todo el cuarto.
        // Cada llamada cambia su posicion, escala y color mediante uModel.
        // Piso: cubo ancho y profundo, pero muy delgado en Z.
        drawObject(shader, cube, view, projection,
                   {0.0f, ROOM_CENTER_Y, FLOOR_Z - 0.1f},
                   {12.4f, ROOM_DEPTH, 0.2f}, {0.10f, 0.14f, 0.22f});
        // Techo.
        drawObject(shader, cube, view, projection,
                   {0.0f, ROOM_CENTER_Y, CEILING_Z + 0.1f},
                   {12.4f, ROOM_DEPTH, 0.2f}, {0.08f, 0.12f, 0.20f});
        // Pared izquierda.
        drawObject(shader, cube, view, projection,
                   {LEFT_WALL - 0.1f, ROOM_CENTER_Y, ROOM_CENTER_Z},
                   {0.2f, ROOM_DEPTH, ROOM_HEIGHT}, {0.12f, 0.25f, 0.48f});
        // Pared derecha.
        drawObject(shader, cube, view, projection,
                   {RIGHT_WALL + 0.1f, ROOM_CENTER_Y, ROOM_CENTER_Z},
                   {0.2f, ROOM_DEPTH, ROOM_HEIGHT}, {0.12f, 0.25f, 0.48f});
        // Pared del fondo. El frente queda abierto para la camara.
        drawObject(shader, cube, view, projection,
                   {0.0f, BACK_WALL + 0.1f, ROOM_CENTER_Z},
                   {12.4f, 0.2f, ROOM_HEIGHT}, {0.12f, 0.25f, 0.48f});

        // Primera pasada: bloques normales y completamente opacos.
        for (const Block& block : blocks)
            if (block.alive && !block.disappearing)
                drawObject(shader, cube, view, projection,
                           block.position,
                           BLOCK_HALF_SIZE * 2.0f, block.color);

        // Los bloques que desaparecen se dibujan transparentes y sin escribir
        // profundidad, para que el parpadeo no oculte objetos posteriores.
        // El depth test sigue activo, pero estos objetos no escriben profundidad.
        glDepthMask(GL_FALSE);
        for (const Block& block : blocks)
        {
            if (!block.alive || !block.disappearing)
                continue;

            // progress pasa de 0 a 1 durante los 0.72 segundos.
            float progress = std::clamp(block.disappearTime / 0.72f, 0.0f, 1.0f);
            // Se encoge hasta quedar en 12 por ciento de su tamano.
            float shrink = 1.0f - progress * 0.88f;
            // El seno produce un parpadeo rapido entre valores claros y bajos.
            float blink = 0.35f + 0.65f * std::abs(sinf(block.disappearTime * 42.0f));
            // Ademas del parpadeo, la opacidad baja gradualmente hasta cero.
            float alpha = (1.0f - progress) * blink;
            // mix acerca poco a poco el color original hacia blanco.
            glm::vec3 flashColor = glm::mix(block.color, glm::vec3(1.0f),
                                            progress * 0.45f);

            drawObject(shader, cube, view, projection,
                       block.position,
                       BLOCK_HALF_SIZE * 2.0f * shrink,
                       flashColor, false, glm::vec3(0.0f), false, alpha);
        }
        // Restauramos la escritura normal de profundidad.
        glDepthMask(GL_TRUE);

        // La esfera local tiene radio uno: la escala le da ball.radius.
        // ball.rotation mueve las marcas y permite observar el giro.
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

        // Mostramos el back buffer terminado y procesamos teclado/ventana.
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // ---------------------------------------------------------------------
    // 4. CERRAR Y LIBERAR RECURSOS
    // ---------------------------------------------------------------------
    // Se eliminan mientras el contexto de OpenGL todavia existe.
    deleteMesh(cube);
    deleteMesh(sphere);
    // Eliminamos el programa shader antes de destruir el contexto.
    shader.destroy();
    // GLFW cierra la ventana y el contexto de OpenGL.
    glfwTerminate();
    // Cero indica que la aplicacion termino correctamente.
    return 0;
}
