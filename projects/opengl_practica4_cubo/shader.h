#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class Shader
{
public:
    unsigned int ID; // ID del programa de shaders

    // Constructor: lee los archivos y compila los shaders
    Shader(const char* vertexPath, const char* fragmentPath)
    {
        // 1. Leer el código fuente desde los archivos
        std::string vertexCode;
        std::string fragmentCode;
        std::ifstream vShaderFile;
        std::ifstream fShaderFile;

        // Asegurarse de que ifstream pueda lanzar excepciones
        vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

        try
        {
            std::cout << "Cargando vertex shader:   " << vertexPath   << std::endl;
            std::cout << "Cargando fragment shader: " << fragmentPath << std::endl;

            vShaderFile.open(vertexPath);
            fShaderFile.open(fragmentPath);

            std::stringstream vShaderStream, fShaderStream;

            // Leer el contenido del buffer del archivo al stream
            vShaderStream << vShaderFile.rdbuf();
            fShaderStream << fShaderFile.rdbuf();

            vShaderFile.close();
            fShaderFile.close();

            // Convertir el stream a string
            vertexCode   = vShaderStream.str();
            fragmentCode = fShaderStream.str();

            if (vertexCode.empty())
            {
                std::cout << "ERROR::SHADER: El archivo vertex shader esta vacio o no se pudo leer: "
                          << vertexPath << std::endl;
            }
            if (fragmentCode.empty())
            {
                std::cout << "ERROR::SHADER: El archivo fragment shader esta vacio o no se pudo leer: "
                          << fragmentPath << std::endl;
            }
        }
        catch (std::ifstream::failure& e)
        {
            std::cout << "ERROR::SHADER::FILE_NOT_FOUND\n"
                      << "  Vertex:   " << vertexPath   << "\n"
                      << "  Fragment: " << fragmentPath << "\n"
                      << "  Detalle:  " << e.what()     << "\n"
                      << "  Asegurate de ejecutar el programa desde la carpeta raiz del proyecto."
                      << std::endl;
        }

        const char* vShaderCode = vertexCode.c_str();
        const char* fShaderCode = fragmentCode.c_str();

        // 2. Compilar los shaders
        unsigned int vertex, fragment;

        // Vertex Shader
        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, NULL);
        glCompileShader(vertex);
        checkCompileErrors(vertex, "VERTEX");

        // Fragment Shader
        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, NULL);
        glCompileShader(fragment);
        checkCompileErrors(fragment, "FRAGMENT");

        // Programa de Shaders (link)
        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");

        // Eliminar shaders individuales (ya están linkeados al programa)
        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }

    // Activar el shader
    void use()
    {
        glUseProgram(ID);
    }

    // Limpiar el programa al terminar
    void del()
    {
        glDeleteProgram(ID);
    }

private:
    // Verificar errores de compilación/linkeo
    void checkCompileErrors(unsigned int shader, std::string type)
    {
        int success;
        char infoLog[1024];
        if (type != "PROGRAM")
        {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success)
            {
                glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::SHADER_COMPILATION_ERROR de tipo: " << type << "\n"
                          << infoLog << "\n -- --------------------------------------------------- -- "
                          << std::endl;
            }
        }
        else
        {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success)
            {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::PROGRAM_LINKING_ERROR de tipo: " << type << "\n"
                          << infoLog << "\n -- --------------------------------------------------- -- "
                          << std::endl;
            }
        }
    }
};

#endif
