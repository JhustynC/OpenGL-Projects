#version 330 core

in vec4 vertexColor;
out vec4 FragColor;

void main()
{
    // El color ya viene calculado del vertex shader (modelo Phong)
    // El rasterizador lo interpolo entre los vertices del triangulo
    FragColor = vertexColor;
}
