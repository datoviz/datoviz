/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* lipid_brain_atlas - section/channel lipid intensity showcase.
 *
 * Scenario: showcase_lipid_brain_atlas
 * Style: showcase, graphite_cyan, 1600x1200 capture target
 *
 * Prepared data is loaded from `.cache/datoviz/examples/lipid_brain_atlas/prepared/`.
 * Generate a compact validation bundle with:
 *
 *   python tools/data/prepare_lipid_brain_atlas.py --synthetic --force
 *
 * Build:  just example-c showcases/lipid_brain_atlas
 * Run:    ./build/examples/c/showcases/lipid_brain_atlas --live
 * Smoke:  ./build/examples/c/showcases/lipid_brain_atlas --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "datoviz/scene.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  1600u
#define HEIGHT 1200u

#define LIPID_CACHE_PATH ".cache/datoviz/examples/lipid_brain_atlas/prepared/lipid_atlas.bin"
#define LIPID_MAGIC      "DVZLBA1"
#define LIPID_VERSION    1u
#define LIPID_MAX_WIDTH  2048u
#define LIPID_MAX_HEIGHT 2048u
#define LIPID_MAX_FRAMES 128u

#define FALLBACK_WIDTH    384u
#define FALLBACK_HEIGHT   240u
#define FALLBACK_SECTIONS 5u
#define FALLBACK_CHANNELS 4u

static const float TAU = 6.28318530718f;

static const char* FALLBACK_CHANNEL_NAMES[FALLBACK_CHANNELS] = {
    "m/z 760.58", "m/z 782.57", "m/z 834.59", "m/z 885.55",
};



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct LipidAtlasHeader
{
    char magic[8];
    uint32_t version;
    uint32_t width;
    uint32_t height;
    uint32_t sections;
    uint32_t channels;
    uint32_t reserved;
    float value_min;
    float value_max;
} LipidAtlasHeader;


typedef struct LipidAtlasData
{
    uint32_t width;
    uint32_t height;
    uint32_t sections;
    uint32_t channels;
    uint32_t* section_ids;
    float* channel_mz;
    float* values;
    bool cache_loaded;
} LipidAtlasData;


typedef struct LipidAtlasState
{
    DvzPanel* panel;
    DvzSampledField* field;
    DvzColorbar* colorbar;
    DvzOverlayCard* readout;
    LipidAtlasData data;
    float* current_values;
    uint32_t current_section;
    uint32_t current_channel;
} LipidAtlasState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Clamp a float to the unit interval.
 *
 * @param value input value
 * @return clamped value
 */
static float _clamp01(float value)
{
    if (value < 0.0f)
        return 0.0f;
    if (value > 1.0f)
        return 1.0f;
    return value;
}


/**
 * Free atlas data arrays.
 *
 * @param data atlas data
 */
static void _free_data(LipidAtlasData* data)
{
    if (data == NULL)
        return;
    dvz_free(data->values);
    dvz_free(data->channel_mz);
    dvz_free(data->section_ids);
    memset(data, 0, sizeof(*data));
}


/**
 * Free scenario state.
 *
 * @param state scenario state
 */
static void _free_state(LipidAtlasState* state)
{
    if (state == NULL)
        return;
    dvz_free(state->current_values);
    _free_data(&state->data);
    dvz_free(state);
}


/**
 * Return one synthetic brain-section mask value.
 *
 * @param x normalized X in [-1, 1]
 * @param y normalized Y in [-1, 1]
 * @param section section index
 * @return mask value in [0, 1]
 */
static float _mask(float x, float y, uint32_t section)
{
    const float taper = 0.86f - 0.055f * fabsf((float)section - 2.0f);
    const float left = powf((x + 0.33f) / 0.54f, 2.0f) + powf((y + 0.02f) / taper, 2.0f);
    const float right = powf((x - 0.33f) / 0.54f, 2.0f) + powf((y + 0.02f) / taper, 2.0f);
    const float stem = powf(x / 0.28f, 2.0f) + powf((y + 0.70f) / 0.28f, 2.0f);
    const float m = 1.0f - fminf(fminf(left, right), stem);
    return powf(_clamp01(m), 0.55f);
}


/**
 * Return one synthetic lipid intensity sample.
 *
 * @param x normalized X in [-1, 1]
 * @param y normalized Y in [-1, 1]
 * @param section section index
 * @param channel channel index
 * @return normalized intensity
 */
static float _synthetic_value(float x, float y, uint32_t section, uint32_t channel)
{
    const float ap = ((float)section - 0.5f * (float)(FALLBACK_SECTIONS - 1u)) /
                     (float)(FALLBACK_SECTIONS - 1u);
    const float ridge_center = 0.18f * sinf(1.7f * x + (float)channel);
    const float ridge = expf(-powf(y - ridge_center, 2.0f) / (0.035f + 0.01f * channel));
    const float ventricle = expf(-powf(x / 0.20f, 2.0f) - powf((y + 0.02f) / 0.34f, 2.0f));
    const float lateral =
        expf(-powf((fabsf(x) - 0.42f) / 0.17f, 2.0f) - powf((y + 0.02f) / 0.42f, 2.0f));
    const float gradient = 0.5f + 0.5f * sinf((2.0f + 0.2f * channel) * x - 1.4f * y + 0.8f * ap);
    float value = 0.24f * gradient + (0.36f + 0.08f * channel) * ridge +
                  (0.18f + 0.04f * section) * lateral - (0.30f - 0.03f * channel) * ventricle;
    value *= _mask(x, y, section);
    return _clamp01(value);
}


/**
 * Fill an in-memory deterministic fallback atlas.
 *
 * @param data output atlas data
 * @return true on success
 */
static bool _fill_fallback(LipidAtlasData* data)
{
    ANN(data);

    data->width = FALLBACK_WIDTH;
    data->height = FALLBACK_HEIGHT;
    data->sections = FALLBACK_SECTIONS;
    data->channels = FALLBACK_CHANNELS;

    const uint64_t pixels = (uint64_t)data->width * data->height;
    const uint64_t total = pixels * data->sections * data->channels;
    data->section_ids = (uint32_t*)dvz_calloc(data->sections, sizeof(*data->section_ids));
    data->channel_mz = (float*)dvz_calloc(data->channels, sizeof(*data->channel_mz));
    data->values = (float*)dvz_calloc(total, sizeof(*data->values));
    if (data->section_ids == NULL || data->channel_mz == NULL || data->values == NULL)
    {
        _free_data(data);
        return false;
    }

    for (uint32_t s = 0; s < data->sections; s++)
        data->section_ids[s] = s;
    const float mz[FALLBACK_CHANNELS] = {760.58f, 782.57f, 834.59f, 885.55f};
    memcpy(data->channel_mz, mz, sizeof(mz));

    for (uint32_t s = 0; s < data->sections; s++)
    {
        for (uint32_t c = 0; c < data->channels; c++)
        {
            float max_value = 1e-6f;
            float* dst = data->values + ((uint64_t)s * data->channels + c) * pixels;
            for (uint32_t y = 0; y < data->height; y++)
            {
                const float yy = -1.0f + 2.0f * (float)y / (float)(data->height - 1u);
                for (uint32_t x = 0; x < data->width; x++)
                {
                    const float xx = -1.0f + 2.0f * (float)x / (float)(data->width - 1u);
                    const float value = _synthetic_value(xx, yy, s, c);
                    dst[(uint64_t)y * data->width + x] = value;
                    if (value > max_value)
                        max_value = value;
                }
            }
            for (uint64_t i = 0; i < pixels; i++)
                dst[i] = _clamp01(dst[i] / max_value);
        }
    }

    data->cache_loaded = false;
    return true;
}


/**
 * Load a compact atlas binary from cache.
 *
 * @param data output atlas data
 * @return true on success
 */
static bool _load_cache(LipidAtlasData* data)
{
    ANN(data);

    FILE* fp = fopen(LIPID_CACHE_PATH, "rb");
    if (fp == NULL)
        return false;

    bool ok = false;
    LipidAtlasHeader header = {0};
    if (fread(&header, sizeof(header), 1, fp) != 1)
        goto cleanup;
    if (
        memcmp(header.magic, LIPID_MAGIC, strlen(LIPID_MAGIC)) != 0 ||
        header.version != LIPID_VERSION || header.width == 0 || header.height == 0 ||
        header.sections == 0 || header.channels == 0 || header.width > LIPID_MAX_WIDTH ||
        header.height > LIPID_MAX_HEIGHT ||
        (uint64_t)header.sections * header.channels > LIPID_MAX_FRAMES)
    {
        goto cleanup;
    }

    data->width = header.width;
    data->height = header.height;
    data->sections = header.sections;
    data->channels = header.channels;
    const uint64_t pixels = (uint64_t)data->width * data->height;
    const uint64_t frames = (uint64_t)data->sections * data->channels;
    data->section_ids = (uint32_t*)dvz_calloc(data->sections, sizeof(*data->section_ids));
    data->channel_mz = (float*)dvz_calloc(data->channels, sizeof(*data->channel_mz));
    data->values = (float*)dvz_calloc(pixels * frames, sizeof(*data->values));
    if (data->section_ids == NULL || data->channel_mz == NULL || data->values == NULL)
        goto cleanup;

    if (fread(data->section_ids, sizeof(*data->section_ids), data->sections, fp) != data->sections)
        goto cleanup;
    if (fread(data->channel_mz, sizeof(*data->channel_mz), data->channels, fp) != data->channels)
        goto cleanup;
    if (fread(data->values, sizeof(*data->values), pixels * frames, fp) != pixels * frames)
        goto cleanup;

    data->cache_loaded = true;
    ok = true;

cleanup:
    fclose(fp);
    if (!ok)
        _free_data(data);
    return ok;
}


/**
 * Load prepared cache data, or fall back to deterministic synthetic data.
 *
 * @param data output atlas data
 * @return true on success
 */
static bool _load_data(LipidAtlasData* data)
{
    ANN(data);
    memset(data, 0, sizeof(*data));

    if (_load_cache(data))
        return true;

    dvz_fprintf(
        stderr, "lipid_brain_atlas: missing prepared cache. Run "
                "`python tools/data/prepare_lipid_brain_atlas.py --synthetic --force`; "
                "using fallback.\n");
    return _fill_fallback(data);
}


/**
 * Return a pointer to one section/channel image.
 *
 * @param data atlas data
 * @param section section index
 * @param channel channel index
 * @return frame pointer
 */
static const float* _frame_values(const LipidAtlasData* data, uint32_t section, uint32_t channel)
{
    ANN(data);
    const uint64_t pixels = (uint64_t)data->width * data->height;
    return data->values + ((uint64_t)section * data->channels + channel) * pixels;
}


/**
 * Return the display channel name.
 *
 * @param data atlas data
 * @param channel channel index
 * @param out output text buffer
 * @param out_size output buffer size
 */
static void _channel_name(const LipidAtlasData* data, uint32_t channel, char* out, size_t out_size)
{
    ANN(data);
    ANN(out);
    if (channel < FALLBACK_CHANNELS)
    {
        snprintf(out, out_size, "%s", FALLBACK_CHANNEL_NAMES[channel]);
        return;
    }
    snprintf(out, out_size, "m/z %.2f", data->channel_mz[channel]);
}


/**
 * Update image payload and readout for the active section/channel.
 *
 * @param state showcase state
 * @param section section index
 * @param channel channel index
 * @return true on success
 */
static bool _set_frame(LipidAtlasState* state, uint32_t section, uint32_t channel)
{
    if (state == NULL || state->field == NULL || state->current_values == NULL)
        return false;
    if (section >= state->data.sections || channel >= state->data.channels)
        return false;

    const uint64_t pixels = (uint64_t)state->data.width * state->data.height;
    memcpy(state->current_values, _frame_values(&state->data, section, channel), pixels * sizeof(float));
    if (!dvz_sampled_field_update_region(
            state->field,
            (DvzFieldRegion){
                .x = 0,
                .y = 0,
                .z = 0,
                .width = state->data.width,
                .height = state->data.height,
                .depth = 1,
            },
            &(DvzFieldDataView){DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView),
                .data = state->current_values,
                .bytes_per_row = state->data.width * sizeof(float),
                .rows_per_image = state->data.height,
            }))
    {
        return false;
    }

    state->current_section = section;
    state->current_channel = channel;

    char channel_text[32] = {0};
    _channel_name(&state->data, channel, channel_text, sizeof(channel_text));
    if (state->colorbar != NULL)
        dvz_colorbar_set_title(state->colorbar, channel_text);
    if (state->readout != NULL)
    {
        char text[160] = {0};
        snprintf(
            text, sizeof(text), "Lipid Brain Atlas  section %u/%u  %s  source %s",
            section + 1u, state->data.sections, channel_text,
            state->data.cache_loaded ? "cache" : "fallback");
        dvz_overlay_card_set_text(state->readout, text);
    }
    return true;
}


/**
 * Create the continuous scale shared by the image and colorbar.
 *
 * @param scene scene owning scale resources
 * @return created scale, or NULL
 */
static DvzScale* _add_scale(DvzScene* scene)
{
    ANN(scene);

    DvzScale* scale = dvz_scale(
        scene, &(DvzScaleDesc){DVZ_STRUCT_INIT_FIELDS(DvzScaleDesc),
                   .kind = DVZ_SCALE_CONTINUOUS,
                   .label = "intensity",
                   .format = {DVZ_STRUCT_INIT_FIELDS(DvzFormatDesc),
                              .precision = 2,
                              .trim_trailing_zeros = true},
               });
    if (scale == NULL)
        return NULL;

    dvz_scale_set_domain(scale, 0.0, 1.0);
    dvz_scale_set_view_range(scale, 0.0, 1.0);

    DvzColormap* colormap = example_graphite_cyan_colormap(scene);
    if (colormap == NULL)
        return NULL;
    dvz_scale_set_colormap(scale, colormap);
    return scale;
}


/**
 * Add the sampled-field image visual.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @param scale color scale
 * @param state showcase state
 * @return true on success
 */
static bool _add_image(DvzScene* scene, DvzPanel* panel, DvzScale* scale, LipidAtlasState* state)
{
    ANN(scene);
    ANN(panel);
    ANN(scale);
    ANN(state);

    vec3 data_positions[4] = {
        {0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        {1.0f, 1.0f, 0.0f},
    };
    vec3 visual_positions[4] = {{0}};
    vec2 texcoords[4] = {
        {0.0f, 0.0f},
        {0.0f, 1.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
    };
    if (dvz_panel_data_to_visual_positions(
            panel, (const float*)data_positions, (float*)visual_positions, 4) != 0)
        return false;

    DvzVisual* image = dvz_image(scene, 0);
    if (image == NULL)
        return false;
    if (dvz_visual_set_data(image, "position", visual_positions, 4) != 0)
        return false;
    if (dvz_visual_set_data(image, "texcoords", texcoords, 4) != 0)
        return false;
    if (dvz_visual_set_scale(image, "color", scale) != 0)
        return false;

    const uint64_t pixels = (uint64_t)state->data.width * state->data.height;
    state->current_values = (float*)dvz_calloc(pixels, sizeof(*state->current_values));
    if (state->current_values == NULL)
        return false;
    memcpy(state->current_values, _frame_values(&state->data, 0, 0), pixels * sizeof(float));

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){DVZ_STRUCT_INIT_FIELDS(DvzSampledFieldDesc),
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_R32_FLOAT,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = state->data.width,
                   .height = state->data.height,
                   .depth = 1,
               });
    if (field == NULL)
        return false;
    if (!dvz_sampled_field_set_data(
            field, &(DvzFieldDataView){DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView),
                       .data = state->current_values,
                       .bytes_per_row = state->data.width * sizeof(float),
                       .rows_per_image = state->data.height,
                   }))
    {
        return false;
    }
    if (!dvz_visual_set_field(image, "field", field))
        return false;
    if (dvz_visual_set_depth_test(image, false) != 0)
        return false;
    if (dvz_panel_add_visual(panel, image, NULL) != 0)
        return false;

    state->field = field;
    return true;
}


/**
 * Add a colorbar for the active lipid channel.
 *
 * @param panel panel receiving the colorbar
 * @param scale color scale
 * @return created colorbar, or NULL
 */
static DvzColorbar* _add_colorbar(DvzPanel* panel, DvzScale* scale)
{
    ANN(panel);
    ANN(scale);

    DvzColorbarDesc desc = dvz_colorbar_desc();
    desc.title = "m/z 760.58";
    desc.reserve_px = 120.0f;
    desc.ramp_width_px = 18.0f;
    desc.edge_offset_px = 22.0f;
    desc.plot_gap_px = 14.0f;
    DvzColorbar* colorbar = dvz_colorbar(panel, scale, &desc);
    if (colorbar != NULL)
        dvz_colorbar_set_format(
            colorbar, &(DvzFormatDesc){DVZ_STRUCT_INIT_FIELDS(DvzFormatDesc),
                          .precision = 2,
                          .trim_trailing_zeros = true});
    return colorbar;
}


/**
 * Add a screen-space section/channel readout card.
 *
 * @param panel panel owning the overlay
 * @return created card, or NULL
 */
static DvzOverlayCard* _add_readout(DvzPanel* panel)
{
    ANN(panel);

    DvzOverlay* overlay = dvz_overlay(panel, 0);
    if (overlay == NULL)
        return NULL;

    DvzOverlayCardStyle style = dvz_overlay_card_style();
    DvzColor panel_bg = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_PANEL_BG);
    DvzColor text = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_TEXT);
    style.background_color = dvz_color_rgba(panel_bg.r, panel_bg.g, panel_bg.b, 226u);
    style.text_color = text;
    style.padding_px[0] = 12.0f;
    style.padding_px[1] = 7.0f;
    style.min_width_px = 500.0f;
    style.height_px = 34.0f;
    style.glyph_advance_px = 7.5f;
    style.text_size_px = 14.0f;
    style.text_renderer = DVZ_TEXT_RENDERER_MSDF_ATLAS;
    style.max_text_chars = 160u;

    return dvz_overlay_card(
        overlay,
        &(DvzOverlayCardDesc){DVZ_STRUCT_INIT_FIELDS(DvzOverlayCardDesc),
            .text = "Lipid Brain Atlas",
            .placement = DVZ_OVERLAY_CARD_PLACEMENT_BOTTOM_LEFT,
            .offset_px = {30.0f, -46.0f},
            .style = &style,
        });
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Advance the deterministic section/channel sweep.
 *
 * @param ctx scenario context
 * @param user scenario state
 */
static void _scenario_frame(DvzScenarioContext* ctx, void* user)
{
    LipidAtlasState* state = (LipidAtlasState*)user;
    if (ctx == NULL || state == NULL || state->data.sections == 0 || state->data.channels == 0)
        return;

    const uint32_t frame_span = 45u;
    const uint32_t frame = (uint32_t)(ctx->frame_index / frame_span);
    const uint32_t total = state->data.sections * state->data.channels;
    const uint32_t slot = total > 0 ? frame % total : 0;
    const uint32_t section = slot / state->data.channels;
    const uint32_t channel = slot % state->data.channels;
    if (section != state->current_section || channel != state->current_channel)
    {
        if (!_set_frame(state, section, channel))
            fprintf(stderr, "lipid_brain_atlas: failed to update sampled field\n");
    }
}


/**
 * Initialize the lipid brain atlas showcase.
 *
 * @param ctx scenario context
 * @param out_user scenario state output
 * @return true on success
 */
static bool _scenario_init(DvzScenarioContext* ctx, void** out_user)
{
    if (ctx == NULL)
        return false;
    if (out_user != NULL)
        *out_user = NULL;

    LipidAtlasState* state = (LipidAtlasState*)dvz_calloc(1, sizeof(*state));
    if (state == NULL)
        return false;
    if (!_load_data(&state->data))
        goto error;

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    if (ctx->figure == NULL)
        goto error;

    DvzPanel* panel = dvz_panel_full(ctx->figure);
    if (panel == NULL)
        goto error;
    example_graphite_cyan_set_panel_background(panel);

    if (!dvz_panel_set_layout_reserve(
            panel, &(DvzPanelLayoutReserve){.left = 0.05f, .right = 0.14f, .bottom = 0.07f,
                                            .top = 0.04f}))
        goto error;
    if (dvz_panel_set_domain(panel, DVZ_DIM_X, 0.0, 1.0) != 0)
        goto error;
    if (dvz_panel_set_domain(panel, DVZ_DIM_Y, 0.0, 1.0) != 0)
        goto error;

    DvzScale* scale = _add_scale(ctx->scene);
    if (scale == NULL)
        goto error;
    if (!_add_image(ctx->scene, panel, scale, state))
        goto error;

    DvzColorbar* colorbar = _add_colorbar(panel, scale);
    if (colorbar == NULL)
        goto error;
    DvzOverlayCard* readout = _add_readout(panel);
    if (readout == NULL)
        goto error;

    if (dvz_scenario_panzoom(ctx, panel, NULL, DVZ_DIM_MASK_XY) == NULL)
        goto error;

    state->panel = panel;
    state->colorbar = colorbar;
    state->readout = readout;
    if (!_set_frame(state, 0, 0))
        goto error;

    if (out_user != NULL)
        *out_user = state;
    return true;

error:
    _free_state(state);
    return false;
}


/**
 * Destroy the lipid brain atlas showcase state.
 *
 * @param ctx scenario context
 * @param user scenario state
 */
static void _scenario_destroy(DvzScenarioContext* ctx, void* user)
{
    (void)ctx;
    _free_state((LipidAtlasState*)user);
}


/**
 * Return the lipid brain atlas scenario specification.
 *
 * @return scenario specification
 */
static DvzScenarioSpec _lipid_brain_atlas_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "showcase_lipid_brain_atlas",
        .title = "lipid_brain_atlas",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .requirements = DVZ_SCENARIO_REQ_CONTROLLER | DVZ_SCENARIO_REQ_PANZOOM |
                        DVZ_SCENARIO_REQ_FRAME_CALLBACKS,
        .init = _scenario_init,
        .frame = _scenario_frame,
        .destroy = _scenario_destroy,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the lipid brain atlas showcase through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = _lipid_brain_atlas_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
