/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Error handling                                                                               */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz/common/macros.h"



/*************************************************************************************************/
/*  Definitions                                                                                  */
/*************************************************************************************************/

EXTERN_C_ON

// Error callback function type.
typedef void (*DvzErrorCallback)(const char* message);

extern char error_message[2048];
extern DvzErrorCallback error_callback;


EXTERN_C_OFF
