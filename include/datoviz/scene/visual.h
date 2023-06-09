/*************************************************************************************************/
/* Visual                                                                                        */
/*************************************************************************************************/

#ifndef DVZ_HEADER_VISUAL
#define DVZ_HEADER_VISUAL



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "../_enums.h"
#include "../_obj.h"
#include "mvp.h"
#include "viewport.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzVisual DvzVisual;
typedef struct DvzVisualAttr DvzVisualAttr;

// Forward declarations.
typedef struct DvzRequester DvzRequester;
typedef struct DvzBaker DvzBaker;
typedef struct DvzView DvzView;
typedef struct DvzTransform DvzTransform;

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



typedef enum
{
    DVZ_PROP_NONE = 0x00,

    // use instance vertex rate so that the same value is used for all vertices
    DVZ_PROP_CONSTANT = 0x01,

    DVZ_PROP_DYNAMIC = 0x02,
} DvzPropFlags;



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
    void* params; // may be used for visual-specific data

    DvzBaker* baker;
    DvzId graphics_id;
    DvzVisualAttr attrs[DVZ_MAX_VERTEX_ATTRS];
    DvzTransform* transforms[DVZ_MAX_VERTEX_ATTRS];

    // Data.
    uint32_t item_count;
    uint32_t vertex_count;
    uint32_t group_count;
    uint32_t* group_sizes;

    // Drawing.
    uint32_t draw_first;     // first item (offset).
    uint32_t draw_count;     // number of items to draw.
    uint32_t first_instance; // instancing.
    uint32_t instance_count;
    bool is_visible;

    // Visual draw callback.
    DvzVisualCallback callback;
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Visual lifecycle                                                                             */
/*************************************************************************************************/

DVZ_EXPORT DvzVisual* dvz_visual(DvzRequester* rqr, DvzPrimitiveTopology primitive, int flags);



DVZ_EXPORT void dvz_visual_update(DvzVisual* visual);



DVZ_EXPORT void dvz_visual_destroy(DvzVisual* visual);



/*************************************************************************************************/
/*  Visual fixed pipeline                                                                        */
/*************************************************************************************************/

DVZ_EXPORT void dvz_visual_blend(DvzVisual* visual, DvzBlendType blend_type);



DVZ_EXPORT void dvz_visual_depth(DvzVisual* visual, DvzDepthTest depth_test);



DVZ_EXPORT void dvz_visual_polygon(DvzVisual* visual, DvzPolygonMode polygon_mode);



DVZ_EXPORT void dvz_visual_cull(DvzVisual* visual, DvzCullMode cull_mode);



DVZ_EXPORT void dvz_visual_front(DvzVisual* visual, DvzFrontFace front_face);



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



DVZ_EXPORT void dvz_visual_alloc(DvzVisual* visual, uint32_t item_count, uint32_t vertex_count);



/*************************************************************************************************/
/*  Visual common bindings                                                                       */
/*************************************************************************************************/

// NOTE: only used for testing, otherwise the viewset takes care of this (transform & view)

DVZ_EXPORT void dvz_visual_mvp(DvzVisual* visual, DvzMVP* mvp); // update the MVP



DVZ_EXPORT void
dvz_visual_viewport(DvzVisual* visual, DvzViewport* viewport); // update the viewport



/*************************************************************************************************/
/*  Visual data                                                                                  */
/*************************************************************************************************/

DVZ_EXPORT void
dvz_visual_data(DvzVisual* visual, uint32_t attr_idx, uint32_t first, uint32_t count, void* data);



DVZ_EXPORT void dvz_visual_quads(
    DvzVisual* visual, uint32_t attr_idx, uint32_t first, uint32_t count, vec2 quad_size,
    vec2* positions);



/*************************************************************************************************/
/*  Visual drawing internal functions                                                            */
/*************************************************************************************************/

DVZ_EXPORT void dvz_visual_instance(
    DvzVisual* visual, DvzId canvas, uint32_t first, uint32_t vertex_offset, uint32_t count,
    uint32_t first_instance, uint32_t instance_count);



DVZ_EXPORT void dvz_visual_indirect(DvzVisual* visual, DvzId canvas, uint32_t draw_count);



DVZ_EXPORT void dvz_visual_record(DvzVisual* visual, DvzId canvas);



DVZ_EXPORT void dvz_visual_callback(DvzVisual* visual, DvzVisualCallback callback);



/*************************************************************************************************/
/*  Visual drawing                                                                               */
/*************************************************************************************************/

/**
 *
 */
DVZ_EXPORT void dvz_visual_visible(DvzVisual* visual, bool is_visible);



EXTERN_C_OFF

#endif
