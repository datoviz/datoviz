/*************************************************************************************************/
/*  Sdf                                                                                          */
/*************************************************************************************************/


/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/sdf.h"
#include "_macros.h"
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

DvzSdf* dvz_sdf(DvzSdfMode mode)
{
    DvzSdf* sdf = (DvzSdf*)calloc(1, sizeof(DvzSdf));
    ANN(sdf);

    sdf->mode = mode;

    return sdf;
}



void dvz_sdf_svg(DvzSdf* sdf, const char* svg_path)
{
    ANN(sdf);
    sdf->svg_path = svg_path;
}



void dvz_sdf_generate(DvzSdf* sdf)
{
    ANN(sdf);

    // Build the Shape.
    Shape shape;
    buildShapeFromSvgPath(shape, sdf->svg_path);
    shape.normalize();

    //                      max. angle
    edgeColoringSimple(shape, 3.0);

    //           image width, height
    Bitmap<float, 3> msdf(32, 32);

    //                     range, scale, translation
    generateMSDF(msdf, shape, 4.0, 1.0, Vector2(4.0, 4.0));
}



void dvz_sdf_shape(DvzSdf* sdf, uvec3 shape)
{
    ANN(sdf);
    // TODO
}



uint8_t* dvz_sdf_data(DvzSdf* sdf, DvzSize* size)
{
    ANN(sdf);
    // TODO
    return NULL;
}



DvzId dvz_sdf_tex(DvzSdf* sdf)
{
    ANN(sdf);
    // TODO
    return 0;
}



void dvz_sdf_destroy(DvzSdf* sdf)
{
    ANN(sdf);
    FREE(sdf);
}
