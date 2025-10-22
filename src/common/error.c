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

#include "_assertions.h"
#include "_error.h"
#include "datoviz/common/functions.h"
#include <stdlib.h>



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

char error_message[2048] = {0};
DvzErrorCallback error_callback = NULL;



void dvz_error_callback(DvzErrorCallback cb)
{
    ANN(cb);
    // log_debug("Registering an error callback function");
    error_callback = cb;
}
