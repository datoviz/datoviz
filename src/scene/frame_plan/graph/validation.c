/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene FramePlan graph validation                                                             */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <stdint.h>
#include <string.h>

#include "_assertions.h"
#include "frame_plan/internal.h"
#include "internal.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Validate one declared graph resource access.
 *
 * @param plan the FramePlan
 * @param access graph access descriptor
 * @param pass_index owning pass index
 * @param report optional diagnostic report
 * @return whether the access is valid
 */
static bool _graph_validate_access(
    const DvzFramePlan* plan, const DvzFrameGraphAccess* access, uint32_t pass_index,
    DvzDiagnosticReport* report)
{
    ANN(plan);
    ANN(access);
    if (access->resource_id[0] == '\0' || access->usage == DVZ_FRAME_GRAPH_ACCESS_NONE)
    {
        _frame_plan_graph_report(report, "FramePlan graph pass access is incomplete");
        return false;
    }

    uint32_t resource_index = 0;
    if (!_frame_plan_graph_resource_index(plan, access->resource_id, &resource_index))
    {
        _frame_plan_graph_report(report, "FramePlan graph access references unknown resource '%s'",
                      access->resource_id);
        return false;
    }

    const DvzFrameGraphResource* resource = &plan->graph_resources[resource_index];
    uint32_t required = _frame_plan_graph_usage_flag(access->usage);
    if (required != 0 && (resource->usage_flags & required) == 0)
    {
        _frame_plan_graph_report(
            report, "FramePlan graph resource '%s' is missing usage for %s", resource->id,
            _frame_graph_access_usage_name(access->usage));
        return false;
    }

    if (_frame_plan_graph_access_reads(access->usage) &&
        resource->lifetime == DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME &&
        !_frame_plan_graph_resource_written_before(plan, access->resource_id, pass_index))
    {
        _frame_plan_graph_report(
            report, "FramePlan graph pass '%s' reads resource '%s' before any producer",
            plan->graph_passes[pass_index].id, access->resource_id);
        return false;
    }
    return true;
}



/**
 * Validate one graph render attachment.
 *
 * @param plan the FramePlan
 * @param attachment graph attachment descriptor
 * @param usage graph access usage represented by the attachment
 * @param pass_index owning pass index
 * @param report optional diagnostic report
 * @return whether the attachment is valid
 */
static bool _graph_validate_attachment(
    const DvzFramePlan* plan, const DvzFrameGraphAttachment* attachment,
    DvzFrameGraphAccessUsage usage, uint32_t pass_index, DvzDiagnosticReport* report)
{
    ANN(plan);
    ANN(attachment);
    if (attachment->resource_id[0] == '\0' ||
        attachment->access == DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_NONE)
    {
        _frame_plan_graph_report(report, "FramePlan graph attachment is incomplete");
        return false;
    }

    DvzFrameGraphAccess access = {0};
    _frame_plan_copy_label(access.resource_id, DVZ_SCENE_LABEL_SIZE, attachment->resource_id);
    access.usage = usage;
    if (!_graph_validate_access(plan, &access, pass_index, report))
        return false;

    uint32_t resource_index = 0;
    if (!_frame_plan_graph_resource_index(plan, attachment->resource_id, &resource_index))
        return false;

    const DvzFrameGraphResource* resource = &plan->graph_resources[resource_index];
    uint32_t sample_count = _frame_plan_graph_resource_sample_count(resource);
    if (!_frame_plan_graph_resource_sample_count_valid(sample_count))
    {
        _frame_plan_graph_report(
            report, "FramePlan graph resource '%s' has invalid sample count %" PRIu32,
            resource->id, sample_count);
        return false;
    }
    if (usage == DVZ_FRAME_GRAPH_ACCESS_COLOR_ATTACHMENT &&
        !_frame_plan_graph_resource_is_color_attachment_compatible(resource))
    {
        _frame_plan_graph_report(
            report, "FramePlan graph color attachment resource '%s' is not renderable",
            resource->id);
        return false;
    }
    if ((usage == DVZ_FRAME_GRAPH_ACCESS_DEPTH_ATTACHMENT_READ ||
         usage == DVZ_FRAME_GRAPH_ACCESS_DEPTH_ATTACHMENT_WRITE) &&
        !_frame_plan_graph_resource_is_depth_attachment_compatible(resource))
    {
        _frame_plan_graph_report(
            report, "FramePlan graph depth attachment resource '%s' is not a texture",
            resource->id);
        return false;
    }
    if (_frame_plan_graph_attachment_reads(attachment) &&
        resource->lifetime == DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME &&
        !_frame_plan_graph_resource_written_before(plan, attachment->resource_id, pass_index))
    {
        _frame_plan_graph_report(
            report, "FramePlan graph pass '%s' loads attachment resource '%s' before any producer",
            plan->graph_passes[pass_index].id, attachment->resource_id);
        return false;
    }
    if (usage == DVZ_FRAME_GRAPH_ACCESS_COLOR_ATTACHMENT &&
        attachment->resolve_resource_id[0] != '\0')
    {
        uint32_t resolve_index = 0;
        if (!_frame_plan_graph_resource_index(plan, attachment->resolve_resource_id, &resolve_index))
        {
            _frame_plan_graph_report(
                report, "FramePlan graph resolve attachment references unknown resource '%s'",
                attachment->resolve_resource_id);
            return false;
        }
        const DvzFrameGraphResource* resolve = &plan->graph_resources[resolve_index];
        if (!_frame_plan_graph_resource_is_color_attachment_compatible(resolve))
        {
            _frame_plan_graph_report(
                report, "FramePlan graph resolve resource '%s' is not renderable",
                resolve->id);
            return false;
        }
        if (_frame_plan_graph_resource_sample_count(resource) <= 1 ||
            _frame_plan_graph_resource_sample_count(resolve) != 1)
        {
            _frame_plan_graph_report(
                report,
                "FramePlan graph resolve from '%s' to '%s' requires multisample color and "
                "single-sample resolve",
                resource->id, resolve->id);
            return false;
        }
        if (!_frame_plan_graph_resource_extent_matches(resource, resolve))
        {
            _frame_plan_graph_report(
                report,
                "FramePlan graph resolve resource '%s' extent does not match color attachment "
                "'%s'",
                resolve->id, resource->id);
            return false;
        }
        if (resource->format != 0 && resolve->format != 0 && resource->format != resolve->format)
        {
            _frame_plan_graph_report(
                report,
                "FramePlan graph resolve resource '%s' format does not match color attachment "
                "'%s'",
                resolve->id, resource->id);
            return false;
        }
    }
    return true;
}



/**
 * Validate render-pass attachment extent and sample compatibility.
 *
 * @param plan the FramePlan
 * @param pass graph pass descriptor
 * @param report optional diagnostic report
 * @return whether the render-pass attachments are compatible
 */
static bool _graph_validate_render_pass_attachment_extents(
    const DvzFramePlan* plan, const DvzFrameGraphPass* pass, DvzDiagnosticReport* report)
{
    ANN(plan);
    ANN(pass);

    if (pass->color_attachment_count == 0)
        return true;

    uint32_t first_color_index = 0;
    if (!_frame_plan_graph_resource_index(
            plan, pass->color_attachments[0].resource_id, &first_color_index))
        return true;

    bool ok = true;
    const DvzFrameGraphResource* first_color = &plan->graph_resources[first_color_index];
    uint32_t first_sample_count = _frame_plan_graph_resource_sample_count(first_color);
    for (uint32_t i = 1; i < pass->color_attachment_count; i++)
    {
        uint32_t color_index = 0;
        if (!_frame_plan_graph_resource_index(plan, pass->color_attachments[i].resource_id, &color_index))
            continue;
        const DvzFrameGraphResource* color = &plan->graph_resources[color_index];
        if (!_frame_plan_graph_resource_extent_matches(first_color, color))
        {
            _frame_plan_graph_report(
                report,
                "FramePlan graph color attachment resource '%s' extent does not match '%s'",
                color->id, first_color->id);
            ok = false;
        }
        if (_frame_plan_graph_resource_sample_count(color) != first_sample_count)
        {
            _frame_plan_graph_report(
                report,
                "FramePlan graph color attachment resource '%s' sample count does not match "
                "'%s'",
                color->id, first_color->id);
            ok = false;
        }
    }

    if (pass->has_depth_attachment)
    {
        uint32_t depth_index = 0;
        if (_frame_plan_graph_resource_index(plan, pass->depth_attachment.resource_id, &depth_index))
        {
            const DvzFrameGraphResource* depth = &plan->graph_resources[depth_index];
            if (!_frame_plan_graph_resource_extent_matches(first_color, depth))
            {
                _frame_plan_graph_report(
                    report,
                    "FramePlan graph depth attachment resource '%s' extent does not match color "
                    "attachment '%s'",
                    depth->id, first_color->id);
                ok = false;
            }
            if (_frame_plan_graph_resource_sample_count(depth) != first_sample_count)
            {
                _frame_plan_graph_report(
                    report,
                    "FramePlan graph depth attachment resource '%s' sample count does not "
                    "match color attachment '%s'",
                    depth->id, first_color->id);
                ok = false;
            }
        }
    }
    return ok;
}



/**
 * Validate pass kind against declared render-only resources.
 *
 * @param pass graph pass descriptor
 * @param report optional diagnostic report
 * @return whether the pass kind is valid
 */
static bool _graph_validate_pass_kind(const DvzFrameGraphPass* pass, DvzDiagnosticReport* report)
{
    ANN(pass);

    bool ok = true;
    bool has_attachment = pass->color_attachment_count > 0 || pass->has_depth_attachment ||
                          pass->has_stencil_attachment;
    if (has_attachment && pass->kind != DVZ_FRAME_GRAPH_PASS_RENDER)
    {
        _frame_plan_graph_report(
            report, "FramePlan graph pass '%s' has render attachments but is not a render pass",
            pass->id);
        ok = false;
    }

    if (pass->kind == DVZ_FRAME_GRAPH_PASS_RENDER && pass->color_attachment_count == 0)
    {
        _frame_plan_graph_report(
            report, "FramePlan graph render pass '%s' has no color attachments", pass->id);
        ok = false;
    }

    for (uint32_t i = 0; i < pass->read_count; i++)
    {
        if (pass->kind != DVZ_FRAME_GRAPH_PASS_RENDER &&
            (pass->reads[i].usage == DVZ_FRAME_GRAPH_ACCESS_COLOR_ATTACHMENT ||
             pass->reads[i].usage == DVZ_FRAME_GRAPH_ACCESS_DEPTH_ATTACHMENT_READ))
        {
            _frame_plan_graph_report(
                report, "FramePlan graph pass '%s' has render-only read access", pass->id);
            ok = false;
        }
    }
    for (uint32_t i = 0; i < pass->write_count; i++)
    {
        if (pass->kind != DVZ_FRAME_GRAPH_PASS_RENDER &&
            (pass->writes[i].usage == DVZ_FRAME_GRAPH_ACCESS_COLOR_ATTACHMENT ||
             pass->writes[i].usage == DVZ_FRAME_GRAPH_ACCESS_DEPTH_ATTACHMENT_WRITE))
        {
            _frame_plan_graph_report(
                report, "FramePlan graph pass '%s' has render-only write access", pass->id);
            ok = false;
        }
    }
    return ok;
}



/**
 * Validate producer declarations and producer availability for a graph pass.
 *
 * @param plan the FramePlan
 * @param pass the graph pass descriptor
 * @param pass_index the graph pass index
 * @param report optional diagnostic report
 * @return whether producer declarations are valid
 */
static bool _graph_validate_pass_producers(
    const DvzFramePlan* plan, const DvzFrameGraphPass* pass, uint32_t pass_index,
    DvzDiagnosticReport* report)
{
    ANN(plan);
    ANN(pass);
    bool ok = true;
    for (uint32_t i = 0; i < plan->graph_resource_count; i++)
    {
        uint32_t write_count = 0;
        DvzFrameGraphAccessUsage usage = DVZ_FRAME_GRAPH_ACCESS_NONE;
        bool writes_resource = _frame_plan_graph_pass_write_count_resource(
            pass, plan->graph_resources[i].id, &write_count, &usage);
        if (writes_resource && write_count > 1)
        {
            _frame_plan_graph_report(
                report,
                "FramePlan graph pass '%s' has ambiguous producer declarations for resource '%s'",
                pass->id, plan->graph_resources[i].id);
            ok = false;
        }
    }

    for (uint32_t i = 0; i < pass->read_count; i++)
    {
        if (!_frame_plan_graph_access_reads(pass->reads[i].usage) ||
            !_frame_plan_graph_resource_is_per_frame(plan, pass->reads[i].resource_id))
            continue;
        if (!_frame_plan_graph_dependency_from_access(
                plan, pass->reads[i].resource_id, pass->reads[i].usage, pass_index, NULL))
        {
            uint32_t producer_index = 0;
            if (_frame_plan_graph_find_first_writer_after(
                    plan, pass->reads[i].resource_id, pass_index, &producer_index, NULL))
            {
                _frame_plan_graph_report(
                    report,
                    "FramePlan graph pass '%s' reads resource '%s' before producer pass '%s'; "
                    "graph passes must be topological",
                    pass->id, pass->reads[i].resource_id, plan->graph_passes[producer_index].id);
            }
            else
            {
                _frame_plan_graph_report(
                    report,
                    "FramePlan graph pass '%s' has no producer for resource '%s'",
                    pass->id, pass->reads[i].resource_id);
            }
            ok = false;
        }
    }
    return ok;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Validate the typed FramePlan graph descriptors.
 *
 * @param plan the FramePlan
 * @param report the diagnostic report
 * @return whether the graph descriptors are valid
 */
bool dvz_frame_plan_graph_validate(const DvzFramePlan* plan, DvzDiagnosticReport* report)
{
    if (plan == NULL)
    {
        _frame_plan_graph_report(report, "FramePlan graph validation requires a plan");
        return false;
    }

    bool ok = true;
    for (uint32_t i = 0; i < plan->graph_resource_count; i++)
    {
        const DvzFrameGraphResource* resource = &plan->graph_resources[i];
        if (resource->id[0] == '\0' || resource->kind == DVZ_FRAME_GRAPH_RESOURCE_NONE ||
            resource->lifetime == DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_NONE)
        {
            _frame_plan_graph_report(report, "FramePlan graph resource at index %" PRIu32 " is incomplete", i);
            ok = false;
        }
        for (uint32_t j = i + 1; j < plan->graph_resource_count; j++)
        {
            if (strcmp(resource->id, plan->graph_resources[j].id) == 0)
            {
                _frame_plan_graph_report(
                    report, "FramePlan graph resource id '%s' is duplicated", resource->id);
                ok = false;
            }
        }
    }

    for (uint32_t i = 0; i < plan->graph_pass_count; i++)
    {
        const DvzFrameGraphPass* pass = &plan->graph_passes[i];
        if (pass->id[0] == '\0' || pass->kind == DVZ_FRAME_GRAPH_PASS_NONE)
        {
            _frame_plan_graph_report(report, "FramePlan graph pass at index %" PRIu32 " is incomplete", i);
            ok = false;
        }
        ok = _graph_validate_pass_kind(pass, report) && ok;
        ok = _graph_validate_pass_producers(plan, pass, i, report) && ok;
        if (_frame_plan_graph_pass_id_exists_before(plan, pass->id, i))
        {
            _frame_plan_graph_report(report, "FramePlan graph pass id '%s' is duplicated", pass->id);
            ok = false;
        }

        for (uint32_t j = 0; j < pass->read_count; j++)
        {
            if (!_frame_plan_graph_access_reads(pass->reads[j].usage))
            {
                _frame_plan_graph_report(
                    report, "FramePlan graph read access for resource '%s' is not a read usage",
                    pass->reads[j].resource_id);
                ok = false;
                continue;
            }
            ok = _graph_validate_access(plan, &pass->reads[j], i, report) && ok;
        }
        for (uint32_t j = 0; j < pass->write_count; j++)
        {
            if (!_frame_plan_graph_access_writes(pass->writes[j].usage))
            {
                _frame_plan_graph_report(
                    report, "FramePlan graph write access for resource '%s' is not a write usage",
                    pass->writes[j].resource_id);
                ok = false;
                continue;
            }
            ok = _graph_validate_access(plan, &pass->writes[j], i, report) && ok;
        }
        for (uint32_t j = 0; j < pass->color_attachment_count; j++)
            ok = _graph_validate_attachment(
                     plan, &pass->color_attachments[j], DVZ_FRAME_GRAPH_ACCESS_COLOR_ATTACHMENT,
                     i, report) &&
                 ok;
        if (pass->has_depth_attachment)
        {
            DvzFrameGraphAccessUsage usage = pass->depth_attachment.access ==
                                                     DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_READ
                                                 ? DVZ_FRAME_GRAPH_ACCESS_DEPTH_ATTACHMENT_READ
                                                 : DVZ_FRAME_GRAPH_ACCESS_DEPTH_ATTACHMENT_WRITE;
            ok = _graph_validate_attachment(plan, &pass->depth_attachment, usage, i, report) && ok;
        }
        if (pass->has_stencil_attachment)
        {
            DvzFrameGraphAccessUsage usage = pass->stencil_attachment.access ==
                                                     DVZ_FRAME_GRAPH_ATTACHMENT_ACCESS_READ
                                                 ? DVZ_FRAME_GRAPH_ACCESS_DEPTH_ATTACHMENT_READ
                                                 : DVZ_FRAME_GRAPH_ACCESS_DEPTH_ATTACHMENT_WRITE;
            ok = _graph_validate_attachment(plan, &pass->stencil_attachment, usage, i, report) &&
                 ok;
        }
        if (pass->kind == DVZ_FRAME_GRAPH_PASS_RENDER)
            ok = _graph_validate_render_pass_attachment_extents(plan, pass, report) && ok;
    }
    return ok;
}
