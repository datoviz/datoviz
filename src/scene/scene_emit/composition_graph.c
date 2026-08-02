/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Generic composition graph lowering                                                          */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>

#include <vulkan/vulkan_core.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_scene.h"
#include "frame_plan/internal.h"
#include "scene_emit/panel_render_plan.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define COMPOSITION_GRAPH_MAX_PRODUCT_REALIZATIONS                                                \
    (DVZ_PANEL_COMPOSITION_MAX_TECHNIQUES * DVZ_PANEL_COMPOSITION_MAX_PRODUCTS_PER_TECHNIQUE)



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct
{
    DvzSceneResourceRefKind ref_kind;
    DvzRenderProductId product_id;
    DvzSceneScratchResourceId scratch_id;
    uint32_t graph_resource_index;
} CompositionGraphRealization;



typedef struct
{
    uint32_t resource_count;
    uint32_t pass_count;
    uint32_t realization_count;
    CompositionGraphRealization realizations
        [COMPOSITION_GRAPH_MAX_PRODUCT_REALIZATIONS + DVZ_PANEL_COMPOSITION_MAX_SCRATCH_RESOURCES];
} CompositionGraphDraft;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static bool _composition_graph_report(DvzDiagnosticReport* report, const char* fmt, ...)
{
    ANN(fmt);
    char message[DVZ_SCENE_DIAGNOSTIC_SIZE] = {0};
    va_list args;
    va_start(args, fmt);
    const int written = dvz_vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);
    if (written < 0)
        return false;
    log_error("%s", message);
    if (report != NULL)
        (void)dvz_diagnostic_report_add(report, message);
    return false;
}



static bool
_composition_graph_resource_index(const DvzFramePlan* plan, const char* resource_id, uint32_t* out)
{
    ANN(plan);
    ANN(resource_id);
    ANN(out);
    for (uint32_t i = 0; i < plan->graph_resource_count; i++)
    {
        if (strcmp(plan->graph_resources[i].id, resource_id) == 0)
        {
            *out = i;
            return true;
        }
    }
    return false;
}



static bool _composition_graph_resource(
    DvzFramePlan* plan, const DvzFrameGraphResource* resource, uint32_t* out)
{
    ANN(plan);
    ANN(resource);
    ANN(out);
    if (_composition_graph_resource_index(plan, resource->id, out))
    {
        const DvzFrameGraphResource* existing = &plan->graph_resources[*out];
        const uint32_t existing_samples = existing->sample_count > 0 ? existing->sample_count : 1;
        const uint32_t required_samples = resource->sample_count > 0 ? resource->sample_count : 1;
        return existing->kind == resource->kind && existing->format == resource->format &&
               existing->extent_kind == resource->extent_kind &&
               existing->width == resource->width && existing->height == resource->height &&
               existing->depth == resource->depth && existing_samples == required_samples &&
               strcmp(existing->extent_resource_id, resource->extent_resource_id) == 0 &&
               (existing->usage_flags & resource->usage_flags) == resource->usage_flags &&
               existing->lifetime == resource->lifetime;
    }
    if (!dvz_frame_plan_graph_resource(plan, resource))
        return false;
    *out = plan->graph_resource_count - 1;
    return true;
}



static bool _composition_graph_add_realization(
    CompositionGraphDraft* draft, DvzSceneResourceRefKind ref_kind, DvzRenderProductId product_id,
    DvzSceneScratchResourceId scratch_id, uint32_t graph_resource_index)
{
    ANN(draft);
    const uint32_t capacity =
        (uint32_t)(sizeof(draft->realizations) / sizeof(draft->realizations[0]));
    if (draft->realization_count >= capacity)
        return false;
    draft->realizations[draft->realization_count++] = (CompositionGraphRealization){
        .ref_kind = ref_kind,
        .product_id = product_id,
        .scratch_id = scratch_id,
        .graph_resource_index = graph_resource_index,
    };
    return true;
}



static bool _composition_graph_realization(
    const CompositionGraphDraft* draft, DvzSceneResourceRefKind ref_kind,
    DvzRenderProductId product_id, DvzSceneScratchResourceId scratch_id, uint32_t* out)
{
    ANN(draft);
    ANN(out);
    for (uint32_t i = 0; i < draft->realization_count; i++)
    {
        const CompositionGraphRealization* realization = &draft->realizations[i];
        if (realization->ref_kind == ref_kind &&
            (ref_kind != DVZ_SCENE_RESOURCE_REF_PRODUCT ||
             realization->product_id.value == product_id.value) &&
            (ref_kind != DVZ_SCENE_RESOURCE_REF_SCRATCH ||
             realization->scratch_id.value == scratch_id.value))
        {
            *out = realization->graph_resource_index;
            return true;
        }
    }
    return false;
}



static DvzRenderProductKind _composition_graph_product_kind(
    const DvzPanelCompositionSnapshot* snapshot, DvzRenderProductId product_id)
{
    ANN(snapshot);
    for (uint32_t i = 0; i < snapshot->technique_count; i++)
    {
        const DvzSceneResolvedTechnique* technique = &snapshot->techniques[i];
        for (uint32_t j = 0; j < technique->input_count; j++)
            if (technique->input_ids[j].value == product_id.value)
                return technique->inputs[j];
        for (uint32_t j = 0; j < technique->output_count; j++)
            if (technique->output_ids[j].value == product_id.value)
                return technique->outputs[j];
    }
    return DVZ_RENDER_PRODUCT_NONE;
}



static uint32_t _composition_graph_product_usage(
    const DvzPanelCompositionSnapshot* snapshot, DvzRenderProductId product_id)
{
    ANN(snapshot);
    uint32_t usage = 0;
    for (uint32_t i = 0; i < snapshot->pass_count; i++)
    {
        const DvzSceneResolvedPass* pass = &snapshot->passes[i];
        for (uint32_t j = 0; j < pass->binding_count; j++)
        {
            const DvzSceneWorkBinding* binding = &pass->bindings[j];
            if (binding->ref_kind != DVZ_SCENE_RESOURCE_REF_PRODUCT ||
                binding->product_id.value != product_id.value)
                continue;
            if (binding->usage == DVZ_SCENE_WORK_BINDING_ATTACHMENT)
                usage |= DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT;
            else if (binding->usage == DVZ_SCENE_WORK_BINDING_SAMPLED)
                usage |= DVZ_FRAME_GRAPH_RESOURCE_USAGE_SAMPLED;
        }
    }
    return usage;
}



static bool _composition_graph_realize_product(
    DvzFramePlan* plan, const DvzPanelCompositionSnapshot* snapshot, CompositionGraphDraft* draft,
    DvzRenderProductId product_id, DvzDiagnosticReport* report)
{
    ANN(plan);
    ANN(snapshot);
    ANN(draft);
    uint32_t resource_index = 0;
    if (_composition_graph_realization(
            draft, DVZ_SCENE_RESOURCE_REF_PRODUCT, product_id, (DvzSceneScratchResourceId){0},
            &resource_index))
        return true;

    const DvzRenderProductKind kind = _composition_graph_product_kind(snapshot, product_id);
    DvzFrameGraphResource resource = {0};
    resource.kind = DVZ_FRAME_GRAPH_RESOURCE_TEXTURE;
    resource.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
    resource.sample_count = 1;
    resource.usage_flags = _composition_graph_product_usage(snapshot, product_id);
    resource.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME;
    if (kind == DVZ_RENDER_PRODUCT_SCENE_COLOR)
    {
        dvz_strlcpy(resource.id, "rt", sizeof(resource.id));
        resource.kind = DVZ_FRAME_GRAPH_RESOURCE_EXTERNAL_TARGET;
        resource.usage_flags |= DVZ_FRAME_GRAPH_RESOURCE_USAGE_COPY_SRC;
        resource.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_BORROWED;
    }
    else if (kind == DVZ_RENDER_PRODUCT_AMBIENT_VISIBILITY)
    {
        resource.format = DVZ_FORMAT_R8_UNORM;
        const int written = dvz_snprintf(
            resource.id, sizeof(resource.id), "%s.product.%" PRIu32, snapshot->panel_id,
            product_id.value);
        if (written < 0 || (size_t)written >= sizeof(resource.id))
            return _composition_graph_report(
                report, "panel %s product %" PRIu32 " resource key is truncated",
                snapshot->panel_id, product_id.value);
    }
    else if (kind == DVZ_RENDER_PRODUCT_TRANSPARENT_ACCUMULATION)
    {
        resource.format = DVZ_FORMAT_R16G16B16A16_SFLOAT;
        const int written = dvz_snprintf(
            resource.id, sizeof(resource.id), "%s.product.%" PRIu32, snapshot->panel_id,
            product_id.value);
        if (written < 0 || (size_t)written >= sizeof(resource.id))
            return _composition_graph_report(
                report, "panel %s product %" PRIu32 " resource key is truncated",
                snapshot->panel_id, product_id.value);
    }
    else
    {
        return _composition_graph_report(
            report, "panel %s product %" PRIu32 " kind %u has no physical realization policy",
            snapshot->panel_id, product_id.value, (uint32_t)kind);
    }
    if (resource.usage_flags == 0 ||
        !_composition_graph_resource(plan, &resource, &resource_index) ||
        !_composition_graph_add_realization(
            draft, DVZ_SCENE_RESOURCE_REF_PRODUCT, product_id, (DvzSceneScratchResourceId){0},
            resource_index))
        return _composition_graph_report(
            report, "panel %s product %" PRIu32 " realization failed", snapshot->panel_id,
            product_id.value);
    return true;
}



static bool _composition_graph_realize_scratch(
    DvzFramePlan* plan, const DvzPanelCompositionSnapshot* snapshot, CompositionGraphDraft* draft,
    const DvzSceneScratchResource* scratch, DvzDiagnosticReport* report)
{
    ANN(plan);
    ANN(snapshot);
    ANN(draft);
    ANN(scratch);
    DvzFrameGraphResource resource = {0};
    const int written = dvz_snprintf(
        resource.id, sizeof(resource.id), "%s.scratch.%" PRIu32, snapshot->panel_id,
        scratch->id.value);
    if (written < 0 || (size_t)written >= sizeof(resource.id))
        return _composition_graph_report(
            report, "panel %s scratch %" PRIu32 " resource key is truncated", snapshot->panel_id,
            scratch->id.value);
    resource.kind = DVZ_FRAME_GRAPH_RESOURCE_TEXTURE;
    resource.format = scratch->format;
    resource.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
    resource.sample_count = scratch->sample_count;
    resource.usage_flags = scratch->usage_mask;
    resource.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME;
    uint32_t resource_index = 0;
    if (!_composition_graph_resource(plan, &resource, &resource_index) ||
        !_composition_graph_add_realization(
            draft, DVZ_SCENE_RESOURCE_REF_SCRATCH, (DvzRenderProductId){0}, scratch->id,
            resource_index))
        return _composition_graph_report(
            report, "panel %s scratch %" PRIu32 " realization failed", snapshot->panel_id,
            scratch->id.value);
    return true;
}



static bool _composition_graph_binding_resource(
    const DvzFramePlan* plan, const CompositionGraphDraft* draft,
    const DvzSceneWorkBinding* binding, const char** out)
{
    ANN(plan);
    ANN(draft);
    ANN(binding);
    ANN(out);
    uint32_t resource_index = 0;
    if (!_composition_graph_realization(
            draft, binding->ref_kind, binding->product_id, binding->scratch_id, &resource_index) ||
        resource_index >= plan->graph_resource_count)
        return false;
    *out = plan->graph_resources[resource_index].id;
    return true;
}



static DvzFrameGraphAttachmentLoadOp _composition_graph_load(DvzSceneAttachmentLoad load)
{
    if (load == DVZ_SCENE_ATTACHMENT_LOAD_CLEAR)
        return DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_CLEAR;
    if (load == DVZ_SCENE_ATTACHMENT_LOAD_LOAD)
        return DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_LOAD;
    return DVZ_FRAME_GRAPH_ATTACHMENT_LOAD_DONT_CARE;
}



static DvzFrameGraphAttachmentStoreOp _composition_graph_store(DvzSceneAttachmentStore store)
{
    return store == DVZ_SCENE_ATTACHMENT_STORE_STORE ? DVZ_FRAME_GRAPH_ATTACHMENT_STORE_STORE
                                                     : DVZ_FRAME_GRAPH_ATTACHMENT_STORE_DONT_CARE;
}



static DvzFrameGraphAttachmentAccess
_composition_graph_attachment_access(DvzSceneWorkAccess access)
{
    if (access == DVZ_SCENE_WORK_ACCESS_READ)
        return DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_READ;
    if (access == DVZ_SCENE_WORK_ACCESS_WRITE)
        return DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_WRITE;
    if (access == DVZ_SCENE_WORK_ACCESS_READ_WRITE)
        return DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_READ_WRITE;
    return DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_NONE;
}



static bool _composition_graph_add_attachment(
    DvzFramePlan* plan, const DvzPanelCompositionSnapshot* snapshot, CompositionGraphDraft* draft,
    const DvzSceneResolvedPass* resolved, const DvzSceneWorkBinding* binding,
    DvzFrameGraphPass* pass, DvzDiagnosticReport* report)
{
    ANN(plan);
    ANN(snapshot);
    ANN(draft);
    ANN(resolved);
    ANN(binding);
    ANN(pass);
    const char* resource_id = NULL;
    if (!_composition_graph_binding_resource(plan, draft, binding, &resource_id))
        return _composition_graph_report(
            report, "panel %s pass %u attachment has no realized resource", snapshot->panel_id,
            resolved->id.value);
    if (binding->load == DVZ_SCENE_ATTACHMENT_LOAD_LOAD)
    {
        uint32_t resource_index = 0;
        uint32_t load_source_index = 0;
        if (!_composition_graph_realization(
                draft, binding->ref_kind, binding->product_id, binding->scratch_id,
                &resource_index) ||
            !_composition_graph_realization(
                draft, binding->load_source_ref_kind, binding->load_source_product_id,
                binding->load_source_scratch_id, &load_source_index) ||
            resource_index != load_source_index)
            return _composition_graph_report(
                report, "panel %s pass %u attachment load source is not physically aliased",
                snapshot->panel_id, resolved->id.value);
    }
    DvzFrameGraphAttachment attachment = {0};
    dvz_strlcpy(attachment.resource_id, resource_id, sizeof(attachment.resource_id));
    attachment.load_op = _composition_graph_load(binding->load);
    attachment.store_op = _composition_graph_store(binding->store);
    attachment.access = _composition_graph_attachment_access(binding->access);
    for (uint32_t i = 0; i < 4; i++)
        attachment.clear_color[i] = binding->clear_value[i];
    attachment.clear_depth = binding->clear_value[0];

    if (!binding->depth_attachment && resolved->sample_count > 1)
    {
        if (binding->load == DVZ_SCENE_ATTACHMENT_LOAD_LOAD)
            return _composition_graph_report(
                report, "panel %s pass %u cannot LOAD an undeclared multisample source",
                snapshot->panel_id, resolved->id.value);
        if (resolved->resolve_policy != DVZ_RENDER_PRODUCT_RESOLVE_LINEAR_COLOR)
            return _composition_graph_report(
                report, "panel %s pass %u has unsupported multisample resolve policy",
                snapshot->panel_id, resolved->id.value);
        DvzFrameGraphResource multisample = {0};
        const int written = dvz_snprintf(
            multisample.id, sizeof(multisample.id), "%s.pass.%u.msaa.%u", snapshot->panel_id,
            resolved->id.value, binding->slot);
        if (written < 0 || (size_t)written >= sizeof(multisample.id))
            return _composition_graph_report(
                report, "panel %s pass %u multisample resource key is truncated",
                snapshot->panel_id, resolved->id.value);
        multisample.kind = DVZ_FRAME_GRAPH_RESOURCE_TEXTURE;
        multisample.extent_kind = DVZ_FRAME_GRAPH_EXTENT_FIGURE;
        multisample.sample_count = resolved->sample_count;
        multisample.usage_flags = DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT;
        multisample.lifetime = DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME;
        uint32_t multisample_index = 0;
        if (!_composition_graph_resource(plan, &multisample, &multisample_index))
            return false;
        dvz_strlcpy(
            attachment.resolve_resource_id, attachment.resource_id,
            sizeof(attachment.resolve_resource_id));
        attachment.resolve_mode = VK_RESOLVE_MODE_AVERAGE_BIT;
        dvz_strlcpy(
            attachment.resource_id, plan->graph_resources[multisample_index].id,
            sizeof(attachment.resource_id));
    }
    return binding->depth_attachment ? dvz_frame_graph_pass_depth_attachment(pass, &attachment)
                                     : dvz_frame_graph_pass_color_attachment(pass, &attachment);
}



static bool _composition_graph_lower_pass(
    DvzFramePlan* plan, const DvzPanelCompositionSnapshot* snapshot, CompositionGraphDraft* draft,
    const DvzSceneResolvedPass* resolved, DvzDiagnosticReport* report)
{
    ANN(plan);
    ANN(snapshot);
    ANN(draft);
    ANN(resolved);
    if (resolved->work_class == DVZ_SCENE_WORK_COMPUTE)
        return _composition_graph_report(
            report, "panel %s pass %u compute lowering is not implemented", snapshot->panel_id,
            resolved->id.value);
    DvzFrameGraphPass pass = {0};
    const int written = dvz_snprintf(
        pass.id, sizeof(pass.id), "%s.composition.%u", snapshot->panel_id, resolved->id.value);
    if (written < 0 || (size_t)written >= sizeof(pass.id))
        return _composition_graph_report(
            report, "panel %s pass %u graph key is truncated", snapshot->panel_id,
            resolved->id.value);
    dvz_strlcpy(pass.panel_id, snapshot->panel_id, sizeof(pass.panel_id));
    dvz_snprintf(
        pass.work_label, sizeof(pass.work_label), "provider_%u", (uint32_t)resolved->provider);
    pass.kind = DVZ_FRAME_GRAPH_PASS_RENDER;
    pass.has_composition_pass = true;
    pass.composition_pass_id = resolved->id;
    pass.alpha_to_coverage = resolved->alpha_to_coverage;

    for (uint32_t i = 0; i < resolved->binding_count; i++)
    {
        const DvzSceneWorkBinding* binding = &resolved->bindings[i];
        if (binding->usage == DVZ_SCENE_WORK_BINDING_STORAGE)
            return _composition_graph_report(
                report, "panel %s pass %u storage lowering is not implemented", snapshot->panel_id,
                resolved->id.value);
        if (binding->usage != DVZ_SCENE_WORK_BINDING_SAMPLED)
            continue;
        const char* resource_id = NULL;
        if (!_composition_graph_binding_resource(plan, draft, binding, &resource_id) ||
            !dvz_frame_graph_pass_read(&pass, resource_id, DVZ_FRAME_GRAPH_ACCESS_SAMPLED))
            return _composition_graph_report(
                report, "panel %s pass %u sampled binding lowering failed", snapshot->panel_id,
                resolved->id.value);
    }
    bool color_slots[DVZ_FRAME_PLAN_MAX_GRAPH_COLOR_ATTACHMENTS] = {0};
    uint32_t color_binding_count = 0;
    for (uint32_t i = 0; i < resolved->binding_count; i++)
    {
        const DvzSceneWorkBinding* binding = &resolved->bindings[i];
        if (binding->usage != DVZ_SCENE_WORK_BINDING_ATTACHMENT || binding->depth_attachment)
            continue;
        if (binding->slot >= DVZ_FRAME_PLAN_MAX_GRAPH_COLOR_ATTACHMENTS ||
            color_slots[binding->slot])
            return _composition_graph_report(
                report, "panel %s pass %u has an invalid color attachment slot",
                snapshot->panel_id, resolved->id.value);
        color_slots[binding->slot] = true;
        color_binding_count++;
    }
    for (uint32_t slot = 0; slot < color_binding_count; slot++)
    {
        if (!color_slots[slot])
            return _composition_graph_report(
                report, "panel %s pass %u has a sparse color attachment layout",
                snapshot->panel_id, resolved->id.value);
    }
    for (uint32_t slot = 0; slot < DVZ_FRAME_PLAN_MAX_GRAPH_COLOR_ATTACHMENTS; slot++)
    {
        uint32_t slot_count = 0;
        for (uint32_t i = 0; i < resolved->binding_count; i++)
        {
            const DvzSceneWorkBinding* binding = &resolved->bindings[i];
            if (binding->usage == DVZ_SCENE_WORK_BINDING_ATTACHMENT &&
                !binding->depth_attachment && binding->slot == slot)
            {
                slot_count++;
                if (slot_count > 1 || !_composition_graph_add_attachment(
                                          plan, snapshot, draft, resolved, binding, &pass, report))
                    return false;
            }
        }
    }
    if (pass.color_attachment_count != color_binding_count)
        return _composition_graph_report(
            report, "panel %s pass %u omitted a color attachment", snapshot->panel_id,
            resolved->id.value);
    uint32_t depth_count = 0;
    for (uint32_t i = 0; i < resolved->binding_count; i++)
    {
        const DvzSceneWorkBinding* binding = &resolved->bindings[i];
        if (binding->usage == DVZ_SCENE_WORK_BINDING_ATTACHMENT && binding->depth_attachment)
        {
            depth_count++;
            if (depth_count > 1 || !_composition_graph_add_attachment(
                                       plan, snapshot, draft, resolved, binding, &pass, report))
                return false;
        }
    }
    if (!dvz_frame_plan_graph_pass(plan, &pass))
        return _composition_graph_report(
            report, "panel %s pass %u graph append failed", snapshot->panel_id,
            resolved->id.value);
    return true;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Lower one immutable panel composition into generic physical graph resources and passes.
 *
 * @param plan destination frame plan
 * @param snapshot immutable panel composition
 * @param report optional diagnostic report
 * @return whether the complete composition graph was appended transactionally
 */
bool _scene_panel_composition_lower_graph(
    DvzFramePlan* plan, const DvzPanelCompositionSnapshot* snapshot, DvzDiagnosticReport* report)
{
    ANN(plan);
    ANN(snapshot);
    if (!_frame_plan_composition_validate(snapshot, report))
        return false;
    CompositionGraphDraft draft = {
        .resource_count = plan->graph_resource_count,
        .pass_count = plan->graph_pass_count,
    };
    bool ok = true;
    for (uint32_t i = 0; ok && i < snapshot->scratch_resource_count; i++)
        ok = _composition_graph_realize_scratch(
            plan, snapshot, &draft, &snapshot->scratch_resources[i], report);
    for (uint32_t i = 0; ok && i < snapshot->pass_count; i++)
    {
        const DvzSceneResolvedPass* pass = &snapshot->passes[i];
        for (uint32_t j = 0; ok && j < pass->binding_count; j++)
        {
            const DvzSceneWorkBinding* binding = &pass->bindings[j];
            if (binding->ref_kind == DVZ_SCENE_RESOURCE_REF_PRODUCT)
                ok = _composition_graph_realize_product(
                    plan, snapshot, &draft, binding->product_id, report);
            if (ok && binding->load_source_ref_kind == DVZ_SCENE_RESOURCE_REF_PRODUCT)
                ok = _composition_graph_realize_product(
                    plan, snapshot, &draft, binding->load_source_product_id, report);
        }
    }
    for (uint32_t i = 0; ok && i < snapshot->pass_count; i++)
        ok = _composition_graph_lower_pass(plan, snapshot, &draft, &snapshot->passes[i], report);
    if (ok)
        ok = dvz_frame_plan_graph_validate(plan, report);
    if (!ok)
    {
        plan->graph_resource_count = draft.resource_count;
        plan->graph_pass_count = draft.pass_count;
    }
    return ok;
}
