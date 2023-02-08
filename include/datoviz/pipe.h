/*************************************************************************************************/
/*  Pipe: wrap a graphics/compute pipeline with bindings and dat/tex resources                   */
/*************************************************************************************************/

#ifndef DVZ_HEADER_PIPE
#define DVZ_HEADER_PIPE



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "vklite.h"



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    DVZ_PIPE_NONE,
    DVZ_PIPE_GRAPHICS,
    DVZ_PIPE_COMPUTE
} DvzPipeType;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzPipe DvzPipe;
typedef struct DvzPipeBinding DvzPipeBinding; // vertex binding
typedef union DvzPipeUnion DvzPipeUnion;

// Forward declarations.
typedef struct DvzDat DvzDat;
typedef struct DvzTex DvzTex;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

// Vertex bindings.
struct DvzPipeBinding
{
    uint32_t binding_idx;
    DvzDat* dat;
    DvzSize offset;
};



union DvzPipeUnion
{
    DvzGraphics graphics;
    DvzCompute compute;
};



struct DvzPipe
{
    DvzObject obj;
    DvzGpu* gpu;

    DvzPipeType type;
    DvzPipeUnion u;
    int flags;

    // Bindings.
    DvzBindings bindings;

    // Vertex buffers.
    DvzPipeBinding vertex_bindings[DVZ_MAX_VERTEX_BINDINGS];
    uint32_t vertex_bindings_count;

    // Index buffer.
    DvzPipeBinding index_binding;

    // Dat resources.
    bool bindings_set[DVZ_MAX_BINDINGS_SIZE];
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Pipe                                                                                         */
/*************************************************************************************************/

/**
 * Create a pipe.
 *
 * @param gpu the GPU
 */
DVZ_EXPORT DvzPipe dvz_pipe(DvzGpu* gpu);



/**
 * Setup a graphics pipe.
 *
 * @param pipe the pipe
 * @param graphics the graphics
 */
DVZ_EXPORT DvzGraphics* dvz_pipe_graphics(DvzPipe* pipe);



/**
 * Setup a compute pipe.
 *
 * @param pipe the pipe
 * @param compute the compute
 */
DVZ_EXPORT DvzCompute* dvz_pipe_compute(DvzPipe* pipe, const char* shader_path);



/**
 * Set up a vertex dat.
 *
 * @param pipe the pipe
 * @param dat the dat with the vertex buffer
 */
DVZ_EXPORT void
dvz_pipe_vertex(DvzPipe* pipe, uint32_t binding_idx, DvzDat* dat_vertex, DvzSize offset);



/**
 * Set up an index dat.
 *
 * @param pipe the pipe
 * @param dat the dat with the index buffer
 */
DVZ_EXPORT void dvz_pipe_index(DvzPipe* pipe, DvzDat* dat_index, DvzSize offset);



/**
 * Set up a uniform dat.
 *
 * @param pipe the pipe
 * @param idx the slot index
 * @param dat the dat with the uniform data
 */
DVZ_EXPORT void dvz_pipe_dat(DvzPipe* pipe, uint32_t idx, DvzDat* dat);



/**
 * Set up a uniform tex.
 *
 * @param pipe the pipe
 * @param idx the slot index
 * @param tex the tex
 * @param sampler the sampler
 */
DVZ_EXPORT void dvz_pipe_tex(DvzPipe* pipe, uint32_t idx, DvzTex* tex, DvzSampler* sampler);



/**
 * Return whether all bindings have been set for a pipe.
 *
 * @param pipe the pipe
 * @returns whether all bindings been set or not
 */
DVZ_EXPORT bool dvz_pipe_complete(DvzPipe* pipe);



/**
 * Create a pipe.
 *
 * @param pipe the pipe
 */
DVZ_EXPORT void dvz_pipe_create(DvzPipe* pipe);



/**
 * Insert a direct draw command in a command buffer.
 *
 * @param pipe the pipe
 * @param cmds the command buffers
 * @param idx the command buffer index
 * @param first_vertex index of the first vertex
 * @param vertex_count number of vertices to draw
 * @param first_instance index of the first instance
 * @param instance_count number of instances to draw
 */
DVZ_EXPORT void dvz_pipe_draw( //
    DvzPipe* pipe, DvzCommands* cmds, uint32_t idx, uint32_t first_vertex, uint32_t vertex_count,
    uint32_t first_instance, uint32_t instance_count);



/**
 * Insert an indexed draw command in a command buffer.
 *
 * @param pipe the pipe
 * @param cmds the command buffers
 * @param idx the command buffer index
 * @param first_index index of the first index
 * @param vertex_offset offset of the vertex
 * @param index_count number of indices to draw
 * @param vertex_count number of vertices to draw
 * @param first_instance index of the first instance
 * @param instance_count number of instances to draw
 */
DVZ_EXPORT void dvz_pipe_draw_indexed(                                  //
    DvzPipe* pipe, DvzCommands* cmds, uint32_t idx,                     //
    uint32_t first_index, uint32_t vertex_offset, uint32_t index_count, //
    uint32_t first_instance, uint32_t instance_count);



/**
 * Insert an indirect draw command in a command buffer.
 *
 * @param pipe the pipe
 * @param cmds the command buffers
 * @param idx the command buffer index
 * @param dat_indirect dat with the indirect draw info
 * @param draw_count the number of indirect draw calls to make
 */
DVZ_EXPORT void dvz_pipe_draw_indirect( //
    DvzPipe* pipe, DvzCommands* cmds, uint32_t idx, DvzDat* dat_indirect, uint32_t draw_count);



/**
 * Insert an indexed indirect draw command in a command buffer.
 *
 * @param pipe the pipe
 * @param cmds the command buffers
 * @param idx the command buffer index
 * @param dat_indirect dat with the indirect draw info
 * @param draw_count the number of indirect draw calls to make
 */
DVZ_EXPORT void dvz_pipe_draw_indexed_indirect( //
    DvzPipe* pipe, DvzCommands* cmds, uint32_t idx, DvzDat* dat_indirect, uint32_t draw_count);



/**
 * Insert a compute command in a command buffer.
 *
 * @param pipe the pipe
 * @param cmds the command buffers
 * @param idx the command buffer index
 */
DVZ_EXPORT void dvz_pipe_run(DvzPipe* pipe, DvzCommands* cmds, uint32_t idx, uvec3 size);



/**
 * Destroy a pipe.
 *
 * @param pipe the pipe
 */
DVZ_EXPORT void dvz_pipe_destroy(DvzPipe* pipe);



EXTERN_C_OFF

#endif
