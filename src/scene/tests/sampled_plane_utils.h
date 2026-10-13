#pragma once

#include "datoviz/scene.h"

/**
 * Create an indexed unlit plane with four nearest-sampled linear-color texels.
 *
 * @param scene scene owning the visual and resources
 * @param pixels four RGBA texels, copied by the sampled field
 * @param depth data-space plane depth
 * @param mode alpha mode
 * @return created plane, or NULL on setup failure
 */
static inline DvzVisual* _test_sampled_plane(
    DvzScene* scene, const uint8_t pixels[16], float depth, DvzAlphaMode mode)
{
    DvzVisual* visual = dvz_mesh(scene, 0);
    if (visual == NULL)
        return NULL;
    vec3 positions[4] = {
        {-0.9f, -0.9f, depth}, {-0.9f, 0.9f, depth},
        {0.9f, -0.9f, depth}, {0.9f, 0.9f, depth},
    };
    vec3 normals[4] = {{0, 0, 1}, {0, 0, 1}, {0, 0, 1}, {0, 0, 1}};
    vec2 uv[4] = {{0, 0}, {0, 1}, {1, 0}, {1, 1}};
    DvzColor colors[4] = {
        {255, 255, 255, 255}, {255, 255, 255, 255},
        {255, 255, 255, 255}, {255, 255, 255, 255},
    };
    DvzIndex indices[6] = {0, 1, 2, 2, 1, 3};
    DvzSceneBufferDesc buffer_desc = dvz_scene_buffer_desc();
    buffer_desc.usage = DVZ_SCENE_BUFFER_USAGE_INDEX;
    buffer_desc.stride = sizeof(DvzIndex);
    DvzSceneBuffer* buffer = dvz_scene_buffer(scene, &buffer_desc);
    DvzSampledFieldDesc field_desc = dvz_sampled_field_desc();
    field_desc.semantic = DVZ_FIELD_SEMANTIC_COLOR;
    field_desc.color_role = DVZ_COLOR_ROLE_LINEAR_COLOR;
    field_desc.width = 4;
    field_desc.height = 1;
    DvzSampledField* field = dvz_sampled_field(scene, &field_desc);
    if (buffer == NULL || field == NULL)
        return NULL;
    DvzFieldDataView view = dvz_field_data_view();
    view.data = pixels;
    view.bytes_per_row = 16;
    view.rows_per_image = 1;
    DvzFieldSamplingDesc sampling = dvz_field_sampling_desc();
    sampling.min_filter = DVZ_FIELD_FILTER_NEAREST;
    sampling.mag_filter = DVZ_FIELD_FILTER_NEAREST;
    DvzMaterialDesc material = dvz_material_desc();
    material.model = DVZ_MATERIAL_MODEL_UNLIT;
    material.alpha_mode = mode;
    if (dvz_scene_buffer_set_data(buffer, indices, sizeof(indices)) != DVZ_OK ||
        dvz_sampled_field_set_data(field, &view) != DVZ_OK ||
        dvz_visual_set_data(visual, "position", positions, 4) != DVZ_OK ||
        dvz_visual_set_data(visual, "color", colors, 4) != DVZ_OK ||
        dvz_visual_set_data(visual, "normal", normals, 4) != DVZ_OK ||
        dvz_visual_set_data(visual, "texcoords", uv, 4) != DVZ_OK ||
        dvz_visual_set_buffer(visual, "index", buffer) != DVZ_OK ||
        dvz_visual_set_field(visual, "texture", field) != DVZ_OK ||
        dvz_visual_set_field_sampling(visual, "texture", &sampling) != DVZ_OK ||
        dvz_visual_set_material(visual, &material) != DVZ_OK)
        return NULL;
    return visual;
}
