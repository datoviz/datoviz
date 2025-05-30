/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/* Reference                                                                                     */
/*************************************************************************************************/

#ifndef DVZ_HEADER_REF
#define DVZ_HEADER_REF



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_log.h"
#include "box.h"
#include "datoviz_enums.h"
#include "datoviz_math.h"
#include "datoviz_types.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzRef DvzRef;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzRef
{
    DvzBox box;
    bool is_set[DVZ_DIM_COUNT];
    int flags;
};



#endif
