/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene-owned lights                                                                           */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_log.h"
#include "_scene.h"
#include "core/scene_notify_internal.h"
#include "datoviz/scene.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_LIGHT_DESC_KNOWN_FLAGS 0u
#define DVZ_LIGHT_DIRECTION_EPS    1e-12f



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return whether a light type is active in the RC3 foundation.
 *
 * @param type the light type
 * @return whether the type is supported
 */
static bool _light_type_valid(DvzLightType type)
{
    return type == DVZ_LIGHT_AMBIENT || type == DVZ_LIGHT_DIRECTIONAL;
}



/**
 * Validate and normalize one light descriptor.
 *
 * @param desc the authored descriptor
 * @param out the normalized descriptor
 * @return whether validation succeeded
 */
static bool _light_desc_normalize(const DvzLightDesc* desc, DvzLightDesc* out)
{
    ANN(out);
    if (desc == NULL || !DVZ_STRUCT_VALID(desc, DvzLightDesc, DVZ_LIGHT_DESC_KNOWN_FLAGS))
    {
        log_error("invalid DvzLightDesc ABI prologue");
        return false;
    }
    if (!_light_type_valid(desc->type))
    {
        log_error("unsupported light type %d", (int)desc->type);
        return false;
    }
    if (!isfinite(desc->intensity) || desc->intensity < 0.0f)
    {
        log_error("light intensity must be finite and nonnegative");
        return false;
    }
    for (uint32_t i = 0; i < 3; i++)
    {
        if (!isfinite(desc->color[i]) || desc->color[i] < 0.0f)
        {
            log_error("light color values must be finite and nonnegative");
            return false;
        }
    }

    *out = *desc;
    if (desc->type == DVZ_LIGHT_AMBIENT)
    {
        out->direction[0] = 0.0f;
        out->direction[1] = 0.0f;
        out->direction[2] = 0.0f;
        return true;
    }

    float norm2 = 0.0f;
    for (uint32_t i = 0; i < 3; i++)
    {
        if (!isfinite(desc->direction[i]))
        {
            log_error("directional light direction values must be finite");
            return false;
        }
        norm2 += desc->direction[i] * desc->direction[i];
    }
    if (!isfinite(norm2) || norm2 <= DVZ_LIGHT_DIRECTION_EPS)
    {
        log_error("directional light direction must be nonzero");
        return false;
    }
    float inv_norm = 1.0f / sqrtf(norm2);
    for (uint32_t i = 0; i < 3; i++)
        out->direction[i] = desc->direction[i] * inv_norm;
    return true;
}



/**
 * Add a panel to a light's weak reverse-reference list.
 *
 * @param light the light
 * @param panel the panel
 * @return whether the reverse edge exists
 */
static bool _light_attach_panel(DvzLight* light, DvzPanel* panel)
{
    ANN(light);
    ANN(panel);
    for (uint32_t i = 0; i < light->panel_count; i++)
    {
        if (light->panels[i] == panel)
            return true;
    }
    if (light->panel_count >= light->panel_capacity)
    {
        uint32_t capacity = light->panel_capacity == 0 ? 4 : 2 * light->panel_capacity;
        DvzPanel** panels = (DvzPanel**)dvz_realloc(light->panels, capacity * sizeof(DvzPanel*));
        if (panels == NULL)
            return false;
        light->panels = panels;
        light->panel_capacity = capacity;
    }
    light->panels[light->panel_count++] = panel;
    return true;
}



/**
 * Remove a panel from a light's weak reverse-reference list.
 *
 * @param light the light
 * @param panel the panel
 */
static void _light_detach_panel(DvzLight* light, DvzPanel* panel)
{
    if (light == NULL || panel == NULL)
        return;
    for (uint32_t i = 0; i < light->panel_count; i++)
    {
        if (light->panels[i] != panel)
            continue;
        for (uint32_t j = i + 1; j < light->panel_count; j++)
            light->panels[j - 1] = light->panels[j];
        light->panel_count--;
        light->panels[light->panel_count] = NULL;
        return;
    }
}



/**
 * Mark one panel's light payload dirty and request a frame.
 *
 * @param panel the panel
 */
static void _panel_lights_invalidate(DvzPanel* panel)
{
    ANN(panel);
    panel->lights.gpu_dirty = true;
    panel->lights.revision++;
    if (panel->lights.revision == 0)
        panel->lights.revision = 1;
    if (panel->figure != NULL)
        _scene_notify_request_frame(panel->figure);
}



/**
 * Replace a validated panel light sequence and maintain reverse edges.
 *
 * @param panel the panel
 * @param lights validated ordered light handles
 * @param count number of handles
 * @param explicit_set whether the sequence is explicitly authored
 * @return whether every reverse edge was installed
 */
static bool _panel_lights_replace(
    DvzPanel* panel, DvzLight* const* lights, uint32_t count, bool explicit_set)
{
    ANN(panel);
    for (uint32_t i = 0; i < panel->lights.count; i++)
        _light_detach_panel(panel->lights.items[i], panel);

    DvzLight* installed[DVZ_SCENE_MAX_PANEL_LIGHTS] = {0};
    uint32_t installed_count = 0;
    for (uint32_t i = 0; i < count; i++)
    {
        if (!_light_attach_panel(lights[i], panel))
        {
            for (uint32_t j = 0; j < installed_count; j++)
                _light_detach_panel(installed[j], panel);
            for (uint32_t j = 0; j < panel->lights.count; j++)
                (void)_light_attach_panel(panel->lights.items[j], panel);
            return false;
        }
        installed[installed_count++] = lights[i];
    }

    for (uint32_t i = 0; i < DVZ_SCENE_MAX_PANEL_LIGHTS; i++)
        panel->lights.items[i] = i < count ? lights[i] : NULL;
    panel->lights.count = count;
    panel->lights.explicit_set = explicit_set;
    _panel_lights_invalidate(panel);
    return true;
}



/**
 * Collect the active default light sequence for one scene.
 *
 * @param scene the scene
 * @param out output handles
 * @return number of active defaults
 */
static uint32_t _scene_default_lights(DvzScene* scene, DvzLight* out[2])
{
    ANN(scene);
    ANN(out);
    uint32_t count = 0;
    if (scene->default_ambient_light != NULL && scene->default_ambient_light->scene == scene)
        out[count++] = scene->default_ambient_light;
    if (scene->default_directional_light != NULL &&
        scene->default_directional_light->scene == scene)
        out[count++] = scene->default_directional_light;
    return count;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzLightDesc dvz_light_desc(DvzLightType type)
{
    DvzLightDesc desc = {
        DVZ_STRUCT_INIT_FIELDS(DvzLightDesc),
        .type = type,
        .color = {1.0f, 1.0f, 1.0f},
        .intensity = type == DVZ_LIGHT_AMBIENT ? 0.15f : 1.0f,
        .direction = {-0.45f, +0.35f, 0.82f},
    };
    DvzLightDesc normalized = {0};
    return _light_desc_normalize(&desc, &normalized) ? normalized : desc;
}



DvzLight* dvz_light(DvzScene* scene, const DvzLightDesc* desc)
{
    if (scene == NULL)
        return NULL;
    DvzLightDesc normalized = {0};
    if (!_light_desc_normalize(desc, &normalized))
        return NULL;

    DvzLight* light = NULL;
    for (uint32_t i = 0; i < scene->light_count; i++)
    {
        if (scene->lights[i].scene == NULL)
        {
            light = &scene->lights[i];
            break;
        }
    }
    if (light == NULL)
    {
        if (scene->light_count >= DVZ_SCENE_MAX_LIGHTS)
        {
            log_error("scene light capacity reached");
            return NULL;
        }
        light = &scene->lights[scene->light_count++];
    }
    DvzPanel** panels = light->panels;
    uint32_t panel_capacity = light->panel_capacity;
    dvz_memset(light, sizeof(DvzLight), 0, sizeof(DvzLight));
    light->panels = panels;
    light->panel_capacity = panel_capacity;
    light->scene = scene;
    light->id = _scene_next_id(scene);
    light->desc = normalized;
    light->revision = 1;
    return light;
}



DvzResult dvz_light_set_desc(DvzLight* light, const DvzLightDesc* desc)
{
    if (light == NULL || light->scene == NULL)
        return DVZ_ERROR;
    DvzLightDesc normalized = {0};
    if (!_light_desc_normalize(desc, &normalized))
        return DVZ_ERROR;
    light->desc = normalized;
    light->revision++;
    if (light->revision == 0)
        light->revision = 1;
    for (uint32_t i = 0; i < light->panel_count; i++)
        _panel_lights_invalidate(light->panels[i]);
    return DVZ_OK;
}



DvzResult dvz_light_set_color(DvzLight* light, const float color[3])
{
    if (light == NULL || light->scene == NULL || color == NULL)
        return DVZ_ERROR;
    DvzLightDesc desc = light->desc;
    for (uint32_t i = 0; i < 3; i++)
        desc.color[i] = color[i];
    return dvz_light_set_desc(light, &desc);
}



DvzResult dvz_light_set_intensity(DvzLight* light, float intensity)
{
    if (light == NULL || light->scene == NULL)
        return DVZ_ERROR;
    DvzLightDesc desc = light->desc;
    desc.intensity = intensity;
    return dvz_light_set_desc(light, &desc);
}



DvzResult dvz_light_set_direction(DvzLight* light, const float direction[3])
{
    if (light == NULL || light->scene == NULL || direction == NULL ||
        light->desc.type != DVZ_LIGHT_DIRECTIONAL)
        return DVZ_ERROR;
    DvzLightDesc desc = light->desc;
    for (uint32_t i = 0; i < 3; i++)
        desc.direction[i] = direction[i];
    return dvz_light_set_desc(light, &desc);
}



void dvz_light_destroy(DvzLight* light)
{
    if (light == NULL || light->scene == NULL)
        return;
    DvzScene* scene = light->scene;
    while (light->panel_count > 0)
    {
        DvzPanel* panel = light->panels[0];
        DvzLight* survivors[DVZ_SCENE_MAX_PANEL_LIGHTS] = {0};
        uint32_t count = 0;
        for (uint32_t i = 0; i < panel->lights.count; i++)
        {
            if (panel->lights.items[i] != light)
                survivors[count++] = panel->lights.items[i];
        }
        if (!_panel_lights_replace(panel, survivors, count, panel->lights.explicit_set))
            break;
    }
    if (scene->default_ambient_light == light)
        scene->default_ambient_light = NULL;
    if (scene->default_directional_light == light)
        scene->default_directional_light = NULL;
    light->scene = NULL;
    light->id = DVZ_ID_NONE;
    light->revision = 0;
    light->desc = (DvzLightDesc){0};
}



DvzLight* dvz_scene_default_ambient(DvzScene* scene)
{
    return scene != NULL ? scene->default_ambient_light : NULL;
}



DvzLight* dvz_scene_default_directional(DvzScene* scene)
{
    return scene != NULL ? scene->default_directional_light : NULL;
}



DvzResult dvz_panel_set_lights(DvzPanel* panel, DvzLight* const* lights, uint32_t count)
{
    if (panel == NULL || panel->figure == NULL || panel->figure->scene == NULL ||
        count > DVZ_SCENE_MAX_PANEL_LIGHTS || (count > 0 && lights == NULL))
        return DVZ_ERROR;
    DvzScene* scene = panel->figure->scene;
    for (uint32_t i = 0; i < count; i++)
    {
        if (lights[i] == NULL || lights[i]->scene != scene)
        {
            log_error("panel light sets require active lights from the same scene");
            return DVZ_ERROR;
        }
        for (uint32_t j = 0; j < i; j++)
        {
            if (lights[j] == lights[i])
            {
                log_error("panel light sets do not accept duplicate handles");
                return DVZ_ERROR;
            }
        }
    }
    return _panel_lights_replace(panel, lights, count, true) ? DVZ_OK : DVZ_ERROR;
}



DvzResult dvz_panel_reset_lights(DvzPanel* panel)
{
    if (panel == NULL || panel->figure == NULL || panel->figure->scene == NULL)
        return DVZ_ERROR;
    DvzLight* defaults[2] = {0};
    uint32_t count = _scene_default_lights(panel->figure->scene, defaults);
    return _panel_lights_replace(panel, defaults, count, false) ? DVZ_OK : DVZ_ERROR;
}



void _scene_lights_init(DvzScene* scene)
{
    ANN(scene);
    DvzLightDesc ambient = dvz_light_desc(DVZ_LIGHT_AMBIENT);
    scene->default_ambient_light = dvz_light(scene, &ambient);
    DvzLightDesc directional = dvz_light_desc(DVZ_LIGHT_DIRECTIONAL);
    scene->default_directional_light = dvz_light(scene, &directional);
}



void _scene_lights_destroy_all(DvzScene* scene)
{
    if (scene == NULL)
        return;
    for (uint32_t i = 0; i < scene->light_count; i++)
    {
        DvzLight* light = &scene->lights[i];
        dvz_free(light->panels);
        light->panels = NULL;
        light->panel_count = 0;
        light->panel_capacity = 0;
        light->scene = NULL;
    }
    scene->light_count = 0;
    scene->default_ambient_light = NULL;
    scene->default_directional_light = NULL;
}



void _scene_panel_lights_init(DvzPanel* panel)
{
    ANN(panel);
    panel->lights = (DvzScenePanelLights){.gpu_dirty = true, .revision = 1};
    if (panel->figure != NULL && panel->figure->scene != NULL)
    {
        DvzLight* defaults[2] = {0};
        uint32_t count = _scene_default_lights(panel->figure->scene, defaults);
        (void)_panel_lights_replace(panel, defaults, count, false);
    }
}



void _scene_panel_lights_detach(DvzPanel* panel)
{
    if (panel == NULL)
        return;
    for (uint32_t i = 0; i < panel->lights.count; i++)
        _light_detach_panel(panel->lights.items[i], panel);
    panel->lights = (DvzScenePanelLights){0};
}
