/*************************************************************************************************/
/* Volume                                                                                        */
/*************************************************************************************************/

#ifndef DVZ_HEADER_VOLUME
#define DVZ_HEADER_VOLUME



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "../../_enums.h"
#include "../viewport.h"
#include "../visual.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzVolumeVertex DvzVolumeVertex;
typedef struct DvzVolumeParams DvzVolumeParams;

// Forward declarations.
typedef struct DvzBatch DvzBatch;
typedef struct DvzVisual DvzVisual;
typedef struct DvzShape DvzShape;



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

// NOTE: must correspond to values in utils_volume.glsl
#define VOLUME_TYPE_SCALAR 0
#define VOLUME_TYPE_RGBA   1

#define VOLUME_COLOR_DIRECT   0
#define VOLUME_COLOR_COLORMAP 1

#define VOLUME_DIR_FRONT_BACK 0
#define VOLUME_DIR_BACK_FRONT 1



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    DVZ_VOLUME_FLAGS_NONE = 0x0000,
    DVZ_VOLUME_FLAGS_RGBA = 0x0001,
    DVZ_VOLUME_FLAGS_COLORMAP = 0x0002,
    DVZ_VOLUME_FLAGS_BACK_FRONT = 0x0004,
} DvzVolumeFlags;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzVolumeVertex
{
    vec3 pos; /* position */
};

struct DvzVolumeParams
{
    vec4 box_size; /* size of the box containing the volume, in NDC */
    vec4 uvw0;     /* texture coordinates of the 2 corner points */
    vec4 uvw1;     /* texture coordinates of the 2 corner points */
    vec4 transfer;
};



static void volume_specialization(DvzVisual* visual)
{
    ANN(visual);
    int flags = visual->flags;

    // Specialization constants.
    int volume_type = VOLUME_TYPE_SCALAR;
    if ((flags & DVZ_VOLUME_FLAGS_RGBA) != 0)
        volume_type = VOLUME_TYPE_RGBA;

    int volume_color = VOLUME_COLOR_DIRECT;
    if ((flags & DVZ_VOLUME_FLAGS_COLORMAP) != 0)
        volume_color = VOLUME_COLOR_COLORMAP;

    int volume_dir = VOLUME_DIR_FRONT_BACK;
    if ((flags & DVZ_VOLUME_FLAGS_BACK_FRONT) != 0)
        volume_dir = VOLUME_DIR_BACK_FRONT;

    dvz_visual_specialization(visual, DVZ_SHADER_FRAGMENT, 0, sizeof(int), &volume_type);
    dvz_visual_specialization(visual, DVZ_SHADER_FRAGMENT, 1, sizeof(int), &volume_color);
    dvz_visual_specialization(visual, DVZ_SHADER_FRAGMENT, 2, sizeof(int), &volume_dir);
}



#endif
