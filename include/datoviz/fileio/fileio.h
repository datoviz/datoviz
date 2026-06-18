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

#include "datoviz/common/macros.h"
#include "datoviz/math/types.h"



/*************************************************************************************************/
/*  Generic file I/O utils                                                                       */
/*************************************************************************************************/

EXTERN_C_ON



/**
 * Return the size of a file.
 *
 * @param filename path of the file
 * @returns the size of the file
 */
DVZ_EXPORT DvzSize dvz_file_size(const char* filename);



/**
 * Read a binary file.
 *
 * @param filename path of the file to open
 * @param[out] size of the file
 * @returns owned byte buffer allocated with the Datoviz allocator, or NULL on failure; free with
 * dvz_free()
 */
DVZ_EXPORT void* dvz_read_file(const char* filename, DvzSize* size);



/**
 * Read a NumPy NPY file.
 *
 * @param filename path of the file to open
 * @param[out] size of the file
 * @returns owned buffer containing the array elements, or NULL on failure; free with dvz_free()
 */
DVZ_EXPORT void* dvz_read_npy(const char* filename, DvzSize* size);



/**
 * Read a NumPy NPY file from memory.
 *
 * @param size of the file
 * @param npy_bytes the contents of the NPY file
 * @returns owned buffer containing the array elements, or NULL on failure; free with dvz_free()
 */
DVZ_EXPORT void* dvz_parse_npy(DvzSize size, char* npy_bytes);



/**
 * Read a compressed GZIP file.
 *
 * @param filename path of the GZIP compressed file to open
 * @param[out] size of the decompressed buffer
 * @returns owned decompressed buffer, or NULL on failure; free with dvz_free()
 */
DVZ_EXPORT char* dvz_read_gz(const char* filename, DvzSize* size);



/**
 * Save a binary file.
 *
 * @param filename path to the PPM file to create
 * @param mode typically "wb" or "ab"
 * @param size size of the buffer
 * @param bytes buffer
 */
DVZ_EXPORT int
dvz_write_bytes(const char* filename, const char* mode, DvzSize size, const uint8_t* bytes);



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
DVZ_EXPORT int
dvz_write_ppm(const char* filename, uint32_t width, uint32_t height, const uint8_t* image);



/**
 * Read a PPM image file.
 *
 * @param filename path of the file to open
 * @param[out] width width of the image
 * @param[out] height of the image
 * @returns owned tightly packed RGB8 pixel buffer, or NULL on failure; free with dvz_free()
 */
DVZ_EXPORT uint8_t* dvz_read_ppm(const char* filename, int* width, int* height);



/**
 * Save an sRGB RGBA8 image to a PNG file.
 *
 * @param filename path to the PNG file to create
 * @param width width of the image
 * @param height height of the image
 * @param rgba pointer to tightly packed sRGB RGBA8 pixels with straight linear alpha
 */
DVZ_EXPORT int
dvz_write_png(const char* filename, uint32_t width, uint32_t height, const uint8_t* rgba);



/**
 * Compress an sRGB RGB8 image to PNG and write it to a memory buffer.
 *
 * @param width width of the image
 * @param height height of the image
 * @param rgb pointer to tightly packed sRGB RGB8 pixels
 * @param size pointer to a variable that will contain the size of the buffer
 * @param out pointer to an owned PNG byte buffer allocated with the Datoviz allocator; free with
 * dvz_free()
 */
DVZ_EXPORT int
dvz_make_png(uint32_t width, uint32_t height, const uint8_t* rgb, DvzSize* size, void** out);



/**
 * Decode a PNG image from memory into tightly packed RGB8 pixels.
 *
 * @param size size of the PNG byte buffer
 * @param bytes PNG byte buffer
 * @param[out] width decoded image width
 * @param[out] height decoded image height
 * @returns owned RGB8 pixel buffer allocated with the Datoviz allocator, or NULL on failure; free
 * with dvz_free()
 */
DVZ_EXPORT uint8_t*
dvz_load_png(DvzSize size, unsigned char* bytes, uint32_t* width, uint32_t* height);



/*************************************************************************************************/
/*  JPEG I/O                                                                                     */
/*************************************************************************************************/

/**
 * Decode a JPEG image from memory into tightly packed RGBA8 pixels.
 *
 * @param size size of the JPEG byte buffer
 * @param bytes JPEG byte buffer
 * @param[out] width decoded image width
 * @param[out] height decoded image height
 * @returns RGBA8 pixel buffer allocated with the Datoviz allocator, or NULL on failure
 *
 * @note Free the returned buffer with dvz_free().
 */
uint8_t*
dvz_load_jpeg(DvzSize size, const unsigned char* bytes, uint32_t* width, uint32_t* height);



/**
 * Read and decode a JPEG image file into tightly packed RGBA8 pixels.
 *
 * @param filename path of the JPEG file to open
 * @param[out] width decoded image width
 * @param[out] height decoded image height
 * @returns RGBA8 pixel buffer allocated with the Datoviz allocator, or NULL on failure
 *
 * @note Free the returned buffer with dvz_free().
 */
uint8_t* dvz_read_jpeg(const char* filename, uint32_t* width, uint32_t* height);



/*************************************************************************************************/
/*  Resources utils (files included in the shared dynamic library)                               */
/*************************************************************************************************/

// Defined in cmake-generated file build/_shaders.c
const unsigned char* dvz_resource_shader(const char* name, unsigned long* size);


// Defined in cmake-generated file build/_wgsl_shaders.c
const char* dvz_resource_wgsl(const char* name, unsigned long* size);


// Defined in cmake-generated file build/_glsl_shaders.c
const char* dvz_resource_glsl(const char* name, unsigned long* size);



// Defined in cmake-generated file build/_textures.c
unsigned char* dvz_resource_texture(const char* name, unsigned long* size);



// Defined in cmake-generated file build/_fonts.c
const unsigned char* dvz_resource_font(const char* name, unsigned long* size);



// Defined in cmake-generated file build/_testdata.c
// NOTE: only built in the CLI, not in libdatoviz.
unsigned char* dvz_resource_testdata(const char* name, unsigned long* size);



EXTERN_C_OFF

#endif
