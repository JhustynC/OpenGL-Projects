#version 330 core

// Datos recibidos desde el VBO segun la configuracion del VAO.
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

// Matrices enviadas desde drawObject() para este objeto.
uniform mat4 uModel;
uniform mat4 uMVP;

// Datos que cada vertice entrega al fragment shader.
out vec3 FragPos;
out vec3 Normal;
out vec3 LocalPos;

void main()
{
    // LOCAL -> WORLD: la matriz modelo coloca el objeto en el juego.
    vec4 worldPosition = uModel * vec4(aPos, 1.0);
    // La iluminacion necesita saber donde quedo el punto en el mundo.
    FragPos = worldPosition.xyz;

    // La normal tambien debe pasar del marco local al marco del mundo.
    mat3 normalMatrix = transpose(inverse(mat3(uModel)));
    Normal = normalize(normalMatrix * aNormal);
    // Conservamos la posicion local para que la marca siga a la pelota.
    LocalPos = aPos;

    // LOCAL -> WORLD -> VIEW -> CLIP. OpenGL exige clip en gl_Position.
    gl_Position = uMVP * vec4(aPos, 1.0);
}
