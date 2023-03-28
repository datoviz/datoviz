/*************************************************************************************************/
/* Visual                                                                                        */
/*************************************************************************************************/

#ifndef DVZ_HEADER_VISUAL
#define DVZ_HEADER_VISUAL



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_enums.h"
#include "_obj.h"
#include "scene/mvp.h"
#include "scene/viewport.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzVisual DvzVisual;
typedef struct DvzVisualAttr DvzVisualAttr;

// Forward declarations.
typedef struct DvzRequester DvzRequester;
typedef struct DvzBaker DvzBaker;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    DVZ_VISUALS_FLAGS_DEFAULT = 0x0000,
    DVZ_VISUALS_FLAGS_INDEXED = 0x0001,
    DVZ_VISUALS_FLAGS_INDIRECT = 0x0002,
} DvzVisualFlags;



typedef enum
{
    DVZ_ATTR_FLAGS_DEFAULT = 0x0000,
    DVZ_ATTR_FLAGS_DYNAMIC = 0x0001,
    DVZ_ATTR_FLAGS_CONSTANT = 0x0002,
    DVZ_ATTR_FLAGS_REPEATS_2 = 0x0020,
    DVZ_ATTR_FLAGS_REPEATS_3 = 0x0030,
    DVZ_ATTR_FLAGS_REPEATS_4 = 0x0040,
    DVZ_ATTR_FLAGS_REPEATS_5 = 0x0050,
    DVZ_ATTR_FLAGS_REPEATS_6 = 0x0060,
    DVZ_ATTR_FLAGS_QUAD = 0x00F0,
} DvzAttrFlags;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzVisualAttr
{
    DvzFormat format;
    int flags;
};



struct DvzVisual
{
    DvzObject obj;
    DvzRequester* rqr;
    int flags;

    DvzBaker* baker;
    DvzId graphics_id;
    DvzVisualAttr attrs[DVZ_MAX_VERTEX_ATTRS];

    uint32_t item_count;
    uint32_t group_count;
    uint32_t* group_sizes;

    DvzMVP mvp;
    DvzViewport viewport;
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DVZ_EXPORT DvzVisual*
dvz_visual(DvzRequester* rqr, DvzPrimitiveTopology primitive, uint32_t item_count, int flags);



DVZ_EXPORT void
dvz_visual_spirv(DvzVisual* visual, DvzShaderType type, DvzSize size, const unsigned char* buffer);



DVZ_EXPORT void dvz_visual_shader(DvzVisual* visual, const char* name);



DVZ_EXPORT void dvz_visual_count(DvzVisual* visual, uint32_t item_count);



DVZ_EXPORT void dvz_visual_groups(DvzVisual* visual, uint32_t group_count, uint32_t* group_sizes);



DVZ_EXPORT void dvz_visual_attr(DvzVisual* visual, uint32_t attr_idx, DvzFormat format, int flags);



DVZ_EXPORT void dvz_visual_dat(DvzVisual* visual, uint32_t slot_idx, DvzSize size);



DVZ_EXPORT void dvz_visual_tex(DvzVisual* visual, uint32_t slot_idx, DvzTexDims dims, int flags);



DVZ_EXPORT void dvz_visual_create(DvzVisual* visual);



DVZ_EXPORT void dvz_visual_mvp(DvzVisual* visual, DvzMVP mvp); // update the MVP



DVZ_EXPORT void
dvz_visual_viewport(DvzVisual* visual, DvzViewport viewport); // update the viewport



DVZ_EXPORT void
dvz_visual_data(DvzVisual* visual, uint32_t attr_idx, uint32_t first, uint32_t count, void* data);



DVZ_EXPORT void dvz_visual_draw(DvzVisual* visual, uint32_t first, uint32_t count);



DVZ_EXPORT void dvz_visual_instance(DvzVisual* visual, uint32_t first, uint32_t count); // TODO



DVZ_EXPORT void dvz_visual_destroy(DvzVisual* visual);



EXTERN_C_OFF

#endif
