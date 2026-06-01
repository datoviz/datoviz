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
#include "_log.h"
#include "_scene.h"
#include "core/format_state_internal.h"


/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_FORMAT_DESC_KNOWN_FLAGS 0u


/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return whether a format descriptor contains only zero bytes.
 *
 * @param src the source descriptor
 * @return whether the descriptor is zero-filled
 */
bool _scene_format_desc_is_zero(const DvzFormatDesc* src)
{
    if (src == NULL)
        return true;
    return src->struct_size == 0 && src->flags == 0 && src->precision == 0 &&
           !src->scientific && !src->trim_trailing_zeros && !src->show_unit &&
           src->unit == NULL && src->prefix == NULL && src->suffix == NULL;
}


/**
 * Validate public format descriptor ABI fields.
 *
 * @param src the source descriptor
 * @return whether the descriptor is accepted
 */
bool _scene_format_desc_validate(const DvzFormatDesc* src)
{
    if (src == NULL)
        return true;
    if (!DVZ_STRUCT_VALID(src, DvzFormatDesc, DVZ_FORMAT_DESC_KNOWN_FLAGS))
    {
        log_error("invalid format descriptor ABI");
        return false;
    }
    return true;
}


/**
 * Return the default format descriptor.
 *
 * @return default format descriptor
 */
DvzFormatDesc dvz_format_desc(void)
{
    return (DvzFormatDesc){DVZ_STRUCT_INIT_FIELDS(DvzFormatDesc)};
}


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
    if (!_scene_format_desc_validate(src))
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
