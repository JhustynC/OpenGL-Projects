#ifndef ARKANOID_UI_H
#define ARKANOID_UI_H

#include <GLFW/glfw3.h>

#include <array>
#include <cmath>
#include <sstream>
#include <string>
#include <vector>

#include "config.h"
#include "game.h"
#include "rendering.h"
#include "shader.h"

inline std::array<const char*, 7> glyphFor(char c)
{
    switch (c)
    {
        case 'A': return { "01110","10001","10001","11111","10001","10001","10001" };
        case 'B': return { "11110","10001","10001","11110","10001","10001","11110" };
        case 'C': return { "01111","10000","10000","10000","10000","10000","01111" };
        case 'D': return { "11110","10001","10001","10001","10001","10001","11110" };
        case 'E': return { "11111","10000","10000","11110","10000","10000","11111" };
        case 'F': return { "11111","10000","10000","11110","10000","10000","10000" };
        case 'G': return { "01111","10000","10000","10111","10001","10001","01111" };
        case 'H': return { "10001","10001","10001","11111","10001","10001","10001" };
        case 'I': return { "11111","00100","00100","00100","00100","00100","11111" };
        case 'J': return { "00111","00010","00010","00010","10010","10010","01100" };
        case 'K': return { "10001","10010","10100","11000","10100","10010","10001" };
        case 'L': return { "10000","10000","10000","10000","10000","10000","11111" };
        case 'M': return { "10001","11011","10101","10101","10001","10001","10001" };
        case 'N': return { "10001","11001","10101","10011","10001","10001","10001" };
        case 'O': return { "01110","10001","10001","10001","10001","10001","01110" };
        case 'P': return { "11110","10001","10001","11110","10000","10000","10000" };
        case 'Q': return { "01110","10001","10001","10001","10101","10010","01101" };
        case 'R': return { "11110","10001","10001","11110","10100","10010","10001" };
        case 'S': return { "01111","10000","10000","01110","00001","00001","11110" };
        case 'T': return { "11111","00100","00100","00100","00100","00100","00100" };
        case 'U': return { "10001","10001","10001","10001","10001","10001","01110" };
        case 'V': return { "10001","10001","10001","10001","10001","01010","00100" };
        case 'W': return { "10001","10001","10001","10101","10101","10101","01010" };
        case 'X': return { "10001","10001","01010","00100","01010","10001","10001" };
        case 'Y': return { "10001","10001","01010","00100","00100","00100","00100" };
        case 'Z': return { "11111","00001","00010","00100","01000","10000","11111" };
        case '0': return { "01110","10001","10011","10101","11001","10001","01110" };
        case '1': return { "00100","01100","00100","00100","00100","00100","01110" };
        case '2': return { "01110","10001","00001","00010","00100","01000","11111" };
        case '3': return { "11110","00001","00001","01110","00001","00001","11110" };
        case '4': return { "00010","00110","01010","10010","11111","00010","00010" };
        case '5': return { "11111","10000","10000","11110","00001","00001","11110" };
        case '6': return { "01110","10000","10000","11110","10001","10001","01110" };
        case '7': return { "11111","00001","00010","00100","01000","01000","01000" };
        case '8': return { "01110","10001","10001","01110","10001","10001","01110" };
        case '9': return { "01110","10001","10001","01111","00001","00001","01110" };
        case ':': return { "00000","00100","00100","00000","00100","00100","00000" };
        case '-': return { "00000","00000","00000","11111","00000","00000","00000" };
        case '/': return { "00001","00001","00010","00100","01000","10000","10000" };
        case '+': return { "00000","00100","00100","11111","00100","00100","00000" };
        case '.': return { "00000","00000","00000","00000","00000","01100","01100" };
        default:  return { "00000","00000","00000","00000","00000","00000","00000" };
    }
}

inline float textWidth(const std::string& text, float scale)
{
    return (float)text.size() * 6.0f * scale;
}

inline void drawRect2D(Shader& shader, const Mesh& quad, const float* ortho,
                       float x, float y, float w, float h,
                       float r, float g, float b, float a)
{
    drawMesh(shader, quad, ortho,
             x + w * 0.5f, y + h * 0.5f, 0.0f,
             w, h, 1.0f,
             r, g, b, a,
             false, true, false);
}

inline void drawText2D(Shader& shader, const Mesh& quad, const float* ortho,
                       const std::string& text, float x, float y, float scale,
                       float r, float g, float b, float a)
{
    float cursorX = x;
    for (char raw : text)
    {
        char c = (raw >= 'a' && raw <= 'z') ? (char)(raw - 32) : raw;
        if (c == ' ')
        {
            cursorX += 6.0f * scale;
            continue;
        }

        auto glyph = glyphFor(c);
        for (int row = 0; row < 7; row++)
        {
            for (int col = 0; col < 5; col++)
            {
                if (glyph[row][col] == '1')
                {
                    drawRect2D(shader, quad, ortho,
                               cursorX + col * scale,
                               y + row * scale,
                               scale * 0.82f, scale * 0.82f,
                               r, g, b, a);
                }
            }
        }
        cursorX += 6.0f * scale;
    }
}

inline void drawTextCentered2D(Shader& shader, const Mesh& quad, const float* ortho,
                               const std::string& text, float centerX, float y, float scale,
                               float r, float g, float b, float a)
{
    drawText2D(shader, quad, ortho, text,
               centerX - textWidth(text, scale) * 0.5f, y, scale,
               r, g, b, a);
}

inline void updateWindowTitle(GLFWwindow* window, int score, int lives, bool paused, bool won)
{
    std::ostringstream title;
    title << "Arkanoid 3D | Score: " << score << " | Lives: " << lives;

    if (won)
        title << " | GANASTE - R para reiniciar";
    else if (lives <= 0)
        title << " | GAME OVER - R para reiniciar";
    else if (paused)
        title << " | PAUSA";

    glfwSetWindowTitle(window, title.str().c_str());
}

inline void drawHudPanel(Shader& shader, const Mesh& quad, const float* ortho,
                         const std::string& text, float x, float y,
                         float w, float h, float uiGlow)
{
    drawRect2D(shader, quad, ortho, x + 3.0f, y + 3.0f, w, h, 0.0f, 0.0f, 0.0f, 0.38f);
    drawRect2D(shader, quad, ortho, x, y, w, h, 0.03f, 0.09f, 0.14f, 0.62f);
    drawRect2D(shader, quad, ortho, x, y, w, 3.0f, 0.0f, 0.95f, 1.0f, 0.65f * uiGlow);
    drawText2D(shader, quad, ortho, text, x + 16.0f, y + 16.0f, 3.0f,
               0.95f, 0.98f, 1.0f, 0.95f);
}

inline void drawUiOverlay(Shader& shader, const Mesh& quad, const float* ortho,
                          GameState state, int score, int lives, int blocksLeft,
                          bool trailEnabled, float uiGlow, float now)
{
    float pulse = 0.72f + 0.28f * fabsf(sinf(now * 3.2f));
    std::ostringstream scoreText, livesText, blocksText;
    scoreText << "SCORE: " << score;
    livesText << "LIVES: " << lives;
    blocksText << "BLOCKS: " << blocksLeft;

    drawHudPanel(shader, quad, ortho, scoreText.str(), 24.0f, 22.0f, 220.0f, 58.0f, uiGlow);
    drawHudPanel(shader, quad, ortho, livesText.str(), 268.0f, 22.0f, 190.0f, 58.0f, uiGlow);
    drawHudPanel(shader, quad, ortho, blocksText.str(), 482.0f, 22.0f, 230.0f, 58.0f, uiGlow);

    if (state == GameState::Playing)
        return;

    drawRect2D(shader, quad, ortho, 0.0f, 0.0f, (float)SCR_WIDTH, (float)SCR_HEIGHT,
               0.0f, 0.0f, 0.0f, state == GameState::Start ? 0.42f : 0.55f);

    if (state == GameState::Start)
    {
        drawRect2D(shader, quad, ortho, 245.0f, 145.0f, 610.0f, 340.0f, 0.03f, 0.08f, 0.13f, 0.72f);
        drawRect2D(shader, quad, ortho, 245.0f, 145.0f, 610.0f, 6.0f, 0.0f, 0.95f, 1.0f, 0.85f * uiGlow);
        drawRect2D(shader, quad, ortho, 245.0f, 479.0f, 610.0f, 6.0f, 1.0f, 0.08f, 0.02f, 0.85f * uiGlow);
        drawTextCentered2D(shader, quad, ortho, "ARKANOID 3D", SCR_WIDTH * 0.5f, 185.0f, 9.0f, 0.0f, 0.95f, 1.0f, 0.95f);
        drawTextCentered2D(shader, quad, ortho, "NEON BREAKOUT", SCR_WIDTH * 0.5f, 272.0f, 4.0f, 1.0f, 0.14f, 0.04f, 0.95f);
        drawTextCentered2D(shader, quad, ortho, "ENTER PARA INICIAR", SCR_WIDTH * 0.5f, 345.0f, 4.0f, 1.0f, 1.0f, 1.0f, pulse);
        drawTextCentered2D(shader, quad, ortho, "WASD/FLECHAS MOVER  SPACE PAUSA", SCR_WIDTH * 0.5f, 410.0f, 3.0f, 0.70f, 0.92f, 1.0f, 0.92f);
        return;
    }

    drawRect2D(shader, quad, ortho, 305.0f, 155.0f, 490.0f, 405.0f, 0.025f, 0.065f, 0.10f, 0.82f);
    drawRect2D(shader, quad, ortho, 305.0f, 155.0f, 490.0f, 5.0f, 0.0f, 0.95f, 1.0f, 0.9f * uiGlow);
    drawRect2D(shader, quad, ortho, 305.0f, 555.0f, 490.0f, 5.0f, 1.0f, 0.08f, 0.02f, 0.9f * uiGlow);

    if (state == GameState::Paused)
    {
        drawTextCentered2D(shader, quad, ortho, "PAUSA", SCR_WIDTH * 0.5f, 195.0f, 8.0f, 0.0f, 0.95f, 1.0f, 0.95f);
        drawTextCentered2D(shader, quad, ortho, "AJUSTES", SCR_WIDTH * 0.5f, 282.0f, 4.0f, 1.0f, 0.14f, 0.04f, 0.95f);
        drawTextCentered2D(shader, quad, ortho, trailEnabled ? "T ESTELA: ON" : "T ESTELA: OFF", SCR_WIDTH * 0.5f, 340.0f, 3.5f, 0.90f, 0.98f, 1.0f, 0.95f);
        drawTextCentered2D(shader, quad, ortho, "+/- BRILLO UI", SCR_WIDTH * 0.5f, 386.0f, 3.5f, 0.90f, 0.98f, 1.0f, 0.95f);
        drawTextCentered2D(shader, quad, ortho, "SPACE CONTINUAR", SCR_WIDTH * 0.5f, 458.0f, 3.5f, 1.0f, 1.0f, 1.0f, pulse);
        drawTextCentered2D(shader, quad, ortho, "R REINICIAR", SCR_WIDTH * 0.5f, 505.0f, 3.0f, 0.70f, 0.92f, 1.0f, 0.9f);
        return;
    }

    if (state == GameState::GameOver)
    {
        drawTextCentered2D(shader, quad, ortho, "GAME OVER", SCR_WIDTH * 0.5f, 210.0f, 8.0f, 1.0f, 0.08f, 0.02f, 0.98f);
        drawTextCentered2D(shader, quad, ortho, scoreText.str(), SCR_WIDTH * 0.5f, 320.0f, 4.0f, 1.0f, 1.0f, 1.0f, 0.95f);
        drawTextCentered2D(shader, quad, ortho, "R REINICIAR", SCR_WIDTH * 0.5f, 405.0f, 4.0f, 0.0f, 0.95f, 1.0f, pulse);
        drawTextCentered2D(shader, quad, ortho, "ESC SALIR", SCR_WIDTH * 0.5f, 468.0f, 3.0f, 0.70f, 0.92f, 1.0f, 0.90f);
        return;
    }

    drawTextCentered2D(shader, quad, ortho, "GANASTE", SCR_WIDTH * 0.5f, 210.0f, 8.0f, 0.0f, 0.95f, 1.0f, 0.98f);
    drawTextCentered2D(shader, quad, ortho, scoreText.str(), SCR_WIDTH * 0.5f, 320.0f, 4.0f, 1.0f, 1.0f, 1.0f, 0.95f);
    drawTextCentered2D(shader, quad, ortho, "R JUGAR OTRA VEZ", SCR_WIDTH * 0.5f, 405.0f, 4.0f, 1.0f, 0.14f, 0.04f, pulse);
}

#endif
