/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene FramePlan diagnostics                                                                  */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "internal.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Initialize a diagnostic report.
 *
 * @param report the diagnostic report
 */
void dvz_diagnostic_report_init(DvzDiagnosticReport* report)
{
    ANN(report);
    dvz_memset(report, sizeof(DvzDiagnosticReport), 0, sizeof(DvzDiagnosticReport));
}



/**
 * Add a diagnostic message.
 *
 * @param report the diagnostic report
 * @param message the diagnostic message
 * @return whether the message was added
 */
bool dvz_diagnostic_report_add(DvzDiagnosticReport* report, const char* message)
{
    ANN(report);
    ANN(message);
    if (report->count >= DVZ_SCENE_MAX_DIAGNOSTICS)
        return false;
    _frame_plan_copy_label(
        report->messages[report->count], DVZ_SCENE_DIAGNOSTIC_SIZE, message);
    report->count++;
    return true;
}



/**
 * Return a diagnostic count.
 *
 * @param report the diagnostic report
 * @return the number of diagnostic messages
 */
uint32_t dvz_diagnostic_report_count(const DvzDiagnosticReport* report)
{
    if (report == NULL)
        return 0;
    return report->count;
}



/**
 * Return a diagnostic message.
 *
 * @param report the diagnostic report
 * @param index the diagnostic index
 * @return the diagnostic message, or NULL when index is out of bounds
 */
const char* dvz_diagnostic_report_get(const DvzDiagnosticReport* report, uint32_t index)
{
    if (report == NULL || index >= report->count)
        return NULL;
    return report->messages[index];
}
