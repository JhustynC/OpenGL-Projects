#version 330 core

// ------------------------------------------------------------------
// VERSION B: normal de VERTICE = posicion normalizada (porque la
// esfera tiene radio 1 y centro en el origen, la normal en cualquier
// punto de su superficie apunta exactamente en la direccion de ese
// punto desde el origen).
//
// La iluminacion se calcula por vertice con esa normal suave ->
// el rasterizador interpola el color entre vertices adyacentes ->
// resultado: "Gouraud shading", superficie visualmente lisa aunque
// la malla tenga pocos poligonos.
// ------------------------------------------------------------------

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal; // normal de vertice (= aPos normalizado)

out vec4 vertexColor;

uniform mat4 uModel;
uniform mat4 uProjection;

uniform vec3  uLightPos;
uniform vec3  uLightAmbient;
uniform vec3  uLightDiffuse;
uniform vec3  uLightSpecular;

uniform vec3  uMatAmbient;
uniform vec3  uMatDiffuse;
uniform vec3  uMatSpecular;
uniform float uMatShininess;

uniform float uAttC0;
uniform float uAttC1;
uniform float uAttC2;

uniform vec3 uCameraPos;

void main()
{
    vec3 fragPos = vec3(uModel * vec4(aPos, 1.0));

    // La normal de vertice ya es la posicion normalizada (esfera unitaria).
    // Al transformar con el modelo se aplica la matriz de normales.
    mat3 normalMatrix = transpose(inverse(mat3(uModel)));
    vec3 N = normalize(normalMatrix * aNormal);

    vec3 L = uLightPos - fragPos;
    float distancia = length(L);
    L = normalize(L);

    vec3 V = normalize(uCameraPos - fragPos);
    vec3 R = reflect(-L, N);

    float atenuacion = 1.0 / (uAttC0 + uAttC1 * distancia + uAttC2 * distancia * distancia);

    float diffFactor = max(dot(L, N), 0.0);
    float specFactor = 0.0;
    if (diffFactor > 0.0)
        specFactor = pow(max(dot(R, V), 0.0), uMatShininess);

    vec3 ambiental = uMatAmbient   * uLightAmbient;
    vec3 difusa    = uMatDiffuse   * uLightDiffuse  * diffFactor;
    vec3 especular = uMatSpecular  * uLightSpecular * specFactor;

    vec3 color = clamp(ambiental + atenuacion * (difusa + especular), 0.0, 1.0);
    vertexColor = vec4(color, 1.0);

    gl_Position = uProjection * uModel * vec4(aPos, 1.0);
}
