/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

#include "../../drp2/_stream.h"
#include "../core/_scene.h"
#include "../core/_scene_resource_key.h"
#include "../core/figure_emit_internal.h"
#include "../domain/buffer_internal.h"
#include "../scene_emit/scene_emit.h"
#include "../shaders/_scene_shader_abi.h"
#include "_alloc.h"
#include "_assertions.h"
#include "datoviz/geom.h"
#include "datoviz/scene.h"
#include "helpers.h"
#include "test_scene.h"
#include "testing.h"
#include "visuals/_visual_internal.h"
#include <string.h>
#include "visuals/mesh/internal.h"
#include <math.h>


static DvzGeometry* _indexed_state_triangle(bool lit)
{
    DvzGeometry* geometry = dvz_geometry(3, 3);
    if (geometry == NULL)
        return NULL;
    const double positions[3][3] = {{-0.5, -0.5, 0.0}, {0.5, -0.5, 0.0}, {0.0, 0.5, 0.0}};
    for (uint32_t i = 0; i < 3; i++)
    {
        memcpy(geometry->positions[i], positions[i], sizeof(positions[i]));
        geometry->normals[i][2] = 1.0;
        geometry->colors[i] = (DvzColor){255, 255, 255, 255};
        geometry->indices[i] = i;
    }
    if (!lit)
    {
        dvz_free(geometry->normals);
        geometry->normals = NULL;
    }
    return geometry;
}


int test_scene_indexed_item_state_buffer_copies_and_updates_atomically(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);

    uint32_t initial[] = {DVZ_ITEM_STATE_ACTIVE, DVZ_ITEM_STATE_FILTERED, DVZ_ITEM_STATE_DISABLED};
    DvzItemStateBufferDesc desc = dvz_item_state_buffer_desc();
    desc.slot_count = 3;
    desc.words = initial;
    DvzItemStateBuffer* bank = dvz_item_state_buffer(scene, &desc);
    AT(bank != NULL);
    AT(((uint32_t*)bank->words->data)[0] == DVZ_ITEM_STATE_ACTIVE);

    // The bank owns its initial words after construction returns.
    initial[0] = DVZ_ITEM_STATE_NONE;
    AT(((uint32_t*)bank->words->data)[0] == DVZ_ITEM_STATE_ACTIVE);

    uint32_t slots[] = {0, 2};
    uint32_t words[] = {DVZ_ITEM_STATE_SELECTED, DVZ_ITEM_STATE_LINKED};
    AT(dvz_item_state_buffer_update(bank, slots, words, 2) == DVZ_OK);
    words[0] = DVZ_ITEM_STATE_NONE;
    AT(((uint32_t*)bank->words->data)[0] == DVZ_ITEM_STATE_SELECTED);
    AT(((uint32_t*)bank->words->data)[2] == DVZ_ITEM_STATE_LINKED);

    // A bad later slot rejects the complete update and leaves earlier slots intact.
    uint32_t invalid_slots[] = {1, 3};
    uint32_t invalid_words[] = {DVZ_ITEM_STATE_ACTIVE, DVZ_ITEM_STATE_NONE};
    AT_EXPECTED_ERROR_STRICT(
        suite, dvz_item_state_buffer_update(bank, invalid_slots, invalid_words, 2) == DVZ_ERROR);
    AT(((uint32_t*)bank->words->data)[1] == DVZ_ITEM_STATE_FILTERED);

    uint32_t unknown_word[] = {1u << 31};
    uint32_t slot_one[] = {1};
    AT_EXPECTED_ERROR_STRICT(
        suite, dvz_item_state_buffer_update(bank, slot_one, unknown_word, 1) == DVZ_ERROR);
    AT(((uint32_t*)bank->words->data)[1] == DVZ_ITEM_STATE_FILTERED);

    // Empty updates are valid no-ops, and duplicate slots deterministically keep the last word.
    AT(dvz_item_state_buffer_update(bank, NULL, NULL, 0) == DVZ_OK);
    uint32_t duplicate_slots[] = {1, 1};
    uint32_t duplicate_words[] = {DVZ_ITEM_STATE_ACTIVE, DVZ_ITEM_STATE_DISABLED};
    AT(dvz_item_state_buffer_update(bank, duplicate_slots, duplicate_words, 2) == DVZ_OK);
    AT(((uint32_t*)bank->words->data)[1] == DVZ_ITEM_STATE_DISABLED);

    dvz_item_state_buffer_destroy(bank);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_indexed_item_state_buffer_rejects_invalid_cardinality(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzItemStateBufferDesc desc = dvz_item_state_buffer_desc();
    uint32_t word = DVZ_ITEM_STATE_NONE;
    desc.slot_count = 0;
    desc.words = &word;
    AT(dvz_item_state_buffer(scene, &desc) == NULL);
    desc.slot_count = 1;
    word = 1u << 31;
    AT(dvz_item_state_buffer(scene, &desc) == NULL);
    desc.slot_count = 1;
    desc.words = NULL;
    DvzItemStateBuffer* bank = dvz_item_state_buffer(scene, &desc);
    AT(bank != NULL);
    AT(((uint32_t*)bank->words->data)[0] == DVZ_ITEM_STATE_NONE);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_indexed_item_state_mesh_binding_copies_and_unbinds(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);

    DvzVisual* mesh = dvz_mesh(scene, 0);
    AT(mesh != NULL);
    DvzGeometry* geometry = _indexed_state_triangle(true);
    AT(geometry != NULL);
    AT(dvz_mesh_set_geometry(mesh, geometry) == DVZ_OK);
    dvz_geometry_destroy(geometry);
    AT(dvz_panel_add_visual(panel, mesh, NULL) == DVZ_OK);

    uint32_t initial[] = {DVZ_ITEM_STATE_NONE, DVZ_ITEM_STATE_NONE};
    DvzItemStateBufferDesc desc = dvz_item_state_buffer_desc();
    desc.slot_count = 2;
    desc.words = initial;
    DvzItemStateBuffer* bank = dvz_item_state_buffer(scene, &desc);
    AT(bank != NULL);
    uint32_t state_indices[] = {0, 1, 0};
    AT(dvz_visual_set_item_state_buffer(mesh, bank, state_indices, 3) == DVZ_OK);
    state_indices[0] = 1;
    DvzVisualAttr* retained = &mesh->attrs[_attr_index(mesh, "state_index")];
    AT(((uint32_t*)retained->data)[0] == 0);
    uint64_t material_version = mesh->material.version;
    uint64_t query_revision = figure->query_input_revision;
    AT(dvz_visual_set_alpha_mode(mesh, DVZ_ALPHA_BLENDED) == DVZ_OK);
    AT(dvz_visual_set_alpha_mode(mesh, DVZ_ALPHA_WBOIT) == DVZ_OK);
    AT(dvz_visual_set_alpha_mode(mesh, DVZ_ALPHA_OPAQUE) == DVZ_OK);
    AT(mesh->material.version == material_version);
    AT(figure->query_input_revision == query_revision);
    uint32_t replacement_indices[] = {1, 1, 1};
    AT_EXPECTED_ERROR_STRICT(
        suite, dvz_visual_set_data(mesh, "state_index", replacement_indices, 3) == DVZ_ERROR);
    AT(((uint32_t*)retained->data)[0] == 0);
    AT_EXPECTED_ERROR_STRICT(
        suite, dvz_visual_set_data(mesh, "item_state", replacement_indices, 3) == DVZ_ERROR);

    uint32_t out_of_bounds[] = {0, 2, 0};
    AT(dvz_visual_set_item_state_buffer(mesh, bank, out_of_bounds, 3) == DVZ_ERROR);
    AT(((uint32_t*)retained->data)[0] == 0);
    AT_EXPECTED_ERROR_STRICT(
        suite, dvz_visual_set_item_state_buffer(mesh, bank, state_indices, 2) == DVZ_ERROR);

    DvzVisual* authored_mesh = dvz_mesh(scene, 0);
    AT(authored_mesh != NULL);
    geometry = _indexed_state_triangle(true);
    AT(geometry != NULL);
    AT(dvz_mesh_set_geometry(authored_mesh, geometry) == DVZ_OK);
    dvz_geometry_destroy(geometry);
    uint32_t dense_state[] = {DVZ_ITEM_STATE_ACTIVE, DVZ_ITEM_STATE_NONE, DVZ_ITEM_STATE_NONE};
    AT(dvz_visual_set_data(authored_mesh, "item_state", dense_state, 3) == DVZ_OK);
    AT_EXPECTED_ERROR_STRICT(
        suite,
        dvz_visual_set_item_state_buffer(authored_mesh, bank, state_indices, 3) == DVZ_ERROR);

    DvzVisual* instanced_mesh = dvz_mesh(scene, 0);
    AT(instanced_mesh != NULL);
    geometry = _indexed_state_triangle(true);
    AT(geometry != NULL);
    AT(dvz_mesh_set_geometry(instanced_mesh, geometry) == DVZ_OK);
    dvz_geometry_destroy(geometry);
    float transform[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    AT(dvz_visual_set_data(instanced_mesh, "instance_transform", transform, 1) == DVZ_OK);
    AT_EXPECTED_ERROR_STRICT(
        suite,
        dvz_visual_set_item_state_buffer(instanced_mesh, bank, state_indices, 3) == DVZ_ERROR);

    AT(dvz_visual_set_alpha_mode(mesh, DVZ_ALPHA_MASK) == DVZ_OK);
    AT(mesh->material.version > material_version);
    AT(figure->query_input_revision > query_revision);

    dvz_item_state_buffer_destroy(bank);
    AT(_visual_family_state(mesh)->item_state_bank == NULL);
    int state_index_attr = _attr_index(mesh, "state_index");
    AT(state_index_attr >= 0);
    AT(mesh->attrs[state_index_attr].data == NULL);
    AT(mesh->attrs[state_index_attr].item_count == 0);
    dvz_scene_destroy(scene);
    return 0;
}


static int _indexed_state_emit_and_check_sparse(
    TstContext* suite, DvzFigure* figure, DvzItemStateBuffer* bank, uint32_t expected_writes)
{
    ANN(suite);
    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream != NULL);

    char key[DVZ_SCENE_LABEL_SIZE] = {0};
    AT(dvz_scene_buffer_resource_key(bank->words, key, sizeof(key)));
    uint64_t bank_id = dvz_drp2_stream_label_id(stream, key);
    if (expected_writes > 0)
        AT(bank_id != 0);

    uint32_t writes = 0;
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(stream, i);
        ANN(command);
        if (command->type != DVZ_DRP2_COMMAND_WRITE_BUFFER ||
            command->u.write_buffer.buffer_id != bank_id)
            continue;
        writes++;
        AT(command->u.write_buffer.data_raw != NULL);
        if (expected_writes == 1)
        {
            AT(command->u.write_buffer.offset == 0);
            AT(command->u.write_buffer.size == (uint64_t)bank->slot_count * sizeof(uint32_t));
        }
        else
        {
            AT(command->u.write_buffer.size == sizeof(uint32_t));
            uint32_t slot = (uint32_t)(command->u.write_buffer.offset / sizeof(uint32_t));
            const uint32_t* word = (const uint32_t*)command->u.write_buffer.data_raw;
            AT((slot == 11 && *word == DVZ_ITEM_STATE_ACTIVE) ||
               (slot == 4000 && *word == DVZ_ITEM_STATE_SELECTED));
        }
    }
    AT(writes == expected_writes);
    _test_scene_stream_destroy(stream);
    return 0;
}


int test_scene_indexed_item_state_sparse_updates_reach_each_figure(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figures[2] = {dvz_figure(scene, 64, 64, 0), dvz_figure(scene, 64, 64, 0)};
    AT(figures[0] != NULL && figures[1] != NULL);
    DvzPanel* panels[2] = {
        dvz_panel(figures[0], &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f}),
        dvz_panel(figures[1], &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f}),
    };
    AT(panels[0] != NULL && panels[1] != NULL);

    DvzItemStateBufferDesc desc = dvz_item_state_buffer_desc();
    desc.slot_count = 4096;
    DvzItemStateBuffer* bank = dvz_item_state_buffer(scene, &desc);
    AT(bank != NULL);
    uint32_t state_indices[3] = {0, 11, 4000};
    DvzGeometry* geometry = _indexed_state_triangle(true);
    AT(geometry != NULL);

    DvzVisual* meshes[2] = {0};
    for (uint32_t i = 0; i < 2; i++)
    {
        meshes[i] = dvz_mesh(scene, 0);
        AT(meshes[i] != NULL);
        AT(dvz_mesh_set_geometry(meshes[i], geometry) == DVZ_OK);
        AT(dvz_visual_set_item_state_buffer(meshes[i], bank, state_indices, 3) == DVZ_OK);
        AT(dvz_panel_add_visual(panels[i], meshes[i], NULL) == DVZ_OK);
    }
    dvz_geometry_destroy(geometry);

    // Realize each consumer's baseline before changing two widely separated slots.
    AT(_indexed_state_emit_and_check_sparse(suite, figures[0], bank, 1) == 0);
    AT(_indexed_state_emit_and_check_sparse(suite, figures[1], bank, 1) == 0);
    uint64_t query_revision[2] = {
        figures[0]->query_input_revision, figures[1]->query_input_revision};

    DvzSelectionVisualStyle selection_style = dvz_selection_visual_style();
    DvzItemStateVisualStyle hover_style = dvz_item_state_visual_style();
    hover_style.visual_flags = DVZ_ITEM_STATE_VISUAL_SCALE;
    uint64_t style_revision = bank->style->content_revision;
    AT_EXPECTED_ERROR_STRICT(
        suite,
        dvz_item_state_buffer_style(bank, &selection_style, &hover_style, true) == DVZ_ERROR);
    AT(bank->style->content_revision == style_revision);
    hover_style.visual_flags = DVZ_ITEM_STATE_VISUAL_ALPHA;
    hover_style.alpha = 0.25f;
    AT(dvz_item_state_buffer_style(bank, &selection_style, &hover_style, true) == DVZ_OK);
    AT(bank->style->content_revision > style_revision);
    DvzSceneItemStateStyleParams saved_style = *(DvzSceneItemStateStyleParams*)bank->style->data;
    uint64_t saved_revision = bank->style->content_revision;
    bank->style->content_revision = UINT64_MAX;
    hover_style.alpha = 0.5f;
    AT(dvz_item_state_buffer_style(bank, &selection_style, &hover_style, true) == DVZ_ERROR);
    AT(memcmp(bank->style->data, &saved_style, sizeof(saved_style)) == 0);
    bank->style->content_revision = saved_revision;
    AT(figures[0]->query_input_revision == query_revision[0]);
    AT(figures[1]->query_input_revision == query_revision[1]);

    uint32_t slots[2] = {11, 4000};
    uint32_t words[2] = {DVZ_ITEM_STATE_ACTIVE, DVZ_ITEM_STATE_SELECTED};
    AT(dvz_item_state_buffer_update(bank, slots, words, 2) == DVZ_OK);
    AT(figures[0]->query_input_revision == query_revision[0]);
    AT(figures[1]->query_input_revision == query_revision[1]);

    // A bounded append failure must leave the consumer's word revision unconsumed.
    uint32_t bank_index = _scene_buffer_index(scene, bank->words);
    AT(bank_index < DVZ_SCENE_MAX_BUFFERS);
    uint64_t consumed = figures[0]->item_state_consumed[bank_index];
    DvzFramePlan* failed_plan = dvz_frame_plan("indexed.item_state.failure", 0);
    AT(failed_plan != NULL);
    uint32_t original_count = failed_plan->count;
    uint32_t original_capacity = failed_plan->capacity;
    failed_plan->count = UINT32_MAX;
    failed_plan->capacity = UINT32_MAX;
    DvzDiagnosticReport failure_report;
    dvz_diagnostic_report_init(&failure_report);
    AT_EXPECTED_ERROR_STRICT(
        suite, !_scene_emit_visual_uploads(figures[0], failed_plan, &failure_report));
    failed_plan->count = original_count;
    failed_plan->capacity = original_capacity;
    dvz_frame_plan_destroy(failed_plan);
    AT(figures[0]->item_state_consumed[bank_index] == consumed);

    // Emitting consumers in either order must upload both sparse words to each one.
    AT(dvz_visual_set_visible(meshes[1], false) == DVZ_OK);
    AT(_indexed_state_emit_and_check_sparse(suite, figures[1], bank, 0) == 0);
    AT(_indexed_state_emit_and_check_sparse(suite, figures[1], bank, 0) == 0);
    AT(!_scene_figure_has_pending_render_work(figures[1]));
    AT(dvz_visual_set_visible(meshes[1], true) == DVZ_OK);
    AT(_indexed_state_emit_and_check_sparse(suite, figures[1], bank, 2) == 0);
    AT(_indexed_state_emit_and_check_sparse(suite, figures[0], bank, 2) == 0);
    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Reject the scalar-volume transfer staging allocation while preserving normal test allocations.
 *
 * @param count allocation item count
 * @param size allocation item size
 * @return allocated storage, or NULL for transfer staging
 */
static void* _indexed_state_reject_transfer_calloc(DvzSize count, DvzSize size)
{
    if (count == 1024 && size == 1)
        return NULL;
    return dvz_system_allocator()->calloc_fn(count, size);
}



/**
 * Retry state-bank updates when later volume transfer preparation aborts the whole frame.
 *
 * @param suite test context
 * @param item test descriptor
 * @return zero on success
 */
int test_scene_indexed_item_state_retries_after_volume_preparation_failure(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;
    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0, 0, 1, 1});
    AT(panel != NULL);
    DvzItemStateBufferDesc desc = dvz_item_state_buffer_desc();
    desc.slot_count = 3;
    DvzItemStateBuffer* bank = dvz_item_state_buffer(scene, &desc);
    AT(bank != NULL);
    DvzGeometry* geometry = _indexed_state_triangle(true);
    AT(geometry != NULL);
    DvzVisual* mesh = dvz_mesh(scene, 0);
    AT(mesh != NULL);
    AT(dvz_mesh_set_geometry(mesh, geometry) == DVZ_OK);
    dvz_geometry_destroy(geometry);
    const uint32_t state_indices[] = {0, 1, 2};
    AT(dvz_visual_set_item_state_buffer(mesh, bank, state_indices, 3) == DVZ_OK);
    AT(dvz_panel_add_visual(panel, mesh, NULL) == DVZ_OK);

    // The bank uploads precede the volume, whose later preparation failure must abort both.
    DvzVisual* volume = dvz_volume(scene, 0);
    AT(volume != NULL);
    DvzSampledFieldDesc field_desc = dvz_sampled_field_desc();
    field_desc.dim = DVZ_FIELD_DIM_3D;
    field_desc.format = DVZ_FIELD_FORMAT_R8_UNORM;
    field_desc.semantic = DVZ_FIELD_SEMANTIC_SCALAR;
    field_desc.color_role = DVZ_COLOR_ROLE_DATA;
    field_desc.width = field_desc.height = field_desc.depth = 2;
    DvzSampledField* field = dvz_sampled_field(scene, &field_desc);
    AT(field != NULL);
    const uint8_t values[] = {0, 64, 128, 255, 32, 96, 160, 224};
    DvzFieldDataView data = {DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView),
                           .data = values, .bytes_per_row = 2, .rows_per_image = 2};
    AT(dvz_sampled_field_set_data(field, &data) == DVZ_OK);
    AT(dvz_visual_set_field(volume, "field", field) == DVZ_OK);
    AT(dvz_volume_set_render_mode(volume, DVZ_VOLUME_RENDER_COMPOSITE) == DVZ_OK);
    AT(dvz_panel_add_visual(panel, volume, NULL) == DVZ_OK);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
    AT(stream != NULL);
    AT(dvz_diagnostic_report_count(&report) == 0);
    _test_scene_stream_destroy(stream);

    uint32_t bank_index = _scene_buffer_index(scene, bank->words);
    AT(bank_index < DVZ_SCENE_MAX_BUFFERS);
    const uint64_t consumed = figure->item_state_consumed[bank_index];
    const uint64_t consumed_id = figure->item_state_consumed_ids[bank_index];
    AT(consumed == bank->words->content_revision);
    const uint32_t slot = 1;
    const uint32_t selected = DVZ_ITEM_STATE_SELECTED;
    AT(dvz_item_state_buffer_update(bank, &slot, &selected, 1) == DVZ_OK);
    AT(bank->words->content_revision > consumed);
    const DvzVolumeAlphaStop alpha[] = {
        {.position = 0, .alpha = 0.5f}, {.position = 1, .alpha = 0.5f}};
    AT(dvz_volume_set_alpha_stops(volume, alpha, 2) == DVZ_OK);
    const uint64_t realized = _visual_family_state(volume)->volume_realized_version;
    dvz_free(_visual_family_state(volume)->texture.rgba);
    _visual_family_state(volume)->texture.rgba = NULL;
    _visual_family_state(volume)->texture.rgba_size = 0;

    const DvzAllocator* saved_allocator = dvz_get_allocator();
    DvzAllocator failing_allocator = *saved_allocator;
    failing_allocator.calloc_fn = _indexed_state_reject_transfer_calloc;
    tst_expect_error_begin(suite);
    dvz_set_allocator(&failing_allocator);
    stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
    dvz_set_allocator(saved_allocator);
    const int expected_error = tst_expect_error_end(suite);
    AT(expected_error == 0);
    AT(stream == NULL);
    AT(dvz_diagnostic_report_count(&report) > 0);
    AT(figure->item_state_consumed[bank_index] == consumed);
    AT(figure->item_state_consumed_ids[bank_index] == consumed_id);
    AT(_visual_family_state(volume)->volume_realized_version == realized);
    // Alpha changes invalidate the transfer through its version, independently of source texture.
    AT(_visual_family_state(volume)->volume_realized_version !=
       _visual_family_state(volume)->volume.version);

    char bank_key[DVZ_SCENE_LABEL_SIZE] = {0};
    AT(dvz_scene_buffer_resource_key(bank->words, bank_key, sizeof(bank_key)));
    for (uint32_t attempt = 0; attempt < 2; attempt++)
    {
        dvz_diagnostic_report_init(&report);
        stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
        AT(stream != NULL);
        AT(dvz_diagnostic_report_count(&report) == 0);
        const uint64_t bank_id = dvz_drp2_stream_label_id(stream, bank_key);
        uint32_t bank_writes = 0, transfer_writes = 0;
        for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
        {
            const DvzDrp2Command* command = dvz_drp2_stream_get(stream, i);
            if (command->type == DVZ_DRP2_COMMAND_WRITE_BUFFER &&
                command->u.write_buffer.buffer_id == bank_id)
            {
                bank_writes++;
                AT(command->u.write_buffer.offset == sizeof(uint32_t));
                AT(command->u.write_buffer.size == sizeof(uint32_t));
                const uint32_t* word = command->u.write_buffer.data_raw;
                AT(word != NULL && *word == selected);
            }
            if (command->type == DVZ_DRP2_COMMAND_WRITE_TEXTURE &&
                command->u.write_texture.width == 256 &&
                command->u.write_texture.height == 1 && command->u.write_texture.depth == 1)
            {
                transfer_writes++;
                const uint8_t* rgba = command->u.write_texture.data_raw;
                AT(rgba != NULL && rgba[3] == 128 && rgba[4 * 255 + 3] == 128);
            }
        }
        AT(bank_writes == (attempt == 0 ? 1u : 0u));
        AT(transfer_writes == (attempt == 0 ? 1u : 0u));
        AT(figure->item_state_consumed[bank_index] == bank->words->content_revision);
        _test_scene_stream_destroy(stream);
    }
    dvz_scene_destroy(scene);
    return 0;
}



static int _indexed_state_profile_emits(
    TstContext* suite, bool lit, DvzAlphaMode alpha_mode, DvzSceneShaderFormat shader_format)
{
    DvzScene* scene = dvz_scene();
    AT(scene != NULL);
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    AT(figure != NULL);
    DvzPanel* panel = dvz_panel(figure, &(DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});
    AT(panel != NULL);

    DvzItemStateBufferDesc bank_desc = dvz_item_state_buffer_desc();
    bank_desc.slot_count = 4;
    DvzItemStateBuffer* bank = dvz_item_state_buffer(scene, &bank_desc);
    AT(bank != NULL);
    DvzVisual* mesh = dvz_mesh(scene, 0);
    AT(mesh != NULL);
    DvzGeometry* geometry = _indexed_state_triangle(lit);
    AT(geometry != NULL);
    AT(dvz_mesh_set_geometry(mesh, geometry) == DVZ_OK);
    dvz_geometry_destroy(geometry);
    uint32_t state_indices[3] = {0, 1, 2};
    AT(dvz_visual_set_item_state_buffer(mesh, bank, state_indices, 3) == DVZ_OK);
    AT(dvz_visual_set_alpha_mode(mesh, alpha_mode) == DVZ_OK);
    AT(dvz_panel_add_visual(panel, mesh, NULL) == DVZ_OK);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.max_color_attachments = 3;
    caps.render_target_format_rgba16float = true;
    caps.render_target_format_r16float = true;
    caps.supports_render_target_sampling = true;
    caps.supports_color_blending = true;
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
    cfg.shader_format = shader_format;
    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
    AT(dvz_diagnostic_report_count(&report) == 0);
    AT(stream != NULL);

    bool found_pipeline = false;
    bool found_style_layout = false;
    bool found_words_binding = false;
    bool found_vertex_shader = false;
    char words_key[DVZ_SCENE_LABEL_SIZE] = {0};
    AT(dvz_scene_buffer_resource_key(bank->words, words_key, sizeof(words_key)));
    uint64_t words_id = dvz_drp2_stream_label_id(stream, words_key);
    AT(words_id != 0);

    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(stream, i);
        ANN(command);
        if (command->type == DVZ_DRP2_COMMAND_CREATE_SHADER_MODULE &&
            strcmp(command->u.create_shader_module.stage, "VERTEX") == 0 &&
            strcmp(command->u.create_shader_module.builtin_family, "scene.primitive") == 0)
        {
            const char* expected_variant =
                alpha_mode == DVZ_ALPHA_WBOIT
                    ? (lit ? "wboit_lit_indexed_item_state" : "wboit_indexed_item_state")
                    : (lit ? "lit_indexed_item_state" : "indexed_item_state");
            AT(strcmp(command->u.create_shader_module.builtin_variant, expected_variant) == 0);
            found_vertex_shader = true;
            AT(command->u.create_shader_module.code != NULL ||
               command->u.create_shader_module.spirv != NULL);
        }
        if (command->type == DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
        {
            const char* label =
                dvz_drp2_stream_label(stream, command->u.create_render_pipeline.id);
            if (label == NULL || strstr(label, "_pipe_prim_indexed_item_state") == NULL)
                continue;
            found_pipeline = true;
            AT(command->u.create_render_pipeline.binding_count >= 3);
            AT(command->u.create_render_pipeline.attr_count >= 3);
            bool found_state_attr = false;
            for (uint32_t ai = 0; ai < command->u.create_render_pipeline.attr_count; ai++)
                if (command->u.create_render_pipeline.attr_locations[ai] == 7)
                {
                    uint32_t binding = command->u.create_render_pipeline.attr_bindings[ai];
                    AT(command->u.create_render_pipeline.attr_formats[ai] == DVZ_FORMAT_R32_UINT);
                    AT(binding + 1 == command->u.create_render_pipeline.binding_count);
                    AT(command->u.create_render_pipeline.binding_step_modes[binding] ==
                       DVZ_DRP2_VERTEX_STEP_MODE_VERTEX);
                    AT(command->u.create_render_pipeline.binding_strides[binding] ==
                       sizeof(uint32_t));
                    found_state_attr = true;
                }
            AT(found_state_attr);
            AT(command->u.create_render_pipeline.bind_group_layout_count >= 2);
            uint64_t style_layout_id = command->u.create_render_pipeline.bind_group_layout_ids[1];
            for (uint32_t j = 0; j < dvz_drp2_stream_count(stream); j++)
            {
                const DvzDrp2Command* layout = dvz_drp2_stream_get(stream, j);
                if (layout->type == DVZ_DRP2_COMMAND_CREATE_BIND_GROUP_LAYOUT &&
                    layout->u.create_bind_group_layout.id == style_layout_id)
                {
                    for (uint32_t ei = 0; ei < layout->u.create_bind_group_layout.entry_count;
                         ei++)
                        if (layout->u.create_bind_group_layout.entries[ei].binding ==
                            DVZ_SCENE_SHADER_BINDING_ITEM_STATE_WORDS)
                        {
                            AT(layout->u.create_bind_group_layout.entries[ei].binding_type ==
                               DVZ_DRP2_BINDING_TYPE_STORAGE_BUFFER);
                            AT(layout->u.create_bind_group_layout.entries[ei].visibility ==
                               DVZ_DRP2_SHADER_STAGE_VERTEX);
                            found_style_layout = true;
                        }
                }
                if (layout->type == DVZ_DRP2_COMMAND_CREATE_BIND_GROUP &&
                    layout->u.create_bind_group.bind_group_layout_id == style_layout_id)
                {
                    for (uint32_t ei = 0; ei < layout->u.create_bind_group.entry_count; ei++)
                        if (layout->u.create_bind_group.entries[ei].binding ==
                            DVZ_SCENE_SHADER_BINDING_ITEM_STATE_WORDS)
                        {
                            AT(layout->u.create_bind_group.entries[ei].resource_id == words_id);
                            AT(layout->u.create_bind_group.entries[ei].size ==
                               (uint64_t)bank->slot_count * sizeof(uint32_t));
                            found_words_binding = true;
                        }
                }
            }
        }
    }
    AT(found_pipeline);
    AT(found_style_layout);
    AT(found_words_binding);
    AT(found_vertex_shader);
    _test_scene_stream_destroy(stream);
    dvz_scene_destroy(scene);
    return 0;
}


int test_scene_indexed_item_state_shader_profile_matrix(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;
    for (uint32_t lit = 0; lit < 2; lit++)
        for (uint32_t alpha_mode = 0; alpha_mode < 3; alpha_mode++)
            for (uint32_t wgsl = 0; wgsl < 2; wgsl++)
            {
                const DvzAlphaMode modes[] = {
                    DVZ_ALPHA_OPAQUE, DVZ_ALPHA_BLENDED, DVZ_ALPHA_WBOIT};
                DvzAlphaMode mode = modes[alpha_mode];
                DvzSceneShaderFormat format =
                    wgsl ? DVZ_SCENE_SHADER_FORMAT_WGSL : DVZ_SCENE_SHADER_FORMAT_GLSL;
                AT(_indexed_state_profile_emits(suite, lit != 0, mode, format) == 0);
            }
    return 0;
}


int test_scene_mesh_fragment_clip_contract(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;
    DvzScene* scene = dvz_scene();
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    DvzPanel* panel = dvz_panel_full(figure);
    DvzVisual* mesh = dvz_mesh(scene, 0);
    vec3 pos[6] = {{-.8f, -.8f, 0}, {.8f, -.8f, 0}, {0, .8f, 0},
                   {-.8f, -.8f, 0}, {.8f, -.8f, 0}, {0, .8f, 0}};
    DvzIndex indices[6] = {0, 1, 2, 3, 4, 5};
    vec2 clip[6] = {{.8f, 0}, {-.8f, 0}, {-0.f, 0}, {-.8f, 1}, {.8f, 1}, {0, 1}};
    AT(dvz_visual_set_data(mesh, "position", pos, 6) == DVZ_OK);
    AT(dvz_visual_set_index_data(mesh, indices, 6) == DVZ_OK);
    AT(dvz_panel_add_visual(panel, mesh, NULL) == DVZ_OK);
    uint64_t query_version = figure->query_input_revision;
    AT(dvz_mesh_set_fragment_clip(mesh, (const vec2*)clip, 6) == DVZ_OK);
    AT(figure->query_input_revision > query_version);
    int attr_idx = _attr_index(mesh, "fragment_clip");
    AT(attr_idx >= 0);
    DvzVisualAttr* attr = &mesh->attrs[attr_idx];
    uint64_t version = attr->version;
    clip[0][0] = 9;
    AT(((vec2*)attr->data)[0][0] == .8f);
    clip[1][1] = 1;
    AT(dvz_mesh_set_fragment_clip(mesh, (const vec2*)clip, 6) == DVZ_ERROR);
    AT(attr->version == version);
    clip[1][1] = 0;
    clip[0][0] = NAN;
    AT(dvz_mesh_set_fragment_clip(mesh, (const vec2*)clip, 6) == DVZ_ERROR);
    clip[0][0] = 1;
    clip[0][1] = .5f;
    AT(dvz_mesh_set_fragment_clip(mesh, (const vec2*)clip, 6) == DVZ_ERROR);
    AT(dvz_mesh_set_fragment_clip(mesh, (const vec2*)clip, 5) == DVZ_ERROR);
    AT_EXPECTED_ERROR_STRICT(
        suite, dvz_visual_set_data(mesh, "fragment_clip", clip, 6) == DVZ_ERROR);
    uint32_t dense = 0;
    AT_EXPECTED_ERROR_STRICT(
        suite, dvz_visual_set_data(mesh, "item_state", &dense, 1) == DVZ_ERROR);
    AT(_scene_mesh_fragment_clip_error(mesh) == NULL);
    indices[2] = 3;
    AT(dvz_visual_set_index_data(mesh, indices, 6) == DVZ_OK);
    AT(_scene_mesh_fragment_clip_error(mesh) != NULL);
    indices[2] = 2;
    AT(dvz_visual_set_index_data(mesh, indices, 6) == DVZ_OK);
    AT(_scene_mesh_fragment_clip_error(mesh) == NULL);
    // A new u16 buffer identity is revalidated even when all counts remain unchanged.
    DvzSceneBufferDesc index_desc = dvz_scene_buffer_desc();
    index_desc.usage = DVZ_SCENE_BUFFER_USAGE_INDEX;
    index_desc.stride = sizeof(uint16_t);
    DvzSceneBuffer* narrow = dvz_scene_buffer(scene, &index_desc);
    uint16_t narrow_data[6] = {0, 1, 2, 3, 4, 5};
    AT(dvz_scene_buffer_set_data(narrow, narrow_data, sizeof(narrow_data)) == DVZ_OK);
    AT(dvz_visual_set_buffer(mesh, "index", narrow) == DVZ_OK);
    AT(_scene_mesh_fragment_clip_error(mesh) == NULL);
    narrow_data[2] = 3;
    AT(dvz_scene_buffer_set_data(narrow, narrow_data, sizeof(narrow_data)) == DVZ_OK);
    AT(_scene_mesh_fragment_clip_error(mesh) != NULL);
    narrow_data[2] = 2;
    AT(dvz_scene_buffer_set_data(narrow, narrow_data, sizeof(narrow_data)) == DVZ_OK);
    AT(_scene_mesh_fragment_clip_error(mesh) == NULL);
    AT(dvz_visual_set_alpha_mode(mesh, DVZ_ALPHA_MASK) == DVZ_OK);
    AT(_scene_mesh_fragment_clip_error(mesh) != NULL);
    AT(dvz_visual_set_alpha_mode(mesh, DVZ_ALPHA_OPAQUE) == DVZ_OK);
    AT(_scene_mesh_fragment_clip_error(mesh) == NULL);
    query_version = figure->query_input_revision;
    AT(dvz_mesh_set_fragment_clip(mesh, NULL, 0) == DVZ_OK);
    AT(attr->data == NULL && attr->item_count == 0);
    AT(figure->query_input_revision > query_version);
    AT(dvz_visual_set_data(mesh, "item_state", &dense, 1) == DVZ_OK);
    dvz_scene_destroy(scene);
    return 0;
}

int test_scene_mesh_fragment_clip_shader_matrix(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;
    DvzAlphaMode modes[] = {DVZ_ALPHA_OPAQUE, DVZ_ALPHA_BLENDED, DVZ_ALPHA_WBOIT};
    for (uint32_t bank_bound = 0; bank_bound < 2; bank_bound++)
        for (uint32_t lit = 0; lit < 2; lit++)
            for (uint32_t mode = 0; mode < 3; mode++)
                for (uint32_t wgsl = 0; wgsl < 2; wgsl++)
                {
                    DvzScene* scene = dvz_scene();
                    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
                    DvzPanel* panel = dvz_panel_full(figure);
                    DvzVisual* mesh = dvz_mesh(scene, 0);
                    DvzGeometry* geometry = _indexed_state_triangle(lit != 0);
                    AT(geometry != NULL);
                    AT(dvz_mesh_set_geometry(mesh, geometry) == DVZ_OK);
                    dvz_geometry_destroy(geometry);
                    vec2 clip[3] = {{-1, 1}, {1, 1}, {0, 1}};
                    AT(dvz_mesh_set_fragment_clip(mesh, (const vec2*)clip, 3) == DVZ_OK);
                    if (bank_bound)
                    {
                        DvzItemStateBufferDesc desc = dvz_item_state_buffer_desc();
                        desc.slot_count = 2;
                        DvzItemStateBuffer* bank = dvz_item_state_buffer(scene, &desc);
                        uint32_t slots[3] = {0, 0, 0};
                        AT(dvz_visual_set_item_state_buffer(mesh, bank, slots, 3) == DVZ_OK);
                    }
                    AT(dvz_visual_set_alpha_mode(mesh, modes[mode]) == DVZ_OK);
                    AT(dvz_panel_add_visual(panel, mesh, NULL) == DVZ_OK);
                    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
                    caps.max_color_attachments = 3;
                    caps.render_target_format_rgba16float = true;
                    caps.render_target_format_r16float = true;
                    caps.supports_render_target_sampling = true;
                    caps.supports_color_blending = true;
                    DvzDiagnosticReport report;
                    dvz_diagnostic_report_init(&report);
                    DvzFramePlanEmitConfig cfg = dvz_frame_plan_emit_config();
                    cfg.shader_format =
                        wgsl ? DVZ_SCENE_SHADER_FORMAT_WGSL : DVZ_SCENE_SHADER_FORMAT_GLSL;
                    DvzDrp2CommandStream* stream =
                        _test_scene_emit_stream_ex(figure, &caps, &report, &cfg);
                    AT(dvz_diagnostic_report_count(&report) == 0);
                    AT(stream != NULL);
                    bool found = false;
                    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
                    {
                        const DvzDrp2Command* c = dvz_drp2_stream_get(stream, i);
                        if (c->type != DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE)
                            continue;
                        for (uint32_t ai = 0; ai < c->u.create_render_pipeline.attr_count; ai++)
                            if (c->u.create_render_pipeline.attr_locations[ai] == 8)
                            {
                                AT(c->u.create_render_pipeline.attr_formats[ai] ==
                                   DVZ_FORMAT_R32G32_SFLOAT);
                                uint32_t binding = c->u.create_render_pipeline.attr_bindings[ai];
                                AT(c->u.create_render_pipeline.binding_step_modes[binding] ==
                                   DVZ_DRP2_VERTEX_STEP_MODE_VERTEX);
                                found = true;
                            }
                    }
                    AT(found);
                    _test_scene_stream_destroy(stream);
                    dvz_scene_destroy(scene);
                }
    return 0;
}


int test_scene_mesh_fragment_clip_consumers_retry(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;
    for (uint32_t bank_bound = 0; bank_bound < 2; bank_bound++)
    {
        DvzScene* scene = dvz_scene();
        DvzVisual* mesh = dvz_mesh(scene, 0);
        DvzGeometry* geometry = _indexed_state_triangle(false);
        AT(dvz_mesh_set_geometry(mesh, geometry) == DVZ_OK);
        dvz_geometry_destroy(geometry);
        vec2 clip[3] = {{1, 0}, {-1, 0}, {0, 0}};
        AT(dvz_mesh_set_fragment_clip(mesh, (const vec2*)clip, 3) == DVZ_OK);
        DvzItemStateBufferDesc desc = dvz_item_state_buffer_desc();
        desc.slot_count = 2;
        DvzItemStateBuffer* bank = bank_bound ? dvz_item_state_buffer(scene, &desc) : NULL;
        uint32_t slots[3] = {0, 0, 0};
        if (bank_bound)
            AT(dvz_visual_set_item_state_buffer(mesh, bank, slots, 3) == DVZ_OK);
        DvzFigure* figures[2];
        DvzCapabilitySnapshot caps = dvz_capability_snapshot();
        for (uint32_t fi = 0; fi < 2; fi++)
        {
            figures[fi] = dvz_figure(scene, 64, 64, 0);
            AT(dvz_panel_add_visual(dvz_panel_full(figures[fi]), mesh, NULL) == DVZ_OK);
            DvzDiagnosticReport report;
            dvz_diagnostic_report_init(&report);
            DvzDrp2CommandStream* stream = _test_scene_emit_stream(figures[fi], &caps, &report);
            AT(stream != NULL && dvz_diagnostic_report_count(&report) == 0);
            _test_scene_stream_destroy(stream);
        }
        clip[0][0] = -1;
        clip[1][0] = 1;
        AT(dvz_mesh_set_fragment_clip(mesh, (const vec2*)clip, 3) == DVZ_OK);
        if (bank_bound)
        {
            slots[0] = slots[1] = slots[2] = 1;
            AT(dvz_visual_set_item_state_buffer(mesh, bank, slots, 3) == DVZ_OK);
        }
        uint64_t consumed = figures[0]->typed_attr_consumed[0][1];
        DvzFramePlan* failed = dvz_frame_plan("clip.failure", 0);
        uint32_t count = failed->count, capacity = failed->capacity;
        failed->count = failed->capacity = UINT32_MAX;
        DvzDiagnosticReport failure_report;
        dvz_diagnostic_report_init(&failure_report);
        AT_EXPECTED_ERROR_STRICT(
            suite, !_scene_emit_visual_uploads(figures[0], failed, &failure_report));
        failed->count = count;
        failed->capacity = capacity;
        dvz_frame_plan_destroy(failed);
        AT(figures[0]->typed_attr_consumed[0][1] == consumed);
        for (uint32_t fi = 0; fi < 2; fi++)
        {
            AT(_scene_figure_has_pending_render_work(figures[fi]));
            DvzDiagnosticReport report;
            dvz_diagnostic_report_init(&report);
            DvzDrp2CommandStream* stream = _test_scene_emit_stream(figures[fi], &caps, &report);
            AT(stream != NULL && dvz_diagnostic_report_count(&report) == 0);
            uint64_t clip_id = dvz_drp2_stream_label_id(stream, "v0_fragment_clip");
            uint64_t indices_id = dvz_drp2_stream_label_id(stream, "v0_state_index");
            uint32_t clips = 0, indices = 0;
            for (uint32_t ci = 0; ci < dvz_drp2_stream_count(stream); ci++)
            {
                const DvzDrp2Command* c = dvz_drp2_stream_get(stream, ci);
                if (c->type == DVZ_DRP2_COMMAND_WRITE_BUFFER &&
                    c->u.write_buffer.buffer_id == clip_id)
                {
                    AT(c->u.write_buffer.size == sizeof(clip));
                    clips++;
                }
                if (c->type == DVZ_DRP2_COMMAND_WRITE_BUFFER &&
                    c->u.write_buffer.buffer_id == indices_id)
                {
                    AT(c->u.write_buffer.size == sizeof(slots));
                    indices++;
                }
            }
            AT(clips == 1 && indices == bank_bound);
            _test_scene_stream_destroy(stream);
            AT(!_scene_figure_has_pending_render_work(figures[fi]));
        }
        AT(dvz_mesh_set_fragment_clip(mesh, NULL, 0) == DVZ_OK);
        for (uint32_t fi = 0; fi < 2; fi++)
        {
            DvzDiagnosticReport report;
            dvz_diagnostic_report_init(&report);
            DvzDrp2CommandStream* stream = _test_scene_emit_stream(figures[fi], &caps, &report);
            AT(stream != NULL && dvz_diagnostic_report_count(&report) == 0);
            _test_scene_stream_destroy(stream);
        }
        dvz_scene_destroy(scene);
    }
    return 0;
}
