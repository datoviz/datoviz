/*************************************************************************************************/
/*  Holds all GPU data resources (buffers, images, dats, texs)                                   */
/*************************************************************************************************/

#ifndef DVZ_HEADER_RESOURCES
#define DVZ_HEADER_RESOURCES



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_enums.h"
#include "common.h"
#include "vklite.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define TRANSFERABLE (VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT)

#define DVZ_BUFFER_DEFAULT_SIZE (1 * 1024 * 1024)



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzDat DvzDat;
typedef struct DvzTex DvzTex;
typedef struct DvzResources DvzResources;

// Forward declarations.
typedef struct DvzContext DvzContext;
typedef struct DvzDatAlloc DvzDatAlloc;
typedef struct DvzTransfers DvzTransfers;



/*************************************************************************************************/
/*  Dat and Tex structs                                                                          */
/*************************************************************************************************/

struct DvzDat
{
    DvzObject obj;

    DvzContext* ctx;
    DvzResources* res;
    DvzDatAlloc* datalloc;
    DvzTransfers* transfers;

    int flags;
    DvzBufferRegions br;

    DvzDat* stg; // used for persistent staging, resized when the dat is resized
};



struct DvzTex
{
    DvzObject obj;

    DvzContext* ctx;
    DvzResources* res;

    DvzTexDims dims;
    uvec3 shape;

    int flags;
    DvzImages* img;

    DvzDat* stg; // used for persistent staging, resized when the tex is resized
};



/*************************************************************************************************/
/*  Resources struct                                                                             */
/*************************************************************************************************/

struct DvzResources
{
    DvzObject obj;
    DvzGpu* gpu;
    uint32_t img_count;

    DvzContainer buffers;
    DvzContainer images;
    DvzContainer dats;
    DvzContainer texs;
    DvzContainer samplers;
    DvzContainer computes;
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Resources                                                                                    */
/*************************************************************************************************/

/**
 * Create a resources object.
 *
 * This object is responsible for creating new GPU buffers and images.
 *
 * !!! note
 *     This is only for internal use. Users should allocate "dats" (buffer regions) and "texs"
 *     (images) instead, which abstract away the low-level implementation of these GPU objects.
 *
 * @param gpu the GPU
 * @param res the DvzResources pointer
 */
DVZ_EXPORT void dvz_resources(DvzGpu* gpu, DvzResources* res);



/**
 * Create a new GPU buffer.
 *
 * @param res the DvzResources pointer
 * @param type the buffer type
 * @param mappable whether the buffer should be mappable
 * @param mappable the buffer size
 */
DVZ_EXPORT DvzBuffer*
dvz_resources_buffer(DvzResources* res, DvzBufferType type, bool mappable, DvzSize size);



/**
 * Create a new GPU image.
 *
 * @param res the DvzResources pointer
 * @param dims the number of dimensions (1D, 2D, or 3D)
 * @param shape the width, height, and depth
 * @param format the image format
 */
DVZ_EXPORT DvzImages*
dvz_resources_image(DvzResources* res, DvzTexDims dims, uvec3 shape, DvzFormat format);



/**
 * NOTE: useless, to remove?
 *
 * Create a new GPU sampler, to be used along with an image to create a texture that can be bound
 * to a graphics pipeline.
 *
 * @param res the DvzResources pointer
 * @param filter the sampler filtering
 * @param mode the address mode (along all dimensions)
 */
DVZ_EXPORT DvzSampler*
dvz_resources_sampler(DvzResources* res, VkFilter filter, VkSamplerAddressMode mode);



/**
 * Create a new compute pipeloine.
 *
 * @param res the DvzResources pointer
 * @param shader_path the path to the compiled .spv compute shader
 */
DVZ_EXPORT DvzCompute* dvz_resources_compute(DvzResources* res, const char* shader_path);



/**
 * Destroy a resources object.
 *
 * @param res the DvzResources pointer
 */
DVZ_EXPORT void dvz_resources_destroy(DvzResources* res);



/*************************************************************************************************/
/*  Dats                                                                                         */
/*************************************************************************************************/

/**
 * Allocate a new Dat.
 *
 * A Dat represents an area of data on GPU memory. It abstracts away the underlying implementation
 * based on DvzBufferRegions and DvzBuffer. The context manages the automatic allocation of Dats.
 * When Dats are freed, the space is reusable for future allocations.
 *
 * A Dat is associated to a buffer region on a buffer of the requested buffer type. The buffer may
 * be a standalone buffer containing only the Dat, or shared with other Dats, depending on the
 * flags.
 *
 * The flags also describe how data is to be transferred to the dat. There are several options
 * depending on the expected frequency of transfers and how the data is going to be used on the
 * GPU.
 *
 * Defragmentation is not implemented yet.
 *
 * !!! important
 *     Must be called from the main thread.
 *
 * @param ctx the context object
 * @param type the buffer type
 * @param size the buffer size
 * @param flags the flags
 * @returns the newly-allocated Dat
 */
DVZ_EXPORT DvzDat* dvz_dat(DvzContext* ctx, DvzBufferType type, DvzSize size, int flags);



/**
 * Resize a dat.
 *
 * !!! note
 *     Not implemented yet: deciding whether the existing data should be kept or not upon resizing.
 *
 * !!! important
 *     Must be called from the main thread.
 *
 * @param dat the Dat
 * @param new_size the new size
 */
DVZ_EXPORT void dvz_dat_resize(DvzDat* dat, DvzSize new_size);



/**
 * Upload data to a Dat.
 *
 * This function may be asynchronous (wait=false) or synchronous (wait=true). If it is
 * asynchronous, the function `dvz_transfers_frame()` must be called at every frame in the event
 * loop.
 *
 * This function handles all types of uploads: with or without a staging buffer, normal or dup
 * transfers, etc.
 *
 * @param dat the Dat
 * @param offset the offset within the Dat
 * @param size the size of the data to upload to the Dat
 * @param wait whether this function should wait until the upload is complete or not
 */
DVZ_EXPORT void dvz_dat_upload(DvzDat* dat, DvzSize offset, DvzSize size, void* data, bool wait);



/**
 * Download data from a Dat.
 *
 * @param dat the Dat
 * @param offset the offset within the Dat
 * @param size the size of the data to download from the Dat
 * @param wait whether this function should wait until the download is complete or not
 */
DVZ_EXPORT void dvz_dat_download(DvzDat* dat, DvzSize offset, DvzSize size, void* data, bool wait);



/**
 * Destroy a dat.
 *
 * !!! important
 *     Must be called from the main thread.
 *
 * @param dat the dat
 */
DVZ_EXPORT void dvz_dat_destroy(DvzDat* dat);



/*************************************************************************************************/
/*  Texs                                                                                         */
/*************************************************************************************************/

/**
 * Create a new Tex.
 *
 * A Tex represents an image on the GPU. It abstracts away the underlying GPU implementation based
 * on DvzImages, itself based on VkImage.
 *
 * !!! important
 *     Must be called from the main thread.
 *
 * @param res the resources object
 * @param dims the number of dimensions of the image (1D, 2D, or 3D)
 * @param shape the width, height, depth
 * @param format the image format
 * @param flags the flags
 * @returns the Tex
 */
DVZ_EXPORT DvzTex*
dvz_tex(DvzContext* ctx, DvzTexDims dims, uvec3 shape, DvzFormat format, int flags);



/**
 * Resize a Tex.
 *
 * !!! important
 *     Must be called from the main thread.
 *
 * @param tex the Tex
 * @param new_shape the new width, height, depth
 * @param new_size the number of bytes corresponding to the new image shape
 */
DVZ_EXPORT void dvz_tex_resize(DvzTex* tex, uvec3 new_shape, DvzSize new_size);



/**
 * Upload data to a Tex.
 *
 * @param tex the Tex
 * @param offset the offset within the image
 * @param shape the width, height, depth of the data to upload
 * @param size the number of bytes of the data
 * @param data the data
 * @param wait whether this function should wait until the upload is complete or not
 */
DVZ_EXPORT void
dvz_tex_upload(DvzTex* tex, uvec3 offset, uvec3 shape, DvzSize size, void* data, bool wait);



/**
 * Download data from a Tex.
 *
 * @param tex the Tex
 * @param offset the offset within the image
 * @param shape the width, height, depth of the data to download
 * @param size the number of bytes of the data
 * @param data the data
 * @param wait whether this function should wait until the download is complete or not
 */
DVZ_EXPORT void
dvz_tex_download(DvzTex* tex, uvec3 offset, uvec3 shape, DvzSize size, void* data, bool wait);



/**
 * Destroy a tex.
 *
 * !!! important
 *     Must be called from the main thread.
 *
 * @param tex the tex
 */
DVZ_EXPORT void dvz_tex_destroy(DvzTex* tex);



EXTERN_C_OFF

#endif
