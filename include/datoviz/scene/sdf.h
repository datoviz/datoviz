/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/* Sdf                                                                                           */
/*************************************************************************************************/

#ifndef DVZ_HEADER_SDF
#define DVZ_HEADER_SDF



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_log.h"
#include "_macros.h"
#include "_map.h"
#include "datoviz_math.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

// Forward declarations.
typedef struct DvzBatch DvzBatch;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    DVZ_SDF_MODE_NONE = 0,
    DVZ_SDF_MODE_SDF = 1,
    DVZ_SDF_MODE_MSDF = 2,
    // DVZ_SDF_MODE_MTSDF = 3,
} DvzSdfMode;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/


EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 */
float* dvz_sdf_from_svg(const char* svg_path, uint32_t width, uint32_t height);



/**
 */
float* dvz_msdf_from_svg(const char* svg_path, uint32_t width, uint32_t height);



/**
 */
uint8_t* dvz_sdf_to_rgb(float* sdf, uint32_t width, uint32_t height);



/**
 */
uint8_t* dvz_msdf_to_rgb(float* sdf, uint32_t width, uint32_t height);



/**
 */
uint8_t* dvz_rgb_to_rgba_char(uint32_t count, uint8_t* rgb);



/**
 */
float* dvz_rgb_to_rgba_float(uint32_t count, float* rgb);



EXTERN_C_OFF

#endif
