/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Immutable panel composition snapshots                                                        */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdarg.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_overflow.h"
#include "internal.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static bool _composition_report(DvzDiagnosticReport* report, const char* fmt, ...)
{
    ANN(fmt);
    if (report != NULL)
    {
        char message[DVZ_SCENE_DIAGNOSTIC_SIZE] = {0};
        va_list args;
        va_start(args, fmt);
        int written = dvz_vsnprintf(message, sizeof(message), fmt, args);
        va_end(args);
        if (written >= 0)
            (void)dvz_diagnostic_report_add(report, message);
    }
    return false;
}



static bool _composition_ensure_capacity(DvzFramePlan* plan, uint32_t count)
{
    ANN(plan);
    if (count <= plan->composition_capacity)
        return true;
    uint32_t capacity = plan->composition_capacity > 0
                            ? plan->composition_capacity
                            : DVZ_FRAME_PLAN_INITIAL_COMPOSITION_CAPACITY;
    while (capacity < count)
    {
        if (capacity > UINT32_MAX / 2)
            return false;
        capacity *= 2;
    }
    uint64_t bytes = 0;
    if (_dvz_mul_u64_overflows(capacity, sizeof(DvzPanelCompositionSnapshot), &bytes))
        return false;
    DvzPanelCompositionSnapshot* snapshots =
        (DvzPanelCompositionSnapshot*)dvz_realloc(plan->compositions, bytes);
    if (snapshots == NULL)
        return false;
    plan->compositions = snapshots;
    plan->composition_capacity = capacity;
    return true;
}



static int32_t _composition_technique_index(
    const DvzPanelCompositionSnapshot* snapshot, DvzSceneTechniqueInstanceId id)
{
    ANN(snapshot);
    for (uint32_t i = 0; i < snapshot->technique_count; i++)
    {
        if (snapshot->techniques[i].instance_id.value == id.value)
            return (int32_t)i;
    }
    return -1;
}



static uint32_t _composition_product_producer_count(
    const DvzPanelCompositionSnapshot* snapshot, DvzRenderProductId id,
    uint32_t* out_producer_index)
{
    ANN(snapshot);
    uint32_t count = 0;
    for (uint32_t i = 0; i < snapshot->technique_count; i++)
    {
        const DvzSceneResolvedTechnique* technique = &snapshot->techniques[i];
        for (uint32_t j = 0; j < technique->output_count; j++)
        {
            if (technique->output_ids[j].value == id.value)
            {
                if (out_producer_index != NULL)
                    *out_producer_index = i;
                count++;
            }
        }
    }
    return count;
}



static bool _composition_scratch_contract_valid(const DvzSceneScratchResource* scratch)
{
    ANN(scratch);
    const uint32_t color_sampled =
        DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT | DVZ_FRAME_GRAPH_RESOURCE_USAGE_SAMPLED;
    const uint32_t depth = DVZ_FRAME_GRAPH_RESOURCE_USAGE_DEPTH_ATTACHMENT;
    const uint32_t depth_sampled = depth | DVZ_FRAME_GRAPH_RESOURCE_USAGE_SAMPLED;
    uint32_t format = 0;
    DvzRenderProductFormatClass format_class = DVZ_RENDER_PRODUCT_FORMAT_NONE;
    uint32_t usage = 0;
    DvzSceneScratchScope scope = DVZ_SCENE_SCRATCH_SCOPE_NONE;
    switch (scratch->kind)
    {
    case DVZ_SCENE_SCRATCH_SCENE_OCCLUSION_DEVICE_DEPTH:
    case DVZ_SCENE_SCRATCH_VOLUME_OCCLUSION_DEVICE_DEPTH:
        format = DVZ_FORMAT_R32_SFLOAT;
        format_class = DVZ_RENDER_PRODUCT_FORMAT_SCALAR_FLOAT;
        usage = color_sampled;
        scope = DVZ_SCENE_SCRATCH_SCOPE_PANEL;
        break;
    case DVZ_SCENE_SCRATCH_Z_ONLY_DEPTH:
        format = DVZ_FORMAT_D32_SFLOAT;
        format_class = DVZ_RENDER_PRODUCT_FORMAT_DEPTH_FLOAT;
        usage = depth;
        scope = DVZ_SCENE_SCRATCH_SCOPE_TECHNIQUE;
        break;
    case DVZ_SCENE_SCRATCH_SURFACE_NORMAL_LEGACY:
        format = DVZ_FORMAT_R16G16B16A16_SFLOAT;
        format_class = DVZ_RENDER_PRODUCT_FORMAT_NORMAL_FLOAT;
        usage = color_sampled;
        scope = DVZ_SCENE_SCRATCH_SCOPE_PANEL;
        break;
    case DVZ_SCENE_SCRATCH_SURFACE_DEPTH:
    case DVZ_SCENE_SCRATCH_EDL_DEPTH:
        format = DVZ_FORMAT_D32_SFLOAT;
        format_class = DVZ_RENDER_PRODUCT_FORMAT_DEPTH_FLOAT;
        usage = depth_sampled;
        scope = DVZ_SCENE_SCRATCH_SCOPE_PANEL;
        break;
    case DVZ_SCENE_SCRATCH_FORWARD_DEPTH:
        format = DVZ_FORMAT_D32_SFLOAT;
        format_class = DVZ_RENDER_PRODUCT_FORMAT_DEPTH_FLOAT;
        usage = scratch->usage_mask;
        scope = DVZ_SCENE_SCRATCH_SCOPE_PANEL;
        if (usage != depth && usage != depth_sampled)
            return false;
        break;
    case DVZ_SCENE_SCRATCH_PEEL_FORWARD_DEPTH:
        format = DVZ_FORMAT_D32_SFLOAT;
        format_class = DVZ_RENDER_PRODUCT_FORMAT_DEPTH_FLOAT;
        usage = depth;
        scope = DVZ_SCENE_SCRATCH_SCOPE_PANEL;
        break;
    case DVZ_SCENE_SCRATCH_SSAO_RAW:
        format = DVZ_FORMAT_R8_UNORM;
        format_class = DVZ_RENDER_PRODUCT_FORMAT_SCALAR_FLOAT;
        usage = color_sampled;
        scope = DVZ_SCENE_SCRATCH_SCOPE_TECHNIQUE;
        break;
    case DVZ_SCENE_SCRATCH_WBOIT_WEIGHT:
        format = DVZ_FORMAT_R16_SFLOAT;
        format_class = DVZ_RENDER_PRODUCT_FORMAT_SCALAR_FLOAT;
        usage = color_sampled;
        scope = DVZ_SCENE_SCRATCH_SCOPE_TECHNIQUE;
        break;
    case DVZ_SCENE_SCRATCH_PEEL_BACK_ACCUM:
        format = DVZ_FORMAT_R16G16B16A16_SFLOAT;
        format_class = DVZ_RENDER_PRODUCT_FORMAT_LINEAR_COLOR;
        usage = color_sampled;
        scope = DVZ_SCENE_SCRATCH_SCOPE_TECHNIQUE;
        break;
    case DVZ_SCENE_SCRATCH_PEEL_DEPTH_PING:
    case DVZ_SCENE_SCRATCH_PEEL_DEPTH_PONG:
        format = DVZ_FORMAT_R32G32_SFLOAT;
        format_class = DVZ_RENDER_PRODUCT_FORMAT_VECTOR2_FLOAT;
        usage = color_sampled;
        scope = DVZ_SCENE_SCRATCH_SCOPE_TECHNIQUE;
        break;
    case DVZ_SCENE_SCRATCH_EDL_COLOR:
        format = DVZ_FORMAT_R8G8B8A8_UNORM;
        format_class = DVZ_RENDER_PRODUCT_FORMAT_PRESENTATION_COLOR;
        usage = color_sampled;
        scope = DVZ_SCENE_SCRATCH_SCOPE_PANEL;
        break;
    default:
        return false;
    }
    if (scratch->format != format || scratch->format_class != format_class ||
        scratch->usage_mask != usage || scratch->scope != scope ||
        scratch->extent_policy != DVZ_RENDER_PRODUCT_EXTENT_TARGET_RELATIVE ||
        scratch->lifetime != (scope == DVZ_SCENE_SCRATCH_SCOPE_PANEL
                                  ? DVZ_SCENE_SCRATCH_LIFETIME_FRAME
                                  : DVZ_SCENE_SCRATCH_LIFETIME_TECHNIQUE))
        return false;
    if (scratch->sample_count > 1)
        return scratch->kind == DVZ_SCENE_SCRATCH_FORWARD_DEPTH &&
               scratch->sample_domain == DVZ_RENDER_PRODUCT_SAMPLES_MULTISAMPLE;
    return scratch->sample_count == 1 &&
           scratch->sample_domain == DVZ_RENDER_PRODUCT_SAMPLES_SINGLE;
}



static uint64_t _composition_hash_u32(uint64_t hash, uint32_t value)
{
    for (uint32_t i = 0; i < sizeof(value); i++)
    {
        hash ^= (value >> (8u * i)) & 0xffu;
        hash *= UINT64_C(1099511628211);
    }
    return hash;
}



static uint64_t _composition_hash_u64(uint64_t hash, uint64_t value)
{
    for (uint32_t i = 0; i < sizeof(value); i++)
    {
        hash ^= (value >> (8u * i)) & 0xffu;
        hash *= UINT64_C(1099511628211);
    }
    return hash;
}



uint64_t _frame_plan_composition_work_fingerprint(const DvzPanelCompositionSnapshot* snapshot)
{
    ANN(snapshot);
    uint64_t hash = UINT64_C(1469598103934665603);
#define HASH32(_value) hash = _composition_hash_u32(hash, (uint32_t)(_value))
#define HASH64(_value) hash = _composition_hash_u64(hash, (uint64_t)(_value))
    HASH32(snapshot->scratch_resource_count);
    for (uint32_t i = 0; i < snapshot->scratch_resource_count; i++)
    {
        const DvzSceneScratchResource* scratch = &snapshot->scratch_resources[i];
        HASH32(scratch->id.value);
        HASH32(scratch->technique_instance_id.value);
        HASH32(scratch->scope);
        HASH32(scratch->kind);
        HASH32(scratch->format);
        HASH32(scratch->format_class);
        HASH32(scratch->extent_policy);
        HASH32(scratch->sample_domain);
        HASH32(scratch->sample_count);
        HASH32(scratch->usage_mask);
        HASH32(scratch->lifetime);
    }
    HASH32(snapshot->pass_count);
    for (uint32_t i = 0; i < snapshot->pass_count; i++)
    {
        const DvzSceneResolvedPass* pass = &snapshot->passes[i];
        HASH32(pass->id.value);
        HASH32(pass->technique_instance_id.value);
        HASH32(pass->technique_id);
        HASH32(pass->phase);
        HASH32(pass->role);
        HASH32(pass->ordinal);
        HASH32(pass->authored_order_begin);
        HASH32(pass->authored_order_end);
        HASH32(pass->work_index);
        HASH32(pass->work_class);
        HASH32(pass->provider);
        HASH32(pass->coordinate_space);
        HASH32(pass->viewport_panel_local);
        HASH32(pass->scissor_panel_local);
        HASH32(pass->sample_count);
        HASH32(pass->resolve_policy);
        HASH32(pass->alpha_to_coverage);
        HASH32(pass->visual_layer_filter);
        HASH32(pass->visual_order_begin);
        HASH32(pass->visual_order_end);
        HASH32(pass->dispatch_x);
        HASH32(pass->dispatch_y);
        HASH32(pass->dispatch_z);
        HASH32(pass->legacy_transition);
        HASH32(pass->unrealized_product_count);
        for (uint32_t j = 0; j < pass->unrealized_product_count; j++)
            HASH32(pass->unrealized_product_ids[j].value);
        HASH32(pass->binding_count);
        for (uint32_t j = 0; j < pass->binding_count; j++)
        {
            const DvzSceneWorkBinding* binding = &pass->bindings[j];
            HASH32(binding->ref_kind);
            HASH32(binding->product_id.value);
            HASH32(binding->scratch_id.value);
            HASH32(binding->usage);
            HASH32(binding->access);
            HASH32(binding->slot);
            HASH32(binding->set);
            HASH32(binding->binding);
            HASH32(binding->load);
            HASH32(binding->store);
            HASH32(binding->clear);
            HASH32(binding->clear_value_kind);
            HASH32(binding->depth_attachment);
            for (uint32_t k = 0; k < 4; k++)
            {
                uint32_t bits = 0;
                memcpy(&bits, &binding->clear_value[k], sizeof(bits));
                HASH32(bits);
            }
            HASH32(binding->load_source_ref_kind);
            HASH32(binding->load_source_product_id.value);
            HASH32(binding->load_source_scratch_id.value);
        }
        HASH32(pass->auxiliary_binding_count);
        for (uint32_t j = 0; j < pass->auxiliary_binding_count; j++)
        {
            const DvzSceneAuxiliaryBinding* binding = &pass->auxiliary_bindings[j];
            HASH32(binding->kind);
            HASH32(binding->upload_node_index);
            HASH32(binding->set);
            HASH32(binding->binding);
            HASH32(binding->byte_offset);
            HASH32(binding->byte_size);
        }
    }
#undef HASH32
#undef HASH64
    return hash != 0 ? hash : UINT64_C(1);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Validate one immutable panel composition snapshot independently of retained scene state.
 *
 * @param snapshot the composition snapshot
 * @param report optional diagnostic report
 * @return whether the snapshot is complete and internally coherent
 */
bool _frame_plan_composition_validate(
    const DvzPanelCompositionSnapshot* snapshot, DvzDiagnosticReport* report)
{
    ANN(snapshot);
    const char* panel_id = snapshot->panel_id[0] != '\0' ? snapshot->panel_id : "?";
    if (!snapshot->valid)
        return _composition_report(
            report, "panel %s composition snapshot is not finalized", panel_id);
    if (snapshot->panel_id[0] == '\0')
        return _composition_report(report, "composition snapshot has no panel identity");
    if (snapshot->technique_count > DVZ_PANEL_COMPOSITION_MAX_TECHNIQUES ||
        snapshot->pass_count > DVZ_PANEL_COMPOSITION_MAX_PASSES ||
        snapshot->scratch_resource_count > DVZ_PANEL_COMPOSITION_MAX_SCRATCH_RESOURCES)
        return _composition_report(
            report, "panel %s composition snapshot exceeds fixed bounds", panel_id);
    for (uint32_t i = 0; i < snapshot->scratch_resource_count; i++)
    {
        const DvzSceneScratchResource* scratch = &snapshot->scratch_resources[i];
        const bool panel_scoped = scratch->scope == DVZ_SCENE_SCRATCH_SCOPE_PANEL;
        const bool technique_scoped = scratch->scope == DVZ_SCENE_SCRATCH_SCOPE_TECHNIQUE;
        if (scratch->id.value != i + 1 || (!panel_scoped && !technique_scoped) ||
            (panel_scoped && scratch->technique_instance_id.value != 0) ||
            (technique_scoped && scratch->technique_instance_id.value == 0) ||
            scratch->kind == DVZ_SCENE_SCRATCH_NONE || scratch->format == 0 ||
            scratch->format_class == DVZ_RENDER_PRODUCT_FORMAT_NONE ||
            scratch->extent_policy == DVZ_RENDER_PRODUCT_EXTENT_NONE ||
            scratch->sample_domain == DVZ_RENDER_PRODUCT_SAMPLES_NONE ||
            scratch->sample_count == 0 || scratch->usage_mask == 0 ||
            scratch->lifetime == DVZ_SCENE_SCRATCH_LIFETIME_NONE)
            return _composition_report(
                report, "panel %s has an invalid typed scratch resource", panel_id);
        if (!_composition_scratch_contract_valid(scratch))
            return _composition_report(
                report, "panel %s scratch resource drifts from its physical contract", panel_id);
        if (technique_scoped &&
            _composition_technique_index(snapshot, scratch->technique_instance_id) < 0)
            return _composition_report(
                report, "panel %s scratch resource has no technique owner", panel_id);
    }

    uint32_t present_phase_mask = 0;
    uint64_t product_mask = 0;
    bool dependencies[DVZ_PANEL_COMPOSITION_MAX_TECHNIQUES][DVZ_PANEL_COMPOSITION_MAX_TECHNIQUES] =
        {{false}};
    for (uint32_t i = 0; i < snapshot->technique_count; i++)
    {
        const DvzSceneResolvedTechnique* technique = &snapshot->techniques[i];
        if (technique->instance_id.value == 0 || technique->id <= DVZ_SCENE_TECHNIQUE_NONE ||
            technique->version == 0)
            return _composition_report(
                report, "panel %s has an invalid technique identity", panel_id);
        if (technique->phase <= DVZ_SCENE_PHASE_NONE || technique->phase > DVZ_SCENE_PHASE_QUERY)
            return _composition_report(
                report, "panel %s technique %u has an invalid phase", panel_id,
                (uint32_t)technique->id);
        if (i > 0 && technique->phase < snapshot->techniques[i - 1].phase)
            return _composition_report(
                report, "panel %s technique %u creates a phase-order cycle", panel_id,
                (uint32_t)technique->id);
        for (uint32_t j = 0; j < i; j++)
        {
            if (snapshot->techniques[j].instance_id.value == technique->instance_id.value)
                return _composition_report(
                    report, "panel %s has duplicate technique instance identity %u", panel_id,
                    technique->instance_id.value);
        }
        if (technique->input_count > DVZ_PANEL_COMPOSITION_MAX_PRODUCTS_PER_TECHNIQUE ||
            technique->output_count > DVZ_PANEL_COMPOSITION_MAX_PRODUCTS_PER_TECHNIQUE)
            return _composition_report(
                report, "panel %s technique %u exceeds product-use bounds", panel_id,
                (uint32_t)technique->id);
        present_phase_mask |= 1u << (uint32_t)technique->phase;
        product_mask |= technique->input_product_mask | technique->output_product_mask;
    }

    const uint32_t known_phase_mask = ((1u << ((uint32_t)DVZ_SCENE_PHASE_QUERY + 1u)) - 1u) &
                                      ~(1u << (uint32_t)DVZ_SCENE_PHASE_NONE);
    uint32_t prior_phase_mask = 0;
    for (uint32_t i = 0; i < snapshot->technique_count; i++)
    {
        const DvzSceneResolvedTechnique* technique = &snapshot->techniques[i];
        if ((technique->must_follow_phase_mask & ~known_phase_mask) != 0 ||
            (technique->must_follow_phase_mask & (1u << (uint32_t)technique->phase)) != 0)
            return _composition_report(
                report, "panel %s technique %u has an invalid must_follow phase constraint",
                panel_id, (uint32_t)technique->id);
        uint32_t expected_missing =
            technique->required_capability_mask & ~snapshot->available_capability_mask;
        if (technique->missing_capability_mask != expected_missing)
            return _composition_report(
                report, "panel %s technique %u has inconsistent missing capabilities", panel_id,
                (uint32_t)technique->id);
        if (expected_missing != 0 &&
            technique->fallback != DVZ_SCENE_TECHNIQUE_FALLBACK_LEGACY_TRANSITION)
            return _composition_report(
                report, "panel %s technique %u has no capability fallback", panel_id,
                (uint32_t)technique->id);
        uint32_t required_present = technique->must_follow_phase_mask & present_phase_mask;
        if ((required_present & prior_phase_mask) != required_present)
            return _composition_report(
                report, "panel %s technique %u violates its must_follow phase constraint",
                panel_id, (uint32_t)technique->id);
        prior_phase_mask |= 1u << (uint32_t)technique->phase;

        uint64_t input_mask = 0;
        for (uint32_t j = 0; j < technique->input_count; j++)
        {
            DvzRenderProductKind kind = technique->inputs[j];
            DvzRenderProductId id = technique->input_ids[j];
            if (kind <= DVZ_RENDER_PRODUCT_NONE || kind > DVZ_RENDER_PRODUCT_PRESENTATION_COLOR ||
                id.value == 0)
                return _composition_report(
                    report, "panel %s technique %u has an invalid input product identity",
                    panel_id, (uint32_t)technique->id);
            input_mask |= UINT64_C(1) << (uint32_t)kind;
            uint32_t producer = UINT32_MAX;
            uint32_t producer_count = _composition_product_producer_count(snapshot, id, &producer);
            if (producer_count == 0)
                return _composition_report(
                    report, "panel %s technique %u has no producer for product %u", panel_id,
                    (uint32_t)technique->id, id.value);
            if (producer_count > 1)
                return _composition_report(
                    report, "panel %s product %u has ambiguous producers", panel_id, id.value);
            const DvzSceneResolvedTechnique* producer_technique = &snapshot->techniques[producer];
            bool kind_matches = false;
            for (uint32_t k = 0; k < producer_technique->output_count; k++)
            {
                kind_matches =
                    kind_matches || (producer_technique->output_ids[k].value == id.value &&
                                     producer_technique->outputs[k] == kind);
            }
            if (!kind_matches)
                return _composition_report(
                    report, "panel %s product %u producer has an incompatible kind", panel_id,
                    id.value);
            dependencies[producer][i] = true;
        }
        if (input_mask != technique->input_product_mask)
            return _composition_report(
                report, "panel %s technique %u input product mask is inconsistent", panel_id,
                (uint32_t)technique->id);

        uint64_t output_mask = 0;
        for (uint32_t j = 0; j < technique->output_count; j++)
        {
            DvzRenderProductKind kind = technique->outputs[j];
            DvzRenderProductId id = technique->output_ids[j];
            if (kind <= DVZ_RENDER_PRODUCT_NONE || kind > DVZ_RENDER_PRODUCT_PRESENTATION_COLOR ||
                id.value == 0)
                return _composition_report(
                    report, "panel %s technique %u has an invalid output product identity",
                    panel_id, (uint32_t)technique->id);
            output_mask |= UINT64_C(1) << (uint32_t)kind;
        }
        if (output_mask != technique->output_product_mask)
            return _composition_report(
                report, "panel %s technique %u output product mask is inconsistent", panel_id,
                (uint32_t)technique->id);
    }

    for (uint32_t k = 0; k < snapshot->technique_count; k++)
    {
        for (uint32_t i = 0; i < snapshot->technique_count; i++)
        {
            for (uint32_t j = 0; j < snapshot->technique_count; j++)
                dependencies[i][j] =
                    dependencies[i][j] || (dependencies[i][k] && dependencies[k][j]);
        }
    }
    for (uint32_t i = 0; i < snapshot->technique_count; i++)
    {
        if (dependencies[i][i])
            return _composition_report(
                report, "panel %s composition has a product dependency cycle", panel_id);
        for (uint32_t j = 0; j <= i; j++)
        {
            if (i != j && dependencies[i][j])
                return _composition_report(
                    report, "panel %s product dependencies are not topologically ordered",
                    panel_id);
        }
    }
    if (product_mask != snapshot->required_product_mask)
        return _composition_report(
            report, "panel %s required product mask is inconsistent", panel_id);

    for (uint32_t i = 0; i < snapshot->pass_count; i++)
    {
        const DvzSceneResolvedPass* pass = &snapshot->passes[i];
        if (pass->id.value == 0)
            return _composition_report(report, "panel %s has an invalid pass identity", panel_id);
        for (uint32_t j = 0; j < i; j++)
        {
            if (snapshot->passes[j].id.value == pass->id.value)
                return _composition_report(
                    report, "panel %s has duplicate pass identity %u", panel_id, pass->id.value);
            if (snapshot->passes[j].role == pass->role &&
                snapshot->passes[j].ordinal == pass->ordinal)
                return _composition_report(
                    report, "panel %s has duplicate pass role/ordinal identity", panel_id);
        }
        int32_t technique_index =
            _composition_technique_index(snapshot, pass->technique_instance_id);
        if (technique_index < 0 ||
            snapshot->techniques[technique_index].id != pass->technique_id ||
            snapshot->techniques[technique_index].phase != pass->phase)
            return _composition_report(
                report, "panel %s pass %u has no matching technique phase", panel_id,
                pass->id.value);
        if (pass->authored_order_begin != UINT32_MAX &&
            pass->authored_order_end < pass->authored_order_begin)
            return _composition_report(
                report, "panel %s pass %u has an invalid authored-order range", panel_id,
                pass->id.value);
        if (pass->provider <= DVZ_SCENE_WORK_PROVIDER_NONE ||
            pass->provider > DVZ_SCENE_WORK_PROVIDER_PRESENTATION)
            return _composition_report(
                report, "panel %s pass %u has an unknown work provider", panel_id, pass->id.value);
        if (pass->work_class <= DVZ_SCENE_WORK_NONE ||
            pass->coordinate_space != DVZ_RENDER_PRODUCT_COORDINATES_FRAMEBUFFER_PIXEL ||
            pass->viewport_panel_local || pass->scissor_panel_local || pass->sample_count == 0 ||
            (pass->binding_count == 0 && pass->unrealized_product_count == 0) ||
            pass->binding_count > DVZ_PANEL_COMPOSITION_MAX_WORK_BINDINGS)
            return _composition_report(
                report, "panel %s pass %u has invalid declarative work", panel_id, pass->id.value);
        if (pass->unrealized_product_count > DVZ_PANEL_COMPOSITION_MAX_UNREALIZED_PRODUCTS ||
            (pass->unrealized_product_count != 0 && !pass->legacy_transition))
            return _composition_report(
                report, "panel %s pass %u has unapproved legacy work omission", panel_id,
                pass->id.value);
        for (uint32_t k = 0; k < pass->unrealized_product_count; k++)
        {
            const DvzRenderProductId id = pass->unrealized_product_ids[k];
            bool declared = false;
            for (uint32_t j = 0; j < snapshot->techniques[technique_index].input_count; j++)
                declared = declared ||
                           snapshot->techniques[technique_index].input_ids[j].value == id.value;
            for (uint32_t j = 0; j < snapshot->techniques[technique_index].output_count; j++)
                declared = declared ||
                           snapshot->techniques[technique_index].output_ids[j].value == id.value;
            for (uint32_t j = 0; j < k; j++)
                if (pass->unrealized_product_ids[j].value == id.value)
                    return _composition_report(
                        report, "panel %s pass %u duplicates an unrealized product id", panel_id,
                        pass->id.value);
            if (id.value == 0 || !declared)
                return _composition_report(
                    report, "panel %s pass %u omits an undeclared product id", panel_id,
                    pass->id.value);
        }
        for (uint32_t k = 0; k < pass->binding_count; k++)
        {
            const DvzSceneWorkBinding* binding = &pass->bindings[k];
            if (binding->ref_kind == DVZ_SCENE_RESOURCE_REF_PRODUCT)
            {
                if (binding->product_id.value == 0 || binding->scratch_id.value != 0)
                    return _composition_report(
                        report, "panel %s pass %u has invalid product binding", panel_id,
                        pass->id.value);
                bool input = false;
                bool output = false;
                for (uint32_t j = 0; j < snapshot->techniques[technique_index].input_count; j++)
                    input = input || snapshot->techniques[technique_index].input_ids[j].value ==
                                         binding->product_id.value;
                for (uint32_t j = 0; j < snapshot->techniques[technique_index].output_count; j++)
                    output = output || snapshot->techniques[technique_index].output_ids[j].value ==
                                           binding->product_id.value;
                if ((!input && !output) ||
                    (binding->usage == DVZ_SCENE_WORK_BINDING_ATTACHMENT && !output))
                    return _composition_report(
                        report, "panel %s pass %u binds a product outside its technique", panel_id,
                        pass->id.value);
                if (binding->usage == DVZ_SCENE_WORK_BINDING_ATTACHMENT &&
                    binding->depth_attachment)
                    return _composition_report(
                        report, "panel %s pass %u misclassifies a semantic product attachment",
                        panel_id, pass->id.value);
            }
            else if (binding->ref_kind == DVZ_SCENE_RESOURCE_REF_SCRATCH)
            {
                if (binding->scratch_id.value == 0 ||
                    binding->scratch_id.value > snapshot->scratch_resource_count ||
                    binding->product_id.value != 0)
                    return _composition_report(
                        report, "panel %s pass %u has invalid scratch binding", panel_id,
                        pass->id.value);
                const DvzSceneScratchResource* scratch =
                    &snapshot->scratch_resources[binding->scratch_id.value - 1];
                if (scratch->scope == DVZ_SCENE_SCRATCH_SCOPE_TECHNIQUE &&
                    scratch->technique_instance_id.value != pass->technique_instance_id.value)
                    return _composition_report(
                        report, "panel %s pass %u binds another technique's scratch resource",
                        panel_id, pass->id.value);
                const uint32_t required_usage =
                    binding->usage == DVZ_SCENE_WORK_BINDING_ATTACHMENT
                        ? (binding->depth_attachment
                               ? DVZ_FRAME_GRAPH_RESOURCE_USAGE_DEPTH_ATTACHMENT
                               : DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT)
                        : (binding->usage == DVZ_SCENE_WORK_BINDING_SAMPLED
                               ? DVZ_FRAME_GRAPH_RESOURCE_USAGE_SAMPLED
                               : 0);
                if (required_usage == 0 ||
                    (scratch->usage_mask & required_usage) != required_usage ||
                    (binding->usage == DVZ_SCENE_WORK_BINDING_ATTACHMENT &&
                     binding->depth_attachment !=
                         (scratch->format_class == DVZ_RENDER_PRODUCT_FORMAT_DEPTH_FLOAT)))
                    return _composition_report(
                        report, "panel %s pass %u has an incompatible scratch access", panel_id,
                        pass->id.value);
            }
            else
                return _composition_report(
                    report, "panel %s pass %u has undeclared work binding reference", panel_id,
                    pass->id.value);
            if (binding->usage == DVZ_SCENE_WORK_BINDING_NONE ||
                binding->access == DVZ_SCENE_WORK_ACCESS_NONE)
                return _composition_report(
                    report, "panel %s pass %u has incomplete work binding", panel_id,
                    pass->id.value);
            for (uint32_t j = 0; j < 4; j++)
                if (!isfinite(binding->clear_value[j]))
                    return _composition_report(
                        report, "panel %s pass %u has a non-finite clear value", panel_id,
                        pass->id.value);
            if (binding->usage == DVZ_SCENE_WORK_BINDING_ATTACHMENT)
            {
                if (binding->set != UINT32_MAX || binding->binding != UINT32_MAX ||
                    binding->depth_attachment != (binding->slot == UINT32_MAX) ||
                    binding->load == DVZ_SCENE_ATTACHMENT_LOAD_NONE ||
                    binding->store == DVZ_SCENE_ATTACHMENT_STORE_NONE ||
                    binding->clear != (binding->load == DVZ_SCENE_ATTACHMENT_LOAD_CLEAR) ||
                    (binding->clear && binding->clear_value_kind == DVZ_SCENE_CLEAR_VALUE_NONE) ||
                    (!binding->clear && binding->clear_value_kind != DVZ_SCENE_CLEAR_VALUE_NONE) ||
                    (binding->load == DVZ_SCENE_ATTACHMENT_LOAD_LOAD &&
                     binding->access == DVZ_SCENE_WORK_ACCESS_WRITE))
                    return _composition_report(
                        report, "panel %s pass %u has an invalid attachment contract", panel_id,
                        pass->id.value);
                if (binding->load == DVZ_SCENE_ATTACHMENT_LOAD_LOAD)
                {
                    if (binding->load_source_ref_kind != binding->ref_kind ||
                        (binding->ref_kind == DVZ_SCENE_RESOURCE_REF_PRODUCT &&
                         binding->load_source_product_id.value == 0) ||
                        (binding->ref_kind == DVZ_SCENE_RESOURCE_REF_SCRATCH &&
                         binding->load_source_scratch_id.value == 0))
                        return _composition_report(
                            report, "panel %s pass %u lacks an attachment load source", panel_id,
                            pass->id.value);
                }
                else if (binding->load_source_ref_kind != DVZ_SCENE_RESOURCE_REF_NONE)
                    return _composition_report(
                        report, "panel %s pass %u has a spurious attachment load source", panel_id,
                        pass->id.value);
            }
            else if (binding->usage == DVZ_SCENE_WORK_BINDING_SAMPLED)
            {
                const bool provider_owned =
                    binding->set == UINT32_MAX && binding->binding == UINT32_MAX;
                const bool concrete = binding->set != UINT32_MAX && binding->binding != UINT32_MAX;
                if (binding->slot != UINT32_MAX || (!provider_owned && !concrete) ||
                    binding->load != DVZ_SCENE_ATTACHMENT_LOAD_NONE ||
                    binding->store != DVZ_SCENE_ATTACHMENT_STORE_NONE || binding->clear ||
                    binding->clear_value_kind != DVZ_SCENE_CLEAR_VALUE_NONE ||
                    binding->depth_attachment ||
                    binding->load_source_ref_kind != DVZ_SCENE_RESOURCE_REF_NONE)
                    return _composition_report(
                        report, "panel %s pass %u has an invalid sampled binding contract",
                        panel_id, pass->id.value);
            }
        }
        if (pass->auxiliary_binding_count > 2)
            return _composition_report(
                report, "panel %s pass %u has too many auxiliary bindings", panel_id,
                pass->id.value);
        for (uint32_t k = 0; k < pass->auxiliary_binding_count; k++)
        {
            const DvzSceneAuxiliaryBinding* binding = &pass->auxiliary_bindings[k];
            const bool edl = pass->provider == DVZ_SCENE_WORK_PROVIDER_EDL &&
                             binding->kind == DVZ_SCENE_AUXILIARY_EDL_PARAMS;
            const bool ssao =
                (pass->provider == DVZ_SCENE_WORK_PROVIDER_SSAO ||
                 pass->provider == DVZ_SCENE_WORK_PROVIDER_SSAO_BLUR ||
                 pass->provider == DVZ_SCENE_WORK_PROVIDER_AMBIENT_COMPOSITE) &&
                binding->kind == DVZ_SCENE_AUXILIARY_SSAO_PARAMS;
            if ((!edl && !ssao) || binding->set == UINT32_MAX ||
                binding->binding == UINT32_MAX || binding->byte_size == 0)
                return _composition_report(
                    report, "panel %s pass %u has an invalid auxiliary binding", panel_id,
                    pass->id.value);
        }
    }
    return _scene_panel_composition_contract_validate(snapshot, report);
}



/**
 * Persist one validated panel composition snapshot in a FramePlan.
 *
 * @param plan the destination FramePlan
 * @param snapshot the finalized immutable snapshot
 * @param report optional diagnostic report
 * @return whether the snapshot was persisted
 */
bool _frame_plan_composition_append(
    DvzFramePlan* plan, const DvzPanelCompositionSnapshot* snapshot, DvzDiagnosticReport* report)
{
    ANN(plan);
    ANN(snapshot);
    if (!_frame_plan_composition_validate(snapshot, report))
        return false;
    for (uint32_t i = 0; i < snapshot->pass_count; i++)
    {
        const DvzSceneResolvedPass* pass = &snapshot->passes[i];
        for (uint32_t j = 0; j < pass->auxiliary_binding_count; j++)
        {
            const DvzSceneAuxiliaryBinding* binding = &pass->auxiliary_bindings[j];
            if (binding->upload_node_index >= plan->count ||
                plan->nodes[binding->upload_node_index].type != DVZ_FRAME_PLAN_NODE_UPLOAD ||
                binding->byte_offset + binding->byte_size < binding->byte_offset ||
                binding->byte_offset + binding->byte_size >
                    plan->nodes[binding->upload_node_index].u.upload.byte_size)
                return _composition_report(
                    report, "panel %s pass %u has an invalid auxiliary upload", snapshot->panel_id,
                    pass->id.value);
        }
    }
    if (_frame_plan_composition_get(plan, snapshot->panel_id) != NULL)
        return _composition_report(
            report, "panel %s has duplicate composition snapshot identity", snapshot->panel_id);
    if (!_composition_ensure_capacity(plan, plan->composition_count + 1))
        return _composition_report(
            report, "panel %s composition snapshot allocation failed", snapshot->panel_id);
    plan->compositions[plan->composition_count++] = *snapshot;
    return true;
}



/**
 * Return the immutable composition snapshot for one panel.
 *
 * @param plan the FramePlan
 * @param panel_id the panel identity
 * @return the matching snapshot, or NULL when absent
 */
const DvzPanelCompositionSnapshot*
_frame_plan_composition_get(const DvzFramePlan* plan, const char* panel_id)
{
    ANN(plan);
    ANN(panel_id);
    for (uint32_t i = 0; i < plan->composition_count; i++)
    {
        if (strcmp(plan->compositions[i].panel_id, panel_id) == 0)
            return &plan->compositions[i];
    }
    return NULL;
}
