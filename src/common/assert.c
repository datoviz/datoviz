/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Assertions                                                                                   */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_assertions.h"
#include "_compat.h"
#include "_error.h"
#include "_log.h"
#include <stdlib.h>



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

void dvz_assert(bool assertion, const char* message, const char* filename, int line)
{
    if (!assertion)
    {
        // Prepare the error message with the filename, line number, and failing assertion.
        dvz_snprintf(
            error_message, sizeof(error_message), "Assertion error in %s:%d: %s\n", filename, line,
            message);

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
