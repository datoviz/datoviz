/*************************************************************************************************/
/*  File I/O utilities                                                                           */
/*************************************************************************************************/

#include "fileio.h"
#include "common.h"

// Optional PNG support
#if HAS_PNG
#include <png.h>
#include <zlib.h>
#endif


/*************************************************************************************************/
/*  Generic file I/O utils                                                                       */
/*************************************************************************************************/

uint32_t* dvz_read_file(const char* filename, size_t* size)
{
    /* The returned pointer must be freed by the caller. */
    uint32_t* buffer = NULL;
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



char* dvz_read_npy(const char* filename, size_t* size)
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
/*  Image file I/O utils                                                                         */
/*************************************************************************************************/


int dvz_write_ppm(const char* filename, uint32_t width, uint32_t height, const uint8_t* image)
{
    // from https://github.com/SaschaWillems/Vulkan/blob/master/examples/screenshot/screenshot.cpp
    FILE* fp;
    fp = fopen(filename, "wb");
    if (fp == NULL)
        return 1;
    // ppm header
    char buffer[256];
    snprintf(buffer, 256, "P6\n%d\n%d\n255\n", (int)width, (int)height);
    fwrite(buffer, strlen(buffer), 1, fp);
    // Write the RGB image.
    fwrite(image, width * height * 3, 1, fp);
    fclose(fp);
    return 0;
}



uint8_t* dvz_read_ppm(const char* filename, int* width, int* height)
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
        fclose(fp);
        return NULL;
    }

    // check the image format
    if (buff[0] != 'P' || buff[1] != '6')
    {
        log_error("invalid image format (must be 'P6') in  %s", filename);
        fclose(fp);
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



int dvz_write_png(const char* filename, uint32_t width, uint32_t height, const uint8_t* rgb)
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
        row_pointers[k] = (png_bytep)((int64_t)rgb + k * width * bytes_per_pixel);
    png_write_image(png_ptr, row_pointers);
    png_write_end(png_ptr, info_ptr);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(fp);
    FREE(row_pointers);
    return 0;
#else
    log_error("datoviz was not build with PNG support, please install libpng-dev");
    return 1;
#endif
}



// from https://stackoverflow.com/a/1823604/1595060
/* structure to store PNG image bytes */
struct mem_encode
{
    char* buffer;
    size_t size;
    size_t buf_size;
};

static void my_png_write_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
    /* with libpng15 next line causes pointer deference error; use libpng12 */
    struct mem_encode* p = (struct mem_encode*)png_get_io_ptr(png_ptr); /* was png_ptr->io_ptr */
    size_t nsize = p->size + length;

    /* allocate or grow buffer */
    if (p->buffer == NULL)
    {
        p->buffer = malloc(nsize);
        p->buf_size = nsize;
    }
    else if (p->buf_size < nsize)
    {
        while (p->buf_size < nsize)
            p->buf_size *= 2;
        log_trace("while writing PNG, reallocating buffer to %d bytes", p->buf_size);
        p->buffer = realloc(p->buffer, p->buf_size);
    }

    if (!p->buffer)
        png_error(png_ptr, "Write Error");

    /* copy new bytes to end of buffer */
    memcpy(p->buffer + p->size, data, length);
    p->size += length;
}

/* This is optional but included to show how png_set_write_fn() is called */
static void my_png_flush(png_structp png_ptr) {}

int dvz_make_png(uint32_t width, uint32_t height, const uint8_t* rgb, DvzSize* size, void** out)
{
    // #if HAS_PNG
    // from https://fossies.org/linux/libpng/example.c
    // FILE* fp;
    png_structp png_ptr;
    png_infop info_ptr;

    /* static */
    struct mem_encode state;

    /* initialise - put this before png_write_png() call */
    state.buffer = NULL;
    state.size = 0;

    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL)
    {
        // fclose(fp);
        return 1;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL)
    {
        // fclose(fp);
        png_destroy_write_struct(&png_ptr, NULL);
        return 1;
    }

    if (setjmp(png_jmpbuf(png_ptr)))
    {
        // fclose(fp);
        png_destroy_write_struct(&png_ptr, &info_ptr);
        return 1;
    }

    // png_init_io(png_ptr, fp);
    // TODO: smaller buffer, increase dynamically. Here we just assume (hope) that the PNG will be
    // smaller than the full size...
    // *out = calloc(width * height, 3);

    /* if my_png_flush() is not needed, change the arg to NULL */
    png_set_write_fn(png_ptr, &state, my_png_write_data, my_png_flush);

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
        row_pointers[k] = (png_bytep)((int64_t)rgb + k * width * bytes_per_pixel);
    png_write_image(png_ptr, row_pointers);
    png_write_end(png_ptr, info_ptr);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    // fclose(fp);

    // /* cleanup */
    // if (state.buffer)
    //     free(state.buffer);
    *out = state.buffer;
    *size = state.size;

    FREE(row_pointers);
    return 0;
    // #else
    //     log_error("datoviz was not build with PNG support, please install libpng-dev");
    //     return 1;
    // #endif
}
