#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

uniform mat4 uModel;
uniform mat4 uMVP;

out vec3 FragPos;
out vec3 Normal;
out vec3 LocalPos;

void main()
{
    // LOCAL -> WORLD: la matriz modelo coloca el objeto en el juego.
    vec4 worldPosition = uModel * vec4(aPos, 1.0);
    FragPos = worldPosition.xyz;

    // La normal tambien debe pasar del marco local al marco del mundo.
    mat3 normalMatrix = transpose(inverse(mat3(uModel)));
    Normal = normalize(normalMatrix * aNormal);
    LocalPos = aPos;

    // LOCAL -> WORLD -> VIEW -> CLIP.
    gl_Position = uMVP * vec4(aPos, 1.0);
}
