#ifndef SPHERE_H
#define SPHERE_H

#include <vector>
#include <cmath>
#include <map>

// ============================================================================
// CLASE Sphere
// ----------------------------------------------------------------------------
// Genera una esfera aproximada a partir de un ICOSAEDRO (20 caras
// triangulares) mediante subdivisiones sucesivas.
//
// Algoritmo (igual que en la diapositiva):
//   1. Partir de un icosaedro inscrito en la esfera unitaria.
//   2. Por cada triangulo, calcular los 3 puntos medios de sus aristas.
//   3. NORMALIZAR cada punto medio (p^ = p/|p|) -> proyectarlo sobre la esfera.
//   4. Reemplazar el triangulo original por 4 sub-triangulos.
//   5. Repetir N veces.
// ============================================================================
class Sphere
{
public:

    // Estructura simple para un punto 3D (evita problemas con float[3] y std::array)
    struct V3 {
        float x, y, z;
        V3() : x(0), y(0), z(0) {}
        V3(float x, float y, float z) : x(x), y(y), z(z) {}
    };

    struct Face {
        int a, b, c;
        Face() : a(0), b(0), c(0) {}
        Face(int a, int b, int c) : a(a), b(b), c(c) {}
    };

    std::vector<float> positions; // x,y,z por vertice
    std::vector<float> normals;   // nx,ny,nz por vertice (flat o smooth segun modo)

    // numSubdivisiones: 3=~320 tri, 4=~1280 tri, 5=~5120 tri
    // flatNormals: true = normal de cara (Version A), false = normal de vertice (Version B)
    Sphere(int numSubdivisiones = 4, bool flatNormals = true)
    {
        // ------------------------------------------------------------------
        // 1. Icosaedro inicial (12 vertices, 20 triangulos)
        // ------------------------------------------------------------------
        const float t = (1.0f + sqrtf(5.0f)) / 2.0f; // razon aurea

        std::vector<V3> v;

        // 12 vertices del icosaedro, normalizados para que queden en la esfera
        auto addVert = [&](float x, float y, float z) {
            float len = sqrtf(x*x + y*y + z*z);
            v.push_back(V3(x/len, y/len, z/len));
        };

        addVert(-1,  t,  0); addVert( 1,  t,  0);
        addVert(-1, -t,  0); addVert( 1, -t,  0);
        addVert( 0, -1,  t); addVert( 0,  1,  t);
        addVert( 0, -1, -t); addVert( 0,  1, -t);
        addVert( t,  0, -1); addVert( t,  0,  1);
        addVert(-t,  0, -1); addVert(-t,  0,  1);

        // 20 caras del icosaedro
        std::vector<Face> faces;
        faces.push_back(Face(0,11,5));  faces.push_back(Face(0,5,1));
        faces.push_back(Face(0,1,7));   faces.push_back(Face(0,7,10));
        faces.push_back(Face(0,10,11)); faces.push_back(Face(1,5,9));
        faces.push_back(Face(5,11,4));  faces.push_back(Face(11,10,2));
        faces.push_back(Face(10,7,6));  faces.push_back(Face(7,1,8));
        faces.push_back(Face(3,9,4));   faces.push_back(Face(3,4,2));
        faces.push_back(Face(3,2,6));   faces.push_back(Face(3,6,8));
        faces.push_back(Face(3,8,9));   faces.push_back(Face(4,9,5));
        faces.push_back(Face(2,4,11));  faces.push_back(Face(6,2,10));
        faces.push_back(Face(8,6,7));   faces.push_back(Face(9,8,1));

        // ------------------------------------------------------------------
        // 2. Subdividir N veces
        // ------------------------------------------------------------------
        for (int sub = 0; sub < numSubdivisiones; sub++)
        {
            std::vector<Face> faces2;
            std::map<std::pair<int,int>, int> midCache;

            auto midPoint = [&](int a, int b) -> int
            {
                auto key = std::make_pair(std::min(a,b), std::max(a,b));
                auto it  = midCache.find(key);
                if (it != midCache.end()) return it->second;

                // Punto medio entre v[a] y v[b]
                float mx = (v[a].x + v[b].x) * 0.5f;
                float my = (v[a].y + v[b].y) * 0.5f;
                float mz = (v[a].z + v[b].z) * 0.5f;
                // Normalizar: proyectar sobre la esfera unitaria (p^ = p/|p|)
                float len = sqrtf(mx*mx + my*my + mz*mz);
                v.push_back(V3(mx/len, my/len, mz/len));
                int idx = (int)v.size() - 1;
                midCache[key] = idx;
                return idx;
            };

            for (int i = 0; i < (int)faces.size(); i++)
            {
                Face& f = faces[i];
                int ma = midPoint(f.a, f.b);
                int mb = midPoint(f.b, f.c);
                int mc = midPoint(f.c, f.a);
                faces2.push_back(Face(f.a, ma, mc));
                faces2.push_back(Face(f.b, mb, ma));
                faces2.push_back(Face(f.c, mc, mb));
                faces2.push_back(Face(ma,  mb, mc));
            }
            faces = faces2;
        }

        // ------------------------------------------------------------------
        // 3. Construir los arreglos planos de posiciones y normales
        // ------------------------------------------------------------------
        positions.reserve(faces.size() * 3 * 3);
        normals.reserve(faces.size() * 3 * 3);

        for (int i = 0; i < (int)faces.size(); i++)
        {
            Face& f = faces[i];
            V3& p0 = v[f.a];
            V3& p1 = v[f.b];
            V3& p2 = v[f.c];

            // Normal de CARA: producto cruzado de dos aristas, normalizado
            float ex = p1.x-p0.x, ey = p1.y-p0.y, ez = p1.z-p0.z;
            float fx = p2.x-p0.x, fy = p2.y-p0.y, fz = p2.z-p0.z;
            float nx = ey*fz - ez*fy;
            float ny = ez*fx - ex*fz;
            float nz = ex*fy - ey*fx;
            float nlen = sqrtf(nx*nx + ny*ny + nz*nz);
            if (nlen > 0.0f) { nx/=nlen; ny/=nlen; nz/=nlen; }

            V3* pts[3] = { &p0, &p1, &p2 };

            for (int vi = 0; vi < 3; vi++)
            {
                V3* pos = pts[vi];

                positions.push_back(pos->x);
                positions.push_back(pos->y);
                positions.push_back(pos->z);

                if (flatNormals)
                {
                    // Version A: normal de CARA (igual para los 3 vertices del triangulo)
                    // -> iluminacion facetada, bordes visibles entre triangulos
                    normals.push_back(nx);
                    normals.push_back(ny);
                    normals.push_back(nz);
                }
                else
                {
                    // Version B: normal de VERTICE = posicion normalizada
                    // (la esfera tiene radio 1 y centro en el origen, asi que
                    // la normal en cualquier punto apunta en la misma direccion
                    // que el vector posicion)
                    // -> iluminacion Gouraud, superficie visualmente suave
                    normals.push_back(pos->x);
                    normals.push_back(pos->y);
                    normals.push_back(pos->z);
                }
            }
        }
    }

    int    vertexCount()         const { return (int)positions.size() / 3; }
    size_t positionsSizeBytes()  const { return positions.size() * sizeof(float); }
    size_t normalsSizeBytes()    const { return normals.size()   * sizeof(float); }
};

#endif