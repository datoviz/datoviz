/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Datoviz testing adapters                                                                     */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdlib.h>

#include "_log.h"
#include "datoviz_testing.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static int _dvz_testing_log_intercept(
    void* user_data, int level, const char* file, int line, const char* message)
{
    return tst_context_log((TstContext*)user_data, level, file, line, message);
}



static void _dvz_testing_log_install(TstContext* ctx, void* user_data)
{
    (void)user_data;
    log_set_intercept(_dvz_testing_log_intercept, ctx);
}



static void _dvz_testing_log_uninstall(void* user_data)
{
    (void)user_data;
    log_set_intercept(NULL, NULL);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

void dvz_testing_install_log_adapter(TstSuite* suite)
{
    // Release builds are silent by default, but strict negative tests must observe expected logs.
    if (getenv("DVZ_LOG_LEVEL") == NULL)
        log_set_level(LOG_INFO);

    TstLogAdapter adapter = {0};
    adapter.install = _dvz_testing_log_install;
    adapter.uninstall = _dvz_testing_log_uninstall;
    adapter.user_data = NULL;
    adapter.error_level = LOG_ERROR;
    tst_suite_set_log_adapter(suite, &adapter);
}
