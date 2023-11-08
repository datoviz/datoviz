/*************************************************************************************************/
/*  Sdf                                                                                          */
/*************************************************************************************************/


/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/sdf.h"
#include "_macros.h"
#include "_math.h"
#include "fileio.h"
#include "request.h"

// Include msdfgen
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow" //
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include "msdfgen-ext.h"
#include "msdfgen.h"
#pragma GCC diagnostic pop

using namespace msdfgen;



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Utility functions                                                                            */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Sdf functions                                                                                */
/*************************************************************************************************/


// NOTE: the caller must FREE the returned pointer.
uint8_t* dvz_svg_sdf(const char* svg_path, uint32_t width, uint32_t height)
{
    ANN(svg_path);
    ASSERT(width > 0);
    ASSERT(height > 0);
    uint32_t w = width;
    uint32_t h = height;

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

    vec2 min_max = {0};
    dvz_min_max(w * h, bitmap.pixels, min_max);
    float m = min_max[0];
    float M = min_max[1];
    if (m == M)
        M = m + 1;
    ASSERT(m < M);
    float d = 1. / (M - m);

    DvzSize size = w * h * 3;
    uint8_t* rgb = (uint8_t*)malloc(size);
    uint32_t x, y, i, j;
    uint8_t value = 0;

    for (y = 0; y < h; y++)
    {
        for (x = 0; x < w; x++)
        {
            i = (y * w + x);
            j = 3 * ((h - 1 - y) * w + (x));

            value = round((bitmap.pixels[i] - m) * d * 255);
            rgb[j + 0] = value;
            rgb[j + 1] = value;
            rgb[j + 2] = value;
        }
    }

    return rgb;
}



// NOTE: the caller must FREE the returned pointer.
uint8_t* dvz_svg_msdf(const char* svg_path, uint32_t width, uint32_t height)
{
    ANN(svg_path);
    ASSERT(width > 0);
    ASSERT(height > 0);
    uint32_t w = width;
    uint32_t h = height;

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

    vec2 min_max = {0};
    dvz_min_max(w * h, bitmap.pixels, min_max);
    float m = min_max[0];
    float M = min_max[1];
    if (m == M)
        M = m + 1;
    ASSERT(m < M);
    float d = 1. / (M - m);

    DvzSize size = w * h * 3;
    uint8_t* rgb = (uint8_t*)malloc(size);
    uint32_t x, y, i, j;

    for (y = 0; y < h; y++)
    {
        for (x = 0; x < w; x++)
        {
            i = 3 * (y * w + x);
            j = 3 * ((h - 1 - y) * w + (x));

            rgb[j + 0] = round((bitmap.pixels[i + 0] - m) * d * 255);
            rgb[j + 1] = round((bitmap.pixels[i + 1] - m) * d * 255);
            rgb[j + 2] = round((bitmap.pixels[i + 2] - m) * d * 255);
        }
    }

    return rgb;
}
