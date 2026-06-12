#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aColor;

out vec4 vertexColor;

// ----------------------------------------------------------------------
// VERSION A: las transformaciones se calculan AQUI, dentro del shader,
// a partir de parametros simples (angulos, desplazamientos, escalas)
// que la aplicacion envia como "uniforms".
// ----------------------------------------------------------------------

uniform vec3  uTraslacion;      // desplazamiento en x, y, z
uniform vec3  uEscala;          // factores de escala en x, y, z
uniform float uAngulo;          // angulo de rotacion interactiva (radianes)
uniform int   uEjeRotacion;     // 0 = X, 1 = Y, 2 = Z
uniform mat4  uProjection;      // proyeccion (perspectiva)

// Pose isometrica inicial (fija, no depende del teclado)
uniform float uAnguloInicialX;  // ~ -25 a -35 grados, en radianes
uniform float uAnguloInicialY;  // ~ -45 grados, en radianes

// Matriz de rotacion alrededor del eje X
mat4 rotacionX(float a)
{
    float c = cos(a);
    float s = sin(a);
    return mat4(
        1.0, 0.0, 0.0, 0.0,
        0.0,   c,   s, 0.0,
        0.0,  -s,   c, 0.0,
        0.0, 0.0, 0.0, 1.0
    );
}

// Matriz de rotacion alrededor del eje Y
mat4 rotacionY(float a)
{
    float c = cos(a);
    float s = sin(a);
    return mat4(
          c, 0.0,  -s, 0.0,
        0.0, 1.0, 0.0, 0.0,
          s, 0.0,   c, 0.0,
        0.0, 0.0, 0.0, 1.0
    );
}

// Matriz de rotacion alrededor del eje Z
mat4 rotacionZ(float a)
{
    float c = cos(a);
    float s = sin(a);
    return mat4(
          c,   s, 0.0, 0.0,
         -s,   c, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0
    );
}

// Matriz de escalamiento
mat4 escalamiento(vec3 e)
{
    return mat4(
        e.x, 0.0, 0.0, 0.0,
        0.0, e.y, 0.0, 0.0,
        0.0, 0.0, e.z, 0.0,
        0.0, 0.0, 0.0, 1.0
    );
}

// Matriz de traslacion
mat4 traslacion(vec3 t)
{
    return mat4(
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        t.x, t.y, t.z, 1.0
    );
}

void main()
{
    // Elegir la matriz de rotacion interactiva segun el eje activo
    mat4 rotacionInteractiva;
    if (uEjeRotacion == 0)
        rotacionInteractiva = rotacionX(uAngulo);
    else if (uEjeRotacion == 1)
        rotacionInteractiva = rotacionY(uAngulo);
    else
        rotacionInteractiva = rotacionZ(uAngulo);

    // Pose isometrica inicial: rota primero en Y, luego en X.
    // Esto da la vista "de esquina" donde se ven 3 caras del cubo
    // (como en la imagen de referencia), independientemente de la
    // rotacion interactiva del usuario.
    mat4 poseInicial = rotacionX(uAnguloInicialX) * rotacionY(uAnguloInicialY);

    // Orden de aplicacion: Escala -> Rotacion interactiva -> Pose inicial -> Traslacion -> Proyeccion
    mat4 modelo = traslacion(uTraslacion) * poseInicial * rotacionInteractiva * escalamiento(uEscala);

    gl_Position = uProjection * modelo * vec4(aPos, 1.0);
    vertexColor = aColor;
}
