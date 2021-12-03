/*************************************************************************************************/
/*  Pipe: wrap a graphics/compute pipeline with bindings and dat/tex resources                   */
/*************************************************************************************************/

#ifndef DVZ_HEADER_PIPE
#define DVZ_HEADER_PIPE



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "common.h"
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
    DvzBindings bindings;
    DvzDat* dat_vertex;
    DvzDat* dat_index;
    uint32_t dat_count;
    DvzDat** dats; // uniforms, the first ones are mvp, viewport, params
    uint32_t tex_count;
    DvzTex** texs;
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
DVZ_EXPORT void dvz_pipe_graphics(DvzPipe* pipe, DvzGraphics* graphics);



/**
 * Setup a compute pipe.
 *
 * @param pipe the pipe
 * @param compute the compute
 */
DVZ_EXPORT void dvz_pipe_compute(DvzPipe* pipe, DvzCompute* compute);



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
 * Destroy a pipe.
 *
 * @param pipe the pipe
 */
DVZ_EXPORT void dvz_pipe_destroy(DvzPipe* pipe);



EXTERN_C_OFF

#endif
