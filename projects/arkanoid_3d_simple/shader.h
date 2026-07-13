#ifndef ARKANOID_SIMPLE_SHADER_H
#define ARKANOID_SIMPLE_SHADER_H

#include <glad/glad.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

class Shader
{
public:
    unsigned int ID = 0;

    Shader(const char* vertexPath, const char* fragmentPath)
    {
        std::string vertexCode = readFile(vertexPath);
        std::string fragmentCode = readFile(fragmentPath);

        unsigned int vertex = compile(GL_VERTEX_SHADER, vertexCode.c_str(), "VERTEX");
        unsigned int fragment = compile(GL_FRAGMENT_SHADER, fragmentCode.c_str(), "FRAGMENT");

        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);

        int success = 0;
        char log[1024];
        glGetProgramiv(ID, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(ID, 1024, nullptr, log);
            std::cout << "Error enlazando shaders:\n" << log << std::endl;
        }

        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }

    void use() const
    {
        glUseProgram(ID);
    }

    void destroy()
    {
        glDeleteProgram(ID);
        ID = 0;
    }

private:
    static std::string readFile(const char* path)
    {
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
        unsigned int shader = glCreateShader(type);
        glShaderSource(shader, 1, &code, nullptr);
        glCompileShader(shader);

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
