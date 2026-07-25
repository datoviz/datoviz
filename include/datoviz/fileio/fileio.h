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
 * @param filename path of the file; must not be NULL
 * @return file size in bytes, or zero if the file cannot be opened (also valid for an empty file)
 */
DVZ_EXPORT DvzSize dvz_file_size(const char* filename);



/**
 * Read a binary file.
 *
 * @param filename path of the file to open; must not be NULL
 * @param[out] size optional destination receiving the buffer size in bytes
 * @return owned byte buffer, or NULL if the file cannot be opened; free with `dvz_memory_free()`
 */
DVZ_EXPORT void* dvz_read_file(const char* filename, DvzSize* size);



/**
 * Read a text file into an owned null-terminated buffer.
 *
 * The returned size excludes the terminating null byte. Embedded null bytes are preserved and
 * remain observable through the explicit size.
 *
 * @param filename path of the file to open; must not be NULL
 * @param[out] size optional destination receiving the text size in bytes
 * @return owned null-terminated text, or NULL on failure; free with `dvz_memory_free()`
 */
DVZ_EXPORT char* dvz_read_text(const char* filename, DvzSize* size);



/**
 * Read the data payload of a NumPy NPY v1 file.
 *
 * This minimal reader strips the NPY header but does not expose or convert the array dtype, shape,
 * byte order, or storage order.
 *
 * @param filename path of the file to open; must not be NULL
 * @param[out] size optional destination receiving the payload size in bytes
 * @return owned buffer containing the array payload, or NULL on failure; free with
 * `dvz_memory_free()`
 */
DVZ_EXPORT void* dvz_read_npy(const char* filename, DvzSize* size);



/**
 * Extract the data payload from an in-memory NumPy NPY v1 buffer.
 *
 * This minimal parser validates the magic and payload offset but does not expose or convert the
 * array dtype, shape, byte order, or storage order.
 *
 * @param bytes complete NPY file bytes; must not be NULL
 * @param size_bytes size of @p bytes in bytes
 * @return owned copy of the array payload, or NULL for invalid input or allocation failure; free
 * with `dvz_memory_free()`
 */
DVZ_EXPORT void* dvz_parse_npy(const void* bytes, DvzSize size_bytes);



/**
 * Read a compressed GZIP file.
 *
 * @param filename path of the GZIP-compressed file; must not be NULL
 * @param[out] size destination receiving the decompressed size in bytes; must not be NULL when
 * zlib support is enabled
 * @return owned decompressed byte buffer, or NULL on failure or when zlib support is unavailable;
 * free with `dvz_memory_free()`
 */
DVZ_EXPORT char* dvz_read_gz(const char* filename, DvzSize* size);



/**
 * Write bytes to a file.
 *
 * @param filename destination file path; must not be NULL
 * @param mode standard `fopen()` mode, typically `"wb"` or `"ab"`; must not be NULL
 * @param size number of bytes to write
 * @param bytes source buffer containing at least @p size bytes; must not be NULL when size is nonzero
 * @return zero if the file was opened, nonzero otherwise; write errors are not reported
 */
DVZ_EXPORT int
dvz_write_bytes(const char* filename, const char* mode, DvzSize size, const uint8_t* bytes);



/*************************************************************************************************/
/*  Image file I/O utils                                                                         */
/*************************************************************************************************/

/**
 * Save an image to a PPM file (short ASCII header and flat binary RGB values).
 *
 * @param filename destination PPM file path; must not be NULL
 * @param width image width in pixels
 * @param height image height in pixels
 * @param image tightly packed RGB8 pixels containing `width * height * 3` bytes; must not be NULL
 * @return zero if the file was opened, nonzero otherwise; write errors are not reported
 */
DVZ_EXPORT int
dvz_write_ppm(const char* filename, uint32_t width, uint32_t height, const uint8_t* image);



/**
 * Read a PPM image file.
 *
 * @param filename source P6 PPM file path; must not be NULL
 * @param[out] width destination receiving the image width in pixels; must not be NULL
 * @param[out] height destination receiving the image height in pixels; must not be NULL
 * @return owned tightly packed RGB8 pixel buffer, or NULL on failure; free with
 * `dvz_memory_free()`
 */
DVZ_EXPORT uint8_t* dvz_read_ppm(const char* filename, uint32_t* width, uint32_t* height);



/**
 * Save an sRGB RGBA8 image to a PNG file.
 *
 * @param filename destination PNG file path; must not be NULL
 * @param width image width in pixels; must be positive
 * @param height image height in pixels; must be positive
 * @param rgba tightly packed sRGB RGBA8 pixels with straight linear alpha; must contain
 * `width * height * 4` bytes
 * @return zero after the encode attempt
 */
DVZ_EXPORT int
dvz_write_png(const char* filename, uint32_t width, uint32_t height, const uint8_t* rgba);



/**
 * Compress an sRGB RGB8 image to PNG and write it to a memory buffer.
 *
 * @param width image width in pixels; must be positive
 * @param height image height in pixels; must be positive
 * @param rgb tightly packed sRGB RGB8 pixels containing `width * height * 3` bytes; must not be NULL
 * @param[out] size destination receiving the PNG buffer size in bytes; must not be NULL
 * @param[out] out destination receiving an owned PNG byte buffer; must not be NULL and the returned
 * buffer must be freed with `dvz_memory_free()`
 * @return zero after the encode attempt
 */
DVZ_EXPORT int
dvz_make_png(uint32_t width, uint32_t height, const uint8_t* rgb, DvzSize* size, void** out);



/**
 * Decode a PNG image from memory into tightly packed RGB8 pixels.
 *
 * @param bytes complete PNG byte buffer; must not be NULL
 * @param size_bytes size of the PNG byte buffer in bytes
 * @param[out] width destination receiving the decoded image width in pixels; must not be NULL
 * @param[out] height destination receiving the decoded image height in pixels; must not be NULL
 * @return owned RGB8 pixel buffer, or NULL on decode failure; free with `dvz_memory_free()`
 */
DVZ_EXPORT uint8_t*
dvz_load_png(const void* bytes, DvzSize size_bytes, uint32_t* width, uint32_t* height);



/*************************************************************************************************/
/*  JPEG I/O                                                                                     */
/*************************************************************************************************/

/**
 * Decode a JPEG image from memory into tightly packed RGBA8 pixels.
 *
 * @param bytes complete JPEG byte buffer; must not be NULL
 * @param size_bytes size of the JPEG byte buffer in bytes
 * @param[out] width destination receiving the decoded image width in pixels; must not be NULL
 * @param[out] height destination receiving the decoded image height in pixels; must not be NULL
 * @return owned RGBA8 pixel buffer, or NULL on failure; free with `dvz_memory_free()`
 */
DVZ_EXPORT uint8_t*
dvz_load_jpeg(const void* bytes, DvzSize size_bytes, uint32_t* width, uint32_t* height);



/**
 * Read and decode a JPEG image file into tightly packed RGBA8 pixels.
 *
 * @param filename source JPEG file path; must not be NULL
 * @param[out] width destination receiving the decoded image width in pixels; must not be NULL
 * @param[out] height destination receiving the decoded image height in pixels; must not be NULL
 * @return owned RGBA8 pixel buffer, or NULL on failure; free with `dvz_memory_free()`
 */
DVZ_EXPORT uint8_t* dvz_read_jpeg(const char* filename, uint32_t* width, uint32_t* height);



/*************************************************************************************************/
/*  Resources utils (files included in the shared dynamic library)                               */
/*************************************************************************************************/

/**
 * Look up an embedded SPIR-V shader resource by name.
 *
 * @param name resource name without a file extension; must not be NULL
 * @param[out] size optional destination receiving the resource size in bytes, or zero if not found
 * @return borrowed immutable bytes with static lifetime, or NULL if @p name is not found
 */
DVZ_EXPORT const unsigned char* dvz_resource_shader(const char* name, DvzSize* size);


/**
 * Look up an embedded WGSL shader source by name.
 *
 * @param name resource name without a file extension; must not be NULL
 * @param[out] size optional destination receiving the source size in bytes, or zero if not found
 * @return borrowed immutable source bytes with static lifetime, or NULL if @p name is not found
 */
DVZ_EXPORT const char* dvz_resource_wgsl(const char* name, DvzSize* size);


/**
 * Look up an embedded GLSL shader source by name.
 *
 * @param name resource name without a file extension; must not be NULL
 * @param[out] size optional destination receiving the source size in bytes, or zero if not found
 * @return borrowed immutable source bytes with static lifetime, or NULL if @p name is not found
 */
DVZ_EXPORT const char* dvz_resource_glsl(const char* name, DvzSize* size);



/**
 * Look up an embedded font resource by name.
 *
 * @param name resource name without a file extension; must not be NULL
 * @param[out] size optional destination receiving the font size in bytes, or zero if not found
 * @return borrowed immutable font bytes with static lifetime, or NULL if @p name is not found
 */
DVZ_EXPORT const unsigned char* dvz_resource_font(const char* name, DvzSize* size);



EXTERN_C_OFF

#endif
