/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene format state                                                                           */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_alloc.h"
#include "_compat.h"
#include "_scene.h"
#include "core/format_state_internal.h"


/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Copy optional public formatting state into retained scene storage.
 *
 * @param dst the destination format state
 * @param src the source descriptor, or NULL to clear the destination
 */
void _scene_format_state_copy(DvzSceneFormatState* dst, const DvzFormatDesc* src)
{
    ANN(dst);
    dvz_memset(dst, sizeof(DvzSceneFormatState), 0, sizeof(DvzSceneFormatState));
    if (src == NULL)
        return;
    dst->precision = src->precision;
    dst->scientific = src->scientific;
    dst->trim_trailing_zeros = src->trim_trailing_zeros;
    dst->show_unit = src->show_unit;
    if (src->unit != NULL)
        dvz_strlcpy(dst->unit, src->unit, sizeof(dst->unit));
    if (src->prefix != NULL)
        dvz_strlcpy(dst->prefix, src->prefix, sizeof(dst->prefix));
    if (src->suffix != NULL)
        dvz_strlcpy(dst->suffix, src->suffix, sizeof(dst->suffix));
}
