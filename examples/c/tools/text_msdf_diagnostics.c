/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* text_msdf_diagnostics - write offscreen text-rendering PNG baselines.
 *
 * Build:  cmake --build build --target example_c_tools_text_msdf_diagnostics
 * Run:    ./build/examples/c/tools/text_msdf_diagnostics
 * Output: build/text_diagnostics/baseline/
 */

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "_alloc.h"
#include "_compat.h"
#include "_scene.h"
#include "datoviz/app.h"
#include "datoviz/fileio.h"
#include "datoviz/scene.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define TEXT_DIAG_WIDTH  900u
#define TEXT_DIAG_HEIGHT 220u



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct TextDiagRenderer
{
    const char* name;
    DvzTextRenderer renderer;
} TextDiagRenderer;



typedef struct TextDiagSample
{
    const char* name;
    const char* text;
} TextDiagSample;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Create one directory if it does not already exist.
 *
 * @param path directory path
 * @return whether the directory exists or was created
 */
static bool make_dir_one(const char* path)
{
    ANN(path);
    if (mkdir(path, 0755) == 0 || errno == EEXIST)
        return true;
    dvz_fprintf(stderr, "failed to create directory %s: %s\n", path, strerror(errno));
    return false;
}



/**
 * Create the diagnostic output directory tree.
 *
 * @param path output directory
 * @return whether the directory exists or was created
 */
static bool make_output_dirs(const char* path)
{
    ANN(path);
    return make_dir_one("build") && make_dir_one("build/text_diagnostics") &&
           make_dir_one(path);
}



/**
 * Return the atlas generated for a renderer.
 *
 * @param scene the scene
 * @param renderer selected renderer
 * @return atlas pointer, or NULL if unavailable
 */
static DvzTextAtlas* renderer_atlas(DvzScene* scene, DvzTextRenderer renderer)
{
    ANN(scene);
    if (scene->font_count == 0)
        return NULL;
    DvzFont* font = &scene->fonts[0];
    if (renderer == DVZ_TEXT_RENDERER_BITMAP_ATLAS)
        return font->bitmap_atlas;
    if (renderer == DVZ_TEXT_RENDERER_MSDF_ATLAS)
        return font->msdf_atlas != NULL ? font->msdf_atlas : font->sdf_atlas;
    return font->sdf_atlas;
}



/**
 * Return a human-readable atlas backend name.
 *
 * @param backend atlas backend
 * @return backend name
 */
static const char* atlas_backend_name(DvzTextAtlasBackend backend)
{
    switch (backend)
    {
    case DVZ_TEXT_ATLAS_BACKEND_BUILTIN_BITMAP:
        return "builtin_bitmap";
    case DVZ_TEXT_ATLAS_BACKEND_FREETYPE_BITMAP:
        return "freetype_bitmap";
    case DVZ_TEXT_ATLAS_BACKEND_STB_SDF:
        return "stb_sdf";
    case DVZ_TEXT_ATLAS_BACKEND_MSDF:
        return "msdf";
    default:
        return "unknown";
    }
}


/**
 * Return a human-readable atlas encoding name.
 *
 * @param encoding atlas texture encoding
 * @return encoding name
 */
static const char* atlas_encoding_name(DvzTextAtlasEncoding encoding)
{
    switch (encoding)
    {
    case DVZ_TEXT_ATLAS_ENCODING_BITMAP_ALPHA:
        return "bitmap_alpha";
    case DVZ_TEXT_ATLAS_ENCODING_SDF_ALPHA:
        return "sdf_alpha";
    case DVZ_TEXT_ATLAS_ENCODING_MSDF_RGB:
        return "msdf_rgb";
    default:
        return "unknown";
    }
}


/**
 * Emit a parseable atlas metadata line for one diagnostic case.
 *
 * @param renderer renderer descriptor
 * @param sample sample descriptor
 * @param size text size
 * @param atlas generated atlas, or NULL
 * @param scene_path scene PNG path
 * @param atlas_path atlas PNG path
 */
static void emit_atlas_metadata(
    const TextDiagRenderer* renderer, const TextDiagSample* sample, float size,
    const DvzTextAtlas* atlas, const char* scene_path, const char* atlas_path)
{
    ANN(renderer);
    ANN(sample);
    ANN(scene_path);
    ANN(atlas_path);
    if (atlas == NULL)
    {
        dvz_fprintf(
            stdout,
            "{\"kind\":\"text_atlas\",\"renderer\":\"%s\",\"sample\":\"%s\","
            "\"size\":%.1f,\"backend\":\"none\",\"scene_png\":\"%s\","
            "\"atlas_png\":\"%s\"}\n",
            renderer->name, sample->name, size, scene_path, atlas_path);
        return;
    }

    dvz_fprintf(
        stdout,
        "{\"kind\":\"text_atlas\",\"renderer\":\"%s\",\"sample\":\"%s\","
        "\"size\":%.1f,\"backend\":\"%s\",\"encoding\":\"%s\","
        "\"width\":%u,\"height\":%u,\"channels\":%u,\"glyphs\":%u,"
        "\"capacity\":%u,\"missing\":%u,\"generation\":%" PRIu64 ","
        "\"pixel_height\":%.3f,\"pixel_range\":%.3f,\"ascent\":%.3f,"
        "\"descent\":%.3f,\"line_height\":%.3f,\"scene_png\":\"%s\","
        "\"atlas_png\":\"%s\"}\n",
        renderer->name, sample->name, size, atlas_backend_name(atlas->backend),
        atlas_encoding_name(atlas->encoding), atlas->width, atlas->height, atlas->channels,
        atlas->glyph_count, DVZ_SCENE_TEXT_ATLAS_MAX_GLYPHS, atlas->missing_glyph_count,
        atlas->generation, atlas->pixel_height, atlas->pixel_range, atlas->ascent,
        atlas->descent, atlas->line_height, scene_path, atlas_path);
}


/**
 * Write the generated atlas texture for one render.
 *
 * @param scene the scene
 * @param renderer selected renderer
 * @param path output PNG path
 */
static void write_atlas_png(DvzScene* scene, DvzTextRenderer renderer, const char* path)
{
    ANN(scene);
    ANN(path);
    DvzTextAtlas* atlas = renderer_atlas(scene, renderer);
    if (atlas == NULL || atlas->field == NULL || atlas->field->data == NULL)
        return;
    if (dvz_write_png(path, atlas->width, atlas->height, (const uint8_t*)atlas->field->data) != 0)
        dvz_fprintf(stderr, "failed to write %s\n", path);
}



/**
 * Configure one text visual.
 *
 * @param visual the text visual
 * @param renderer selected renderer
 * @param text UTF-8 text string
 * @param size text size
 * @return whether configuration succeeded
 */
static bool configure_text_visual(
    DvzVisual* visual, DvzTextRenderer renderer, const char* text, float size)
{
    ANN(visual);
    ANN(text);
    if (dvz_text_set_renderer(visual, renderer) != 0)
        return false;

    const char* strings[1] = {text};
    float positions[1][3] = {{36.0f, 94.0f, 0.0f}};
    float anchors[1][2] = {{0.0f, 0.0f}};
    float sizes[1] = {size};
    float angles[1] = {0.0f};
    DvzColor colors[1] = {{222, 238, 255, 255}};
    DvzVisualDataUpdate updates[5] = {
        {.attr_name = "position", .data = positions, .item_count = 1},
        {.attr_name = "anchor", .data = anchors, .item_count = 1},
        {.attr_name = "size", .data = sizes, .item_count = 1},
        {.attr_name = "color", .data = colors, .item_count = 1},
        {.attr_name = "angle", .data = angles, .item_count = 1},
    };
    return dvz_visual_set_strings(visual, "text", strings, 1) == 0 &&
           dvz_visual_set_data_many(visual, updates, 5) == 0;
}



/**
 * Render one diagnostic case and write scene and atlas PNGs.
 *
 * @param out_dir output directory
 * @param renderer renderer descriptor
 * @param sample sample text descriptor
 * @param size text size
 * @return zero on success
 */
static int render_case(
    const char* out_dir, const TextDiagRenderer* renderer, const TextDiagSample* sample, float size)
{
    ANN(out_dir);
    ANN(renderer);
    ANN(sample);
    DvzScene* scene = dvz_scene();
    if (scene == NULL)
        return -1;
    DvzFigure* figure = dvz_figure(scene, TEXT_DIAG_WIDTH, TEXT_DIAG_HEIGHT, 0);
    DvzPanel* panel = dvz_panel(
        figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    if (figure == NULL || panel == NULL)
    {
        dvz_scene_destroy(scene);
        return -1;
    }
    dvz_panel_set_background_color(panel, 0.055f, 0.065f, 0.085f, 1.0f);

    DvzVisual* text = dvz_text(scene, 0);
    if (text == NULL || !configure_text_visual(text, renderer->renderer, sample->text, size))
    {
        dvz_scene_destroy(scene);
        return -1;
    }
    DvzVisualAttachDesc attach = {
        .z_layer = 1,
        .controller_mode = DVZ_CONTROLLER_FIXED,
    };
    if (dvz_panel_add_visual(panel, text, &attach) != 0)
    {
        dvz_scene_destroy(scene);
        return -1;
    }

    DvzAppConfig cfg = dvz_app_config();
    cfg.schedule_mode = DVZ_APP_SCHEDULE_ON_DEMAND;
    DvzApp* app = dvz_app_with_config(scene, &cfg);
    if (app == NULL)
    {
        dvz_scene_destroy(scene);
        return -1;
    }
    DvzAppWindow* win = dvz_app_window(app, figure, TEXT_DIAG_WIDTH, TEXT_DIAG_HEIGHT);
    if (win == NULL)
    {
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return -1;
    }

    int render_status = dvz_app_window_render_once(win);
    if (render_status < 0)
    {
        dvz_fprintf(stderr, "render failed for %s/%s/%.0f\n", renderer->name, sample->name, size);
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return -1;
    }

    char scene_path[512] = {0};
    char atlas_path[512] = {0};
    dvz_snprintf(
        scene_path, sizeof(scene_path), "%s/%s_%s_%02u_scene.png", out_dir, renderer->name,
        sample->name, (uint32_t)size);
    dvz_snprintf(
        atlas_path, sizeof(atlas_path), "%s/%s_%s_%02u_atlas.png", out_dir, renderer->name,
        sample->name, (uint32_t)size);
    if (dvz_app_window_capture_png(win, scene_path) != 0)
        dvz_fprintf(stderr, "failed to capture %s\n", scene_path);
    write_atlas_png(scene, renderer->renderer, atlas_path);

    emit_atlas_metadata(
        renderer, sample, size, renderer_atlas(scene, renderer->renderer), scene_path, atlas_path);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}



/*************************************************************************************************/
/*  Entry-point                                                                                  */
/*************************************************************************************************/

/**
 * Write text-rendering diagnostic PNG files.
 *
 * @param argc argument count
 * @param argv arguments
 * @return process exit code
 */
int main(int argc, char** argv)
{
    const char* out_dir = argc >= 2 ? argv[1] : "build/text_diagnostics/baseline";
    if (!make_output_dirs(out_dir))
        return 1;

    const TextDiagRenderer renderers[] = {
        {.name = "small_bitmap", .renderer = DVZ_TEXT_RENDERER_SMALL_BITMAP_ATLAS},
        {.name = "freetype_bitmap", .renderer = DVZ_TEXT_RENDERER_BITMAP_ATLAS},
        {.name = "msdf", .renderer = DVZ_TEXT_RENDERER_MSDF_ATLAS},
    };
    const TextDiagSample samples[] = {
        {.name = "glyphs", .text = "e b S  space  0123456789"},
        {.name = "sentence", .text = "The quick brown fox jumps over 13 lazy glyphs."},
        {.name = "utf8", .text = "UTF-8: cafe" "\xCC" "\x81" " cafe" "\xC3" "\xA9" " A"
                                  "\xCE" "\xA9" "B -> ?"},
    };
    const float sizes[] = {14.0f, 24.0f, 64.0f, 128.0f};
    int failed = 0;
    for (uint32_t ri = 0; ri < (uint32_t)(sizeof(renderers) / sizeof(renderers[0])); ri++)
    {
        for (uint32_t si = 0; si < (uint32_t)(sizeof(samples) / sizeof(samples[0])); si++)
        {
            for (uint32_t zi = 0; zi < (uint32_t)(sizeof(sizes) / sizeof(sizes[0])); zi++)
            {
                if (render_case(out_dir, &renderers[ri], &samples[si], sizes[zi]) != 0)
                    failed++;
            }
        }
    }
    return failed == 0 ? 0 : 1;
}
