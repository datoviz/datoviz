/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene visual facade                                                                          */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdio.h>
#include <string.h>

#include "_visual_internal.h"
#include "registry/registry.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static DvzSceneVisualFamily _visual_family_from_type(DvzVisualType type)
{
    switch (type)
    {
    case DVZ_VISUAL_TYPE_POINT:
        return DVZ_SCENE_VISUAL_FAMILY_POINT;
    case DVZ_VISUAL_TYPE_PIXEL:
        return DVZ_SCENE_VISUAL_FAMILY_PIXEL;
    case DVZ_VISUAL_TYPE_MARKER:
        return DVZ_SCENE_VISUAL_FAMILY_MARKER;
    case DVZ_VISUAL_TYPE_SEGMENT:
        return DVZ_SCENE_VISUAL_FAMILY_SEGMENT;
    case DVZ_VISUAL_TYPE_VECTOR:
        return DVZ_SCENE_VISUAL_FAMILY_VECTOR;
    case DVZ_VISUAL_TYPE_PATH:
        return DVZ_SCENE_VISUAL_FAMILY_PATH;
    case DVZ_VISUAL_TYPE_IMAGE:
        return DVZ_SCENE_VISUAL_FAMILY_IMAGE;
    case DVZ_VISUAL_TYPE_MESH:
        return DVZ_SCENE_VISUAL_FAMILY_MESH;
    case DVZ_VISUAL_TYPE_VOLUME:
        return DVZ_SCENE_VISUAL_FAMILY_VOLUME;
    case DVZ_VISUAL_TYPE_PRIMITIVE:
        return DVZ_SCENE_VISUAL_FAMILY_PRIMITIVE;
    case DVZ_VISUAL_TYPE_SPHERE:
        return DVZ_SCENE_VISUAL_FAMILY_SPHERE;
    case DVZ_VISUAL_TYPE_GLYPH:
        return DVZ_SCENE_VISUAL_FAMILY_GLYPH;
    case DVZ_VISUAL_TYPE_TEXT:
        return DVZ_SCENE_VISUAL_FAMILY_TEXT;
    case DVZ_VISUAL_TYPE_LABELS:
        return DVZ_SCENE_VISUAL_FAMILY_LABELS;
    case DVZ_VISUAL_TYPE_SPLAT:
        return DVZ_SCENE_VISUAL_FAMILY_SPLAT;
    default:
        return DVZ_SCENE_VISUAL_FAMILY_NONE;
    }
}


static DvzVisualType _visual_type_from_family(DvzSceneVisualFamily family)
{
    switch (family)
    {
    case DVZ_SCENE_VISUAL_FAMILY_POINT:
        return DVZ_VISUAL_TYPE_POINT;
    case DVZ_SCENE_VISUAL_FAMILY_PIXEL:
        return DVZ_VISUAL_TYPE_PIXEL;
    case DVZ_SCENE_VISUAL_FAMILY_MARKER:
        return DVZ_VISUAL_TYPE_MARKER;
    case DVZ_SCENE_VISUAL_FAMILY_SEGMENT:
        return DVZ_VISUAL_TYPE_SEGMENT;
    case DVZ_SCENE_VISUAL_FAMILY_VECTOR:
        return DVZ_VISUAL_TYPE_VECTOR;
    case DVZ_SCENE_VISUAL_FAMILY_PATH:
        return DVZ_VISUAL_TYPE_PATH;
    case DVZ_SCENE_VISUAL_FAMILY_IMAGE:
        return DVZ_VISUAL_TYPE_IMAGE;
    case DVZ_SCENE_VISUAL_FAMILY_MESH:
        return DVZ_VISUAL_TYPE_MESH;
    case DVZ_SCENE_VISUAL_FAMILY_VOLUME:
        return DVZ_VISUAL_TYPE_VOLUME;
    case DVZ_SCENE_VISUAL_FAMILY_PRIMITIVE:
        return DVZ_VISUAL_TYPE_PRIMITIVE;
    case DVZ_SCENE_VISUAL_FAMILY_SPHERE:
        return DVZ_VISUAL_TYPE_SPHERE;
    case DVZ_SCENE_VISUAL_FAMILY_GLYPH:
        return DVZ_VISUAL_TYPE_GLYPH;
    case DVZ_SCENE_VISUAL_FAMILY_TEXT:
        return DVZ_VISUAL_TYPE_TEXT;
    case DVZ_SCENE_VISUAL_FAMILY_LABELS:
        return DVZ_VISUAL_TYPE_LABELS;
    case DVZ_SCENE_VISUAL_FAMILY_SPLAT:
        return DVZ_VISUAL_TYPE_SPLAT;
    default:
        return DVZ_VISUAL_TYPE_NONE;
    }
}


static const char* _visual_public_attr_name(
    const DvzVisualFamilyOps* ops, const DvzVisualFamilyAttrDesc* attr)
{
    if (ops == NULL || attr == NULL || attr->name == NULL)
        return NULL;
    if (ops->attr_alias_public != NULL && ops->attr_alias_storage != NULL &&
        strcmp(attr->name, ops->attr_alias_storage) == 0)
        return ops->attr_alias_public;
    return attr->name;
}



/*************************************************************************************************/
/*  Visual introspection                                                                         */
/*************************************************************************************************/

DvzSceneVisualFamily dvz_visual_family(const DvzVisual* visual)
{
    return visual != NULL ? _visual_family_from_type(visual->type) : DVZ_SCENE_VISUAL_FAMILY_NONE;
}


const char* dvz_visual_family_name(DvzSceneVisualFamily family)
{
    if (family == DVZ_SCENE_VISUAL_FAMILY_NONE)
        return "none";
    const DvzVisualFamilyOps* ops = _scene_visual_family_ops(_visual_type_from_family(family));
    return ops != NULL && ops->name != NULL ? ops->name : "none";
}


uint32_t dvz_visual_attr_count(const DvzVisual* visual)
{
    if (visual == NULL)
        return 0;
    const DvzVisualFamilyOps* ops = visual->ops != NULL ? visual->ops :
                                                           _scene_visual_family_ops(visual->type);
    return ops != NULL ? ops->attr_count : 0;
}


DvzResult dvz_visual_attr_info(
    const DvzVisual* visual, uint32_t index, DvzVisualAttrInfo* out)
{
    if (visual == NULL || out == NULL)
        return DVZ_ERROR;
    const DvzVisualFamilyOps* ops = visual->ops != NULL ? visual->ops :
                                                           _scene_visual_family_ops(visual->type);
    if (ops == NULL || index >= ops->attr_count)
        return DVZ_ERROR;

    const DvzVisualFamilyAttrDesc* attr = &ops->attrs[index];
    const char* name = _visual_public_attr_name(ops, attr);
    *out = (DvzVisualAttrInfo){
        .name = name,
        .item_size = attr->item_size,
        .source_mask = attr->source_mask,
        .default_format = _attr_default_format(visual->type, name),
        .instance = attr->instance,
    };
    return DVZ_OK;
}


bool dvz_visual_attr_supported(const DvzVisual* visual, const char* attr_name)
{
    if (visual == NULL || attr_name == NULL)
        return false;
    return _visual_family_attr_desc(visual->type, attr_name) != NULL;
}


static void _visual_validation_report(DvzDiagnosticReport* report, const char* message)
{
    if (report != NULL && message != NULL)
        (void)dvz_diagnostic_report_add(report, message);
}


bool dvz_visual_validate(const DvzVisual* visual, DvzDiagnosticReport* report)
{
    bool ok = true;
    if (visual == NULL)
    {
        _visual_validation_report(report, "visual validation failed: visual is NULL");
        return false;
    }
    const DvzVisualFamilyOps* ops = visual->ops != NULL ? visual->ops :
                                                           _scene_visual_family_ops(visual->type);
    if (visual->scene == NULL)
    {
        _visual_validation_report(report, "visual validation failed: visual has no scene");
        ok = false;
    }
    if (ops == NULL)
    {
        _visual_validation_report(report, "visual validation failed: unknown visual family");
        return false;
    }

    char message[DVZ_SCENE_DIAGNOSTIC_SIZE];
    for (uint32_t i = 0; i < visual->attr_count; i++)
    {
        const DvzVisualAttr* attr = &visual->attrs[i];
        if (attr->name[0] == '\0')
        {
            snprintf(
                message, sizeof(message),
                "visual validation failed: visual %llu attribute %u has no name",
                (unsigned long long)visual->id, i);
            _visual_validation_report(report, message);
            ok = false;
            continue;
        }
        if (_visual_family_attr_desc(visual->type, attr->name) == NULL)
        {
            snprintf(
                message, sizeof(message),
                "visual validation failed: %s visual attribute '%s' is unsupported", ops->name,
                attr->name);
            _visual_validation_report(report, message);
            ok = false;
            continue;
        }
        if (!_attr_source_supported(visual->type, attr->name, attr->source))
        {
            snprintf(
                message, sizeof(message),
                "visual validation failed: %s visual attribute '%s' has unsupported source %d",
                ops->name, attr->name, (int)attr->source);
            _visual_validation_report(report, message);
            ok = false;
        }
        uint32_t expected_size =
            _attr_item_size_for_format(visual->type, attr->name, attr->format);
        if (expected_size == 0 || attr->item_size != expected_size)
        {
            snprintf(
                message, sizeof(message),
                "visual validation failed: %s visual attribute '%s' has item size %u, expected "
                "%u",
                ops->name, attr->name, attr->item_size, expected_size);
            _visual_validation_report(report, message);
            ok = false;
        }
        if (attr->item_count > 0 && attr->data == NULL && attr->buffer == NULL)
        {
            snprintf(
                message, sizeof(message),
                "visual validation failed: %s visual attribute '%s' has items but no payload",
                ops->name, attr->name);
            _visual_validation_report(report, message);
            ok = false;
        }
    }
    return ok;
}
