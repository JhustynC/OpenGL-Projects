#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec3 LocalPos;

out vec4 FragColor;

uniform vec4 uColor;
uniform vec3 uLightPos;
uniform vec3 uCameraPos;
uniform bool uUseLighting;
uniform bool uIsLight;
uniform bool uIsBall;

void main()
{
    vec4 baseColor = uColor;

    if (uIsBall)
    {
        float bandA = abs(sin((LocalPos.x * 7.0 + LocalPos.z * 5.0) * 3.14159265));
        float bandB = abs(sin((LocalPos.y * 9.0 - LocalPos.z * 4.0) * 3.14159265));
        float stripe = step(0.68, max(bandA, bandB));
        vec3 bodyColor = vec3(0.08, 0.95, 1.0);
        vec3 stripeColor = vec3(1.0, 0.08, 0.02);
        vec3 markedColor = mix(bodyColor, stripeColor, stripe);
        FragColor = vec4(markedColor * 1.35, baseColor.a);
        return;
    }

    if (uIsLight || !uUseLighting)
    {
        FragColor = baseColor;
        return;
    }

    vec3 N = normalize(Normal);
    vec3 L = uLightPos - FragPos;
    float distanceToLight = length(L);
    L = normalize(L);

    vec3 V = normalize(uCameraPos - FragPos);
    vec3 R = reflect(-L, N);

    vec3 lightAmbient = vec3(0.08, 0.11, 0.16);
    vec3 lightDiffuse = vec3(0.65, 0.95, 1.0);
    vec3 lightSpecular = vec3(1.0, 1.0, 1.0);

    float diff = max(dot(N, L), 0.0);
    float spec = 0.0;
    if (diff > 0.0)
        spec = pow(max(dot(V, R), 0.0), 48.0);

    float attenuation = 1.0 / (0.65 + 0.07 * distanceToLight + 0.015 * distanceToLight * distanceToLight);

    vec3 ambient = lightAmbient * baseColor.rgb;
    vec3 diffuse = lightDiffuse * baseColor.rgb * diff;
    vec3 specular = lightSpecular * spec * 0.55;
    vec3 color = ambient + attenuation * (diffuse + specular);

    FragColor = vec4(color, baseColor.a);
}
