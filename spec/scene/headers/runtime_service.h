/*
 * Draft header sketch derived from spec/scene/.
 * This file is informative only and is not part of the installed public API.
 */

/*************************************************************************************************/
/*  Scene runtime service sketch                                                                 */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "datoviz/common/macros.h"

#include "diagnostics.h"



/*************************************************************************************************/
/*  Forward declarations                                                                         */
/*************************************************************************************************/

typedef struct DvzRuntimeService DvzRuntimeService;
typedef struct DvzSceneFramePlan DvzSceneFramePlan;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    DVZ_RUNTIME_SUBMISSION_ACCEPTED = 0,
    DVZ_RUNTIME_SUBMISSION_REJECTED_CAPABILITY = 1,
    DVZ_RUNTIME_SUBMISSION_REJECTED_RESOURCE_STATE = 2,
    DVZ_RUNTIME_SUBMISSION_REJECTED_INTERNAL_FAILURE = 3,
} DvzRuntimeSubmissionStatus;


typedef enum
{
    DVZ_RUNTIME_COMPLETION_NONE = 0,
    DVZ_RUNTIME_COMPLETION_PICK = 1,
    DVZ_RUNTIME_COMPLETION_IMAGE = 2,
    DVZ_RUNTIME_COMPLETION_COMPUTE = 3,
    DVZ_RUNTIME_COMPLETION_FAILURE = 4,
} DvzRuntimeCompletionKind;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct DvzCapabilitySnapshot
{
    bool has_compute;
    bool has_fp64;
    bool has_pick_readback;
    bool has_offscreen_readback;
    uint32_t max_sample_count;
    uint32_t max_color_targets;
    uint32_t max_texture_2d_dimension;
    uint32_t max_texture_3d_dimension;
    uint32_t capability_generation;
} DvzCapabilitySnapshot;


typedef struct
{
    uint64_t submission_id;
    uint64_t scene_revision;
    DvzRuntimeSubmissionStatus status;
    DvzDiagnosticReport diagnostics;
} DvzRuntimeSubmissionResult;


typedef struct
{
    DvzRuntimeCompletionKind kind;
    uint64_t submission_id;
    uint64_t request_id;
    uint64_t target_id;
    bool discardable;
    const void* payload;
    size_t payload_size;
    DvzDiagnosticReport diagnostics;
} DvzRuntimeCompletion;



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Query a stable capability snapshot for scene validation and capability adaptation.
 *
 * @param runtime runtime service handle
 * @returns capability snapshot for the current runtime
 */
DVZ_EXPORT DvzCapabilitySnapshot dvz_runtime_query_capabilities(DvzRuntimeService* runtime);



/**
 * Submit one scene-level frame plan to the runtime execution layer.
 *
 * @param runtime runtime service handle
 * @param frame_plan scene-level frame plan built by the scene layer
 * @returns submission result including acceptance state and diagnostics
 */
DVZ_EXPORT DvzRuntimeSubmissionResult
dvz_runtime_submit(DvzRuntimeService* runtime, const DvzSceneFramePlan* frame_plan);



/**
 * Poll for one runtime completion event.
 *
 * @param runtime runtime service handle
 * @param out_completion completion record written on success
 * @returns true when one completion record was written, false when no completion is available
 */
DVZ_EXPORT bool
dvz_runtime_poll_completion(DvzRuntimeService* runtime, DvzRuntimeCompletion* out_completion);



EXTERN_C_OFF
