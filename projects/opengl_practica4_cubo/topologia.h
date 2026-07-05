#ifndef TOPOLOGIA_H
#define TOPOLOGIA_H

#include <glad/glad.h>
#include <vector>
#include <array>

// ============================================================================
// CLASE Vertex
// ============================================================================
// Representa un vértice con posición y color en el espacio 3D.
// ============================================================================
struct Vertex
{
    float x, y, z;           // Posición 3D
    float r, g, b, a;        // Color RGBA

    Vertex() : x(0), y(0), z(0), r(1), g(1), b(1), a(1) {}

    Vertex(float x, float y, float z, float r, float g, float b, float a = 1.0f)
        : x(x), y(y), z(z), r(r), g(g), b(b), a(a) {}
};

// ============================================================================
// CLASE Edge
// ============================================================================
// Representa una arista del cubo mediante INDICES a dos vértices únicos.
// Una arista conecta dos vértices del cubo.
// ============================================================================
class Edge
{
public:
    int v0_idx, v1_idx;  // Índices de los 2 vértices que conecta la arista

    Edge() : v0_idx(-1), v1_idx(-1) {}

    // Constructor: recibe 2 índices que apuntan a vértices en la lista global
    Edge(int v0, int v1) : v0_idx(v0), v1_idx(v1) {}

    // Método para agregar la arista como línea para GL_LINES
    // Útil para renderizar wireframe
    void addToGeometry(const std::array<Vertex, 8>& vertices_topo,
                       std::vector<float>& positions,
                       std::vector<float>& colors) const
    {
        // Agregar el primer vértice de la arista
        const Vertex& v0 = vertices_topo[v0_idx];
        positions.push_back(v0.x);
        positions.push_back(v0.y);
        positions.push_back(v0.z);
        colors.push_back(v0.r);
        colors.push_back(v0.g);
        colors.push_back(v0.b);
        colors.push_back(v0.a);

        // Agregar el segundo vértice de la arista
        const Vertex& v1 = vertices_topo[v1_idx];
        positions.push_back(v1.x);
        positions.push_back(v1.y);
        positions.push_back(v1.z);
        colors.push_back(v1.r);
        colors.push_back(v1.g);
        colors.push_back(v1.b);
        colors.push_back(v1.a);
    }
};

// ============================================================================
// CLASE Face
// ============================================================================
// Representa una cara del cubo mediante INDICES a los vértices únicos.
// Una cara está compuesta por 4 índices que apuntan a la lista de vértices.
// La clase genera automáticamente los índices para los 2 triángulos que 
// forman la cara (para el EBO).
// ============================================================================
class Face
{
public:
    std::array<int, 4> vertexIndices;    // Índices de los 4 vértices de la cara
    std::array<unsigned int, 6> indices; // Los 6 índices del EBO (2 triángulos)

    Face() {}

    // Constructor: recibe 4 índices que apuntan a vértices en la lista global
    Face(int idx0, int idx1, int idx2, int idx3)
        : vertexIndices({idx0, idx1, idx2, idx3})
    {
        // Generar índices para 2 triángulos (base = 0, relativo a esta cara)
        //   3-------2
        //   |     / |
        //   |   /   |
        //   | /     |
        //   0-------1
        indices[0] = 0; indices[1] = 1; indices[2] = 2;  // Triángulo 1
        indices[3] = 0; indices[4] = 2; indices[5] = 3;  // Triángulo 2
    }

    // Agregar los vértices y los índices de esta cara a las listas del cubo
    void addToGeometry(const std::array<Vertex, 8>& vertices_topo,
                       std::vector<float>& positions, 
                       std::vector<float>& colors,
                       std::vector<unsigned int>& indices_out,
                       unsigned int baseIndex) const
    {
        // Agregar los 4 vértices de esta cara (obtenidos de los índices)
        for (int idx : vertexIndices)
        {
            const Vertex& v = vertices_topo[idx];
            positions.push_back(v.x);
            positions.push_back(v.y);
            positions.push_back(v.z);

            colors.push_back(v.r);
            colors.push_back(v.g);
            colors.push_back(v.b);
            colors.push_back(v.a);
        }

        // Agregar los 6 índices (ajustados con baseIndex)
        for (unsigned int idx : indices)
        {
            indices_out.push_back(baseIndex + idx);
        }
    }
};

// ============================================================================
// CLASE Cube
// ============================================================================
// Representa un cubo con su topología (vértices, caras, aristas) y su geometría
// ============================================================================
class Cube
{
public:
    // TOPOLOGIA: 8 vértices únicos
    std::array<Vertex, 8> vertices;

    // TOPOLOGIA: 6 caras (cada una con 4 índices a los vértices)
    std::array<Face, 6> faces;

    // TOPOLOGIA: 12 aristas (cada una con 2 índices a los vértices)
    // Las 12 aristas de un cubo: 4 en el plano frontal, 4 en el plano trasero,
    // y 4 verticales que conectan ambos planos.
    std::array<Edge, 12> edges;

    // GEOMETRIA: listas planas para la GPU
    std::vector<float> positions; // x,y,z por vértice
    std::vector<float> colors;    // r,g,b,a por vértice
    std::vector<unsigned int> indices; // EBO: 6 índices por cara (2 triángulos)

    // Constructor: construye la topología y la geometría
    Cube()
    {
        positions.reserve(24 * 3);  // 6 caras * 4 vértices * 3 componentes
        colors.reserve(24 * 4);     // 6 caras * 4 vértices * 4 componentes
        indices.reserve(36);        // 6 caras * 2 triángulos * 3 índices

        // ==================================================================
        // PASO 1: Definir la TOPOLOGIA - los 8 vértices únicos
        // ==================================================================
        // El cubo tiene lado = 1.0, centrado en origen
        //
        //        7-------6
        //       /|      /|
        //      4-------5 |
        //      | |     | |
        //      | 3-----|-2
        //      |/      |/
        //      0-------1
        //
        // Asignar a cada vértice su color según el cubo RGB
        vertices[0] = Vertex(-0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 1.0f, 1.0f); // 0: Azul      (-,-,+)
        vertices[1] = Vertex( 0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 1.0f, 1.0f); // 1: Magenta   (+,-,+)
        vertices[2] = Vertex( 0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 1.0f, 1.0f); // 2: Blanco    (+,+,+)
        vertices[3] = Vertex(-0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 1.0f, 1.0f); // 3: Cyan      (-,+,+)
        vertices[4] = Vertex(-0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 0.0f, 1.0f); // 4: Negro     (-,-,-)
        vertices[5] = Vertex( 0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f, 1.0f); // 5: Rojo      (+,-,-)
        vertices[6] = Vertex( 0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 0.0f, 1.0f); // 6: Amarillo  (+,+,-)
        vertices[7] = Vertex(-0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f, 1.0f); // 7: Verde     (-,+,-)

        // ==================================================================
        // PASO 2: Definir la TOPOLOGIA - las 6 caras con índices a vértices
        // ==================================================================
        faces[0] = Face(0, 1, 2, 3); // Cara A: frente   (z = +0.5)
        faces[1] = Face(1, 5, 6, 2); // Cara B: derecha  (x = +0.5)
        faces[2] = Face(5, 4, 7, 6); // Cara C: atrás    (z = -0.5)
        faces[3] = Face(4, 0, 3, 7); // Cara D: izquierda(x = -0.5)
        faces[4] = Face(3, 2, 6, 7); // Cara E: arriba   (y = +0.5)
        faces[5] = Face(4, 5, 1, 0); // Cara F: abajo    (y = -0.5)

        // ==================================================================
        // PASO 2B: Definir la TOPOLOGIA - las 12 aristas con índices
        // ==================================================================
        // Aristas del plano frontal (z = +0.5)
        edges[0] = Edge(0, 1);   // Arista inferior frontal
        edges[1] = Edge(1, 2);   // Arista derecha frontal
        edges[2] = Edge(2, 3);   // Arista superior frontal
        edges[3] = Edge(3, 0);   // Arista izquierda frontal

        // Aristas del plano trasero (z = -0.5)
        edges[4] = Edge(4, 5);   // Arista inferior trasera
        edges[5] = Edge(5, 6);   // Arista derecha trasera
        edges[6] = Edge(6, 7);   // Arista superior trasera
        edges[7] = Edge(7, 4);   // Arista izquierda trasera

        // Aristas verticales (conectan frente y atrás)
        edges[8]  = Edge(0, 4);  // Arista vertical izquierda-inferior
        edges[9]  = Edge(1, 5);  // Arista vertical derecha-inferior
        edges[10] = Edge(2, 6);  // Arista vertical derecha-superior
        edges[11] = Edge(3, 7);  // Arista vertical izquierda-superior

        // ==================================================================
        // PASO 3: Convertir la TOPOLOGIA a GEOMETRIA para la GPU
        // ==================================================================
        for (int i = 0; i < 6; i++)
        {
            unsigned int baseIndex = (unsigned int)(i * 4);
            faces[i].addToGeometry(vertices, positions, colors, indices, baseIndex);
        }
    }

    // Tamaños en bytes, útiles para glBufferData / glBufferSubData
    size_t positionsSizeBytes() const { return positions.size() * sizeof(float); }
    size_t colorsSizeBytes()    const { return colors.size()    * sizeof(float); }
    size_t indicesSizeBytes()   const { return indices.size()   * sizeof(unsigned int); }
    int    indexCount()         const { return (int)indices.size(); }

    // Construir geometría de wireframe a partir de las aristas
    void buildWireframeGeometry(std::vector<float>& positions_out,
                                std::vector<float>& colors_out) const
    {
        positions_out.clear();
        colors_out.clear();

        // Agregar cada arista como una línea (2 vértices)
        for (const auto& edge : edges)
        {
            edge.addToGeometry(vertices, positions_out, colors_out);
        }
    }
};

#endif