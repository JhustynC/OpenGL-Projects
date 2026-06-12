#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aColor;

out vec4 vertexColor;

// ----------------------------------------------------------------------
// VERSION B: la aplicacion (con GLM) ya calculo TODA la transformacion
// (modelo * vista * proyeccion) y nos la entrega lista en una sola
// matriz uniform. El shader solo aplica la multiplicacion final.
// ----------------------------------------------------------------------
uniform mat4 uMVP; // Model * View * Projection, calculada en C++ con GLM

void main()
{
    gl_Position = uMVP * vec4(aPos, 1.0);
    vertexColor = aColor;
}
