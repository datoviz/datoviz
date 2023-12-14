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
#include "params.h"
#include "viewport.h"



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#define FIELD(t, f) offsetof(t, f), fsizeof(t, f)



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzVisual DvzVisual;
typedef struct DvzVisualAttr DvzVisualAttr;

// Forward declarations.
typedef struct DvzBatch DvzBatch;
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
    DVZ_ATTR_FLAGS_REPEAT_X2 = 0x1200,
    DVZ_ATTR_FLAGS_REPEAT_X4 = 0x1400,
    DVZ_ATTR_FLAGS_REPEAT_X6 = 0x1600,
    DVZ_ATTR_FLAGS_REPEAT_X8 = 0x1800,

    // DVZ_ATTR_FLAGS_QUAD = 0x2000,
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
    DvzBatch* batch;
    int flags;
    void* user_data;

    DvzId graphics_id;
    DvzBaker* baker;
    DvzSize strides[DVZ_MAX_BINDINGS];
    DvzVisualAttr attrs[DVZ_MAX_VERTEX_ATTRS];
    DvzTransform* transforms[DVZ_MAX_VERTEX_ATTRS];

    // Bindings
    DvzParams* params[DVZ_MAX_BINDINGS]; // dats
    DvzId texs[DVZ_MAX_BINDINGS];        // texs

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

/**
 *
 */
DVZ_EXPORT DvzVisual* dvz_visual(DvzBatch* batch, DvzPrimitiveTopology primitive, int flags);



/**
 *
 */
DVZ_EXPORT void dvz_visual_update(DvzVisual* visual);



/**
 *
 */
DVZ_EXPORT void dvz_visual_destroy(DvzVisual* visual);



/*************************************************************************************************/
/*  Visual fixed pipeline                                                                        */
/*************************************************************************************************/

/**
 *
 */
DVZ_EXPORT void dvz_visual_primitive(DvzVisual* visual, DvzPrimitiveTopology primitive);


/**
 *
 */
DVZ_EXPORT void dvz_visual_blend(DvzVisual* visual, DvzBlendType blend_type);



/**
 *
 */
DVZ_EXPORT void dvz_visual_depth(DvzVisual* visual, DvzDepthTest depth_test);



/**
 *
 */
DVZ_EXPORT void dvz_visual_polygon(DvzVisual* visual, DvzPolygonMode polygon_mode);



/**
 *
 */
DVZ_EXPORT void dvz_visual_cull(DvzVisual* visual, DvzCullMode cull_mode);



/**
 *
 */
DVZ_EXPORT void dvz_visual_front(DvzVisual* visual, DvzFrontFace front_face);



/**
 *
 */
DVZ_EXPORT void dvz_visual_specialization(
    DvzVisual* visual, DvzShaderType shader, uint32_t idx, DvzSize size, void* value);



/*************************************************************************************************/
/*  Visual declaration                                                                           */
/*************************************************************************************************/

/**
 *
 */
DVZ_EXPORT void
dvz_visual_spirv(DvzVisual* visual, DvzShaderType type, DvzSize size, const unsigned char* buffer);



/**
 *
 */
DVZ_EXPORT void dvz_visual_shader(DvzVisual* visual, const char* name);



/**
 *
 */
DVZ_EXPORT void dvz_visual_resize(DvzVisual* visual, uint32_t item_count, uint32_t vertex_count);



/**
 *
 */
DVZ_EXPORT void dvz_visual_groups(DvzVisual* visual, uint32_t group_count, uint32_t* group_sizes);



/**
 *
 */
DVZ_EXPORT void dvz_visual_attr(
    DvzVisual* visual, uint32_t attr_idx, DvzSize offset, DvzSize item_size, //
    DvzFormat format, int flags);



/**
 *
 */
DVZ_EXPORT void dvz_visual_stride(DvzVisual* visual, uint32_t binding_idx, DvzSize stride);



/**
 *
 */
DVZ_EXPORT void dvz_visual_slot(DvzVisual* visual, uint32_t slot_idx, DvzSlotType type);



/**
 *
 */
DVZ_EXPORT DvzParams* dvz_visual_params(DvzVisual* visual, uint32_t slot_idx, DvzSize size);



/**
 *
 */
DVZ_EXPORT void dvz_visual_dat(DvzVisual* visual, uint32_t slot_idx, DvzId dat);



/**
 *
 */
void dvz_visual_tex(DvzVisual* visual, uint32_t slot_idx, DvzId tex, DvzId sampler, uvec3 offset);



/*************************************************************************************************/
/*  Visual creation                                                                              */
/*************************************************************************************************/

/**
 *
 */
DVZ_EXPORT void dvz_visual_alloc(
    DvzVisual* visual, uint32_t item_count, uint32_t vertex_count, uint32_t index_count);



/**
 *
 */
DVZ_EXPORT void dvz_visual_transform(DvzVisual* visual, DvzTransform* tr, uint32_t vertex_attr);



/*************************************************************************************************/
/*  Visual common bindings                                                                       */
/*************************************************************************************************/

// NOTE: only used for testing, otherwise the viewset takes care of this (transform & view)

/**
 *
 */
DVZ_EXPORT void dvz_visual_mvp(DvzVisual* visual, DvzMVP* mvp); // update the MVP



/**
 *
 */
DVZ_EXPORT void
dvz_visual_viewport(DvzVisual* visual, DvzViewport* viewport); // update the viewport



/*************************************************************************************************/
/*  Visual data                                                                                  */
/*************************************************************************************************/

/**
 *
 */
DVZ_EXPORT void
dvz_visual_data(DvzVisual* visual, uint32_t attr_idx, uint32_t first, uint32_t count, void* data);



/**
 *
 */
DVZ_EXPORT void dvz_visual_quads(
    DvzVisual* visual, uint32_t attr_idx, uint32_t first, uint32_t count, vec4* ul_lr);



/**
 *
 */
DVZ_EXPORT void
dvz_visual_index(DvzVisual* visual, uint32_t first, uint32_t count, DvzIndex* data);



DVZ_EXPORT void
dvz_visual_param(DvzVisual* visual, uint32_t slot_idx, uint32_t attr_idx, void* item);



/*************************************************************************************************/
/*  Visual drawing internal functions                                                            */
/*************************************************************************************************/

/**
 *
 */
DVZ_EXPORT void dvz_visual_instance(
    DvzVisual* visual, DvzId canvas, uint32_t first, uint32_t vertex_offset, uint32_t count,
    uint32_t first_instance, uint32_t instance_count);



/**
 *
 */
DVZ_EXPORT void dvz_visual_indirect(DvzVisual* visual, DvzId canvas, uint32_t draw_count);



/**
 *
 */
DVZ_EXPORT void dvz_visual_record(DvzVisual* visual, DvzId canvas);



/**
 *
 */
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
