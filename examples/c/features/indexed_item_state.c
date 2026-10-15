/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* This example styles complementary clipped halves through one copied state bank.
 *
 * What to look for: the left half remains selected while a white hover tint alternates
 * between both halves. Selection context dims the right half. The application composes
 * complete replacement words and updates only slots 11 and 4000, without replacing geometry
 * or author colors. Complementary signed distances preserve a full source triangle footprint with
 * exact-zero ownership. The same retained lookup and coverage run through native and WebGPU WBOIT
 * shaders.
 *
 * Scenario: features_indexed_item_state
 * Build: just example-c features/indexed_item_state
 * Run: ./build/examples/c/features/indexed_item_state --live
 */

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_alloc.h"
#include "datoviz/scene.h"
#include "example_style.h"
#include "runner/scenario_runner.h"

DvzScenarioSpec dvz_example_indexed_item_state_scenario(void);



typedef struct IndexedStateExample
{
    DvzItemStateBuffer* bank;
    uint32_t hovered;
} IndexedStateExample;



/** Duplicate a full crossing triangle, with complementary half-open fragment coverage.
 * @param ctx scenario context
 * @param out_user application compositor state
 * @return whether construction succeeded
 */
static bool _scenario_init(DvzScenarioContext* ctx, void** out_user)
{
    IndexedStateExample* state = dvz_calloc(1, sizeof(IndexedStateExample));
    if (state == NULL)
        return false;
    *out_user = state;
    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    DvzPanel* panel = dvz_panel_full(ctx->figure);
    DvzVisual* mesh = dvz_mesh(ctx->scene, 0);
    if (panel == NULL || mesh == NULL)
        return false;
    example_graphite_cyan_set_panel_background(panel);
    vec3 positions[6] = {{-.85f, -.65f, 0}, {.85f, -.65f, 0}, {0, .65f, 0},
                         {-.85f, -.65f, 0},  {.85f, -.65f, 0},  {0, .65f, 0}};
    vec2 clip[6] = {{.85f, 0}, {-.85f, 0}, {-0.f, 0}, {-.85f, 1}, {.85f, 1}, {0, 1}};
    vec3 normals[6] = {{0, 0, 1}, {0, 0, 1}, {0, 0, 1}, {0, 0, 1}, {0, 0, 1}, {0, 0, 1}};
    DvzColor colors[6] = {{38, 132, 167, 255}, {38, 132, 167, 255}, {38, 132, 167, 255},
                          {42, 176, 142, 255}, {42, 176, 142, 255}, {42, 176, 142, 255}};
    DvzIndex geometry_indices[6] = {0, 1, 2, 3, 4, 5};
    uint32_t state_indices[6] = {11, 11, 11, 4000, 4000, 4000};
    if (dvz_visual_set_data(mesh, "position", positions, 6) != DVZ_OK ||
        dvz_visual_set_data(mesh, "normal", normals, 6) != DVZ_OK ||
        dvz_visual_set_data(mesh, "color", colors, 6) != DVZ_OK ||
        dvz_visual_set_index_data(mesh, geometry_indices, 6) != DVZ_OK)
        return false;
    if (dvz_mesh_set_fragment_clip(mesh, (const vec2*)clip, 6) != DVZ_OK)
        return false;
    DvzItemStateBufferDesc desc = dvz_item_state_buffer_desc();
    desc.slot_count = 4096;
    state->bank = dvz_item_state_buffer(ctx->scene, &desc);
    if (state->bank == NULL ||
        dvz_visual_set_item_state_buffer(mesh, state->bank, state_indices, 6) != DVZ_OK)
        return false;
    DvzSelectionVisualStyle selection = dvz_selection_visual_style();
    selection.unselected_alpha = .35f;
    DvzItemStateVisualStyle hovered = dvz_item_state_visual_style();
    hovered.visual_flags = DVZ_ITEM_STATE_VISUAL_TINT;
    hovered.tint = dvz_color_rgba(255, 255, 255, 255);
    hovered.tint_mix = .65f;
    uint32_t slots[2] = {11, 4000};
    uint32_t words[2] = {DVZ_ITEM_STATE_SELECTED | DVZ_ITEM_STATE_HOVERED, 0};
    state->hovered = 11;
    if (dvz_item_state_buffer_style(state->bank, &selection, &hovered, true) != DVZ_OK ||
        dvz_item_state_buffer_update(state->bank, slots, words, 2) != DVZ_OK ||
        dvz_visual_set_alpha_mode(mesh, DVZ_ALPHA_WBOIT) != DVZ_OK)
        return false;
    return dvz_panel_add_visual(panel, mesh, NULL) == DVZ_OK;
}



/** Move hover while the application retains the selected bit in the complete replacement word.
 * @param ctx scenario context
 * @param user application compositor state
 */
static void _scenario_frame(DvzScenarioContext* ctx, void* user)
{
    IndexedStateExample* state = user;
    double time = ctx->preview_mode ? dvz_scenario_preview_time(ctx) : ctx->time;
    uint32_t hovered = ((uint32_t)time % 2u) == 0 ? 11 : 4000;
    if (hovered == state->hovered)
        return;
    uint32_t slots[2] = {11, 4000};
    uint32_t words[2] = {DVZ_ITEM_STATE_SELECTED, 0};
    words[hovered == 11 ? 0 : 1] |= DVZ_ITEM_STATE_HOVERED;
    if (dvz_item_state_buffer_update(state->bank, slots, words, 2) == DVZ_OK)
        state->hovered = hovered;
}



/** Release host compositor storage; the scene owns the bank.
 * @param ctx scenario context
 * @param user compositor storage
 */
static void _scenario_destroy(DvzScenarioContext* ctx, void* user)
{
    (void)ctx;
    dvz_free(user);
}



/** Return the portable indexed-state scenario.
 * @return the scenario
 */
DvzScenarioSpec dvz_example_indexed_item_state_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "features_indexed_item_state",
        .title = "Indexed Item State",
        .width = EXAMPLE_WINDOW_WIDTH,
        .height = EXAMPLE_WINDOW_HEIGHT,
        .fps = 60,
        .continuous_frames = true,
        .init = _scenario_init,
        .frame = _scenario_frame,
        .destroy = _scenario_destroy};
}



#ifndef DVZ_EXAMPLE_NO_MAIN
/** Run the portable scenario.
 * @param argc argument count
 * @param argv arguments
 * @return process status
 */
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = dvz_example_indexed_item_state_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
#endif
