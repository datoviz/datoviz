/*************************************************************************************************/
/*  Common mathematical macros                                                                   */
/*************************************************************************************************/

#ifndef DVZ_HEADER_MATH
#define DVZ_HEADER_MATH



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <string.h>

#include "_macros.h"
MUTE_ON
#include <cglm/cglm.h>
MUTE_OFF



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define M_2PI 6.28318530717958647692

#define M_INV_255 0.00392156862745098

#define DVZ_ZERO_OFFSET                                                                           \
    (uvec3) { 0, 0, 0 }



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#define MIN(a, b)     (((a) < (b)) ? (a) : (b))
#define MAX(a, b)     (((a) > (b)) ? (a) : (b))
#define CLIP(x, a, b) MAX(MIN((x), (b)), (a))

#define _DMAT4_IDENTITY_INIT                                                                      \
    {                                                                                             \
        {1.0, 0.0, 0.0, 0.0}, {0.0, 1.0, 0.0, 0.0}, {0.0, 0.0, 1.0, 0.0}, { 0.0, 0.0, 0.0, 1.0 }  \
    }



/*************************************************************************************************/
/*  Byte size                                                                                    */
/*************************************************************************************************/

#define GB 1073741824
#define MB 1048576
#define KB 1024

static char _PRETTY_SIZE[64] = {0};

typedef uint64_t DvzSize;

static inline char* pretty_size(DvzSize size)
{
    if (size <= 8192)
    {
        snprintf(_PRETTY_SIZE, 64, "%" PRIu64 " bytes", size);
        return _PRETTY_SIZE;
    }
    float s = (float)size;
    const char* u;
    if (size >= GB)
    {
        s /= GB;
        u = "GB";
    }
    else if (size >= MB)
    {
        s /= MB;
        u = "MB";
    }
    else if (size >= KB)
    {
        s /= KB;
        u = "KB";
    }
    else
    {
        u = "bytes";
    }
    snprintf(_PRETTY_SIZE, 64, "%.3f %s", s, u);
    return _PRETTY_SIZE;
}



/*************************************************************************************************/
/*  8-bit integers                                                                               */
/*************************************************************************************************/

typedef uint8_t cvec2[2];
typedef uint8_t cvec3[3];
typedef uint8_t cvec4[4]; // used for color index



/*************************************************************************************************/
/*  16-bit integers                                                                              */
/*************************************************************************************************/

/* Signed */
typedef int16_t svec2[2];
typedef int16_t svec3[3];
typedef int16_t svec4[4]; // used for glyph as vec4 of uint16

/* Unsigned */
typedef uint16_t usvec2[2];
typedef uint16_t usvec3[3];
typedef uint16_t usvec4[4]; // used for glyph as vec4 of uint16



/*************************************************************************************************/
/*  32-bit integers                                                                              */
/*************************************************************************************************/

/* Signed */
typedef int32_t ivec2[2];
typedef int32_t ivec3[3];
typedef int32_t ivec4[4];

/* Unsigned */
typedef uint32_t uvec2[2];
typedef uint32_t uvec3[3];
typedef uint32_t uvec4[4];

/* Index */
typedef uint32_t DvzIndex;



/*************************************************************************************************/
/*  Single-precision floating-point numbers                                                      */
/*************************************************************************************************/

typedef vec2 fvec2;
typedef vec3 fvec3;
typedef vec4 fvec4;



/*************************************************************************************************/
/*  Double-precision floating-point numbers                                                      */
/*************************************************************************************************/

/* Array types */
typedef double dvec2[2];
typedef double dvec3[3];
typedef double dvec4[4];

/* Matrix types */
typedef dvec2 dmat2[2];
typedef dvec3 dmat3[3];
typedef dvec4 dmat4[4];



/*************************************************************************************************/
/*  Vector operations                                                                            */
/*************************************************************************************************/

static inline void _vec2_copy(const vec2 a, vec2 b)
{
    b[0] = a[0];
    b[1] = a[1];
}



static inline void _vec3_copy(const vec3 a, vec3 b)
{
    b[0] = a[0];
    b[1] = a[1];
    b[2] = a[2];
}



static inline void _vec3_cast(const dvec3* a, vec3* b)
{
    b[0][0] = (float)a[0][0];
    b[0][1] = (float)a[0][1];
    b[0][2] = (float)a[0][2];
}



static inline void _dvec3_copy(const dvec3 a, dvec3 b)
{
    b[0] = a[0];
    b[1] = a[1];
    b[2] = a[2];
}



static inline void _dvec4_copy(const dvec4 a, dvec4 b)
{
    b[0] = a[0];
    b[1] = a[1];
    b[2] = a[2];
    b[3] = a[3];
}



static inline void _dmat4_copy(dmat4 mat, dmat4 dest)
{
    dest[0][0] = mat[0][0];
    dest[1][0] = mat[1][0];
    dest[0][1] = mat[0][1];
    dest[1][1] = mat[1][1];
    dest[0][2] = mat[0][2];
    dest[1][2] = mat[1][2];
    dest[0][3] = mat[0][3];
    dest[1][3] = mat[1][3];

    dest[2][0] = mat[2][0];
    dest[3][0] = mat[3][0];
    dest[2][1] = mat[2][1];
    dest[3][1] = mat[3][1];
    dest[2][2] = mat[2][2];
    dest[3][2] = mat[3][2];
    dest[2][3] = mat[2][3];
    dest[3][3] = mat[3][3];
}



static inline void _dmat4_mat4(mat4 mat, dmat4 dest)
{
    dest[0][0] = mat[0][0];
    dest[1][0] = mat[1][0];
    dest[0][1] = mat[0][1];
    dest[1][1] = mat[1][1];
    dest[0][2] = mat[0][2];
    dest[1][2] = mat[1][2];
    dest[0][3] = mat[0][3];
    dest[1][3] = mat[1][3];

    dest[2][0] = mat[2][0];
    dest[3][0] = mat[3][0];
    dest[2][1] = mat[2][1];
    dest[3][1] = mat[3][1];
    dest[2][2] = mat[2][2];
    dest[3][2] = mat[3][2];
    dest[2][3] = mat[2][3];
    dest[3][3] = mat[3][3];
}



static inline void _dmat4_identity(dmat4 mat)
{
    dmat4 t = _DMAT4_IDENTITY_INIT;
    _dmat4_copy(t, mat);
}



static inline void _dmat4_mul(dmat4 m1, dmat4 m2, dmat4 dest)
{
    double a00 = m1[0][0], a01 = m1[0][1], a02 = m1[0][2], a03 = m1[0][3], a10 = m1[1][0],
           a11 = m1[1][1], a12 = m1[1][2], a13 = m1[1][3], a20 = m1[2][0], a21 = m1[2][1],
           a22 = m1[2][2], a23 = m1[2][3], a30 = m1[3][0], a31 = m1[3][1], a32 = m1[3][2],
           a33 = m1[3][3],

           b00 = m2[0][0], b01 = m2[0][1], b02 = m2[0][2], b03 = m2[0][3], b10 = m2[1][0],
           b11 = m2[1][1], b12 = m2[1][2], b13 = m2[1][3], b20 = m2[2][0], b21 = m2[2][1],
           b22 = m2[2][2], b23 = m2[2][3], b30 = m2[3][0], b31 = m2[3][1], b32 = m2[3][2],
           b33 = m2[3][3];

    dest[0][0] = a00 * b00 + a10 * b01 + a20 * b02 + a30 * b03;
    dest[0][1] = a01 * b00 + a11 * b01 + a21 * b02 + a31 * b03;
    dest[0][2] = a02 * b00 + a12 * b01 + a22 * b02 + a32 * b03;
    dest[0][3] = a03 * b00 + a13 * b01 + a23 * b02 + a33 * b03;
    dest[1][0] = a00 * b10 + a10 * b11 + a20 * b12 + a30 * b13;
    dest[1][1] = a01 * b10 + a11 * b11 + a21 * b12 + a31 * b13;
    dest[1][2] = a02 * b10 + a12 * b11 + a22 * b12 + a32 * b13;
    dest[1][3] = a03 * b10 + a13 * b11 + a23 * b12 + a33 * b13;
    dest[2][0] = a00 * b20 + a10 * b21 + a20 * b22 + a30 * b23;
    dest[2][1] = a01 * b20 + a11 * b21 + a21 * b22 + a31 * b23;
    dest[2][2] = a02 * b20 + a12 * b21 + a22 * b22 + a32 * b23;
    dest[2][3] = a03 * b20 + a13 * b21 + a23 * b22 + a33 * b23;
    dest[3][0] = a00 * b30 + a10 * b31 + a20 * b32 + a30 * b33;
    dest[3][1] = a01 * b30 + a11 * b31 + a21 * b32 + a31 * b33;
    dest[3][2] = a02 * b30 + a12 * b31 + a22 * b32 + a32 * b33;
    dest[3][3] = a03 * b30 + a13 * b31 + a23 * b32 + a33 * b33;
}



static inline void _dmat4_mulv(dmat4 m, dvec4 v, dvec4 dest)
{
    dvec4 res = {0};
    res[0] = m[0][0] * v[0] + m[1][0] * v[1] + m[2][0] * v[2] + m[3][0] * v[3];
    res[1] = m[0][1] * v[0] + m[1][1] * v[1] + m[2][1] * v[2] + m[3][1] * v[3];
    res[2] = m[0][2] * v[0] + m[1][2] * v[1] + m[2][2] * v[2] + m[3][2] * v[3];
    res[3] = m[0][3] * v[0] + m[1][3] * v[1] + m[2][3] * v[2] + m[3][3] * v[3];
    memcpy(dest, res, sizeof(dvec4));
}



static inline void _dvec4(dvec3 v3, double last, dvec4 dest)
{
    dest[0] = v3[0];
    dest[1] = v3[1];
    dest[2] = v3[2];
    dest[3] = last;
}



static inline void _dvec3(dvec4 v4, dvec3 dest)
{
    dest[0] = v4[0];
    dest[1] = v4[1];
    dest[2] = v4[2];
}



static inline void _dmat4_mulv3(dmat4 m, dvec3 v, double last, dvec3 dest)
{
    dvec4 res;
    _dvec4(v, last, res);
    _dmat4_mulv(m, res, res);
    _dvec3(res, dest);
}



static inline void _dmat4_scale_p(dmat4 m, double s)
{
    m[0][0] *= s;
    m[0][1] *= s;
    m[0][2] *= s;
    m[0][3] *= s;
    m[1][0] *= s;
    m[1][1] *= s;
    m[1][2] *= s;
    m[1][3] *= s;
    m[2][0] *= s;
    m[2][1] *= s;
    m[2][2] *= s;
    m[2][3] *= s;
    m[3][0] *= s;
    m[3][1] *= s;
    m[3][2] *= s;
    m[3][3] *= s;
}



static inline void _dmat4_inv(dmat4 mat, dmat4 dest)
{
    double t[6] = {0};
    double det = 0;
    double a = mat[0][0], b = mat[0][1], c = mat[0][2], d = mat[0][3], e = mat[1][0],
           f = mat[1][1], g = mat[1][2], h = mat[1][3], i = mat[2][0], j = mat[2][1],
           k = mat[2][2], l = mat[2][3], m = mat[3][0], n = mat[3][1], o = mat[3][2],
           p = mat[3][3];

    t[0] = k * p - o * l;
    t[1] = j * p - n * l;
    t[2] = j * o - n * k;
    t[3] = i * p - m * l;
    t[4] = i * o - m * k;
    t[5] = i * n - m * j;

    dest[0][0] = f * t[0] - g * t[1] + h * t[2];
    dest[1][0] = -(e * t[0] - g * t[3] + h * t[4]);
    dest[2][0] = e * t[1] - f * t[3] + h * t[5];
    dest[3][0] = -(e * t[2] - f * t[4] + g * t[5]);

    dest[0][1] = -(b * t[0] - c * t[1] + d * t[2]);
    dest[1][1] = a * t[0] - c * t[3] + d * t[4];
    dest[2][1] = -(a * t[1] - b * t[3] + d * t[5]);
    dest[3][1] = a * t[2] - b * t[4] + c * t[5];

    t[0] = g * p - o * h;
    t[1] = f * p - n * h;
    t[2] = f * o - n * g;
    t[3] = e * p - m * h;
    t[4] = e * o - m * g;
    t[5] = e * n - m * f;

    dest[0][2] = b * t[0] - c * t[1] + d * t[2];
    dest[1][2] = -(a * t[0] - c * t[3] + d * t[4]);
    dest[2][2] = a * t[1] - b * t[3] + d * t[5];
    dest[3][2] = -(a * t[2] - b * t[4] + c * t[5]);

    t[0] = g * l - k * h;
    t[1] = f * l - j * h;
    t[2] = f * k - j * g;
    t[3] = e * l - i * h;
    t[4] = e * k - i * g;
    t[5] = e * j - i * f;

    dest[0][3] = -(b * t[0] - c * t[1] + d * t[2]);
    dest[1][3] = a * t[0] - c * t[3] + d * t[4];
    dest[2][3] = -(a * t[1] - b * t[3] + d * t[5]);
    dest[3][3] = a * t[2] - b * t[4] + c * t[5];

    det = 1.0f / (a * dest[0][0] + b * dest[1][0] + c * dest[2][0] + d * dest[3][0]);

    _dmat4_scale_p(dest, det);
}



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

// Smallest power of 2 larger or equal than a positive integer.
static inline uint64_t dvz_next_pow2(uint64_t x)
{
    uint64_t p = 1;
    while (p < x)
        p *= 2;
    return p;
}



/**
 * Compute the mean of an array of double values.
 *
 * @param n the number of values
 * @param values an array of double numbers
 * @returns the mean
 */
static inline double dvz_mean(uint32_t n, double* values)
{
    ASSERT(n > 0);
    ASSERT(values != NULL);
    double mean = 0;
    for (uint32_t i = 0; i < n; i++)
        mean += values[i];
    mean /= n;
    ASSERT(mean >= 0);
    return mean;
}



/*************************************************************************************************/
/*  Random number generation                                                                     */
/*************************************************************************************************/

/**
 * Return a random integer number between 0 and 255.
 *
 * @returns random number
 */
static inline uint8_t dvz_rand_byte() { return (uint8_t)(rand() % 256); }



/**
 * Return a random integer number.
 *
 * @returns random number
 */
static inline int dvz_rand_int() { return rand(); }



/**
 * Return a random floating-point number between 0 and 1.
 *
 * @returns random number
 */
static inline float dvz_rand_float() { return (float)rand() / (float)(RAND_MAX); }



/**
 * Return a random floating-point number between 0 and 1.
 *
 * @returns random number
 */
static inline double dvz_rand_double() { return (double)rand() / (double)(RAND_MAX); }



/**
 * Return a random normal floating-point number.
 *
 * @returns random number
 */
static inline double dvz_rand_normal()
{
    return sqrt(-2.0 * log(dvz_rand_double())) * cos(2 * M_PI * dvz_rand_double());
}



#endif
