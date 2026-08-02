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
        return _composition_report(report, "panel %s composition snapshot is not finalized", panel_id);
    if (snapshot->panel_id[0] == '\0')
        return _composition_report(report, "composition snapshot has no panel identity");
    if (snapshot->technique_count > DVZ_PANEL_COMPOSITION_MAX_TECHNIQUES ||
        snapshot->pass_count > DVZ_PANEL_COMPOSITION_MAX_PASSES)
        return _composition_report(report, "panel %s composition snapshot exceeds fixed bounds", panel_id);

    uint32_t present_phase_mask = 0;
    uint64_t product_mask = 0;
    bool dependencies[DVZ_PANEL_COMPOSITION_MAX_TECHNIQUES]
                     [DVZ_PANEL_COMPOSITION_MAX_TECHNIQUES] = {{false}};
    for (uint32_t i = 0; i < snapshot->technique_count; i++)
    {
        const DvzSceneResolvedTechnique* technique = &snapshot->techniques[i];
        if (technique->instance_id.value == 0 ||
            technique->id <= DVZ_SCENE_TECHNIQUE_NONE || technique->version == 0)
            return _composition_report(report, "panel %s has an invalid technique identity", panel_id);
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

    const uint32_t known_phase_mask =
        ((1u << ((uint32_t)DVZ_SCENE_PHASE_QUERY + 1u)) - 1u) &
        ~(1u << (uint32_t)DVZ_SCENE_PHASE_NONE);
    uint32_t prior_phase_mask = 0;
    for (uint32_t i = 0; i < snapshot->technique_count; i++)
    {
        const DvzSceneResolvedTechnique* technique = &snapshot->techniques[i];
        if ((technique->must_follow_phase_mask & ~known_phase_mask) != 0 ||
            (technique->must_follow_phase_mask &
             (1u << (uint32_t)technique->phase)) != 0)
            return _composition_report(
                report, "panel %s technique %u has an invalid must_follow phase constraint",
                panel_id, (uint32_t)technique->id);
        uint32_t expected_missing = technique->required_capability_mask &
                                    ~snapshot->available_capability_mask;
        if (technique->missing_capability_mask != expected_missing)
            return _composition_report(
                report, "panel %s technique %u has inconsistent missing capabilities",
                panel_id, (uint32_t)technique->id);
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
            uint32_t producer_count =
                _composition_product_producer_count(snapshot, id, &producer);
            if (producer_count == 0)
                return _composition_report(
                    report, "panel %s technique %u has no producer for product %u", panel_id,
                    (uint32_t)technique->id, id.value);
            if (producer_count > 1)
                return _composition_report(
                    report, "panel %s product %u has ambiguous producers", panel_id, id.value);
            const DvzSceneResolvedTechnique* producer_technique =
                &snapshot->techniques[producer];
            bool kind_matches = false;
            for (uint32_t k = 0; k < producer_technique->output_count; k++)
            {
                kind_matches = kind_matches ||
                               (producer_technique->output_ids[k].value == id.value &&
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
                dependencies[i][j] = dependencies[i][j] ||
                                     (dependencies[i][k] && dependencies[k][j]);
        }
    }
    for (uint32_t i = 0; i < snapshot->technique_count; i++)
    {
        if (dependencies[i][i])
            return _composition_report(report, "panel %s composition has a product dependency cycle", panel_id);
        for (uint32_t j = 0; j <= i; j++)
        {
            if (i != j && dependencies[i][j])
                return _composition_report(
                    report, "panel %s product dependencies are not topologically ordered",
                    panel_id);
        }
    }
    if (product_mask != snapshot->required_product_mask)
        return _composition_report(report, "panel %s required product mask is inconsistent", panel_id);

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
    DvzFramePlan* plan, const DvzPanelCompositionSnapshot* snapshot,
    DvzDiagnosticReport* report)
{
    ANN(plan);
    ANN(snapshot);
    if (!_frame_plan_composition_validate(snapshot, report))
        return false;
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
const DvzPanelCompositionSnapshot* _frame_plan_composition_get(
    const DvzFramePlan* plan, const char* panel_id)
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
