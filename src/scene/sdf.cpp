/*************************************************************************************************/
/*  Sdf                                                                                          */
/*************************************************************************************************/


/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/sdf.h"
#include "_macros.h"
#include "datoviz_math.h"
#include "fileio.h"
#include "request.h"

#if HAS_MSDF
// Include msdfgen
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow" //
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include "msdfgen-ext.h"
#include "msdfgen.h"
#pragma GCC diagnostic pop

using namespace msdfgen;
#endif



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Utility functions                                                                            */
/*************************************************************************************************/

// NOTE: the returned pointer will have to be freed.
static void* _cpy(DvzSize size, const void* data)
{
    void* data_cpy = malloc(size);
    memcpy(data_cpy, data, size);
    return data_cpy;
}



static inline void _normalizer(uint32_t count, const float* values, vec2 out)
{
    ASSERT(count > 0);
    ANN(values);

    vec2 min_max = {0};
    dvz_min_max(count, values, min_max);
    float m = min_max[0];
    float M = min_max[1];
    if (m == M)
        M = m + 1;
    ASSERT(m < M);
    float d = 1. / (M - m);
    out[0] = d;
    out[1] = -m;
}



/*************************************************************************************************/
/*  Sdf functions                                                                                */
/*************************************************************************************************/

// NOTE: the caller must FREE the returned pointer.
float* dvz_sdf_from_svg(const char* svg_path, uint32_t width, uint32_t height)
{
    ANN(svg_path);
    ASSERT(width > 0);
    ASSERT(height > 0);
    uint32_t w = width;
    uint32_t h = height;

#if HAS_MSDF
    // Build the Shape.
    Shape shape;
    buildShapeFromSvgPath(shape, svg_path);
    shape.normalize();

    //                      max. angle
    edgeColoringSimple(shape, 3.0);

    //           image width, height
    Bitmap<float, 1> msdf((int)width, (int)height);

    //                     range, scale, translation
    generateSDF(msdf, shape, 4.0, 1.0, Vector2(0.0, 0.0));
    BitmapConstRef<float, 1> bitmap = msdf;

    DvzSize size = w * h * sizeof(float);
    return (float*)_cpy(size, bitmap.pixels);

#else
    return NULL;
#endif
}



// NOTE: the caller must FREE the returned pointer.
float* dvz_msdf_from_svg(const char* svg_path, uint32_t width, uint32_t height)
{
    ANN(svg_path);
    ASSERT(width > 0);
    ASSERT(height > 0);
    uint32_t w = width;
    uint32_t h = height;

#if HAS_MSDF
    // Build the Shape.
    Shape shape;
    buildShapeFromSvgPath(shape, svg_path);
    shape.normalize();

    //                      max. angle
    edgeColoringSimple(shape, 3.0);

    //           image width, height
    Bitmap<float, 3> msdf((int)width, (int)height);

    //                     range, scale, translation
    generateMSDF(msdf, shape, 4.0, 1.0, Vector2(0.0, 0.0));
    BitmapConstRef<float, 3> bitmap = msdf;

    DvzSize size = w * h * 3 * sizeof(float);
    return (float*)_cpy(size, bitmap.pixels);

#else
    return NULL;
#endif
}



// NOTE: the caller must FREE the returned pointer.
uint8_t* dvz_sdf_to_rgb(float* sdf, uint32_t width, uint32_t height)
{
    if (sdf == NULL)
        return NULL;
    ANN(sdf);

    uint32_t w = width;
    uint32_t h = height;

    vec2 ab = {0}; // d, -m
    _normalizer(w * h, sdf, ab);
    float a = ab[0] * 255;
    float b = ab[1];

    uint8_t* rgb = (uint8_t*)calloc(w * h, 3 * sizeof(uint8_t));
    uint32_t x, y, i, j;
    uint8_t value = 0;

    for (y = 0; y < h; y++)
    {
        for (x = 0; x < w; x++)
        {
            i = (y * w + x);
            j = 3 * ((h - 1 - y) * w + (x));
            value = round(a * (sdf[i] + b));
            rgb[j + 0] = value;
            rgb[j + 1] = value;
            rgb[j + 2] = value;
        }
    }

    return rgb;
}



// NOTE: the caller must FREE the returned pointer.
uint8_t* dvz_msdf_to_rgb(float* msdf, uint32_t width, uint32_t height)
{
    ANN(msdf);

    uint32_t w = width;
    uint32_t h = height;

    vec2 ab = {0}; // d, -m
    _normalizer(w * h, msdf, ab);
    float a = ab[0] * 255;
    float b = ab[1];

    uint8_t* rgb = (uint8_t*)calloc(w * h, 3 * sizeof(uint8_t));
    uint32_t x, y, i, j;

    for (y = 0; y < h; y++)
    {
        for (x = 0; x < w; x++)
        {
            i = 3 * (y * w + x);
            j = 3 * ((h - 1 - y) * w + (x));
            rgb[j + 0] = round(a * (msdf[i + 0] + b));
            rgb[j + 1] = round(a * (msdf[i + 1] + b));
            rgb[j + 2] = round(a * (msdf[i + 2] + b));
        }
    }

    return rgb;
}



/*************************************************************************************************/
/*  Image utils                                                                                  */
/*************************************************************************************************/

// NOTE: the caller must FREE the returned pointer.
uint8_t* dvz_rgb_to_rgba_char(uint32_t count, uint8_t* rgb)
{
    ASSERT(count > 0);
    ANN(rgb);
    DvzSize size = 4 * count * sizeof(uint8_t);
    uint8_t* rgba = (uint8_t*)malloc(size);
    for (uint32_t i = 0; i < count; i++)
    {
        rgba[4 * i + 0] = rgb[3 * i + 0];
        rgba[4 * i + 1] = rgb[3 * i + 1];
        rgba[4 * i + 2] = rgb[3 * i + 2];
        rgba[4 * i + 3] = 255;
    }
    return rgba;
}



// NOTE: the caller must FREE the returned pointer.
float* dvz_rgb_to_rgba_float(uint32_t count, float* rgb)
{
    ASSERT(count > 0);
    ANN(rgb);
    DvzSize size = 4 * count * sizeof(float);
    float* rgba = (float*)malloc(size);
    for (uint32_t i = 0; i < count; i++)
    {
        rgba[4 * i + 0] = rgb[3 * i + 0];
        rgba[4 * i + 1] = rgb[3 * i + 1];
        rgba[4 * i + 2] = rgb[3 * i + 2];
        rgba[4 * i + 3] = 1;
    }
    return rgba;
}
