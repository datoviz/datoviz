/*************************************************************************************************/
/* Baker                                                                                         */
/*************************************************************************************************/

#ifndef DVZ_HEADER_BAKER
#define DVZ_HEADER_BAKER



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_enums.h"
#include "_log.h"
#include "datoviz_math.h"
#include "scene/dual.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzBaker DvzBaker;
typedef struct DvzBakerVertex DvzBakerVertex;
typedef struct DvzBakerAttr DvzBakerAttr;
// typedef struct DvzBakerDescriptor DvzBakerDescriptor;
// typedef union DvzBakerDescriptorUnion DvzBakerDescriptorUnion;
// typedef struct DvzBakerParam DvzBakerParam;

// Forward declarations.
typedef struct DvzArray DvzArray;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    DVZ_BAKER_FLAGS_DEFAULT = 0x000000,
    DVZ_BAKER_FLAGS_VERTEX_NONMAPPABLE = 0x400000, // WARNING: needs to match DVZ_VISUAL_FLAGS
    DVZ_BAKER_FLAGS_INDEX_NONMAPPABLE = 0x500000,  // WARNING: needs to match DVZ_VISUAL_FLAGS
} DvzBakerFlags;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzBakerAttr
{
    uint32_t attr_idx;
    uint32_t binding_idx;
    DvzSize offset;
    DvzSize item_size;
};

struct DvzBakerVertex
{
    uint32_t binding_idx;
    DvzSize stride;
    DvzDual dual;
    bool shared; // if a dual is shared, it won't be bound upon baker creation
};

// union DvzBakerDescriptorUnion
// {
//     struct
//     {
//         DvzSize item_size;
//         DvzDual dual;
//     } dat;
//     struct
//     {
//         DvzId tex;

//         // NOTE: are the following necessary?
//         DvzTexDims dims;
//         DvzFormat format;
//         uvec3 shape;
//     } tex;
// };

// struct DvzBakerDescriptor
// {
//     uint32_t slot_idx;
//     int flags;
//     bool shared; // if a dual is shared, it won't be bound upon baker creation
//     DvzSlotType type;
//     DvzBakerDescriptorUnion u;
// };

// struct DvzBakerParam
// {
//     uint32_t prop_idx;
//     uint32_t slot_idx;
//     DvzSize offset;
//     DvzSize size;
// };



struct DvzBaker
{
    DvzBatch* batch;
    int flags;
    bool is_indirect;

    uint32_t binding_count;
    uint32_t attr_count;
    uint32_t slot_count;

    DvzBakerAttr vertex_attrs[DVZ_MAX_VERTEX_ATTRS];
    DvzBakerVertex vertex_bindings[DVZ_MAX_VERTEX_BINDINGS];
    // DvzBakerDescriptor descriptors[DVZ_MAX_BINDINGS];
    // DvzBakerParam params[DVZ_MAX_PARAMS];

    DvzDual index; // index buffer
    bool index_shared;
    DvzDual indirect; // indirect buffer
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Baker lifecycle                                                                              */
/*************************************************************************************************/

// 00xx: which attributes should be in a different buf (8 max)
// xx00: which attributes should be constants
/**
 *
 */
DvzBaker* dvz_baker(DvzBatch* batch, int flags);



// Internal function, used to instantiate the DvzDual instances.
/**
 *
 */
void dvz_baker_create(DvzBaker* baker, uint32_t index_count, uint32_t vertex_count);



// emit the dat update commands to synchronize the dual arrays on the GPU
/**
 *
 */
void dvz_baker_update(DvzBaker* baker);



/**
 *
 */
void dvz_baker_destroy(DvzBaker* baker);



/*************************************************************************************************/
/*  Baker specification                                                                          */
/*************************************************************************************************/

// declare a vertex binding
/**
 *
 */
void dvz_baker_vertex(DvzBaker* baker, uint32_t binding_idx, DvzSize stride);



// declare a GLSL attribute
/**
 *
 */
void dvz_baker_attr(
    DvzBaker* baker, uint32_t attr_idx, uint32_t binding_idx, DvzSize offset, DvzSize item_size);



// declare a descriptor slot for a dat
/**
 *
 */
// void dvz_baker_slot_dat(DvzBaker* baker, uint32_t slot_idx, DvzSize item_size);



// declare a descriptor slot for a tex
/**
 *
 */
// void dvz_baker_slot_tex(DvzBaker* baker, uint32_t slot_idx);



/**
 *
 */
// void dvz_baker_property(
//     DvzBaker* baker, uint32_t prop_idx, uint32_t slot_idx, DvzSize offset, DvzSize size);



/**
 *
 */
void dvz_baker_indirect(DvzBaker* baker);



/*************************************************************************************************/
/*  Baker sharing                                                                                */
/*************************************************************************************************/

/**
 *
 */
void dvz_baker_share_vertex(DvzBaker* baker, uint32_t binding_idx);



/**
 *
 */
// void dvz_baker_share_binding(DvzBaker* baker, uint32_t binding_idx);



/**
 *
 */
void dvz_baker_share_index(DvzBaker* baker);



/*************************************************************************************************/
/*  Baker data                                                                                   */
/*************************************************************************************************/

/**
 *
 */
void dvz_baker_data(
    DvzBaker* baker, uint32_t attr_idx, uint32_t first, uint32_t count, void* data);



/**
 *
 */
void dvz_baker_resize(DvzBaker* baker, uint32_t vertex_count, uint32_t index_count);



/**
 *
 */
void dvz_baker_repeat(
    DvzBaker* baker, uint32_t attr_idx, uint32_t first, uint32_t count, uint32_t repeats,
    void* data);



/**
 *
 */
void dvz_baker_quads(
    DvzBaker* baker, uint32_t attr_idx, uint32_t first, uint32_t count, vec4* ul_lr);



/**
 *
 */
void dvz_baker_index(DvzBaker* baker, uint32_t first, uint32_t count, DvzIndex* data);



/**
 *
 */
// void dvz_baker_uniform(DvzBaker* baker, uint32_t binding_idx, DvzSize size, void*
// data);



/**
 *
 */
// void dvz_baker_param(DvzBaker* baker, uint32_t prop_idx, void* data);



EXTERN_C_OFF

#endif
