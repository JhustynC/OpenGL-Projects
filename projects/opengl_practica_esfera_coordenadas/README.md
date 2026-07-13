# Practica: coordenadas esfericas

Practica independiente para estudiar como una esfera se aproxima mediante
triangulos y como los angulos phi y theta producen puntos (x,y,z).

## Controles

- A/D: disminuir/aumentar theta alrededor del eje Z.
- W/S: aumentar/disminuir phi entre los dos polos.
- Z/X: disminuir/aumentar stacks.
- C/V: disminuir/aumentar slices.
- 1: modo solido con cuadricula esferica.
- 2: modo malla para observar los triangulos.
- Espacio: pausar o continuar la rotacion.
- R: restaurar valores iniciales.
- ESC: salir.

La barra de titulo muestra stacks, slices, triangulos, angulos y la coordenada
local del punto rojo.

## Archivos

- main.cpp: generacion de la esfera, buffers, controles y dibujo.
- shader.h: compilacion y enlace de shaders.
- shaders/vertex_shader.glsl: transformaciones y normales.
- shaders/fragment_shader.glsl: Phong y cuadricula phi/theta.
- GUIA_ESFERA.md: explicacion conceptual detallada.
