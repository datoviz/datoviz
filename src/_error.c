/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Assertions and error handling                                                                */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_log.h"
#include "datoviz.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

char error_message[2048] = {0};
DvzErrorCallback error_callback = NULL;



void _assert(bool assertion, const char* message, const char* filename, int line)
{
    if (!assertion)
    {
        // Prepare the error message with the filename, line number, and failing assertion.
        sprintf(error_message, "Assertion error in %s:%d: %s\n", filename, line, message);

        // Log the error message
        log_error("%s", error_message);

        // Call the error callback if there is one.
        if (error_callback)
        {
            error_callback(error_message);
        }

        // Exit the process.
        exit(EXIT_FAILURE);
    }
}



void dvz_error_callback(DvzErrorCallback cb)
{
    ANN(cb);
    log_debug("Registering an error callback function");
    error_callback = cb;
}
