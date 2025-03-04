/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Request                                                                                      */
/*************************************************************************************************/

#ifndef DVZ_HEADER_PUBLIC_REQUEST
#define DVZ_HEADER_PUBLIC_REQUEST



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz_app.h"
#include "datoviz_enums.h"
#include "datoviz_types.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_REQUEST_VERSION 1
// filename for the dump, to activate with the DVZ_DUMP=1 environment variable
#define DVZ_DUMP_FILENAME          "requests.dvz"
#define DVZ_BATCH_DEFAULT_CAPACITY 4



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Graphics request flags.
// NOTE: these flags are also passed to PipelibFlags.
typedef enum
{
    DVZ_GRAPHICS_REQUEST_FLAGS_NONE = 0x000000,
    DVZ_GRAPHICS_REQUEST_FLAGS_OFFSCREEN = 0x008000,
} DvzGraphicsRequestFlags;



// Print flags.
typedef enum
{
    DVZ_PRINT_FLAGS_NONE = 0x0000,
    DVZ_PRINT_FLAGS_ALL = 0x0001,
    DVZ_PRINT_FLAGS_SMALL = 0x0003,
} DvzPrintFlagsFlags;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

// Forward declarations.
typedef struct DvzPipe DvzPipe;
typedef struct DvzFifo DvzFifo;
typedef struct DvzList DvzList;
typedef struct DvzRequester DvzRequester;
typedef struct DvzBatch DvzBatch;
typedef uint64_t DvzId;



EXTERN_C_ON

/*************************************************************************************************/
/*  Requester                                                                                    */
/*************************************************************************************************/

/**
 * Create a requester, used to create requests.
 *
 * @returns the requester struct
 */
DVZ_EXPORT DvzRequester* dvz_requester(void);



/**
 * Destroy a requester.
 *
 * @param rqr the requester
 */
DVZ_EXPORT void dvz_requester_destroy(DvzRequester* rqr);



/*************************************************************************************************/
/*  Low-level structures                                                                         */
/*************************************************************************************************/

/**
 * Create a MVP structure.
 *
 * @param model the model matrix
 * @param view the view matrix
 * @param proj the projection matrix
 * @param[out] mvp the MVP structure
 */
DVZ_EXPORT void dvz_mvp(mat4 model, mat4 view, mat4 proj, DvzMVP* mvp);



/**
 * Return a default DvzMVP struct
 *
 * @param[out] mvp the DvzMVP struct
 */
DVZ_EXPORT void dvz_mvp_default(DvzMVP* mvp);



/**
 * Return a default viewport
 *
 * @param width the viewport width, in framebuffer pixels
 * @param height the viewport height, in framebuffer pixels
 * @param[out] viewport the viewport
 */
DVZ_EXPORT void dvz_viewport_default(uint32_t width, uint32_t height, DvzViewport* viewport);



/*************************************************************************************************/
/*  Batch functions                                                                              */
/*************************************************************************************************/

/**
 * Create a batch holding a number of requests.
 */
DVZ_EXPORT DvzBatch* dvz_batch(void);



/**
 * Remove all requests in a batch.
 *
 * @param batch the batch
 */
DVZ_EXPORT void dvz_batch_clear(DvzBatch* batch);



/**
 * Add a request to a batch.
 *
 * @param batch the batch
 * @param req the request
 */
DVZ_EXPORT void dvz_batch_add(DvzBatch* batch, DvzRequest req);



/**
 * Set the description of the last added request.
 *
 * @param batch the batch
 * @param desc the description
 */
DVZ_EXPORT void dvz_batch_desc(DvzBatch* batch, const char* desc);



/**
 * Return a pointer to the array of all requests in the batch.
 *
 * @param batch the batch
 */
DVZ_EXPORT DvzRequest* dvz_batch_requests(DvzBatch* batch);



/**
 * Return the number of requests in the batch.
 *
 * @param batch the batch
 */
DVZ_EXPORT uint32_t dvz_batch_size(DvzBatch* batch);



/**
 * Display information about all requests in the batch.
 *
 * @param batch the batch
 * @param flags the flags
 */
DVZ_EXPORT void dvz_batch_print(DvzBatch* batch, int flags);



/**
 * Export requests in a YAML file.
 *
 * @param batch the batch
 * @param filename the YAML filename
 */
DVZ_EXPORT void dvz_batch_yaml(DvzBatch* batch, const char* filename);



/**
 * Dump all batch requests in raw binary file.
 *
 * @param batch the batch
 * @param filename the dump filename
 */
DVZ_EXPORT int dvz_batch_dump(DvzBatch* batch, const char* filename);



/**
 * Load a dump of batch requests into an existing batch object.
 *
 * @param batch the batch
 * @param filename the dump filename
 */
DVZ_EXPORT void dvz_batch_load(DvzBatch* batch, const char* filename);



/**
 * Create a copy of a batch.
 *
 * @param batch the batch
 */
DVZ_EXPORT DvzBatch* dvz_batch_copy(DvzBatch* batch);



/**
 * Destroy a batch.
 *
 * @param batch the batch
 */
DVZ_EXPORT void dvz_batch_destroy(DvzBatch* batch);



/*************************************************************************************************/
/*  Requester functions                                                                          */
/*************************************************************************************************/

/**
 * Add a batch's requests to a requester.
 *
 * @param rqr the requester
 * @param batch the batch
 */
DVZ_EXPORT void dvz_requester_commit(DvzRequester* rqr, DvzBatch* batch);



/**
 * Return the requests in the requester and clear it.
 *
 * NOTE: the caller must free the output.
 *
 * @param rqr the requester
 * @param[out] count pointer to the number of requests, set by this function
 * @returns an array with all requests in the requester
 */
DVZ_EXPORT DvzBatch* dvz_requester_flush(DvzRequester* rqr, uint32_t* count);



/**
 * Display information about a request.
 *
 * @param req the request
 * @param flags the flags
 */
DVZ_EXPORT void dvz_request_print(DvzRequest* req, int flags);



/*************************************************************************************************/
/*  Canvas                                                                                       */
/*************************************************************************************************/

/**
 * Create a request for canvas creation.
 *
 * A canvas is a live window on which to render.
 *
 * NOTE: background color not implemented yet
 *
 * @param batch the batch
 * @param width the canvas width (in screen pixels)
 * @param height the canvas height (in screen pixels)
 * @param background the background color
 * @param flags the canvas creation flags
 * @returns the request, containing a newly-generated id for the canvas to be created
 */
DVZ_EXPORT DvzRequest
dvz_create_canvas(DvzBatch* batch, uint32_t width, uint32_t height, cvec4 background, int flags);



/**
 * Change the background color of the canvas.
 *
 * @param batch the batch
 * @param id the canvas id
 * @param background the background color
 * @returns the request
 */
DVZ_EXPORT DvzRequest dvz_set_background(DvzBatch* batch, DvzId id, cvec4 background);



/**
 * Create a request for a canvas redraw (command buffer submission).
 *
 * @param batch the batch
 * @param id the canvas id
 * @returns the request
 */
DVZ_EXPORT DvzRequest dvz_update_canvas(DvzBatch* batch, DvzId id);



/**
 * Create a request to resize an offscreen canvas (regular canvases are resized by the client).
 *
 * @param batch the batch
 * @param canvas the canvas id
 * @param width the new canvas width
 * @param height the new canvas height
 * @returns the request
 */
DVZ_EXPORT DvzRequest
dvz_resize_canvas(DvzBatch* batch, DvzId canvas, uint32_t width, uint32_t height);



/**
 * Create a request for a canvas deletion.
 *
 * @param batch the batch
 * @param id the canvas id
 * @returns the request
 */
DVZ_EXPORT DvzRequest dvz_delete_canvas(DvzBatch* batch, DvzId id);



/*************************************************************************************************/
/*  Dat                                                                                          */
/*************************************************************************************************/

/**
 * Create a request for a dat creation.
 *
 * @param batch the batch
 * @param type the buffer type
 * @param size the dat size, in bytes
 * @param flags the dat creation flags
 * @returns the request, containing a newly-generated id for the dat to be created
 */
DVZ_EXPORT DvzRequest dvz_create_dat(DvzBatch* batch, DvzBufferType type, DvzSize size, int flags);



/**
 * Create a request to resize a dat.
 *
 * @param batch the batch
 * @param dat the dat id
 * @param size the new dat size, in bytes
 * @returns the request
 */
DVZ_EXPORT DvzRequest dvz_resize_dat(DvzBatch* batch, DvzId dat, DvzSize size);



/**
 * Create a request for dat upload.
 *
 * NOTE: this function makes a COPY of the buffer to ensure it will live until the upload actually
 * occurs. The copy will be freed automatically as soon as it's safe.
 *
 * @param batch the batch
 * @param dat the id of the dat to upload to
 * @param offset the byte offset of the upload transfer
 * @param size the number of bytes in data to transfer
 * @param data a pointer to the data to upload
 * @param flags the upload flags
 * @returns the request
 */
DVZ_EXPORT DvzRequest
dvz_upload_dat(DvzBatch* batch, DvzId dat, DvzSize offset, DvzSize size, void* data, int flags);



/**
 * Create a request for dat deletion.
 *
 * @param batch the batch
 * @param id the dat id
 * @returns the request
 */
DVZ_EXPORT DvzRequest dvz_delete_dat(DvzBatch* batch, DvzId id);



/*************************************************************************************************/
/*  Tex                                                                                          */
/*************************************************************************************************/

/**
 * Create a request for a tex creation.
 *
 * @param batch the batch
 * @param dims the number of dimensions, 1, 2, or 3
 * @param format the image format
 * @param shape the texture shape
 * @param flags the dat creation flags
 * @returns the request, containing a newly-generated id for the tex to be created
 */
DVZ_EXPORT DvzRequest
dvz_create_tex(DvzBatch* batch, DvzTexDims dims, DvzFormat format, uvec3 shape, int flags);



/**
 * Create a request to resize a tex.
 *
 * @param batch the batch
 * @param tex the tex id
 * @param shape the new tex shape
 * @returns the request
 */
DVZ_EXPORT DvzRequest dvz_resize_tex(DvzBatch* batch, DvzId tex, uvec3 shape);



/**
 * Create a request for tex upload.
 *
 * NOTE: this function makes a COPY of the buffer to ensure it will live until the upload actually
 * occurs. The copy will be freed automatically as soon as it's safe.
 *
 * @param batch the batch
 * @param tex the id of the tex to upload to
 * @param offset the offset
 * @param shape the shape
 * @param size the number of bytes in data to transfer
 * @param data a pointer to the data to upload
 * @param flags the upload flags
 * @returns the request
 */
DVZ_EXPORT DvzRequest dvz_upload_tex(
    DvzBatch* batch, DvzId tex, uvec3 offset, uvec3 shape, DvzSize size, void* data, int flags);



/**
 * Create a request for tex deletion.
 *
 * @param batch the batch
 * @param id the tex id
 * @returns the request
 */
DVZ_EXPORT DvzRequest dvz_delete_tex(DvzBatch* batch, DvzId id);



/*************************************************************************************************/
/*  Sampler                                                                                      */
/*************************************************************************************************/

/**
 * Create a request for a sampler creation.
 *
 * @param batch the batch
 * @param filter the sampler filter
 * @param mode the sampler address mode
 * @returns the request, containing a newly-generated id for the sampler to be created
 */
DVZ_EXPORT DvzRequest
dvz_create_sampler(DvzBatch* batch, DvzFilter filter, DvzSamplerAddressMode mode);



/**
 * Create a request for sampler deletion.
 *
 * @param batch the batch
 * @param id the sampler id
 * @returns the request
 */
DVZ_EXPORT DvzRequest dvz_delete_sampler(DvzBatch* batch, DvzId id);



/*************************************************************************************************/
/*  Shaders                                                                                      */
/*************************************************************************************************/

/**
 * Create a request for GLSL shader creation.
 *
 * @param batch the batch
 * @param shader_type the shader type
 * @param code an ASCII string with the GLSL code
 * @returns the request
 */
DVZ_EXPORT DvzRequest
dvz_create_glsl(DvzBatch* batch, DvzShaderType shader_type, const char* code);



/**
 * Create a request for SPIR-V shader creation.
 *
 * @param batch the batch
 * @param shader_type the shader type
 * @param size the size in bytes of the SPIR-V buffer
 * @param buffer pointer to a buffer with the SPIR-V bytecode
 * @returns the request
 */
DVZ_EXPORT DvzRequest dvz_create_spirv(
    DvzBatch* batch, DvzShaderType shader_type, DvzSize size, const unsigned char* buffer);



/*************************************************************************************************/
/*  Graphics                                                                                     */
/*************************************************************************************************/

/**
 * Create a request for a builtin graphics pipe creation.
 *
 * @param batch the batch
 * @param type the graphics type
 * @param flags the graphics creation flags
 * @returns the request, containing a newly-generated id for the graphics pipe to be created
 */
DVZ_EXPORT DvzRequest dvz_create_graphics(DvzBatch* batch, DvzGraphicsType type, int flags);



/**
 * Create a request for setting the primitive topology of a graphics pipe.
 *
 * @param batch the batch
 * @param graphics the graphics pipe id
 * @param primitive the graphics primitive topology
 * @returns the request
 */
DVZ_EXPORT DvzRequest
dvz_set_primitive(DvzBatch* batch, DvzId graphics, DvzPrimitiveTopology primitive);



/**
 * Create a request for setting the blend type of a graphics pipe.
 *
 * @param batch the batch
 * @param graphics the graphics pipe id
 * @param blend_type the graphics blend type
 * @returns the request
 */
DVZ_EXPORT DvzRequest dvz_set_blend(DvzBatch* batch, DvzId graphics, DvzBlendType blend_type);



/**
 * Create a request for setting the color mask of a graphics pipe.
 *
 * @param batch the batch
 * @param graphics the graphics pipe id
 * @param mask the mask with RGBA boolean masks on the lower bits
 * @returns the request
 */
DVZ_EXPORT DvzRequest dvz_set_mask(DvzBatch* batch, DvzId graphics, int32_t mask);



/**
 * Create a request for setting the depth test of a graphics pipe.
 *
 * @param batch the batch
 * @param graphics the graphics pipe id
 * @param depth_test the graphics depth test
 * @returns the request
 */
DVZ_EXPORT DvzRequest dvz_set_depth(DvzBatch* batch, DvzId graphics, DvzDepthTest depth_test);



/**
 * Create a request for setting the polygon mode of a graphics pipe.
 *
 * @param batch the batch
 * @param graphics the graphics pipe id
 * @param polygon_mode the polygon mode
 * @returns the request
 */
DVZ_EXPORT DvzRequest
dvz_set_polygon(DvzBatch* batch, DvzId graphics, DvzPolygonMode polygon_mode);



/**
 * Create a request for setting the cull mode of a graphics pipe.
 *
 * @param batch the batch
 * @param graphics the graphics pipe id
 * @param cull_mode the cull mode
 * @returns the request
 */
DVZ_EXPORT DvzRequest dvz_set_cull(DvzBatch* batch, DvzId graphics, DvzCullMode cull_mode);



/**
 * Create a request for setting the front face of a graphics pipe.
 *
 * @param batch the batch
 * @param graphics the graphics pipe id
 * @param front_face the front face
 * @returns the request
 */
DVZ_EXPORT DvzRequest dvz_set_front(DvzBatch* batch, DvzId graphics, DvzFrontFace front_face);



/**
 * Create a request for setting a shader a graphics pipe.
 *
 * @param batch the batch
 * @param graphics the graphics pipe id
 * @param shader the id of the shader object
 * @returns the request
 */
DVZ_EXPORT DvzRequest dvz_set_shader(DvzBatch* batch, DvzId graphics, DvzId shader);



/**
 * Create a request for setting a vertex binding of a graphics pipe.
 *
 * @param batch the batch
 * @param graphics the graphics pipe id
 * @param binding_idx the index of the vertex binding
 * @param stride the binding stride
 * @param input_rate the vertex input rate, per-vertex or per-instance
 * @returns the request
 */
DVZ_EXPORT DvzRequest dvz_set_vertex(
    DvzBatch* batch, DvzId graphics, uint32_t binding_idx, DvzSize stride,
    DvzVertexInputRate input_rate);



/**
 * Create a request for setting a vertex attribute of a graphics pipe.
 *
 * @param batch the batch
 * @param graphics the graphics pipe id
 * @param binding_idx the index of the vertex binding
 * @param location the GLSL attribute location
 * @param format the attribute format
 * @param offset the byte offset of the attribute within the vertex binding
 * @returns the request
 */
DVZ_EXPORT DvzRequest dvz_set_attr(
    DvzBatch* batch, DvzId graphics, uint32_t binding_idx, uint32_t location, DvzFormat format,
    DvzSize offset);



/**
 * Create a request for setting a binding slot (descriptor) of a graphics pipe.
 *
 * @param batch the batch
 * @param graphics the graphics pipe id
 * @param slot_idx the index of the GLSL binding slot
 * @param type the descriptor type
 * @returns the request
 */
DVZ_EXPORT DvzRequest
dvz_set_slot(DvzBatch* batch, DvzId graphics, uint32_t slot_idx, DvzDescriptorType type);



/**
 * Create a request for setting a push constant layout for a graphics pipe.
 *
 * @param batch the batch
 * @param graphics the graphics pipe id
 * @param shader_stages the shader stages with the push constant
 * @param offset the byte offset for the push data visibility from the shader
 * @param size how much bytes the shader can see from the push constant
 * @returns the request
 */
DVZ_EXPORT DvzRequest dvz_set_push(
    DvzBatch* batch, DvzId graphics, DvzShaderStageFlags shader_stages, DvzSize offset,
    DvzSize size);



/**
 * Create a request for setting a specialization constant of a graphics pipe.
 *
 * @param batch the batch
 * @param graphics the graphics pipe id
 * @param shader the shader with the specialization constant
 * @param idx the specialization constant index as specified in the GLSL code
 * @param size the byte size of the value
 * @param value a pointer to the specialization constant value
 * @returns the request
 */
DVZ_EXPORT DvzRequest dvz_set_specialization(
    DvzBatch* batch, DvzId graphics, DvzShaderType shader, uint32_t idx, //
    DvzSize size, void* value);



/**
 * Create a request for graphics deletion.
 *
 * @param batch the batch
 * @param id the graphics id
 * @returns the request
 */
DVZ_EXPORT DvzRequest dvz_delete_graphics(DvzBatch* batch, DvzId id);



/*************************************************************************************************/
/*  Bindings                                                                                     */
/*************************************************************************************************/

/**
 * Create a request for associating a vertex dat to a graphics pipe.
 *
 * @param batch the batch
 * @param graphics the id of the graphics pipe
 * @param binding_idx the vertex binding index
 * @param dat the id of the dat with the vertex data
 * @param offset the offset within the dat
 * @returns the request
 */
DVZ_EXPORT DvzRequest
dvz_bind_vertex(DvzBatch* batch, DvzId graphics, uint32_t binding_idx, DvzId dat, DvzSize offset);



/**
 * Create a request for associating an index dat to a graphics pipe.
 *
 * @param batch the batch
 * @param graphics the id of the graphics pipe
 * @param dat the id of the dat with the index data
 * @param offset the offset within the dat
 * @returns the request
 */
DVZ_EXPORT DvzRequest dvz_bind_index(DvzBatch* batch, DvzId graphics, DvzId dat, DvzSize offset);



/**
 * Create a request for associating a dat to a pipe's slot.
 *
 * @param batch the batch
 * @param pipe the id of the pipe
 * @param slot_idx the index of the descriptor slot
 * @param dat the id of the dat to bind to the pipe
 * @param offset the offset
 * @returns the request
 */
DVZ_EXPORT DvzRequest
dvz_bind_dat(DvzBatch* batch, DvzId pipe, uint32_t slot_idx, DvzId dat, DvzSize offset);



/**
 * Create a request for associating a tex to a pipe's slot.
 *
 * @param batch the batch
 * @param pipe the id of the pipe
 * @param slot_idx the index of the descriptor slot
 * @param tex the id of the tex to bind to the pipe
 * @param sampler the id of the sampler
 * @param offset the offset
 * @returns the request
 */
DVZ_EXPORT DvzRequest dvz_bind_tex(
    DvzBatch* batch, DvzId pipe, uint32_t slot_idx, DvzId tex, DvzId sampler, uvec3 offset);



/*************************************************************************************************/
/*  Command buffer                                                                               */
/*************************************************************************************************/

/**
 * Create a request for starting recording of command buffer.
 *
 * @param batch the batch
 * @param canvas_id the id of the canvas
 * @returns the request
 */
DVZ_EXPORT DvzRequest dvz_record_begin(DvzBatch* batch, DvzId canvas_id);



/**
 * Create a request for setting the viewport during command buffer recording.
 *
 * @param batch the batch
 * @param canvas_id the id of the canvas
 * @param offset the viewport offset, in framebuffer pixels
 * @param shape the viewport size, in framebuffer pixels
 * @returns the request
 */
DVZ_EXPORT DvzRequest
dvz_record_viewport(DvzBatch* batch, DvzId canvas_id, vec2 offset, vec2 shape);



/**
 * Create a request for a direct draw of a graphics during command buffer recording.
 *
 * @param batch the batch
 * @param canvas_id the id of the canvas
 * @param graphics the id of the graphics pipe to draw
 * @param first_vertex the index of the first vertex to draw
 * @param vertex_count the number of vertices to draw
 * @param first_instance the index of the first instance to draw
 * @param instance_count the number of instances to draw
 * @returns the request
 */
DVZ_EXPORT DvzRequest dvz_record_draw(
    DvzBatch* batch, DvzId canvas_id, DvzId graphics, //
    uint32_t first_vertex, uint32_t vertex_count,     //
    uint32_t first_instance, uint32_t instance_count);



/**
 * Create a request for an indexed draw of a graphics during command buffer recording.
 *
 * @param batch the batch
 * @param canvas_id the id of the canvas
 * @param graphics the id of the graphics pipe to draw
 * @param first_index the index of the first index to draw
 * @param vertex_offset the vertex offset within the vertices indexed by the indexes
 * @param index_count the number of indexes to draw
 * @param first_instance the index of the first instance to draw
 * @param instance_count the number of instances to draw
 * @returns the request
 */
DVZ_EXPORT DvzRequest dvz_record_draw_indexed(
    DvzBatch* batch, DvzId canvas_id, DvzId graphics,                   //
    uint32_t first_index, uint32_t vertex_offset, uint32_t index_count, //
    uint32_t first_instance, uint32_t instance_count);



/**
 * Create a request for an indirect draw of a graphics during command buffer recording.
 *
 * @param batch the batch
 * @param canvas_id the id of the canvas
 * @param graphics the id of the graphics pipe to draw
 * @param indirect the id of the dat containing the indirect draw data
 * @param draw_count the number of draws to make
 * @returns the request
 */
DVZ_EXPORT DvzRequest dvz_record_draw_indirect(
    DvzBatch* batch, DvzId canvas_id, DvzId graphics, DvzId indirect, uint32_t draw_count);



/**
 * Create a request for an indexed indirect draw of a graphics during command buffer recording.
 *
 * @param batch the batch
 * @param canvas_id the id of the canvas
 * @param graphics the id of the graphics pipe to draw
 * @param indirect the id of the dat containing the indirect draw data
 * @param draw_count the number of draws to make
 * @returns the request
 */
DVZ_EXPORT DvzRequest dvz_record_draw_indexed_indirect(
    DvzBatch* batch, DvzId canvas_id, DvzId graphics, DvzId indirect, uint32_t draw_count);



/**
 * Create a request for sending a push constant value while recording a command buffer.
 *
 * @param batch the batch
 * @param canvas_id the id of the canvas
 * @param graphics_id the id of the graphics pipeline
 * @param shader_stages the shader stages
 * @param offset the byte offset
 * @param size the size of the data to upload
 * @param data the push constant data to upload
 * @returns the request
 */
DVZ_EXPORT DvzRequest dvz_record_push(
    DvzBatch* batch, DvzId canvas_id, DvzId graphics_id, DvzShaderStageFlags shader_stages, //
    DvzSize offset, DvzSize size, void* data);



/**
 * Create a request for ending recording of command buffer.
 *
 * @param batch the batch
 * @param canvas_id the id of the canvas
 * @returns the request
 */
DVZ_EXPORT DvzRequest dvz_record_end(DvzBatch* batch, DvzId canvas_id);



EXTERN_C_OFF

#endif
