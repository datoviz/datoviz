/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  GUI tests                                                                                    */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_gui.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../_gui.h"
#include "../../app/_app.h"
#include "_alloc.h"
#include "_assertions.h"
#include "_log.h"
#include "datoviz/app.h"
#include "datoviz/canvas.h"
#include "datoviz/controller/arcball.h"
#include "datoviz/fileio/fileio.h"
#include "datoviz/gui.h"
#include "datoviz/imgui.h"
#include "datoviz/input/pointer.h"
#include "datoviz/input/router.h"
#include "datoviz/scene.h"
#include "datoviz/vk/gpu_ctx.h"
#include "datoviz/window.h"
#include "datoviz_testing.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct GuiViewportSmoke
{
    DvzGuiViewport* viewport;
    uint32_t frame;
    uint32_t shown_count;
    uint32_t hidden_count;
    uint32_t transition_drawable_count;
    uint32_t visible_transition_count;
    uint32_t visible_transition_shown_count;
} GuiViewportSmoke;



typedef struct GuiInputRecorder
{
    uint32_t count;
    uint32_t drag_count;
    uint32_t union_types[DVZ_POINTER_EVENT_WHEEL + 1];
    DvzPointerEventType last_type;
} GuiInputRecorder;



typedef struct GuiTestGpuResources
{
    DvzGpuCtx* gpu_ctx;
    DvzWindowHost* window_host;
} GuiTestGpuResources;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return whether a GUI smoke can use the active GPU/GLFW path.
 *
 * @return whether the smoke can run
 */
static bool _gui_smoke_available(void)
{
#if defined(DVZ_HAS_GUI) && DVZ_HAS_GUI && defined(DVZ_HAS_GLFW) && DVZ_HAS_GLFW
    return true;
#else
    return false;
#endif
}


/**
 * Check that the build-generated GUI font resources are present and non-empty.
 *
 * @param suite test context
 * @param item test case descriptor
 * @return zero on success
 */
int test_gui_embedded_font_resources(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    const char* names[] = {
        "SourceSans3_Regular", "SourceSans3_Bold",     "SourceSans3_It",
        "SourceSans3_BoldIt",  "SourceCodePro_Regular", "NotoSansMath_Regular"};
    for (uint32_t i = 0; i < 6; i++)
    {
        DvzSize size = 0;
        const unsigned char* bytes = dvz_resource_font(names[i], &size);
        AT(bytes != NULL);
        AT(size > 1024);
    }
    return 0;
}


/**
 * Verify the transfer function used for ImGui vertex colors on sRGB targets.
 */
int test_gui_srgb_vertex_colors(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    AT(_dvz_gui_srgb_to_linear_u8(0) == 0);
    AT(_dvz_gui_srgb_to_linear_u8(15) == 1);
    AT(_dvz_gui_srgb_to_linear_u8(128) == 55);
    AT(_dvz_gui_srgb_to_linear_u8(255) == 255);
    return 0;
}



/**
 * Verify retained data widgets copy keys and preserve queryable state without a GPU.
 */
int test_gui_data_widget_storage(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    const uint64_t keys[] = {7, 9};
    const uint32_t parents[] = {UINT32_MAX, 0};
    const char* labels[] = {"root", "child"};
    DvzGuiTree* tree = dvz_gui_tree("storage", DVZ_GUI_DATA_WIDGET_FLAGS_MULTI_SELECT);
    AT(tree != NULL);
    AT(dvz_gui_tree_set_rows(tree, 2, keys, parents, labels, NULL, 0) == DVZ_OK);
    AT(dvz_gui_tree_set_selection(tree, 1, &keys[1]) == DVZ_OK);
    uint64_t selected = 0;
    AT(dvz_gui_tree_get_selection(tree, 1, &selected) == 1);
    AT(selected == 9);
    AT(dvz_gui_tree_expand_all(tree) == DVZ_OK);
    AT(dvz_gui_tree_get_expanded(tree, 0, NULL) == 2);
    dvz_gui_tree_destroy(tree);
    return 0;
}


/**
 * Verify tree validation, state preservation, filtering, and cached flattening without a GPU.
 */
int test_gui_data_tree_model(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    AT(dvz_gui_tree("", 0) == NULL);
    AT(dvz_gui_tree("bad", 1u << 20) == NULL);
    DvzGuiTree* tree = dvz_gui_tree("tree-model", DVZ_GUI_DATA_WIDGET_FLAGS_FILTER);
    AT(tree != NULL);
    DvzGuiTreeLayout layout = dvz_gui_tree_layout();
    AT(layout.struct_size == sizeof(layout));
    AT(layout.indent_em < 1.0f);
    AT(dvz_gui_tree_set_layout(tree, &layout) == DVZ_OK);
    layout.indent_em = -1.0f;
    AT(dvz_gui_tree_set_layout(tree, &layout) == DVZ_ERROR);
    const uint64_t keys[] = {10, 11, 12, 20};
    const uint32_t parents[] = {UINT32_MAX, 0, 1, UINT32_MAX};
    const char* labels[] = {"Root", "Branch", "Needle", "Other"};
    const char* secondary[] = {"", "secondary", "leaf", ""};
    AT(dvz_gui_tree_set_rows(tree, 4, keys, parents, labels, secondary, 0) == DVZ_OK);
    DvzGuiDataDebugState state = {};
    AT(_dvz_gui_tree_debug_state(tree, &state));
    AT(state.row_count == 4);
    AT(state.display_count == 2);
    const uint64_t rebuild = state.rebuild_count;
    AT(_dvz_gui_tree_debug_state(tree, &state));
    AT(state.rebuild_count == rebuild);

    AT(dvz_gui_tree_set_filter(tree, "needle") == DVZ_OK);
    AT(_dvz_gui_tree_debug_state(tree, &state));
    AT(state.display_count == 3);
    AT(_dvz_gui_tree_debug_display_key(tree, 0) == 10);
    AT(_dvz_gui_tree_debug_display_key(tree, 2) == 12);
    const bool visible[] = {false, true, true, true};
    AT(dvz_gui_tree_set_visible(tree, 4, visible) == DVZ_OK);
    AT(_dvz_gui_tree_debug_state(tree, &state));
    AT(state.display_count == 0);
    AT(dvz_gui_tree_set_visible(tree, 0, NULL) == DVZ_OK);
    const bool matches[] = {true, false, false, true};
    AT(dvz_gui_tree_set_filter(tree, "") == DVZ_OK);
    AT(dvz_gui_tree_set_matches(tree, 4, matches) == DVZ_OK);
    AT(_dvz_gui_tree_debug_state(tree, &state));
    AT(state.display_count == 2);
    AT(_dvz_gui_tree_debug_display_key(tree, 1) == 20);
    AT(dvz_gui_tree_set_matches(tree, 0, NULL) == DVZ_OK);

    AT(dvz_gui_tree_expand_all(tree) == DVZ_OK);
    AT(dvz_gui_tree_set_selection(tree, 1, &keys[2]) == DVZ_OK);
    const uint64_t reordered[] = {20, 10, 11, 12};
    const uint32_t reordered_parents[] = {UINT32_MAX, UINT32_MAX, 1, 2};
    const char* reordered_labels[] = {"Other", "Root", "Branch", "Needle"};
    AT(dvz_gui_tree_set_rows(tree, 4, reordered, reordered_parents, reordered_labels, NULL, 0) ==
       DVZ_OK);
    uint64_t selected = 0;
    AT(dvz_gui_tree_get_selection(tree, 1, &selected) == 1);
    AT(selected == 12);
    AT(dvz_gui_tree_get_expanded(tree, 0, NULL) == 4);

    const uint64_t duplicate[] = {1, 1};
    const uint32_t roots[] = {UINT32_MAX, UINT32_MAX};
    const char* duplicate_labels[] = {"a", "b"};
    AT(dvz_gui_tree_set_rows(tree, 2, duplicate, roots, duplicate_labels, NULL, 0) == DVZ_ERROR);
    AT(dvz_gui_tree_get_selection(tree, 1, &selected) == 1);
    const uint32_t invalid_preorder[] = {UINT32_MAX, 0, UINT32_MAX, 0};
    AT(dvz_gui_tree_set_rows(tree, 4, keys, invalid_preorder, labels, NULL, 0) == DVZ_ERROR);
    DvzGuiDataStyle style = dvz_gui_data_style();
    style.row_key = 12;
    style.flags = DVZ_GUI_DATA_STYLE_FLAGS_BACKGROUND;
    AT(dvz_gui_tree_set_styles(tree, 1, &style) == DVZ_OK);
    style.flags |= 1u << 20;
    AT(dvz_gui_tree_set_styles(tree, 1, &style) == DVZ_ERROR);
    AT(dvz_gui_tree_set_swatches(tree, 0, NULL) == DVZ_OK);
    dvz_gui_tree_destroy(tree);
    return 0;
}


/**
 * Verify transactional controlled projections and copied row annotations.
 *
 * @param suite test context
 * @param item test item
 * @return zero on success
 */
int test_gui_data_tree_controlled_model(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    const uint64_t keys[] = {10, 11, 12};
    const uint32_t parents[] = {UINT32_MAX, 0, 1};
    const char* labels[] = {"root", "branch", "écho écho"};
    DvzGuiTree* tree = dvz_gui_tree("controlled-model", DVZ_GUI_DATA_WIDGET_FLAGS_CONTROLLED);
    AT(tree != NULL);
    AT(dvz_gui_tree_set_rows(tree, 3, keys, parents, labels, NULL, 0) == DVZ_OK);
    DvzGuiDataDebugState state = {};
    AT(_dvz_gui_tree_debug_state(tree, &state));
    AT(state.display_count == 0);
    uint64_t projected[] = {12, 10};
    uint32_t depths[] = {0, 0};
    bool branches[] = {false, true}, selectable[] = {true, false};
    AT(dvz_gui_tree_set_projection(tree, 2, projected, depths, branches, selectable) == DVZ_OK);
    projected[0] = 999;
    depths[0] = 100;
    branches[0] = true;
    selectable[0] = false;
    AT(_dvz_gui_tree_debug_display_key(tree, 0) == 12);
    AT(dvz_gui_tree_set_projection(tree, 2, projected, depths, branches, selectable) == DVZ_ERROR);
    projected[0] = 10;
    AT(dvz_gui_tree_set_projection(tree, 2, projected, depths, branches, selectable) == DVZ_ERROR);
    AT(_dvz_gui_tree_debug_display_key(tree, 0) == 12);
    const bool hidden[] = {false, false, false};
    AT(dvz_gui_tree_set_visible(tree, 3, hidden) == DVZ_OK);
    AT(dvz_gui_tree_set_matches(tree, 3, hidden) == DVZ_OK);
    AT(dvz_gui_tree_set_filter(tree, "no match") == DVZ_OK);
    AT(dvz_gui_tree_collapse_all(tree) == DVZ_OK);
    AT(_dvz_gui_tree_debug_state(tree, &state));
    AT(state.display_count == 2);
    AT(dvz_gui_tree_reveal(tree, 12) == DVZ_OK);
    AT(dvz_gui_tree_set_focus(tree, 12) == DVZ_OK);
    AT(dvz_gui_tree_get_expanded(tree, 0, NULL) == 0);
    AT(dvz_gui_tree_reveal(tree, 11) == DVZ_ERROR);
    AT(dvz_gui_tree_set_focus(tree, 11) == DVZ_ERROR);
    char value[] = "missing", tooltip[] = "root > branch > leaf";
    DvzGuiTreeTextSpan spans[] = {{0, 5}, {6, 5}};
    DvzGuiTreeAnnotation annotation = dvz_gui_tree_annotation();
    annotation.row_key = 12;
    annotation.flags = DVZ_GUI_TREE_ANNOTATION_FLAGS_MISSING;
    annotation.value_text = value;
    annotation.tooltip = tooltip;
    annotation.label_span_count = 2;
    annotation.label_spans = spans;
    AT(dvz_gui_tree_set_annotations(tree, 1, &annotation) == DVZ_OK);
    value[0] = 'X';
    tooltip[0] = 'X';
    spans[0].start = 1;
    const DvzGuiTreeAnnotation* copied = _dvz_gui_tree_debug_annotation(tree, 12);
    AT(copied != NULL);
    AT(strcmp(copied->value_text, "missing") == 0);
    AT(strcmp(copied->tooltip, "root > branch > leaf") == 0);
    AT(copied->label_spans[0].start == 0);
    // A span starting inside é must reject the whole replacement.
    AT(dvz_gui_tree_set_annotations(tree, 1, &annotation) == DVZ_ERROR);
    AT(_dvz_gui_tree_debug_annotation(tree, 12) == copied);
    spans[0] = {0, 100};
    AT(dvz_gui_tree_set_annotations(tree, 1, &annotation) == DVZ_ERROR);
    annotation.row_key = 999;
    AT(dvz_gui_tree_set_annotations(tree, 1, &annotation) == DVZ_ERROR);
    AT(dvz_gui_tree_set_rows(tree, 3, keys, parents, labels, NULL, 0) == DVZ_OK);
    AT(_dvz_gui_tree_debug_state(tree, &state));
    AT(state.display_count == 0);
    AT(_dvz_gui_tree_debug_annotation(tree, 12) == NULL);
    AT(dvz_gui_tree_set_projection(tree, 0, NULL, NULL, NULL, NULL) == DVZ_OK);
    dvz_gui_tree_destroy(tree);
    DvzGuiTree* automatic = dvz_gui_tree("automatic", 0);
    AT(dvz_gui_tree_set_projection(automatic, 0, NULL, NULL, NULL, NULL) == DVZ_ERROR);
    AT(dvz_gui_tree_set_focus(automatic, 10) == DVZ_ERROR);
    dvz_gui_tree_destroy(automatic);
    DvzGuiTableColumnDesc column = dvz_gui_table_column_desc();
    column.title = "table";
    AT(dvz_gui_table("bad-controlled-table", 1, &column, DVZ_GUI_DATA_WIDGET_FLAGS_CONTROLLED) ==
       NULL);
    return 0;
}


/**
 * Draw one CPU-only ImGui frame and optionally retry a capacity rejection before input is
 * consumed.
 *
 * @param tree tree
 * @param events capacity-sized destination
 * @param reject_first whether to try one too-small destination first
 * @param origin output first row position
 * @return emitted count, or UINT32_MAX on a draw/capacity invariant failure
 */
static uint32_t
_gui_controlled_frame(DvzGuiTree* tree, DvzGuiDataEvent* events, bool reject_first, ImVec2* origin)
{
    igNewFrame();
    igSetNextWindowPos(ImVec2{0, 0}, ImGuiCond_Always, ImVec2{0, 0});
    igSetNextWindowSize(ImVec2{360, 180}, ImGuiCond_Always);
    igBegin("controlled CPU", NULL, ImGuiWindowFlags_NoSavedSettings);
    igGetCursorScreenPos(origin);
    uint32_t written = 99, dropped = 99;
    if (reject_first && (_dvz_gui_tree_debug_draw(
                             tree, events, DVZ_GUI_TREE_CONTROLLED_EVENT_CAPACITY - 1, &written,
                             &dropped) != DVZ_ERROR ||
                         written != 0 || dropped != 0))
        return UINT32_MAX;
    DvzResult result = _dvz_gui_tree_debug_draw(
        tree, events, DVZ_GUI_TREE_CONTROLLED_EVENT_CAPACITY, &written, &dropped);
    igEnd();
    igRender();
    return result == DVZ_OK && dropped == 0 ? written : UINT32_MAX;
}


/**
 * Find one emitted input intent.
 *
 * @param events events
 * @param n event count
 * @param type event type
 * @param key expected row key
 * @param detail expected event detail
 * @return whether the intent was emitted
 */
static bool _gui_controlled_event(
    const DvzGuiDataEvent* events, uint32_t n, uint32_t type, uint64_t key, int32_t detail)
{
    if (n > DVZ_GUI_TREE_CONTROLLED_EVENT_CAPACITY)
        return false;
    for (uint32_t i = 0; i < n; i++)
        if (events[i].type == type && events[i].row_key == key && events[i].detail == detail)
            return true;
    return false;
}


/**
 * Verify real ImGui click, disclosure, hover, focus, clipped reveal, and keyboard input intents.
 *
 * @param suite test context
 * @param item test item
 * @return zero on success
 */
int test_gui_data_tree_controlled_draw(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    ImGuiContext* context = igCreateContext(NULL);
    AT(context != NULL);
    ImGuiIO* io = igGetIO_Nil();
    io->DisplaySize = ImVec2{640, 480};
    io->DeltaTime = 1.0f / 60.0f;
    io->IniFilename = NULL;
    io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    AT(ImFontAtlas_Build(io->Fonts));
    const uint32_t count = 100;
    uint64_t keys[count];
    uint32_t parents[count], depths[count];
    bool branches[count], selectable[count];
    const char* labels[count];
    for (uint32_t i = 0; i < count; i++)
    {
        keys[i] = i + 10;
        parents[i] = i ? i - 1 : UINT32_MAX;
        labels[i] = "row";
        depths[i] = 0;
        branches[i] = i == 0;
        selectable[i] = i != 0;
    }
    selectable[2] = false;
    DvzGuiTree* tree = dvz_gui_tree("controlled-draw", DVZ_GUI_DATA_WIDGET_FLAGS_CONTROLLED);
    AT(tree != NULL);
    AT(dvz_gui_tree_set_rows(tree, count, keys, parents, labels, NULL, 0) == DVZ_OK);
    AT(dvz_gui_tree_set_projection(tree, count, keys, depths, branches, selectable) == DVZ_OK);
    AT(dvz_gui_tree_set_selection(tree, 1, &keys[2]) == DVZ_OK);
    DvzGuiTreeTextSpan highlight[] = {{0, 1}, {2, 1}};
    DvzGuiTreeAnnotation annotation = dvz_gui_tree_annotation();
    annotation.row_key = keys[0];
    annotation.value_text = "missing";
    annotation.tooltip = "host lineage";
    annotation.flags = DVZ_GUI_TREE_ANNOTATION_FLAGS_MISSING;
    annotation.label_span_count = 2;
    annotation.label_spans = highlight;
    AT(dvz_gui_tree_set_annotations(tree, 1, &annotation) == DVZ_OK);
    DvzGuiDataEvent events[DVZ_GUI_TREE_CONTROLLED_EVENT_CAPACITY] = {};
    ImVec2 origin = {};
    AT(_gui_controlled_frame(tree, events, false, &origin) != UINT32_MAX);
    const float row_step = io->FontDefault ? io->FontDefault->FontSize : 13.0f;
    // Draw another frame to settle the window's clipped region and overlapping hit targets.
    AT(_gui_controlled_frame(tree, events, false, &origin) != UINT32_MAX);
    const float step = row_step + 2 * igGetStyle()->FramePadding.y + igGetStyle()->ItemSpacing.y;
    ImGuiIO_AddMousePosEvent(io, origin.x + 80, origin.y + step + 5);
    AT(_gui_controlled_frame(tree, events, false, &origin) != UINT32_MAX);
    ImGuiIO_AddMouseButtonEvent(io, 0, true);
    uint32_t n = _gui_controlled_frame(tree, events, true, &origin);
    AT(_gui_controlled_event(events, n, DVZ_GUI_DATA_EVENT_ROW_CLICKED, keys[1], 0));
    uint64_t selected = 0;
    AT(dvz_gui_tree_get_selection(tree, 1, &selected) == 1);
    AT(selected == keys[2]);
    ImGuiIO_AddMouseButtonEvent(io, 0, false);
    AT(_gui_controlled_frame(tree, events, false, &origin) != UINT32_MAX);
    // A nonselectable leaf suppresses selection input but retains host-provided selection state.
    ImGuiIO_AddMousePosEvent(io, origin.x + 80, origin.y + 2 * step + 5);
    AT(_gui_controlled_frame(tree, events, false, &origin) != UINT32_MAX);
    ImGuiIO_AddMouseButtonEvent(io, 0, true);
    n = _gui_controlled_frame(tree, events, true, &origin);
    AT(!_gui_controlled_event(events, n, DVZ_GUI_DATA_EVENT_ROW_CLICKED, keys[2], 0));
    ImGuiIO_AddMouseButtonEvent(io, 0, false);
    AT(_gui_controlled_frame(tree, events, false, &origin) != UINT32_MAX);
    // A nonselectable branch body still delegates its click to the host.
    ImGuiIO_AddMousePosEvent(io, origin.x + 80, origin.y + 5);
    n = _gui_controlled_frame(tree, events, false, &origin);
    AT(_gui_controlled_event(events, n, DVZ_GUI_DATA_EVENT_HOVER_CHANGED, keys[2], 0));
    AT(_gui_controlled_event(events, n, DVZ_GUI_DATA_EVENT_HOVER_CHANGED, keys[0], 1));
    ImGuiIO_AddMouseButtonEvent(io, 0, true);
    n = _gui_controlled_frame(tree, events, true, &origin);
    AT(_gui_controlled_event(events, n, DVZ_GUI_DATA_EVENT_ROW_CLICKED, keys[0], 0));
    AT(dvz_gui_tree_get_selection(tree, 1, &selected) == 1 && selected == keys[2]);
    ImGuiIO_AddMouseButtonEvent(io, 0, false);
    AT(_gui_controlled_frame(tree, events, false, &origin) != UINT32_MAX);
    ImGuiIO_AddMousePosEvent(io, origin.x + 6, origin.y + 5);
    n = _gui_controlled_frame(tree, events, false, &origin);
    AT(n != UINT32_MAX);
    ImGuiIO_AddMouseButtonEvent(io, 0, true);
    n = _gui_controlled_frame(tree, events, true, &origin);
    AT(_gui_controlled_event(events, n, DVZ_GUI_DATA_EVENT_DISCLOSURE_CLICKED, keys[0], 1));
    AT(!_gui_controlled_event(events, n, DVZ_GUI_DATA_EVENT_ROW_CLICKED, keys[0], 0));
    AT(dvz_gui_tree_get_expanded(tree, 0, NULL) == 0);
    ImGuiIO_AddMouseButtonEvent(io, 0, false);
    ImGuiIO_AddMousePosEvent(io, -100, -100);
    AT(dvz_gui_tree_set_focus(tree, keys[count - 1]) == DVZ_OK);
    AT(_gui_controlled_frame(tree, events, false, &origin) != UINT32_MAX);
    n = _gui_controlled_frame(tree, events, false, &origin);
    AT(_gui_controlled_event(events, n, DVZ_GUI_DATA_EVENT_FOCUS_CHANGED, keys[count - 1], 1));
    ImGuiIO_AddKeyEvent(io, ImGuiMod_Shift, true);
    ImGuiIO_AddKeyEvent(io, ImGuiKey_UpArrow, true);
    n = _gui_controlled_frame(tree, events, true, &origin);
    AT(_gui_controlled_event(
        events, n, DVZ_GUI_DATA_EVENT_KEY_PRESSED, keys[count - 1], DVZ_KEY_UP));
    bool modifier_found = false;
    for (uint32_t i = 0; i < n; i++)
        if (events[i].type == DVZ_GUI_DATA_EVENT_KEY_PRESSED)
            modifier_found |= (events[i].flags & DVZ_KEY_MODIFIER_SHIFT) != 0;
    AT(modifier_found);
    AT(dvz_gui_tree_get_expanded(tree, 0, NULL) == 0);
    AT(dvz_gui_tree_get_selection(tree, 1, &selected) == 1 && selected == keys[2]);
    ImGuiIO_AddKeyEvent(io, ImGuiKey_UpArrow, false);
    ImGuiIO_AddKeyEvent(io, ImGuiMod_Shift, false);
    AT(_gui_controlled_frame(tree, events, false, &origin) != UINT32_MAX);
    const ImGuiKey input_keys[] = {ImGuiKey_UpArrow,    ImGuiKey_DownArrow, ImGuiKey_LeftArrow,
                                   ImGuiKey_RightArrow, ImGuiKey_Home,      ImGuiKey_End,
                                   ImGuiKey_PageUp,     ImGuiKey_PageDown,  ImGuiKey_Enter,
                                   ImGuiKey_Space,      ImGuiKey_Escape};
    const int32_t keycodes[] = {DVZ_KEY_UP,    DVZ_KEY_DOWN,  DVZ_KEY_LEFT,    DVZ_KEY_RIGHT,
                                DVZ_KEY_HOME,  DVZ_KEY_END,   DVZ_KEY_PAGE_UP, DVZ_KEY_PAGE_DOWN,
                                DVZ_KEY_ENTER, DVZ_KEY_SPACE, DVZ_KEY_ESCAPE};
    io->ConfigInputTrickleEventQueue = false;
    for (uint32_t i = 0; i < 11; i++)
        ImGuiIO_AddKeyEvent(io, input_keys[i], true);
    n = _gui_controlled_frame(tree, events, true, &origin);
    for (uint32_t i = 0; i < 11; i++)
        AT(_gui_controlled_event(
            events, n, DVZ_GUI_DATA_EVENT_KEY_PRESSED, keys[count - 1], keycodes[i]));
    for (uint32_t i = 0; i < 11; i++)
        ImGuiIO_AddKeyEvent(io, input_keys[i], false);
    AT(_gui_controlled_frame(tree, events, false, &origin) != UINT32_MAX);
    dvz_gui_tree_destroy(tree);
    // Automatic mode retains native selection and expansion changes.
    tree = dvz_gui_tree("automatic-draw", DVZ_GUI_DATA_WIDGET_FLAGS_MULTI_SELECT);
    AT(tree != NULL);
    AT(dvz_gui_tree_set_rows(tree, 3, keys, parents, labels, NULL, 0) == DVZ_OK);
    AT(dvz_gui_tree_expand_all(tree) == DVZ_OK);
    for (uint32_t i = 0; i < 3; i++)
        AT(_gui_controlled_frame(tree, events, false, &origin) != UINT32_MAX);
    ImGuiIO_AddMousePosEvent(io, origin.x + 80, origin.y + step + 5);
    AT(_gui_controlled_frame(tree, events, false, &origin) != UINT32_MAX);
    ImGuiIO_AddMouseButtonEvent(io, 0, true);
    n = _gui_controlled_frame(tree, events, false, &origin);
    AT(_gui_controlled_event(events, n, DVZ_GUI_DATA_EVENT_SELECTION_CHANGED, keys[1], 1));
    AT(dvz_gui_tree_get_selection(tree, 1, &selected) == 1 && selected == keys[1]);
    ImGuiIO_AddMouseButtonEvent(io, 0, false);
    AT(_gui_controlled_frame(tree, events, false, &origin) != UINT32_MAX);
    ImGuiIO_AddMousePosEvent(io, origin.x + 6, origin.y + 5);
    AT(_gui_controlled_frame(tree, events, false, &origin) != UINT32_MAX);
    ImGuiIO_AddMouseButtonEvent(io, 0, true);
    n = _gui_controlled_frame(tree, events, false, &origin);
    AT(_gui_controlled_event(events, n, DVZ_GUI_DATA_EVENT_EXPANSION_CHANGED, keys[0], 0));
    AT(dvz_gui_tree_get_expanded(tree, 0, NULL) == 2);
    dvz_gui_tree_destroy(tree);
    igDestroyContext(context);
    return 0;
}


/**
 * Verify typed table columns, combined filtering, and deterministic sorting without a GPU.
 */
int test_gui_data_table_model(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);
    DvzGuiTableColumnDesc columns[3] = {};
    for (uint32_t i = 0; i < 3; i++)
        columns[i] = dvz_gui_table_column_desc();
    columns[0].column_id = 7;
    columns[0].type = DVZ_GUI_TABLE_COLUMN_TEXT;
    columns[0].title = "Name";
    columns[0].flags = DVZ_GUI_TABLE_COLUMN_FLAGS_SEARCHABLE | DVZ_GUI_TABLE_COLUMN_FLAGS_SORTABLE;
    columns[1].column_id = 8;
    columns[1].type = DVZ_GUI_TABLE_COLUMN_DOUBLE;
    columns[1].title = "Value";
    columns[1].format = "%.2f";
    columns[1].flags = DVZ_GUI_TABLE_COLUMN_FLAGS_SORTABLE;
    columns[2].column_id = 9;
    columns[2].type = DVZ_GUI_TABLE_COLUMN_COLOR;
    columns[2].title = "Color";
    DvzGuiTable* table = dvz_gui_table("table-model", 3, columns, 0);
    AT(table != NULL);
    const uint64_t keys[] = {3, 1, 4, 2};
    const char* names[] = {"beta", "Alpha", "alpha", "gamma"};
    const double values[] = {INFINITY, NAN, -INFINITY, 4.0};
    const DvzColor colors[] = {{1, 2, 3, 4}, {2, 3, 4, 5}, {3, 4, 5, 6}, {4, 5, 6, 7}};
    AT(dvz_gui_table_set_rows(table, 4, keys, 0) == DVZ_OK);
    AT(dvz_gui_table_set_column_text(table, 7, 4, names) == DVZ_OK);
    AT(dvz_gui_table_set_column_double(table, 8, 4, values) == DVZ_OK);
    AT(dvz_gui_table_set_column_color(table, 9, 4, colors) == DVZ_OK);
    DvzGuiDataDebugState state = {};
    AT(_dvz_gui_table_debug_state(table, &state));
    AT(state.display_count == 4);
    uint64_t rebuild = state.rebuild_count;
    AT(_dvz_gui_table_debug_state(table, &state));
    AT(state.rebuild_count == rebuild);
    AT(dvz_gui_table_set_filter(table, "ALPHA") == DVZ_OK);
    AT(_dvz_gui_table_debug_state(table, &state));
    AT(state.display_count == 2);
    const bool matches[] = {true, true, false, true};
    AT(dvz_gui_table_set_matches(table, 4, matches) == DVZ_OK);
    AT(_dvz_gui_table_debug_state(table, &state));
    AT(state.display_count == 1);
    AT(_dvz_gui_table_debug_display_key(table, 0) == 1);
    AT(dvz_gui_table_set_matches(table, 0, NULL) == DVZ_OK);
    AT(dvz_gui_table_set_filter(table, "") == DVZ_OK);
    AT(_dvz_gui_table_debug_sort(table, 8, 1) == DVZ_OK);
    AT(_dvz_gui_table_debug_display_key(table, 0) == 4);
    AT(_dvz_gui_table_debug_display_key(table, 3) == 1);
    AT(_dvz_gui_table_debug_sort(table, 8, -1) == DVZ_OK);
    AT(_dvz_gui_table_debug_display_key(table, 0) == 3);
    AT(_dvz_gui_table_debug_display_key(table, 3) == 1);
    AT(_dvz_gui_table_debug_sort(table, 7, 1) == DVZ_OK);
    AT(_dvz_gui_table_debug_display_key(table, 0) == 1);
    AT(_dvz_gui_table_debug_display_key(table, 1) == 4);

    const char invalid_utf8[] = {(char)0xc0, (char)0x80, 0};
    const char* invalid_names[] = {"ok", invalid_utf8, "ok", "ok"};
    AT(dvz_gui_table_set_column_text(table, 7, 4, invalid_names) == DVZ_ERROR);
    AT(_dvz_gui_table_debug_display_key(table, 0) == 1);
    AT(dvz_gui_table_set_column_double(table, 7, 4, values) == DVZ_ERROR);
    DvzGuiTableColumnDesc invalid = dvz_gui_table_column_desc();
    invalid.column_id = 1;
    invalid.type = DVZ_GUI_TABLE_COLUMN_DOUBLE;
    invalid.title = "invalid";
    invalid.format = "%s";
    AT(dvz_gui_table("invalid", 1, &invalid, 0) == NULL);
    invalid.type = DVZ_GUI_TABLE_COLUMN_COLOR;
    invalid.format = NULL;
    invalid.flags = DVZ_GUI_TABLE_COLUMN_FLAGS_SORTABLE;
    AT(dvz_gui_table("invalid", 1, &invalid, 0) == NULL);
    dvz_gui_table_destroy(table);
    return 0;
}



/**
 * Create selected-GPU resources for a GLFW presentation app smoke.
 *
 * @param suite test context
 * @param[out] resources borrowed app resources to initialize
 * @return NULL on success, otherwise a skip reason
 */
static const char* _gui_test_gpu_resources_create(TstContext* suite, GuiTestGpuResources* resources)
{
    ANN(suite);
    ANN(resources);
    *resources = {};
    resources->window_host = dvz_window_host();
    if (resources->window_host == NULL)
        return "window host creation failed";

    DvzGpuCtxConfig config = dvz_testing_gpu_ctx_config(suite);
    if (dvz_canvas_configure_gpu_ctx(
            resources->window_host, DVZ_BACKEND_GLFW, DVZ_CANVAS_RENDER_MODE_PRESENT, &config) !=
        DVZ_OK)
    {
        dvz_window_host_destroy(resources->window_host);
        resources->window_host = NULL;
        return "GLFW GPU context configuration failed";
    }
    resources->gpu_ctx = dvz_gpu_ctx(&config);
    if (resources->gpu_ctx == NULL)
    {
        dvz_window_host_destroy(resources->window_host);
        resources->window_host = NULL;
        return "GPU context creation failed";
    }
    return NULL;
}



/**
 * Destroy selected-GPU resources after every borrowing app has been destroyed.
 *
 * @param resources resources to destroy
 */
static void _gui_test_gpu_resources_destroy(GuiTestGpuResources* resources)
{
    ANN(resources);
    if (resources->gpu_ctx != NULL)
        dvz_gpu_ctx_destroy(resources->gpu_ctx);
    if (resources->window_host != NULL)
        dvz_window_host_destroy(resources->window_host);
    *resources = {};
}



/**
 * Create an app borrowing the selected GPU context and GLFW window host.
 *
 * @param scene app scene
 * @param config optional app configuration
 * @param resources selected GPU resources
 * @return created app or NULL
 */
static DvzApp* _gui_test_app(
    DvzScene* scene, const DvzAppConfig* config, const GuiTestGpuResources* resources)
{
    ANN(scene);
    ANN(resources);
    DvzAppResources app_resources = dvz_app_resources();
    app_resources.gpu_ctx = resources->gpu_ctx;
    app_resources.window_host = resources->window_host;
    return dvz_app_with_resources(scene, config, &app_resources);
}



/**
 * Create a minimal figure with a full-size panel.
 *
 * @param scene scene that owns the figure
 * @param width figure width
 * @param height figure height
 * @return created figure, or NULL on failure
 */
static DvzFigure* _gui_test_figure(DvzScene* scene, uint32_t width, uint32_t height)
{
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, width, height, 0);
    if (figure == NULL)
        return NULL;

    DvzPanelDesc full = {0.0f, 0.0f, 1.0f, 1.0f};
    DvzPanel* panel = dvz_panel(figure, &full);
    if (panel == NULL)
        return NULL;
    dvz_panel_set_background_color(panel, dvz_color_from_unit(0.04f, 0.05f, 0.07f, 1.0f));
    return figure;
}



/**
 * Build a viewport window with changing size and collapsed state.
 *
 * @param gui GUI overlay
 * @param win view
 * @param user_data GUI viewport smoke state
 */
static void _gui_viewport_resize_callback(DvzGui* gui, DvzView* win, void* user_data)
{
    (void)gui;
    (void)win;
    GuiViewportSmoke* smoke = (GuiViewportSmoke*)user_data;
    ANN(smoke);

    if (igBegin("Raw cimgui smoke", NULL, 0))
    {
        igTextUnformatted("ig* raw calls are available", NULL);
        (void)igButton("raw button", ImVec2{0, 0});
    }
    igEnd();

    if (smoke->frame == 0)
    {
        igSetNextWindowSize(ImVec2{360, 260}, ImGuiCond_Always);
        igSetNextWindowCollapsed(false, ImGuiCond_Always);
    }
    else if (smoke->frame == 1)
    {
        igSetNextWindowSize(ImVec2{260, 220}, ImGuiCond_Always);
        igSetNextWindowCollapsed(false, ImGuiCond_Always);
    }
    else if (smoke->frame == 2)
    {
        igSetNextWindowSize(ImVec2{260, 220}, ImGuiCond_Always);
        igSetNextWindowCollapsed(false, ImGuiCond_Always);
    }
    else
    {
        igSetNextWindowCollapsed(true, ImGuiCond_Always);
    }

    bool expect_visible = smoke->frame <= 2;
    bool shown = dvz_gui_viewport_window(
        smoke->viewport, "GUI viewport smoke", NULL, ImGuiWindowFlags_NoSavedSettings);
    if (shown)
        smoke->shown_count++;
    else
        smoke->hidden_count++;

    DvzGuiViewportDebugState debug = {};
    if (_dvz_gui_viewport_debug_state(smoke->viewport, &debug) && debug.display_drawable &&
        !debug.display_ready)
    {
        smoke->transition_drawable_count++;
        if (expect_visible)
        {
            smoke->visible_transition_count++;
            if (shown)
                smoke->visible_transition_shown_count++;
        }
    }
    smoke->frame++;
}



/**
 * Record a pointer event received by a viewport input router.
 *
 * @param router input router
 * @param event pointer event
 * @param user_data input recorder
 */
static void _gui_record_pointer(
    DvzInputRouter* router, const DvzPointerEvent* event, void* user_data)
{
    ANN(router);
    ANN(event);
    GuiInputRecorder* recorder = (GuiInputRecorder*)user_data;
    ANN(recorder);
    recorder->count++;
    recorder->last_type = event->type;
}


static void _gui_record_input_event(
    DvzInputRouter* router, const DvzInputEvent* event, void* user_data)
{
    ANN(router);
    ANN(event);
    GuiInputRecorder* recorder = (GuiInputRecorder*)user_data;
    ANN(recorder);
    if (event->type == DVZ_INPUT_EVENT_POINTER)
    {
        DvzPointerEventType type = event->content.pointer.type;
        if (type >= DVZ_POINTER_EVENT_RELEASE && type <= DVZ_POINTER_EVENT_WHEEL)
            recorder->union_types[type]++;
        if (type == DVZ_POINTER_EVENT_DRAG)
            recorder->drag_count++;
    }
}



/**
 * Emit one pointer event directly through a GUI viewport input router.
 *
 * @param router input router
 * @param type pointer event type
 */
static void _gui_emit_pointer(DvzInputRouter* router, DvzPointerEventType type)
{
    ANN(router);
    dvz_pointer_emit_position(
        router, type, 10.0f, 20.0f, 100.0f, 80.0f, DVZ_POINTER_BUTTON_NONE,
        DVZ_KEY_MODIFIER_SHIFT, 1.0f, dvz_input_timestamp_ns(), NULL);
}



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

/**
 * Check GUI viewport default configuration values.
 *
 * @param suite test suite
 * @param item test item
 * @return 0 on success
 */
static int test_gui_viewport_config_defaults(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzGuiViewportConfig config = dvz_gui_viewport_config();
    AT(config.struct_size == DVZ_STRUCT_SIZE(DvzGuiViewportConfig));
    AT(config.flags == 0);
    AT((config.viewport_flags & DVZ_GUI_VIEWPORT_FLAGS_FORWARD_INPUT) != 0);
    AT((config.viewport_flags & DVZ_GUI_VIEWPORT_FLAGS_RENDER_WHEN_HIDDEN) == 0);
    AT(config.initial_width == 640);
    AT(config.initial_height == 480);
    AT(config.min_width > 0);
    AT(config.min_height > 0);
    AT(config.resize_step > 0);
    AT(config.resize_delay_frames > 0);
    return 0;
}


/**
 * Check GUI overlay default font policy.
 *
 * @param suite test suite
 * @param item test item
 * @return 0 on success
 */
static int test_gui_config_font_defaults(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzGuiConfig config = dvz_gui_config();
    AT(config.struct_size == DVZ_STRUCT_SIZE(DvzGuiConfig));
    AT(config.flags == 0);
    AT((config.gui_flags & DVZ_GUI_FLAGS_DOCKING) != 0);
    AT((config.gui_flags & DVZ_GUI_FLAGS_DOCKSPACE) != 0);
    AT(config.default_window_width == 200);
    AT(config.ini_path == NULL);
    return 0;
}



/**
 * Check the bridge between Datoviz, backend, and ImGui coordinate scales.
 *
 * @param suite test suite
 * @param item test item
 * @return 0 on success
 */
static int test_gui_scale_resolution(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzGuiScaleDebugState scale = {};
    DvzScaleXY device = {};
    device.x = device.y = 2.0f;
    DvzExtent native = {};
    native.width = 1600;
    native.height = 900;
    DvzExtent framebuffer = native;
    AT(_dvz_gui_scale_resolve(device, native, framebuffer, 1.5f, &scale));
    AC(scale.framebuffer.x, 1.0f, 1e-6f);
    AC(scale.coordinate.x, 2.0f, 1e-6f);
    AC(scale.coordinate.y, 2.0f, 1e-6f);
    AC(scale.user, 1.5f, 1e-6f);

    framebuffer.width = 3200;
    framebuffer.height = 1800;
    AT(_dvz_gui_scale_resolve(device, native, framebuffer, 1.0f, &scale));
    AC(scale.framebuffer.x, 2.0f, 1e-6f);
    AC(scale.coordinate.x, 1.0f, 1e-6f);
    AC(scale.coordinate.y, 1.0f, 1e-6f);

    device.x = 2.0f;
    device.y = 1.5f;
    framebuffer.width = 1600;
    framebuffer.height = 1800;
    AT(_dvz_gui_scale_resolve(device, native, framebuffer, 0.0f, &scale));
    AC(scale.coordinate.x, 2.0f, 1e-6f);
    AC(scale.coordinate.y, 0.75f, 1e-6f);
    AC(scale.scalar_coordinate, 1.375f, 1e-6f);
    AC(scale.user, 1.0f, 1e-6f);
    return 0;
}



/**
 * Check that repeated and fractional style scaling starts from canonical metrics.
 *
 * @param suite test suite
 * @param item test item
 * @return 0 on success
 */
static int test_gui_style_scale(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    ImGuiContext* context = igCreateContext(NULL);
    AT(context != NULL);
    _dvz_gui_style_scale(0.25f);
    ImGuiStyle* style = igGetStyle();
    AT(style->WindowBorderHoverPadding > 0.0f);

    _dvz_gui_style_scale(2.0f);
    const float padding_a = style->WindowPadding.x;
    _dvz_gui_style_scale(2.0f);
    AC(style->WindowPadding.x, padding_a, 1e-6f);
    AT(style->WindowBorderHoverPadding > 0.0f);
    igDestroyContext(context);
    return 0;
}



typedef struct GuiCompactLayoutTest
{
    float scale;
    bool passed;
    uint32_t frame;
} GuiCompactLayoutTest;


/**
 * Check logical geometry, width sentinels, ordinary placement, and balanced nested scopes.
 *
 * @param gui CPU-only GUI wrapper
 * @param view unused view
 * @param user_data test state
 */
static void _gui_compact_layout_callback(DvzGui* gui, DvzView* view, void* user_data)
{
    (void)view;
#define GUI_LAYOUT_CHECK(condition)                                                               \
    do                                                                                            \
    {                                                                                             \
        if (!(condition))                                                                         \
        {                                                                                         \
            log_error("compact layout check failed at line %d", __LINE__);                        \
            state->passed = false;                                                                \
        }                                                                                         \
    } while (0)
    GuiCompactLayoutTest* state = (GuiCompactLayoutTest*)user_data;
    const float scale = state->scale;
    const int flags = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking;
    DvzRect requested = {50, 60, 320, 240}, actual = {};
    uint64_t frame = 0;
    GUI_LAYOUT_CHECK(!_dvz_gui_data_draw_context(gui, &frame));
    GUI_LAYOUT_CHECK(dvz_gui_set_next_window_rect(gui, &requested, 0) == DVZ_OK);
    dvz_gui_begin(gui, "logical layout", NULL, flags);
    GUI_LAYOUT_CHECK(dvz_gui_current_window_rect(gui, &actual));
    GUI_LAYOUT_CHECK(fabsf(actual.x - requested.x) < 1 / scale);
    GUI_LAYOUT_CHECK(fabsf(actual.y - requested.y) < 1 / scale);
    GUI_LAYOUT_CHECK(fabsf(actual.width - requested.width) < 1 / scale);
    GUI_LAYOUT_CHECK(fabsf(actual.height - requested.height) < 1 / scale);
    float available[2] = {};
    ImVec2 native_available = {};
    igGetContentRegionAvail(&native_available);
    GUI_LAYOUT_CHECK(dvz_gui_content_avail(gui, available));
    GUI_LAYOUT_CHECK(fabsf(available[0] * scale - native_available.x) < 0.001f);
    GUI_LAYOUT_CHECK(fabsf(available[1] * scale - native_available.y) < 0.001f);
    char text[32] = "";
    dvz_gui_set_next_item_width(gui, 80);
    GUI_LAYOUT_CHECK(!dvz_gui_input_text_with_hint(gui, "##hint", "Search", text, sizeof(text)));
    ImVec2 item_size = {};
    igGetItemRectSize(&item_size);
    GUI_LAYOUT_CHECK(fabsf(item_size.x - 80 * scale) < 0.001f);
    igGetContentRegionAvail(&native_available);
    dvz_gui_set_next_item_width(gui, -7);
    dvz_gui_input_text_with_hint(gui, "##right", "Search", text, sizeof(text));
    igGetItemRectSize(&item_size);
    GUI_LAYOUT_CHECK(fabsf(item_size.x - (native_available.x - 7 * scale)) < 1.0f);
    dvz_gui_set_next_item_width(gui, 0);
    const float default_width = igCalcItemWidth();
    dvz_gui_input_text_with_hint(gui, "##default", "Search", text, sizeof(text));
    igGetItemRectSize(&item_size);
    GUI_LAYOUT_CHECK(fabsf(item_size.x - default_width) < 0.001f);
    GUI_LAYOUT_CHECK(!dvz_gui_input_text_with_hint(gui, "##zero", "Search", text, 0));
    dvz_gui_begin_child(gui, "remaining", 0, 25, 0);
    igGetContentRegionAvail(&native_available);
    GUI_LAYOUT_CHECK(dvz_gui_content_avail(gui, available));
    GUI_LAYOUT_CHECK(fabsf(available[0] * scale - native_available.x) < 0.001f);
    dvz_gui_end_child(gui);
    igSetNextWindowCollapsed(true, ImGuiCond_Always);
    const bool nested_visible = dvz_gui_begin(gui, "collapsed nested", NULL, flags);
    GUI_LAYOUT_CHECK(_dvz_gui_data_draw_context(gui, &frame) == nested_visible);
    if (state->frame > 0)
        GUI_LAYOUT_CHECK(!nested_visible);
    dvz_gui_end(gui);
    GUI_LAYOUT_CHECK(_dvz_gui_data_draw_context(gui, &frame));
    dvz_gui_end(gui);
    GUI_LAYOUT_CHECK(!_dvz_gui_data_draw_context(gui, &frame));

    dvz_gui_begin(gui, "default width", NULL, flags);
    dvz_gui_current_window_rect(gui, &actual);
    GUI_LAYOUT_CHECK(fabsf(actual.width - 200) < 1.0f);
    dvz_gui_end(gui);
    igSetNextWindowSize(ImVec2{300 * scale, 100 * scale}, ImGuiCond_Always);
    dvz_gui_begin(gui, "raw explicit width", NULL, flags);
    dvz_gui_current_window_rect(gui, &actual);
    GUI_LAYOUT_CHECK(fabsf(actual.width - 300) < 1.0f);
    dvz_gui_end(gui);

    requested = {700, -20, 900, 600};
    GUI_LAYOUT_CHECK(dvz_gui_set_next_window_rect(gui, &requested, 0) == DVZ_OK);
    dvz_gui_begin(gui, "bounded tool", NULL, flags);
    dvz_gui_current_window_rect(gui, &actual);
    GUI_LAYOUT_CHECK(fabsf(actual.x) < 0.001f && fabsf(actual.y) < 0.001f);
    GUI_LAYOUT_CHECK(fabsf(actual.width - 720) < 0.001f);
    GUI_LAYOUT_CHECK(fabsf(actual.height - 480) < 0.001f);
    dvz_gui_end(gui);

    requested = {10, 15, state->frame == 0 ? 250.0f : 400.0f, 150};
    GUI_LAYOUT_CHECK(dvz_gui_set_next_window_rect(gui, &requested, ImGuiCond_Once) == DVZ_OK);
    dvz_gui_begin(gui, "once placement", NULL, flags);
    dvz_gui_current_window_rect(gui, &actual);
    GUI_LAYOUT_CHECK(fabsf(actual.width - 250) < 0.001f);
    dvz_gui_end(gui);
    GUI_LAYOUT_CHECK(dvz_gui_set_next_window_rect(gui, &requested, 3) == DVZ_ERROR);
    requested.width = NAN;
    GUI_LAYOUT_CHECK(dvz_gui_set_next_window_rect(gui, &requested, 0) == DVZ_ERROR);
    GUI_LAYOUT_CHECK(dvz_gui_set_next_window_rect(gui, NULL, 0) == DVZ_ERROR);
    GUI_LAYOUT_CHECK(!dvz_gui_content_avail(gui, NULL));
#undef GUI_LAYOUT_CHECK
}


/**
 * Verify curated geometry at coordinate scales one/two and user scales one/one-and-a-half.
 *
 * @param suite test context
 * @param item test item
 * @return zero on success
 */
static int test_gui_compact_layout(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;
    for (uint32_t i = 0; i < 4; i++)
    {
        ImGuiContext* context = igCreateContext(NULL);
        AT(context != NULL);
        const float scale = i < 2 ? 1.0f : 2.0f;
        const float user_scale = i % 2 ? 1.5f : 1.0f;
        ImGuiIO* io = igGetIO_Nil();
        io->DisplaySize = ImVec2{720 * scale, 480 * scale};
        io->DeltaTime = 1.0f / 60.0f;
        io->IniFilename = NULL;
        io->FontGlobalScale = scale * user_scale;
        _dvz_gui_style_scale(scale * user_scale);
        AT(ImFontAtlas_Build(io->Fonts));
        GuiCompactLayoutTest state = {scale, true, 0};
        for (state.frame = 0; state.frame < 3; state.frame++)
        {
            igNewFrame();
            AT(_dvz_gui_debug_callback(_gui_compact_layout_callback, &state, scale, 200));
            igRender();
            AT(state.passed);
        }
        igDestroyContext(context);
    }
    return 0;
}



/**
 * Count vertices using one exact color in newly emitted GUI geometry.
 *
 * @param draw native draw list
 * @param start first vertex
 * @param color expected packed color
 * @return matching vertex count
 */
static uint32_t _gui_vertices_with_color(const ImDrawList* draw, int start, ImU32 color)
{
    uint32_t count = 0;
    for (int i = start; i < draw->VtxBuffer.Size; i++)
        count += draw->VtxBuffer.Data[i].col == color;
    return count;
}


typedef struct GuiIconTest
{
    float scale;
    uint32_t flags;
    bool pressed;
    bool focused;
    bool tooltip;
    ImGuiID id;
    ImVec2 center;
    ImVec2 disabled_center;
    bool indicator;
    bool normal_background;
    bool hovered_background;
    bool held_background;
    bool passed;
} GuiIconTest;


/**
 * Draw icon controls with stable IDs and collect native geometry and input behavior.
 *
 * @param gui CPU-only GUI wrapper
 * @param view unused view
 * @param user_data test state
 */
static void _gui_icon_callback(DvzGui* gui, DvzView* view, void* user_data)
{
    (void)view;
#define GUI_ICON_CHECK(condition)                                                                 \
    do                                                                                            \
    {                                                                                             \
        if (!(condition))                                                                         \
        {                                                                                         \
            log_error("compact icon check failed at line %d", __LINE__);                          \
            state->passed = false;                                                                \
        }                                                                                         \
    } while (0)
    GuiIconTest* state = (GuiIconTest*)user_data;
    const float scale = state->scale;
    DvzRect rect = {20, 30, 650, 300};
    GUI_ICON_CHECK(dvz_gui_set_next_window_rect(gui, &rect, 0) == DVZ_OK);
    dvz_gui_begin(gui, "icon CPU", NULL, ImGuiWindowFlags_NoSavedSettings);
    const ImDrawList* draw = igGetWindowDrawList();
    const int first_vertex = draw->VtxBuffer.Size;
    state->pressed = dvz_gui_icon_button(
        gui, "primary", DVZ_GUI_ICON_CLEAR, 28, "Clear selection", state->flags);
    const bool disabled = (state->flags & DVZ_GUI_ICON_BUTTON_FLAGS_DISABLED) != 0;
    const float alpha = disabled ? igGetStyle()->DisabledAlpha : 1.0f;
    state->normal_background = _gui_vertices_with_color(
        draw, first_vertex, igGetColorU32_Col(ImGuiCol_Button, alpha)) > 0;
    state->hovered_background = _gui_vertices_with_color(
        draw, first_vertex, igGetColorU32_Col(ImGuiCol_ButtonHovered, alpha)) > 0;
    state->held_background = _gui_vertices_with_color(
        draw, first_vertex, igGetColorU32_Col(ImGuiCol_ButtonActive, alpha)) > 0;
    const ImGuiID id = igGetItemID();
    GUI_ICON_CHECK(state->id == 0 || id == state->id);
    state->id = id;
    state->focused = igIsItemFocused();
    ImVec2 size = {}, a = {}, b = {};
    igGetItemRectSize(&size);
    GUI_ICON_CHECK(fabsf(size.x - 28 * scale) < 0.001f);
    GUI_ICON_CHECK(fabsf(size.y - 28 * scale) < 0.001f);
    igGetItemRectMin(&a);
    igGetItemRectMax(&b);
    // The persistent selection mark sits below the glyph, inside the native frame.
    state->indicator = false;
    const ImU32 text_color = igGetColorU32_Col(ImGuiCol_Text, alpha);
    for (int vertex = first_vertex; vertex < draw->VtxBuffer.Size; vertex++)
    {
        const ImDrawVert* v = &draw->VtxBuffer.Data[vertex];
        state->indicator |= v->col == text_color && v->pos.y > b.y - size.y * 0.2f;
    }
    state->center = ImVec2{(a.x + b.x) / 2, (a.y + b.y) / 2};
    const float primary_right = b.x;
    dvz_gui_same_line(gui, 0, 7);
    const bool disabled_pressed = dvz_gui_icon_button(
        gui, "disabled", DVZ_GUI_ICON_SETTINGS, 28, "Disabled settings",
        DVZ_GUI_ICON_BUTTON_FLAGS_DISABLED);
    GUI_ICON_CHECK(!disabled_pressed && !igIsItemFocused() && id != igGetItemID());
    igGetItemRectMin(&a);
    igGetItemRectMax(&b);
    GUI_ICON_CHECK(fabsf(a.x - primary_right - 7 * scale) < 0.001f);
    state->disabled_center = ImVec2{(a.x + b.x) / 2, (a.y + b.y) / 2};
    ImVec2 window_pos = {};
    igGetWindowPos(&window_pos);
    dvz_gui_same_line(gui, 150, 0);
    dvz_gui_icon_button(gui, "font sized", DVZ_GUI_ICON_INFO, 0, NULL, 0);
    igGetItemRectMin(&a);
    igGetItemRectSize(&size);
    GUI_ICON_CHECK(fabsf(a.x - window_pos.x - 150 * scale) < 0.001f);
    GUI_ICON_CHECK(fabsf(size.x - igGetFrameHeight()) < 0.001f);
    GUI_ICON_CHECK(fabsf(size.y - igGetFrameHeight()) < 0.001f);
    dvz_gui_same_line(gui, 0, -1);
    dvz_gui_icon_button(gui, "native gap", DVZ_GUI_ICON_EXPAND, 28, NULL, 0);
    igGetItemRectMin(&b);
    GUI_ICON_CHECK(fabsf(b.x - a.x - size.x - igGetStyle()->ItemSpacing.x) < 0.001f);
    // Every vector glyph adds geometry through the existing draw list.
    for (int icon = DVZ_GUI_ICON_CLEAR; icon <= DVZ_GUI_ICON_SORT; icon++)
    {
        char identifier[32] = {};
        snprintf(identifier, sizeof(identifier), "glyph %d", icon);
        const int vertices = igGetWindowDrawList()->VtxBuffer.Size;
        dvz_gui_icon_button(gui, identifier, (DvzGuiIcon)icon, 18, NULL, 0);
        GUI_ICON_CHECK(igGetWindowDrawList()->VtxBuffer.Size > vertices);
        dvz_gui_same_line(gui, 0, 2);
    }
    GUI_ICON_CHECK(!dvz_gui_icon_button(gui, "", DVZ_GUI_ICON_CLEAR, 28, NULL, 0));
    GUI_ICON_CHECK(!dvz_gui_icon_button(gui, "bad", (DvzGuiIcon)99, 28, NULL, 0));
    GUI_ICON_CHECK(!dvz_gui_icon_button(gui, "bad", DVZ_GUI_ICON_CLEAR, -1, NULL, 0));
    GUI_ICON_CHECK(!dvz_gui_icon_button(gui, "bad", DVZ_GUI_ICON_CLEAR, NAN, NULL, 0));
    GUI_ICON_CHECK(!dvz_gui_icon_button(gui, "bad", DVZ_GUI_ICON_CLEAR, 28, NULL, 4));
    dvz_gui_end(gui);
    const ImGuiWindow* tooltip = igFindWindowByName("##Tooltip_00");
    state->tooltip = tooltip != NULL && tooltip->Active;
#undef GUI_ICON_CHECK
}


/**
 * Advance one CPU icon frame.
 *
 * @param state borrowed frame state
 * @return whether ordinary GUI scopes balanced
 */
static bool _gui_icon_frame(GuiIconTest* state)
{
    igNewFrame();
    const bool balanced = _dvz_gui_debug_callback(_gui_icon_callback, state, state->scale, 200);
    igRender();
    return balanced;
}


/**
 * Verify vector icons, logical row gaps, stable IDs, mouse/keyboard input, and disabled help.
 *
 * @param suite test context
 * @param item test item
 * @return zero on success
 */
static int test_gui_compact_icons(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;
    for (uint32_t i = 0; i < 4; i++)
    {
        ImGuiContext* context = igCreateContext(NULL);
        AT(context != NULL);
        const float scale = i < 2 ? 1.0f : 2.0f;
        const float user = i % 2 ? 1.5f : 1.0f;
        ImGuiIO* io = igGetIO_Nil();
        io->DisplaySize = ImVec2{720 * scale, 480 * scale};
        io->DeltaTime = 1.0f / 60.0f;
        io->IniFilename = NULL;
        io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io->FontGlobalScale = scale * user;
        _dvz_gui_style_scale(scale * user);
        AT(ImFontAtlas_Build(io->Fonts));
        GuiIconTest state = {};
        state.scale = scale;
        state.passed = true;
        ImGuiIO_AddMousePosEvent(io, -100, -100);
        AT(_gui_icon_frame(&state));
        AT(_gui_icon_frame(&state));
        // Initial native focus alone must not display help before navigation starts.
        AT(state.passed && !state.pressed && !state.tooltip);
        AT(state.normal_background && !state.indicator);
        state.flags = DVZ_GUI_ICON_BUTTON_FLAGS_ACTIVE;
        AT(_gui_icon_frame(&state));
        AT(state.normal_background && state.indicator);
        ImGuiIO_AddMousePosEvent(io, state.center.x, state.center.y);
        AT(_gui_icon_frame(&state));
        AT(state.tooltip);
        AT(state.hovered_background && state.indicator && !state.held_background);
        ImGuiIO_AddMouseButtonEvent(io, 0, true);
        AT(_gui_icon_frame(&state));
        AT(state.held_background && state.indicator && !state.hovered_background);
        ImGuiIO_AddMouseButtonEvent(io, 0, false);
        AT(_gui_icon_frame(&state));
        AT(state.pressed);
        // Mouse focus remains after the click; help must disappear when the pointer leaves.
        ImGuiIO_AddMousePosEvent(io, -100, -100);
        AT(_gui_icon_frame(&state));
        AT(!state.tooltip);
        ImGuiIO_AddMousePosEvent(io, state.disabled_center.x, state.disabled_center.y);
        AT(_gui_icon_frame(&state));
        AT(state.tooltip);
        ImGuiIO_AddMouseButtonEvent(io, 0, true);
        AT(_gui_icon_frame(&state));
        ImGuiIO_AddMouseButtonEvent(io, 0, false);
        AT(_gui_icon_frame(&state));
        AT(!state.pressed);
        ImGuiIO_AddMousePosEvent(io, -100, -100);
        // Tab enters native navigation from mouse mode and skips the disabled button.
        for (uint32_t step = 0; step < 16; step++)
        {
            ImGuiIO_AddKeyEvent(io, ImGuiKey_Tab, true);
            AT(_gui_icon_frame(&state));
            ImGuiIO_AddKeyEvent(io, ImGuiKey_Tab, false);
            AT(_gui_icon_frame(&state));
            if (state.focused)
                break;
        }
        AT(state.focused && state.tooltip);
        ImGuiIO_AddKeyEvent(io, ImGuiKey_Space, true);
        AT(_gui_icon_frame(&state));
        const bool key_pressed = state.pressed;
        ImGuiIO_AddKeyEvent(io, ImGuiKey_Space, false);
        AT(_gui_icon_frame(&state));
        AT(key_pressed || state.pressed);
        state.flags = DVZ_GUI_ICON_BUTTON_FLAGS_ACTIVE | DVZ_GUI_ICON_BUTTON_FLAGS_DISABLED;
        ImGuiIO_AddKeyEvent(io, ImGuiKey_Space, true);
        AT(_gui_icon_frame(&state));
        AT(!state.pressed);
        ImGuiIO_AddKeyEvent(io, ImGuiKey_Space, false);
        AT(_gui_icon_frame(&state));
        AT(!state.pressed && state.passed);
        AT(state.indicator && !state.held_background);
        igDestroyContext(context);
    }
    return 0;
}


/**
 * Verify opt-in tree density and secondary text while preserving missing-value styling.
 *
 * @param suite test context
 * @param item test item
 * @return zero on success
 */
static int test_gui_compact_tree_layout(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;
    ImGuiContext* context = igCreateContext(NULL);
    AT(context != NULL);
    ImGuiIO* io = igGetIO_Nil();
    io->DisplaySize = ImVec2{720, 600};
    io->DeltaTime = 1.0f / 60.0f;
    io->IniFilename = NULL;
    AT(ImFontAtlas_Build(io->Fonts));
    ImGuiStyle* style = igGetStyle();
    style->FramePadding.y = 10;
    style->ItemSpacing.y = 8;
    style->Colors[ImGuiCol_Text] = ImVec4{1, 0, 0, 1};
    style->Colors[ImGuiCol_TextDisabled] = ImVec4{0, 1, 0, 1};
    DvzGuiTree* tree = dvz_gui_tree("density", 0);
    const uint64_t keys[] = {1, 2};
    const uint32_t parents[] = {UINT32_MAX, UINT32_MAX};
    const char* labels[] = {"First", "Second"};
    const char* secondary[] = {"description", "description"};
    AT(dvz_gui_tree_set_rows(tree, 2, keys, parents, labels, secondary, 0) == DVZ_OK);
    igNewFrame();
    igSetNextWindowSize(ImVec2{700, 580}, ImGuiCond_Always);
    igBegin("density CPU", NULL, ImGuiWindowFlags_NoSavedSettings);
    ImDrawList* draw = igGetWindowDrawList();
    const ImU32 dim = igGetColorU32_Col(ImGuiCol_TextDisabled, 1);
    DvzGuiDataEvent events[32] = {};
    uint32_t written = 0, dropped = 0;
    ImVec2 start = {}, end = {};
    igGetCursorScreenPos(&start);
    int vertices = draw->VtxBuffer.Size;
    AT(_dvz_gui_tree_debug_draw(tree, events, 32, &written, &dropped) == DVZ_OK);
    igGetCursorScreenPos(&end);
    const float default_height = end.y - start.y;
    AT(_gui_vertices_with_color(draw, vertices, dim) > 0);
    DvzGuiTreeLayout layout = dvz_gui_tree_layout();
    AT(layout.flags == 0);
    layout.flags =
        DVZ_GUI_TREE_LAYOUT_FLAGS_EXACT_SPACING | DVZ_GUI_TREE_LAYOUT_FLAGS_READABLE_SECONDARY;
    layout.row_padding_em = 0.1f;
    layout.item_spacing_em = 0;
    AT(dvz_gui_tree_set_layout(tree, &layout) == DVZ_OK);
    igGetCursorScreenPos(&start);
    vertices = draw->VtxBuffer.Size;
    AT(_dvz_gui_tree_debug_draw(tree, events, 32, &written, &dropped) == DVZ_OK);
    igGetCursorScreenPos(&end);
    const float exact_height = end.y - start.y;
    AC(exact_height, 2 * igGetFontSize() * 1.2f, 2.0f);
    AT(exact_height < default_height);
    AT(_gui_vertices_with_color(draw, vertices, dim) == 0);
    AC(style->FramePadding.y, 10, 0.001f);
    AC(style->ItemSpacing.y, 8, 0.001f);
    DvzGuiTreeAnnotation annotation = dvz_gui_tree_annotation();
    annotation.row_key = keys[0];
    annotation.flags = DVZ_GUI_TREE_ANNOTATION_FLAGS_MISSING;
    annotation.value_text = "missing";
    AT(dvz_gui_tree_set_annotations(tree, 1, &annotation) == DVZ_OK);
    vertices = draw->VtxBuffer.Size;
    AT(_dvz_gui_tree_debug_draw(tree, events, 32, &written, &dropped) == DVZ_OK);
    AT(_gui_vertices_with_color(draw, vertices, dim) > 0);
    layout.flags = 4;
    AT(dvz_gui_tree_set_layout(tree, &layout) == DVZ_ERROR);
    igEnd();
    igRender();
    dvz_gui_tree_destroy(tree);
    igDestroyContext(context);
    return 0;
}



/**
 * Check that the curated GUI widget wrappers are exported with C-callable signatures.
 *
 * @param suite test suite
 * @param item test item
 * @return 0 on success
 */
static int test_gui_widget_wrapper_symbols(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    bool (*slider_int)(DvzGui*, const char*, int*, int, int) = dvz_gui_slider_int;
    bool (*slider_float2)(DvzGui*, const char*, float*, float, float) = dvz_gui_slider_float2;
    bool (*slider_float3)(DvzGui*, const char*, float*, float, float) = dvz_gui_slider_float3;
    bool (*slider_float4)(DvzGui*, const char*, float*, float, float) = dvz_gui_slider_float4;
    bool (*slider_range_float)(
        DvzGui*, const char*, float*, float*, float, float, const char*) =
        dvz_gui_slider_range_float;
    bool (*range_float)(
        DvzGui*, const char*, float*, float*, float, float, float, const char*) =
        dvz_gui_range_float;
    bool (*color_edit4)(DvzGui*, const char*, float*, int) = dvz_gui_color_edit4;
    bool (*color_edit_dvz)(DvzGui*, const char*, DvzColor*, int) = dvz_gui_color_edit_dvz;
    bool (*color_picker4)(DvzGui*, const char*, float*, int) = dvz_gui_color_picker4;
    void (*separator_text)(DvzGui*, const char*) = dvz_gui_separator_text;
    bool (*collapsing_header)(DvzGui*, const char*, int) = dvz_gui_collapsing_header;
    void (*same_line)(DvzGui*, float, float) = dvz_gui_same_line;
    DvzResult (*dock_window_once)(DvzGui*, const char*, DvzGuiDockSlot, float) =
        dvz_gui_dock_window_once;
    bool (*current_window_docked)(DvzGui*) = dvz_gui_current_window_docked;
    bool (*current_window_rect)(DvzGui*, DvzRect*) = dvz_gui_current_window_rect;
    void (*tooltip)(DvzGui*, const char*) = dvz_gui_tooltip;
    bool (*input_text)(DvzGui*, const char*, char*, uint32_t) = dvz_gui_input_text;
    bool (*begin_child)(DvzGui*, const char*, float, float, int) = dvz_gui_begin_child;
    void (*end_child)(DvzGui*) = dvz_gui_end_child;

    AT(slider_int != NULL);
    AT(slider_float2 != NULL);
    AT(slider_float3 != NULL);
    AT(slider_float4 != NULL);
    AT(slider_range_float != NULL);
    AT(range_float != NULL);
    AT(color_edit4 != NULL);
    AT(color_edit_dvz != NULL);
    AT(color_picker4 != NULL);
    AT(separator_text != NULL);
    AT(collapsing_header != NULL);
    AT(same_line != NULL);
    AT(dock_window_once != NULL);
    AT(current_window_docked != NULL);
    AT(current_window_rect != NULL);
    AT(tooltip != NULL);
    AT(input_text != NULL);
    AT(begin_child != NULL);
    AT(end_child != NULL);
    return 0;
}



/**
 * Smoke viewport resize handling and hidden-window render gating.
 *
 * @param suite test suite
 * @param item test item
 * @return 0 on success
 */
static int test_gui_viewport_resize_hidden_smoke(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    if (!_gui_smoke_available())
    {
        tst_skip(suite, "GUI/GLFW support unavailable");
        return 0;
    }

    GuiTestGpuResources gpu_resources = {};
    tst_expect_error_begin(suite);
    const char* gpu_skip = _gui_test_gpu_resources_create(suite, &gpu_resources);
    (void)tst_expect_error_end(suite);
    if (gpu_skip != NULL)
    {
        tst_skip(suite, gpu_skip);
        return 0;
    }
    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* source_figure = _gui_test_figure(scene, 160, 120);
    DvzFigure* host_figure = _gui_test_figure(scene, 640, 480);
    AT(source_figure != NULL);
    AT(host_figure != NULL);

    DvzApp* app = _gui_test_app(scene, NULL, &gpu_resources);
    if (app == NULL)
    {
        log_warn("test_gui_viewport_resize_hidden_smoke skipped: GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        dvz_scene_destroy(scene);
        _gui_test_gpu_resources_destroy(&gpu_resources);
        return 0;
    }

    DvzView* source_win = dvz_view_offscreen(app, source_figure, 160, 120);
    tst_expect_error_begin(suite);
    DvzView* host_win =
        dvz_view_window(app, host_figure, 640, 480, "test_gui_viewport_resize_hidden_smoke");
    (void)tst_expect_error_end(suite);
    if (source_win == NULL || host_win == NULL)
    {
        log_warn("test_gui_viewport_resize_hidden_smoke skipped: view creation failed");
        tst_skip(suite, "view creation failed");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        _gui_test_gpu_resources_destroy(&gpu_resources);
        return 0;
    }

    DvzGuiConfig invalid_gui = dvz_gui_config();
    invalid_gui.struct_size = 0;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_view_gui(host_win, &invalid_gui) == NULL);

    invalid_gui = dvz_gui_config();
    invalid_gui.flags = 1;
    AT_EXPECTED_ERROR_STRICT(suite, dvz_view_gui(host_win, &invalid_gui) == NULL);

    DvzGui* gui = dvz_view_gui(host_win, NULL);
    if (gui == NULL)
    {
        log_warn("test_gui_viewport_resize_hidden_smoke skipped: GUI creation failed");
        tst_skip(suite, "GUI creation failed");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        _gui_test_gpu_resources_destroy(&gpu_resources);
        return 0;
    }

    DvzGuiViewportConfig config = dvz_gui_viewport_config();
    config.resize_step = 1;
    config.resize_delay_frames = 2;

    AT(dvz_view_resize_scaled(source_win, 160, 120, 2.0f) == 0);
    uint32_t logical_width = 0;
    uint32_t logical_height = 0;
    uint32_t framebuffer_width = 0;
    uint32_t framebuffer_height = 0;
    dvz_view_logical_size(source_win, &logical_width, &logical_height);
    dvz_view_framebuffer_size(source_win, &framebuffer_width, &framebuffer_height);
    AT(logical_width == 160);
    AT(logical_height == 120);
    AT(framebuffer_width == 320);
    AT(framebuffer_height == 240);
    AC(dvz_view_device_scale(source_win), 2.0f, 1e-6f);

    AT(dvz_view_resize_scaled_xy(source_win, 160, 120, 2.0f, 1.5f) == 0);
    dvz_view_logical_size(source_win, &logical_width, &logical_height);
    dvz_view_framebuffer_size(source_win, &framebuffer_width, &framebuffer_height);
    DvzScaleXY device_scale = dvz_view_device_scale_xy(source_win);
    AT(logical_width == 160);
    AT(logical_height == 120);
    AT(framebuffer_width == 320);
    AT(framebuffer_height == 180);
    AC(device_scale.x, 2.0f, 1e-6f);
    AC(device_scale.y, 1.5f, 1e-6f);

    DvzGuiViewportConfig invalid_viewport = config;
    invalid_viewport.struct_size = 0;
    AT_EXPECTED_ERROR_STRICT(
        suite, dvz_gui_viewport_from_window(gui, source_win, &invalid_viewport) == NULL);

    invalid_viewport = config;
    invalid_viewport.flags = 1;
    AT_EXPECTED_ERROR_STRICT(
        suite, dvz_gui_viewport_from_window(gui, source_win, &invalid_viewport) == NULL);

    GuiViewportSmoke smoke = {};
    smoke.viewport = dvz_gui_viewport_from_window(gui, source_win, &config);
    AT(smoke.viewport != NULL);
    AT(dvz_view_request_frame(source_win) == 0);
    AT(!_dvz_view_scheduler_should_render(source_win, UINT64_MAX));

    dvz_view_set_gui_callback(host_win, _gui_viewport_resize_callback, &smoke);
    dvz_app_run(app, 6);

    AT(smoke.shown_count > 0);
    AT(smoke.hidden_count > 0);
    AT(smoke.transition_drawable_count > 0);
    AT(smoke.visible_transition_count > 0);
    AT(smoke.visible_transition_shown_count == smoke.visible_transition_count);
    AT(!dvz_view_render_enabled(source_win));

    DvzGuiViewportDebugState debug = {};
    AT(_dvz_gui_viewport_debug_state(smoke.viewport, &debug));
    AT(debug.has_frame);
    AT(debug.image_valid);
    AT(debug.displayed_resource_generation > 0);
    AT(debug.display_drawable);
    AT(debug.display_ready);
    AT(debug.pending_width == 0);
    AT(debug.pending_height == 0);
    AT(debug.displayed_framebuffer_width == debug.requested_framebuffer_width);
    AT(debug.displayed_framebuffer_height == debug.requested_framebuffer_height);

    /* Reopen and settle the viewport after its hidden-state and scrollbar transitions. */
    smoke.frame = 2;
    AT(dvz_view_render_once(host_win) == DVZ_CANVAS_FRAME_READY);
    AT(_dvz_gui_viewport_debug_state(smoke.viewport, &debug));

    smoke.frame = 2;
    AT(dvz_view_render_once(host_win) == DVZ_CANVAS_FRAME_READY);
    AT(_dvz_gui_viewport_debug_state(smoke.viewport, &debug));
    uint64_t source_frame_count = debug.source_frame_count;

    /* A stable host frame must not resubmit an idle viewport source. */
    smoke.frame = 2;
    AT(dvz_view_render_once(host_win) == DVZ_CANVAS_FRAME_READY);
    AT(_dvz_gui_viewport_debug_state(smoke.viewport, &debug));
    AT(debug.source_frame_count == source_frame_count);

    /* Explicit source demand must still produce exactly one submitted frame. */
    smoke.frame = 2;
    AT(dvz_view_request_frame(source_win) == 0);
    AT(dvz_view_render_once(host_win) == DVZ_CANVAS_FRAME_READY);
    AT(_dvz_gui_viewport_debug_state(smoke.viewport, &debug));
    AT(debug.source_frame_count == source_frame_count + 1);

    uint32_t width = 0;
    uint32_t height = 0;
    uint8_t* rgba = NULL;
    AT(dvz_canvas_capture_rgba(dvz_view_canvas(source_win), &width, &height, &rgba) == 0);
    AT(width > 0);
    AT(height > 0);
    dvz_free(rgba);

    dvz_gui_viewport_destroy(smoke.viewport);
    AT(dvz_view_request_frame(source_win) == 0);
    AT(_dvz_view_scheduler_should_render(source_win, UINT64_MAX));
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    _gui_test_gpu_resources_destroy(&gpu_resources);
    return 0;
}


static int test_gui_config_inherits_app_font_defaults(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    if (!_gui_smoke_available())
    {
        tst_skip(suite, "GUI/GLFW support unavailable");
        return 0;
    }

    DvzAppConfig app_config = dvz_app_config();
    app_config.font_sans_family = "App Sans";
    app_config.font_sans_style = "Book";
    app_config.font_mono_family = "App Mono";
    app_config.font_ui_size_px = 19.0f;
    app_config.font_mono_size_px = 17.0f;
    app_config.font_text_size_px = 23.0f;

    float base_font_size = 0.0f;
    for (uint32_t i = 0; i < 2; i++)
    {
        GuiTestGpuResources gpu_resources = {};
        tst_expect_error_begin(suite);
        const char* gpu_skip = _gui_test_gpu_resources_create(suite, &gpu_resources);
        (void)tst_expect_error_end(suite);
        if (gpu_skip != NULL)
        {
            tst_skip(suite, gpu_skip);
            return 0;
        }

        DvzScene* scene = dvz_scene();
        ANN(scene);
        DvzFigure* figure = _gui_test_figure(scene, 320, 240);
        ANN(figure);

        DvzApp* app = _gui_test_app(scene, &app_config, &gpu_resources);
        if (app == NULL)
        {
            log_warn("test_gui_config_inherits_app_font_defaults skipped: GPU context creation failed");
            tst_skip(suite, "GPU context creation failed");
            dvz_scene_destroy(scene);
            _gui_test_gpu_resources_destroy(&gpu_resources);
            return 0;
        }

        const char* title =
            i == 0 ? "test_gui_null_font_defaults" : "test_gui_explicit_font_defaults";
        tst_expect_error_begin(suite);
        DvzView* win = dvz_view_window(app, figure, 320, 240, title);
        (void)tst_expect_error_end(suite);
        if (win == NULL)
        {
            log_warn("test_gui_config_inherits_app_font_defaults skipped: view creation failed");
            tst_skip(suite, "view creation failed");
            dvz_app_destroy(app);
            dvz_scene_destroy(scene);
            _gui_test_gpu_resources_destroy(&gpu_resources);
            return 0;
        }
        const float user_scale = i == 0 ? 1.0f : 1.5f;
        AT(dvz_view_set_user_scale(win, user_scale) == DVZ_OK);

        DvzGuiConfig gui_config = dvz_gui_config();
        gui_config.gui_flags |= DVZ_GUI_FLAGS_NAV_KEYBOARD;
        DvzGui* gui = dvz_view_gui(win, i == 0 ? NULL : &gui_config);
        if (gui == NULL)
        {
            log_warn("test_gui_config_inherits_app_font_defaults skipped: GUI creation failed");
            tst_skip(suite, "GUI creation failed");
            dvz_app_destroy(app);
            dvz_scene_destroy(scene);
            _gui_test_gpu_resources_destroy(&gpu_resources);
            return 0;
        }

        DvzFontDefaults fonts = _dvz_gui_font_defaults(gui);
        AT(strcmp(fonts.sans_family, "App Sans") == 0);
        AT(strcmp(fonts.sans_style, "Book") == 0);
        AT(strcmp(fonts.mono_family, "App Mono") == 0);
        AC(fonts.ui_size_px, 19.0f, 1e-6f);
        AC(fonts.mono_size_px, 17.0f, 1e-6f);
        AC(fonts.text_size_px, 23.0f, 1e-6f);
        _dvz_gui_set_current(gui);
        ImGuiIO* io = igGetIO_Nil();
        AT(io != NULL);
        AT(io->FontDefault != NULL);
        AC(io->FontGlobalScale, 1.0f, 1e-6f);
        AT(((io->ConfigFlags & ImGuiConfigFlags_NavEnableKeyboard) != 0) == (i != 0));
        if (i == 0)
            base_font_size = io->FontDefault->FontSize;
        else
            AC(io->FontDefault->FontSize, 1.5f * base_font_size, 1e-5f);

        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        _gui_test_gpu_resources_destroy(&gpu_resources);
    }
    return 0;
}



/**
 * Smoke independent input routers for multiple GUI viewports.
 *
 * @param suite test suite
 * @param item test item
 * @return 0 on success
 */
static int test_gui_multi_viewport_input_routers(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    if (!_gui_smoke_available())
    {
        tst_skip(suite, "GUI/GLFW support unavailable");
        return 0;
    }

    GuiTestGpuResources gpu_resources = {};
    tst_expect_error_begin(suite);
    const char* gpu_skip = _gui_test_gpu_resources_create(suite, &gpu_resources);
    (void)tst_expect_error_end(suite);
    if (gpu_skip != NULL)
    {
        tst_skip(suite, gpu_skip);
        return 0;
    }

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* source_a = _gui_test_figure(scene, 160, 120);
    DvzFigure* source_b = _gui_test_figure(scene, 160, 120);
    DvzFigure* host_figure = _gui_test_figure(scene, 640, 480);
    AT(source_a != NULL);
    AT(source_b != NULL);
    AT(host_figure != NULL);

    DvzApp* app = _gui_test_app(scene, NULL, &gpu_resources);
    if (app == NULL)
    {
        log_warn("test_gui_multi_viewport_input_routers skipped: GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        dvz_scene_destroy(scene);
        _gui_test_gpu_resources_destroy(&gpu_resources);
        return 0;
    }

    tst_expect_error_begin(suite);
    DvzView* host_win =
        dvz_view_window(app, host_figure, 640, 480, "test_gui_multi_viewport_input_routers");
    (void)tst_expect_error_end(suite);
    if (host_win == NULL)
    {
        log_warn("test_gui_multi_viewport_input_routers skipped: GLFW window creation failed");
        tst_skip(suite, "view creation failed");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        _gui_test_gpu_resources_destroy(&gpu_resources);
        return 0;
    }

    DvzGui* gui = dvz_view_gui(host_win, NULL);
    if (gui == NULL)
    {
        log_warn("test_gui_multi_viewport_input_routers skipped: GUI creation failed");
        tst_skip(suite, "GUI creation failed");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        _gui_test_gpu_resources_destroy(&gpu_resources);
        return 0;
    }

    DvzGuiViewportConfig config = dvz_gui_viewport_config();
    config.initial_width = 160;
    config.initial_height = 120;
    AT(dvz_view_set_user_scale(host_win, 1.5f) == DVZ_OK);
    DvzGuiViewport* viewport_a = dvz_gui_viewport(gui, source_a, &config);
    DvzGuiViewport* viewport_b = dvz_gui_viewport(gui, source_b, &config);
    AT(viewport_a != NULL);
    AT(viewport_b != NULL);
    DvzGuiViewportDebugState debug_a = {};
    AT(_dvz_gui_viewport_debug_state(viewport_a, &debug_a));
    AC(debug_a.source_user_scale, 1.5f, 1e-6f);

    DvzInputRouter* router_a = dvz_gui_viewport_input(viewport_a);
    DvzInputRouter* router_b = dvz_gui_viewport_input(viewport_b);
    AT(router_a != NULL);
    AT(router_b != NULL);
    AT(router_a != router_b);

    GuiInputRecorder recorder_a = {};
    GuiInputRecorder recorder_b = {};
    DvzCallbackId id_a =
        dvz_input_subscribe_pointer(router_a, _gui_record_pointer, &recorder_a);
    DvzCallbackId id_b =
        dvz_input_subscribe_pointer(router_b, _gui_record_pointer, &recorder_b);
    DvzCallbackId event_id_a =
        dvz_input_subscribe_event(router_a, _gui_record_input_event, &recorder_a);
    AT(id_a != DVZ_CALLBACK_ID_NONE);
    AT(id_b != DVZ_CALLBACK_ID_NONE);
    AT(event_id_a != DVZ_CALLBACK_ID_NONE);

    _gui_emit_pointer(router_a, DVZ_POINTER_EVENT_MOVE);
    AT(recorder_a.count == 1);
    AT(recorder_a.last_type == DVZ_POINTER_EVENT_MOVE);
    AT(recorder_a.union_types[DVZ_POINTER_EVENT_MOVE] == 1);
    AT(recorder_b.count == 0);

    _gui_emit_pointer(router_b, DVZ_POINTER_EVENT_PRESS);
    AT(recorder_a.count == 1);
    AT(recorder_b.count == 1);
    AT(recorder_b.last_type == DVZ_POINTER_EVENT_PRESS);

    /* Owned offscreen viewports synthesize gestures from forwarded raw events. */
    dvz_pointer_emit_position(
        router_a, DVZ_POINTER_EVENT_PRESS, 10.0f, 20.0f, 100.0f, 80.0f,
        DVZ_POINTER_BUTTON_LEFT, 0, 1.0f, dvz_input_timestamp_ns(), NULL);
    dvz_pointer_emit_position(
        router_a, DVZ_POINTER_EVENT_MOVE, 20.0f, 25.0f, 100.0f, 80.0f,
        DVZ_POINTER_BUTTON_NONE, 0, 1.0f, dvz_input_timestamp_ns(), NULL);
    dvz_pointer_emit_position(
        router_a, DVZ_POINTER_EVENT_MOVE, 30.0f, 30.0f, 100.0f, 80.0f,
        DVZ_POINTER_BUTTON_NONE, 0, 1.0f, dvz_input_timestamp_ns(), NULL);
    AT(recorder_a.drag_count == 1);
    AT(recorder_a.union_types[DVZ_POINTER_EVENT_PRESS] == 1);
    AT(recorder_a.union_types[DVZ_POINTER_EVENT_DRAG_START] == 1);
    dvz_pointer_emit_position(
        router_a, DVZ_POINTER_EVENT_RELEASE, 20.0f, 25.0f, 100.0f, 80.0f,
        DVZ_POINTER_BUTTON_LEFT, 0, 1.0f, dvz_input_timestamp_ns(), NULL);

    AT(recorder_a.union_types[DVZ_POINTER_EVENT_DRAG_STOP] == 1);
    AT(recorder_a.union_types[DVZ_POINTER_EVENT_RELEASE] == 1);
    dvz_pointer_emit_wheel(
        router_a, 30, 30, 100, 80, 0, -1, 0, 1, dvz_input_timestamp_ns(), NULL);
    AT(recorder_a.union_types[DVZ_POINTER_EVENT_WHEEL] == 1);

    /* Stationary forwarding settles despite the query's own frame request. */
    DvzPanel* panel = dvz_panel_full(source_a);
    AT(panel != NULL);
    AT(dvz_item_interaction(panel, NULL) != NULL);
    AT(dvz_panel_connect_input(panel, router_a) == DVZ_OK);
    DvzVisual* points = dvz_point(scene, 0);
    vec3 position[1] = {{0, 0, 0}};
    AT(dvz_visual_set_data(points, "position", position, 1) == DVZ_OK);
    AT(dvz_panel_add_visual(panel, points, NULL) == DVZ_OK);
    AT(_dvz_gui_viewport_forward_move(viewport_a, true, 50, 40, 160, 120, 0));
    uint32_t stationary_count = recorder_a.count;
    for (uint32_t i = 0; i < 100; i++)
        AT(!_dvz_gui_viewport_forward_move(viewport_a, true, 50, 40, 160, 120, 0));
    AT(recorder_a.count == stationary_count);

    /* Geometry, camera, size, modifiers and re-entry independently refresh input. */
    position[0][0] = .2f;
    AT(dvz_visual_set_data(points, "position", position, 1) == DVZ_OK);
    AT(_dvz_gui_viewport_forward_move(viewport_a, true, 50, 40, 160, 120, 0));
    AT(!_dvz_gui_viewport_forward_move(viewport_a, true, 50, 40, 160, 120, 0));
    DvzCameraDesc camera = dvz_camera_desc();
    AT(dvz_panel_set_camera_desc(panel, &camera) == DVZ_OK);
    AT(_dvz_gui_viewport_forward_move(viewport_a, true, 50, 40, 160, 120, 0));
    AT(_dvz_gui_viewport_forward_move(viewport_a, true, 50, 40, 180, 120, 0));
    AT(_dvz_gui_viewport_forward_move(
        viewport_a, true, 50, 40, 180, 120, DVZ_KEY_MODIFIER_SHIFT));
    AT(!_dvz_gui_viewport_forward_move(viewport_a, false, 50, 40, 180, 120, 0));
    AT(_dvz_gui_viewport_forward_move(viewport_a, true, 50, 40, 180, 120, 0));
    AT(_dvz_gui_viewport_forward_move(viewport_b, true, 50, 40, 160, 120, 0));
    AT(!_dvz_gui_viewport_forward_move(viewport_b, true, 50, 40, 160, 120, 0));

    dvz_input_unsubscribe(router_a, id_a);
    dvz_input_unsubscribe(router_a, event_id_a);
    dvz_input_unsubscribe(router_b, id_b);
    dvz_gui_viewport_destroy(viewport_a);
    dvz_gui_viewport_destroy(viewport_b);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    _gui_test_gpu_resources_destroy(&gpu_resources);
    return 0;
}



/**
 * Verify the native view-input path used by docked GUI viewport forwarding.
 *
 * This deliberately goes through dvz_view_emit_pointer(), rather than calling
 * the arcball directly. The GUI bridge forwards ImGui pointer events to this
 * view API, so this test catches broken panel connection, coordinate routing,
 * and press/move/release drag synthesis. ImGui/X11 injection remains a manual
 * acceptance concern because the test runner does not own a stable desktop.
 *
 * @param suite test suite
 * @param item test case
 * @return 0 on success
 */
static int test_gui_forwarded_viewport_arcball(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    if (!_gui_smoke_available())
    {
        tst_skip(suite, "GUI/GLFW support unavailable");
        return 0;
    }

    GuiTestGpuResources gpu_resources = {};
    tst_expect_error_begin(suite);
    const char* gpu_skip = _gui_test_gpu_resources_create(suite, &gpu_resources);
    (void)tst_expect_error_end(suite);
    if (gpu_skip != NULL)
    {
        tst_skip(suite, gpu_skip);
        return 0;
    }

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* source_figure = dvz_figure(scene, 240, 180, 0);
    DvzFigure* host_figure = _gui_test_figure(scene, 640, 480);
    AT(source_figure != NULL);
    AT(host_figure != NULL);
    DvzPanel* source_panel = dvz_panel_full(source_figure);
    AT(source_panel != NULL);

    /* Use asymmetric 3-D geometry so the rendered image must change when the arcball changes. */
    DvzVisual* points = dvz_point(scene, 0);
    AT(points != NULL);
    vec3 positions[3] = {
        {-0.72f, -0.38f, +0.18f},
        {+0.48f, -0.12f, -0.44f},
        {+0.16f, +0.68f, +0.35f},
    };
    DvzColor colors[3] = {
        {238, 82, 76, 255},
        {66, 184, 131, 255},
        {75, 132, 235, 255},
    };
    float sizes[3] = {28.0f, 20.0f, 24.0f};
    AT(dvz_visual_set_data(points, "position", positions, 3) == DVZ_OK);
    AT(dvz_visual_set_data(points, "color", colors, 3) == DVZ_OK);
    AT(dvz_visual_set_data(points, "size", sizes, 3) == DVZ_OK);
    AT(dvz_panel_add_visual(source_panel, points, NULL) == DVZ_OK);

    DvzApp* app = _gui_test_app(scene, NULL, &gpu_resources);
    if (app == NULL)
    {
        tst_skip(suite, "GPU context creation failed");
        dvz_scene_destroy(scene);
        _gui_test_gpu_resources_destroy(&gpu_resources);
        return 0;
    }

    DvzView* source_win = dvz_view_offscreen(app, source_figure, 240, 180);
    tst_expect_error_begin(suite);
    DvzView* host_win =
        dvz_view_window(app, host_figure, 640, 480, "test_gui_forwarded_viewport_arcball");
    (void)tst_expect_error_end(suite);
    if (source_win == NULL || host_win == NULL)
    {
        tst_skip(suite, "view creation failed");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        _gui_test_gpu_resources_destroy(&gpu_resources);
        return 0;
    }

    DvzGui* gui = dvz_view_gui(host_win, NULL);
    if (gui == NULL)
    {
        tst_skip(suite, "GUI creation failed");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        _gui_test_gpu_resources_destroy(&gpu_resources);
        return 0;
    }
    DvzGuiViewportConfig viewport_config = dvz_gui_viewport_config();
    DvzGuiViewport* viewport = dvz_gui_viewport_from_window(gui, source_win, &viewport_config);
    AT(viewport != NULL);

    DvzController* controller = dvz_arcball(scene, NULL);
    AT(controller != NULL);
    DvzArcball* arcball = dvz_controller_arcball(controller);
    AT(arcball != NULL);
    AT(dvz_view_bind_controller(source_win, source_panel, controller, DVZ_DIM_MASK_XYZ) == DVZ_OK);

    GuiInputRecorder recorder = {};
    DvzInputRouter* router = dvz_view_input(source_win);
    AT(router != NULL);
    DvzCallbackId pointer_id = dvz_input_subscribe_pointer(router, _gui_record_pointer, &recorder);
    AT(pointer_id != DVZ_CALLBACK_ID_NONE);
    DvzCallbackId event_id = dvz_input_subscribe_event(router, _gui_record_input_event, &recorder);
    AT(event_id != DVZ_CALLBACK_ID_NONE);

    AT(dvz_view_render_once(source_win) == DVZ_CANVAS_FRAME_READY);
    uint32_t before_width = 0;
    uint32_t before_height = 0;
    uint8_t* before_rgba = NULL;
    AT(dvz_canvas_capture_rgba(
           dvz_view_canvas(source_win), &before_width, &before_height, &before_rgba) == DVZ_OK);
    ANN(before_rgba);

    vec3 before = {0};
    vec3 after = {0};
    dvz_arcball_angles(arcball, before);
    AT(dvz_view_emit_pointer(
           source_win, DVZ_POINTER_EVENT_PRESS, 48.0f, 54.0f, 240.0f, 180.0f,
           DVZ_POINTER_BUTTON_LEFT, 0) == DVZ_OK);
    AT(dvz_view_emit_pointer(
           source_win, DVZ_POINTER_EVENT_MOVE, 132.0f, 94.0f, 240.0f, 180.0f,
           DVZ_POINTER_BUTTON_NONE, 0) == DVZ_OK);
    AT(dvz_view_emit_pointer(
           source_win, DVZ_POINTER_EVENT_MOVE, 168.0f, 120.0f, 240.0f, 180.0f,
           DVZ_POINTER_BUTTON_NONE, 0) == DVZ_OK);
    AT(dvz_view_emit_pointer(
           source_win, DVZ_POINTER_EVENT_RELEASE, 168.0f, 120.0f, 240.0f, 180.0f,
           DVZ_POINTER_BUTTON_LEFT, 0) == DVZ_OK);
    dvz_arcball_angles(arcball, after);

    AT(recorder.count == 4);
    AT(recorder.drag_count == 1);
    AT(recorder.union_types[DVZ_POINTER_EVENT_PRESS] == 1);
    AT(recorder.union_types[DVZ_POINTER_EVENT_DRAG_START] == 1);
    AT(recorder.union_types[DVZ_POINTER_EVENT_DRAG_STOP] == 1);
    AT(recorder.union_types[DVZ_POINTER_EVENT_RELEASE] == 1);
    AT(fabsf(after[0] - before[0]) > 1e-5f || fabsf(after[1] - before[1]) > 1e-5f);
    AT(!dvz_arcball_is_interacting(arcball));

    AT(dvz_view_render_once(source_win) == DVZ_CANVAS_FRAME_READY);
    uint32_t after_width = 0;
    uint32_t after_height = 0;
    uint8_t* after_rgba = NULL;
    AT(dvz_canvas_capture_rgba(
           dvz_view_canvas(source_win), &after_width, &after_height, &after_rgba) == DVZ_OK);
    ANN(after_rgba);
    AT(after_width == before_width);
    AT(after_height == before_height);
    uint32_t changed_pixels = 0;
    for (uint32_t i = 0; i < before_width * before_height; i++)
    {
        if (memcmp(&before_rgba[4 * i], &after_rgba[4 * i], 4) != 0)
            changed_pixels++;
    }
    AT(changed_pixels > 32);
    dvz_free(after_rgba);
    dvz_free(before_rgba);

    /* Destroying the borrowing viewport must preserve the source window interpreter. */
    dvz_gui_viewport_destroy(viewport);
    uint32_t clicks_before = recorder.union_types[DVZ_POINTER_EVENT_CLICK];
    dvz_pointer_emit_position(
        router, DVZ_POINTER_EVENT_PRESS, 10, 10, 160, 120, DVZ_POINTER_BUTTON_LEFT,
        0, 1, 10000000000ULL, NULL);
    dvz_pointer_emit_position(
        router, DVZ_POINTER_EVENT_RELEASE, 10, 10, 160, 120, DVZ_POINTER_BUTTON_LEFT,
        0, 1, 10010000000ULL, NULL);
    AT(recorder.union_types[DVZ_POINTER_EVENT_CLICK] == clicks_before + 1);
    dvz_input_unsubscribe(router, pointer_id);
    dvz_input_unsubscribe(router, event_id);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    _gui_test_gpu_resources_destroy(&gpu_resources);
    return 0;
}



/**
 * Register GUI tests.
 *
 * @param suite test suite
 * @return 0 on success
 */
int test_gui(TstSuite* suite)
{
    ANN(suite);
    const char* tags = "gui";
    TST_MODULE(suite, tags);

#define TST_GUI_GPU_CASE(test)                                                                   \
    do                                                                                            \
    {                                                                                             \
        TstCaseDesc _tst_desc = tst_case_desc(#test, #test, (test));                              \
        _tst_desc.tags = tags;                                                                    \
        _tst_desc.resources = TST_RES_CPU | TST_RES_GPU | TST_RES_VULKAN | TST_RES_GLFW;          \
        _tst_desc.isolation = TST_ISOLATION_PROCESS;                                              \
        _tst_desc.run_flags = TST_RUN_CASE_ADAPTER_SUPPORTED;                                     \
        tst_suite_add_case((suite), _tst_desc);                                                   \
    } while (0)

    TST_CASE(test_gui_imgui_public_header);
    TST_CASE(test_gui_viewport_config_defaults);
    TST_CASE(test_gui_config_font_defaults);
    TST_CASE(test_gui_scale_resolution);
    TST_CASE(test_gui_style_scale);
    TST_CASE(test_gui_embedded_font_resources);
    TST_CASE(test_gui_srgb_vertex_colors);
    TST_CASE(test_gui_data_widget_storage);
    TST_CASE(test_gui_data_tree_model);
    TST_CASE(test_gui_data_tree_controlled_model);
    TST_CASE(test_gui_data_tree_controlled_draw);
    TST_CASE(test_gui_data_table_model);
    TST_CASE(test_gui_compact_layout);
    TST_CASE(test_gui_compact_icons);
    TST_CASE(test_gui_compact_tree_layout);
    TST_CASE(test_gui_widget_wrapper_symbols);
    TST_GUI_GPU_CASE(test_gui_config_inherits_app_font_defaults);
    TST_GUI_GPU_CASE(test_gui_viewport_resize_hidden_smoke);
    TST_GUI_GPU_CASE(test_gui_multi_viewport_input_routers);
    TST_GUI_GPU_CASE(test_gui_forwarded_viewport_arcball);

#undef TST_GUI_GPU_CASE
    return 0;
}
