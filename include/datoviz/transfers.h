/*************************************************************************************************/
/*  GPU data transfers                                                                           */
/*************************************************************************************************/

#ifndef DVZ_HEADER_TRANSFERS
#define DVZ_HEADER_TRANSFERS



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "vklite.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_TRANSFER_DEQ_UL   0
#define DVZ_TRANSFER_DEQ_DL   1
#define DVZ_TRANSFER_DEQ_COPY 2
#define DVZ_TRANSFER_DEQ_EV   3
#define DVZ_TRANSFER_DEQ_DUP  4

// Three deq processes: upload/download, copy, event (incl. download_done)
#define DVZ_TRANSFER_PROC_UD  0
#define DVZ_TRANSFER_PROC_CPY 1
#define DVZ_TRANSFER_PROC_EV  2
#define DVZ_TRANSFER_PROC_DUP 3

// Maximum number of pending dup transfers.
#define DVZ_DUPS_MAX 16



/*************************************************************************************************/
/*  Transfer enums                                                                               */
/*************************************************************************************************/

// Transfer type.
typedef enum
{
    DVZ_TRANSFER_NONE,

    DVZ_TRANSFER_BUFFER_UPLOAD,
    DVZ_TRANSFER_BUFFER_DOWNLOAD,
    DVZ_TRANSFER_BUFFER_COPY,

    DVZ_TRANSFER_IMAGE_COPY,
    DVZ_TRANSFER_IMAGE_BUFFER,
    DVZ_TRANSFER_BUFFER_IMAGE,

    DVZ_TRANSFER_DOWNLOAD_DONE, // download is only possible from a buffer, not a texture
    DVZ_TRANSFER_UPLOAD_DONE,

    DVZ_TRANSFER_DUP_UPLOAD,
    DVZ_TRANSFER_DUP_COPY,
} DvzTransferType;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

// Forward declarations.
typedef struct DvzDeq DvzDeq;



/*************************************************************************************************/
/*  Transfer typedefs                                                                            */
/*************************************************************************************************/

typedef struct DvzTransferBuffer DvzTransferBuffer;
typedef struct DvzTransferBufferCopy DvzTransferBufferCopy;
typedef struct DvzTransferBufferImage DvzTransferBufferImage;
typedef struct DvzTransferImageCopy DvzTransferImageCopy;
typedef struct DvzTransferDownloadDone DvzTransferDownloadDone;
typedef struct DvzTransferUploadDone DvzTransferUploadDone;
typedef struct DvzTransferDup DvzTransferDup;
typedef struct DvzTransfers DvzTransfers;
typedef struct DvzTransferDupItem DvzTransferDupItem;
typedef struct DvzTransferDups DvzTransferDups;



/*************************************************************************************************/
/*  Transfer structs                                                                             */
/*************************************************************************************************/

struct DvzTransferBuffer
{
    DvzBufferRegions br;
    DvzSize offset;
    DvzSize size;
    void* data;
};



struct DvzTransferBufferCopy
{
    DvzBufferRegions src, dst;
    DvzSize src_offset, dst_offset, size;
};



struct DvzTransferImageCopy
{
    DvzImages *src, *dst;
    uvec3 src_offset, dst_offset, shape;
    DvzSize size;
};



struct DvzTransferBufferImage
{
    DvzImages* img;
    uvec3 img_offset, shape;
    DvzBufferRegions br;
    DvzSize buf_offset;
    DvzSize size;
};



struct DvzTransferDownloadDone
{
    DvzSize size;
    void* data;
};



struct DvzTransferUploadDone
{
    void* user_data;
};



struct DvzTransferDup
{
    DvzTransferType type;
    DvzBufferRegions br;
    DvzSize offset, size;
    bool recurrent;

    // For upload dup only:
    void* data;

    // For copy dup only:
    DvzBufferRegions stg;
    DvzSize stg_offset;
};



/*************************************************************************************************/
/*  Transfer dups                                                                                */
/*************************************************************************************************/

struct DvzTransferDupItem
{
    bool is_set;
    DvzTransferDup tr; // If there is a staging buffer, the Transfers know it will need to
                       // copy the data from it
    bool done[DVZ_MAX_BUFFER_REGIONS_PER_SET];
};



struct DvzTransferDups
{
    uint32_t count;
    DvzTransferDupItem dups[DVZ_DUPS_MAX];
};



/*************************************************************************************************/
/*  Transfers struct                                                                             */
/*************************************************************************************************/

struct DvzTransfers
{
    DvzObject obj;
    DvzGpu* gpu;

    DvzDeq* deq;      // transfer dequeues
    DvzThread thread; // transfer thread

    DvzTransferDups dups;
};



/*************************************************************************************************/
/*  Transfers                                                                                    */
/*************************************************************************************************/

/**
 * Create a transfers object.
 *
 * This object is responsible for the data transfers (buffers and images). The context uses it in
 * the dat/tex data transfer functions. The idea is to only provide simple upload/download
 * functions, with a wait boolean parameter, and let the Transfers and the Context handle the
 * implementation.
 *
 * There are several ways of transferring data, particularly on GPU buffers. If a GPU buffer is
 * mappable (host-visible and host-coherent), one can directly transfer the data. Otherwise, a
 * staging buffer must be used. In this case, we may want this staging buffer to be persistent
 * (useful if there will be many tranfers, but this uses more GPU memory) or recreated at every
 * transfer. Also, there can be just one copy of the data, or N copies, where N is the number of
 * swapchain images. Using multiple copies is useful when transferring data continuously, to avoid
 * hard GPU synchronization (if there is a single copy of the data on the GPU, we cannot upload to
 * it while the GPU uses it to render the image). These are called "dup transfers" (for
 * "duplicates"). All of this must be properly synchronized within the event loop.
 *
 * The way it works is that the user just can call the transfer functions with the context from any
 * thread, at any point. That enqueues transfer tasks in a queue. The event loop just has to call
 * `dvz_transfers_frame()` at every frame, with the current swapchain image index. This function
 * takes care of processing the pending transfers.
 *
 * @param transfers the DvzTransfers pointer
 */
DVZ_EXPORT void dvz_transfers(DvzGpu* gpu, DvzTransfers* transfers);



/**
 * Process the pending transfers within the event loop.
 *
 * Copying to staging buffers is done in a background thread, because these are not used for
 * rendering. However, once the CPU->GPU transfer is done, we need to copy the contents of the
 * staging buffers to the actual device-only buffers that are used for rendering. And we need
 * proper synchronization for that. This is where `dvz_transfers_frame()` comes to the rescue: it
 * processes the transfers and, since it knows which swapchain image is about to be rendered, it
 * can safely access to the corresponding GPU buffer regions.
 *
 * @param transfers the DvzTransfers pointer
 */
DVZ_EXPORT void dvz_transfers_frame(DvzTransfers* transfers, uint32_t img_idx);



/**
 * Destroy a transfers object.
 *
 * @param transfers the DvzTransfers pointer
 */
DVZ_EXPORT void dvz_transfers_destroy(DvzTransfers* transfers);



/*************************************************************************************************/
/*  Convenient but slow transfer functions, essentially used in testing or offscreen settings    */
/*************************************************************************************************/

// WARNING: do not use the functions below except for offscreen/testing purposes.

/**
 * Synchronously upload data from the CPU to GPU buffer regions.
 *
 * !!! warning
 *     Slow and inefficient, only for debugging/testing/offscreen purposes.
 *
 * @param transfers the DvzTransfers pointer
 * @param br the buffer regions to update
 * @param offset the offset within the buffer regions, in bytes
 * @param size the size of the data to upload, in bytes
 * @param data pointer to the data to upload to the GPU
 */
DVZ_EXPORT void dvz_upload_buffer(
    DvzTransfers* transfers, DvzBufferRegions br, //
    DvzSize offset, DvzSize size, void* data);



/**
 * Synchronously download data from a buffer region to the CPU.
 *
 * !!! warning
 *     Slow and inefficient, only for debugging/testing/offscreen purposes.
 *
 * @param transfers the DvzTransfers pointer
 * @param br the buffer regions to update
 * @param offset the offset within the buffer regions, in bytes
 * @param size the size of the data to upload, in bytes
 * @param[out] data pointer to a buffer already allocated to contain `size` bytes
 */
DVZ_EXPORT void dvz_download_buffer(
    DvzTransfers* transfers, DvzBufferRegions br, //
    DvzSize offset, DvzSize size, void* data);



/**
 * Synchronously copy data between two GPU buffer regions.
 *
 * !!! warning
 *     Slow and inefficient, only for debugging/testing/offscreen purposes.
 *
 * @param transfers the DvzTransfers pointer
 * @param src the buffer region to copy from
 * @param src_offset the offset within the source buffer region
 * @param dst the buffer region to copy to
 * @param dst_offset the offset within the target buffer region
 * @param size the size of the data to copy
 */
DVZ_EXPORT void dvz_copy_buffer(
    DvzTransfers* transfers, DvzBufferRegions src, DvzSize src_offset, //
    DvzBufferRegions dst, DvzSize dst_offset, DvzSize size);



/**
 * Synchronously upload data to a image.
 *
 * !!! warning
 *     Slow and inefficient, only for debugging/testing/offscreen purposes.
 *
 * @param transfers the DvzTransfers pointer
 * @param img the image to update
 * @param offset the offset within the image
 * @param shape the shape of the region to update within the image
 * @param size the size of the uploaded data, in bytes
 * @param data pointer to the data to upload to the GPU
 */
DVZ_EXPORT void dvz_upload_image(
    DvzTransfers* transfers, DvzImages* img, //
    uvec3 offset, uvec3 shape, DvzSize size, void* data);



/**
 * Synchronously download data from a image.
 *
 * !!! warning
 *     Slow and inefficient, only for debugging/testing/offscreen purposes.
 *
 * @param transfers the DvzTransfers pointer
 * @param img the image to download from
 * @param offset the offset within the image
 * @param shape the shape of the region to update within the image
 * @param size the size of the downloaded data, in bytes
 * @param[out] data pointer to the buffer that will hold the downloaded data
 */
DVZ_EXPORT void dvz_download_image(
    DvzTransfers* transfers, DvzImages* img, //
    uvec3 offset, uvec3 shape, DvzSize size, void* data);



/**
 * Synchronously copy part of a image to another.
 *
 * !!! warning
 *     Slow and inefficient, only for debugging/testing/offscreen purposes.
 *
 * @param transfers the DvzTransfers pointer
 * @param src the source image
 * @param src_offset the offset within the source image
 * @param dst the target image
 * @param dst_offset the offset within the target image
 * @param shape the shape of the part of the image to copy
 * @param size the corresponding size of that part, in bytes
 */
DVZ_EXPORT void dvz_copy_image(
    DvzTransfers* transfers,          //
    DvzImages* src, uvec3 src_offset, //
    DvzImages* dst, uvec3 dst_offset, //
    uvec3 shape, DvzSize size);



#endif
