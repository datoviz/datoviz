/*
* Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
* Licensed under the MIT license. See LICENSE file in the project root for details.
* SPDX-License-Identifier: MIT
*/

/*************************************************************************************************/
/*  File I/O utilities                                                                           */
/*************************************************************************************************/

#ifndef DVZ_HEADER_FILEIO
#define DVZ_HEADER_FILEIO



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stddef.h>
#include <stdint.h>

#include "_macros.h"
#include "datoviz_math.h"
#include "fileio.h"



/*************************************************************************************************/
/*  Generic file I/O utils                                                                       */
/*************************************************************************************************/

EXTERN_C_ON

/**
 * Read a binary file.
 *
 * @param filename path of the file to open
 * @param[out] size of the file
 * @returns pointer to a byte buffer with the file contents
 */
void* dvz_read_file(const char* filename, DvzSize* size);



/**
 * Read a NumPy NPY file.
 *
 * @param filename path of the file to open
 * @param[out] size of the file
 * @returns pointer to a buffer containing the array elements
 */
char* dvz_read_npy(const char* filename, DvzSize* size);



/**
 * Save a binary file.
 *
 * @param filename path to the PPM file to create
 * @param mode typically "wb" or "ab"
 * @param size size of the buffer
 * @param bytes buffer
 */
int dvz_write_bytes(const char* filename, const char* mode, DvzSize size, const uint8_t* bytes);



/*************************************************************************************************/
/*  Image file I/O utils                                                                         */
/*************************************************************************************************/

/**
 * Save an image to a PPM file (short ASCII header and flat binary RGB values).
 *
 * @param filename path to the PPM file to create
 * @param width width of the image
 * @param height height of the image
 * @param image pointer to an array of 24-bit RGB values
 */
int dvz_write_ppm(const char* filename, uint32_t width, uint32_t height, const uint8_t* image);



/**
 * Read a PPM image file.
 *
 * @param filename path of the file to open
 * @param[out] width width of the image
 * @param[out] height of the image
 * @returns pointer to a buffer with the loaded RGBA pixel colors
 */
uint8_t* dvz_read_ppm(const char* filename, int* width, int* height);


/**
 * Save an image to a PNG file
 *
 * @param filename path to the PNG file to create
 * @param width width of the image
 * @param height height of the image
 * @param image pointer to an array of 24-bit RGB values
 */
int dvz_write_png(const char* filename, uint32_t width, uint32_t height, const uint8_t* rgb);



/**
 * Compress an image to PNG and write it to a memory buffer.
 *
 * @param width width of the image
 * @param height height of the image
 * @param rgb pointer to an array of 24-bit RGB values
 * @param size pointer to a variable that will contain the size of the buffer
 * @param out pointer to a variable that will contain a pointer to the recorded image
 */
int dvz_make_png(uint32_t width, uint32_t height, const uint8_t* rgb, DvzSize* size, void** out);



/**
 * Load a PNG image.
 *
 * @param size pointer to a variable that will contain the size of the buffer
 * @param png_buffer pointer to an array of 24-bit RGB values
 * @param width width of the image
 * @param height height of the image
 * @returns RGB buffer
 */
uint8_t* dvz_load_png(DvzSize size, unsigned char* bytes, uint32_t* width, uint32_t* height);



/*************************************************************************************************/
/*  JPG I/O                                                                                      */
/*************************************************************************************************/

/**
 * Read a JPG buffer.
 *
 */
// uint8_t* dvz_read_jpg(
//     unsigned long size, unsigned char* jpg_bytes, uint32_t* out_width, uint32_t* out_height);



/*************************************************************************************************/
/*  Resources utils (files included in the shared dynamic library)                               */
/*************************************************************************************************/

// Defined in cmake-generated file build/_shaders.c
unsigned char* dvz_resource_shader(const char* name, unsigned long* size);



// Defined in cmake-generated file build/_textures.c
unsigned char* dvz_resource_texture(const char* name, unsigned long* size);



// Defined in cmake-generated file build/_fonts.c
unsigned char* dvz_resource_font(const char* name, unsigned long* size);



// Defined in cmake-generated file build/_testdata.c
// NOTE: only built in the CLI, not in libdatoviz.
unsigned char* dvz_resource_testdata(const char* name, unsigned long* size);



EXTERN_C_OFF

#endif
