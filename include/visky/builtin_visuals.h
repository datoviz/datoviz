#ifndef VKL_BUILTIN_VISUALS_HEADER
#define VKL_BUILTIN_VISUALS_HEADER

#include "visuals.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Variants.
typedef enum
{
    VKL_VISUAL_VARIANT_NONE = 0,

    VKL_VISUAL_VARIANT_RAW = 0x0001,
    VKL_VISUAL_VARIANT_AGG = 0x0002,
    VKL_VISUAL_VARIANT_SHADED = 0x0004,

    VKL_VISUAL_VARIANT_TEXTURED = 0x0010,
    VKL_VISUAL_VARIANT_TEXTURED_MULTI = 0x0020,

} VklVisualVariant;



// Visual types.
typedef enum
{
    VKL_VISUAL_NONE,

    VKL_VISUAL_SCATTER,     // raw, agg
    VKL_VISUAL_SEGMENT,     // raw, agg
    VKL_VISUAL_ARROW,       //
    VKL_VISUAL_PATH,        // raw, agg
    VKL_VISUAL_TEXT,        // raw, agg
    VKL_VISUAL_TRIANGLE,    //
    VKL_VISUAL_RECTANGLE,   //
    VKL_VISUAL_IMAGE,       // single, multi
    VKL_VISUAL_DISC,        //
    VKL_VISUAL_SECTOR,      //
    VKL_VISUAL_MESH,        // raw, textured, textured_multi, shaded
    VKL_VISUAL_POLYGON,     //
    VKL_VISUAL_PSLG,        //
    VKL_VISUAL_HISTOGRAM,   //
    VKL_VISUAL_AREA,        //
    VKL_VISUAL_CANDLE,      //
    VKL_VISUAL_GRAPH,       //
    VKL_VISUAL_SURFACE,     //
    VKL_VISUAL_VOLUME,      //
    VKL_VISUAL_FAKE_SPHERE, //

    VKL_VISUAL_COUNT,

    VKL_VISUAL_CUSTOM,
} VklVisualType;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

VKY_EXPORT VklVisual vkl_visual_builtin(VklCanvas* canvas, VklVisualType type, int flags);



#endif
