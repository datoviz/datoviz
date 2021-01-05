#ifndef VKL_TRANSFERS_HEADER
#define VKL_TRANSFERS_HEADER

// #include "../include/visky/canvas.h"
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
    uint32_t update_count; // TODO: remove
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

VKY_EXPORT void vkl_upload_buffers(
    VklCanvas* canvas, VklBufferRegions br, VkDeviceSize offset, VkDeviceSize size, void* data);

VKY_EXPORT void vkl_download_buffers(
    VklCanvas* canvas, VklBufferRegions br, VkDeviceSize offset, VkDeviceSize size, void* data);

VKY_EXPORT void vkl_copy_buffers(
    VklCanvas* canvas, VklBufferRegions src, VkDeviceSize src_offset, //
    VklBufferRegions dst, VkDeviceSize dst_offset, VkDeviceSize size);



VKY_EXPORT void vkl_upload_texture(
    VklCanvas* canvas, VklTexture* texture, uvec3 offset, uvec3 shape, VkDeviceSize size,
    void* data);

VKY_EXPORT void vkl_download_texture(
    VklCanvas* canvas, VklTexture* texture, uvec3 offset, uvec3 shape, VkDeviceSize size,
    void* data);

VKY_EXPORT void vkl_copy_texture(
    VklCanvas* canvas, VklTexture* src, uvec3 src_offset, VklTexture* dst, uvec3 dst_offset,
    uvec3 shape, VkDeviceSize size);



VKY_EXPORT void vkl_process_transfers(VklCanvas* canvas);



#endif
