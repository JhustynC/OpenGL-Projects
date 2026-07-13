#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec3 LocalPos;

out vec4 FragColor;

uniform vec3 uColor;
uniform vec3 uLightPos;
uniform vec3 uCameraPos;
uniform bool uUnlit;
uniform bool uIsBall;
uniform float uAlpha;

void main()
{
    // La marca esta unida a la superficie y permite observar el giro.
    if (uIsBall)
    {
        float bandA = abs(sin((LocalPos.x * 5.0 + LocalPos.z * 3.0) * 3.14159265));
        float bandB = abs(sin((LocalPos.y * 6.0 - LocalPos.z * 2.0) * 3.14159265));
        float stripe = step(0.72, max(bandA, bandB));
        vec3 markedColor = mix(uColor, vec3(0.08, 0.18, 0.85), stripe);
        FragColor = vec4(markedColor * 1.25, uAlpha);
        return;
    }

    if (uUnlit)
    {
        FragColor = vec4(uColor, uAlpha);
        return;
    }

    vec3 N = normalize(Normal);
    vec3 lightVector = uLightPos - FragPos;
    float lightDistance = length(lightVector);
    vec3 L = normalize(lightVector);
    vec3 V = normalize(uCameraPos - FragPos);
    vec3 R = reflect(-L, N);

    // Modelo de Phong: ambiental + difusa + especular.
    vec3 ambient = 0.18 * uColor;
    float diffuseFactor = max(dot(N, L), 0.0);
    vec3 diffuse = 0.75 * diffuseFactor * uColor;
    float specularFactor = pow(max(dot(V, R), 0.0), 32.0);
    vec3 specular = 0.45 * specularFactor * vec3(1.0);

    float attenuation = 1.0 / (0.55 + 0.12 * lightDistance
                               + 0.025 * lightDistance * lightDistance);
    FragColor = vec4(ambient + attenuation * (diffuse + specular), uAlpha);
}
