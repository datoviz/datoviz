/*************************************************************************************************/
/*  Common mathematical macros                                                                   */
/*************************************************************************************************/

#ifndef DVZ_HEADER_PUBLIC_MATH
#define DVZ_HEADER_PUBLIC_MATH



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <float.h>
#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "datoviz_enums.h"
#include "datoviz_macros.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define M_2PI 6.28318530717958647692
#define M_PI2 1.57079632679489650726

#define M_INV_255 0.00392156862745098

#define EPSILON 1e-10

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
static uint32_t _ZERO_OFFSET[3] = {0, 0, 0};
#pragma GCC diagnostic pop
#define DVZ_ZERO_OFFSET _ZERO_OFFSET



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#define MIN(a, b)     (((a) < (b)) ? (a) : (b))
#define MAX(a, b)     (((a) > (b)) ? (a) : (b))
#define CLIP(x, a, b) MAX(MIN((x), (b)), (a))

#define TO_BYTE(x) (uint8_t) round(CLIP((x), 0, 1) * 255)

#define _DMAT4_IDENTITY_INIT                                                                      \
    {                                                                                             \
        {1.0, 0.0, 0.0, 0.0}, {0.0, 1.0, 0.0, 0.0}, {0.0, 0.0, 1.0, 0.0}, { 0.0, 0.0, 0.0, 1.0 }  \
    }



/*************************************************************************************************/
/*  ID                                                                                           */
/*************************************************************************************************/

typedef uint64_t DvzId;



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
    snprintf(_PRETTY_SIZE, 64, "%.1f %s", s, u);
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

/* Unsigned */
typedef uint32_t uvec2[2];
typedef uint32_t uvec3[3];
typedef uint32_t uvec4[4];

/* Index */
typedef uint32_t DvzIndex;



/*************************************************************************************************/
/*  Single-precision floating-point numbers                                                      */
/*************************************************************************************************/

// NOTE: copy from cglm

#if (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L)
#include <stdalign.h>
#endif

#if defined(_MSC_VER)
/* do not use alignment for older visual studio versions */
#if _MSC_VER < 1913 /*  Visual Studio 2017 version 15.6  */
#define CGLM_ALL_UNALIGNED
#define CGLM_ALIGN(X) /* no alignment */
#else
#define CGLM_ALIGN(X) __declspec(align(X))
#endif
#else
#define CGLM_ALIGN(X) __attribute((aligned(X)))
#endif

#ifndef CGLM_ALL_UNALIGNED
#define CGLM_ALIGN_IF(X) CGLM_ALIGN(X)
#else
#define CGLM_ALIGN_IF(X) /* no alignment */
#endif

#ifdef __AVX__
#define CGLM_ALIGN_MAT CGLM_ALIGN(32)
#else
#define CGLM_ALIGN_MAT CGLM_ALIGN(16)
#endif

#ifndef CGLM_HAVE_BUILTIN_ASSUME_ALIGNED

#if defined(__has_builtin)
#if __has_builtin(__builtin_assume_aligned)
#define CGLM_HAVE_BUILTIN_ASSUME_ALIGNED 1
#endif
#elif defined(__GNUC__) && defined(__GNUC_MINOR__)
#if __GNUC__ >= 4 && __GNUC_MINOR__ >= 7
#define CGLM_HAVE_BUILTIN_ASSUME_ALIGNED 1
#endif
#endif

#ifndef CGLM_HAVE_BUILTIN_ASSUME_ALIGNED
#define CGLM_HAVE_BUILTIN_ASSUME_ALIGNED 0
#endif

#endif

#if CGLM_HAVE_BUILTIN_ASSUME_ALIGNED
#define CGLM_ASSUME_ALIGNED(expr, alignment) __builtin_assume_aligned((expr), (alignment))
#else
#define CGLM_ASSUME_ALIGNED(expr, alignment) (expr)
#endif

#if (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L)
#define CGLM_CASTPTR_ASSUME_ALIGNED(expr, type) ((type*)CGLM_ASSUME_ALIGNED((expr), alignof(type)))
#elif defined(_MSC_VER)
#define CGLM_CASTPTR_ASSUME_ALIGNED(expr, type)                                                   \
    ((type*)CGLM_ASSUME_ALIGNED((expr), __alignof(type)))
#else
#define CGLM_CASTPTR_ASSUME_ALIGNED(expr, type)                                                   \
    ((type*)CGLM_ASSUME_ALIGNED((expr), __alignof__(type)))
#endif

typedef int ivec2[2];
typedef int ivec3[3];
typedef int ivec4[4];

typedef float vec2[2];
typedef float vec3[3];
typedef CGLM_ALIGN_IF(16) float vec4[4];

typedef vec3 mat3[3];
typedef vec2 mat3x2[3];
typedef vec4 mat3x4[3];

typedef CGLM_ALIGN_IF(16) vec2 mat2[2];
typedef vec3 mat2x3[2];
typedef vec4 mat2x4[2];

typedef CGLM_ALIGN_MAT vec4 mat4[4];
typedef vec2 mat4x2[4];
typedef vec3 mat4x3[4];

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

EXTERN_C_ON



/**
 * Return the smallest power of 2 larger or equal than a positive integer.
 *
 * @param x the value
 * @returns the power of 2
 */
DVZ_EXPORT uint64_t dvz_next_pow2(uint64_t x);



/**
 * Compute the mean of an array of double values.
 *
 * @param n the number of values
 * @param values an array of double numbers
 * @returns the mean
 */
DVZ_EXPORT double dvz_mean(uint32_t n, double* values);



/**
 * Compute the min and max of an array of float values.
 *
 * @param n the number of values
 * @param values an array of float numbers
 * @param out_min_max the min and max
 * @returns the mean
 */
DVZ_EXPORT void dvz_min_max(uint32_t n, const float* values, vec2 out_min_max);



/**
 * Normalize the array.
 *
 * @param count the number of values
 * @param values an array of float numbers
 * @returns the normalized array
 */
DVZ_EXPORT uint8_t* dvz_normalize_bytes(uint32_t count, float* values);



/**
 * Compute the range of an array of double values.
 *
 * @param n the number of values
 * @param values an array of double numbers
 * @param[out] the min and max values
 */
DVZ_EXPORT void dvz_range(uint32_t n, double* values, dvec2 min_max);



/*************************************************************************************************/
/*  Random number generation                                                                     */
/*************************************************************************************************/

/**
 * Return a random integer number between 0 and 255.
 *
 * @returns random number
 */
DVZ_EXPORT uint8_t dvz_rand_byte(void);



/**
 * Return a random integer number.
 *
 * @returns random number
 */
DVZ_EXPORT int dvz_rand_int(void);



/**
 * Return a random floating-point number between 0 and 1.
 *
 * @returns random number
 */
DVZ_EXPORT float dvz_rand_float(void);



/**
 * Return a random floating-point number between 0 and 1.
 *
 * @returns random number
 */
DVZ_EXPORT double dvz_rand_double(void);



/**
 * Return a random normal floating-point number.
 *
 * @returns random number
 */
DVZ_EXPORT double dvz_rand_normal(void);



/*************************************************************************************************/
/*  Mock random data                                                                             */
/*************************************************************************************************/

/**
 * Generate a set of random 2D positions.
 *
 * @param count the number of positions to generate
 * @param std the standard deviation
 * @returns the positions
 */
DVZ_EXPORT vec3* dvz_mock_pos2D(uint32_t count, float std);



/**
 * Generate points on a circle.
 *
 * @param count the number of positions to generate
 * @param radius the radius of the circle
 * @returns the positions
 */
DVZ_EXPORT vec3* dvz_mock_circle(uint32_t count, float radius);



/**
 * Generate points on a band.
 *
 * @param count the number of positions to generate
 * @param size the size of the band
 * @returns the positions
 */
DVZ_EXPORT vec3* dvz_mock_band(uint32_t count, vec2 size);



/**
 * Generate a set of random 3D positions.
 *
 * @param count the number of positions to generate
 * @param std the standard deviation
 * @returns the positions
 */
DVZ_EXPORT vec3* dvz_mock_pos3D(uint32_t count, float std);



/**
 * Generate identical 3D positions.
 *
 * @param count the number of positions to generate
 * @param fixed the position
 * @returns the repeated positions
 */
DVZ_EXPORT vec3* dvz_mock_fixed(uint32_t count, vec3 fixed);



/**
 * Generate 3D positions on a line.
 *
 * @param count the number of positions to generate
 * @param p0 initial position
 * @param p1 terminal position
 * @returns the positions
 */
DVZ_EXPORT vec3* dvz_mock_line(uint32_t count, vec3 p0, vec3 p1);



/**
 * Generate a set of uniformly random scalar values.
 *
 * @param count the number of values to generate
 * @param vmin the minimum value of the interval
 * @param vmax the maximum value of the interval
 * @returns the values
 */
DVZ_EXPORT float* dvz_mock_uniform(uint32_t count, float vmin, float vmax);



/**
 * Generate an array with the same value.
 *
 * @param count the number of scalars to generate
 * @param value the value
 * @returns the values
 */
DVZ_EXPORT float* dvz_mock_full(uint32_t count, float value);



/**
 * Generate an array of consecutive positive numbers.
 *
 * @param count the number of consecutive integers to generate
 * @param initial the initial value
 * @returns the values
 */
DVZ_EXPORT uint32_t* dvz_mock_range(uint32_t count, uint32_t initial);



/**
 * Generate an array ranging from an initial value to a final value.
 *
 * @param count the number of scalars to generate
 * @param initial the initial value
 * @param final the final value
 * @returns the values
 */
DVZ_EXPORT float* dvz_mock_linspace(uint32_t count, float initial, float final);



/**
 * Generate a set of random colors.
 *
 * @param count the number of colors to generate
 * @param alpha the alpha value
 * @returns random colors
 */
DVZ_EXPORT cvec4* dvz_mock_color(uint32_t count, uint8_t alpha);



/**
 * Repeat a color in an array.
 *
 * @param count the number of colors to generate
 * @param mono the color to repeat
 * @returns colors
 */
DVZ_EXPORT cvec4* dvz_mock_monochrome(uint32_t count, cvec4 mono);



/**
 * Generate a set of HSV colors.
 *
 * @param count the number of colors to generate
 * @param alpha the alpha value
 * @returns colors
 */
DVZ_EXPORT cvec4* dvz_mock_cmap(uint32_t count, DvzColormap cmap, uint8_t alpha);



EXTERN_C_OFF

#endif
