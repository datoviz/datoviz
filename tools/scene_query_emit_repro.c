/*
 * Narrow repro for query FramePlan -> runtime DRP2 emission.
 *
 * Build against a configured tree, for example:
 *
 * clang -fsanitize=address,undefined -fno-omit-frame-pointer -g \
 *   -Iinclude -Isrc/common -Isrc/scene -Isrc/scene/frame_plan -Isrc/scene/runtime \
 *   -Isrc/scene/visuals -Isrc/drp2 -Iexternal/cglm/include \
 *   tools/scene_query_emit_repro.c -Lbuild-asan-scene/src -ldatoviz \
 *   -Wl,-rpath,$PWD/build-asan-scene/src -o build-asan-scene/scene_query_emit_repro
 */

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "datoviz/drp2.h"
#include "datoviz/scene.h"
#include "datoviz/vk/enums.h"
#include "frame_plan/emit.h"
#include "frame_plan/frame_plan.h"
#include "visuals/_visual_pipeline.h"



static void copy_label(char* dst, size_t dst_size, const char* src)
{
    assert(dst != NULL);
    assert(src != NULL);
    assert(dst_size > 0);
    size_t len = strlen(src);
    if (len >= dst_size)
        len = dst_size - 1;
    memcpy(dst, src, len);
    dst[len] = '\0';
}



static uint32_t render_node_count(const DvzFramePlan* plan, uint32_t* out_visual_count)
{
    assert(plan != NULL);
    uint32_t render_count = 0;
    uint32_t visual_count = 0;
    for (uint32_t i = 0; i < plan->count; i++)
    {
        if (plan->nodes[i].type != DVZ_FRAME_PLAN_NODE_RENDER)
            continue;
        render_count++;
        visual_count += plan->nodes[i].u.render.visual_count;
    }
    if (out_visual_count != NULL)
        *out_visual_count = visual_count;
    return render_count;
}



int main(void)
{
    DvzFramePlan* plan = dvz_frame_plan("figure.query.point.repro", 1);
    assert(plan != NULL);

    DvzFramePlanUploadMeta upload_meta = {0};
    upload_meta.kind = DVZ_FRAME_PLAN_RESOURCE_KIND_BUFFER;
    upload_meta.visual_type = DVZ_VISUAL_TYPE_POINT;
    upload_meta.visual_index = 0;
    upload_meta.buffer_index = UINT32_MAX;

    assert(dvz_frame_plan_upload(plan, "query0_position", 0, 3 * 3 * sizeof(float), "position"));
    upload_meta.role = DVZ_FRAME_PLAN_RESOURCE_ROLE_POSITION;
    assert(dvz_frame_plan_upload_metadata(plan, &upload_meta));
    assert(dvz_frame_plan_upload(plan, "query0_color", 0, 3 * sizeof(DvzColor), "color"));
    upload_meta.role = DVZ_FRAME_PLAN_RESOURCE_ROLE_COLOR;
    assert(dvz_frame_plan_upload_metadata(plan, &upload_meta));
    assert(dvz_frame_plan_upload(plan, "query0_size", 0, 3 * sizeof(float), "size"));
    upload_meta.role = DVZ_FRAME_PLAN_RESOURCE_ROLE_SIZE;
    assert(dvz_frame_plan_upload_metadata(plan, &upload_meta));

    assert(dvz_frame_plan_render(plan, "panel.query", "target.query", true));
    assert(dvz_frame_plan_render_visual(plan, "query0"));

    DvzFramePlanVisualMeta metadata = {0};
    metadata.visual_type = DVZ_VISUAL_TYPE_POINT;
    metadata.renderable_kind = (uint32_t)DVZ_RENDERABLE_POINT_LIKE;
    metadata.desc_kind = (uint32_t)DVZ_SCENE_VISUAL_DESC_POINT;
    metadata.point_like_kind = (uint32_t)DVZ_SCENE_POINT_LIKE_POINT;
    metadata.visual_index = 0;
    metadata.buffer_index = UINT32_MAX;
    metadata.topology = UINT32_MAX;
    metadata.alpha_mode = DVZ_ALPHA_OPAQUE;
    copy_label(metadata.position_id, sizeof(metadata.position_id), "query0_position");
    copy_label(metadata.color_id, sizeof(metadata.color_id), "query0_color");
    copy_label(metadata.size_id, sizeof(metadata.size_id), "query0_size");
    assert(dvz_frame_plan_render_visual_metadata(plan, &metadata));

    assert(dvz_frame_plan_copy(plan, "target.query", "buf.query", sizeof(uint32_t)));
    assert(dvz_frame_plan_readback(plan, "buf.query", "request.query"));

    uint32_t visual_count_before = 0;
    const uint32_t count_before = plan->count;
    const uint32_t render_count_before = render_node_count(plan, &visual_count_before);
    fprintf(
        stderr, "before count=%u render_count=%u visual_count=%u\n", count_before,
        render_count_before, visual_count_before);
    assert(count_before == 6);
    assert(render_count_before == 1);
    assert(visual_count_before == 1);

    DvzFramePlanEmitter* emitter = dvz_frame_plan_emitter();
    assert(emitter != NULL);
    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    DvzDiagnosticReport report = {0};
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_WGSL;
    emit_cfg.color_target_format = DVZ_FORMAT_R32_UINT;

    DvzDrp2CommandStream* stream =
        dvz_frame_plan_emitter_emit_drp2(emitter, plan, &caps, &report, &emit_cfg);
    fprintf(stderr, "after count=%u diagnostics=%u stream=%p\n", plan->count, report.count, (void*)stream);
    for (uint32_t i = 0; i < report.count; i++)
    {
        const char* message = dvz_diagnostic_report_get(&report, i);
        fprintf(stderr, "diagnostic[%u]=%s\n", i, message != NULL ? message : "<null>");
    }
    assert(stream != NULL);
    assert(plan->count == count_before);
    uint32_t visual_count_after = 0;
    const uint32_t render_count_after = render_node_count(plan, &visual_count_after);
    assert(render_count_after == render_count_before);
    assert(visual_count_after == visual_count_before);

    dvz_drp2_stream_destroy(stream);
    dvz_frame_plan_emitter_destroy(emitter);
    dvz_frame_plan_destroy(plan);
    return 0;
}
