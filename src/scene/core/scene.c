/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene graph — DvzScene / DvzFigure / DvzPanel / DvzVisual                                  */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_controllers.h"
#include "frame_plan/frame_plan.h"
#include "_log.h"
#include "_overflow.h"
#include "scene_emit/scene_emit.h"
#include "render_contract/render_contract.h"
#include "datoviz/drp2/runtime.h"
#include "datoviz/math/_cglm.h"
#include "../../drp2/_stream.h"
#include "_scene.h"
#include "core/scene_notify_internal.h"
#include "_technique.h"
#include "_visual_internal.h"
#include "core/frame_trace_internal.h"
#include "domain/buffer_internal.h"
#include "domain/compute_internal.h"
#include "domain/field_internal.h"
#include "domain/polygon_internal.h"
#include "domain/graph_internal.h"
#include "query/internal.h"
#include "text/text_internal.h"
#include "datoviz/scene.h"


/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_VOLUME_OCCLUSION_DESC_KNOWN_FLAGS 0u
#define DVZ_SCENE_OCCLUSION_DESC_KNOWN_FLAGS 0u
#define DVZ_FONT_DESC_KNOWN_FLAGS             0u
#define DVZ_FONT_DEFAULTS_KNOWN_FLAGS         0u



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static bool _volume_occlusion_desc_validate(const DvzVolumeOcclusionDesc* desc)
{
    if (desc == NULL)
        return true;
    if (!DVZ_STRUCT_VALID(desc, DvzVolumeOcclusionDesc, DVZ_VOLUME_OCCLUSION_DESC_KNOWN_FLAGS))
    {
        log_error("invalid DvzVolumeOcclusionDesc ABI prologue");
        return false;
    }
    return true;
}


static bool _font_desc_validate(const DvzFontDesc* desc)
{
    if (desc == NULL)
        return false;
    if (!DVZ_STRUCT_VALID(desc, DvzFontDesc, DVZ_FONT_DESC_KNOWN_FLAGS))
    {
        log_error("invalid font descriptor ABI");
        return false;
    }
    return true;
}


static bool _font_defaults_validate(const DvzFontDefaults* defaults)
{
    if (defaults == NULL)
        return true;
    if (!DVZ_STRUCT_VALID(defaults, DvzFontDefaults, DVZ_FONT_DEFAULTS_KNOWN_FLAGS))
    {
        log_error("invalid font defaults ABI");
        return false;
    }
    if (!_font_desc_validate(&defaults->sans) || !_font_desc_validate(&defaults->mono))
        return false;
    return true;
}



static bool _scene_occlusion_desc_validate(const DvzSceneOcclusionDesc* desc)
{
    if (desc == NULL)
        return true;
    if (!DVZ_STRUCT_VALID(desc, DvzSceneOcclusionDesc, DVZ_SCENE_OCCLUSION_DESC_KNOWN_FLAGS))
    {
        log_error("invalid DvzSceneOcclusionDesc ABI prologue");
        return false;
    }
    return true;
}


/**
 * Allocate the next monotonically increasing scene-local identity.
 *
 * @param scene the scene
 * @return a non-zero scene-local identity
 */
DvzId _scene_next_id(DvzScene* scene)
{
    ANN(scene);
    scene->next_id++;
    if (scene->next_id == DVZ_ID_NONE)
        scene->next_id++;
    return scene->next_id;
}



/**
 * Allocate the next monotonically increasing request freshness serial.
 *
 * Serial 0 stays reserved as the "no freshness tracking" sentinel used by legacy synthetic tests.
 *
 * @param scene the owning scene
 * @return the next non-zero request freshness serial
 */
uint64_t _scene_next_request_serial(DvzScene* scene)
{
    ANN(scene);
    scene->next_request_serial++;
    if (scene->next_request_serial == 0)
        scene->next_request_serial++;
    return scene->next_request_serial;
}



/*************************************************************************************************/
/*  Scene                                                                                        */
/*************************************************************************************************/

DvzScene* dvz_scene(void)
{
    DvzScene* scene = (DvzScene*)dvz_calloc(1, sizeof(DvzScene));
    if (scene == NULL)
        return NULL;
    scene->id = _scene_next_id(scene);
    scene->caps = dvz_capability_snapshot();
    _scene_technique_state_init(&scene->techniques);
    scene->font_defaults = dvz_font_defaults();
    scene->clock.mode = DVZ_CLOCK_REALTIME;
    scene->clock.fps = 60.0;
    _scene_request_executor_init(&scene->query_executor);
    scene->emitter = dvz_frame_plan_emitter();
    if (scene->emitter == NULL)
    {
        dvz_free(scene);
        return NULL;
    }
    return scene;
}


DvzId dvz_scene_id(const DvzScene* scene)
{
    return scene != NULL ? scene->id : DVZ_ID_NONE;
}


/**
 * Set the scene font defaults used by text objects without an explicit font.
 *
 * @param scene the scene
 * @param defaults font defaults, or NULL for built-in defaults
 */
void dvz_scene_set_font_defaults(DvzScene* scene, const DvzFontDefaults* defaults)
{
    ANN(scene);
    if (!_font_defaults_validate(defaults))
        return;
    scene->font_defaults = defaults != NULL ? *defaults : dvz_font_defaults();
}


/**
 * Return the scene font defaults.
 *
 * @param scene the scene
 * @return scene font defaults
 */
DvzFontDefaults dvz_scene_font_defaults(const DvzScene* scene)
{
    if (scene == NULL)
        return dvz_font_defaults();
    return scene->font_defaults;
}


void dvz_scene_set_capabilities(DvzScene* scene, const DvzCapabilitySnapshot* caps)
{
    ANN(scene);
    if (!dvz_capability_snapshot_valid(caps))
        return;
    dvz_capability_snapshot_copy(&scene->caps, caps);
}


void dvz_scene_destroy(DvzScene* scene)
{
    if (scene == NULL)
        return;
    if (!_scene_visual_mutation_allowed(scene, "destroy scene-owned visual data"))
        return;
    for (uint32_t i = 0; i < scene->figure_count; i++)
        _scene_figure_frame_plan_trace_reset(&scene->figures[i]);
    for (uint32_t i = 0; i < scene->composite_count; i++)
        _scene_composite_reset(&scene->composites[i]);
    for (uint32_t i = 0; i < scene->polygon_set_count; i++)
        _scene_polygon_set_reset(&scene->polygon_sets[i]);
    for (uint32_t i = 0; i < scene->polygon_count; i++)
        _scene_polygon_reset(&scene->polygons[i]);
    for (uint32_t i = 0; i < scene->graph_count; i++)
        _scene_graph_reset(&scene->graphs[i]);
    for (uint32_t i = 0; i < scene->bars_count; i++)
        _scene_bars_reset(&scene->bars[i]);
    for (uint32_t i = 0; i < scene->band_count; i++)
        _scene_band_reset(&scene->bands[i]);
    for (uint32_t i = 0; i < scene->visual_count; i++)
        _scene_visual_reset(&scene->visuals[i], true);
    for (uint32_t i = 0; i < scene->scale_count; i++)
        dvz_scale_destroy(&scene->scales[i]);
    for (uint32_t i = 0; i < scene->colormap_count; i++)
        dvz_colormap_destroy(&scene->colormaps[i]);
    for (uint32_t i = 0; i < scene->font_count; i++)
        _scene_font_release(&scene->fonts[i]);
    for (uint32_t i = 0; i < scene->symbol_set_count; i++)
    {
        for (uint32_t j = 0; j < scene->symbol_sets[i].source_count; j++)
        {
            dvz_free(scene->symbol_sets[i].sources[j].data);
            scene->symbol_sets[i].sources[j].data = NULL;
        }
        for (uint32_t j = 0; j <= DVZ_SYMBOL_SOURCE_MSDF; j++)
        {
            dvz_free(scene->symbol_sets[i].atlas_pages[j].data);
            scene->symbol_sets[i].atlas_pages[j].data = NULL;
        }
    }
    for (uint32_t i = 0; i < DVZ_SCENE_MAX_FIELDS; i++)
        _scene_field_reset(&scene->fields[i]);
    for (uint32_t i = 0; i < DVZ_SCENE_MAX_BUFFERS; i++)
        _scene_buffer_reset(&scene->buffers[i]);
    for (uint32_t i = 0; i < DVZ_SCENE_MAX_COMPUTES; i++)
        _scene_compute_reset(&scene->computes[i]);
    for (uint32_t i = 0; i < scene->selection_count; i++)
        scene->selections[i].scene = NULL;
    for (uint32_t i = 0; i < scene->hover_count; i++)
        scene->hovers[i].scene = NULL;
    for (uint32_t i = 0; i < scene->interaction_count; i++)
        scene->interactions[i].scene = NULL;
    for (uint32_t i = 0; i < scene->item_interaction_count; i++)
        scene->item_interactions[i].scene = NULL;
    for (uint32_t i = 0; i < scene->link_channel_count; i++)
        scene->link_channels[i].scene = NULL;
    for (uint32_t i = 0; i < scene->pinned_readout_count; i++)
        scene->pinned_readouts[i].scene = NULL;
    for (uint32_t i = 0; i < scene->overlay_count; i++)
        scene->overlays[i].scene = NULL;
    for (uint32_t i = 0; i < scene->overlay_card_count; i++)
        scene->overlay_cards[i].scene = NULL;
    for (uint32_t i = 0; i < scene->controller_count; i++)
        _scene_controller_destroy(&scene->controllers[i]);
    _scene_request_executor_destroy(&scene->query_executor);
    if (scene->emitter != NULL)
    {
        dvz_frame_plan_emitter_destroy(scene->emitter);
        scene->emitter = NULL;
    }
    dvz_free(scene);
}



/*************************************************************************************************/
/*  Figure                                                                                       */
/*************************************************************************************************/

DvzFigure* dvz_figure(DvzScene* scene, uint32_t width, uint32_t height, uint32_t flags)
{
    ANN(scene);
    DvzFigure* fig = NULL;
    for (uint32_t i = 0; i < scene->figure_count; i++)
    {
        if (scene->figures[i].scene == NULL)
        {
            fig = &scene->figures[i];
            break;
        }
    }
    if (fig == NULL)
    {
        if (scene->figure_count >= DVZ_SCENE_MAX_FIGURES)
            return NULL;
        fig = &scene->figures[scene->figure_count++];
    }

    dvz_memset(fig, sizeof(DvzFigure), 0, sizeof(DvzFigure));
    fig->scene  = scene;
    fig->id     = _scene_next_id(scene);
    fig->width  = width;
    fig->height = height;
    fig->flags  = flags;
    fig->device_scale_x = 1.0f;
    fig->device_scale_y = 1.0f;
    fig->render_scale = 1.0f;
    fig->user_scale = 1.0f;
    return fig;
}


DvzId dvz_figure_id(const DvzFigure* figure)
{
    return figure != NULL && figure->scene != NULL ? figure->id : DVZ_ID_NONE;
}




/**
 * Return the scene that owns a figure.
 *
 * @param figure the figure
 * @return the owning scene, or NULL
 */
DvzScene* dvz_figure_scene(DvzFigure* figure)
{
    if (figure == NULL)
        return NULL;
    return figure->scene;
}


/**
 * Reset one panel slot.
 *
 * @param panel the panel
 * @param detach_grid whether to remove the panel from its owning grid attachment list
 */
static void _scene_panel_reset(DvzPanel* panel, bool detach_grid)
{
    if (panel == NULL)
        return;
    if (detach_grid && panel->grid != NULL)
        (void)_scene_grid_detach_panel(panel->grid, panel);
    (void)dvz_panel_connect_input(panel, NULL);
    panel->panzoom = NULL;
    panel->arcball = NULL;
    panel->fly = NULL;
    for (uint32_t i = 0; i < 3; i++)
        panel->controllers[i] = NULL;
    panel->turntable = NULL;
    panel->grid = NULL;
    panel->grid_cell = (DvzGridCell){0};
    if (panel->camera != NULL)
    {
        dvz_camera_destroy(panel->camera);
        panel->camera = NULL;
    }
    panel->figure       = NULL;
    panel->visual_count = 0;
    panel->bounds_visual = NULL;
    panel->bounds_occluded_visual = NULL;
    panel->bounds_visible = false;
    panel->colorbar_count = 0;
    panel->legend_count = 0;
    panel->interaction = NULL;
    panel->item_interaction = NULL;
    panel->pinned_readout_count = 0;
    dvz_memset(&panel->hover, sizeof(DvzHoverState), 0, sizeof(DvzHoverState));
}




/**
 * Update a figure logical size.
 *
 * @param figure the figure
 * @param width width in logical pixels
 * @param height height in logical pixels
 */
void dvz_figure_resize(DvzFigure* figure, uint32_t width, uint32_t height)
{
    ANN(figure);
    if (figure->width == width && figure->height == height)
        return;
    figure->width = width;
    figure->height = height;
    (void)_scene_figure_resolve_layouts(figure);
    for (uint32_t i = 0; i < figure->panel_count; i++)
    {
        DvzPanel* panel = &figure->panels[i];
        (void)_scene_panel_refresh_border(panel);
        if (panel->camera != NULL)
        {
            float panel_width = 0.0f;
            float panel_height = 0.0f;
            _scene_panel_pixel_size(panel, &panel_width, &panel_height);
            dvz_camera_resize(panel->camera, panel_width, panel_height);
        }
    }
    _scene_notify_request_frame(figure);
}



/**
 * Return a figure logical size.
 *
 * @param figure the figure
 * @param out_width output width in logical pixels, may be NULL
 * @param out_height output height in logical pixels, may be NULL
 */
void dvz_figure_size(const DvzFigure* figure, uint32_t* out_width, uint32_t* out_height)
{
    ANN(figure);
    if (out_width != NULL)
        *out_width = figure->width;
    if (out_height != NULL)
        *out_height = figure->height;
}




void dvz_figure_destroy(DvzFigure* figure)
{
    if (figure == NULL || figure->scene == NULL)
        return;
    if (!_scene_visual_mutation_allowed(figure->scene, "destroy figure"))
        return;
    _scene_figure_frame_plan_trace_reset(figure);
    for (uint32_t i = 0; i < figure->panel_count; i++)
        _scene_panel_reset(&figure->panels[i], false);
    dvz_memset(figure->panels, sizeof(figure->panels), 0, sizeof(figure->panels));
    dvz_memset(figure->grids, sizeof(figure->grids), 0, sizeof(figure->grids));
    dvz_memset(figure->computes, sizeof(figure->computes), 0, sizeof(figure->computes));
    figure->panel_count = 0;
    figure->grid_count = 0;
    figure->compute_count = 0;
    dvz_memset(figure, sizeof(DvzFigure), 0, sizeof(DvzFigure));
}


/*************************************************************************************************/
/*  Panel                                                                                        */
/*************************************************************************************************/

/**
 * Return default panel MSAA options.
 *
 * @return MSAA descriptor with 4x samples and alpha-to-coverage enabled
 */
DvzMsaaDesc dvz_msaa_desc(void)
{
    return (DvzMsaaDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzMsaaDesc),
        .enabled = true,
        .sample_count = 4,
        .alpha_to_coverage = true,
    };
}


DvzEdlDesc dvz_edl_desc(void)
{
    return (DvzEdlDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzEdlDesc),
        .radius = 2.0f,
        .strength = 55.0f,
        .depth_scale = 1.0f,
    };
}



DvzSsaoDesc dvz_ssao_desc(void)
{
    return (DvzSsaoDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzSsaoDesc),
        .radius = 3.0f,
        .strength = 8.0f,
        .bias = 0.0f,
        .power = 1.0f,
        .min_visibility = 0.0f,
        .blur_radius = 2.0f,
        .blur_depth_sigma = 0.65f,
        .blur_normal_sigma = 0.35f,
        .sample_count = 16,
        .blur_enabled = true,
        .debug_view = false,
    };
}



DvzVolumeOcclusionDesc dvz_volume_occlusion_desc(void)
{
    return (DvzVolumeOcclusionDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzVolumeOcclusionDesc),
        .enabled = true,
        .alpha_threshold = 0.08f,
        .fade_distance = 0.08f,
        .occluded_alpha = 0.20f,
    };
}



DvzSceneOcclusionDesc dvz_scene_occlusion_desc(void)
{
    return (DvzSceneOcclusionDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzSceneOcclusionDesc),
        .enabled = true,
        .depth_bias = 0.0005f,
        .soft_edge = 0.002f,
        .hidden_alpha = 0.20f,
    };
}



DvzPanel* dvz_panel(DvzFigure* figure, DvzPanelDesc desc)
{
    ANN(figure);
    if (!_panel_desc_valid(desc))
        return NULL;
    if (figure->panel_count >= DVZ_SCENE_MAX_PANELS)
        return NULL;
    DvzPanel* panel       = &figure->panels[figure->panel_count++];
    panel->figure         = figure;
    panel->id             = _scene_next_id(figure->scene);
    panel->desc           = desc;
    panel->grid           = NULL;
    panel->grid_cell      = (DvzGridCell){0};
    panel->base_reserve   = (DvzPanelReserve){0};
    panel->axis_reserve   = (DvzPanelReserve){0};
    panel->colorbar_reserve = (DvzPanelReserve){0};
    panel->legend_reserve = (DvzPanelReserve){0};
    panel->reserve        = (DvzPanelReserve){0};
    panel->padding        = (DvzPanelReserve){0};
    panel->view2d_enabled = false;
    panel->view2d = dvz_panel_view2d();
    _scene_technique_state_init(&panel->techniques);
    panel->visual_count = 0;
    panel->bounds_visual = NULL;
    panel->bounds_occluded_visual = NULL;
    panel->bounds_visible = false;
    panel->border_visual = NULL;
    panel->border = dvz_panel_border_desc();
    panel->border.visible = false;
    return panel;
}


DvzId dvz_panel_id(const DvzPanel* panel)
{
    return panel != NULL && panel->figure != NULL ? panel->id : DVZ_ID_NONE;
}



/**
 * Update a panel rectangle in normalized figure coordinates.
 *
 * @param panel the panel
 * @param desc panel position and size in normalized [0, 1] figure coordinates
 * @return whether the descriptor was accepted
 */
bool dvz_panel_set_desc(DvzPanel* panel, DvzPanelDesc desc)
{
    if (panel == NULL)
        return false;
    if (!_panel_desc_valid(desc))
        return false;
    panel->grid = NULL;
    panel->grid_cell = (DvzGridCell){0};
    return _scene_panel_set_desc_internal(panel, desc);
}



/**
 * Create a panel that fills the whole figure.
 *
 * @param figure the figure
 * @return the panel
 */
DvzPanel* dvz_panel_full(DvzFigure* figure)
{
    return dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
}



/**
 * Configure Eye-Dome Lighting for one panel.
 *
 * @param panel the panel
 * @param desc EDL descriptor, or NULL to disable
 * @return whether the panel EDL state was updated
 */
bool dvz_panel_set_edl(DvzPanel* panel, const DvzEdlDesc* desc)
{
    ANN(panel);
    bool ok = _scene_technique_state_set_edl(&panel->techniques, desc);
    if (ok)
        _scene_notify_request_frame(panel->figure);
    return ok;
}


/**
 * Configure internal multisample antialiasing for one panel.
 *
 * @param panel the panel
 * @param desc MSAA descriptor, or NULL to disable
 * @return whether the panel MSAA state was updated
 */
bool dvz_panel_set_msaa(DvzPanel* panel, const DvzMsaaDesc* desc)
{
    ANN(panel);
    bool ok = _scene_technique_state_set_msaa(&panel->techniques, desc);
    if (ok)
        _scene_notify_request_frame(panel->figure);
    return ok;
}


/**
 * Configure screen-space ambient occlusion for one panel.
 *
 * @param panel the panel
 * @param desc SSAO descriptor, or NULL to disable
 * @return whether the panel SSAO state was updated
 */
bool dvz_panel_set_ssao(DvzPanel* panel, const DvzSsaoDesc* desc)
{
    ANN(panel);
    bool ok = _scene_technique_state_set_ssao(&panel->techniques, desc);
    if (ok)
        _scene_notify_request_frame(panel->figure);
    return ok;
}


/**
 * Configure generic screen-space scene occlusion for one panel.
 *
 * @param panel the panel
 * @param desc scene occlusion descriptor, or NULL to disable
 * @return 0 on success, -1 on validation error
 */
int dvz_panel_set_scene_occlusion(DvzPanel* panel, const DvzSceneOcclusionDesc* desc)
{
    ANN(panel);
    if (!_scene_occlusion_desc_validate(desc))
        return -1;
    if (desc == NULL || !desc->enabled)
    {
        panel->scene_occlusion_enabled = false;
        dvz_memset(
            &panel->scene_occlusion, sizeof(DvzSceneOcclusionDesc), 0,
            sizeof(DvzSceneOcclusionDesc));
        _scene_notify_request_frame(panel->figure);
        return 0;
    }

    panel->scene_occlusion = *desc;
    panel->scene_occlusion.enabled = true;
    if (panel->scene_occlusion.soft_edge <= 0.0f)
        panel->scene_occlusion.soft_edge = 0.002f;
    if (panel->scene_occlusion.hidden_alpha < 0.0f)
        panel->scene_occlusion.hidden_alpha = 0.0f;
    if (panel->scene_occlusion.hidden_alpha > 1.0f)
        panel->scene_occlusion.hidden_alpha = 1.0f;
    panel->scene_occlusion_enabled = true;
    _scene_notify_request_frame(panel->figure);
    return 0;
}


/**
 * Configure a panel volume visual as the screen-space occluder for embedded visuals.
 *
 * @param panel the panel
 * @param volume the volume visual attached to the same panel, or NULL to disable
 * @param desc volume occlusion descriptor, or NULL to disable
 * @return 0 on success, -1 on validation error
 */
int dvz_panel_set_volume_occluder(
    DvzPanel* panel, DvzVisual* volume, const DvzVolumeOcclusionDesc* desc)
{
    ANN(panel);
    if (!_volume_occlusion_desc_validate(desc))
        return -1;
    if (volume == NULL || desc == NULL || !desc->enabled)
    {
        panel->volume_occluder_visual = NULL;
        panel->volume_occlusion_enabled = false;
        dvz_memset(
            &panel->volume_occlusion, sizeof(DvzVolumeOcclusionDesc), 0,
            sizeof(DvzVolumeOcclusionDesc));
        _scene_notify_request_frame(panel->figure);
        return 0;
    }
    if (volume->type != DVZ_VISUAL_TYPE_VOLUME)
    {
        log_error("dvz_panel_set_volume_occluder requires a volume visual");
        return -1;
    }

    bool attached = false;
    for (uint32_t i = 0; i < panel->visual_count; i++)
        attached = attached || panel->visuals[i].visual == volume;
    if (!attached)
    {
        log_error("volume occluder must be attached to the panel");
        return -1;
    }

    panel->volume_occluder_visual = volume;
    panel->volume_occlusion = *desc;
    panel->volume_occlusion.enabled = true;
    if (panel->volume_occlusion.alpha_threshold <= 0.0f)
        panel->volume_occlusion.alpha_threshold = 0.08f;
    if (panel->volume_occlusion.fade_distance <= 0.0f)
        panel->volume_occlusion.fade_distance = 0.08f;
    if (panel->volume_occlusion.occluded_alpha <= 0.0f)
        panel->volume_occlusion.occluded_alpha = 0.20f;
    panel->volume_occlusion_enabled = true;
    _scene_notify_request_frame(panel->figure);
    return 0;
}



void dvz_panel_destroy(DvzPanel* panel)
{
    _scene_panel_reset(panel, true);
}
