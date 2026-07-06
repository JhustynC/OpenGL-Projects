#ifndef ARKANOID_MATH_UTILS_H
#define ARKANOID_MATH_UTILS_H

#include <cmath>

#include "config.h"

struct Vec3
{
    float x, y, z;
};

inline Vec3 sub3(Vec3 a, Vec3 b)
{
    return { a.x - b.x, a.y - b.y, a.z - b.z };
}

inline float dot3(Vec3 a, Vec3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline Vec3 cross3(Vec3 a, Vec3 b)
{
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

inline Vec3 normalize3(Vec3 v)
{
    float len = sqrtf(dot3(v, v));
    if (len <= 0.00001f) return { 0.0f, 0.0f, 0.0f };
    return { v.x / len, v.y / len, v.z / len };
}

inline void identity(float* m)
{
    for (int i = 0; i < 16; i++) m[i] = 0.0f;
    m[0] = 1.0f;
    m[5] = 1.0f;
    m[10] = 1.0f;
    m[15] = 1.0f;
}

inline void matMul(float* out, const float* A, const float* B)
{
    float tmp[16] = {};
    for (int col = 0; col < 4; col++)
        for (int row = 0; row < 4; row++)
            for (int k = 0; k < 4; k++)
                tmp[col * 4 + row] += A[k * 4 + row] * B[col * 4 + k];

    for (int i = 0; i < 16; i++) out[i] = tmp[i];
}

inline void buildPerspective(float* m, float fovDeg, float aspect, float nearP, float farP)
{
    float f = 1.0f / tanf(fovDeg * PI / 360.0f);
    for (int i = 0; i < 16; i++) m[i] = 0.0f;
    m[0]  = f / aspect;
    m[5]  = f;
    m[10] = (farP + nearP) / (nearP - farP);
    m[11] = -1.0f;
    m[14] = (2.0f * farP * nearP) / (nearP - farP);
}

inline void buildOrtho(float* m, float left, float right, float bottom, float top, float nearP, float farP)
{
    for (int i = 0; i < 16; i++) m[i] = 0.0f;
    m[0] = 2.0f / (right - left);
    m[5] = 2.0f / (top - bottom);
    m[10] = -2.0f / (farP - nearP);
    m[12] = -(right + left) / (right - left);
    m[13] = -(top + bottom) / (top - bottom);
    m[14] = -(farP + nearP) / (farP - nearP);
    m[15] = 1.0f;
}

inline void buildLookAt(float* m, Vec3 eye, Vec3 center, Vec3 up)
{
    Vec3 f = normalize3(sub3(center, eye));
    Vec3 s = normalize3(cross3(f, up));
    Vec3 u = cross3(s, f);

    identity(m);
    m[0] = s.x;  m[4] = s.y;  m[8]  = s.z;
    m[1] = u.x;  m[5] = u.y;  m[9]  = u.z;
    m[2] = -f.x; m[6] = -f.y; m[10] = -f.z;

    m[12] = -dot3(s, eye);
    m[13] = -dot3(u, eye);
    m[14] =  dot3(f, eye);
}

inline void buildTranslation(float* m, float tx, float ty, float tz)
{
    identity(m);
    m[12] = tx;
    m[13] = ty;
    m[14] = tz;
}

inline void buildScale(float* m, float sx, float sy, float sz)
{
    identity(m);
    m[0] = sx;
    m[5] = sy;
    m[10] = sz;
}

inline void buildRotationX(float* m, float angle)
{
    identity(m);
    float c = cosf(angle), s = sinf(angle);
    m[5] = c;
    m[6] = s;
    m[9] = -s;
    m[10] = c;
}

inline void buildRotationY(float* m, float angle)
{
    identity(m);
    float c = cosf(angle), s = sinf(angle);
    m[0] = c;
    m[2] = -s;
    m[8] = s;
    m[10] = c;
}

inline void buildRotationZ(float* m, float angle)
{
    identity(m);
    float c = cosf(angle), s = sinf(angle);
    m[0] = c;
    m[1] = s;
    m[4] = -s;
    m[5] = c;
}

inline void buildModel(float* m,
                       float tx, float ty, float tz,
                       float sx, float sy, float sz,
                       float rx = 0.0f, float ry = 0.0f, float rz = 0.0f)
{
    float T[16], S[16], RX[16], RY[16], RZ[16], tmp1[16], tmp2[16], tmp3[16];

    buildTranslation(T, tx, ty, tz);
    buildScale(S, sx, sy, sz);
    buildRotationX(RX, rx);
    buildRotationY(RY, ry);
    buildRotationZ(RZ, rz);

    matMul(tmp1, RX, S);
    matMul(tmp2, RY, tmp1);
    matMul(tmp3, RZ, tmp2);
    matMul(m, T, tmp3);
}

#endif
