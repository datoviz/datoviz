#ifndef VKL_CONTEXT_HEADER
#define VKL_CONTEXT_HEADER

#include <pthread.h>

#include "vklite2.h"



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static inline void vkl_sleep(int milliseconds)
{
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
typedef struct VklTexture VklTexture;
typedef struct VklTransfer VklTransfer;
typedef struct VklTransferFifo VklTransferFifo;
typedef struct VklTransferBuffer VklTransferBuffer;
typedef struct VklTransferTexture VklTransferTexture;
typedef union VklTransferUnion VklTransferUnion;



/*************************************************************************************************/
/*  Enums                                                                                      */
/*************************************************************************************************/

typedef enum
{
    VKL_DEFAULT_BUFFER_STAGING,
    VKL_DEFAULT_BUFFER_VERTEX,
    VKL_DEFAULT_BUFFER_INDEX,
    VKL_DEFAULT_BUFFER_STORAGE,
    VKL_DEFAULT_BUFFER_UNIFORM,
    VKL_DEFAULT_BUFFER_COUNT,
} VklDefaultBuffer;



typedef enum
{
    VKL_DEFAULT_QUEUE_TRANSFER,
    VKL_DEFAULT_QUEUE_COMPUTE,
    VKL_DEFAULT_QUEUE_RENDER,
    VKL_DEFAULT_QUEUE_PRESENT,
    VKL_DEFAULT_QUEUE_COUNT,
} VklDefaultQueue;



typedef enum
{
    VKL_FILTER_MIN,
    VKL_FILTER_MAX,
} VklFilterType;



typedef enum
{
    VKL_TRANSFER_MODE_SYNC,
    VKL_TRANSFER_MODE_ASYNC,
} VklTransferMode;



typedef enum
{
    VKL_TRANSFER_NULL,
    VKL_TRANSFER_BUFFER_UPLOAD,
    VKL_TRANSFER_BUFFER_DOWNLOAD,
    VKL_TRANSFER_TEXTURE_UPLOAD,
    VKL_TRANSFER_TEXTURE_DOWNLOAD,
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
};



/*************************************************************************************************/
/*  Transfer structs                                                                             */
/*************************************************************************************************/

struct VklTransferBuffer
{
    VklBufferRegions regions;
    VkDeviceSize offset, size;
    void* data;
};



struct VklTransferTexture
{
    VklTexture* texture;
    uvec3 offset, shape;
    VkDeviceSize size;
    void* data;
};



union VklTransferUnion
{
    VklTransferBuffer buf;
    VklTransferTexture tex;
};



struct VklTransfer
{
    VklDataTransferType type;
    VklTransferUnion u;
};



struct VklTransferFifo
{
    VklFifo queue;
    VklTransfer transfers[VKL_MAX_TRANSFERS];
};



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct VklContext
{
    VklObject obj;
    VklGpu* gpu;

    VklTransferMode transfer_mode;
    VklCommands* transfer_cmd;
    VklTransferFifo transfer_fifo;

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



struct VklTexture
{
    VklObject obj;

    VklContext* context;

    VklImages* image;
    VklSampler* sampler;
};



/*************************************************************************************************/
/*  FIFO queue                                                                                   */
/*************************************************************************************************/

VKY_EXPORT VklFifo vkl_fifo(int32_t capacity);

VKY_EXPORT void vkl_fifo_enqueue(VklFifo* fifo, void* item);

VKY_EXPORT void* vkl_fifo_dequeue(VklFifo* fifo, bool wait);

VKY_EXPORT void vkl_fifo_reset(VklFifo* fifo);



/*************************************************************************************************/
/*  Context                                                                                      */
/*************************************************************************************************/

VKY_EXPORT VklContext* vkl_context(VklGpu* gpu);



/*************************************************************************************************/
/*  Transfer queue                                                                               */
/*************************************************************************************************/

VKY_EXPORT void vkl_transfer_mode(VklContext* context, VklTransferMode mode);

VKY_EXPORT void vkl_transfer_loop(VklContext* context);

VKY_EXPORT void vkl_transfer_end(VklContext* context);



/*************************************************************************************************/
/*  Buffer allocation                                                                            */
/*************************************************************************************************/

VKY_EXPORT VklBufferRegions vkl_alloc_buffers(
    VklContext* context, uint32_t buffer_idx, uint32_t buffer_count, VkDeviceSize size);



/*************************************************************************************************/
/*  Compute                                                                                      */
/*************************************************************************************************/

VKY_EXPORT VklCompute* vkl_new_compute(VklContext* context, const char* shader_path);



/*************************************************************************************************/
/*  Texture                                                                                      */
/*************************************************************************************************/

VKY_EXPORT VklTexture*
vkl_new_texture(VklContext* context, uint32_t dims, uvec3 size, VkFormat format);

VKY_EXPORT void vkl_texture_resize(VklTexture* texture, uvec3 size);

VKY_EXPORT void vkl_texture_filter(VklTexture* texture, VklFilterType type, VkFilter filter);

VKY_EXPORT void vkl_texture_address_mode(
    VklTexture* texture, VklTextureAxis axis, VkSamplerAddressMode address_mode);

VKY_EXPORT void vkl_texture_destroy(VklTexture* texture);



/*************************************************************************************************/
/*  Data transfers                                                                               */
/*************************************************************************************************/

VKY_EXPORT void vkl_texture_upload_region(
    VklContext* context, VklTexture* texture, uvec3 offset, uvec3 shape, VkDeviceSize size,
    void* data);

VKY_EXPORT void
vkl_texture_upload(VklContext* context, VklTexture* texture, VkDeviceSize size, void* data);

VKY_EXPORT void vkl_texture_download_region(
    VklContext* context, VklTexture* texture, uvec3 offset, uvec3 shape, VkDeviceSize size,
    void* data);

VKY_EXPORT void
vkl_texture_download(VklContext* context, VklTexture* texture, VkDeviceSize size, void* data);

VKY_EXPORT void vkl_buffer_regions_upload(
    VklContext* context, VklBufferRegions* regions, VkDeviceSize offset, VkDeviceSize size,
    void* data);

VKY_EXPORT void vkl_buffer_regions_download(
    VklContext* context, VklBufferRegions* regions, VkDeviceSize offset, VkDeviceSize size,
    void* data);



#endif
