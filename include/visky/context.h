#ifndef VKL_CONTEXT_HEADER
#define VKL_CONTEXT_HEADER

#include "vklite2.h"



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static inline void vkl_sleep(int milliseconds)
{
    log_trace("sleep for %d ms", milliseconds);
#ifdef WIN32
    Sleep(milliseconds);
#else
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, NULL);
#endif
}



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define VKL_MAX_BUFFERS       16
#define VKL_MAX_COMPUTES      256
#define VKL_MAX_TEXTURES      256
#define VKL_MAX_FIFO_CAPACITY 256
#define VKL_MAX_TRANSFERS     VKL_MAX_FIFO_CAPACITY

// Poll period in ms when using vkl_transfer_wait()
#define VKL_TRANSFER_POLL_PERIOD 1

#define VKL_DEFAULT_WIDTH  800
#define VKL_DEFAULT_HEIGHT 600

#define VKL_DEFAULT_BUFFER_STAGING_SIZE (16 * 1024 * 1024)
#define VKL_DEFAULT_BUFFER_VERTEX_SIZE  (16 * 1024 * 1024)
#define VKL_DEFAULT_BUFFER_INDEX_SIZE   (16 * 1024 * 1024)
#define VKL_DEFAULT_BUFFER_STORAGE_SIZE (16 * 1024 * 1024)
#define VKL_DEFAULT_BUFFER_UNIFORM_SIZE (4 * 1024 * 1024)



/*************************************************************************************************/
/*  Type definitions                                                                             */
/*************************************************************************************************/

typedef struct VklFifo VklFifo;

typedef struct VklTransfer VklTransfer;
typedef struct VklTransferBuffer VklTransferBuffer;
typedef struct VklTransferBufferCopy VklTransferBufferCopy;
typedef struct VklTransferTexture VklTransferTexture;
typedef struct VklTransferTextureCopy VklTransferTextureCopy;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Default buffer.
typedef enum
{
    VKL_DEFAULT_BUFFER_STAGING,
    VKL_DEFAULT_BUFFER_VERTEX,
    VKL_DEFAULT_BUFFER_INDEX,
    VKL_DEFAULT_BUFFER_STORAGE,
    VKL_DEFAULT_BUFFER_UNIFORM,
    VKL_DEFAULT_BUFFER_UNIFORM_MAPPABLE,
    VKL_DEFAULT_BUFFER_COUNT,
} VklDefaultBuffer;



// Default queue.
typedef enum
{
    VKL_DEFAULT_QUEUE_TRANSFER,
    VKL_DEFAULT_QUEUE_COMPUTE,
    VKL_DEFAULT_QUEUE_RENDER,
    VKL_DEFAULT_QUEUE_PRESENT,
    VKL_DEFAULT_QUEUE_COUNT,
} VklDefaultQueue;



// Filter type.
typedef enum
{
    VKL_FILTER_MIN,
    VKL_FILTER_MAX,
} VklFilterType;



// Transfer mode.
typedef enum
{
    VKL_TRANSFER_MODE_SYNC,
    VKL_TRANSFER_MODE_ASYNC,
} VklTransferMode;



// Transfer type.
typedef enum
{
    VKL_TRANSFER_NONE,
    VKL_TRANSFER_BUFFER_UPLOAD,
    VKL_TRANSFER_BUFFER_UPLOAD_FAST,
    VKL_TRANSFER_BUFFER_DOWNLOAD,
    VKL_TRANSFER_BUFFER_COPY,
    VKL_TRANSFER_TEXTURE_UPLOAD,
    VKL_TRANSFER_TEXTURE_DOWNLOAD,
    VKL_TRANSFER_TEXTURE_COPY,
} VklDataTransferType;



/*************************************************************************************************/
/*  FIFO queue                                                                                   */
/*************************************************************************************************/

struct VklFifo
{
    int32_t head, tail;
    int32_t capacity;
    void* items[VKL_MAX_FIFO_CAPACITY];
    void* user_data;

    pthread_mutex_t lock;
    pthread_cond_t cond;

    _Atomic bool is_processing;
};



/*************************************************************************************************/
/*  Transfer structs                                                                             */
/*************************************************************************************************/

struct VklTransferBuffer
{
    VklBufferRegions regions;
    VkDeviceSize offset, size;
    uint32_t update_count;
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



struct VklTransfer
{
    VklDataTransferType type;
    union
    {
        VklTransferBuffer buf;
        VklTransferTexture tex;
        VklTransferBufferCopy buf_copy;
        VklTransferTextureCopy tex_copy;
    } u;
};



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct VklContext
{
    VklObject obj;
    VklGpu* gpu;

    VklTransferMode transfer_mode;
    VklCommands transfer_cmd;

    VklFifo fifo; // transfer queue
    VklTransfer transfers[VKL_MAX_TRANSFERS];

    uint32_t max_buffers;
    VklBuffer* buffers;
    VkDeviceSize* allocated_sizes; // for each buffer, how much is already allocated

    uint32_t max_images;
    VklImages* images;

    uint32_t max_samplers;
    VklSampler* samplers;

    uint32_t max_textures;
    VklTexture* textures;

    uint32_t max_computes;
    VklCompute* computes;
};



/*************************************************************************************************/
/*  FIFO queue                                                                                   */
/*************************************************************************************************/

VKY_EXPORT VklFifo vkl_fifo(int32_t capacity);

VKY_EXPORT void vkl_fifo_enqueue(VklFifo* fifo, void* item);

VKY_EXPORT void* vkl_fifo_dequeue(VklFifo* fifo, bool wait);

VKY_EXPORT int vkl_fifo_size(VklFifo* fifo);

// Discard all but max_size items in the queue (only keep the most recent ones)
VKY_EXPORT void vkl_fifo_discard(VklFifo* fifo, int max_size);

VKY_EXPORT void vkl_fifo_reset(VklFifo* fifo);

VKY_EXPORT void vkl_fifo_destroy(VklFifo* fifo);



/*************************************************************************************************/
/*  Context                                                                                      */
/*************************************************************************************************/

VKY_EXPORT VklContext* vkl_context(VklGpu* gpu, VklWindow* window);

VKY_EXPORT void vkl_context_reset(VklContext* context);



/*************************************************************************************************/
/*  Transfer queue                                                                               */
/*************************************************************************************************/

VKY_EXPORT void vkl_transfer_mode(VklContext* context, VklTransferMode mode);

VKY_EXPORT void vkl_transfer_loop(VklContext* context, bool wait);

VKY_EXPORT void vkl_transfer_wait(VklContext* context, int poll_period);

VKY_EXPORT void vkl_transfer_reset(VklContext* context);

VKY_EXPORT void vkl_transfer_stop(VklContext* context);



/*************************************************************************************************/
/*  Buffer allocation                                                                            */
/*************************************************************************************************/

VKY_EXPORT VklBufferRegions vkl_ctx_buffers(
    VklContext* context, uint32_t buffer_idx, uint32_t buffer_count, VkDeviceSize size);



/*************************************************************************************************/
/*  Compute                                                                                      */
/*************************************************************************************************/

VKY_EXPORT VklCompute* vkl_ctx_compute(VklContext* context, const char* shader_path);



/*************************************************************************************************/
/*  Texture                                                                                      */
/*************************************************************************************************/

VKY_EXPORT VklTexture*
vkl_ctx_texture(VklContext* context, uint32_t dims, uvec3 size, VkFormat format);

VKY_EXPORT void vkl_texture_resize(VklTexture* texture, uvec3 size);

VKY_EXPORT void vkl_texture_filter(VklTexture* texture, VklFilterType type, VkFilter filter);

VKY_EXPORT void vkl_texture_address_mode(
    VklTexture* texture, VklTextureAxis axis, VkSamplerAddressMode address_mode);

VKY_EXPORT void vkl_texture_destroy(VklTexture* texture);



/*************************************************************************************************/
/*  Data transfers                                                                               */
/*************************************************************************************************/

VKY_EXPORT void vkl_upload_buffers(
    VklContext* context, VklBufferRegions regions, VkDeviceSize offset, VkDeviceSize size,
    void* data);

VKY_EXPORT void vkl_upload_texture_region(
    VklContext* context, VklTexture* texture, uvec3 offset, uvec3 shape, VkDeviceSize size,
    void* data);

VKY_EXPORT void
vkl_upload_texture(VklContext* context, VklTexture* texture, VkDeviceSize size, void* data);



VKY_EXPORT void vkl_download_buffers(
    VklContext* context, VklBufferRegions regions, VkDeviceSize offset, VkDeviceSize size,
    void* data);

VKY_EXPORT void vkl_download_texture_region(
    VklContext* context, VklTexture* texture, uvec3 offset, uvec3 shape, VkDeviceSize size,
    void* data);

VKY_EXPORT void
vkl_download_texture(VklContext* context, VklTexture* texture, VkDeviceSize size, void* data);



VKY_EXPORT void vkl_copy_buffers(
    VklContext* context,                           //
    VklBufferRegions src, VkDeviceSize src_offset, //
    VklBufferRegions dst, VkDeviceSize dst_offset, //
    VkDeviceSize size);

VKY_EXPORT void vkl_copy_textures(
    VklContext* context, VklTexture* src, uvec3 src_offset, //
    VklTexture* dst, uvec3 dst_offset, uvec3 shape);



#endif
