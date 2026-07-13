#ifndef ARKANOID_SIMPLE_SHADER_H
#define ARKANOID_SIMPLE_SHADER_H

// Estas protecciones evitan incluir el mismo archivo dos veces.

#include <glad/glad.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

class Shader
{
public:
    // Identificador del programa completo dentro de OpenGL.
    unsigned int ID = 0;

    Shader(const char* vertexPath, const char* fragmentPath)
    {
        // Primero leemos el texto de ambos archivos GLSL.
        std::string vertexCode = readFile(vertexPath);
        std::string fragmentCode = readFile(fragmentPath);

        // Cada shader se compila por separado.
        unsigned int vertex = compile(GL_VERTEX_SHADER, vertexCode.c_str(), "VERTEX");
        unsigned int fragment = compile(GL_FRAGMENT_SHADER, fragmentCode.c_str(), "FRAGMENT");

        // Despues se unen en un solo programa que puede usar la GPU.
        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);

        // Consultamos si la union funciono y mostramos el error si fallo.
        int success = 0;
        char log[1024];
        glGetProgramiv(ID, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(ID, 1024, nullptr, log);
            std::cout << "Error enlazando shaders:\n" << log << std::endl;
        }

        // Tras enlazarlos, las piezas individuales ya no hacen falta.
        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }

    void use() const
    {
        // Desde aqui, los siguientes dibujos usan este programa.
        glUseProgram(ID);
    }

    void destroy()
    {
        // Libera el programa al terminar la aplicacion.
        glDeleteProgram(ID);
        ID = 0;
    }

private:
    static std::string readFile(const char* path)
    {
        // Abrimos el archivo indicado y devolvemos todo su contenido como texto.
        std::ifstream file(path);
        if (!file)
        {
            std::cout << "No se pudo abrir: " << path << std::endl;
            return {};
        }

        std::stringstream stream;
        stream << file.rdbuf();
        return stream.str();
    }

    static unsigned int compile(GLenum type, const char* code, const char* name)
    {
        // Creamos el tipo pedido, copiamos su codigo y solicitamos compilarlo.
        unsigned int shader = glCreateShader(type);
        glShaderSource(shader, 1, &code, nullptr);
        glCompileShader(shader);

        // Si la compilacion falla, la consola mostrara la razon.
        int success = 0;
        char log[1024];
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(shader, 1024, nullptr, log);
            std::cout << "Error compilando " << name << ":\n" << log << std::endl;
        }

        return shader;
    }
};

#endif
