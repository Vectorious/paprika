#pragma once

#define PI32 3.141592653589793f
#define EPSILON 1e-8f

#define V3_UP V3(0.0f, 1.0f, 0.0f)
#define V3_FORWARD V3(0.0f, 0.0f, -1.0f)
#define V3_RIGHT V3(1.0f, 0.0f, 0.0f)


#ifndef PAPRIKA_TYPES

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

#endif


internal inline i32
Round(f64 d)
{
    union Cast
    {
        f64 d;
        i64 l;
    };
    volatile Cast c;

    // f32 nearest = 6755399441055743.5f;
    f32 ceil = 6755399441055744.0f;
    c.d = d + ceil;

    return (i32)c.l;
}

internal inline Vector2
V2(f32 x = 0.0f, f32 y = 0.0f)
{
    return {x, y};
}

internal inline IVector2
IV2(i32 x = 0, i32 y = 0)
{
    return {x, y};
}

internal inline Vector3
V3(f32 x = 0.0f, f32 y = 0.0f, f32 z = 0.0f)
{
    return {x, y, z};
}

internal inline Vector3
V3V2Z(Vector2 xy, f32 z)
{
    return V3(xy.x, xy.y, z);
}

internal inline IVector3
IV3(i32 x = 0, i32 y = 0, i32 z = 0)
{
    return {x, y, z};
}

internal inline Vector4
V4(f32 x = 0.0f, f32 y = 0.0f, f32 z = 0.0f, f32 w = 0.0f)
{
    return {x, y, z, w};
}

internal inline IVector4
IV4(i32 x = 0, i32 y = 0, i32 z = 0, i32 w = 0)
{
    return {x, y, z, w};
}

internal inline Vector4
V4V3(Vector3 xyz, f32 w = 0.0f)
{
    return V4(xyz.x, xyz.y, xyz.z, w);
}

internal inline Matrix4
Transpose(Matrix4 a)
{
    Matrix4 result;

    result.elems[0][0] = a.elems[0][0];
    result.elems[0][1] = a.elems[1][0];
    result.elems[0][2] = a.elems[2][0];
    result.elems[0][3] = a.elems[3][0];
    result.elems[1][0] = a.elems[0][1];
    result.elems[1][1] = a.elems[1][1];
    result.elems[1][2] = a.elems[2][1];
    result.elems[2][0] = a.elems[0][2];
    result.elems[2][1] = a.elems[1][2];
    result.elems[2][2] = a.elems[2][2];
    result.elems[3][0] = a.elems[0][3];
    result.elems[3][1] = a.elems[1][3];
    result.elems[3][2] = a.elems[2][3];

    return result;
}

internal inline Matrix4
Mat4(f32 ax, f32 ay, f32 az, f32 aw,
     f32 bx, f32 by, f32 bz, f32 bw,
     f32 cx, f32 cy, f32 cz, f32 cw,
     f32 dx, f32 dy, f32 dz, f32 dw)
{
    Matrix4 result;

    result.cols[0] = V4(ax, bx, cx, dx);
    result.cols[1] = V4(ay, by, cy, dy);
    result.cols[2] = V4(az, bz, cz, dz);
    result.cols[3] = V4(aw, bw, cw, dw);

    return result;
}

internal inline Matrix4
Mat4Identity()
{
    Matrix4 result = Mat4(1, 0, 0, 0,
                          0, 1, 0, 0,
                          0, 0, 1, 0,
                          0, 0, 0, 1);
    return result;
}

internal inline Matrix4
Mat4Scale(Vector3 scale)
{
    Matrix4 result = Mat4(scale.x, 0, 0, 0,
                          0, scale.y, 0, 0,
                          0, 0, scale.z, 0,
                          0, 0, 0, 1);
    return result;
}

internal inline Matrix4
Mat4Rotation(Quaternion q)
{
    f32 xx = Square(q.x);
    f32 yy = Square(q.y);
    f32 zz = Square(q.z);

    f32 xy = q.x * q.y;
    f32 xz = q.x * q.z;
    f32 xw = q.x * q.w;
    f32 yz = q.y * q.z;
    f32 yw = q.y * q.w;
    f32 zw = q.z * q.w;

    Matrix4 result;

    result.elems[0][0] = 1.0f - 2.0f * (yy + zz);
    result.elems[0][1] = 2.0f * (xy + zw);
    result.elems[0][2] = 2.0f * (xz - yw);
    result.elems[0][3] = 0;

    result.elems[1][0] = 2.0f * (xy - zw);
    result.elems[1][1] = 1.0f - 2.0f * (xx + zz);
    result.elems[1][2] = 2.0f * (yz + xw);
    result.elems[1][3] = 0;

    result.elems[2][0] = 2.0f * (xz + yw);
    result.elems[2][1] = 2.0f * (yz - xw);
    result.elems[2][2] = 1.0f - 2.0f * (xx + yy);
    result.elems[2][3] = 0;

    result.elems[3][0] = 0;
    result.elems[3][1] = 0;
    result.elems[3][2] = 0;
    result.elems[3][3] = 1;

    return result;
}

internal inline Matrix4
Mat4Translation(f32 x, f32 y, f32 z)
{
    Matrix4 result = Mat4Identity();

    result.elems[3][0] = x;
    result.elems[3][1] = y;
    result.elems[3][2] = z;

    return result;
}

internal inline Matrix4
Mat4Translation(Vector3 translate)
{
    return Mat4Translation(translate.x, translate.y, translate.z);
}

internal inline Matrix4
Mat4Rotation(Vector3 axis, f32 angle)
{
    f32 c = cosf(angle);
    f32 s = sinf(angle);
    f32 ic = 1.0f - c;

    Matrix4 result;

    result.elems[0][0] = axis.x * axis.x * ic + c;
    result.elems[0][1] = axis.x * axis.y * ic + axis.z * s;
    result.elems[0][2] = axis.x * axis.z * ic - axis.y * s;
    result.elems[0][3] = 0;

    result.elems[1][0] = axis.y * axis.x * ic - axis.z * s;
    result.elems[1][1] = axis.y * axis.y * ic + c;
    result.elems[1][2] = axis.y * axis.z * ic + axis.x * s;
    result.elems[1][3] = 0;

    result.elems[2][0] = axis.z * axis.x * ic + axis.y * s;
    result.elems[2][1] = axis.z * axis.y * ic - axis.x * s;
    result.elems[2][2] = axis.z * axis.z * ic + c;
    result.elems[2][3] = 0;

    result.cols[3] = V4(0, 0, 0, 1);

    return result;
}

internal inline Quaternion
QIdentity()
{
    return V4(0.0f, 0.0f, 0.0f, 1.0f);
}

internal inline Vector2
operator+(Vector2 a, Vector2 b)
{
    Vector2 result;

    result.x = a.x + b.x;
    result.y = a.y + b.y;

    return result;
}

internal inline IVector2
operator+(IVector2 a, IVector2 b)
{
    IVector2 result;

    result.x = a.x + b.x;
    result.y = a.y + b.y;

    return result;
}

internal inline Vector3
operator+(Vector3 a, Vector3 b)
{
    Vector3 result;

    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;

    return result;
}

internal inline IVector3
operator+(IVector3 a, IVector3 b)
{
    IVector3 result;

    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;

    return result;
}

internal inline Vector4
operator+(Vector4 a, Vector4 b)
{
    Vector4 result;

    result.w = a.w + b.w;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;

    return result;
}

internal inline IVector4
operator+(IVector4 a, IVector4 b)
{
    IVector4 result;

    result.w = a.w + b.w;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;

    return result;
}

internal inline Vector2
operator-(Vector2 a)
{
    Vector2 result;

    result.x = -a.x;
    result.y = -a.y;

    return result;
}

internal inline IVector2
operator-(IVector2 a)
{
    IVector2 result;

    result.x = -a.x;
    result.y = -a.y;

    return result;
}

internal inline Vector3
operator-(Vector3 a)
{
    Vector3 result;

    result.x = -a.x;
    result.y = -a.y;
    result.z = -a.z;

    return result;
}

internal inline IVector3
operator-(IVector3 a)
{
    IVector3 result;

    result.x = -a.x;
    result.y = -a.y;
    result.z = -a.z;

    return result;
}

internal inline Vector4
operator-(Vector4 a)
{
    Vector4 result;

    result.w = -a.w;
    result.x = -a.x;
    result.y = -a.y;
    result.z = -a.z;

    return result;
}

internal inline IVector4
operator-(IVector4 a)
{
    IVector4 result;

    result.w = -a.w;
    result.x = -a.x;
    result.y = -a.y;
    result.z = -a.z;

    return result;
}

internal inline Vector2
operator-(Vector2 a, Vector2 b)
{
    Vector2 result;

    result.x = a.x - b.x;
    result.y = a.y - b.y;

    return result;
}

internal inline IVector2
operator-(IVector2 a, IVector2 b)
{
    IVector2 result;

    result.x = a.x - b.x;
    result.y = a.y - b.y;

    return result;
}

internal inline Vector3
operator-(Vector3 a, Vector3 b)
{
    Vector3 result;

    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;

    return result;
}

internal inline IVector3
operator-(IVector3 a, IVector3 b)
{
    IVector3 result;

    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;

    return result;
}

internal inline Vector4
operator-(Vector4 a, Vector4 b)
{
    Vector4 result;

    result.w = a.w - b.w;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;

    return result;
}

internal inline IVector4
operator-(IVector4 a, IVector4 b)
{
    IVector4 result;

    result.w = a.w - b.w;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;

    return result;
}

internal inline Vector2
operator*(f32 a, Vector2 b)
{
    Vector2 result;

    result.x = a * b.x;
    result.y = a * b.y;

    return result;
}

internal inline IVector2
operator*(i32 a, IVector2 b)
{
    IVector2 result;

    result.x = a * b.x;
    result.y = a * b.y;

    return result;
}

internal inline Vector3
operator*(f32 a, Vector3 b)
{
    Vector3 result;

    result.x = a * b.x;
    result.y = a * b.y;
    result.z = a * b.z;

    return result;
}

internal inline IVector3
operator*(i32 a, IVector3 b)
{
    IVector3 result;

    result.x = a * b.x;
    result.y = a * b.y;
    result.z = a * b.z;

    return result;
}

internal inline Vector4
operator*(f32 a, Vector4 b)
{
    Vector4 result;

    result.x = a * b.x;
    result.y = a * b.y;
    result.z = a * b.z;
    result.w = a * b.w;

    return result;
}

internal inline IVector4
operator*(i32 a, IVector4 b)
{
    IVector4 result;

    result.x = a * b.x;
    result.y = a * b.y;
    result.z = a * b.z;
    result.w = a * b.w;

    return result;
}

internal inline Vector2
operator*(Vector2 a, f32 b)
{
    return b * a;
}

internal inline IVector2
operator*(IVector2 a, i32 b)
{
    return b * a;
}

internal inline Vector3
operator*(Vector3 a, f32 b)
{
    return b * a;
}

internal inline IVector3
operator*(IVector3 a, f32 b)
{
    return b * a;
}

internal inline Vector4
operator*(Vector4 a, f32 b)
{
    return b * a;
}

internal inline Vector3
operator*(Matrix4 a, Vector3 b)
{
    Vector3 result;

    result.x = b.x * a.elems[0][0] + b.x * a.elems[1][0] + b.x * a.elems[2][0];
    result.y = b.y * a.elems[0][1] + b.y * a.elems[1][1] + b.y * a.elems[2][1];
    result.z = b.z * a.elems[0][2] + b.z * a.elems[1][2] + b.z * a.elems[2][2];

    return result;
}

internal inline Vector4
operator*(Matrix4 a, Vector4 b)
{
    Vector4 result;

    result.x = b.x * a.elems[0][0] + b.x * a.elems[1][0] + b.x * a.elems[2][0] + b.x * a.elems[3][0];
    result.y = b.y * a.elems[0][1] + b.y * a.elems[1][1] + b.y * a.elems[2][1] + b.y * a.elems[3][1];
    result.z = b.z * a.elems[0][2] + b.z * a.elems[1][2] + b.z * a.elems[2][2] + b.z * a.elems[3][2];
    result.w = b.w * a.elems[0][3] + b.w * a.elems[1][3] + b.w * a.elems[2][3] + b.w * a.elems[3][3];

    return result;
}

internal inline Matrix4
operator*(Matrix4 a, Matrix4 b)
{
    Matrix4 result;

    result.cols[0] = a * b.cols[0];
    result.cols[1] = a * b.cols[1];
    result.cols[2] = a * b.cols[2];
    result.cols[3] = a * b.cols[3];

    return result;
}

internal inline Quaternion
operator*(Quaternion a, Quaternion b)
{
    Quaternion result;

    result.x = a.x * b.w + a.y * b.z - a.z * b.y + a.w * b.x;
    result.y = -a.x * b.z + a.y * b.w + a.z * b.x + a.w * b.y;
    result.z = a.x * b.y - a.y * b.x + a.z * b.w + a.w * b.z;
    result.w = -a.x * b.x - a.y * b.y - a.z * b.z + a.w * b.w;

    return result;
}

internal inline Vector2
operator/(Vector2 a, f32 b)
{
    Vector2 result;

    result.x = a.x / b;
    result.y = a.y / b;

    return result;
}

internal inline Vector3
operator/(Vector3 a, f32 b)
{
    Vector3 result;

    result.x = a.x / b;
    result.y = a.y / b;
    result.z = a.z / b;

    return result;
}

internal inline Vector4
operator/(Vector4 a, f32 b)
{
    Vector4 result;

    result.w = a.w / b;
    result.x = a.x / b;
    result.y = a.y / b;
    result.z = a.z / b;

    return result;
}

internal inline bool32
operator==(Vector2 a, Vector2 b)
{
    return (fabs(a.x - b.x) < EPSILON) && ((a.y - b.y) < EPSILON);
}

internal inline bool32
operator==(IVector2 a, IVector2 b)
{
    return (a.x == b.x) && (a.y == b.y);
}

internal inline bool32
operator!=(IVector2 a, IVector2 b)
{
    return !(a == b);
}

inline Vector2 Vector2::
operator+=(Vector2 a)
{
    *this = *this + a;
    return *this;
}

inline IVector2 IVector2::
operator+=(IVector2 a)
{
    *this = *this + a;
    return *this;
}

inline Vector3 Vector3::
operator+=(Vector3 a)
{
    *this = *this + a;
    return *this;
}

inline IVector3 IVector3::
operator+=(IVector3 a)
{
    *this = *this + a;
    return *this;
}

inline Vector4 Vector4::
operator+=(Vector4 a)
{
    *this = *this + a;
    return *this;
}

inline IVector4 IVector4::
operator+=(IVector4 a)
{
    *this = *this + a;
    return *this;
}

inline Vector2 Vector2::
operator-=(Vector2 a)
{
    *this = *this - a;
    return *this;
}

inline IVector2 IVector2::
operator-=(IVector2 a)
{
    *this = *this - a;
    return *this;
}

inline Vector3 Vector3::
operator-=(Vector3 a)
{
    *this = *this - a;
    return *this;
}

inline IVector3 IVector3::
operator-=(IVector3 a)
{
    *this = *this - a;
    return *this;
}

inline Vector4 Vector4::
operator-=(Vector4 a)
{
    *this = *this - a;
    return *this;
}

inline IVector4 IVector4::
operator-=(IVector4 a)
{
    *this = *this - a;
    return *this;
}

inline Vector2 Vector2::
operator*=(f32 a)
{
    *this = a * *this;
    return *this;
}

inline IVector2 IVector2::
operator*=(i32 a)
{
    *this = a * *this;
    return *this;
}

inline Vector3 Vector3::
operator*=(f32 a)
{
    *this = a * *this;
    return *this;
}

inline IVector3 IVector3::
operator*=(i32 a)
{
    *this = a * *this;
    return *this;
}

inline Vector4 Vector4::
operator*=(f32 a)
{
    *this = a * *this;
    return *this;
}

inline IVector4 IVector4::
operator*=(i32 a)
{
    *this = a * *this;
    return *this;
}

inline Quaternion Quaternion::
operator*=(Quaternion a)
{
    *this = *this * a;
    return *this;
}

inline Matrix4 Matrix4::
operator*=(Matrix4 a)
{
    *this = *this * a;
    return *this;
}

internal inline Rect2
Rect2MinMax(Vector2 min, Vector2 max)
{
    return {min, max};
}

internal inline Rect2
Rect2MinDim(Vector2 min, f32 width, f32 height)
{
    Rect2 result = Rect2MinMax(min, min + V2(width, height));
    return result;
}

internal inline f32
Dot(Vector2 a, Vector2 b)
{
    f32 result = a.x * b.x + a.y * b.y;
    return result;
}

internal inline f32
Dot(Vector3 a, Vector3 b)
{
    f32 result = a.x * b.x + a.y * b.y + a.z * b.z;
    return result;
}

internal inline f32
Dot(Vector4 a, Vector4 b)
{
    f32 result = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    return result;
}

internal inline f32
LengthSq(Vector2 a)
{
    return Dot(a, a);
}

internal inline f32
LengthSq(Vector3 a)
{
    return Dot(a, a);
}

internal inline f32
LengthSq(Vector4 a)
{
    return Dot(a, a);
}

internal inline f32
Length(Vector2 a)
{
    return SquareRt(LengthSq(a));
}

internal inline f32
Length(Vector3 a)
{
    return SquareRt(LengthSq(a));
}

internal inline f32
Length(Vector4 a)
{
    return SquareRt(LengthSq(a));
}

internal inline Vector2
Normalize(Vector2 a)
{
    Vector2 result = a;

    f32 length = Length(a);
    if (length != 0.0f)
    {
        result = a / length;
    }

    return result;
}

internal inline Vector3
Normalize(Vector3 a)
{
    Vector3 result = a;

    f32 length = Length(a);
    if (length != 0.0f)
    {
        result = a / length;
    }

    return result;
}

internal inline Vector4
Normalize(Vector4 a)
{
    Vector4 result = a;

    f32 length = Length(a);
    if (length != 0.0f)
    {
        result = a / length;
    }

    return result;
}

internal inline Vector3
Cross(Vector3 a, Vector3 b)
{
    Vector3 result;

    result.x = a.y * b.z - a.z * b.y;
    result.y = a.z * b.x - a.x * b.z;
    result.z = a.x * b.y - a.y * b.x;

    return result;
}

internal inline Vector2
Hadamard(Vector2 a, Vector2 b)
{
    Vector2 result;

    result.x = a.x * b.x;
    result.y = a.y * b.y;

    return result;
}

internal inline IVector2
Hadamard(IVector2 a, IVector2 b)
{
    IVector2 result;

    result.x = a.x * b.x;
    result.y = a.y * b.y;

    return result;
}

internal inline Vector3
Hadamard(Vector3 a, Vector3 b)
{
    Vector3 result;

    result.x = a.x * b.x;
    result.y = a.y * b.y;
    result.z = a.z * b.z;

    return result;
}

internal inline IVector3
Hadamard(IVector3 a, IVector3 b)
{
    IVector3 result;

    result.x = a.x * b.x;
    result.y = a.y * b.y;
    result.z = a.z * b.z;

    return result;
}

internal inline IVector4
Hadamard(IVector4 a, IVector4 b)
{
    IVector4 result;

    result.x = a.x * b.x;
    result.y = a.y * b.y;
    result.z = a.z * b.z;
    result.w = a.w * b.w;

    return result;
}

internal inline Quaternion
Inverse(Quaternion q)
{
    Quaternion result;

    result.x = -q.x;
    result.y = -q.y;
    result.z = -q.z;
    result.w = q.w;

    return result;
}

internal inline Vector2
Rotate(Vector2 v, Quaternion q)
{
    Vector3 v3 = {v.x, v.y, 0.0f};
    Vector3 t = 2.0f * Cross(q.xyz, v3);
    Vector3 rotated = v3 + q.w * t + Cross(q.xyz, t);

    Vector2 result = rotated.xy;

    return result;
}

internal inline Vector3
Rotate(Vector3 v, Quaternion q)
{
    Quaternion inv_q = Inverse(q);
    Vector3 t = 2.0f * Cross(inv_q.xyz, v);
    Vector3 result = v + inv_q.w * t + Cross(inv_q.xyz, t);

    return result;
}

internal inline Quaternion
QuaternionFromAngle(f32 x, f32 y, f32 z, f32 rad)
{
    f32 half_rad = rad * 0.5f;
    f32 s = sinf(half_rad);

    Quaternion result;

    result.x = s * x;
    result.y = s * y;
    result.z = s * z;
    result.w = cosf(half_rad);

    return result;
}

internal inline Quaternion
QuaternionFromAngle(Vector3 angle, f32 rad)
{
    return QuaternionFromAngle(angle.x, angle.y, angle.z, rad);
}

internal inline f32
DegToRad(f32 deg)
{
    return deg * ((2.0f * PI32) / 360.0f);
}

internal inline f32
RadToDeg(f32 rad)
{
    return rad * (360.0f / (2.0f * PI32));
}

internal inline u8
Lerp(u8 a, u8 b,
     f32 alpha)
{
    return ((1.0f - alpha) * a) + (alpha * b);
}

internal inline u32
Lerp(u32 a, u32 b,
     f32 alpha)
{
    return ((1.0f - alpha) * a) + (alpha * b);
}

internal inline i32
Lerp(i32 a, i32 b,
     f32 alpha)
{
    return ((1.0f - alpha) * a) + (alpha * b);
}

internal inline f32
Lerp(f32 a, f32 b,
     f32 alpha)
{
    return ((1.0f - alpha) * a) + (alpha * b);
}

internal inline Vector2
Lerp(Vector2 a, Vector2 b,
     f32 alpha)
{
    return ((1.0f - alpha) * a) + (alpha * b);
}

internal inline Vector3
Lerp(Vector3 a, Vector3 b,
     f32 alpha)
{
    return ((1.0f - alpha) * a) + (alpha * b);
}

internal inline Vector4
Lerp(Vector4 a, Vector4 b,
     f32 alpha)
{
    return ((1.0f - alpha) * a) + (alpha * b);
}

internal inline Matrix4
PerspectiveProjection(f32 fov, f32 aspect_ratio, f32 z_near, f32 z_far)
{
    Matrix4 result = {};

    f32 cotan = 1.0f / tanf(fov * (PI32 / 360.0f));

    result.elems[0][0] = cotan / aspect_ratio;
    result.elems[1][1] = cotan;
    result.elems[2][3] = -1.0f;
    result.elems[2][2] = (z_near + z_far) / (z_near - z_far);
    result.elems[3][2] = (2.0f * z_near * z_far) / (z_near - z_far);
    result.elems[3][3] = 0;

    return result;
}

internal inline Matrix4
OrthographicProjection(f32 left, f32 right, f32 bottom, f32 top, f32 z_near, f32 z_far)
{
    Matrix4 result = {};

    result.elems[0][0] = 2.0f / (right - left);
    result.elems[1][1] = 2.0f / (top - bottom);
    result.elems[2][2] = 2.0f / (z_near - z_far);
    result.elems[3][3] = 1;
    result.elems[3][0] = (left + right) / (left - right);
    result.elems[3][1] = (bottom + top) / (bottom - top);
    result.elems[3][2] = (z_far + z_near) / (z_near - z_far);

    return result;
}

internal inline Matrix4
LookAt(Vector3 camera_position, Vector3 camera_target, Vector3 up = V3_UP)
{
    Vector3 camera_direction = Normalize(camera_position - camera_target);
    Vector3 camera_right = Normalize(Cross(up, camera_direction));
    Vector3 camera_up = Cross(camera_direction, camera_right);

    Matrix4 result;

    result.elems[0][0] = camera_right.x;
    result.elems[0][1] = camera_up.x;
    result.elems[0][2] = -(camera_direction.x);
    result.elems[0][3] = 0;

    result.elems[1][0] = camera_right.y;
    result.elems[1][1] = camera_up.y;
    result.elems[1][2] = -(camera_direction.y);
    result.elems[1][3] = 0;

    result.elems[2][0] = camera_right.z;
    result.elems[2][1] = camera_up.z;
    result.elems[2][2] = -(camera_direction.z);
    result.elems[2][3] = 0;

    result.elems[3][0] = -Dot(camera_right, camera_position);
    result.elems[3][1] = -Dot(camera_up, camera_position);
    result.elems[3][2] = Dot(camera_direction, camera_position);
    result.elems[3][3] = 1;

    return result;
}

#define PIXELS_PER_METER 294.0f
#define FROM_SCREEN_SCALE (1.0f / PIXELS_PER_METER)
#define TO_SCREEN_SCALE PIXELS_PER_METER

internal inline bool32
IsInRect(Rect2 rect, Vector2 point)
{
    // NOTE: this is currently inclusive minimum to exclusive maximum
    bool32 result = ((rect.min.x <= point.x) && (rect.min.y <= point.y) &&
                     (point.x < rect.max.x) && (point.y < rect.max.y));
    return result;
}

internal inline Vector2
ScreenToWorldPosition(Vector2 camera, Vector2 screen_position)
{
    Vector2 result = (camera + screen_position) * FROM_SCREEN_SCALE;
    return result;
}

internal inline Vector2
WorldToScreenPosition(Vector2 camera, Vector2 world_position)
{
    Vector2 result = (world_position - camera) * TO_SCREEN_SCALE;
    return result;
}

internal inline bool32
PointInCircle(Vector2 point, Vector2 circle_center, f32 circle_radius,
              f32 margin = 0.0f)
{
    f32 radius = Max(0.0f, circle_radius - margin);
    return LengthSq(point - circle_center) <= Square(radius);
}
