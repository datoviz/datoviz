/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Vulkan External Memory                                                                       */
/*************************************************************************************************/

#ifndef DVZ_HEADER_PUBLIC_EXTERNAL
#define DVZ_HEADER_PUBLIC_EXTERNAL



/*************************************************************************************************/
/*  Includes                                                                                    */
/*************************************************************************************************/

#include "datoviz_math.h"
#include "datoviz_types.h"



/*************************************************************************************************/
/*  Types                                                                                        */
/*************************************************************************************************/

typedef struct DvzRenderer DvzRenderer;
typedef struct DvzVisual DvzVisual;



EXTERN_C_ON


/*************************************************************************************************/
/*************************************************************************************************/
/*  External API                                                                                 */
/*************************************************************************************************/
/*************************************************************************************************/

/**
 * Get an external memory handle of a vertex dat.
 *
 * @param rd the renderer
 * @param visual the visual
 * @param binding_idx the binding index of the dat that is being used as vertex buffer
 * @param[out] offset the offset, in bytes, of the dat, within the buffer containing that dat
 * @returns the external memory handle of that buffer
 */
DVZ_EXPORT int
dvz_external_vertex(DvzRenderer* rd, DvzVisual* visual, uint32_t binding_idx, DvzSize* offset);



/**
 * Get an external memory handle of an index dat.
 *
 * @param rd the renderer
 * @param visual the visual
 * @param[out] offset the offset, in bytes, of the dat, within the buffer containing that dat
 * @returns the external memory handle of that buffer
 */
DVZ_EXPORT int dvz_external_index(DvzRenderer* rd, DvzVisual* visual, DvzSize* offset);



/**
 * Get an external memory handle of a dat.
 *
 * @param rd the renderer
 * @param visual the visual
 * @param slot_idx the slot index of the dat
 * @param[out] offset the offset, in bytes, of the dat, within the buffer containing that dat
 * @returns the external memory handle of that buffer
 */
DVZ_EXPORT int
dvz_external_dat(DvzRenderer* rd, DvzVisual* visual, uint32_t slot_idx, DvzSize* offset);



/**
 * Get an external memory handle of a tex's staging buffer.
 *
 * @param rd the renderer
 * @param visual the visual
 * @param slot_idx the slot index of the tex
 * @param[out] offset the offset, in bytes, of the tex's staging dat, within the buffer containing
 * that dat
 * @returns the external memory handle of that buffer
 */
DVZ_EXPORT int
dvz_external_tex(DvzRenderer* rd, DvzVisual* visual, uint32_t slot_idx, DvzSize* offset);



#endif
