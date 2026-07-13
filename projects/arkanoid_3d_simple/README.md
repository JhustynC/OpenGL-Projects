# Arkanoid 3D Simple

Version reducida para comprender y presentar los conceptos principales de
Graficos por Computador. El proyecto original no fue modificado.

La bola es una fuente puntual movil y posee marcas superficiales que permiten
observar su rotacion continua y los cambios de giro producidos por colisiones.
La jugabilidad usa los tres ejes: la bola se mueve en X/Y/Z, el paddle se mueve
en X/Z y el escenario forma un cuarto con piso, techo y paredes.
El paddle es translucido mediante blending y los bloques tienen una animacion
de encogimiento, parpadeo y desvanecimiento al ser golpeados.

## Estructura

- `main.cpp`: ventana, geometria, reglas, matrices y ciclo principal.
- `shader.h`: lectura, compilacion y enlace de shaders.
- `shaders/vertex_shader.glsl`: transformacion de local a clip.
- `shaders/fragment_shader.glsl`: iluminacion Phong.
- `GUIA_PRESENTACION.md`: mapa corto para explicar el proyecto.
- `compilar.bat`: recompila el proyecto con las dependencias del curso.

## Controles

- Flechas izquierda/derecha o A/D: mover el paddle.
- Flechas arriba/abajo o W/S: mover el paddle verticalmente.
- R: reiniciar.
- ESC: salir.

## Marcos utilizados

1. Local: vertices del cubo y de la esfera.
2. Mundo: `model` coloca cada objeto en el escenario.
3. Camara: `view = glm::lookAt(...)`.
4. Clip: `projection * view * model` llega a `gl_Position`.
5. NDC: OpenGL divide automaticamente entre `w`.
6. Ventana: `glViewport` transforma NDC a pixeles.

## Compilacion

Ejecutar `compilar.bat`. Usa las mismas dependencias OpenGL de las practicas:
GLAD, GLFW y GLM.
