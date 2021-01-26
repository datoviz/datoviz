/*************************************************************************************************/
/*  GPU data transfers interfacing closely with the canvas event loop                            */
/*************************************************************************************************/

#ifndef VKL_TRANSFERS_HEADER
#define VKL_TRANSFERS_HEADER

// #include "../include/visky/context.h"
#include "../include/visky/vklite.h"



/*************************************************************************************************/
/*  Transfer enums                                                                               */
/*************************************************************************************************/

// Transfer type.
typedef enum
{
    VKL_TRANSFER_NONE,
    VKL_TRANSFER_BUFFER_UPLOAD,
    VKL_TRANSFER_BUFFER_DOWNLOAD,
    VKL_TRANSFER_BUFFER_COPY,
    VKL_TRANSFER_TEXTURE_UPLOAD,
    VKL_TRANSFER_TEXTURE_DOWNLOAD,
    VKL_TRANSFER_TEXTURE_COPY,
} VklDataTransferType;



/*************************************************************************************************/
/*  Transfer typedefs                                                                            */
/*************************************************************************************************/

typedef struct VklTransfer VklTransfer;
typedef struct VklTransferBuffer VklTransferBuffer;
typedef struct VklTransferBufferCopy VklTransferBufferCopy;
typedef struct VklTransferTexture VklTransferTexture;
typedef struct VklTransferTextureCopy VklTransferTextureCopy;
typedef union VklTransferUnion VklTransferUnion;



/*************************************************************************************************/
/*  Transfer structs                                                                             */
/*************************************************************************************************/

struct VklTransferBuffer
{
    VklBufferRegions regions;
    VkDeviceSize offset, size;
    bool update_all_buffers;
    void* data;
};



struct VklTransferBufferCopy
{
    VklBufferRegions src, dst;
    VkDeviceSize src_offset, dst_offset, size;
};



struct VklTransferTexture
{
    VklTexture* texture;
    uvec3 offset, shape;
    VkDeviceSize size;
    void* data;
};



struct VklTransferTextureCopy
{
    VklTexture *src, *dst;
    uvec3 src_offset, dst_offset, shape;
};



union VklTransferUnion
{
    VklTransferBuffer buf;
    VklTransferTexture tex;
    VklTransferBufferCopy buf_copy;
    VklTransferTextureCopy tex_copy;
};



struct VklTransfer
{
    VklDataTransferType type;
    VklTransferUnion u;
};



/*************************************************************************************************/
/*  Transfers                                                                                    */
/*************************************************************************************************/

/**
 * Upload data to 1 or N buffer regions on the GPU while the app event loop is running.
 *
 * @param canvas the canvas
 * @param br the buffer regions to update
 * @param offset the offset within the buffer regions, in bytes
 * @param size the size of the data to upload, in bytes
 * @param data pointer to the data to upload to the GPU
 */
VKY_EXPORT void vkl_upload_buffers(
    VklCanvas* canvas, VklBufferRegions br, VkDeviceSize offset, VkDeviceSize size, void* data);

/**
 * Download data from a buffer region to the CPU while the app event loop is running.
 *
 * @param canvas the canvas
 * @param br the buffer regions to update
 * @param offset the offset within the buffer regions, in bytes
 * @param size the size of the data to upload, in bytes
 * @param[out] data pointer to a buffer already allocated to contain `size` bytes
 */
VKY_EXPORT void vkl_download_buffers(
    VklCanvas* canvas, VklBufferRegions br, VkDeviceSize offset, VkDeviceSize size, void* data);

/**
 * Copy data between two GPU buffer regions.
 *
 * This function does not involve GPU-CPU data transfers.
 *
 * @param canvas the canvas
 * @param src the buffer region to copy from
 * @param src_offset the offset within the source buffer region
 * @param dst the buffer region to copy to
 * @param dst_offset the offset within the target buffer region
 * @param size the size of the data to copy
 */
VKY_EXPORT void vkl_copy_buffers(
    VklCanvas* canvas, VklBufferRegions src, VkDeviceSize src_offset, //
    VklBufferRegions dst, VkDeviceSize dst_offset, VkDeviceSize size);

/**
 * Upload data to a texture.
 *
 * @param canvas the canvas
 * @param texture the texture to update
 * @param offset the offset within the texture
 * @param shape the shape of the region to update within the texture
 * @param size the size of the uploaded data, in bytes
 * @param data pointer to the data to upload to the GPU
 */
VKY_EXPORT void vkl_upload_texture(
    VklCanvas* canvas, VklTexture* texture, uvec3 offset, uvec3 shape, VkDeviceSize size,
    void* data);

/**
 * Download data from a texture.
 *
 * @param canvas the canvas
 * @param texture the texture to download from
 * @param offset the offset within the texture
 * @param shape the shape of the region to update within the texture
 * @param size the size of the downloaded data, in bytes
 * @param[out] data pointer to the buffer that will hold the downloaded data
 */
VKY_EXPORT void vkl_download_texture(
    VklCanvas* canvas, VklTexture* texture, uvec3 offset, uvec3 shape, VkDeviceSize size,
    void* data);

/**
 * Copy part of a texture to another.
 *
 * This function does not involve GPU-CPU data transfers.
 *
 * @param canvas the canvas
 * @param src the source texture
 * @param src_offset the offset within the source texture
 * @param dst the target texture
 * @param dst_offset the offset within the target texture
 * @param shape the shape of the part of the texture to copy
 * @param size the corresponding size of that part, in bytes
 */
VKY_EXPORT void vkl_copy_texture(
    VklCanvas* canvas, VklTexture* src, uvec3 src_offset, VklTexture* dst, uvec3 dst_offset,
    uvec3 shape, VkDeviceSize size);

/**
 * Process the pending transfers.
 *
 * When the event loop is running, all transfers are enqueued in a queue rather than executed
 * directly. The reason is that proper synchronization is required in order to avoid modifying GPU
 * objects while they are being used for rendering. The transfer processing function is called at a
 * deterministic time within the main event loop.
 *
 * @param canvas the canvas
 * @param br the buffer regions to update
 * @param offset the offset within the buffer regions, in bytes
 * @param size the size of the data to upload, in bytes
 * @param data pointer to the data to upload to the GPU
 */
VKY_EXPORT void vkl_process_transfers(VklCanvas* canvas);



#endif
