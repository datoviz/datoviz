/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing file I/O                                                                             */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "datoviz/fileio/fileio.h"
#include "test_fileio.h"
#include "testing.h"
#include "datoviz/math/types.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define EARTH_TEXTURE_PATH "data/assets/textures/world.200412.3x5400x2700.jpg"
#define NPY_FIXTURE_PATH "data/examples/allen_ibl/prepared/allen_ibl_mesh_color.npy"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return whether a fixture file exists.
 *
 * @param path file path
 * @return whether the file can be opened for reading
 */
static bool _file_exists(const char* path)
{
    ANN(path);

    FILE* fp = fopen(path, "rb");
    if (fp == NULL)
        return false;
    fclose(fp);
    return true;
}



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/


int test_png_1(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);

    int32_t width = 2048, height = 1024;
    uint8_t* rgb = (uint8_t*)dvz_calloc((uint32_t)(width * height), 3);
    ANN(rgb);
    for (int32_t i = 0; i < width; i++)
    {
        for (int32_t j = 0; j < height; j++)
        {
            if ((i - width / 2) * (j - height / 2) < 0)
            {
                rgb[3 * i * height + 3 * j + 0] = 128;
                rgb[3 * i * height + 3 * j + 1] = 32;
                rgb[3 * i * height + 3 * j + 2] = 16;
            }
        }
    }

    DvzSize size = 0;
    void* out = NULL;
    int rc = dvz_make_png((uint32_t)width, (uint32_t)height, rgb, &size, &out);
    AT(rc == 0);
    AT(size > 0);
    AT(out != NULL);

    uint32_t decoded_width = 0;
    uint32_t decoded_height = 0;
    uint8_t* decoded = dvz_load_png(out, size, &decoded_width, &decoded_height);
    AT(decoded != NULL);
    AT(decoded_width == (uint32_t)width);
    AT(decoded_height == (uint32_t)height);
    const size_t pixel_size = (size_t)width * (size_t)height * 3;
    const int pixel_compare = memcmp(decoded, rgb, pixel_size);
    AT(pixel_compare == 0);
    dvz_free(decoded);

    decoded_width = 42;
    decoded_height = 42;
    AT_EXPECTED_ERROR_STRICT(
        suite, (decoded = dvz_load_png(out, 8, &decoded_width, &decoded_height)) == NULL);
    AT(decoded_width == 0);
    AT(decoded_height == 0);
    dvz_free(out);

    // profiling:
    // PROF_START(100)
    // dvz_make_png((uint32_t)width, (uint32_t)height, rgb, &size, &out);
    // FREE(out);
    // PROF_END

    // test file:
    // FILE* fp = fopen("a.png", "wb");
    // fwrite(out, size, size, fp);
    // fclose(fp);

    dvz_free(rgb);
    return 0;
}



int test_ppm_io(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);

    char filename[128] = {0};
    dvz_snprintf(filename, sizeof(filename), "dvztest-fileio-%p.ppm", (void*)suite);

    const uint8_t source[] = {1, 2, 3, 4, 5, 6};
    int rc = dvz_write_ppm(filename, 2, 1, source);
    AT(rc == 0);

    uint32_t width = 0;
    uint32_t height = 0;
    uint8_t* image = dvz_read_ppm(filename, &width, &height);
    AT(image != NULL);
    AT(width == 2);
    AT(height == 1);
    const int pixel_compare = memcmp(image, source, sizeof(source));
    AT(pixel_compare == 0);
    dvz_free(image);

    const uint8_t truncated[] = "P6\n# unterminated comment";
    rc = dvz_write_bytes(filename, "wb", sizeof(truncated) - 1, truncated);
    AT(rc == 0);
    width = 42;
    height = 42;
    AT_EXPECTED_ERROR_STRICT(
        suite, (image = dvz_read_ppm(filename, &width, &height)) == NULL);
    AT(width == 0);
    AT(height == 0);

    rc = remove(filename);
    AT(rc == 0);
    return 0;
}



int test_gzip_io(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);

#if DVZ_HAS_ZLIB
    char filename[128] = {0};
    dvz_snprintf(filename, sizeof(filename), "dvztest-fileio-%p.gz", (void*)suite);

    // One gzip member that expands to 4096 'A' bytes. Concatenating 300 members exercises growth
    // beyond the initial 64 KiB allocation without carrying a large fixture.
    uint8_t member[] = {
        0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x03, 0xed, 0xc1, 0x01, 0x0d,
        0x00, 0x00, 0x00, 0xc2, 0xa0, 0x6c, 0xef, 0x5f, 0xca, 0x1e, 0x0e, 0x28, 0x00, 0x00,
        0x00, 0xe0, 0xdd, 0x00, 0x40, 0x34, 0xa6, 0xfe, 0x00, 0x10, 0x00, 0x00,
    };

    int rc = 0;
    for (uint32_t i = 0; i < 300; i++)
    {
        rc = dvz_write_bytes(filename, i == 0 ? "wb" : "ab", sizeof(member), member);
        AT(rc == 0);
    }

    DvzSize size = 0;
    char* raw = dvz_read_gz(filename, &size);
    AT(raw != NULL);
    AT(size == 300 * 4096);
    AT(raw[0] == 'A');
    AT(raw[1024 * 1024] == 'A');
    AT(raw[size - 1] == 'A');
    dvz_free(raw);

    // Corrupt the compressed payload and ensure failures reset the output size.
    member[20] ^= 0xff;
    rc = dvz_write_bytes(filename, "wb", sizeof(member), member);
    AT(rc == 0);
    size = 42;
    AT_EXPECTED_ERROR_STRICT(suite, (raw = dvz_read_gz(filename, &size)) == NULL);
    AT(size == 0);

    rc = remove(filename);
    AT(rc == 0);
#else
    tst_skip(suite, "zlib support disabled");
#endif
    return 0;
}



int test_write_bytes(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);

    const uint8_t byte = 42;
    int rc = dvz_write_bytes(NULL, "wb", 1, &byte);
    AT(rc != 0);
    rc = dvz_write_bytes("unused", NULL, 1, &byte);
    AT(rc != 0);
    rc = dvz_write_bytes("unused", "wb", 1, NULL);
    AT(rc != 0);

#if OS_LINUX
    rc = dvz_write_bytes("/dev/full", "wb", 1, &byte);
    AT(rc != 0);
#endif
    return 0;
}



int test_read_text(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);

    DvzSize size = 0;
    char* text = dvz_read_text(__FILE__, &size);
    AT(text != NULL);
    AT(size > 0);
    AT(text[size] == '\0');
    AT(strlen(text) == size);
    AT(strstr(text, "test_read_text") != NULL);
    dvz_free(text);
    return 0;
}



int test_parse_npy(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);

    const char header[] = "{'descr': '<f8', 'fortran_order': False, 'shape': (1,), }";
    const size_t header_body_len = strlen(header);
    const size_t header_padded_len = ((header_body_len + 1 + 15) / 16) * 16; // include newline
    const size_t data_size = sizeof(double);
    const size_t total_size = 10 + header_padded_len + data_size;

    uint8_t* buffer = (uint8_t*)dvz_calloc(total_size, 1);
    ANN(buffer);

    dvz_memcpy(buffer, total_size, "\x93NUMPY", 6);
    buffer[6] = 1;
    buffer[7] = 0;
    uint16_t header_len = (uint16_t)header_padded_len;
    dvz_memcpy(buffer + 8, sizeof(header_len), &header_len, sizeof(header_len));

    dvz_memset(buffer + 10, total_size - 10, ' ', header_padded_len);
    dvz_memcpy(buffer + 10, total_size - 10, header, header_body_len);
    buffer[10 + header_padded_len - 1] = '\n';

    double value = 42.0;
    dvz_memcpy(buffer + 10 + header_padded_len, data_size, &value, data_size);

    void* parsed = dvz_parse_npy(buffer, (DvzSize)total_size);
    AT(parsed != NULL);

    double parsed_value = 0.0;
    dvz_memcpy(&parsed_value, sizeof(parsed_value), parsed, data_size);
    AC(parsed_value, value, EPS);

    dvz_free(parsed);

    // Reject a truncated header before subtracting its declared size from the input size.
    parsed = dvz_parse_npy(buffer, (DvzSize)(10 + header_padded_len - 1));
    AT(parsed == NULL);

    // This minimal parser intentionally accepts only the documented NPY v1 format.
    buffer[6] = 2;
    parsed = dvz_parse_npy(buffer, (DvzSize)total_size);
    AT(parsed == NULL);
    buffer[6] = 1;

    buffer[0] = 0;
    parsed = dvz_parse_npy(buffer, (DvzSize)total_size);
    AT(parsed == NULL);
    buffer[0] = 0x93;

    // File-based failures must reset the size output and close the opened file.
    DvzSize invalid_size = 42;
    AT_EXPECTED_ERROR_STRICT(
        suite, (parsed = dvz_read_npy(__FILE__, &invalid_size)) == NULL);
    AT(invalid_size == 0);

    dvz_free(buffer);
    return 0;
}



int test_read_npy(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);

    if (!_file_exists(NPY_FIXTURE_PATH))
    {
        tst_skip(suite, "NPY fixture missing");
        return 0;
    }

    DvzSize size = 0;
    void* payload = dvz_read_npy(NPY_FIXTURE_PATH, &size);
    AT(payload != NULL);
    AT(size == 420196);
    dvz_free(payload);
    return 0;
}



int test_jpeg_bytes_earth(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);

    if (!_file_exists(EARTH_TEXTURE_PATH))
    {
        tst_skip(suite, "earth JPEG fixture missing");
        return 0;
    }

    DvzSize size = 0;
    unsigned char* bytes = (unsigned char*)dvz_read_file(EARTH_TEXTURE_PATH, &size);
    AT(bytes != NULL);
    AT(size > 0);

    uint32_t width = 0;
    uint32_t height = 0;
    uint8_t* rgba = dvz_load_jpeg(bytes, size, &width, &height);
    AT(rgba != NULL);
    AT(width == 5400);
    AT(height == 2700);
    AT(rgba[3] == 255);
    AT(rgba[4 * ((size_t)width * (height / 2u) + (width / 2u)) + 3] == 255);

    dvz_free(rgba);
    dvz_free(bytes);
    return 0;
}



int test_jpeg_file_earth(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);

    if (!_file_exists(EARTH_TEXTURE_PATH))
    {
        tst_skip(suite, "earth JPEG fixture missing");
        return 0;
    }

    uint32_t width = 0;
    uint32_t height = 0;
    uint8_t* rgba = dvz_read_jpeg(EARTH_TEXTURE_PATH, &width, &height);
    AT(rgba != NULL);
    AT(width == 5400);
    AT(height == 2700);
    AT(rgba[3] == 255);

    dvz_free(rgba);
    return 0;
}



/*************************************************************************************************/
/*  Entry-point                                                                                  */
/*************************************************************************************************/

int test_fileio(TstSuite* suite)
{
    ANN(suite);

    const char* tags = "fileio";

    TST_MODULE(suite, "fileio");
    TST_GROUP("png");
    TST_CASE(test_png_1);

    TST_GROUP("ppm");
    TST_CASE(test_ppm_io);

    TST_GROUP("text");
    TST_CASE(test_read_text);

    TST_GROUP("bytes");
    TST_CASE(test_write_bytes);

    TST_GROUP("gzip");
    TST_CASE(test_gzip_io);

    TST_GROUP("jpeg");
    TST_CASE(test_jpeg_bytes_earth);
    TST_CASE(test_jpeg_file_earth);

    TST_GROUP("npy");
    TST_CASE(test_parse_npy);
    TST_CASE(test_read_npy);

    return 0;
}
