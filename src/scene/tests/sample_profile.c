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

#include "_assertions.h"
#include "../sample_profile.h"
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

    return 0;
}
