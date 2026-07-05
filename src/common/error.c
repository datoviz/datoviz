/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Error handling                                                                               */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdlib.h>

#include "_alloc.h"
#include "_assertions.h"
#include "datoviz/common/functions.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

char error_message[2048] = {0};
DvzErrorCallback error_callback = NULL;
void* error_callback_user_data = NULL;



DvzResult dvz_error_set_callback(DvzErrorCallback cb, void* user_data)
{
    // log_debug("Registering an error callback function");
    error_callback = cb;
    error_callback_user_data = user_data;
    return DVZ_OK;
}



void dvz_memory_free(void* pointer)
{
    dvz_free(pointer);
}
