#version 330 core

in vec4 vertexColor;
out vec4 FragColor;

void main()
{
    // El color ya viene calculado en main.cpp para la version A.
    FragColor = vertexColor;
}
