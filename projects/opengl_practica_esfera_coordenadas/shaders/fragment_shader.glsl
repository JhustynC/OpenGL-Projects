#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec3 LocalPos;

out vec4 FragColor;

uniform vec3 uColor;
uniform vec3 uLightPos;
uniform vec3 uCameraPos;
uniform bool uIsSphere;
uniform int uStacks;
uniform int uSlices;

const float PI = 3.14159265;

float boundaryLine(float coordinate)
{
    float part = fract(coordinate);
    float distanceToBoundary = min(part, 1.0 - part);
    return 1.0 - smoothstep(0.015, 0.065, distanceToBoundary);
}

void main()
{
    if (!uIsSphere)
    {
        FragColor = vec4(uColor, 1.0);
        return;
    }

    vec3 localDirection = normalize(LocalPos);
    float phi = asin(clamp(localDirection.z, -1.0, 1.0));
    float theta = atan(localDirection.y, localDirection.x);
    if (theta < 0.0)
        theta += 2.0 * PI;

    float stackCoordinate = ((phi + PI * 0.5) / PI) * float(uStacks);
    float sliceCoordinate = (theta / (2.0 * PI)) * float(uSlices);
    float grid = max(boundaryLine(stackCoordinate),
                     boundaryLine(sliceCoordinate));

    vec3 N = normalize(Normal);
    vec3 L = normalize(uLightPos - FragPos);
    vec3 V = normalize(uCameraPos - FragPos);
    vec3 R = reflect(-L, N);

    vec3 ambient = 0.18 * uColor;
    vec3 diffuse = 0.72 * max(dot(N, L), 0.0) * uColor;
    vec3 specular = 0.45 * pow(max(dot(V, R), 0.0), 36.0) * vec3(1.0);
    vec3 litColor = ambient + diffuse + specular;

    vec3 finalColor = mix(litColor, vec3(0.95, 0.98, 1.0), grid * 0.88);
    FragColor = vec4(finalColor, 1.0);
}
