#pragma once


#define PAPRIKA_TYPES


#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float f32;
typedef double f64;

typedef int bool32;

typedef size_t mem_t;

#define internal static


union Vector2
{
    struct
    {
        f32 x;
        f32 y;
    };

    f32 elements[2];

    inline Vector2 operator+=(Vector2 a);
    inline Vector2 operator-=(Vector2 a);
    inline Vector2 operator*=(f32 a);
};

union IVector2
{
    struct
    {
        i32 x;
        i32 y;
    };

    i32 elements[2];

    inline IVector2 operator+=(IVector2 a);
    inline IVector2 operator-=(IVector2 a);
    inline IVector2 operator*=(i32 a);
};

union Vector3
{
    struct
    {
        f32 x;
        f32 y;
        f32 z;
    };

    struct
    {
        f32 r;
        f32 g;
        f32 b;
    };

    struct
    {
        Vector2 xy;
        f32 _ignore_0;
    };

    f32 elements[3];

    inline Vector3 operator+=(Vector3 a);
    inline Vector3 operator-=(Vector3 a);
    inline Vector3 operator*=(f32 a);
};

union IVector3
{
    struct
    {
        i32 x;
        i32 y;
        i32 z;
    };

    i32 elements[3];

    inline IVector3 operator+=(IVector3 a);
    inline IVector3 operator-=(IVector3 a);
    inline IVector3 operator*=(i32 a);
};

union Vector4
{
    struct
    {
        f32 x;
        f32 y;
        f32 z;
        f32 w;
    };

    struct
    {
        f32 r;
        f32 g;
        f32 b;
        f32 a;
    };

    struct
    {
        Vector3 xyz;
        f32 _ignored_0;
    };

    f32 elements[4];

    inline Vector4 operator+=(Vector4 a);
    inline Vector4 operator-=(Vector4 a);
    inline Vector4 operator*=(f32 a);
    inline Vector4 operator*=(Vector4 a);
};

union IVector4
{
    struct
    {
        i32 x;
        i32 y;
        i32 z;
        i32 w;
    };

    i32 elements[4];

    inline IVector4 operator+=(IVector4 a);
    inline IVector4 operator-=(IVector4 a);
    inline IVector4 operator*=(i32 a);
    inline IVector4 operator*=(IVector4 a);
};

typedef Vector4 Quaternion;

// NOTE: This is column-major to match OpenGL default.
union Matrix4
{
    Vector4 cols[4];
    f32 elems[4][4];
    f32 elements[16];

    inline Matrix4 operator*=(Matrix4 a);
};

struct Rect2
{
    Vector2 min;
    Vector2 max;
};
