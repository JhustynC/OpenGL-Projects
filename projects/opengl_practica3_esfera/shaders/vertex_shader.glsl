#version 330 core

layout (location = 0) in vec3 aPos;

void main()
{
    // =========================
    // Rotación manual en Y
    // =========================

    float angle = radians(35.0);

    mat3 rotationY = mat3(
         cos(angle), 0.0, sin(angle),
         0.0,        1.0, 0.0,
        -sin(angle), 0.0, cos(angle)
    );

    vec3 rotatedPos = rotationY * aPos;

    // Simular profundidad
    rotatedPos.z -= 2.0;

    // Perspectiva simple manual
    float perspective = 1.0 / -rotatedPos.z;

    gl_Position = vec4(
        rotatedPos.x * perspective,
        rotatedPos.y * perspective,
        0.0,
        1.0
    );
}