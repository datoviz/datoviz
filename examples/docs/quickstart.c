#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "datoviz/app.h"
#include "datoviz/scene.h"

#define WIDTH  1280u
#define HEIGHT 720u
#define N      10000u

static uint32_t _random_state = 12345u;

static uint32_t _random_u32(void)
{
    uint32_t x = _random_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return _random_state = x;
}

static float _random_f32(float min, float max)
{
    return min + (max - min) * ((float)_random_u32() / (float)UINT32_MAX);
}

int main(int argc, char** argv)
{
    int rc = EXIT_FAILURE;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;
    float* positions = calloc(N * 3u, sizeof(*positions));
    uint8_t* colors = calloc(N * 4u, sizeof(*colors));
    float* diameters = calloc(N, sizeof(*diameters));
    if (positions == NULL || colors == NULL || diameters == NULL)
    {
        fprintf(stderr, "quickstart: data allocation failed\n");
        goto cleanup;
    }

    for (uint32_t i = 0; i < N; i++)
    {
        positions[3 * i + 0] = _random_f32(-1.0f, +1.0f);
        positions[3 * i + 1] = _random_f32(-1.0f, +1.0f);
        colors[4 * i + 0] = (uint8_t)_random_u32();
        colors[4 * i + 1] = (uint8_t)_random_u32();
        colors[4 * i + 2] = (uint8_t)_random_u32();
        colors[4 * i + 3] = 200;
        diameters[i] = _random_f32(4.0f, 12.0f);
    }

    scene = dvz_scene();
    DvzFigure* figure = scene != NULL ? dvz_figure(scene, WIDTH, HEIGHT, 0) : NULL;
    DvzPanel* panel = figure != NULL ? dvz_panel_full(figure) : NULL;
    if (scene == NULL || figure == NULL || panel == NULL)
    {
        fprintf(stderr, "quickstart: scene, figure, or panel creation failed\n");
        goto cleanup;
    }

    DvzColor background = {13, 18, 25, 255};
    if (dvz_panel_set_background_color(panel, background) != 0)
        goto api_error;
    DvzController* controller = dvz_panzoom(scene, NULL);
    if (controller == NULL || dvz_panel_bind_controller(panel, controller, DVZ_DIM_MASK_XY) != 0)
        goto api_error;

    DvzVisual* points = dvz_point(scene, 0);
    if (points == NULL)
        goto api_error;
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = N},
        {.attr_name = "color", .data = colors, .item_count = N},
        {.attr_name = "diameter_px", .data = diameters, .item_count = N},
    };
    if (dvz_visual_set_data_many(points, updates, 3) != 0)
        goto api_error;
    DvzPointStyleDesc style = dvz_point_style_desc();
    style.aspect = DVZ_SHAPE_ASPECT_FILLED;
    style.stroke_width_px = 0.0f;
    if (dvz_point_set_style(points, &style) != 0 ||
        dvz_visual_set_depth_test(points, false) != 0 ||
        dvz_visual_set_alpha_mode(points, DVZ_ALPHA_BLENDED) != 0 ||
        dvz_panel_add_visual(panel, points, NULL) != 0)
        goto api_error;

    app = dvz_app(scene);
    DvzView* view =
        app != NULL ? dvz_view_window(app, figure, WIDTH, HEIGHT, "Datoviz Quickstart") : NULL;
    if (app == NULL || view == NULL)
        goto api_error;
    uint32_t frame_count =
        argc == 3 && strcmp(argv[1], "--frames") == 0 ? (uint32_t)strtoul(argv[2], NULL, 10) : 0;
    dvz_app_run(app, frame_count);
    rc = EXIT_SUCCESS;
    goto cleanup;

api_error:
    fprintf(stderr, "quickstart: Datoviz setup failed\n");

cleanup:
    if (app != NULL)
        dvz_app_destroy(app);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    free(diameters);
    free(colors);
    free(positions);
    return rc;
}
