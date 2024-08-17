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
#include "datoviz_types.h"
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
    DvzView* view;
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
    uint32_t index_count;
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
DvzVisual* dvz_visual(DvzBatch* batch, DvzPrimitiveTopology primitive, int flags);



/**
 *
 */
void dvz_visual_destroy(DvzVisual* visual);



/*************************************************************************************************/
/*  Visual fixed pipeline                                                                        */
/*************************************************************************************************/

/**
 *
 */
void dvz_visual_primitive(DvzVisual* visual, DvzPrimitiveTopology primitive);


/**
 *
 */
void dvz_visual_blend(DvzVisual* visual, DvzBlendType blend_type);



/**
 *
 */
void dvz_visual_polygon(DvzVisual* visual, DvzPolygonMode polygon_mode);



/**
 *
 */
void dvz_visual_cull(DvzVisual* visual, DvzCullMode cull_mode);



/**
 *
 */
void dvz_visual_front(DvzVisual* visual, DvzFrontFace front_face);



/**
 *
 */
void dvz_visual_specialization(
    DvzVisual* visual, DvzShaderType shader, uint32_t idx, DvzSize size, void* value);



/*************************************************************************************************/
/*  Visual declaration                                                                           */
/*************************************************************************************************/

/**
 *
 */
void dvz_visual_spirv(
    DvzVisual* visual, DvzShaderType type, DvzSize size, const unsigned char* buffer);



/**
 *
 */
void dvz_visual_shader(DvzVisual* visual, const char* name);



/**
 *
 */
void dvz_visual_resize(
    DvzVisual* visual, uint32_t item_count, uint32_t vertex_count, uint32_t index_count);



/**
 *
 */
void dvz_visual_groups(DvzVisual* visual, uint32_t group_count, uint32_t* group_sizes);



/**
 *
 */
void dvz_visual_attr(
    DvzVisual* visual, uint32_t attr_idx, DvzSize offset, DvzSize item_size, //
    DvzFormat format, int flags);



/**
 *
 */
void dvz_visual_stride(DvzVisual* visual, uint32_t binding_idx, DvzSize stride);



/**
 *
 */
void dvz_visual_slot(DvzVisual* visual, uint32_t slot_idx, DvzSlotType type);



/**
 *
 */
DvzParams* dvz_visual_params(DvzVisual* visual, uint32_t slot_idx, DvzSize size);



/**
 *
 */
void dvz_visual_dat(DvzVisual* visual, uint32_t slot_idx, DvzId dat);



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
void dvz_visual_alloc(
    DvzVisual* visual, uint32_t item_count, uint32_t vertex_count, uint32_t index_count);



/**
 *
 */
void dvz_visual_transform(DvzVisual* visual, DvzTransform* tr, uint32_t vertex_attr);



/*************************************************************************************************/
/*  Visual common bindings                                                                       */
/*************************************************************************************************/

// NOTE: only used for testing, otherwise the viewset takes care of this (transform & view)

/**
 *
 */
void dvz_visual_mvp(DvzVisual* visual, DvzMVP* mvp); // update the MVP



/**
 *
 */
void dvz_visual_viewport(DvzVisual* visual, DvzViewport* viewport); // update the viewport



/*************************************************************************************************/
/*  Visual data                                                                                  */
/*************************************************************************************************/

/**
 *
 */
void dvz_visual_data(
    DvzVisual* visual, uint32_t attr_idx, uint32_t first, uint32_t count, void* data);



/**
 *
 */
void dvz_visual_quads(
    DvzVisual* visual, uint32_t attr_idx, uint32_t first, uint32_t count, vec4* tl_br);



/**
 *
 */
void dvz_visual_index(DvzVisual* visual, uint32_t first, uint32_t count, DvzIndex* data);



void dvz_visual_param(DvzVisual* visual, uint32_t slot_idx, uint32_t attr_idx, void* item);



/*************************************************************************************************/
/*  Visual drawing internal functions                                                            */
/*************************************************************************************************/

/**
 *
 */
void dvz_visual_drawspec(
    DvzVisual* visual, uint32_t draw_first, uint32_t draw_count, //
    uint32_t first_instance, uint32_t instance_count);



/**
 *
 */
void dvz_visual_instance(
    DvzVisual* visual, DvzId canvas, uint32_t first, uint32_t vertex_offset, uint32_t count,
    uint32_t first_instance, uint32_t instance_count);



/**
 *
 */
void dvz_visual_indirect(DvzVisual* visual, DvzId canvas, uint32_t draw_count);



/**
 *
 */
void dvz_visual_record(DvzVisual* visual, DvzId canvas);



/**
 *
 */
void dvz_visual_callback(DvzVisual* visual, DvzVisualCallback callback);



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

// NOTE: the caller must FREE the output
static inline void* _repeat(uint32_t item_count, DvzSize item_size, void* value)
{
    void* out = (vec3*)calloc(item_count, item_size);
    for (uint32_t i = 0; i < item_count; i++)
    {
        memcpy((void*)((uint64_t)out + i * item_size), value, item_size);
    }
    return out;
}



static inline bool _check_groups(uint32_t item_count, uint32_t group_count, uint32_t* group_size)
{
    if (item_count == 0)
    {
        ASSERT(group_count == 0);
        return true;
    }
    if (!group_size)
    {
        ASSERT(group_count == 0);
        return true;
    }

    ANN(group_size);
    ASSERT(item_count > 0);
    ASSERT(group_count > 0);

    // Check that the group sizes are consistent with the number of items.
    uint32_t total_count = 0;
    for (uint32_t i = 0; i < group_count; i++)
    {
        total_count += group_size[i];
    }
    if (total_count != item_count)
    {
        log_error("inconsistent group size: %d != %d", total_count, item_count);
        return false;
    }
    return true;
}



// NOTE: the caller must FREE the output
static inline void* _repeat_group(
    DvzSize item_size, uint32_t item_count, uint32_t group_count, uint32_t* group_size,
    void* group_values, bool uniform)
{
    ASSERT(item_size > 0);
    ASSERT(item_count > 0);
    ASSERT(group_count > 0);
    ASSERT(group_size != NULL);
    ANN(group_values);

    if (!_check_groups(item_count, group_count, group_size))
        return NULL;

    void* out = (vec3*)calloc(item_count, item_size);
    uint32_t k = 0;
    DvzSize item_size_src = uniform ? 0 : item_size;
    for (uint32_t i = 0; i < group_count; i++)
    {
        for (uint32_t j = 0; j < group_size[i]; j++)
        {
            ASSERT(k < item_count);
            memcpy(
                (void*)((uint64_t)out + (k++) * item_size),
                (void*)((uint64_t)group_values + i * item_size_src), //
                item_size);
        }
    }
    ASSERT(k == item_count);
    return out;
}



EXTERN_C_OFF

#endif
