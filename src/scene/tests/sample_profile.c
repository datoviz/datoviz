/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene sampled-field profile tests                                                            */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <string.h>

#include "_assertions.h"
#include "colorizer.h"
#include "sample_profile.h"
#include "datoviz/scene.h"
#include "test_scene.h"
#include "testing.h"



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

/**
 * Check sampled-field profile resolution for scalar image and volume fields.
 *
 * @param suite the test suite
 * @param item the test case
 * @return 0 on success
 */
static int test_scene_sample_profile_scalar(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzSceneSampleProfile profile = {0};
    AT(_scene_sample_profile_resolve(
        DVZ_FIELD_FORMAT_R32_FLOAT, DVZ_FIELD_SEMANTIC_SCALAR, DVZ_FIELD_DIM_2D, &profile));
    AT(profile.value_kind == DVZ_SCENE_SAMPLE_VALUE_SCALAR_F32);
    AT(profile.sampler_kind == DVZ_SCENE_SAMPLE_SAMPLER_FLOAT);
    AT(profile.colorizer_kind == DVZ_SCENE_COLORIZER_COLORMAP_1D);
    AT(profile.filter_linear_allowed);
    AT(profile.query_raw_supported);
    AT(profile.query_position_supported);

    AT(_scene_sample_profile_resolve(
        DVZ_FIELD_FORMAT_R16_UNORM, DVZ_FIELD_SEMANTIC_SCALAR, DVZ_FIELD_DIM_3D, &profile));
    AT(profile.value_kind == DVZ_SCENE_SAMPLE_VALUE_SCALAR_F32);
    AT(profile.sampler_kind == DVZ_SCENE_SAMPLE_SAMPLER_FLOAT);
    AT(profile.colorizer_kind == DVZ_SCENE_COLORIZER_TRANSFER_1D);
    AT(profile.filter_linear_allowed);

    AT(_scene_sample_profile_resolve(
        DVZ_FIELD_FORMAT_R16_UINT, DVZ_FIELD_SEMANTIC_SCALAR, DVZ_FIELD_DIM_3D, &profile));
    AT(profile.value_kind == DVZ_SCENE_SAMPLE_VALUE_SCALAR_F32);
    AT(profile.sampler_kind == DVZ_SCENE_SAMPLE_SAMPLER_UINT);
    AT(!profile.filter_linear_allowed);

    return 0;
}



/**
 * Check sampled-field profile resolution for signed and unsigned label fields.
 *
 * @param suite the test suite
 * @param item the test case
 * @return 0 on success
 */
static int test_scene_sample_profile_labels(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzSceneSampleProfile profile = {0};
    AT(_scene_sample_profile_resolve(
        DVZ_FIELD_FORMAT_R16_UINT, DVZ_FIELD_SEMANTIC_LABEL, DVZ_FIELD_DIM_2D, &profile));
    AT(profile.value_kind == DVZ_SCENE_SAMPLE_VALUE_LABEL_U32);
    AT(profile.sampler_kind == DVZ_SCENE_SAMPLE_SAMPLER_UINT);
    AT(profile.colorizer_kind == DVZ_SCENE_COLORIZER_CATEGORICAL);
    AT(!profile.filter_linear_allowed);
    AT(_scene_sample_profile_is_integer_label(&profile));

    AT(_scene_sample_profile_resolve(
        DVZ_FIELD_FORMAT_R32_SINT, DVZ_FIELD_SEMANTIC_LABEL, DVZ_FIELD_DIM_3D, &profile));
    AT(profile.value_kind == DVZ_SCENE_SAMPLE_VALUE_LABEL_S32);
    AT(profile.sampler_kind == DVZ_SCENE_SAMPLE_SAMPLER_SINT);
    AT(profile.colorizer_kind == DVZ_SCENE_COLORIZER_CATEGORICAL);
    AT(!profile.filter_linear_allowed);
    AT(_scene_sample_profile_is_integer_label(&profile));

    return 0;
}



/**
 * Check sampled-field profile resolution for direct RGBA fields.
 *
 * @param suite the test suite
 * @param item the test case
 * @return 0 on success
 */
static int test_scene_sample_profile_rgba(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzSceneSampleProfile profile = {0};
    AT(_scene_sample_profile_resolve(
        DVZ_FIELD_FORMAT_RGBA8_UNORM, DVZ_FIELD_SEMANTIC_COLOR, DVZ_FIELD_DIM_2D, &profile));
    AT(profile.value_kind == DVZ_SCENE_SAMPLE_VALUE_RGBA_U8);
    AT(profile.sampler_kind == DVZ_SCENE_SAMPLE_SAMPLER_FLOAT);
    AT(profile.colorizer_kind == DVZ_SCENE_COLORIZER_DIRECT_RGBA);
    AT(profile.filter_linear_allowed);

    AT(_scene_sample_profile_resolve(
        DVZ_FIELD_FORMAT_RGBA8_UNORM, DVZ_FIELD_SEMANTIC_COLOR, DVZ_FIELD_DIM_3D, &profile));
    AT(profile.value_kind == DVZ_SCENE_SAMPLE_VALUE_RGBA_U8);
    AT(profile.colorizer_kind == DVZ_SCENE_COLORIZER_DIRECT_RGBA);

    return 0;
}



/**
 * Check sampled-field profile rejection for unsupported semantic and format combinations.
 *
 * @param suite the test suite
 * @param item the test case
 * @return 0 on success
 */
static int test_scene_sample_profile_rejects_unsupported(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzSceneSampleProfile profile = {0};
    AT(!_scene_sample_profile_resolve(
        DVZ_FIELD_FORMAT_R32_FLOAT, DVZ_FIELD_SEMANTIC_LABEL, DVZ_FIELD_DIM_3D, &profile));
    AT(!_scene_sample_profile_resolve(
        DVZ_FIELD_FORMAT_RGBA8_UNORM, DVZ_FIELD_SEMANTIC_SCALAR, DVZ_FIELD_DIM_2D, &profile));
    AT(!_scene_sample_profile_resolve(
        DVZ_FIELD_FORMAT_R8_UINT, DVZ_FIELD_SEMANTIC_COLOR, DVZ_FIELD_DIM_2D, &profile));

    return 0;
}



/**
 * Check categorical colorizer lookup and dense palette construction.
 *
 * @param suite the test suite
 * @param item the test case
 * @return 0 on success
 */
static int test_scene_colorizer_categorical_palette(TstContext* suite, const TstCase* item)
{
    (void)suite;
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzScale* scale =
        dvz_scale(scene, &(DvzScaleDesc){.kind = DVZ_SCALE_CATEGORICAL, .label = "regions"});
    ANN(scale);
    DvzScaleCategory categories[] = {
        {.category_id = 0, .order = 0, .label = "background", .color = {0, 0, 0, 0}},
        {.category_id = 2, .order = 1, .label = "region", .color = {10, 20, 30, 255}},
    };
    AT(dvz_scale_set_categories(scale, categories, 2));

    DvzSceneColorizer colorizer = {0};
    AT(_scene_colorizer_from_scale(scale, DVZ_SCENE_COLORIZER_CATEGORICAL, &colorizer));
    DvzColor color = {0};
    AT(_scene_colorizer_category_color(&colorizer, 2, &color));
    AT(color.r == 10);
    AT(color.g == 20);
    AT(color.b == 30);
    AT(color.a == 255);

    char label[32] = {0};
    AT(_scene_colorizer_category_label(&colorizer, 2, label, sizeof(label)));
    AT(strcmp(label, "region") == 0);
    AT(!_scene_colorizer_category_label(&colorizer, 7, label, sizeof(label)));
    AT(strcmp(label, "label 7") == 0);

    uint32_t palette_count = 0;
    AT(_scene_colorizer_dense_palette_extent(&colorizer, &palette_count));
    AT(palette_count == 3);
    DvzColor palette[3] = {0};
    DvzColor fallback = {1, 2, 3, 4};
    AT(_scene_colorizer_build_dense_palette(&colorizer, fallback, palette, 3));
    AT(palette[0].a == 0);
    AT(palette[1].r == 1);
    AT(palette[1].a == 4);
    AT(palette[2].r == 10);
    AT(palette[2].a == 255);

    DvzScaleCategory sparse[] = {
        {.category_id = -7, .order = 0, .label = "negative", .color = {1, 2, 3, 4}},
        {.category_id = 4000000000LL, .order = 1, .label = "high", .color = {5, 6, 7, 8}},
        {.category_id = 23, .order = 2, .label = "low", .color = {9, 10, 11, 12}},
    };
    AT(dvz_scale_set_categories(scale, sparse, 3));
    AT(_scene_colorizer_from_scale(scale, DVZ_SCENE_COLORIZER_CATEGORICAL, &colorizer));
    AT(!_scene_colorizer_dense_palette_extent(&colorizer, &palette_count));

    uint32_t entry_count = 0;
    AT(_scene_colorizer_label_lookup_extent(&colorizer, &entry_count));
    AT(entry_count == 4);
    DvzSceneLabelLookupEntry entries[4] = {0};
    AT(_scene_colorizer_build_label_lookup(&colorizer, false, entries, 4));
    AT(entries[0].key == 2);
    AT(entries[1].key == 23);
    AT(entries[1].rgba == 0x0c0b0a09u);
    AT(entries[2].key == 4000000000u);
    AT(entries[2].rgba == 0x08070605u);

    AT(_scene_colorizer_build_label_lookup(&colorizer, true, entries, 4));
    AT(entries[0].key == 2);
    AT(entries[1].key == 23);
    AT(entries[2].key == (uint32_t)(int32_t)-7);
    AT(entries[2].rgba == 0x04030201u);

    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Register sampled-field profile tests.
 *
 * @param suite the active test suite
 * @return 0 on success
 */
int test_scene_sample_profile(TstSuite* suite)
{
    ANN(suite);
    const char* tags = "scene";

    TST_MODULE(suite, "scene");
    TST_GROUP("sample-profile");

    TST_CASE(test_scene_sample_profile_scalar);
    TST_CASE(test_scene_sample_profile_labels);
    TST_CASE(test_scene_sample_profile_rgba);
    TST_CASE(test_scene_sample_profile_rejects_unsupported);
    TST_CASE(test_scene_colorizer_categorical_palette);

    return 0;
}
