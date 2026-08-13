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

#include "datoviz_testing.h"

#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "testing.h"



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
    // Tests may change the process-wide logger threshold. Restore the runner threshold before each
    // case so that a previous test cannot make later expected-error checks silent.
    if (getenv("DVZ_LOG_LEVEL") == NULL)
        log_set_level(LOG_INFO);
    log_set_intercept(_dvz_testing_log_intercept, ctx);
}



static void _dvz_testing_log_uninstall(void* user_data)
{
    (void)user_data;
    log_set_intercept(NULL, NULL);
}



/**
 * Write the Datoviz run metadata used when no GPU adapter is linked.
 *
 * @param state unused adapter state
 * @param json output JSON buffer
 * @param size output buffer size
 * @return zero on success
 */
static int _dvz_testing_null_gpu_write_json(const void* state, char* json, size_t size)
{
    (void)state;
    ANN(json);
    const int written = dvz_snprintf(json, size, "{\"gpu\":null}");
    return written < 0 || (size_t)written >= size ? 1 : 0;
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
    tst_set_strict_unexpected_errors(suite, true);

    // Every Datoviz runner emits the same run shape. GPU-capable runners replace this adapter.
    TstRunAdapter run_adapter = {0};
    run_adapter.write_json = _dvz_testing_null_gpu_write_json;
    tst_suite_set_run_adapter(suite, &run_adapter);
}
