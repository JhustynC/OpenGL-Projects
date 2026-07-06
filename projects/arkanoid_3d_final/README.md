# Arkanoid 3D Final

Base migrada desde el prototipo en Python/PyOpenGL al formato usado en las practicas: C++17, GLFW, GLAD, VAO/VBO y shaders GLSL.

## Controles

- ENTER: iniciar desde el menu principal o continuar desde pausa.
- WASD o flechas: mover el paddle en X/Z.
- ESPACIO: pausa.
- T: activar/desactivar estela.
- +/-: ajustar brillo de la UI.
- R: reiniciar partida.
- ESC: salir.

El puntaje, las vidas, bloques restantes y estados del juego se muestran en una UI estilo arcade sobre la escena 3D.

## Estructura

- `main.cpp`: inicializacion, loop principal, input global y coordinacion.
- `config.h`: constantes de ventana, camara, cuarto y rutas de shaders.
- `math_utils.h`: vectores y matrices manuales.
- `mesh.h`: VAO/VBO y geometria base.
- `game.h`: entidades, estados, colisiones y reglas.
- `rendering.h`: render 3D de escenario, bloques, bola, estela y paddle.
- `ui.h`: HUD, menus y texto pixel-art.
- `shader.h`: carga y compilacion de shaders GLSL.
