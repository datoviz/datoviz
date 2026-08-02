/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene FramePlan semantic render products                                                     */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_log.h"
#include "_overflow.h"
#include "frame_plan/internal.h"
#include "graph/internal.h"
#include "internal.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static bool _ensure_product_capacity(DvzFramePlan* plan)
{
    ANN(plan);
    if (plan->products == NULL || plan->product_capacity == 0)
    {
        plan->product_capacity = DVZ_FRAME_PLAN_INITIAL_PRODUCT_CAPACITY;
        plan->products = (DvzRenderProductContract*)dvz_calloc(
            plan->product_capacity, sizeof(DvzRenderProductContract));
        return plan->products != NULL;
    }
    if (plan->product_count < plan->product_capacity)
        return true;
    if (plan->product_capacity > UINT32_MAX / 2)
        return false;
    uint32_t capacity = plan->product_capacity * 2;
    uint64_t bytes = 0;
    if (_dvz_mul_u64_overflows(capacity, sizeof(DvzRenderProductContract), &bytes))
        return false;
    DvzRenderProductContract* products =
        (DvzRenderProductContract*)dvz_realloc(plan->products, bytes);
    if (products == NULL)
        return false;
    plan->product_capacity = capacity;
    plan->products = products;
    return true;
}



static bool _ensure_product_use_capacity(DvzFramePlan* plan)
{
    ANN(plan);
    if (plan->product_uses == NULL || plan->product_use_capacity == 0)
    {
        plan->product_use_capacity = DVZ_FRAME_PLAN_INITIAL_PRODUCT_USE_CAPACITY;
        plan->product_uses = (DvzRenderProductConsumer*)dvz_calloc(
            plan->product_use_capacity, sizeof(DvzRenderProductConsumer));
        return plan->product_uses != NULL;
    }
    if (plan->product_use_count < plan->product_use_capacity)
        return true;
    if (plan->product_use_capacity > UINT32_MAX / 2)
        return false;
    uint32_t capacity = plan->product_use_capacity * 2;
    uint64_t bytes = 0;
    if (_dvz_mul_u64_overflows(capacity, sizeof(DvzRenderProductConsumer), &bytes))
        return false;
    DvzRenderProductConsumer* uses =
        (DvzRenderProductConsumer*)dvz_realloc(plan->product_uses, bytes);
    if (uses == NULL)
        return false;
    plan->product_use_capacity = capacity;
    plan->product_uses = uses;
    return true;
}



static const char* _product_label(const DvzRenderProductContract* product)
{
    ANN(product);
    return product->diagnostic_label[0] != '\0' ? product->diagnostic_label : "<unnamed>";
}



static bool _product_index(const DvzFramePlan* plan, DvzRenderProductId id, uint32_t* index)
{
    ANN(plan);
    if (id.value == 0)
        return false;
    for (uint32_t i = 0; i < plan->product_count; i++)
    {
        if (plan->products[i].id.value == id.value)
        {
            if (index != NULL)
                *index = i;
            return true;
        }
    }
    return false;
}



static bool _pass_reads_resource(const DvzFrameGraphPass* pass, const char* resource_id)
{
    ANN(pass);
    ANN(resource_id);
    for (uint32_t i = 0; i < pass->read_count; i++)
    {
        if (_frame_plan_graph_access_reads(pass->reads[i].usage) &&
            strcmp(pass->reads[i].resource_id, resource_id) == 0)
            return true;
    }
    for (uint32_t i = 0; i < pass->color_attachment_count; i++)
    {
        if (_frame_plan_graph_attachment_reads(&pass->color_attachments[i]) &&
            strcmp(pass->color_attachments[i].resource_id, resource_id) == 0)
            return true;
    }
    if (pass->has_depth_attachment &&
        _frame_plan_graph_attachment_reads(&pass->depth_attachment) &&
        strcmp(pass->depth_attachment.resource_id, resource_id) == 0)
        return true;
    return false;
}



static uint32_t _pass_resource_usage_flags(const DvzFrameGraphPass* pass, const char* resource_id)
{
    ANN(pass);
    ANN(resource_id);
    uint32_t flags = 0;
    for (uint32_t i = 0; i < pass->read_count; i++)
    {
        if (strcmp(pass->reads[i].resource_id, resource_id) == 0)
            flags |= _frame_plan_graph_usage_flag(pass->reads[i].usage);
    }
    for (uint32_t i = 0; i < pass->write_count; i++)
    {
        if (strcmp(pass->writes[i].resource_id, resource_id) == 0)
            flags |= _frame_plan_graph_usage_flag(pass->writes[i].usage);
    }
    for (uint32_t i = 0; i < pass->color_attachment_count; i++)
    {
        const DvzFrameGraphAttachment* attachment = &pass->color_attachments[i];
        if (strcmp(attachment->resource_id, resource_id) == 0 ||
            strcmp(attachment->resolve_resource_id, resource_id) == 0)
            flags |= DVZ_FRAME_GRAPH_RESOURCE_USAGE_COLOR_ATTACHMENT;
    }
    if (pass->has_depth_attachment && strcmp(pass->depth_attachment.resource_id, resource_id) == 0)
        flags |= DVZ_FRAME_GRAPH_RESOURCE_USAGE_DEPTH_ATTACHMENT;
    if (pass->has_stencil_attachment &&
        strcmp(pass->stencil_attachment.resource_id, resource_id) == 0)
        flags |= DVZ_FRAME_GRAPH_RESOURCE_USAGE_DEPTH_ATTACHMENT;
    return flags;
}



static bool _product_kind_contract_valid(const DvzRenderProductContract* product)
{
    ANN(product);
    switch (product->kind)
    {
    case DVZ_RENDER_PRODUCT_SCENE_COLOR:
        return product->format_class == DVZ_RENDER_PRODUCT_FORMAT_LINEAR_COLOR &&
               product->encoding == DVZ_RENDER_PRODUCT_ENCODING_LINEAR_SCENE_COLOR &&
               product->alpha != DVZ_RENDER_PRODUCT_ALPHA_NONE;
    case DVZ_RENDER_PRODUCT_SURFACE_DEPTH:
    case DVZ_RENDER_PRODUCT_SCENE_OCCLUSION_DEPTH:
    case DVZ_RENDER_PRODUCT_VOLUME_FIRST_HIT_DEPTH:
        return product->format_class == DVZ_RENDER_PRODUCT_FORMAT_DEPTH_FLOAT &&
               product->encoding == DVZ_RENDER_PRODUCT_ENCODING_LINEAR_VIEW_DEPTH;
    case DVZ_RENDER_PRODUCT_SURFACE_NORMAL:
        return product->format_class == DVZ_RENDER_PRODUCT_FORMAT_NORMAL_FLOAT &&
               product->encoding == DVZ_RENDER_PRODUCT_ENCODING_VIEW_NORMAL;
    case DVZ_RENDER_PRODUCT_SURFACE_COVERAGE:
        return product->format_class == DVZ_RENDER_PRODUCT_FORMAT_COVERAGE &&
               product->encoding == DVZ_RENDER_PRODUCT_ENCODING_COVERAGE &&
               product->coverage != DVZ_RENDER_PRODUCT_COVERAGE_NONE;
    case DVZ_RENDER_PRODUCT_OBJECT_ID:
        return product->format_class == DVZ_RENDER_PRODUCT_FORMAT_UINT_ID &&
               product->encoding == DVZ_RENDER_PRODUCT_ENCODING_INTEGER_ID;
    case DVZ_RENDER_PRODUCT_AMBIENT_VISIBILITY:
        return product->format_class == DVZ_RENDER_PRODUCT_FORMAT_SCALAR_FLOAT &&
               product->encoding == DVZ_RENDER_PRODUCT_ENCODING_UNIT_VISIBILITY;
    case DVZ_RENDER_PRODUCT_TRANSPARENT_ACCUMULATION:
        return product->format_class == DVZ_RENDER_PRODUCT_FORMAT_LINEAR_COLOR &&
               product->encoding == DVZ_RENDER_PRODUCT_ENCODING_PREMULTIPLIED_ACCUMULATION &&
               product->alpha == DVZ_RENDER_PRODUCT_ALPHA_PREMULTIPLIED;
    case DVZ_RENDER_PRODUCT_TRANSPARENT_TRANSMITTANCE:
        return product->format_class == DVZ_RENDER_PRODUCT_FORMAT_SCALAR_FLOAT &&
               product->encoding == DVZ_RENDER_PRODUCT_ENCODING_UNIT_TRANSMITTANCE &&
               product->alpha == DVZ_RENDER_PRODUCT_ALPHA_NONE;
    case DVZ_RENDER_PRODUCT_TRANSPARENT_PEEL_DEPTH:
        return product->format_class == DVZ_RENDER_PRODUCT_FORMAT_DEPTH_FLOAT &&
               product->encoding == DVZ_RENDER_PRODUCT_ENCODING_LINEAR_VIEW_DEPTH &&
               product->alpha == DVZ_RENDER_PRODUCT_ALPHA_NONE;
    case DVZ_RENDER_PRODUCT_PRESENTATION_COLOR:
        return product->format_class == DVZ_RENDER_PRODUCT_FORMAT_PRESENTATION_COLOR &&
               product->encoding == DVZ_RENDER_PRODUCT_ENCODING_PRESENTATION_TRANSFER &&
               product->alpha != DVZ_RENDER_PRODUCT_ALPHA_NONE;
    case DVZ_RENDER_PRODUCT_NONE:
    default:
        return false;
    }
}



static bool _surface_member(DvzRenderProductKind kind)
{
    return kind == DVZ_RENDER_PRODUCT_SURFACE_DEPTH || kind == DVZ_RENDER_PRODUCT_SURFACE_NORMAL ||
           kind == DVZ_RENDER_PRODUCT_SURFACE_COVERAGE;
}



static bool _product_panel_scoped(const DvzRenderProductContract* product)
{
    ANN(product);
    return product->domain == DVZ_RENDER_PRODUCT_DOMAIN_PANEL ||
           product->extent_policy == DVZ_RENDER_PRODUCT_EXTENT_PANEL_RELATIVE ||
           product->panel_id[0] != '\0';
}



static bool _surface_pair_compatible(
    const DvzFramePlan* plan, const DvzRenderProductContract* first,
    const DvzRenderProductContract* second)
{
    ANN(plan);
    ANN(first);
    ANN(second);
    bool source_coherent =
        first->source_product_id.value == 0 && second->source_product_id.value == 0;
    if (first->source_product_id.value != 0 && second->source_product_id.value != 0)
    {
        uint32_t first_source_index = 0;
        uint32_t second_source_index = 0;
        if (_product_index(plan, first->source_product_id, &first_source_index) &&
            _product_index(plan, second->source_product_id, &second_source_index))
        {
            const DvzSurfaceRecordId first_record =
                plan->products[first_source_index].surface_record_id;
            const DvzSurfaceRecordId second_record =
                plan->products[second_source_index].surface_record_id;
            source_coherent = first_record.value != 0 && first_record.value == second_record.value;
        }
    }
    return source_coherent && strcmp(first->panel_id, second->panel_id) == 0 &&
           strcmp(first->view_id, second->view_id) == 0 &&
           strcmp(first->camera_id, second->camera_id) == 0 &&
           strcmp(first->projection_id, second->projection_id) == 0 &&
           first->extent_policy == second->extent_policy &&
           first->rounding_policy == second->rounding_policy &&
           first->origin_x == second->origin_x && first->origin_y == second->origin_y &&
           first->width == second->width && first->height == second->height &&
           first->render_scale == second->render_scale &&
           memcmp(
               first->local_to_target, second->local_to_target, sizeof(first->local_to_target)) ==
               0 &&
           first->sample_domain == second->sample_domain &&
           first->sample_count == second->sample_count &&
           first->coordinate_space == second->coordinate_space &&
           first->producer_pass_index == second->producer_pass_index;
}



static bool _resolve_policy_valid(const DvzRenderProductContract* product)
{
    ANN(product);
    switch (product->kind)
    {
    case DVZ_RENDER_PRODUCT_SCENE_COLOR:
    case DVZ_RENDER_PRODUCT_TRANSPARENT_ACCUMULATION:
        return product->resolve_policy == DVZ_RENDER_PRODUCT_RESOLVE_LINEAR_COLOR;
    case DVZ_RENDER_PRODUCT_SURFACE_DEPTH:
    case DVZ_RENDER_PRODUCT_SCENE_OCCLUSION_DEPTH:
    case DVZ_RENDER_PRODUCT_TRANSPARENT_PEEL_DEPTH:
    case DVZ_RENDER_PRODUCT_VOLUME_FIRST_HIT_DEPTH:
        return product->resolve_policy == DVZ_RENDER_PRODUCT_RESOLVE_NEAREST_VALID_DEPTH;
    case DVZ_RENDER_PRODUCT_SURFACE_NORMAL:
        return product->resolve_policy == DVZ_RENDER_PRODUCT_RESOLVE_WINNING_NORMAL;
    case DVZ_RENDER_PRODUCT_SURFACE_COVERAGE:
        return product->resolve_policy == DVZ_RENDER_PRODUCT_RESOLVE_COVERAGE_FRACTION;
    case DVZ_RENDER_PRODUCT_OBJECT_ID:
        return product->resolve_policy == DVZ_RENDER_PRODUCT_RESOLVE_WINNING_ID;
    case DVZ_RENDER_PRODUCT_AMBIENT_VISIBILITY:
    case DVZ_RENDER_PRODUCT_TRANSPARENT_TRANSMITTANCE:
        return product->resolve_policy == DVZ_RENDER_PRODUCT_RESOLVE_VISIBILITY;
    case DVZ_RENDER_PRODUCT_PRESENTATION_COLOR:
    case DVZ_RENDER_PRODUCT_NONE:
    default:
        return false;
    }
}



static bool _format_class_valid(DvzRenderProductFormatClass format_class, uint32_t concrete_format)
{
    switch (format_class)
    {
    case DVZ_RENDER_PRODUCT_FORMAT_LINEAR_COLOR:
        return concrete_format == DVZ_FORMAT_R8G8B8A8_UNORM ||
               concrete_format == DVZ_FORMAT_B8G8R8A8_UNORM ||
               concrete_format == DVZ_FORMAT_R16G16B16A16_SFLOAT ||
               concrete_format == DVZ_FORMAT_R32G32B32A32_SFLOAT;
    case DVZ_RENDER_PRODUCT_FORMAT_DEPTH_FLOAT:
        return concrete_format == DVZ_FORMAT_R16_SFLOAT ||
               concrete_format == DVZ_FORMAT_R32_SFLOAT;
    case DVZ_RENDER_PRODUCT_FORMAT_NORMAL_FLOAT:
        return concrete_format == DVZ_FORMAT_R16G16B16A16_SFLOAT ||
               concrete_format == DVZ_FORMAT_R32G32B32A32_SFLOAT;
    case DVZ_RENDER_PRODUCT_FORMAT_COVERAGE:
    case DVZ_RENDER_PRODUCT_FORMAT_SCALAR_FLOAT:
        return concrete_format == DVZ_FORMAT_R8_UNORM || concrete_format == DVZ_FORMAT_R16_UNORM ||
               concrete_format == DVZ_FORMAT_R16_SFLOAT ||
               concrete_format == DVZ_FORMAT_R32_SFLOAT;
    case DVZ_RENDER_PRODUCT_FORMAT_UINT_ID:
        return concrete_format == DVZ_FORMAT_R8_UINT || concrete_format == DVZ_FORMAT_R16_UINT ||
               concrete_format == DVZ_FORMAT_R32_UINT;
    case DVZ_RENDER_PRODUCT_FORMAT_PRESENTATION_COLOR:
        return concrete_format == DVZ_FORMAT_R8G8B8A8_UNORM ||
               concrete_format == DVZ_FORMAT_R8G8B8A8_SRGB ||
               concrete_format == DVZ_FORMAT_B8G8R8A8_UNORM ||
               concrete_format == DVZ_FORMAT_B8G8R8A8_SRGB;
    case DVZ_RENDER_PRODUCT_FORMAT_NONE:
    default:
        return false;
    }
}



static bool _validate_product_samples(
    const DvzFramePlan* plan, const DvzRenderProductContract* product, DvzDiagnosticReport* report)
{
    ANN(plan);
    ANN(product);
    if (product->sample_domain == DVZ_RENDER_PRODUCT_SAMPLES_SINGLE)
    {
        if (product->sample_count == 1 &&
            product->resolve_policy == DVZ_RENDER_PRODUCT_RESOLVE_NONE)
            return true;
    }
    else if (product->sample_domain == DVZ_RENDER_PRODUCT_SAMPLES_MULTISAMPLE)
    {
        if (product->sample_count > 1 &&
            product->resolve_policy == DVZ_RENDER_PRODUCT_RESOLVE_NONE)
            return true;
    }
    else if (product->sample_domain == DVZ_RENDER_PRODUCT_SAMPLES_RESOLVED)
    {
        uint32_t source_index = 0;
        if (product->sample_count == 1 && _resolve_policy_valid(product) &&
            product->source_product_id.value != 0 &&
            _product_index(plan, product->source_product_id, &source_index))
        {
            const DvzRenderProductContract* source = &plan->products[source_index];
            if (source->kind == product->kind &&
                source->sample_domain == DVZ_RENDER_PRODUCT_SAMPLES_MULTISAMPLE &&
                source->sample_count > 1)
                return true;
        }
    }
    _frame_plan_graph_report(
        report, "FramePlan product '%s' has an invalid sample-domain or resolve contract",
        _product_label(product));
    return false;
}



static bool _validate_product_resource(
    const DvzFramePlan* plan, const DvzRenderProductContract* product, DvzDiagnosticReport* report)
{
    ANN(plan);
    ANN(product);
    if (product->resource_index >= plan->graph_resource_count)
    {
        _frame_plan_graph_report(
            report,
            "FramePlan product '%s' references graph resource index %" PRIu32 " out of bounds",
            _product_label(product), product->resource_index);
        return false;
    }
    const DvzFrameGraphResource* resource = &plan->graph_resources[product->resource_index];
    bool ok = true;
    if (!_format_class_valid(product->format_class, product->concrete_format))
    {
        _frame_plan_graph_report(
            report,
            "FramePlan product '%s' concrete format is incompatible with its semantic format "
            "class",
            _product_label(product));
        ok = false;
    }
    if (product->concrete_format != 0 && resource->format != 0 &&
        product->concrete_format != resource->format)
    {
        _frame_plan_graph_report(
            report, "FramePlan product '%s' concrete format does not match resource '%s'",
            _product_label(product), resource->id);
        ok = false;
    }
    if (product->sample_count != _frame_plan_graph_resource_sample_count(resource))
    {
        _frame_plan_graph_report(
            report, "FramePlan product '%s' sample count does not match resource '%s'",
            _product_label(product), resource->id);
        ok = false;
    }
    if ((resource->usage_flags & product->required_usage_flags) != product->required_usage_flags)
    {
        _frame_plan_graph_report(
            report, "FramePlan product '%s' required accesses are not realized by resource '%s'",
            _product_label(product), resource->id);
        ok = false;
    }
    if (resource->lifetime != product->lifetime)
    {
        _frame_plan_graph_report(
            report, "FramePlan product '%s' lifetime does not match resource '%s'",
            _product_label(product), resource->id);
        ok = false;
    }
    bool extent_matches = (product->extent_policy == DVZ_RENDER_PRODUCT_EXTENT_ABSOLUTE &&
                           resource->extent_kind == DVZ_FRAME_GRAPH_EXTENT_FIXED) ||
                          (product->extent_policy == DVZ_RENDER_PRODUCT_EXTENT_PANEL_RELATIVE &&
                           resource->extent_kind == DVZ_FRAME_GRAPH_EXTENT_PANEL) ||
                          (product->extent_policy == DVZ_RENDER_PRODUCT_EXTENT_SOURCE_RELATIVE &&
                           resource->extent_kind == DVZ_FRAME_GRAPH_EXTENT_RESOURCE_REF);
    if (!extent_matches || product->width != resource->width ||
        product->height != resource->height)
    {
        _frame_plan_graph_report(
            report, "FramePlan product '%s' extent does not match resource '%s'",
            _product_label(product), resource->id);
        ok = false;
    }
    return ok;
}



static bool _product_has_consumer(
    const DvzFramePlan* plan, const DvzRenderProductContract* product, uint32_t pass_index)
{
    ANN(plan);
    ANN(product);
    for (uint32_t i = 0; i < plan->product_use_count; i++)
    {
        if (plan->product_uses[i].product_id.value == product->id.value &&
            plan->product_uses[i].pass_index == pass_index)
            return true;
    }
    return false;
}



static uint32_t
_product_last_use(const DvzFramePlan* plan, const DvzRenderProductContract* product)
{
    ANN(plan);
    ANN(product);
    uint32_t last_use = product->producer_pass_index;
    for (uint32_t i = 0; i < plan->product_use_count; i++)
    {
        const DvzRenderProductConsumer* use = &plan->product_uses[i];
        if (use->product_id.value == product->id.value && use->pass_index > last_use)
            last_use = use->pass_index;
    }
    return last_use;
}



static bool _validate_product_source(
    const DvzFramePlan* plan, const DvzRenderProductContract* product, DvzDiagnosticReport* report)
{
    ANN(plan);
    ANN(product);
    if (product->source_product_id.value == 0)
    {
        if (product->extent_policy == DVZ_RENDER_PRODUCT_EXTENT_SOURCE_RELATIVE ||
            product->sample_domain == DVZ_RENDER_PRODUCT_SAMPLES_RESOLVED)
        {
            _frame_plan_graph_report(
                report, "FramePlan product '%s' requires an explicit source product",
                _product_label(product));
            return false;
        }
        return true;
    }

    uint32_t source_index = 0;
    if (!_product_index(plan, product->source_product_id, &source_index))
    {
        _frame_plan_graph_report(
            report, "FramePlan product '%s' references unknown source product id %" PRIu32,
            _product_label(product), product->source_product_id.value);
        return false;
    }
    const DvzRenderProductContract* source = &plan->products[source_index];
    bool ok = true;
    if (source == product)
    {
        _frame_plan_graph_report(
            report, "FramePlan product '%s' references itself", _product_label(product));
        return false;
    }
    if (source->domain != product->domain || strcmp(source->panel_id, product->panel_id) != 0 ||
        strcmp(source->view_id, product->view_id) != 0 ||
        strcmp(source->camera_id, product->camera_id) != 0 ||
        strcmp(source->projection_id, product->projection_id) != 0)
    {
        _frame_plan_graph_report(
            report, "FramePlan product '%s' source crosses semantic scope",
            _product_label(product));
        ok = false;
    }
    if (source->kind == product->kind && source->version >= product->version)
    {
        _frame_plan_graph_report(
            report, "FramePlan product '%s' does not advance its source product version",
            _product_label(product));
        ok = false;
    }
    if (source->resource_index >= plan->graph_resource_count ||
        product->resource_index >= plan->graph_resource_count ||
        product->producer_pass_index >= plan->graph_pass_count)
        return false;
    const DvzFrameGraphResource* source_resource = &plan->graph_resources[source->resource_index];
    const DvzFrameGraphResource* product_resource =
        &plan->graph_resources[product->resource_index];
    const DvzFrameGraphPass* producer = &plan->graph_passes[product->producer_pass_index];
    bool attachment_resolve = false;
    for (uint32_t i = 0; i < producer->color_attachment_count; i++)
    {
        const DvzFrameGraphAttachment* attachment = &producer->color_attachments[i];
        attachment_resolve = attachment_resolve ||
                             (strcmp(attachment->resource_id, source_resource->id) == 0 &&
                              strcmp(attachment->resolve_resource_id, product_resource->id) == 0);
    }
    if (attachment_resolve && product->resolve_policy != DVZ_RENDER_PRODUCT_RESOLVE_LINEAR_COLOR)
    {
        _frame_plan_graph_report(
            report,
            "FramePlan resolved product '%s' uses attachment resolve for a non-color policy",
            _product_label(product));
        ok = false;
    }
    if (!attachment_resolve && !_product_has_consumer(plan, source, product->producer_pass_index))
    {
        _frame_plan_graph_report(
            report, "FramePlan product '%s' source omits its successor producer consumer",
            _product_label(product));
        ok = false;
    }
    if (!attachment_resolve && !_pass_reads_resource(producer, source_resource->id))
    {
        _frame_plan_graph_report(
            report, "FramePlan product '%s' producer does not read source resource '%s'",
            _product_label(product), source_resource->id);
        ok = false;
    }
    if (source->resource_index == product->resource_index)
    {
        _frame_plan_graph_report(
            report, "FramePlan product '%s' aliases its live source product resource",
            _product_label(product));
        ok = false;
    }
    if (product->extent_policy == DVZ_RENDER_PRODUCT_EXTENT_SOURCE_RELATIVE)
    {
        if (strcmp(product_resource->extent_resource_id, source_resource->id) != 0)
        {
            _frame_plan_graph_report(
                report,
                "FramePlan product '%s' source-relative resource does not reference source "
                "product id %" PRIu32,
                _product_label(product), source->id.value);
            ok = false;
        }
    }
    return ok;
}



static bool _validate_product_passes(
    const DvzFramePlan* plan, const DvzRenderProductContract* product, DvzDiagnosticReport* report)
{
    ANN(plan);
    ANN(product);
    if (product->resource_index >= plan->graph_resource_count ||
        product->producer_pass_index >= plan->graph_pass_count)
    {
        _frame_plan_graph_report(
            report, "FramePlan product '%s' has an out-of-bounds resource or producer index",
            _product_label(product));
        return false;
    }
    const DvzFrameGraphResource* resource = &plan->graph_resources[product->resource_index];
    const DvzFrameGraphPass* producer = &plan->graph_passes[product->producer_pass_index];
    bool ok = true;
    uint32_t realized_usage = _pass_resource_usage_flags(producer, resource->id);
    for (uint32_t i = 0; i < plan->product_use_count; i++)
    {
        const DvzRenderProductConsumer* use = &plan->product_uses[i];
        if (use->product_id.value == product->id.value && use->pass_index < plan->graph_pass_count)
            realized_usage |=
                _pass_resource_usage_flags(&plan->graph_passes[use->pass_index], resource->id);
    }
    if ((product->required_usage_flags & realized_usage) != realized_usage)
    {
        _frame_plan_graph_report(
            report, "FramePlan product '%s' omits realized producer or consumer accesses",
            _product_label(product));
        ok = false;
    }
    if (!_frame_plan_graph_pass_writes_resource(producer, resource->id))
    {
        _frame_plan_graph_report(
            report, "FramePlan product '%s' producer pass '%s' does not write resource '%s'",
            _product_label(product), producer->id, resource->id);
        ok = false;
    }
    if (_product_panel_scoped(product) && strcmp(product->panel_id, producer->panel_id) != 0)
    {
        _frame_plan_graph_report(
            report, "FramePlan product '%s' crosses panel scope at producer pass '%s'",
            _product_label(product), producer->id);
        ok = false;
    }
    for (uint32_t i = 0; i < plan->product_use_count; i++)
    {
        const DvzRenderProductConsumer* use = &plan->product_uses[i];
        if (use->product_id.value != product->id.value)
            continue;
        if (use->pass_index >= plan->graph_pass_count)
        {
            _frame_plan_graph_report(
                report, "FramePlan product '%s' has consumer pass index %" PRIu32 " out of bounds",
                _product_label(product), use->pass_index);
            ok = false;
            continue;
        }
        const DvzFrameGraphPass* consumer = &plan->graph_passes[use->pass_index];
        if (!_pass_reads_resource(consumer, resource->id))
        {
            _frame_plan_graph_report(
                report, "FramePlan product '%s' consumer pass '%s' does not read resource '%s'",
                _product_label(product), consumer->id, resource->id);
            ok = false;
        }
        if (use->pass_index <= product->producer_pass_index)
        {
            _frame_plan_graph_report(
                report,
                "FramePlan product '%s' is consumed before its producer or feeds back in one pass",
                _product_label(product));
            ok = false;
        }
        for (uint32_t pass_index = product->producer_pass_index + 1; pass_index < use->pass_index;
             pass_index++)
        {
            if (_frame_plan_graph_pass_writes_resource(
                    &plan->graph_passes[pass_index], resource->id))
            {
                _frame_plan_graph_report(
                    report, "FramePlan product '%s' is overwritten before consumer pass '%s'",
                    _product_label(product), consumer->id);
                ok = false;
                break;
            }
        }
        if (_product_panel_scoped(product) && strcmp(product->panel_id, consumer->panel_id) != 0)
        {
            _frame_plan_graph_report(
                report, "FramePlan product '%s' crosses panel scope at consumer pass '%s'",
                _product_label(product), consumer->id);
            ok = false;
        }
        bool validity_ok =
            use->validity_requirement == DVZ_RENDER_PRODUCT_VALIDITY_REQUIREMENT_ANY_DEFINED ||
            (use->validity_requirement == DVZ_RENDER_PRODUCT_VALIDITY_REQUIREMENT_FULL_EXTENT &&
             product->validity == DVZ_RENDER_PRODUCT_VALIDITY_FULL_EXTENT) ||
            (use->validity_requirement ==
                 DVZ_RENDER_PRODUCT_VALIDITY_REQUIREMENT_EXPLICIT_COVERAGE &&
             product->validity == DVZ_RENDER_PRODUCT_VALIDITY_EXPLICIT_COVERAGE) ||
            (use->validity_requirement ==
                 DVZ_RENDER_PRODUCT_VALIDITY_REQUIREMENT_BACKGROUND_VALUE &&
             product->validity == DVZ_RENDER_PRODUCT_VALIDITY_BACKGROUND_VALUE) ||
            (use->validity_requirement ==
                 DVZ_RENDER_PRODUCT_VALIDITY_REQUIREMENT_INTEGER_SENTINEL &&
             product->validity == DVZ_RENDER_PRODUCT_VALIDITY_INTEGER_SENTINEL);
        if (use->validity_requirement == DVZ_RENDER_PRODUCT_VALIDITY_REQUIREMENT_NONE ||
            !validity_ok)
        {
            _frame_plan_graph_report(
                report, "FramePlan product '%s' validity is incompatible with consumer pass '%s'",
                _product_label(product), consumer->id);
            ok = false;
        }
        if (use->validity_requirement == DVZ_RENDER_PRODUCT_VALIDITY_REQUIREMENT_EXPLICIT_COVERAGE)
        {
            uint32_t coverage_index = 0;
            if (!_product_index(plan, product->validity_product_id, &coverage_index) ||
                !_product_has_consumer(plan, &plan->products[coverage_index], use->pass_index))
            {
                _frame_plan_graph_report(
                    report,
                    "FramePlan product '%s' consumer pass '%s' omits its explicit coverage use",
                    _product_label(product), consumer->id);
                ok = false;
            }
        }
    }
    for (uint32_t pass_index = product->producer_pass_index + 1;
         pass_index < plan->graph_pass_count; pass_index++)
    {
        const DvzFrameGraphPass* pass = &plan->graph_passes[pass_index];
        if (_frame_plan_graph_pass_writes_resource(pass, resource->id))
            break;
        if (_pass_reads_resource(pass, resource->id) &&
            !_product_has_consumer(plan, product, pass_index))
        {
            _frame_plan_graph_report(
                report, "FramePlan product '%s' omits actual reader pass '%s'",
                _product_label(product), pass->id);
            ok = false;
        }
    }
    return ok;
}



/*************************************************************************************************/
/*  Names                                                                                        */
/*************************************************************************************************/

#define DVZ_PRODUCT_NAME_FUNCTION(function_name, enum_type, ...)                                  \
    const char* function_name(enum_type value)                                                    \
    {                                                                                             \
        static const char* names[] = {__VA_ARGS__};                                               \
        uint32_t index = (uint32_t)value;                                                         \
        return index < sizeof(names) / sizeof(names[0]) ? names[index] : "none";                  \
    }

DVZ_PRODUCT_NAME_FUNCTION(
    _frame_plan_product_kind_name, DvzRenderProductKind, "none", "scene_color", "surface_depth",
    "surface_normal", "surface_coverage", "object_id", "ambient_visibility",
    "scene_occlusion_depth", "transparent_accumulation", "transparent_transmittance",
    "transparent_peel_depth", "volume_first_hit_depth", "presentation_color")
DVZ_PRODUCT_NAME_FUNCTION(
    _frame_plan_product_domain_name, DvzRenderProductDomain, "none", "panel", "view", "scene",
    "query", "presentation")
DVZ_PRODUCT_NAME_FUNCTION(
    _frame_plan_product_extent_name, DvzRenderProductExtentPolicy, "none", "absolute",
    "panel_relative", "source_relative")
DVZ_PRODUCT_NAME_FUNCTION(
    _frame_plan_product_rounding_name, DvzRenderProductRoundingPolicy, "none", "floor", "ceil",
    "nearest", "outward")
DVZ_PRODUCT_NAME_FUNCTION(
    _frame_plan_product_format_name, DvzRenderProductFormatClass, "none", "linear_color",
    "depth_float", "normal_float", "coverage", "uint_id", "scalar_float", "presentation_color")
DVZ_PRODUCT_NAME_FUNCTION(
    _frame_plan_product_samples_name, DvzRenderProductSampleDomain, "none", "single",
    "multisample", "resolved")
DVZ_PRODUCT_NAME_FUNCTION(
    _frame_plan_product_resolve_name, DvzRenderProductResolvePolicy, "none", "linear_color",
    "nearest_valid_depth", "winning_normal", "coverage_fraction", "winning_id", "visibility")
DVZ_PRODUCT_NAME_FUNCTION(
    _frame_plan_product_coordinates_name, DvzRenderProductCoordinateSpace, "none", "panel_local",
    "view", "scene", "target", "world", "clip", "ndc", "framebuffer_pixel", "not_applicable")
DVZ_PRODUCT_NAME_FUNCTION(
    _frame_plan_product_encoding_name, DvzRenderProductEncoding, "none", "linear_scene_color",
    "linear_view_depth", "view_normal", "unit_visibility", "integer_id", "coverage",
    "premultiplied_accumulation", "unit_transmittance", "presentation_transfer")
DVZ_PRODUCT_NAME_FUNCTION(
    _frame_plan_product_alpha_name, DvzRenderProductAlpha, "none", "opaque", "straight",
    "premultiplied")
DVZ_PRODUCT_NAME_FUNCTION(
    _frame_plan_product_coverage_name, DvzRenderProductCoverage, "none", "binary",
    "sample_fraction", "winning_sample")
DVZ_PRODUCT_NAME_FUNCTION(
    _frame_plan_product_validity_name, DvzRenderProductValidity, "none", "full_extent",
    "explicit_coverage", "background_value", "integer_sentinel")
DVZ_PRODUCT_NAME_FUNCTION(
    _frame_plan_product_validity_requirement_name, DvzRenderProductValidityRequirement, "none",
    "any_defined", "full_extent", "explicit_coverage", "background_value", "integer_sentinel")

#undef DVZ_PRODUCT_NAME_FUNCTION



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Append one immutable semantic render-product contract to a FramePlan.
 *
 * @param plan the FramePlan
 * @param product the semantic product contract
 * @return whether the product was appended
 */
bool dvz_frame_plan_product(DvzFramePlan* plan, const DvzRenderProductContract* product)
{
    if (plan == NULL || product == NULL || product->id.value == 0)
        return false;
    if (!_ensure_product_capacity(plan))
    {
        log_error("cannot grow FramePlan semantic product list");
        return false;
    }
    DvzRenderProductContract* dst = &plan->products[plan->product_count++];
    dvz_memset(dst, sizeof(DvzRenderProductContract), 0, sizeof(DvzRenderProductContract));
    dvz_memcpy(dst, sizeof(DvzRenderProductContract), product, sizeof(DvzRenderProductContract));
    return true;
}



/**
 * Declare one typed consumer use for a product already present in a FramePlan.
 *
 * @param plan the FramePlan
 * @param product_id the plan-local product identity
 * @param pass_index the consumer graph-pass index
 * @param validity_requirement the validity representation required by the consumer
 * @return whether the use was appended
 */
bool dvz_frame_plan_product_consumer(
    DvzFramePlan* plan, DvzRenderProductId product_id, uint32_t pass_index,
    DvzRenderProductValidityRequirement validity_requirement)
{
    if (plan == NULL || product_id.value == 0 ||
        validity_requirement == DVZ_RENDER_PRODUCT_VALIDITY_REQUIREMENT_NONE ||
        !_product_index(plan, product_id, NULL) || !_ensure_product_use_capacity(plan))
        return false;
    plan->product_uses[plan->product_use_count++] = (DvzRenderProductConsumer){
        .product_id = product_id,
        .pass_index = pass_index,
        .validity_requirement = validity_requirement,
    };
    return true;
}



/**
 * Return the number of semantic render products in a FramePlan.
 *
 * @param plan the FramePlan
 * @return the product count
 */
uint32_t dvz_frame_plan_product_count(const DvzFramePlan* plan)
{
    return plan != NULL ? plan->product_count : 0;
}



/**
 * Return one immutable semantic render-product contract by plan-local index.
 *
 * @param plan the FramePlan
 * @param index the product array index
 * @return the product contract, or NULL when the index is out of bounds
 */
const DvzRenderProductContract*
dvz_frame_plan_product_get(const DvzFramePlan* plan, uint32_t index)
{
    if (plan == NULL || index >= plan->product_count)
        return NULL;
    return &plan->products[index];
}



/**
 * Validate all semantic products, uses, physical realizations, and live intervals.
 *
 * @param plan the FramePlan
 * @param report the diagnostic report
 * @return whether all semantic product contracts are valid
 */
bool dvz_frame_plan_products_validate(const DvzFramePlan* plan, DvzDiagnosticReport* report)
{
    if (plan == NULL)
        return false;
    bool ok = true;
    for (uint32_t i = 0; i < plan->product_use_count; i++)
    {
        if (!_product_index(plan, plan->product_uses[i].product_id, NULL))
        {
            _frame_plan_graph_report(
                report,
                "FramePlan product use at index %" PRIu32
                " references unknown product id %" PRIu32,
                i, plan->product_uses[i].product_id.value);
            ok = false;
        }
    }
    for (uint32_t i = 0; i < plan->product_count; i++)
    {
        const DvzRenderProductContract* product = &plan->products[i];
        bool complete = product->id.value != 0 && product->version > 0 &&
                        product->kind != DVZ_RENDER_PRODUCT_NONE &&
                        product->domain != DVZ_RENDER_PRODUCT_DOMAIN_NONE &&
                        product->extent_policy != DVZ_RENDER_PRODUCT_EXTENT_NONE &&
                        product->rounding_policy != DVZ_RENDER_PRODUCT_ROUND_NONE &&
                        product->format_class != DVZ_RENDER_PRODUCT_FORMAT_NONE &&
                        product->concrete_format != 0 &&
                        product->sample_domain != DVZ_RENDER_PRODUCT_SAMPLES_NONE &&
                        product->coordinate_space != DVZ_RENDER_PRODUCT_COORDINATES_NONE &&
                        product->encoding != DVZ_RENDER_PRODUCT_ENCODING_NONE &&
                        product->validity != DVZ_RENDER_PRODUCT_VALIDITY_NONE &&
                        product->required_usage_flags != 0 && product->width > 0 &&
                        product->height > 0 && isfinite(product->render_scale) &&
                        product->render_scale > 0 &&
                        product->lifetime != DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_NONE;
        for (uint32_t j = 0; j < 4; j++)
        {
            complete = complete && isfinite(product->local_to_target[j]);
            if (product->has_background_value)
                complete = complete && isfinite(product->background_value[j]);
        }
        if (!complete)
        {
            _frame_plan_graph_report(
                report, "FramePlan product at index %" PRIu32 " is semantically incomplete", i);
            ok = false;
            continue;
        }
        for (uint32_t j = i + 1; j < plan->product_count; j++)
        {
            if (product->id.value == plan->products[j].id.value)
            {
                _frame_plan_graph_report(
                    report, "FramePlan product id %" PRIu32 " is duplicated", product->id.value);
                ok = false;
            }
        }
        if (!_product_kind_contract_valid(product))
        {
            _frame_plan_graph_report(
                report,
                "FramePlan product '%s' kind is incompatible with its semantic format or encoding",
                _product_label(product));
            ok = false;
        }
        if (_product_panel_scoped(product) &&
            (product->panel_id[0] == '\0' || product->view_id[0] == '\0' ||
             product->camera_id[0] == '\0' || product->projection_id[0] == '\0' ||
             product->coordinate_space != DVZ_RENDER_PRODUCT_COORDINATES_PANEL_LOCAL))
        {
            _frame_plan_graph_report(
                report, "FramePlan panel product '%s' lacks panel-local scope",
                _product_label(product));
            ok = false;
        }
        if (_surface_member(product->kind) && product->surface_record_id.value == 0)
        {
            _frame_plan_graph_report(
                report, "FramePlan surface product '%s' lacks a coherent surface-record identity",
                _product_label(product));
            ok = false;
        }
        if (product->validity == DVZ_RENDER_PRODUCT_VALIDITY_EXPLICIT_COVERAGE)
        {
            uint32_t validity_index = 0;
            if (!_product_index(plan, product->validity_product_id, &validity_index) ||
                plan->products[validity_index].kind != DVZ_RENDER_PRODUCT_SURFACE_COVERAGE ||
                plan->products[validity_index].surface_record_id.value !=
                    product->surface_record_id.value ||
                !_surface_pair_compatible(plan, product, &plan->products[validity_index]))
            {
                _frame_plan_graph_report(
                    report, "FramePlan product '%s' lacks a compatible explicit coverage product",
                    _product_label(product));
                ok = false;
            }
        }
        if (product->validity == DVZ_RENDER_PRODUCT_VALIDITY_BACKGROUND_VALUE &&
            !product->has_background_value)
        {
            _frame_plan_graph_report(
                report, "FramePlan product '%s' lacks its declared background value",
                _product_label(product));
            ok = false;
        }
        if (product->validity == DVZ_RENDER_PRODUCT_VALIDITY_INTEGER_SENTINEL &&
            !product->has_integer_sentinel)
        {
            _frame_plan_graph_report(
                report, "FramePlan product '%s' lacks its declared integer sentinel",
                _product_label(product));
            ok = false;
        }
        for (uint32_t j = 0; j < plan->product_use_count; j++)
        {
            const DvzRenderProductConsumer* first_use = &plan->product_uses[j];
            if (first_use->product_id.value != product->id.value)
                continue;
            for (uint32_t k = j + 1; k < plan->product_use_count; k++)
            {
                const DvzRenderProductConsumer* second_use = &plan->product_uses[k];
                if (second_use->product_id.value == product->id.value &&
                    first_use->pass_index == second_use->pass_index)
                {
                    _frame_plan_graph_report(
                        report, "FramePlan product '%s' duplicates consumer pass index %" PRIu32,
                        _product_label(product), first_use->pass_index);
                    ok = false;
                }
            }
        }
        ok = _validate_product_samples(plan, product, report) && ok;
        ok = _validate_product_resource(plan, product, report) && ok;
        ok = _validate_product_source(plan, product, report) && ok;
        ok = _validate_product_passes(plan, product, report) && ok;
    }

    for (uint32_t i = 0; i < plan->product_count; i++)
    {
        const DvzRenderProductContract* first = &plan->products[i];
        if (!_surface_member(first->kind) || first->surface_record_id.value == 0)
            continue;
        bool already_checked = false;
        for (uint32_t j = 0; j < i; j++)
            already_checked = already_checked || first->surface_record_id.value ==
                                                     plan->products[j].surface_record_id.value;
        if (already_checked)
            continue;
        uint32_t members = 0;
        uint32_t kinds = 0;
        for (uint32_t j = 0; j < plan->product_count; j++)
        {
            const DvzRenderProductContract* second = &plan->products[j];
            if (first->surface_record_id.value != second->surface_record_id.value)
                continue;
            if (!_surface_member(second->kind) || !_surface_pair_compatible(plan, first, second))
            {
                _frame_plan_graph_report(
                    report,
                    "FramePlan surface record %" PRIu32 " has incompatible product members",
                    first->surface_record_id.value);
                ok = false;
                break;
            }
            members++;
            kinds |= 1u << (uint32_t)(second->kind - DVZ_RENDER_PRODUCT_SURFACE_DEPTH);
        }
        if (members != 3 || kinds != 0x07u)
        {
            _frame_plan_graph_report(
                report,
                "FramePlan surface record %" PRIu32 " must contain depth, normal, and coverage",
                first->surface_record_id.value);
            ok = false;
        }
    }

    for (uint32_t pass_index = 0; pass_index < plan->graph_pass_count; pass_index++)
    {
        uint32_t surface_record_id = 0;
        uint32_t surface_kinds = 0;
        for (uint32_t i = 0; i < plan->product_count; i++)
        {
            const DvzRenderProductContract* product = &plan->products[i];
            if (!_surface_member(product->kind) ||
                !_product_has_consumer(plan, product, pass_index))
                continue;
            if (surface_record_id == 0)
                surface_record_id = product->surface_record_id.value;
            if (surface_record_id != product->surface_record_id.value)
            {
                _frame_plan_graph_report(
                    report, "FramePlan consumer pass '%s' mixes incompatible surface records",
                    plan->graph_passes[pass_index].id);
                ok = false;
            }
            uint32_t kind_bit = 1u << (uint32_t)(product->kind - DVZ_RENDER_PRODUCT_SURFACE_DEPTH);
            if ((surface_kinds & kind_bit) != 0)
            {
                _frame_plan_graph_report(
                    report,
                    "FramePlan consumer pass '%s' binds multiple products for one surface role",
                    plan->graph_passes[pass_index].id);
                ok = false;
            }
            surface_kinds |= kind_bit;
        }
    }

    for (uint32_t i = 0; i < plan->product_count; i++)
    {
        const DvzRenderProductContract* first = &plan->products[i];
        for (uint32_t j = i + 1; j < plan->product_count; j++)
        {
            const DvzRenderProductContract* second = &plan->products[j];
            if (first->resource_index != second->resource_index)
                continue;
            uint32_t first_end = _product_last_use(plan, first);
            uint32_t second_end = _product_last_use(plan, second);
            bool overlaps = first->producer_pass_index <= second_end &&
                            second->producer_pass_index <= first_end;
            if (overlaps)
            {
                _frame_plan_graph_report(
                    report, "FramePlan products '%s' and '%s' have overlapping physical aliases",
                    _product_label(first), _product_label(second));
                ok = false;
            }
        }
    }

    for (uint32_t resource_index = 0; resource_index < plan->graph_resource_count;
         resource_index++)
    {
        bool product_linked = false;
        for (uint32_t i = 0; i < plan->product_count; i++)
            product_linked = product_linked || plan->products[i].resource_index == resource_index;
        if (!product_linked)
            continue;
        const char* resource_id = plan->graph_resources[resource_index].id;
        for (uint32_t pass_index = 0; pass_index < plan->graph_pass_count; pass_index++)
        {
            const DvzFrameGraphPass* pass = &plan->graph_passes[pass_index];
            uint32_t producers = 0;
            uint32_t consumers = 0;
            for (uint32_t i = 0; i < plan->product_count; i++)
            {
                const DvzRenderProductContract* product = &plan->products[i];
                if (product->resource_index != resource_index)
                    continue;
                producers += product->producer_pass_index == pass_index ? 1 : 0;
                consumers += _product_has_consumer(plan, product, pass_index) ? 1 : 0;
            }
            if (_frame_plan_graph_pass_writes_resource(pass, resource_id) && producers != 1)
            {
                _frame_plan_graph_report(
                    report,
                    "FramePlan pass '%s' writes product-linked resource '%s' without exactly one "
                    "product version",
                    pass->id, resource_id);
                ok = false;
            }
            if (_pass_reads_resource(pass, resource_id) && consumers != 1)
            {
                _frame_plan_graph_report(
                    report,
                    "FramePlan pass '%s' reads product-linked resource '%s' without exactly one "
                    "product use",
                    pass->id, resource_id);
                ok = false;
            }
        }
    }
    return ok;
}
