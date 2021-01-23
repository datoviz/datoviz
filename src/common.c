#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Optional PNG support
#if HAS_PNG
#include <png.h>
#include <zlib.h>
#endif

#include "../include/visky/common.h"

BEGIN_INCL_NO_WARN
#include <cglm/struct.h>
END_INCL_NO_WARN



/*************************************************************************************************/
/*  I/O                                                                                          */
/*************************************************************************************************/

int write_png(const char* filename, uint32_t width, uint32_t height, const uint8_t* image)
{
#if HAS_PNG
    // from https://fossies.org/linux/libpng/example.c
    FILE* fp;
    png_structp png_ptr;
    png_infop info_ptr;

    fp = fopen(filename, "wb");
    if (fp == NULL)
        return 1;

    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL)
    {
        fclose(fp);
        return 1;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL)
    {
        fclose(fp);
        png_destroy_write_struct(&png_ptr, NULL);
        return 1;
    }

    if (setjmp(png_jmpbuf(png_ptr)))
    {
        fclose(fp);
        png_destroy_write_struct(&png_ptr, &info_ptr);
        return 1;
    }

    png_init_io(png_ptr, fp);

    // NOTE: only RGB U8 format is supported currently (swizzling is supposed to have been done
    // already).
    uint8_t bytes_per_pixel = 3;
    int32_t bit_depth = 8;
    png_set_IHDR(
        png_ptr, info_ptr, width, height, bit_depth, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
    png_write_info(png_ptr, info_ptr);

    if (height > PNG_SIZE_MAX / (width * bytes_per_pixel))
        png_error(png_ptr, "Image data buffer would be too large");

    png_bytep* row_pointers = calloc(height, sizeof(png_bytep));
    if (height > PNG_UINT_32_MAX / (sizeof(png_bytep)))
        png_error(png_ptr, "Image is too tall to process in memory");

    for (uint32_t k = 0; k < height; k++)
        row_pointers[k] = (png_bytep)((int64_t)image + k * width * bytes_per_pixel);
    png_write_image(png_ptr, row_pointers);
    png_write_end(png_ptr, info_ptr);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(fp);
    FREE(row_pointers);
    return 0;
#else
    log_error("visky was not build with PNG support, please install libpng-dev");
    return 1;
#endif
}

int write_ppm(const char* filename, uint32_t width, uint32_t height, const uint8_t* image)
{
    // from https://github.com/SaschaWillems/Vulkan/blob/master/examples/screenshot/screenshot.cpp
    FILE* fp;
    fp = fopen(filename, "wb");
    if (fp == NULL)
        return 1;
    // ppm header
    char buffer[100];
    sprintf(buffer, "P6\n%d\n%d\n255\n", width, height);
    fwrite(buffer, strlen(buffer), 1, fp);
    // Write the RGB image.
    fwrite(image, width * height * 3, 1, fp);
    fclose(fp);
    return 0;
}

uint8_t* read_ppm(const char* filename, int* width, int* height)
{
    FILE* fp;
    fp = fopen(filename, "rb");
    if (fp == NULL)
    {
        return NULL;
    }

    // ppm header
    // int width, height, depth, c;
    int c;
    char buff[16];

    // read image format
    if (!fgets(buff, sizeof(buff), fp))
    {
        log_error("unable to read image form in  %s", filename);
        return NULL;
    }

    // check the image format
    if (buff[0] != 'P' || buff[1] != '6')
    {
        log_error("invalid image format (must be 'P6') in  %s", filename);
        return NULL;
    }

    // check for comments
    c = getc(fp);
    while (c == '#')
    {
        while (getc(fp) != '\n')
            ;
        c = getc(fp);
    }

    ungetc(c, fp);
    // read image size information
    if (fscanf(fp, "%d %d", width, height) != 2)
    {
        log_error("invalid image size (error loading '%s')", filename);
        return NULL;
    }

    // read rgb component
    int b;
    if (fscanf(fp, "%d", &b) != 1)
    {
        log_error("invalid rgb component (error loading '%s')", filename);
        return NULL;
    }
    ASSERT(b == 255);
    while (fgetc(fp) != '\n')
        ;

    uint32_t size = (uint32_t)(*width * *height * 3);
    ASSERT(size > 0);
    uint8_t* image = calloc(size, sizeof(uint8_t));
    fread(image, 1, size, fp);
    fclose(fp);
    return image;
}

char* read_file(const char* filename, size_t* size)
{
    /* The returned pointer must be freed by the caller. */
    char* buffer = NULL;
    size_t length = 0;
    FILE* f = fopen(filename, "rb");

    if (!f)
    {
        log_error("Could not find %s.", filename);
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    length = (size_t)ftell(f);
    if (size != NULL)
        *size = length;
    fseek(f, 0, SEEK_SET);
    buffer = malloc((size_t)length);
    fread(buffer, 1, (size_t)length, f);
    fclose(f);

    return buffer;
}

char* read_npy(const char* filename, size_t* size)
{
    /* Tiny NPY reader that requires the user to know in advance the data type of the file. */

    /* The returned pointer must be freed by the caller. */
    char* buffer = NULL;
    size_t length = 0;
    int nread = 0, err = 0;

    FILE* f = fopen(filename, "rb");
    if (!f)
    {
        log_error("the file %s does not exist", filename);
        return NULL;
    }

    // Determine the total file size.
    fseek(f, 0, SEEK_END);
    length = (size_t)ftell(f);
    fseek(f, 0, SEEK_SET);

    // const uint32_t MAGIC_SIZE = 6;
    // char magic[MAGIC_SIZE];

    // // Read NPY header.
    // nread = fread(magic, sizeof(char), MAGIC_SIZE, f);
    // if (!nread)
    //     goto error;

    // TODO: check the magic
    // TODO: check the version numbers

    // Determine the header size.
    uint16_t header_len = 0;
    err = fseek(f, 8, SEEK_SET);
    if (err)
        goto error;
    nread = fread(&header_len, sizeof(uint16_t), 1, f);
    if (!nread)
        goto error;
    log_trace("npy file header size is %d bytes", header_len);
    ASSERT(header_len > 0);
    header_len += 10; // NOTE: the header len does NOT include the 10 bytes with the magic string,
                      // the version, the header length

    // Jump to the beginning of the data buffer.
    fseek(f, 0, header_len);
    length -= header_len;
    if (size != NULL)
        *size = length;
    err = fseek(f, header_len, SEEK_SET);
    if (err)
        goto error;

    // Read the data buffer.
    buffer = calloc((uint32_t)length, 1);
    ASSERT(buffer != NULL);
    fread(buffer, 1, (size_t)length, f);
    fclose(f);

    return buffer;

error:
    log_error("unable to read the NPY file %s", filename);
    return NULL;
}



/*************************************************************************************************/
/*  Thread                                                                                       */
/*************************************************************************************************/

VklThread vkl_thread(VklThreadCallback callback, void* user_data)
{
    VklThread thread = {0};
    if (pthread_create(&thread.thread, NULL, callback, user_data) != 0)
        log_error("thread creation failed");
    if (pthread_mutex_init(&thread.lock, NULL) != 0)
        log_error("mutex creation failed");
    atomic_init(&thread.lock_idx, 0);
    obj_created(&thread.obj);
    return thread;
}



void vkl_thread_join(VklThread* thread)
{
    ASSERT(thread != NULL);
    pthread_join(thread->thread, NULL);
    pthread_mutex_destroy(&thread->lock);
    obj_destroyed(&thread->obj);
}



void vkl_thread_lock(VklThread* thread)
{
    ASSERT(thread != NULL);
    if (!is_obj_created(&thread->obj))
        return;
    // The lock idx is used to ensure that nested thread_lock() will work as expected. Only the
    // first lock is effective. Only the last unlock is effective.
    int lock_idx = atomic_load(&thread->lock_idx);
    ASSERT(lock_idx >= 0);
    if (lock_idx == 0)
    {
        log_trace("acquire lock");
        pthread_mutex_lock(&thread->lock);
    }
    atomic_store(&thread->lock_idx, lock_idx + 1);
}



void vkl_thread_unlock(VklThread* thread)
{
    ASSERT(thread != NULL);
    if (!is_obj_created(&thread->obj))
        return;
    int lock_idx = atomic_load(&thread->lock_idx);
    ASSERT(lock_idx >= 0);
    if (lock_idx == 1)
    {
        log_trace("release lock");
        pthread_mutex_unlock(&thread->lock);
    }
    atomic_store(&thread->lock_idx, lock_idx - 1);
}



/*************************************************************************************************/
/*  Random                                                                                       */
/*************************************************************************************************/

uint8_t rand_byte() { return rand() % 256; }

float rand_float() { return (float)rand() / (float)(RAND_MAX); }

float randn() { return sqrt(-2.0 * log(rand_float())) * cos(2 * M_PI * rand_float()); }
