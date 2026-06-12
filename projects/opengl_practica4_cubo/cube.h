#ifndef CUBE_H
#define CUBE_H

#include <glad/glad.h>
#include <vector>

// ============================================================================
// CLASE Cube
// ----------------------------------------------------------------------
// Representa el cubo mediante su TOPOLOGIA (vertices + caras) y construye
// a partir de ella la REPRESENTACION GEOMETRICA (lista de vertices,
// lista de colores y lista de indices = EBO) para enviar a la GPU.
//
//                    Topologia                     Geometria
//   Cube  ---->  Caras (A..F)  ---->  cada cara tiene 4 vertices
//                                      (indices al arreglo de 8 vertices)
//                                          |
//                                          v
//                              vertices[], colors[], indices[]
//
// COLOREADO :
// El cubo de COLOR RGB tiene sus 8 esquinas en Negro, Rojo, Verde, Azul,
// Amarillo, Magenta, Cyan y Blanco. Cada esquina de color corresponde a
// una combinacion de (R,G,B) en {0,1}^3.
//
// Aqui asignamos a cada uno de los 8 vertices GEOMETRICOS del cubo
// (posiciones en {-0.5,+0.5}^3) un color segun esa misma combinacion de
// signos: por ejemplo, el vertice con (x,y,z) = (+,+,+) recibe el color
// (R,G,B) = (1,1,1) = Blanco, el vertice (-,-,-) recibe (0,0,0) = Negro,
// etc. Cada CARA queda entonces con 4 vertices que pueden tener colores
// DISTINTOS, y la GPU interpola automaticamente esos colores a lo largo
// de la superficie de la cara (igual que en la practica del rectangulo).
// ============================================================================
class Cube
{
public:
    // ------------------------------------------------------------------
    // 1. REPRESENTACION TOPOLOGICA
    // ------------------------------------------------------------------

    // 8 vertices unicos del cubo (posiciones en espacio local, lado = 1.0)
    //
    //        7-------6
    //       /|      /|
    //      4-------5 |
    //      | |     | |
    //      | 3-----|-2
    //      |/      |/
    //      0-------1
    //
    static constexpr int NUM_VERTICES = 8;
    static constexpr float VERTEX_LIST[NUM_VERTICES][3] = {
        { -0.5f, -0.5f,  0.5f }, // 0
        {  0.5f, -0.5f,  0.5f }, // 1
        {  0.5f,  0.5f,  0.5f }, // 2
        { -0.5f,  0.5f,  0.5f }, // 3
        { -0.5f, -0.5f, -0.5f }, // 4
        {  0.5f, -0.5f, -0.5f }, // 5
        {  0.5f,  0.5f, -0.5f }, // 6
        { -0.5f,  0.5f, -0.5f }  // 7
    };

    // Color asignado a CADA UNO DE LOS 8 VERTICES, siguiendo el cubo de
    // color RGB: el signo de cada coordenada (x,y,z) determina si el
    // componente correspondiente (R,G,B) es 0 o 1.
    //
    //   Vertice   (x,y,z)        Color (R,G,B)         Nombre
    //   0  (-,-,+)  -> (0,0,1)  Azul
    //   1  (+,-,+)  -> (1,0,1)  Magenta
    //   2  (+,+,+)  -> (1,1,1)  Blanco
    //   3  (-,+,+)  -> (0,1,1)  Cyan
    //   4  (-,-,-)  -> (0,0,0)  Negro
    //   5  (+,-,-)  -> (1,0,0)  Rojo
    //   6  (+,+,-)  -> (1,1,0)  Amarillo
    //   7  (-,+,-)  -> (0,1,0)  Verde
    static constexpr float VERTEX_COLOR[NUM_VERTICES][4] = {
        { 0.0f, 0.0f, 1.0f, 1.0f }, // 0 -> Azul
        { 1.0f, 0.0f, 1.0f, 1.0f }, // 1 -> Magenta
        { 1.0f, 1.0f, 1.0f, 1.0f }, // 2 -> Blanco
        { 0.0f, 1.0f, 1.0f, 1.0f }, // 3 -> Cyan
        { 0.0f, 0.0f, 0.0f, 1.0f }, // 4 -> Negro
        { 1.0f, 0.0f, 0.0f, 1.0f }, // 5 -> Rojo
        { 1.0f, 1.0f, 0.0f, 1.0f }, // 6 -> Amarillo
        { 0.0f, 1.0f, 0.0f, 1.0f }  // 7 -> Verde
    };

    // 6 caras (A..F). Cada cara referencia 4 de los 8 vertices anteriores.
    // Este es el corazon de la "representacion topologica": no son
    // coordenadas, son INDICES que apuntan a la lista de vertices.
    //
    //   Cara A: frente   (z = +0.5)
    //   Cara B: derecha  (x = +0.5)
    //   Cara C: atras    (z = -0.5)
    //   Cara D: izquierda(x = -0.5)
    //   Cara E: arriba   (y = +0.5)
    //   Cara F: abajo    (y = -0.5)
    static constexpr int NUM_CARAS = 6;
    static constexpr int CARA_VERTICES[NUM_CARAS][4] = {
        { 0, 1, 2, 3 }, // A - frente
        { 1, 5, 6, 2 }, // B - derecha
        { 5, 4, 7, 6 }, // C - atras
        { 4, 0, 3, 7 }, // D - izquierda
        { 3, 2, 6, 7 }, // E - arriba
        { 4, 5, 1, 0 }  // F - abajo
    };

    // ------------------------------------------------------------------
    // 2. REPRESENTACION GEOMETRICA (generada a partir de la topologia)
    // ------------------------------------------------------------------
    std::vector<float> positions; // x,y,z por vertice (4 vertices x 6 caras = 24)
    std::vector<float> colors;    // r,g,b,a por vertice (24 vertices)
    std::vector<unsigned int> indices; // EBO: 6 indices por cara (2 triangulos) x 6 caras = 36

    // Construye las listas planas (geometria) recorriendo la topologia.
    //
    // Por que duplicar vertices (24 en vez de 8)?
    // Aunque ahora cada vertice topologico tiene un color FIJO (no depende
    // de la cara), seguimos necesitando 24 vertices porque cada cara debe
    // poder tener su propia normal/orientacion en el futuro y, sobre todo,
    // porque el EBO de cada cara necesita un grupo de 4 indices consecutivos
    // para formar sus 2 triangulos. Lo importante para la interpolacion es
    // que el COLOR de cada copia es el color de su vertice topologico
    // original (VERTEX_COLOR), de modo que vertices compartidos entre
    // caras conservan el mismo color, y dentro de cada cara la GPU
    // interpola entre los 4 colores de sus esquinas.
    Cube()
    {
        positions.reserve(NUM_CARAS * 4 * 3);
        colors.reserve(NUM_CARAS * 4 * 4);
        indices.reserve(NUM_CARAS * 6);

        for (int cara = 0; cara < NUM_CARAS; cara++)
        {
            unsigned int base = (unsigned int)(cara * 4); // primer indice de esta cara en las listas planas

            // Recorrer los 4 vertices topologicos de esta cara
            for (int v = 0; v < 4; v++)
            {
                int idxVertice = CARA_VERTICES[cara][v];

                // Posicion: copiada del vertice topologico correspondiente
                positions.push_back(VERTEX_LIST[idxVertice][0]);
                positions.push_back(VERTEX_LIST[idxVertice][1]);
                positions.push_back(VERTEX_LIST[idxVertice][2]);

                // Color: el del VERTICE topologico (no de la cara), segun
                // el cubo de color RGB. Esto permite que la GPU interpole
                // entre los colores de las 4 esquinas de cada cara.
                colors.push_back(VERTEX_COLOR[idxVertice][0]);
                colors.push_back(VERTEX_COLOR[idxVertice][1]);
                colors.push_back(VERTEX_COLOR[idxVertice][2]);
                colors.push_back(VERTEX_COLOR[idxVertice][3]);
            }

            // Indices (EBO): cada cara (cuadrilatero) = 2 triangulos
            // usando los 4 vertices recien agregados (base+0..base+3)
            //
            //   3-------2
            //   |     / |
            //   |   /   |
            //   | /     |
            //   0-------1
            indices.push_back(base + 0);
            indices.push_back(base + 1);
            indices.push_back(base + 2);

            indices.push_back(base + 0);
            indices.push_back(base + 2);
            indices.push_back(base + 3);
        }
    }

    // Tamaños en bytes, utiles para glBufferData / glBufferSubData
    size_t positionsSizeBytes() const { return positions.size() * sizeof(float); }
    size_t colorsSizeBytes()    const { return colors.size()    * sizeof(float); }
    size_t indicesSizeBytes()   const { return indices.size()   * sizeof(unsigned int); }
    int    indexCount()         const { return (int)indices.size(); }
};

#endif