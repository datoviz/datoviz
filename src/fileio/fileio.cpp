/*************************************************************************************************/
/*  File I/O utilities                                                                           */
/*************************************************************************************************/

#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <vector>

#if !defined(_WIN32) && !defined(_MSC_VER)
#include <sys/types.h>
#endif

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "datoviz/fileio/fileio.h"
#include "fpng.h"
#include "stb_image.h"

#if DVZ_HAS_ZLIB
#include <zlib.h>
#endif



/*************************************************************************************************/
/*  Generic file I/O utils                                                                       */
/*************************************************************************************************/

/**
 * Seek within a file using a 64-bit offset.
 *
 * @param file open file
 * @param offset byte offset
 * @param origin seek origin
 * @return zero on success, nonzero on failure
 */
static int _file_seek(FILE* file, int64_t offset, int origin)
{
#if defined(_WIN32) || defined(_MSC_VER)
    return _fseeki64(file, offset, origin);
#else
    return fseeko(file, (off_t)offset, origin);
#endif
}



/**
 * Return the current 64-bit file position.
 *
 * @param file open file
 * @return current byte position, or a negative value on failure
 */
static int64_t _file_tell(FILE* file)
{
#if defined(_WIN32) || defined(_MSC_VER)
    return _ftelli64(file);
#else
    return (int64_t)ftello(file);
#endif
}



/**
 * Validate an NPY v1 prefix and return its payload offset.
 *
 * @param bytes NPY prefix containing at least ten bytes
 * @param size_bytes total NPY file size
 * @param offset destination receiving the payload offset
 * @return whether the prefix and payload offset are valid
 */
static bool _npy_payload_offset(const uint8_t* bytes, DvzSize size_bytes, DvzSize* offset)
{
    ANN(bytes);
    ANN(offset);

    if (size_bytes < 10 || memcmp(bytes, "\x93NUMPY", 6) != 0)
        return false;
    if (bytes[6] != 1 || bytes[7] != 0)
        return false;

    const uint16_t header_len = (uint16_t)bytes[8] | ((uint16_t)bytes[9] << 8);
    const DvzSize data_offset = 10 + (DvzSize)header_len;
    if (header_len == 0 || data_offset > size_bytes)
        return false;

    *offset = data_offset;
    return true;
}

DvzSize dvz_file_size(const char* filename)
{
    if (filename == NULL)
        return 0;

    FILE* file = fopen(filename, "rb"); // Open the file in binary mode
    if (file == NULL)
    {
        perror("Error opening file");
        return 0;
    }

    if (_file_seek(file, 0, SEEK_END) != 0)
    {
        fclose(file);
        return 0;
    }
    const int64_t size = _file_tell(file);
    fclose(file);

    return size >= 0 ? (DvzSize)size : 0;
}



void* dvz_read_file(const char* filename, DvzSize* size)
{
    /* The returned pointer must be freed by the caller. */
    int64_t file_size = -1;
    size_t length = 0;
    void* buffer = NULL;

    if (size != NULL)
        *size = 0;
    if (filename == NULL)
        return NULL;

    FILE* f = fopen(filename, "rb");

    if (!f)
    {
        log_error("Could not find %s.", filename);
        return NULL;
    }
    if (_file_seek(f, 0, SEEK_END) != 0)
        goto error;
    file_size = _file_tell(f);
    if (file_size < 0 || (uint64_t)file_size > SIZE_MAX)
        goto error;
    if (_file_seek(f, 0, SEEK_SET) != 0)
        goto error;

    length = (size_t)file_size;
    buffer = dvz_malloc(length > 0 ? length : 1);
    if (buffer == NULL)
        goto error;
    if (length > 0 && fread(buffer, 1, length, f) != length)
    {
        dvz_free(buffer);
        goto error;
    }
    fclose(f);

    if (size != NULL)
        *size = (DvzSize)length;
    return buffer;

error:
    log_error("unable to read file %s", filename);
    fclose(f);
    return NULL;
}



char* dvz_read_text(const char* filename, DvzSize* size)
{
    DvzSize file_size = 0;
    void* bytes = dvz_read_file(filename, &file_size);
    if (bytes == NULL)
        return NULL;

    char* text = (char*)dvz_malloc((size_t)file_size + 1);
    if (text == NULL)
    {
        dvz_free(bytes);
        return NULL;
    }
    if (file_size > 0)
        dvz_memcpy(text, (size_t)file_size + 1, bytes, (size_t)file_size);
    text[file_size] = '\0';
    dvz_free(bytes);
    if (size != NULL)
        *size = file_size;
    return text;
}



void* dvz_read_npy(const char* filename, DvzSize* size)
{
    /* Tiny NPY reader that requires the user to know in advance the data type of the file. */

    /* The returned pointer must be freed by the caller. */
    if (size != NULL)
        *size = 0;
    if (filename == NULL)
        return NULL;

    char* buffer = NULL;
    int64_t file_size = -1;
    DvzSize data_offset = 0;
    size_t length = 0;
    uint8_t prefix[10] = {0};

    FILE* f = fopen(filename, "rb");
    if (!f)
    {
        log_error("the file %s does not exist", filename);
        return NULL;
    }

    if (_file_seek(f, 0, SEEK_END) != 0)
        goto error;
    file_size = _file_tell(f);
    if (file_size < 0 || (uint64_t)file_size > SIZE_MAX)
        goto error;
    if (_file_seek(f, 0, SEEK_SET) != 0)
        goto error;

    if (fread(prefix, 1, sizeof(prefix), f) != sizeof(prefix))
        goto error;

    if (!_npy_payload_offset(prefix, (DvzSize)file_size, &data_offset))
        goto error;
    if (_file_seek(f, (int64_t)data_offset, SEEK_SET) != 0)
        goto error;

    length = (size_t)((DvzSize)file_size - data_offset);
    buffer = (char*)dvz_malloc(length > 0 ? length : 1);
    if (buffer == NULL)
        goto error;
    if (length > 0 && fread(buffer, 1, length, f) != length)
        goto error;
    fclose(f);

    if (size != NULL)
        *size = (DvzSize)length;
    return buffer;

error:
    log_error("unable to read the NPY file %s", filename);
    dvz_free(buffer);
    fclose(f);
    return NULL;
}



void* dvz_parse_npy(const void* bytes, DvzSize size_bytes)
{
    if (bytes == NULL)
        return NULL;

    const uint8_t* npy_bytes = (const uint8_t*)bytes;
    DvzSize data_offset = 0;
    if (!_npy_payload_offset(npy_bytes, size_bytes, &data_offset))
        return NULL;
    if (size_bytes - data_offset > SIZE_MAX)
        return NULL;

    const size_t array_data_size = (size_t)(size_bytes - data_offset);
    char* array_data = (char*)dvz_malloc(array_data_size > 0 ? array_data_size : 1);
    if (array_data == NULL)
        return NULL;

    if (array_data_size > 0)
        dvz_memcpy(array_data, array_data_size, npy_bytes + data_offset, array_data_size);

    return array_data;
}



char* dvz_read_gz(const char* filename, DvzSize* size)
{

#if DVZ_HAS_ZLIB
    if (!filename || !size)
    {
        dvz_fprintf(stderr, "Error: Invalid arguments.\n");
        return NULL;
    }
    ANN(filename);
    ANN(size);

    // Open the gzip file for reading
    gzFile gz_file = gzopen(filename, "rb");
    if (gz_file == NULL)
    {
        perror("Failed to open gzip file");
        return NULL;
    }

    // Allocate an initial buffer to decompress the file into memory
    size_t buffer_size = 1024 * 1024; // Start with 1 MB
    char* buffer = (char*)dvz_malloc(buffer_size);
    if (buffer == NULL)
    {
        perror("Failed to allocate memory");
        gzclose(gz_file);
        return NULL;
    }

    size_t buffer_used = 0;

    // Read and decompress the gzip file into the buffer
    while (1)
    {
        // Expand the buffer if necessary
        if (buffer_used + 4096 > buffer_size)
        {
            buffer_size *= 2;
            char* new_buffer = (char*)dvz_realloc(buffer, buffer_size);
            if (new_buffer == NULL)
            {
                perror("Failed to reallocate memory");
                dvz_free(buffer);
                gzclose(gz_file);
                return NULL;
            }
            buffer = new_buffer;
        }

        // Read data from the gzip file
        int bytes_read = gzread(gz_file, buffer + buffer_used, 4096);
        if (bytes_read < 0)
        {
            dvz_fprintf(stderr, "Decompression error: %s\n", gzerror(gz_file, NULL));
            dvz_free(buffer);
            gzclose(gz_file);
            return NULL;
        }

        if (bytes_read == 0)
        {
            // End of file reached
            break;
        }

        buffer_used += (size_t)bytes_read;
    }

    gzclose(gz_file);

    // Set the size of the decompressed data
    if (size != NULL)
    {
        *size = (DvzSize)buffer_used;
    }

    return buffer;

#else

    log_error(
        "unable to load .gz file, Datoviz was not built with zlib support. Please activate " //
        "CMake option DVZ_WITH_ZLIB");
    return NULL;

#endif
}



int dvz_write_bytes(const char* filename, const char* mode, DvzSize size, const uint8_t* bytes)
{
    FILE* fp;
    fp = fopen(filename, mode);
    if (fp == NULL)
        return 1;
    fwrite(bytes, size, 1, fp);
    fclose(fp);
    return 0;
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
    dvz_snprintf(buffer, sizeof(buffer), "P6\n%d\n%d\n255\n", (int)width, (int)height);
    fwrite(buffer, strlen(buffer), 1, fp);
    // Write the RGB image.
    fwrite(image, width * height * 3, 1, fp);
    fclose(fp);
    return 0;
}



uint8_t* dvz_read_ppm(const char* filename, uint32_t* width, uint32_t* height)
{
    if (width != NULL)
        *width = 0;
    if (height != NULL)
        *height = 0;
    if (filename == NULL || width == NULL || height == NULL)
    {
        log_error("invalid PPM file arguments");
        return NULL;
    }

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
    int parsed_width = 0;
    int parsed_height = 0;
    if (fscanf(fp, "%d %d", &parsed_width, &parsed_height) != 2)
    {
        log_error("invalid image size (error loading '%s')", filename);
        fclose(fp);
        return NULL;
    }
    if (parsed_width <= 0 || parsed_height <= 0)
    {
        log_error("invalid image dimensions (error loading '%s')", filename);
        fclose(fp);
        return NULL;
    }
    *width = (uint32_t)parsed_width;
    *height = (uint32_t)parsed_height;

    // read rgb component
    int b;
    if (fscanf(fp, "%d", &b) != 1)
    {
        log_error("invalid rgb component (error loading '%s')", filename);
        fclose(fp);
        return NULL;
    }
    ASSERT(b == 255);
    while (fgetc(fp) != '\n')
        ;

    DvzSize size = (DvzSize)(*width) * (DvzSize)(*height) * 3;
    ASSERT(size > 0);
    uint8_t* image = (uint8_t*)dvz_calloc((size_t)size, sizeof(uint8_t));
    ANN(image);
    if (fread(image, 1, (size_t)size, fp) != (size_t)size)
    {
        log_error("invalid RGB payload (error loading '%s')", filename);
        dvz_free(image);
        fclose(fp);
        return NULL;
    }
    fclose(fp);
    return image;
}



/*************************************************************************************************/
/*  PNG I/O                                                                                      */
/*************************************************************************************************/

int dvz_write_png(const char* filename, uint32_t width, uint32_t height, const uint8_t* rgba)
{
    ANN(filename);
    ANN(rgba);
    ASSERT(width > 0);
    ASSERT(height > 0);

    fpng::fpng_init();
    fpng::fpng_encode_image_to_file(filename, rgba, width, height, 4, 0);
    return 0;
}



int dvz_make_png(uint32_t width, uint32_t height, const uint8_t* rgb, DvzSize* size, void** out)
{
    ANN(rgb);
    ANN(size);
    ANN(out);
    ASSERT(width > 0);
    ASSERT(height > 0);

    fpng::fpng_init();
    std::vector<uint8_t> outvec;
    fpng::fpng_encode_image_to_memory(rgb, width, height, 3, outvec, 0);
    *size = outvec.size();
    *out = dvz_malloc(*size);
    ANN(*out);
    dvz_memcpy(*out, (size_t)(*size), outvec.data(), (size_t)(*size));
    return 0;
}



uint8_t*
dvz_load_png(const void* bytes, DvzSize size_bytes, uint32_t* width, uint32_t* height)
{
    ASSERT(size_bytes > 0);
    ANN(bytes);
    ANN(width);
    ANN(height);

    // Decode the image from memory
    std::vector<uint8_t> image_data;
    uint32_t img_width, img_height;
    uint32_t channels;
    bool success = fpng::fpng_decode_memory(
        bytes, size_bytes, image_data, img_width, img_height, channels, 3);

    if (!success)
    {
        dvz_fprintf(stderr, "Failed to decode PNG image\n");
        return NULL;
    }

    ASSERT(img_width > 0);
    ASSERT(img_height > 0);
    ASSERT(image_data.size() > 0);

    // Assign the width and height to the pointers provided
    *width = img_width;
    *height = img_height;

    // Check if the decoded image format is RGB (3 channels)
    if (channels != 3)
    {
        dvz_fprintf(stderr, "Decoded image is not in RGB format\n");
        return NULL;
    }

    // Allocate memory for the decoded image
    uint8_t* output = (uint8_t*)dvz_malloc(img_width * img_height * channels);
    if (output == NULL)
    {
        dvz_fprintf(stderr, "Failed to allocate memory for the decoded image\n");
        return NULL;
    }

    // Copy the decoded data to the allocated buffer
    dvz_memcpy(
        output, (size_t)(img_width * img_height * channels), image_data.data(),
        (size_t)(img_width * img_height * channels));

    return output;
}



/*************************************************************************************************/
/*  JPEG I/O                                                                                     */
/*************************************************************************************************/

/**
 * Decode a JPEG image from memory into tightly packed RGBA8 pixels.
 *
 * @param bytes JPEG byte buffer
 * @param size_bytes size of the JPEG byte buffer in bytes
 * @param width decoded image width
 * @param height decoded image height
 * @return RGBA8 pixel buffer allocated with the Datoviz allocator, or NULL on failure
 */
uint8_t*
dvz_load_jpeg(const void* bytes, DvzSize size_bytes, uint32_t* width, uint32_t* height)
{
    if (width != NULL)
        *width = 0;
    if (height != NULL)
        *height = 0;

    if (
        size_bytes == 0 || size_bytes > (DvzSize)INT_MAX || bytes == NULL || width == NULL ||
        height == NULL)
    {
        log_error("invalid JPEG buffer");
        return NULL;
    }

    int decoded_width = 0;
    int decoded_height = 0;
    int decoded_channels = 0;
    uint8_t* rgba = stbi_load_from_memory(
        (const stbi_uc*)bytes, (int)size_bytes, &decoded_width, &decoded_height, &decoded_channels,
        4);
    if (rgba == NULL)
    {
        log_error("unable to decode JPEG image: %s", stbi_failure_reason());
        return NULL;
    }

    if (decoded_width <= 0 || decoded_height <= 0)
    {
        log_error("decoded JPEG image has invalid dimensions");
        dvz_free(rgba);
        return NULL;
    }

    *width = (uint32_t)decoded_width;
    *height = (uint32_t)decoded_height;
    return rgba;
}



/**
 * Read and decode a JPEG image file into tightly packed RGBA8 pixels.
 *
 * @param filename path of the JPEG file to open
 * @param width decoded image width
 * @param height decoded image height
 * @return RGBA8 pixel buffer allocated with the Datoviz allocator, or NULL on failure
 */
uint8_t* dvz_read_jpeg(const char* filename, uint32_t* width, uint32_t* height)
{
    if (width != NULL)
        *width = 0;
    if (height != NULL)
        *height = 0;

    if (filename == NULL || width == NULL || height == NULL)
    {
        log_error("invalid JPEG file arguments");
        return NULL;
    }

    DvzSize size = 0;
    unsigned char* bytes = (unsigned char*)dvz_read_file(filename, &size);
    if (bytes == NULL)
        return NULL;

    uint8_t* rgba = dvz_load_jpeg(bytes, size, width, height);
    dvz_free(bytes);
    return rgba;
}
