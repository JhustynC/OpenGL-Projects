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
    vec4 worldPosition = uModel * vec4(aPos, 1.0);
    FragPos = worldPosition.xyz;
    Normal = normalize(transpose(inverse(mat3(uModel))) * aNormal);
    LocalPos = aPos;
    gl_Position = uMVP * vec4(aPos, 1.0);
}
