/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene visual field binding                                                                   */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_log.h"
#include "_scene.h"
#include "core/scene_notify_internal.h"
#include "field_internal.h"
#include "sample_profile.h"
#include "visuals/bindings_internal.h"
#include "_visual_internal.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Ensure one image-like visual owns a compatible sampled field.
 *
 * @param visual the image or glyph visual
 * @param format the field format
 * @param semantic the field semantic
 * @param width the field width
 * @param height the field height
 * @return the owned field, or NULL on error
 */
static DvzSampledField* _scene_ensure_owned_image_field(
    DvzVisual* visual, DvzFieldFormat format, DvzFieldSemantic semantic, uint32_t width,
    uint32_t height)
{
    ANN(visual);
    DvzSampledField* field = _visual_family_state(visual)->field_owned ? _visual_family_state(visual)->field : NULL;
    if (field != NULL && field->desc.format == format && field->desc.width == width &&
        field->desc.height == height && field->desc.depth == 1)
    {
        return field;
    }
    if (field != NULL)
        dvz_sampled_field_destroy(field);
    DvzSampledFieldDesc desc = dvz_sampled_field_desc();
    desc.format = format;
    desc.semantic = semantic;
    desc.width = width;
    desc.height = height;
    field = dvz_sampled_field(visual->scene, &desc);
    return field;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Clear all visual bindings that currently reference one sampled field.
 *
 * @param field the sampled field being released
 */
void _scene_release_field_bindings(DvzSampledField* field)
{
    if (field == NULL || field->scene == NULL)
        return;
    DvzScene* scene = field->scene;
    for (uint32_t i = 0; i < scene->visual_count; i++)
    {
        DvzVisual* visual = &scene->visuals[i];
        DvzVisualFamilyState* state = _visual_family_state(visual);
        if (state == NULL || state->field != field)
            continue;
        _visual_binding_clear(visual, DVZ_VISUAL_BINDING_FIELD);
        _scene_visual_texture_mark_clean(visual);
        if (state->texture.upload != NULL)
        {
            dvz_free(state->texture.upload);
            state->texture.upload = NULL;
            state->texture.upload_size = 0;
        }
    }
}



/**
 * Bind a retained sampled field to a texture-backed visual.
 *
 * @param visual the visual
 * @param slot_name the field slot name
 * @param field the sampled field, or NULL to clear the binding
 * @return true on success, false on error
 */
bool dvz_visual_set_field(DvzVisual* visual, const char* slot_name, DvzSampledField* field)
{
    ANN(visual);
    ANN(slot_name);
    if (field != NULL && field->scene != visual->scene)
    {
        log_error("cannot bind a sampled field from a different scene");
        return false;
    }
    if (visual->type != DVZ_VISUAL_TYPE_IMAGE && visual->type != DVZ_VISUAL_TYPE_GLYPH &&
        visual->type != DVZ_VISUAL_TYPE_VOLUME && visual->type != DVZ_VISUAL_TYPE_LABELS &&
        visual->type != DVZ_VISUAL_TYPE_MESH)
    {
        log_error(
            "dvz_visual_set_field is only supported for image, glyph, volume, labels, and mesh visuals");
        return false;
    }
    bool mesh_texture_slot =
        visual->type == DVZ_VISUAL_TYPE_MESH && strcmp(slot_name, "texture") == 0;
    if (strcmp(slot_name, "field") != 0 && !mesh_texture_slot)
    {
        log_error(
            "unsupported visual field slot '%s' (expected 'field' or mesh 'texture')",
            slot_name);
        return false;
    }
    if (field != NULL &&
        (visual->type == DVZ_VISUAL_TYPE_IMAGE || visual->type == DVZ_VISUAL_TYPE_GLYPH ||
         visual->type == DVZ_VISUAL_TYPE_LABELS || visual->type == DVZ_VISUAL_TYPE_MESH) &&
        field->desc.dim != DVZ_FIELD_DIM_2D)
    {
        log_error("image, glyph, labels, and mesh visuals require a 2D sampled field");
        return false;
    }
    if (field != NULL && visual->type == DVZ_VISUAL_TYPE_MESH &&
        field->desc.format != DVZ_FIELD_FORMAT_RGBA8_UNORM)
    {
        log_error("mesh texture fields require RGBA8_UNORM format in the first slice");
        return false;
    }
    if (field != NULL && visual->type == DVZ_VISUAL_TYPE_LABELS &&
        field->desc.semantic != DVZ_FIELD_SEMANTIC_LABEL)
    {
        log_error("labels visuals require a sampled field with LABEL semantic");
        return false;
    }
    DvzSceneSampleProfile profile = {0};
    bool supported_profile =
        field != NULL &&
        _scene_sample_profile_resolve(
            field->desc.format, field->desc.semantic, field->desc.dim, &profile);
    if (field != NULL && visual->type == DVZ_VISUAL_TYPE_LABELS && !supported_profile)
    {
        log_error("labels visuals require an R8/R16/R32 signed or unsigned integer field");
        return false;
    }
    if (field != NULL && visual->type == DVZ_VISUAL_TYPE_VOLUME &&
        field->desc.dim != DVZ_FIELD_DIM_3D)
    {
        log_error("volume visuals require a 3D sampled field");
        return false;
    }
    if (
        field != NULL && visual->type == DVZ_VISUAL_TYPE_VOLUME && supported_profile &&
        _scene_sample_profile_is_integer_label(&profile) &&
        _visual_family_state(visual)->volume.render_mode == DVZ_VOLUME_RENDER_MIP)
    {
        log_error("label volumes only support slice and composite render modes");
        return false;
    }
    if (!_scene_visual_mutation_allowed(visual->scene, "bind sampled field"))
        return false;

    if (_visual_family_state(visual)->field != field)
        _scene_release_visual_field(visual);
    if (field != NULL)
    {
        _visual_binding_assign(visual, DVZ_VISUAL_BINDING_FIELD, slot_name, field, false);
        _scene_visual_texture_mark_clean(visual);
        _scene_visual_texture_mark_dirty(visual);
    }
    else
    {
        _visual_binding_clear(visual, DVZ_VISUAL_BINDING_FIELD);
    }
    _scene_notify_visual_changed(visual);
    return true;
}



/**
 * Attach an RGBA8 2D texture to an image or glyph visual.
 *
 * @param visual the visual
 * @param rgba the RGBA8 pixel data
 * @param width the texture width in pixels
 * @param height the texture height in pixels
 * @return 0 on success, -1 on error
 */
int dvz_visual_set_texture(
    DvzVisual* visual, const void* rgba, uint32_t width, uint32_t height)
{
    ANN(visual);
    if (visual->type != DVZ_VISUAL_TYPE_IMAGE && visual->type != DVZ_VISUAL_TYPE_GLYPH)
    {
        log_error("dvz_visual_set_texture is only supported for image and glyph visuals");
        return -1;
    }
    if (rgba == NULL || width == 0 || height == 0)
    {
        log_error("dvz_visual_set_texture: NULL data or zero extent (%ux%u)", width, height);
        return -1;
    }
    if (!_scene_visual_mutation_allowed(visual->scene, "set image texture"))
        return -1;
    DvzSampledField* field = _scene_ensure_owned_image_field(
        visual, DVZ_FIELD_FORMAT_RGBA8_UNORM, DVZ_FIELD_SEMANTIC_COLOR, width, height);
    if (field == NULL)
        return -1;
    if (!dvz_sampled_field_set_data(
            field, &(DvzFieldDataView){DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView),
                       .data = rgba,
                       .bytes_per_row = (uint64_t)width * 4u,
                       .rows_per_image = height,
                   }))
        return -1;
    if (!dvz_visual_set_field(visual, "field", field))
        return -1;
    _visual_binding_assign(visual, DVZ_VISUAL_BINDING_FIELD, "field", field, true);
    return 0;
}



/**
 * Attach a 2D scalar F32 texture to an image or glyph visual.
 *
 * The scalar data must remain valid until emit time. The bound scale and
 * colormap are applied on the CPU during emit to produce the RGBA texture used
 * by the current first-slice image runtime path.
 *
 * @param visual the visual (must be of type IMAGE or GLYPH)
 * @param values scalar F32 pixel data, tightly packed, row-major
 * @param width the texture width in pixels
 * @param height the texture height in pixels
 * @return 0 on success, -1 on error
 */
int dvz_visual_set_texture_f32(
    DvzVisual* visual, const float* values, uint32_t width, uint32_t height)
{
    ANN(visual);
    if (visual->type != DVZ_VISUAL_TYPE_IMAGE && visual->type != DVZ_VISUAL_TYPE_GLYPH)
    {
        log_error("dvz_visual_set_texture_f32 is only supported for image and glyph visuals");
        return -1;
    }
    if (values == NULL || width == 0 || height == 0)
    {
        log_error("dvz_visual_set_texture_f32: NULL data or zero extent (%ux%u)", width, height);
        return -1;
    }
    if (!_scene_visual_mutation_allowed(visual->scene, "set scalar image texture"))
        return -1;
    DvzSampledField* field = _scene_ensure_owned_image_field(
        visual, DVZ_FIELD_FORMAT_R32_FLOAT, DVZ_FIELD_SEMANTIC_SCALAR, width, height);
    if (field == NULL)
        return -1;
    if (!dvz_sampled_field_set_data(
            field, &(DvzFieldDataView){DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView),
                       .data = values,
                       .bytes_per_row = (uint64_t)width * sizeof(float),
                       .rows_per_image = height,
                   }))
        return -1;
    if (!dvz_visual_set_field(visual, "field", field))
        return -1;
    _visual_binding_assign(visual, DVZ_VISUAL_BINDING_FIELD, "field", field, true);
    return 0;
}



/**
 * Release any sampled-field binding owned by a visual.
 *
 * @param visual the visual
 */
void _scene_release_visual_field(DvzVisual* visual)
{
    if (visual == NULL)
        return;
    DvzVisualFamilyState* state = _visual_family_state(visual);
    if (state == NULL)
        return;
    DvzSampledField* field = state->field;
    const DvzVisualBinding* binding = _visual_binding_const(visual, DVZ_VISUAL_BINDING_FIELD);
    bool owned = binding != NULL ? binding->owned : state->field_owned;
    _visual_binding_clear(visual, DVZ_VISUAL_BINDING_FIELD);
    _scene_visual_texture_mark_clean(visual);
    if (state->texture.upload != NULL)
    {
        dvz_free(state->texture.upload);
        state->texture.upload = NULL;
        state->texture.upload_size = 0;
    }
    if (owned && field != NULL)
        dvz_sampled_field_destroy(field);
}
