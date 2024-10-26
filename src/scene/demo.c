/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Demo                                                                                         */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_macros.h"
#include "datoviz.h"
#include <string.h>



/*************************************************************************************************/
/*  CONSTANTS                                                                                    */
/*************************************************************************************************/

#define LEGEND_FONT_SIZE 24
#define LEGEND_ANCHOR    +10, -25



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static void legend(DvzBatch* batch, DvzPanel* panel, const char* text, DvzAtlasFont* af)
{
    DvzVisual* glyph = dvz_glyph(batch, 0);
    dvz_glyph_atlas(glyph, af->atlas);

    uint32_t n = strnlen(text, 1024);
    dvz_glyph_alloc(glyph, n);

    vec3* pos = dvz_mock_fixed(n, (vec3){-1, +1, 0});
    dvz_glyph_position(glyph, 0, n, pos, 0);

    DvzColor* color = dvz_mock_monochrome(n, (DvzColor){10, 10, 10, 255});
    dvz_glyph_color(glyph, 0, n, color, 0);
    dvz_glyph_ascii(glyph, text);

    vec4* xywh = dvz_font_ascii(af->font, text);
    dvz_glyph_xywh(glyph, 0, n, xywh, (vec2){LEGEND_ANCHOR}, 0);

    FREE(pos);
    FREE(color);
    FREE(xywh);

    dvz_panel_visual(panel, glyph, DVZ_VIEW_FLAGS_STATIC);
}



/*************************************************************************************************/
/*  Demo                                                                                         */
/*************************************************************************************************/

void dvz_demo(void)
{
    // Create app object.
    DvzApp* app = dvz_app(DVZ_APP_FLAGS_WHITE_BACKGROUND);
    DvzBatch* batch = dvz_app_batch(app);

    // Create a scene.
    DvzScene* scene = dvz_scene(batch);

    DvzAtlasFont af = dvz_atlas_font(LEGEND_FONT_SIZE);

    // Create a figure.
    uint32_t W = 1000;
    uint32_t H = 1000;
    DvzFigure* figure = dvz_figure(scene, W, H, 0);

    // Panel grid.
    uint32_t n_rows = 4;
    uint32_t n_cols = 4;
    float w = W * 1.0 / n_cols;
    float h = H * 1.0 / n_rows;

    // Panels.
    DvzPanel* p00 = dvz_panel(figure, 0 * w, 0 * h, w, h);
    DvzPanel* p01 = dvz_panel(figure, 1 * w, 0 * h, w, h);
    DvzPanel* p02 = dvz_panel(figure, 2 * w, 0 * h, w, h);
    DvzPanel* p03 = dvz_panel(figure, 3 * w, 0 * h, w, h);

    DvzPanel* p10 = dvz_panel(figure, 0 * w, 1 * h, w, h);
    DvzPanel* p11 = dvz_panel(figure, 1 * w, 1 * h, w, h);
    DvzPanel* p12 = dvz_panel(figure, 2 * w, 1 * h, w, h);
    DvzPanel* p13 = dvz_panel(figure, 3 * w, 1 * h, w, h);

    DvzPanel* p20 = dvz_panel(figure, 0 * w, 2 * h, w, h);
    DvzPanel* p21 = dvz_panel(figure, 1 * w, 2 * h, w, h);
    DvzPanel* p22 = dvz_panel(figure, 2 * w, 2 * h, w, h);
    DvzPanel* p23 = dvz_panel(figure, 3 * w, 2 * h, w, h);

    DvzPanel* p30 = dvz_panel(figure, 0 * w, 3 * h, w, h);
    DvzPanel* p31 = dvz_panel(figure, 1 * w, 3 * h, w, h);
    DvzPanel* p32 = dvz_panel(figure, 2 * w, 3 * h, w, h);
    DvzPanel* p33 = dvz_panel(figure, 3 * w, 3 * h, w, h);


    // Common data.
    uint32_t n = 6;
    float a = 1.0;
    vec3* pos = dvz_mock_band(n, (vec2){a, a});
    DvzColor* color = dvz_mock_cmap(n, DVZ_CMAP_VIRIDIS, 255);
    float* size = dvz_mock_full(n, 50.0);

    const uint32_t n2 = n * 15;
    vec3* pos2 = dvz_mock_band(n2, (vec2){a, a});
    DvzColor* color2 = dvz_mock_cmap(n2, DVZ_CMAP_VIRIDIS, 255);
    float* size2 = dvz_mock_full(n2, 50.0);



    // --------------------------------------------------------------------------------------------
    // First row.
    // --------------------------------------------------------------------------------------------

    // 0,0  LINE_LIST
    DvzVisual* line_list = dvz_basic(batch, DVZ_PRIMITIVE_TOPOLOGY_LINE_LIST, 0);
    {
        dvz_basic_alloc(line_list, n);
        dvz_basic_position(line_list, 0, n, pos, 0);
        dvz_basic_color(line_list, 0, n, color, 0);
    }

    legend(batch, p00, "LINE LIST", &af);
    dvz_panel_panzoom(p00);
    dvz_panel_visual(p00, line_list, 0);


    // 0,1  LINE_STRIP
    DvzVisual* line_strip = dvz_basic(batch, DVZ_PRIMITIVE_TOPOLOGY_LINE_STRIP, 0);
    {
        dvz_basic_alloc(line_strip, n);
        dvz_basic_position(line_strip, 0, n, pos, 0);
        dvz_basic_color(line_strip, 0, n, color, 0);
    }

    legend(batch, p01, "LINE STRIP", &af);
    dvz_panel_panzoom(p01);
    dvz_panel_visual(p01, line_strip, 0);


    // 0,2  TRIANGLE_LIST
    DvzVisual* triangle_list = dvz_basic(batch, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    {
        dvz_basic_alloc(triangle_list, n);
        dvz_basic_position(triangle_list, 0, n, pos, 0);
        dvz_basic_color(triangle_list, 0, n, color, 0);
    }

    legend(batch, p02, "TRIANGLE LIST", &af);
    dvz_panel_panzoom(p02);
    dvz_panel_visual(p02, triangle_list, 0);


    // 0,3  TRIANGLE_STRIP
    DvzVisual* triangle_strip = dvz_basic(batch, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, 0);
    {
        dvz_basic_alloc(triangle_strip, n);
        dvz_basic_position(triangle_strip, 0, n, pos, 0);
        dvz_basic_color(triangle_strip, 0, n, color, 0);
    }

    legend(batch, p03, "TRIANGLE STRIP", &af);
    dvz_panel_panzoom(p03);
    dvz_panel_visual(p03, triangle_strip, 0);



    // --------------------------------------------------------------------------------------------
    // Second row.
    // --------------------------------------------------------------------------------------------

    // 1,0  POINT_LIST
    DvzVisual* point_list = dvz_basic(batch, DVZ_PRIMITIVE_TOPOLOGY_POINT_LIST, 0);
    {
        dvz_basic_alloc(point_list, n2);
        dvz_basic_position(point_list, 0, n2, pos2, 0);
        dvz_basic_color(point_list, 0, n2, color2, 0);
        dvz_basic_size(point_list, 3.0f);
    }

    legend(batch, p10, "POINT LIST", &af);
    dvz_panel_panzoom(p10);
    dvz_panel_visual(p10, point_list, 0);


    // 1,1  PIXEL
    DvzVisual* pixel = dvz_pixel(batch, 0);
    {
        dvz_basic_alloc(pixel, n2);
        dvz_basic_position(pixel, 0, n2, pos2, 0);
        dvz_basic_color(pixel, 0, n2, color2, 0);
    }

    legend(batch, p11, "PIXEL", &af);
    dvz_panel_panzoom(p11);
    dvz_panel_visual(p11, pixel, 0);


    // 1,2  POINT
    DvzVisual* point = dvz_point(batch, 0);
    {
        dvz_point_alloc(point, n);
        dvz_point_position(point, 0, n, pos, 0);
        dvz_point_color(point, 0, n, color, 0);
        dvz_point_size(point, 0, n, size, 0);
    }

    legend(batch, p12, "POINT", &af);
    dvz_panel_panzoom(p12);
    dvz_panel_visual(p12, point, 0);


    // 1,3  MARKER
    DvzVisual* marker = dvz_marker(batch, 0);
    {
        dvz_marker_alloc(marker, n);
        dvz_marker_position(marker, 0, n, pos, 0);
        dvz_marker_color(marker, 0, n, color, 0);
        dvz_marker_size(marker, 0, n, size, 0);

        float* angle = dvz_mock_linspace(n, 0, M_2PI / n * (n + 1));
        dvz_marker_angle(marker, 0, n, angle, 0);
        FREE(angle);

        dvz_marker_aspect(marker, DVZ_MARKER_ASPECT_OUTLINE);
        dvz_marker_shape(marker, DVZ_MARKER_SHAPE_CLUB);
        dvz_marker_edge_color(marker, DVZ_WHITE);
        dvz_marker_edge_width(marker, (float){3.0});
    }

    legend(batch, p13, "MARKER", &af);
    dvz_panel_panzoom(p13);
    dvz_panel_visual(p13, marker, 0);



    // --------------------------------------------------------------------------------------------
    // Third row.
    // --------------------------------------------------------------------------------------------

    // 2,0  GLYPH
    DvzVisual* glyph = dvz_glyph(batch, 0);
    {
        const char* text = "ABCDEF";
        dvz_glyph_atlas(glyph, af.atlas);
        dvz_glyph_alloc(glyph, n);
        dvz_glyph_position(glyph, 0, n, pos, 0);
        dvz_glyph_color(glyph, 0, n, color, 0);

        // HACK: just reuse the bigger array size2 here instead of an array with n vec2.
        ASSERT(n2 >= 2 * n);
        dvz_glyph_size(glyph, 0, n, (vec2*)size2, 0);

        dvz_glyph_ascii(glyph, text);
        vec4* xywh = dvz_font_ascii(af.font, text);
        // NOTE: set the offset of each glyph to (0,0) for this demo.
        // We still need the glyph size.
        for (uint32_t i = 0; i < n; i++)
        {
            xywh[i][0] = 0;
            xywh[i][1] = 0;
        }
        dvz_glyph_xywh(glyph, 0, n, xywh, (vec2){0, 0}, 0);
        FREE(xywh);
    }

    legend(batch, p20, "GLYPH", &af);
    dvz_panel_panzoom(p20);
    dvz_panel_visual(p20, glyph, 0);


    // 2,1  SEGMENT
    DvzVisual* segment = dvz_segment(batch, 0);
    {
        dvz_segment_alloc(segment, n);

        vec3* initial = dvz_mock_line(n / 2, (vec3){-a / 2, -a / 2, 0}, (vec3){+a / 2, -a / 2, 0});
        vec3* terminal =
            dvz_mock_line(n / 2, (vec3){-a / 2, +a / 2, 0}, (vec3){+a / 2, +a / 2, 0});
        DvzColor* segment_color = dvz_mock_cmap(n / 2, DVZ_CMAP_VIRIDIS, DVZ_ALPHA_MAX);
        float* linewidth = dvz_mock_linspace(n, 20.0, 80.0);
        DvzCapType* cap = (DvzCapType*)dvz_mock_range(n / 2, 1);

        dvz_segment_position(segment, 0, n / 2, initial, terminal, 0);
        dvz_segment_color(segment, 0, n / 2, segment_color, 0);
        dvz_segment_linewidth(segment, 0, n, linewidth, 0);
        dvz_segment_cap(segment, 0, n, cap, cap, 0);

        FREE(initial);
        FREE(terminal);
        FREE(segment_color);
        FREE(linewidth);
        FREE(cap);
    }

    legend(batch, p21, "SEGMENT", &af);
    dvz_panel_panzoom(p21);
    dvz_panel_visual(p21, segment, 0);


    // 2,2  PATH
    DvzVisual* path = dvz_path(batch, 0);
    {
        dvz_path_alloc(path, n2);

        vec3* positions = (vec3*)calloc(n2, sizeof(vec3));
        float t = 0;
        for (uint32_t i = 0; i < n2; i++)
        {
            positions[i][0] = -a / 2 + a / n2 * i;
            positions[i][1] = a / 2. * sin(M_2PI * i * 2.0 / n2);
        }

        dvz_path_position(path, n2, positions, 1, (uint32_t[]){n2}, 0);
        dvz_path_color(path, 0, n2, color2, 0);

        FREE(positions)
    }

    legend(batch, p22, "PATH", &af);
    dvz_panel_panzoom(p22);
    dvz_panel_visual(p22, path, 0);


    // 2,3  IMAGE
    DvzVisual* image = dvz_image(batch, 0);
    {
        uint32_t wi = 200;
        uint32_t hi = 200;
        DvzColor* imgdata = (DvzColor*)calloc(wi * hi, sizeof(DvzColor));

        float xmin = -10.0f;
        float xmax = +10.0f;
        float ymin = -10.0f;
        float ymax = +10.0f;

        float vmin = -1.0f;
        float vmax = 1.0f;
        float x = 0, y = 0, r = 0, value = 0;
        for (uint32_t j = 0; j < hi; j++)
        {
            for (uint32_t i = 0; i < wi; i++)
            {
                x = xmin + (xmax - xmin) * i / (wi - 1);
                y = ymin + (ymax - ymin) * j / (hi - 1);
                r = sqrtf(x * x + y * y);
                value = r == 0 ? 1 : sinf(r) / r;
                dvz_colormap_scale(
                    DVZ_CMAP_VIRIDIS, value, vmin, vmax, (DvzAlpha*)&imgdata[j * wi + i]);
            }
        }
        DvzId tex = dvz_tex_image(batch, DVZ_FORMAT_COLOR, wi, hi, imgdata);

        dvz_image_alloc(image, 1);
        dvz_image_position(image, 0, 1, (vec3[]){{0, 0, 0}}, 0);
        dvz_image_size(image, 0, 1, (vec2[]){{wi, hi}}, 0);
        dvz_image_anchor(image, 0, 1, (vec2[]){{.5, .35}}, 0);
        dvz_image_texcoords(image, 0, 1, (vec4[]){{0, 0, +1, +1}}, 0);
        dvz_image_texture(image, tex, DVZ_FILTER_LINEAR, DVZ_SAMPLER_ADDRESS_MODE_REPEAT);

        FREE(imgdata);
    }

    legend(batch, p23, "IMAGE", &af);
    dvz_panel_panzoom(p23);
    dvz_panel_visual(p23, image, 0);



    // --------------------------------------------------------------------------------------------
    // Fourth row.
    // --------------------------------------------------------------------------------------------

    // 3,0  MESH
    DvzShape shape = dvz_shape_cube(color);
    DvzVisual* mesh = dvz_mesh_shape(batch, &shape, DVZ_MESH_FLAGS_LIGHTING);

    legend(batch, p30, "MESH", &af);
    dvz_arcball_initial(dvz_panel_arcball(p30), (vec3){+0.4, -0.8, +2.9});
    dvz_camera_initial(
        dvz_panel_camera(p30, 0), (vec3){0, 0, 3}, (vec3){0, 0, 0}, (vec3){0, 1, 0});
    dvz_panel_visual(p30, mesh, 0);
    dvz_panel_update(p30);
    dvz_shape_destroy(&shape);


    // 3,1  SPHERE
    DvzVisual* sphere = dvz_sphere(batch, 0);
    {
        dvz_sphere_alloc(sphere, n);
        dvz_sphere_position(sphere, 0, n, pos, 0);
        dvz_sphere_color(sphere, 0, n, color, 0);

        float* sphere_size = dvz_mock_linspace(n, 50, 100);
        dvz_sphere_size(sphere, 0, n, sphere_size, 0);
        FREE(sphere_size);

        dvz_sphere_light_pos(sphere, (vec3){-1, 0, +10});
        dvz_sphere_light_params(sphere, (vec4){.5, .5, .5, 16});
    }

    legend(batch, p31, "SPHERE", &af);
    dvz_arcball_initial(dvz_panel_arcball(p31), (vec3){+0.4, -0.8, +2.9});
    dvz_camera_initial(
        dvz_panel_camera(p31, 0), (vec3){0, 0, 3}, (vec3){0, 0, 0}, (vec3){0, 1, 0});
    dvz_panel_visual(p31, sphere, 0);
    dvz_panel_update(p31);


    // 3,2  VOLUME
    DvzVisual* volume = dvz_volume(batch, DVZ_VOLUME_FLAGS_RGBA | DVZ_VOLUME_FLAGS_BACK_FRONT);
    DvzId tex = 0;
    {
        dvz_volume_alloc(volume, 1);
        uint32_t va = 7;
        uint32_t vb = va;
        uint32_t vc = va;
        uint32_t a0 = va / 2;
        uint32_t d0 = 1;
        uint8_t val0 = 2;
        uint8_t val1 = 0;
        DvzSize vsize = va * vb * vc * sizeof(cvec4);
        cvec4 col = {0};
        dvz_colormap_8bit(DVZ_CMAP_VIRIDIS, 128, col);
        col[3] = 64;

        // Generate the texture data.
        cvec4* tex_data = (cvec4*)calloc(va * vb * vc, sizeof(cvec4));
        printf("creating volume texture (%s)\n", pretty_size(vsize));
        memset(tex_data, val0, vsize);
        uint32_t idx = 0;
        for (uint32_t i = a0 - d0; i <= a0 + d0; i++)
        {
            for (uint32_t j = a0 - d0; j <= a0 + d0; j++)
            {
                for (uint32_t k = a0 - d0; k <= a0 + d0; k++)
                {
                    idx = vb * vc * i + vc * j + k;
                    ASSERT((4 * idx + 3) < va * vb * vc * 4);

                    for (uint32_t l = 0; l < 4; l++)
                    {
                        tex_data[idx][l] = col[l];
                    }
                }
            }
        }

        tex = dvz_tex_volume(batch, DVZ_FORMAT_R8G8B8A8_UNORM, va, vb, vc, tex_data);
        dvz_volume_texture(volume, tex, DVZ_FILTER_NEAREST, DVZ_SAMPLER_ADDRESS_MODE_REPEAT);
        FREE(tex_data);
    }

    legend(batch, p32, "VOLUME", &af);
    dvz_arcball_initial(dvz_panel_arcball(p32), (vec3){+0.4, -0.8, +2.9});
    dvz_camera_initial(
        dvz_panel_camera(p32, 0), (vec3){0, 0, 3}, (vec3){0, 0, 0}, (vec3){0, 1, 0});
    dvz_panel_visual(p32, volume, 0);
    dvz_panel_update(p32);


    // 3,3  SLICE
    DvzVisual* slice = dvz_slice(batch, DVZ_VOLUME_FLAGS_RGBA);
    {
        n = 12;
        dvz_slice_alloc(slice, n);

        vec3* p0 = (vec3*)calloc(n, sizeof(vec3));
        vec3* p1 = (vec3*)calloc(n, sizeof(vec3));
        vec3* p2 = (vec3*)calloc(n, sizeof(vec3));
        vec3* p3 = (vec3*)calloc(n, sizeof(vec3));
        vec3* uvw0 = (vec3*)calloc(n, sizeof(vec3));
        vec3* uvw1 = (vec3*)calloc(n, sizeof(vec3));
        vec3* uvw2 = (vec3*)calloc(n, sizeof(vec3));
        vec3* uvw3 = (vec3*)calloc(n, sizeof(vec3));

        float sa = 1;
        float dw = 2.0 * sa / (n - 1.0);
        float dt = 1.0 / (n - 1.0);
        float sw = 0, t = 0;
        for (uint32_t i = 0; i < n; i++)
        {
            sw = i * dw;
            p0[i][0] = -sa;
            p0[i][1] = +sa;
            p0[i][2] = -sa + sw;

            p1[i][0] = -sa;
            p1[i][1] = -sa;
            p1[i][2] = -sa + sw;

            p2[i][0] = +sa;
            p2[i][1] = -sa;
            p2[i][2] = -sa + sw;

            p3[i][0] = +sa;
            p3[i][1] = +sa;
            p3[i][2] = -sa + sw;

            t = i * dt;
            uvw0[i][0] = t;
            uvw0[i][1] = 0;
            uvw0[i][2] = 0;

            uvw1[i][0] = t;
            uvw1[i][1] = 1;
            uvw1[i][2] = 0;

            uvw2[i][0] = t;
            uvw2[i][1] = 1;
            uvw2[i][2] = 1;

            uvw3[i][0] = t;
            uvw3[i][1] = 0;
            uvw3[i][2] = 1;
        }

        dvz_slice_position(slice, 0, n, p0, p1, p2, p3, 0);
        dvz_slice_texcoords(slice, 0, n, uvw0, uvw1, uvw2, uvw3, 0);

        dvz_slice_alpha(slice, .5);
        dvz_slice_texture(slice, tex, DVZ_FILTER_LINEAR, DVZ_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

        FREE(p0);
        FREE(p1);
        FREE(p2);
        FREE(p3);
        FREE(uvw0);
        FREE(uvw1);
        FREE(uvw2);
        FREE(uvw3);
    }

    legend(batch, p33, "SLICE", &af);
    dvz_arcball_initial(dvz_panel_arcball(p33), (vec3){+0.4, -0.8, +2.9});
    dvz_camera_initial(
        dvz_panel_camera(p33, 0), (vec3){0, 0, 3}, (vec3){0, 0, 0}, (vec3){0, 1, 0});
    dvz_panel_visual(p33, slice, 0);
    dvz_panel_update(p33);


    dvz_scene_run(scene, app, checkenv("DVZ_DEBUG") ? 5 : 0);
    dvz_scene_destroy(scene);
    dvz_app_destroy(app);

    FREE(pos);
    FREE(size);
    FREE(color);

    FREE(pos2);
    FREE(size2);
    FREE(color2);

    dvz_atlas_destroy(af.atlas);
    dvz_font_destroy(af.font);

    return;
}
