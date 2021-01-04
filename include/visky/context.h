#ifndef VKL_CONTEXT_HEADER
#define VKL_CONTEXT_HEADER

#include "common.h"
#include "vklite.h"

// #define STB_IMAGE_IMPLEMENTATION
BEGIN_INCL_NO_WARN
#include "../external/stb_image.h"
END_INCL_NO_WARN

#ifdef __cplusplus
extern "C" {
#endif



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define VKL_MAX_FIFO_CAPACITY 64
#define VKL_MAX_TRANSFERS     VKL_MAX_FIFO_CAPACITY

// Poll period in ms when using vkl_transfer_wait()
#define VKL_TRANSFER_POLL_PERIOD 1

#define VKL_DEFAULT_WIDTH  800
#define VKL_DEFAULT_HEIGHT 600

#define VKL_BUFFER_TYPE_STAGING_SIZE (16 * 1024 * 1024)
#define VKL_BUFFER_TYPE_VERTEX_SIZE  (16 * 1024 * 1024)
#define VKL_BUFFER_TYPE_INDEX_SIZE   (16 * 1024 * 1024)
#define VKL_BUFFER_TYPE_STORAGE_SIZE (16 * 1024 * 1024)
#define VKL_BUFFER_TYPE_UNIFORM_SIZE (4 * 1024 * 1024)



/*************************************************************************************************/
/*  Type definitions                                                                             */
/*************************************************************************************************/

typedef struct VklFifo VklFifo;

typedef struct VklTransfer VklTransfer;
typedef struct VklTransferBuffer VklTransferBuffer;
typedef struct VklTransferBufferCopy VklTransferBufferCopy;
typedef struct VklTransferTexture VklTransferTexture;
typedef struct VklTransferTextureCopy VklTransferTextureCopy;
typedef union VklTransferUnion VklTransferUnion;

typedef struct VklFontAtlas VklFontAtlas;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

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
    VKL_TRANSFER_BUFFER_UPLOAD_IMMEDIATE,
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

    atomic(bool, is_processing);
};



/*************************************************************************************************/
/*  Font atlas                                                                                   */
/*************************************************************************************************/

struct VklFontAtlas
{
    const char* name;
    uint32_t width, height;
    uint32_t cols, rows;
    uint8_t* font_texture;
    float glyph_width, glyph_height;
    const char* font_str;
    VklTexture* texture;
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
/*  Structs                                                                                      */
/*************************************************************************************************/

struct VklContext
{
    VklObject obj;
    VklGpu* gpu;

    VklTransferMode transfer_mode;
    VklCommands transfer_cmd;

    VklFifo fifo; // transfer queue

    VklContainer buffers;
    VklContainer images;
    VklContainer samplers;
    VklContainer textures;
    VklContainer computes;
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
    VklContext* context, VklBufferType buffer_idx, uint32_t buffer_count, VkDeviceSize size);

VKY_EXPORT void
vkl_ctx_buffers_resize(VklContext* context, VklBufferRegions* br, VkDeviceSize new_size);



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



/*************************************************************************************************/
/*  Font atlas                                                                                   */
/*************************************************************************************************/

static const char VKL_FONT_ATLAS_STRING[] =
    " !\"#$%&'()*+,-./"
    "0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\x7f";

static VklFontAtlas VKL_FONT_ATLAS;

static size_t _font_atlas_glyph(VklFontAtlas* atlas, const char* str, uint32_t idx)
{
    ASSERT(atlas != NULL);
    ASSERT(atlas->rows > 0);
    ASSERT(atlas->cols > 0);
    ASSERT(str != NULL);
    ASSERT(strlen(str) > 0);
    ASSERT(idx < strlen(str));
    ASSERT(atlas->font_str != NULL);
    ASSERT(strlen(atlas->font_str) > 0);

    char c[2] = {str[idx], 0};
    return strcspn(atlas->font_str, c);
}

static void _font_atlas_glyph_size(VklFontAtlas* atlas, float size, vec2 glyph_size)
{
    ASSERT(atlas != NULL);
    glyph_size[0] = size * atlas->glyph_width / atlas->glyph_height;
    glyph_size[1] = size;
}

static VklTexture* _font_texture(VklContext* ctx, VklFontAtlas* atlas)
{
    ASSERT(ctx != NULL);
    ASSERT(atlas != NULL);
    ASSERT(atlas->font_texture != NULL);

    uvec3 shape = {(uint32_t)atlas->width, (uint32_t)atlas->height, 1};
    VklTexture* texture = vkl_ctx_texture(ctx, 2, shape, VK_FORMAT_R8G8B8A8_UNORM);
    // NOTE: the font texture must have LINEAR filter! otherwise no antialiasing
    vkl_texture_filter(texture, VKL_FILTER_MAX, VK_FILTER_LINEAR);
    vkl_texture_filter(texture, VKL_FILTER_MIN, VK_FILTER_LINEAR);
    vkl_upload_texture(
        ctx, texture, (uint32_t)(atlas->width * atlas->height * 4), atlas->font_texture);
    return texture;
}

static VklFontAtlas* vkl_font_atlas(VklContext* ctx)
{
    if (VKL_FONT_ATLAS.font_texture != NULL)
        return &VKL_FONT_ATLAS;

    // Font texture
    char path[1024];
    snprintf(path, sizeof(path), "%s/textures/%s", DATA_DIR, "font_inconsolata.png");
    ASSERT(path != NULL);

    int width, height, depth;

    VklFontAtlas atlas = {0};
    atlas.font_texture = stbi_load(path, &width, &height, &depth, STBI_rgb_alpha);
    ASSERT(width > 0);
    ASSERT(height > 0);
    ASSERT(depth > 0);

    // TODO: parameters
    atlas.font_str = VKL_FONT_ATLAS_STRING;
    ASSERT(strlen(atlas.font_str) > 0);
    atlas.cols = 16;
    atlas.rows = 6;

    atlas.width = (uint32_t)width;
    atlas.height = (uint32_t)height;
    atlas.glyph_width = atlas.width / (float)atlas.cols;
    atlas.glyph_height = atlas.height / (float)atlas.rows;

    atlas.texture = _font_texture(ctx, &atlas);

    VKL_FONT_ATLAS = atlas;
    return &VKL_FONT_ATLAS;
}

static void vkl_font_atlas_destroy(VklFontAtlas* atlas)
{
    ASSERT(atlas != NULL);
    ASSERT(atlas->font_texture != NULL);
    stbi_image_free(atlas->font_texture);
}



#ifdef __cplusplus
}
#endif

#endif
