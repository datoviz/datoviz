#ifndef VKY_TYPES_HEADER
#define VKY_TYPES_HEADER

#include <math.h>
#include <stdint.h>



/*************************************************************************************************/
/*  8-bit integers                                                                               */
/*************************************************************************************************/

typedef uint8_t cvec2[2];
typedef uint8_t cvec4[4]; // used for color index



/*************************************************************************************************/
/*  16-bit integers                                                                              */
/*************************************************************************************************/

/* Signed */
typedef int16_t svec2[2];
typedef int16_t svec4[4]; // used for glyph as vec4 of uint16

/* Unsigned */
typedef uint16_t usvec2[2];
typedef uint16_t usvec4[4]; // used for glyph as vec4 of uint16



/*************************************************************************************************/
/*  32-bit integers                                                                              */
/*************************************************************************************************/

/* Signed */
typedef int32_t ivec2[2];
typedef int32_t ivec4[4];

/* Unsigned */
typedef uint32_t uvec2[2];
typedef uint32_t uvec4[4];

/* Index */
typedef uint32_t VkyIndex;



/*************************************************************************************************/
/*  Single-precision floating-point numbers                                                      */
/*************************************************************************************************/

typedef vec2 fvec2;
typedef vec4 fvec4;



/*************************************************************************************************/
/*  Double-precision floating-point numbers                                                      */
/*************************************************************************************************/

/* Array types */
typedef double dvec2[2];
typedef double dvec4[4];

/* struct types */
typedef struct dvec2s dvec2s;
struct dvec2s
{
    double x, y;
};

typedef struct dvec4s dvec4s;
struct dvec4s
{
    double x, y, z, t;
};



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

// Avoid warnings with const*
VKY_INLINE void vec3_copy(const float* input, float* output)
{
    memcpy(output, input, sizeof(vec3));
}

VKY_INLINE void vec4_copy(const float* input, float* output)
{
    memcpy(output, input, sizeof(vec4));
}

VKY_INLINE void vec4_scale(const float* input, float s, float* output)
{
    for (uint32_t i = 0; i < 4; i++)
    {
        output[i] = s * input[i];
    }
}



#endif
