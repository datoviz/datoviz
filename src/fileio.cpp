/*************************************************************************************************/
/*  File I/O utilities                                                                           */
/*************************************************************************************************/

#include "fileio.h"
#include "common.h"
#include "fpng.h"

// #include <jerror.h>
// #include <jpeglib.h>



/*************************************************************************************************/
/*  Generic file I/O utils                                                                       */
/*************************************************************************************************/

void* dvz_read_file(const char* filename, DvzSize* size)
{
    /* The returned pointer must be freed by the caller. */
    void* buffer = NULL;
    DvzSize length = 0;
    FILE* f = fopen(filename, "rb");

    if (!f)
    {
        log_error("Could not find %s.", filename);
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    length = (DvzSize)ftell(f);
    if (size != NULL)
        *size = length;
    fseek(f, 0, SEEK_SET);
    buffer = (void*)malloc((size_t)length);
    fread(buffer, 1, (size_t)length, f);
    fclose(f);

    return buffer;
}



char* dvz_read_npy(const char* filename, DvzSize* size)
{
    /* Tiny NPY reader that requires the user to know in advance the data type of the file. */

    /* The returned pointer must be freed by the caller. */
    char* buffer = NULL;
    DvzSize length = 0;
    int nread = 0, err = 0;

    FILE* f = fopen(filename, "rb");
    if (!f)
    {
        log_error("the file %s does not exist", filename);
        return NULL;
    }

    // Determine the total file size.
    fseek(f, 0, SEEK_END);
    length = (DvzSize)ftell(f);
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
    buffer = (char*)calloc((size_t)length, 1);
    ANN(buffer);
    fread(buffer, 1, (size_t)length, f);
    fclose(f);

    return buffer;

error:
    log_error("unable to read the NPY file %s", filename);
    return NULL;
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
    uint8_t* image = (uint8_t*)calloc(size, sizeof(uint8_t));
    fread(image, 1, size, fp);
    fclose(fp);
    return image;
}



/*************************************************************************************************/
/*  PNG I/O                                                                                      */
/*************************************************************************************************/

int dvz_write_png(const char* filename, uint32_t width, uint32_t height, const uint8_t* rgb)
{
    ANN(filename);
    ANN(rgb);
    ASSERT(width > 0);
    ASSERT(height > 0);

    fpng::fpng_init();
    fpng::fpng_encode_image_to_file(filename, rgb, width, height, 3, 0);
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
    *out = malloc(*size);
    ANN(*out);
    memcpy(*out, outvec.data(), *size);
    return 0;
}



/*************************************************************************************************/
/*  JPG I/O                                                                                      */
/*************************************************************************************************/

// uint8_t* dvz_read_jpg(
//     unsigned long size, unsigned char* jpg_bytes, uint32_t* out_width, uint32_t* out_height)
// {
//     struct jpeg_decompress_struct cinfo;
//     struct jpeg_error_mgr jerr;
//     uint8_t* image_data = NULL;

//     // Set up error handling
//     cinfo.err = jpeg_std_error(&jerr);
//     jpeg_create_decompress(&cinfo);

//     // Specify the source of the JPEG data (in-memory buffer)
//     jpeg_mem_src(&cinfo, jpg_bytes, size);

//     // Read JPEG header and set image parameters
//     (void)jpeg_read_header(&cinfo, TRUE);

//     // Set the output color space to JCS_EXT_RGBX for 32-bit RGBA format
//     cinfo.out_color_space = JCS_EXT_RGBX;

//     jpeg_start_decompress(&cinfo);

//     *out_width = cinfo.output_width;
//     *out_height = cinfo.output_height;

//     // Allocate memory for the image data in 32-bit RGBA format
//     image_data = (uint8_t*)malloc(cinfo.output_width * cinfo.output_height * 4);

//     // Read scanlines and decode the image into 32-bit RGBA format
//     uint8_t* row_pointer[1];
//     while (cinfo.output_scanline < cinfo.output_height)
//     {
//         row_pointer[0] = &image_data[cinfo.output_scanline * cinfo.output_width * 4];
//         jpeg_read_scanlines(&cinfo, row_pointer, 1);
//     }

//     // Clean up and return the image data
//     jpeg_finish_decompress(&cinfo);
//     jpeg_destroy_decompress(&cinfo);

//     return image_data;
// }
