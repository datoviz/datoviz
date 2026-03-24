/*
 * Draft header sketch derived from spec/scene/.
 * This file is informative only and is not part of the installed public API.
 */

/*************************************************************************************************/
/*  Scene diagnostics sketch                                                                     */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stddef.h>
#include <stdint.h>

#include "datoviz/common/macros.h"



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    DVZ_DIAGNOSTIC_SEVERITY_FATAL = 0,
    DVZ_DIAGNOSTIC_SEVERITY_RECOVERABLE = 1,
    DVZ_DIAGNOSTIC_SEVERITY_WARNING = 2,
    DVZ_DIAGNOSTIC_SEVERITY_INFO = 3,
} DvzDiagnosticSeverity;


typedef enum
{
    DVZ_DIAGNOSTIC_PHASE_VALIDATION = 0,
    DVZ_DIAGNOSTIC_PHASE_CAPABILITY_ADAPTATION = 1,
    DVZ_DIAGNOSTIC_PHASE_FRAME_PLANNING = 2,
    DVZ_DIAGNOSTIC_PHASE_RUNTIME_SUBMISSION = 3,
    DVZ_DIAGNOSTIC_PHASE_RUNTIME_COMPLETION = 4,
} DvzDiagnosticPhase;


typedef enum
{
    DVZ_DIAGNOSTIC_CATEGORY_STRUCTURE = 0,
    DVZ_DIAGNOSTIC_CATEGORY_RESOURCE = 1,
    DVZ_DIAGNOSTIC_CATEGORY_TRANSFORM = 2,
    DVZ_DIAGNOSTIC_CATEGORY_MAPPING = 3,
    DVZ_DIAGNOSTIC_CATEGORY_PICKING = 4,
    DVZ_DIAGNOSTIC_CATEGORY_ANNOTATION = 5,
    DVZ_DIAGNOSTIC_CATEGORY_CAPABILITY = 6,
    DVZ_DIAGNOSTIC_CATEGORY_PLAN_TOPOLOGY = 7,
    DVZ_DIAGNOSTIC_CATEGORY_READBACK = 8,
    DVZ_DIAGNOSTIC_CATEGORY_RUNTIME_EXECUTION = 9,
} DvzDiagnosticCategory;


typedef enum
{
    DVZ_DIAGNOSTIC_SUBJECT_SCENE = 0,
    DVZ_DIAGNOSTIC_SUBJECT_PANEL = 1,
    DVZ_DIAGNOSTIC_SUBJECT_VISUAL = 2,
    DVZ_DIAGNOSTIC_SUBJECT_RESOURCE = 3,
    DVZ_DIAGNOSTIC_SUBJECT_AXIS = 4,
    DVZ_DIAGNOSTIC_SUBJECT_ANNOTATION = 5,
    DVZ_DIAGNOSTIC_SUBJECT_LEGEND = 6,
    DVZ_DIAGNOSTIC_SUBJECT_COLORBAR = 7,
    DVZ_DIAGNOSTIC_SUBJECT_SCALE_MAPPING = 8,
    DVZ_DIAGNOSTIC_SUBJECT_FRAME_PLAN = 9,
    DVZ_DIAGNOSTIC_SUBJECT_PLAN_NODE = 10,
    DVZ_DIAGNOSTIC_SUBJECT_RUNTIME_SERVICE = 11,
} DvzDiagnosticSubjectKind;


typedef enum
{
    DVZ_DIAGNOSTIC_SCOPE_GLOBAL = 0,
    DVZ_DIAGNOSTIC_SCOPE_SCENE = 1,
    DVZ_DIAGNOSTIC_SCOPE_PANEL = 2,
    DVZ_DIAGNOSTIC_SCOPE_VISUAL = 3,
    DVZ_DIAGNOSTIC_SCOPE_RESOURCE = 4,
    DVZ_DIAGNOSTIC_SCOPE_PLAN_NODE = 5,
    DVZ_DIAGNOSTIC_SCOPE_REQUEST = 6,
} DvzDiagnosticScope;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct
{
    DvzDiagnosticSubjectKind kind;
    uint64_t id;
} DvzDiagnosticIdentity;


typedef struct
{
    DvzDiagnosticSeverity severity;
    DvzDiagnosticPhase phase;
    DvzDiagnosticCategory category;
    uint32_t code;
    const char* message;
    DvzDiagnosticSubjectKind subject_kind;
    uint64_t subject_id;
    DvzDiagnosticScope scope;
    const DvzDiagnosticIdentity* related;
    uint32_t related_count;
    const void* context_payload;
    size_t context_payload_size;
} DvzDiagnosticRecord;


typedef enum
{
    DVZ_DIAGNOSTIC_REPORT_SUCCESS = 0,
    DVZ_DIAGNOSTIC_REPORT_DEGRADED_SUCCESS = 1,
    DVZ_DIAGNOSTIC_REPORT_RECOVERABLE_FAILURE = 2,
    DVZ_DIAGNOSTIC_REPORT_FATAL_FAILURE = 3,
} DvzDiagnosticReportStatus;


typedef struct
{
    DvzDiagnosticPhase phase_first;
    DvzDiagnosticPhase phase_last;
    DvzDiagnosticReportStatus status;
    const DvzDiagnosticRecord* records;
    uint32_t record_count;
} DvzDiagnosticReport;



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return a zero-initialized diagnostic report suitable for sketch-level call sites.
 *
 * @returns empty diagnostic report
 */
DVZ_EXPORT DvzDiagnosticReport dvz_diagnostic_report(void);



EXTERN_C_OFF
