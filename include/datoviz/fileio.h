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
DVZ_EXPORT uint32_t* dvz_read_file(const char* filename, size_t* size);



/**
 * Read a NumPy NPY file.
 *
 * @param filename path of the file to open
 * @param[out] size of the file
 * @returns pointer to a buffer containing the array elements
 */
DVZ_EXPORT char* dvz_read_npy(const char* filename, size_t* size);



/*************************************************************************************************/
/*  Image file I/O utils                                                                         */
/*************************************************************************************************/

/**
 * Save an image to a PPM file (short ASCII header and flat binary RGBA values).
 *
 * @param filename path to the PPM file to create
 * @param width width of the image
 * @param height height of the image
 * @param image pointer to an array of 32-bit RGBA values
 */
DVZ_EXPORT int
dvz_write_ppm(const char* filename, uint32_t width, uint32_t height, const uint8_t* image);



/**
 * Read a PPM image file.
 *
 * @param filename path of the file to open
 * @param[out] width width of the image
 * @param[out] height of the image
 * @returns pointer to a buffer with the loaded RGBA pixel colors
 */
DVZ_EXPORT uint8_t* dvz_read_ppm(const char* filename, int* width, int* height);



/*************************************************************************************************/
/*  Resources utils (files included in the shared dynamic library)                               */
/*************************************************************************************************/

/**
 * Save an image to a PNG file
 *
 * @param filename path to the PNG file to create
 * @param width width of the image
 * @param height height of the image
 * @param image pointer to an array of 32-bit RGBA values
 */
DVZ_EXPORT int
dvz_write_png(const char* filename, uint32_t width, uint32_t height, const uint8_t* image);



// Defined in cmake-generated file build/_shaders.c
DVZ_EXPORT unsigned char* dvz_resource_shader(const char* name, unsigned long* size);



// Defined in cmake-generated file build/_colortex.c
DVZ_EXPORT unsigned char* dvz_resource_texture(const char* name, unsigned long* size);



// Defined in cmake-generated file build/_fonts.c
DVZ_EXPORT unsigned char* dvz_resource_font(const char* name, unsigned long* size);



EXTERN_C_OFF

#endif
