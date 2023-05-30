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

// Visual draw callback function.
typedef void (*DvzVisualCallback)(
    DvzVisual* visual, DvzId canvas, //
    uint32_t first, uint32_t count, uint32_t first_instance, uint32_t instance_count);



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
    DVZ_ATTR_FLAGS_DYNAMIC = 0x0011, // the N in 0x000N indicates the binding idx
    DVZ_ATTR_FLAGS_CONSTANT = 0x0020,
    DVZ_ATTR_FLAGS_REPEAT = 0x1000, // the N in 0x0N00 indicates the number of repeats
    DVZ_ATTR_FLAGS_QUAD = 0x2000,
} DvzAttrFlags;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzVisualAttr
{
    DvzFormat format;
    int flags;
    uint32_t binding_idx;
    DvzSize offset;
    DvzSize item_size;
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
    uint32_t vertex_count;
    uint32_t group_count;
    uint32_t* group_sizes;

    // Visual draw callback.
    DvzVisualCallback callback;

    DvzMVP mvp;
    DvzViewport viewport;
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Visual lifecycle                                                                             */
/*************************************************************************************************/

DVZ_EXPORT DvzVisual* dvz_visual(DvzRequester* rqr, DvzPrimitiveTopology primitive, int flags);



DVZ_EXPORT void dvz_visual_update(DvzVisual* visual);



DVZ_EXPORT void dvz_visual_destroy(DvzVisual* visual);



/*************************************************************************************************/
/*  Visual declaration                                                                           */
/*************************************************************************************************/

DVZ_EXPORT void
dvz_visual_spirv(DvzVisual* visual, DvzShaderType type, DvzSize size, const unsigned char* buffer);



DVZ_EXPORT void dvz_visual_shader(DvzVisual* visual, const char* name);



DVZ_EXPORT void dvz_visual_resize(DvzVisual* visual, uint32_t item_count, uint32_t vertex_count);



DVZ_EXPORT void dvz_visual_groups(DvzVisual* visual, uint32_t group_count, uint32_t* group_sizes);



DVZ_EXPORT void dvz_visual_attr(DvzVisual* visual, uint32_t attr_idx, DvzFormat format, int flags);



DVZ_EXPORT void dvz_visual_dat(DvzVisual* visual, uint32_t slot_idx, DvzSize size);



DVZ_EXPORT void dvz_visual_tex(DvzVisual* visual, uint32_t slot_idx, DvzTexDims dims, int flags);



DVZ_EXPORT void dvz_visual_create(DvzVisual* visual, uint32_t item_count, uint32_t vertex_count);



/*************************************************************************************************/
/*  Visual common bindings                                                                       */
/*************************************************************************************************/

DVZ_EXPORT void dvz_visual_mvp(DvzVisual* visual, DvzMVP mvp); // update the MVP



DVZ_EXPORT void
dvz_visual_viewport(DvzVisual* visual, DvzViewport viewport); // update the viewport



/*************************************************************************************************/
/*  Visual data                                                                                  */
/*************************************************************************************************/

DVZ_EXPORT void
dvz_visual_data(DvzVisual* visual, uint32_t attr_idx, uint32_t first, uint32_t count, void* data);



DVZ_EXPORT void dvz_visual_quads(
    DvzVisual* visual, uint32_t attr_idx, uint32_t first, uint32_t count, vec2 quad_size,
    vec2* positions);



/*************************************************************************************************/
/*  Visual drawing                                                                               */
/*************************************************************************************************/

DVZ_EXPORT void dvz_visual_instance(
    DvzVisual* visual, DvzId canvas, uint32_t first, uint32_t vertex_offset, uint32_t count,
    uint32_t first_instance, uint32_t instance_count);



DVZ_EXPORT void dvz_visual_indirect(DvzVisual* visual, DvzId canvas, uint32_t draw_count);



// Default visual draw callback.
DVZ_EXPORT void dvz_visual_draw(
    DvzVisual* visual, DvzId canvas, uint32_t first, uint32_t count, uint32_t first_instance,
    uint32_t instance_count);



DVZ_EXPORT void dvz_visual_callback(DvzVisual* visual, DvzVisualCallback callback);



EXTERN_C_OFF

#endif
