# Guia de estudio: esfera con stacks y slices

## Idea principal

OpenGL no posee una primitiva esfera. La practica construye una aproximacion
usando una cuadricula de angulos y dos triangulos por cada region.

## Angulos

- phi recorre de -pi/2 a pi/2: polo inferior, ecuador y polo superior.
- theta recorre de 0 a 2pi: una vuelta completa alrededor del eje Z.

## Formula de radio 1

    x = cos(phi) * cos(theta)
    y = cos(phi) * sin(theta)
    z = sin(phi)

sin(phi) determina la altura. cos(phi) determina el radio del anillo
horizontal. theta decide la direccion del punto dentro de ese anillo.

## Una region

Cada combinacion de un intervalo de phi y otro de theta crea cuatro puntos:

    p3 -------- p2   phi1
    |         /  |
    |       /    |
    p0 -------- p1   phi0
    theta0   theta1

Se dibujan los triangulos (p0,p1,p2) y (p2,p3,p0).

## Cantidades

    regiones   = stacks * slices
    triangulos = stacks * slices * 2
    vertices   = stacks * slices * 6

Con 12 stacks y 18 slices se crean 432 triangulos y 1296 entradas de vertice.

## Normal

La esfera local esta centrada en (0,0,0). La normal exterior de un punto es:

    n = normalize(p)

El rasterizador interpola estas normales y Phong produce una apariencia suave.

## Marcador rojo

El punto rojo utiliza exactamente la formula esferica. A/D cambia theta y
W/S cambia phi. La barra de titulo muestra los angulos y el resultado P=(x,y,z).

## Marco local y modelo

La esfera se genera con centro local (0,0,0) y radio local 1. La rotacion forma
parte de la matriz model. El marcador se calcula primero en local y recibe la
misma rotacion, por eso permanece adherido a la superficie.
