/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene render contract diagnostics                                                            */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "internal.h"

#include <string.h>

#include <vulkan/vulkan_core.h>

#include "_assertions.h"
#include "shaders/_scene_shader_abi.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Add a diagnostic message if a report was provided.
 *
 * @param report the optional diagnostic report
 * @param message the diagnostic message
 */
void _contract_report(DvzDiagnosticReport* report, const char* message)
{
    if (report != NULL)
        (void)dvz_diagnostic_report_add(report, message);
}



/**
 * Validate technique-specific attachment facts for one pass contract.
 *
 * @param contract the pass contract
 * @param report optional diagnostic report
 * @return whether technique-specific facts are internally consistent
 */
static bool _scene_pass_contract_validate_technique(
    const DvzScenePassContract* contract, DvzDiagnosticReport* report)
{
    ANN(contract);
    bool ok = true;
    const DvzSceneAttachmentUse* attachment = NULL;

    switch (contract->role)
    {
    case DVZ_FRAME_PLAN_RENDER_PASS_VOLUME_OCCLUSION:
        attachment = contract->attachment_count > 0 ? &contract->attachments[0] : NULL;
        if (contract->color_attachment_count != 1 || attachment == NULL ||
            attachment->role != DVZ_SCENE_ATTACHMENT_COLOR ||
            attachment->resource_id[0] == '\0' || attachment->format != DVZ_FORMAT_R32_SFLOAT ||
            !attachment->write || !attachment->clear)
        {
            _contract_report(report, "volume occlusion pass has invalid output attachment");
            ok = false;
        }
        break;

    case DVZ_FRAME_PLAN_RENDER_PASS_SCENE_OCCLUSION:
        attachment = contract->attachment_count > 0 ? &contract->attachments[0] : NULL;
        if (contract->color_attachment_count != 1 || attachment == NULL ||
            attachment->role != DVZ_SCENE_ATTACHMENT_COLOR ||
            attachment->resource_id[0] == '\0' || attachment->format != DVZ_FORMAT_R32_SFLOAT ||
            !attachment->write || !attachment->clear)
        {
            _contract_report(report, "scene occlusion pass has invalid color attachment");
            ok = false;
        }
        attachment = NULL;
        for (uint32_t i = 0; i < contract->attachment_count; i++)
        {
            if (contract->attachments[i].role == DVZ_SCENE_ATTACHMENT_DEPTH)
            {
                attachment = &contract->attachments[i];
                break;
            }
        }
        if (attachment == NULL || attachment->format != DVZ_FORMAT_D32_SFLOAT ||
            attachment->resource_id[0] == '\0' || !attachment->write || !attachment->clear)
        {
            _contract_report(report, "scene occlusion pass has invalid depth attachment");
            ok = false;
        }
        break;

    case DVZ_FRAME_PLAN_RENDER_PASS_TRANSPARENT_ACCUMULATION:
        attachment = contract->attachment_count > 0 ? &contract->attachments[0] : NULL;
        if (attachment == NULL || attachment->format != DVZ_FORMAT_R16G16B16A16_SFLOAT ||
            attachment->sample_count != 1 || attachment->role != DVZ_SCENE_ATTACHMENT_COLOR)
        {
            _contract_report(report, "WBOIT accumulation pass has invalid accumulation target");
            ok = false;
        }
        attachment = contract->attachment_count > 1 ? &contract->attachments[1] : NULL;
        if (attachment == NULL || attachment->format != DVZ_FORMAT_R16_SFLOAT ||
            attachment->sample_count != 1 || attachment->role != DVZ_SCENE_ATTACHMENT_COLOR)
        {
            _contract_report(report, "WBOIT accumulation pass has invalid transmittance target");
            ok = false;
        }
        if (contract->color_attachment_count != 2)
        {
            _contract_report(report, "WBOIT accumulation pass must have two color attachments");
            ok = false;
        }
        if (_contract_needs_depth(contract) && !contract->has_depth_attachment)
        {
            _contract_report(report, "WBOIT accumulation pass is missing required depth");
            ok = false;
        }
        break;

    case DVZ_FRAME_PLAN_RENDER_PASS_WBOIT_RESOLVE:
        if (contract->draw_count != 0 || contract->color_attachment_count != 1 ||
            contract->sampled_read_count != 2 || !contract->needs_wboit_resolve_layout ||
            contract->sampled_texture_binding_count != 2)
        {
            _contract_report(report, "WBOIT resolve pass has invalid attachment shape");
            ok = false;
        }
        break;

    case DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_INIT:
    case DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_ITER:
    {
        uint32_t color_index = 0;
        for (uint32_t i = 0; i < contract->attachment_count; i++)
        {
            attachment = &contract->attachments[i];
            if (attachment->role != DVZ_SCENE_ATTACHMENT_COLOR)
                continue;
            uint32_t expected_format =
                color_index < 2 ? DVZ_FORMAT_R16G16B16A16_SFLOAT : DVZ_FORMAT_R32G32_SFLOAT;
            color_index++;
            if (attachment->format != expected_format)
            {
                _contract_report(report, "depth peel color attachment has invalid format");
                ok = false;
            }
        }
        if (contract->color_attachment_count != 3)
        {
            _contract_report(report, "depth peel raster pass must have three color attachments");
            ok = false;
        }
        if (contract->role == DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_ITER &&
            (contract->sampled_read_count < 1 ||
             !contract->needs_depth_peel_sampled_layout ||
             contract->sampled_texture_binding_count != 1))
        {
            _contract_report(report, "depth peel iteration pass must sample previous bounds");
            ok = false;
        }
        if (_contract_needs_depth(contract) && !contract->has_depth_attachment)
        {
            _contract_report(report, "depth peel raster pass is missing required depth");
            ok = false;
        }
        break;
    }

    case DVZ_FRAME_PLAN_RENDER_PASS_DEPTH_PEEL_COMPOSITE:
        if (contract->draw_count != 0 || contract->color_attachment_count != 1 ||
            contract->sampled_read_count != 2 || !contract->needs_depth_peel_sampled_layout ||
            contract->sampled_texture_binding_count != 2)
        {
            _contract_report(report, "depth peel composite pass has invalid attachment shape");
            ok = false;
        }
        break;

    default:
        break;
    }

    return ok;
}



/**
 * Validate generic invariants for a passive scene pass contract.
 *
 * @param contract the pass contract
 * @param report optional diagnostic report
 * @return whether the contract is internally consistent
 */
bool _scene_pass_contract_validate(
    const DvzScenePassContract* contract, DvzDiagnosticReport* report)
{
    ANN(contract);
    bool ok = true;
    bool needs_depth = false;
    bool samples_depth = false;

    for (uint32_t i = 0; i < contract->draw_count; i++)
    {
        const DvzSceneDrawContract* draw = &contract->draws[i];
        if (!_draw_pass_role_matches(draw))
        {
            _contract_report(report, "draw alpha mode does not match render pass role");
            ok = false;
        }
        if (contract->transparent_blend && draw->depth_write)
        {
            _contract_report(report, "ordinary transparent draw must not write depth");
            ok = false;
        }
        needs_depth = needs_depth || draw->depth_test || draw->depth_write;
        samples_depth = samples_depth || draw->samples_depth;
        if (draw->samples_volume_occlusion)
        {
            const DvzSceneAttachmentUse* use = _contract_sampled_resource_use(
                contract, draw->volume_occlusion_resource_id);
            if (
                draw->volume_occlusion_resource_id[0] == '\0' ||
                draw->volume_occlusion_producer_pass_id[0] == '\0')
            {
                _contract_report(report, "volume-occluded draw is missing typed resource identity");
                ok = false;
            }
            else if (use == NULL)
            {
                _contract_report(
                    report, "volume-occluded draw has no exact volume occlusion read edge");
                ok = false;
            }
            else if (strcmp(use->producer_pass_id, draw->volume_occlusion_producer_pass_id) != 0)
            {
                _contract_report(
                    report, "volume-occluded draw producer pass mismatches contract");
                ok = false;
            }
            if (
                draw->volume_occlusion_bind_set != DVZ_SCENE_SHADER_SET_VISUAL ||
                draw->volume_occlusion_bind_binding != 3)
            {
                _contract_report(report, "volume-occluded draw has invalid typed binding metadata");
                ok = false;
            }
        }
        if (draw->samples_scene_occlusion)
        {
            const DvzSceneAttachmentUse* use =
                _contract_sampled_resource_use(contract, draw->scene_occlusion_resource_id);
            if (
                draw->scene_occlusion_resource_id[0] == '\0' ||
                draw->scene_occlusion_producer_pass_id[0] == '\0')
            {
                _contract_report(report, "scene-occluded draw is missing typed resource identity");
                ok = false;
            }
            else if (use == NULL)
            {
                _contract_report(
                    report, "scene-occluded draw has no exact scene occlusion read edge");
                ok = false;
            }
            else if (strcmp(use->producer_pass_id, draw->scene_occlusion_producer_pass_id) != 0)
            {
                _contract_report(
                    report, "scene-occluded draw producer pass mismatches contract");
                ok = false;
            }
            if (
                (draw->scene_occlusion_bind_set != DVZ_SCENE_SHADER_SET_VISUAL &&
                 draw->scene_occlusion_bind_set != DVZ_SCENE_SHADER_SET_SCENE_OCCLUSION) ||
                draw->scene_occlusion_bind_binding != 0)
            {
                _contract_report(report, "scene-occluded draw has invalid typed binding metadata");
                ok = false;
            }
        }
    }

    if (needs_depth && !_contract_has_depth_attachment(contract))
    {
        _contract_report(report, "depth-capable draw is in a pass without depth attachment");
        ok = false;
    }
    if (samples_depth && !_contract_has_sampled_depth_resource(contract))
    {
        _contract_report(report, "sampled-depth draw has no produced depth resource");
        ok = false;
    }
    ok = _scene_pass_contract_validate_technique(contract, report) && ok;
    return ok;
}
