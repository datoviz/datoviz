/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene visual material */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <float.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_overflow.h"
#include "_scene.h"
#include "core/scene_notify_internal.h"
#include "_scene_resource_key.h"
#include "_visual_internal.h"
#include "datoviz/scene.h"
#include "registry/registry.h"


/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

#define DVZ_MATERIAL_DESC_KNOWN_FLAGS  0u
#define DVZ_DEPTH_CUE_DESC_KNOWN_FLAGS 0u

/**
 * Initialize material uniform defaults.
 *
 * @param params the material parameter payload
 */
void _material_params_default(DvzSceneMaterialParams* params)
{
    ANN(params);
    dvz_memset(params, sizeof(DvzSceneMaterialParams), 0, sizeof(DvzSceneMaterialParams));
    params->light_direction[2] = 1.0f;
    params->params[0] = 0.2f;
    params->params[1] = 0.8f;
    params->params[2] = 0.25f;
    params->params[3] = 32.0f;
    params->model[0] = (float)DVZ_MATERIAL_MODEL_PHONG;
    params->model[1] = 1.0f;
    params->base_color_factor[0] = 1.0f;
    params->base_color_factor[1] = 1.0f;
    params->base_color_factor[2] = 1.0f;
    params->base_color_factor[3] = 1.0f;
    params->standard_params[0] = 0.5f;
    params->standard_params[1] = 0.5f;
    params->depth_cue[1] = 1.0f;
    params->depth_cue[2] = 1.0f;
    params->depth_cue_extra[2] = 3.0f;
}



/**
 * Return the default public material descriptor.
 *
 * @return default material descriptor
 */
DvzMaterialDesc dvz_material_desc(void)
{
    DvzMaterialDesc desc = {
        DVZ_STRUCT_INIT_FIELDS(DvzMaterialDesc),
        .model = DVZ_MATERIAL_MODEL_PHONG,
        .alpha_mode = DVZ_ALPHA_OPAQUE,
        .opacity = 1.0f,
        .base_color_factor = {1.0f, 1.0f, 1.0f, 1.0f},
        .light_direction = {0.0f, 0.0f, 1.0f},
        .phong = {.ambient = 0.2f, .diffuse = 0.8f, .specular = 0.25f, .shininess = 32.0f},
        .standard = {.roughness = 0.5f, .specular = 0.5f},
    };
    return desc;
}


DvzDepthCueDesc dvz_depth_cue_desc(void)
{
    return (DvzDepthCueDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzDepthCueDesc),
        .mode = DVZ_DEPTH_CUE_FADE_TO_BACKGROUND,
        .metric = DVZ_DEPTH_CUE_METRIC_CLIP_DEPTH,
        .falloff = DVZ_DEPTH_CUE_FALLOFF_LINEAR,
        .near_depth = 0.0f,
        .far_depth = 1.0f,
        .strength = 1.0f,
        .density = 3.0f,
        .background_color = {0.0f, 0.0f, 0.0f, 1.0f},
    };
}



/**
 * Return the default public Phong material descriptor.
 *
 * @return default Phong material descriptor
 */
DvzMaterialDesc dvz_phong_material_desc(void)
{
    DvzMaterialDesc desc = dvz_material_desc();
    desc.model = DVZ_MATERIAL_MODEL_PHONG;
    return desc;
}



/**
 * Return the default public standard material descriptor.
 *
 * @return default standard material descriptor
 */
DvzMaterialDesc dvz_standard_material_desc(void)
{
    DvzMaterialDesc desc = dvz_material_desc();
    desc.model = DVZ_MATERIAL_MODEL_STANDARD;
    return desc;
}



/**
 * Return default circular point styling.
 *
 * @return default point style descriptor
 */



/**
 * Initialize material defaults for one visual family.
 *
 * @param material the material state
 * @param visual_type the retained visual type
 */
void _material_state_default(DvzSceneMaterialState* material, DvzVisualType visual_type)
{
    ANN(material);
    dvz_memset(material, sizeof(DvzSceneMaterialState), 0, sizeof(DvzSceneMaterialState));
    DvzMaterialDesc desc = dvz_material_desc();
    _material_state_apply_desc(material, &desc);
    material->alpha_mode = DVZ_ALPHA_OPAQUE;
    material->scalar_scale = 1.0f;
    material->point_style = dvz_point_style_desc();
    material->point_style_enabled = false;

    const DvzVisualFamilyOps* ops = _scene_visual_family_ops(visual_type);
    material->kind = ops != NULL ? ops->default_material_kind : DVZ_MATERIAL_KIND_UNLIT;
    material->model = ops != NULL ? ops->default_material_model : DVZ_MATERIAL_MODEL_UNLIT;
}



/**
 * Return whether one visual family supports shared surface material parameters.
 *
 * @param visual_type the retained visual type
 * @return whether shared material parameters are supported
 */
bool _material_visual_supported(DvzVisualType visual_type)
{
    const DvzVisualFamilyOps* ops = _scene_visual_family_ops(visual_type);
    return ops != NULL && ops->supports_material;
}



/**
 * Return whether a material model enum value is valid.
 *
 * @param model the material model
 * @return whether the model is valid
 */
bool _material_model_valid(DvzMaterialModel model)
{
    return model >= DVZ_MATERIAL_MODEL_UNLIT && model <= DVZ_MATERIAL_MODEL_STANDARD;
}



/**
 * Return whether an alpha-mode enum value is valid.
 *
 * @param mode the alpha mode
 * @return whether the alpha mode is valid
 */
bool _material_alpha_mode_valid(DvzAlphaMode mode)
{
    return mode >= DVZ_ALPHA_OPAQUE && mode <= DVZ_ALPHA_MASK;
}



/**
 * Return whether one public material descriptor is usable.
 *
 * @param desc the material descriptor
 * @return whether the descriptor is valid
 */
bool _material_desc_valid(const DvzMaterialDesc* desc)
{
    ANN(desc);
    if (!DVZ_STRUCT_VALID(desc, DvzMaterialDesc, DVZ_MATERIAL_DESC_KNOWN_FLAGS))
    {
        log_error("invalid DvzMaterialDesc ABI prologue");
        return false;
    }
    if (!_material_model_valid(desc->model))
    {
        log_error("invalid material model %d", (int)desc->model);
        return false;
    }
    if (!_material_alpha_mode_valid(desc->alpha_mode))
    {
        log_error("invalid material alpha mode %d", (int)desc->alpha_mode);
        return false;
    }
    if (!isfinite(desc->opacity) || desc->opacity < 0.0f || desc->opacity > 1.0f)
    {
        log_error("material opacity must be finite and in [0, 1]");
        return false;
    }
    for (uint32_t i = 0; i < 4; i++)
    {
        if (!isfinite(desc->base_color_factor[i]) || desc->base_color_factor[i] < 0.0f)
        {
            log_error("material base color factor values must be finite and nonnegative");
            return false;
        }
    }
    for (uint32_t i = 0; i < 3; i++)
    {
        if (!isfinite(desc->light_direction[i]))
        {
            log_error("material light direction values must be finite");
            return false;
        }
        if (!isfinite(desc->standard.emissive[i]) || desc->standard.emissive[i] < 0.0f)
        {
            log_error("standard material emissive values must be finite and nonnegative");
            return false;
        }
    }
    if (!isfinite(desc->phong.ambient) || !isfinite(desc->phong.diffuse) ||
        !isfinite(desc->phong.specular))
    {
        log_error("Phong material ADS values must be finite");
        return false;
    }
    if (!isfinite(desc->phong.shininess) || desc->phong.shininess < 0.0f)
    {
        log_error("Phong material shininess must be finite and nonnegative");
        return false;
    }
    if (!isfinite(desc->standard.roughness) || desc->standard.roughness < 0.0f ||
        desc->standard.roughness > 1.0f)
    {
        log_error("standard material roughness must be finite and in [0, 1]");
        return false;
    }
    if (!isfinite(desc->standard.specular) || desc->standard.specular < 0.0f)
    {
        log_error("standard material specular strength must be finite and nonnegative");
        return false;
    }
    if (!isfinite(desc->standard.metallic) || desc->standard.metallic < 0.0f ||
        desc->standard.metallic > 1.0f)
    {
        log_error("standard material metallic must be finite and in [0, 1]");
        return false;
    }
    if (!isfinite(desc->standard.rim_strength) || desc->standard.rim_strength < 0.0f)
    {
        log_error("standard material rim strength must be finite and nonnegative");
        return false;
    }
    return true;
}



/**
 * Apply a public material descriptor to retained material state.
 *
 * @param material the retained material state
 * @param desc the material descriptor
 */
void _material_state_apply_desc(DvzSceneMaterialState* material, const DvzMaterialDesc* desc)
{
    ANN(material);
    ANN(desc);

    material->model = desc->model;
    material->alpha_mode = desc->alpha_mode;
    material->opacity = desc->opacity;
    for (uint32_t i = 0; i < 4; i++)
        material->base_color_factor[i] = desc->base_color_factor[i];
    material->light_direction[0] = desc->light_direction[0];
    material->light_direction[1] = desc->light_direction[1];
    material->light_direction[2] = desc->light_direction[2];
    material->light_direction[3] = 0.0f;
    material->ambient = desc->phong.ambient;
    material->diffuse = desc->phong.diffuse;
    material->specular = desc->phong.specular;
    material->shininess = desc->phong.shininess;
    material->roughness = desc->standard.roughness;
    material->standard_specular = desc->standard.specular;
    material->metallic = desc->standard.metallic;
    material->emissive[0] = desc->standard.emissive[0];
    material->emissive[1] = desc->standard.emissive[1];
    material->emissive[2] = desc->standard.emissive[2];
    material->rim_strength = desc->standard.rim_strength;
    material->depth_cue_far = material->depth_cue_far == 0.0f ? 1.0f : material->depth_cue_far;
    material->depth_cue_strength =
        material->depth_cue_strength == 0.0f ? 1.0f : material->depth_cue_strength;
    material->depth_cue_density =
        material->depth_cue_density == 0.0f ? 3.0f : material->depth_cue_density;
    material->depth_cue_background[3] =
        material->depth_cue_background[3] == 0.0f ? 1.0f : material->depth_cue_background[3];
}



/**
 * Mirror retained material state into the GPU material parameter payload.
 *
 * @param params the material parameter payload
 * @param material the material state
 */
void _material_params_sync_state(
    DvzSceneMaterialParams* params, const DvzSceneMaterialState* material)
{
    ANN(params);
    ANN(material);
    params->light_direction[0] = material->light_direction[0];
    params->light_direction[1] = material->light_direction[1];
    params->light_direction[2] = material->light_direction[2];
    params->light_direction[3] = material->light_direction[3];
    params->model[0] = (float)material->model;
    params->model[1] = material->opacity;
    params->base_color_factor[0] = material->base_color_factor[0];
    params->base_color_factor[1] = material->base_color_factor[1];
    params->base_color_factor[2] = material->base_color_factor[2];
    params->base_color_factor[3] = material->base_color_factor[3];
    params->standard_params[0] = material->roughness;
    params->standard_params[1] = material->standard_specular;
    params->standard_params[2] = material->metallic;
    params->standard_params[3] = material->rim_strength;
    params->emissive_rim[0] = material->emissive[0];
    params->emissive_rim[1] = material->emissive[1];
    params->emissive_rim[2] = material->emissive[2];
    params->emissive_rim[3] = material->rim_strength;
    if (material->model == DVZ_MATERIAL_MODEL_STANDARD)
    {
        float roughness = fminf(fmaxf(material->roughness, 0.0f), 1.0f);
        params->params[0] = fmaxf(0.04f, 0.2f * (1.0f - material->metallic));
        params->params[1] = fmaxf(0.0f, 1.0f - 0.25f * roughness);
        params->params[2] = fmaxf(0.0f, material->standard_specular);
        params->params[3] = fmaxf(1.0f, 128.0f * (1.0f - roughness) + 1.0f);
    }
    else if (material->model == DVZ_MATERIAL_MODEL_UNLIT)
    {
        params->params[0] = 1.0f;
        params->params[1] = 0.0f;
        params->params[2] = 0.0f;
        params->params[3] = 1.0f;
    }
    else
    {
        params->params[0] = material->ambient;
        params->params[1] = material->diffuse;
        params->params[2] = material->specular;
        params->params[3] = material->shininess;
    }
    params->depth_cue[0] = material->depth_cue_near;
    params->depth_cue[1] = material->depth_cue_far;
    params->depth_cue[2] = material->depth_cue_enabled ? material->depth_cue_strength : 0.0f;
    params->depth_cue[3] = (float)material->depth_cue_mode;
    params->depth_cue_color[0] = material->depth_cue_background[0];
    params->depth_cue_color[1] = material->depth_cue_background[1];
    params->depth_cue_color[2] = material->depth_cue_background[2];
    params->depth_cue_color[3] = material->depth_cue_background[3];
    params->depth_cue_extra[0] = (float)material->depth_cue_metric;
    params->depth_cue_extra[1] = (float)material->depth_cue_falloff;
    params->depth_cue_extra[2] = material->depth_cue_density;
    params->depth_cue_extra[3] = 0.0f;
}



/**
 * Return whether one visual family can consume shared depth-cue material parameters.
 *
 * @param visual_type the retained visual type
 * @return whether depth cueing is supported
 */
bool _material_depth_cue_supported(DvzVisualType visual_type)
{
    const DvzVisualFamilyOps* ops = _scene_visual_family_ops(visual_type);
    return ops != NULL && ops->supports_depth_cue;
}



/**
 * Apply or clear a depth-cue descriptor on retained material state.
 *
 * @param material the retained material state
 * @param desc the depth-cue descriptor, or NULL to disable depth cueing
 * @return 0 on success, -1 on validation error
 */
int _material_apply_depth_cue(DvzSceneMaterialState* material, const DvzDepthCueDesc* desc)
{
    ANN(material);

    if (desc == NULL)
    {
        material->depth_cue_enabled = false;
        material->depth_cue_mode = DVZ_DEPTH_CUE_NONE;
        material->depth_cue_metric = DVZ_DEPTH_CUE_METRIC_CLIP_DEPTH;
        material->depth_cue_falloff = DVZ_DEPTH_CUE_FALLOFF_LINEAR;
        material->depth_cue_near = 0.0f;
        material->depth_cue_far = 1.0f;
        material->depth_cue_strength = 1.0f;
        material->depth_cue_density = 3.0f;
        material->depth_cue_background[0] = 0.0f;
        material->depth_cue_background[1] = 0.0f;
        material->depth_cue_background[2] = 0.0f;
        material->depth_cue_background[3] = 1.0f;
        return 0;
    }
    if (!DVZ_STRUCT_VALID(desc, DvzDepthCueDesc, DVZ_DEPTH_CUE_DESC_KNOWN_FLAGS))
    {
        log_error("invalid DvzDepthCueDesc ABI prologue");
        return -1;
    }

    if (desc->mode <= DVZ_DEPTH_CUE_NONE || desc->mode > DVZ_DEPTH_CUE_DARKEN)
    {
        log_error("invalid depth cue mode %d", (int)desc->mode);
        return -1;
    }
    if (desc->metric < DVZ_DEPTH_CUE_METRIC_CLIP_DEPTH ||
        desc->metric > DVZ_DEPTH_CUE_METRIC_WORLD_DISTANCE)
    {
        log_error("invalid depth cue metric %d", (int)desc->metric);
        return -1;
    }
    if (desc->falloff < DVZ_DEPTH_CUE_FALLOFF_LINEAR ||
        desc->falloff > DVZ_DEPTH_CUE_FALLOFF_EXPONENTIAL)
    {
        log_error("invalid depth cue falloff %d", (int)desc->falloff);
        return -1;
    }
    if (!isfinite(desc->near_depth) || !isfinite(desc->far_depth) ||
        desc->far_depth <= desc->near_depth)
    {
        log_error("depth cue near/far values must be finite and strictly increasing");
        return -1;
    }
    if (!isfinite(desc->strength) || desc->strength < 0.0f || desc->strength > 1.0f)
    {
        log_error("depth cue strength must be finite and in [0, 1]");
        return -1;
    }
    if (desc->density < 0.0f || !isfinite(desc->density))
    {
        log_error("depth cue density must be finite and non-negative");
        return -1;
    }
    for (uint32_t i = 0; i < 4; i++)
    {
        if (!isfinite(desc->background_color[i]) || desc->background_color[i] < 0.0f ||
            desc->background_color[i] > 1.0f)
        {
            log_error("depth cue background color values must be finite and in [0, 1]");
            return -1;
        }
    }

    material->depth_cue_enabled = true;
    material->depth_cue_mode = desc->mode;
    material->depth_cue_metric = desc->metric;
    material->depth_cue_falloff = desc->falloff;
    material->depth_cue_near = desc->near_depth;
    material->depth_cue_far = desc->far_depth;
    material->depth_cue_strength = desc->strength;
    material->depth_cue_density = desc->density > 0.0f ? desc->density : 3.0f;
    material->depth_cue_background[0] = desc->background_color[0];
    material->depth_cue_background[1] = desc->background_color[1];
    material->depth_cue_background[2] = desc->background_color[2];
    material->depth_cue_background[3] = desc->background_color[3];
    return 0;
}



/**
 * Synchronize retained material state into the GPU parameter payload and mark it dirty.
 *
 * @param visual the visual owning the material state
 */
void _visual_material_mark_dirty(DvzVisual* visual)
{
    ANN(visual);
    _material_params_sync_state(&_visual_family_state(visual)->material_params, &visual->material);
    const DvzVisualFamilyOps* ops = visual->ops;
    if (ops != NULL && ops->sync_point_style_material)
        _point_style_sync_params(&_visual_family_state(visual)->material_params, &visual->material.point_style);
    _sphere_params_sync_mode(visual);
    _visual_bump_version(&visual->material.version);
    _visual_family_state(visual)->material_params_dirty = true;
    _scene_notify_visual_changed(visual);
}



/**
 * Fill the material parameter upload payload for one visual.
 *
 * @param visual the visual
 * @param point_style_scaled whether point-style size payloads use logical screen units
 * @param screen_scale logical-to-physical screen scale
 * @param out_params output material parameter payload
 */
void _material_params_upload_payload(
    const DvzVisual* visual, bool point_style_scaled, float screen_scale,
    DvzSceneMaterialParams* out_params)
{
    ANN(visual);
    ANN(out_params);
    *out_params = _visual_family_state(visual)->material_params;
    if (point_style_scaled)
        out_params->params[0] *= screen_scale;
}



/**
 * Set shared material parameters for a primitive, mesh, or sphere visual.
 *
 * @param visual the visual
 * @param desc the material descriptor, or NULL to restore defaults
 * @return 0 on success, -1 on error
 */
int dvz_visual_set_material(DvzVisual* visual, const DvzMaterialDesc* desc)
{
    ANN(visual);
    if (!_material_visual_supported(visual->type))
    {
        log_error("materials are only supported for primitive, mesh, and sphere visuals");
        return -1;
    }
    if (!_scene_visual_mutation_allowed(visual->scene, "update visual material"))
        return -1;

    if (desc != NULL && !_material_desc_valid(desc))
        return -1;
    DvzMaterialDesc material_desc = desc != NULL ? *desc : dvz_material_desc();

    _material_state_apply_desc(&visual->material, &material_desc);
    visual->alpha_mode = material_desc.alpha_mode;
    _visual_material_mark_dirty(visual);
    return 0;
}



/**
 * Configure depth cueing for a point, pixel, primitive, or mesh visual.
 *
 * @param visual the visual
 * @param desc the depth-cue descriptor, or NULL to disable depth cueing
 * @return 0 on success, -1 on error
 */
int dvz_visual_set_depth_cue(DvzVisual* visual, const DvzDepthCueDesc* desc)
{
    ANN(visual);
    if (!_material_depth_cue_supported(visual->type))
    {
        log_error("depth cueing is only supported for point, pixel, primitive, and mesh visuals");
        return -1;
    }
    if (!_scene_visual_mutation_allowed(visual->scene, "update visual depth cue"))
        return -1;

    if (_material_apply_depth_cue(&visual->material, desc) != 0)
        return -1;
    _visual_material_mark_dirty(visual);
    return 0;
}



/**
 * Enable or disable depth testing for the visual.
 *
 * @param visual the visual
 * @param enabled whether depth testing is enabled
 * @return 0 on success, -1 on error
 */
int dvz_visual_set_depth_test(DvzVisual* visual, bool enabled)
{
    ANN(visual);
    if (!_scene_visual_mutation_allowed(visual->scene, "mutate scene visual depth test"))
        return -1;
    visual->depth_test_enabled = enabled;
    _visual_bump_version(&visual->material.version);
    _scene_notify_visual_changed(visual);
    return 0;
}



/**
 * Return whether depth testing is enabled for the visual.
 *
 * @param visual the visual
 * @return whether depth testing is enabled
 */
bool dvz_visual_depth_test(const DvzVisual* visual)
{
    ANN(visual);
    return visual->depth_test_enabled;
}



/**
 * Set the visual alpha handling mode.
 *
 * @param visual the visual
 * @param mode the alpha handling mode
 * @return 0 on success, -1 on error
 */
int dvz_visual_set_alpha_mode(DvzVisual* visual, DvzAlphaMode mode)
{
    ANN(visual);
    if (!_scene_visual_mutation_allowed(visual->scene, "mutate scene visual alpha mode"))
        return -1;
    if (mode < DVZ_ALPHA_OPAQUE || mode > DVZ_ALPHA_MASK)
    {
        log_error("invalid visual alpha mode %d", (int)mode);
        return -1;
    }
    visual->alpha_mode = mode;
    visual->material.alpha_mode = mode;
    _visual_bump_version(&visual->material.version);
    _scene_notify_visual_changed(visual);
    return 0;
}



/**
 * Return the visual alpha handling mode.
 *
 * @param visual the visual
 * @return the alpha handling mode
 */
DvzAlphaMode dvz_visual_alpha_mode(const DvzVisual* visual)
{
    ANN(visual);
    return visual->alpha_mode;
}


/**
 * Mark a visual as embedded in the panel volume occluder.
 *
 * @param visual the visual
 * @param enabled whether the visual should sample panel volume occlusion
 * @return 0 on success, -1 on validation error
 */
int dvz_visual_set_volume_occluded(DvzVisual* visual, bool enabled)
{
    ANN(visual);
    if (!_scene_visual_mutation_allowed(visual->scene, "set volume occlusion"))
        return -1;
    _visual_family_state(visual)->volume_occluded = enabled;
    _scene_notify_visual_changed(visual);
    return 0;
}


/**
 * Mark a visual as contributing front depth to panel scene occlusion.
 *
 * @param visual the visual
 * @param enabled whether the visual should act as a scene occluder
 * @return 0 on success, -1 on validation error
 */
int dvz_visual_set_scene_occluder(DvzVisual* visual, bool enabled)
{
    ANN(visual);
    if (!_scene_visual_mutation_allowed(visual->scene, "set scene occluder"))
        return -1;
    visual->scene_occluder = enabled;
    _scene_notify_visual_changed(visual);
    return 0;
}


/**
 * Mark a visual as sampling panel scene occlusion.
 *
 * @param visual the visual
 * @param enabled whether the visual should be attenuated by scene occlusion
 * @return 0 on success, -1 on validation error
 */
int dvz_visual_set_scene_occluded(DvzVisual* visual, bool enabled)
{
    ANN(visual);
    if (!_scene_visual_mutation_allowed(visual->scene, "set scene occluded"))
        return -1;
    visual->scene_occluded = enabled;
    _scene_notify_visual_changed(visual);
    return 0;
}
