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
typedef union DvzPipeUnion DvzPipeUnion;

// Forward declarations.
typedef struct DvzDat DvzDat;
typedef struct DvzTex DvzTex;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

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

    // Vertex and index buffers.
    DvzDat* dat_vertex;
    DvzDat* dat_index;

    // Dat resources.
    bool bindings_set[DVZ_MAX_BINDINGS_SIZE];
    // uint32_t dat_count;
    // DvzDat* dats[DVZ_MAX_BINDINGS_SIZE]; // uniforms, the first ones are mvp, viewport, params

    // Texture resources.
    // uint32_t tex_count;
    // DvzTex* texs[DVZ_MAX_BINDINGS_SIZE];

    // Sampler resources.
    // uint32_t tex_count;
    // DvzSampler* samplers[DVZ_MAX_BINDINGS_SIZE];
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
 * @param count the number of descriptor sets (= number of swapchain images)
 */
DVZ_EXPORT DvzGraphics* dvz_pipe_graphics(DvzPipe* pipe, uint32_t dset_count);



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
DVZ_EXPORT void dvz_pipe_vertex(DvzPipe* pipe, DvzDat* dat_vertex);



/**
 * Set up an index dat.
 *
 * @param pipe the pipe
 * @param dat the dat with the index buffer
 */
DVZ_EXPORT void dvz_pipe_index(DvzPipe* pipe, DvzDat* dat_index);



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
 */
DVZ_EXPORT void dvz_pipe_draw( //
    DvzPipe* pipe, DvzCommands* cmds, uint32_t idx, uint32_t first_vertex, uint32_t vertex_count);



/**
 * Insert an indexed draw command in a command buffer.
 *
 * @param pipe the pipe
 * @param cmds the command buffers
 * @param idx the command buffer index
 * @param first_index index of the first index
 * @param vertex_offset offset of the vertex
 * @param index_count number of indices to draw
 */
DVZ_EXPORT void dvz_pipe_draw_indexed(              //
    DvzPipe* pipe, DvzCommands* cmds, uint32_t idx, //
    uint32_t first_index, uint32_t vertex_offset, uint32_t index_count);



/**
 * Insert an indirect draw command in a command buffer.
 *
 * @param pipe the pipe
 * @param cmds the command buffers
 * @param idx the command buffer index
 * @param dat_indirect dat with the indirect draw info
 */
DVZ_EXPORT void dvz_pipe_draw_indirect( //
    DvzPipe* pipe, DvzCommands* cmds, uint32_t idx, DvzDat* dat_indirect);



/**
 * Insert an indexed indirect draw command in a command buffer.
 *
 * @param pipe the pipe
 * @param cmds the command buffers
 * @param idx the command buffer index
 * @param dat_indirect dat with the indirect draw info
 */
DVZ_EXPORT void dvz_pipe_draw_indexed_indirect( //
    DvzPipe* pipe, DvzCommands* cmds, uint32_t idx, DvzDat* dat_indirect);



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
