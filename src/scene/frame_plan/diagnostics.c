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
#include <string.h>

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
    return dvz_diagnostic_report_add_with_severity(
        report, DVZ_DIAGNOSTIC_SEVERITY_FATAL, message);
}



/**
 * Add a diagnostic message with an explicit severity.
 *
 * @param report the diagnostic report
 * @param severity the diagnostic severity
 * @param message the diagnostic message
 * @return whether the message was added
 */
bool dvz_diagnostic_report_add_with_severity(
    DvzDiagnosticReport* report, DvzDiagnosticSeverity severity, const char* message)
{
    ANN(report);
    ANN(message);
    if (report->count >= DVZ_SCENE_MAX_DIAGNOSTICS)
        return false;
    if (severity < DVZ_DIAGNOSTIC_SEVERITY_FATAL || severity > DVZ_DIAGNOSTIC_SEVERITY_INFO)
        return false;
    report->severities[report->count] = severity;
    char* dst = report->messages[report->count];
    size_t len = strlen(message);
    if (len >= DVZ_SCENE_DIAGNOSTIC_SIZE)
        len = DVZ_SCENE_DIAGNOSTIC_SIZE - 1;
    memcpy(dst, message, len);
    dst[len] = '\0';
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
 * Return the number of fatal or recoverable diagnostics.
 *
 * @param report the diagnostic report
 * @return the number of error-level diagnostic messages
 */
uint32_t dvz_diagnostic_report_error_count(const DvzDiagnosticReport* report)
{
    if (report == NULL)
        return 0;
    uint32_t count = 0;
    for (uint32_t i = 0; i < report->count; i++)
    {
        if (report->severities[i] <= DVZ_DIAGNOSTIC_SEVERITY_RECOVERABLE)
            count++;
    }
    return count;
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



/**
 * Return a diagnostic severity.
 *
 * @param report the diagnostic report
 * @param index the diagnostic index
 * @return the diagnostic severity, or fatal when index is out of bounds
 */
DvzDiagnosticSeverity
dvz_diagnostic_report_get_severity(const DvzDiagnosticReport* report, uint32_t index)
{
    if (report == NULL || index >= report->count)
        return DVZ_DIAGNOSTIC_SEVERITY_FATAL;
    return report->severities[index];
}
