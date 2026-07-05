#version 330 core

// ------------------------------------------------------------------
// VERSION A: la iluminacion se calcula por cara en main.cpp.
// El shader solo transforma la posicion y entrega el color recibido.
// ------------------------------------------------------------------

layout (location = 0) in vec3 aPos;
layout (location = 2) in vec3 aColor;

out vec4 vertexColor;

uniform mat4 uModel;
uniform mat4 uProjection;

void main()
{
    vertexColor = vec4(aColor, 1.0);
    gl_Position = uProjection * uModel * vec4(aPos, 1.0);
}
