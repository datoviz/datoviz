/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  DRP2 runtime semantic validation                                                            */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "datoviz/common/macros.h"
#include "datoviz/drp2/types.h"



EXTERN_C_ON

/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct DvzDrp2ValidationResult DvzDrp2ValidationResult;

struct DvzDrp2ValidationResult
{
    bool ok;
    DvzDrp2ValidationCode code;
    uint32_t command_index;
};



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Validate a DRP2 command stream against the backend-agnostic semantic rules.
 *
 * @param stream the command stream
 * @return the validation result
 */
DVZ_EXPORT DvzDrp2ValidationResult
dvz_drp2_validate_stream(const DvzDrp2CommandStream* stream);

EXTERN_C_OFF
