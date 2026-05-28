/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene query readback                                                                         */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>

#include "datoviz/drp2/runtime.h"
#include "../../drp2/_stream.h"
#include "_scene.h"
#include "_assertions.h"
#include "_log.h"
#include "internal.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Emit, execute, and download one native query readback request.
 *
 * @param scene the owning scene, used for instance-scoped test controls
 * @param executor retained request executor
 * @param caps capability snapshot
 * @param plan prepared frame plan
 * @param target_width offscreen target width
 * @param target_height offscreen target height
 * @param color_format backend-native color target format, or zero for default RGBA8
 * @param bytes destination readback bytes
 * @param byte_size destination byte count
 * @param out_executed whether the stream executed successfully before download
 * @return true on successful execution and download
 */
bool _dvz_scene_query_execute_readback(
    const DvzScene* scene, DvzSceneRequestExecutor* executor, const DvzCapabilitySnapshot* caps,
    DvzFramePlan* plan, uint32_t target_width, uint32_t target_height, uint32_t color_format,
    uint8_t* bytes, uint32_t byte_size, bool* out_executed)
{
    ANN(executor);
    ANN(caps);
    ANN(bytes);
    ANN(out_executed);
    *out_executed = false;
    if (plan == NULL || executor->runtime == NULL || executor->emitter == NULL || byte_size == 0)
    {
        log_error("scene query readback requires a prepared frame plan and emitter");
        return false;
    }

    DvzDiagnosticReport report = {0};
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    cfg.target_width = target_width > 0 ? target_width : 1;
    cfg.target_height = target_height > 0 ? target_height : 1;
    cfg.color_target_format = color_format;
    DvzDrp2CommandStream* stream =
        dvz_frame_plan_emitter_emit_drp2(executor->emitter, plan, caps, &report, &cfg);
    if (stream == NULL)
    {
        log_error("scene query readback DRP2 emission failed");
        return false;
    }
    uint64_t rb_id = dvz_frame_plan_emitter_object_id(executor->emitter, "_rb");
    bool ok = false;
    if (rb_id == 0)
    {
        log_error("scene query readback plan did not emit the _rb buffer");
    }
    else
    {
        DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(executor->runtime, stream);
        if (!result.ok)
        {
            const DvzDrp2Command* failed = dvz_drp2_stream_get(stream, result.command_index);
            uint64_t failed_id = 0;
            if (failed != NULL)
            {
                if (failed->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
                    failed_id = failed->u.create_render_pipeline.id;
                else if (failed->type == DVZ_DRP2_COMMAND_CREATE_SHADER_MODULE)
                    failed_id = failed->u.create_shader_module.id;
                else if (failed->type == DVZ_DRP2_COMMAND_CREATE_BIND_GROUP_LAYOUT)
                    failed_id = failed->u.create_bind_group_layout.id;
                else if (failed->type == DVZ_DRP2_COMMAND_CREATE_BIND_GROUP)
                    failed_id = failed->u.create_bind_group.id;
                else if (failed->type == DVZ_DRP2_COMMAND_DRAW)
                    failed_id = failed->u.draw.pass_id;
            }
            const char* failed_label =
                failed_id != 0 ? dvz_drp2_stream_label(stream, failed_id) : NULL;
            log_error(
                "scene query readback runtime execution failed (code=%d command=%u type=%d "
                "id=%llu label=%s)",
                (int)result.code, result.command_index, failed != NULL ? (int)failed->type : -1,
                (unsigned long long)failed_id, failed_label != NULL ? failed_label : "");
        }
        else
        {
            *out_executed = true;
            if (scene != NULL && scene->test.force_readback_download_failure)
            {
                log_error("scene query readback buffer download forced to fail");
            }
            else
            {
                ok = dvz_drp2_runtime_download_buffer(executor->runtime, rb_id, 0, byte_size, bytes);
                if (!ok)
                    log_error("scene query readback buffer download failed");
            }
        }
    }
    dvz_drp2_stream_destroy(stream);
    return ok;
}
