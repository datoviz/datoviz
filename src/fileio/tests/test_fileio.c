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
    dvz_make_png((uint32_t)width, (uint32_t)height, rgb, &size, &out);
    AT(size > 0);
    AT(out != NULL);
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
    dvz_free(buffer);
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

    TST_GROUP("text");
    TST_CASE(test_read_text);

    TST_GROUP("jpeg");
    TST_CASE(test_jpeg_bytes_earth);
    TST_CASE(test_jpeg_file_earth);

    TST_GROUP("npy");
    TST_CASE(test_parse_npy);

    return 0;
}
