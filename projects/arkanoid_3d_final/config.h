#ifndef ARKANOID_CONFIG_H
#define ARKANOID_CONFIG_H

#include <filesystem>
#include <string>

const unsigned int SCR_WIDTH  = 1100;
const unsigned int SCR_HEIGHT = 750;

const float PI = 3.14159265f;

// Coordenadas del mundo:
// X = izquierda/derecha
// Y = profundidad: camara/paddle abajo -> bloques arriba/fondo
// Z = altura visual en pantalla
const float CAMERA_POS[3]  = { 0.0f, -15.8f, 2.4f };
const float CAMERA_LOOK[3] = { 0.0f,  -1.5f, 0.85f };

const float PADDLE_Y    = -9.0f;
const float BALL_RADIUS = 0.25f;

const float ROOM_X_LIMIT = 6.0f;
const float ROOM_Z_MIN   = -2.5f;
const float ROOM_Z_MAX   = 4.5f;
const float BACK_WALL_Y  = 7.2f;
const float LOSE_Y       = -11.0f;

inline std::string getShaderPath(const std::string& filename)
{
    std::filesystem::path srcFile = __FILE__;
    return (srcFile.parent_path() / "shaders" / filename).string();
}

#endif
