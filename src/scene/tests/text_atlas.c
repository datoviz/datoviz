/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene text atlas tests                                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <limits.h>
#include <stdint.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_log.h"
#include "_scene.h"
#include "_shader_registry.h"
#include "../../drp2/_stream.h"
#include "datoviz/drp2.h"
#include "datoviz/scene.h"
#include "datoviz/vk/gpu_ctx.h"
#include "helpers.h"
#include "annotation/prepare_internal.h"
#include "text/internal.h"
#include "text/text_atlas_product_internal.h"
#include "text/text_internal.h"
#include "test_scene.h"
#include "testing.h"

#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
bool _dvz_drp2_runtime_vklite_download_buffer(
    DvzDrp2Runtime* runtime, uint64_t buffer_id, uint64_t offset, uint64_t size, void* out);
#endif




/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#define TST_SCENE_TEXT_ATLAS_GPU_CASE(test)                                                       \
    do                                                                                            \
    {                                                                                             \
        TstCaseDesc _tst_desc = tst_case_desc(#test, #test, (test));                              \
        _tst_desc.tags = tags;                                                                    \
        _tst_desc.resources = TST_RES_CPU | TST_RES_GPU | TST_RES_VULKAN;                         \
        _tst_desc.isolation = TST_ISOLATION_PROCESS;                                              \
        _tst_desc.run_flags = TST_RUN_CASE_ADAPTER_SUPPORTED;                                     \
        tst_suite_add_case((suite), _tst_desc);                                                   \
    } while (0)

#define TST_SCENE_TEXT_ATLAS_REQUIRE_VKLITE(ctx)                                                  \
    do                                                                                            \
    {                                                                                             \
        if (!_scene_vklite_runtime_available())                                                   \
        {                                                                                         \
            tst_skip((ctx), "Vulkan instance creation failed");                                   \
            return 0;                                                                             \
        }                                                                                         \
    } while (0)




/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return the first color target used by a command stream render pass.
 *
 * @param stream the DRP2 command stream
 * @return target texture id, or zero when no render pass exists
 */
static uint64_t _first_render_target_id(const DvzDrp2CommandStream* stream)
{
    ANN(stream);
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(stream, i);
        ANN(command);
        if (command->type == DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS)
            return command->u.begin_render_pass.texture_id;
    }
    return 0;
}



/**
 * Count green text pixels in a downloaded RGBA buffer.
 *
 * @param pixels downloaded RGBA pixels
 * @param width image width
 * @param height image height
 * @return number of pixels dominated by the green channel
 */
static uint32_t _green_text_pixel_count(const uint8_t* pixels, uint32_t width, uint32_t height)
{
    ANN(pixels);
    uint32_t count = 0;
    for (uint32_t i = 0; i < width * height; i++)
    {
        const uint8_t* pixel = &pixels[4 * i];
        if (pixel[1] > 60 && pixel[1] > pixel[0] + 20 && pixel[1] > pixel[2] + 20)
            count++;
    }
    return count;
}



/**
 * Load the built-in Source Sans 3 face into a product-builder font view.
 *
 * @param out_scene output owning scene
 * @param out_primary output borrowed Source Sans 3 font view
 * @return whether the built-in font bytes are available
 */
static bool _text_atlas_product_source_view(
    DvzScene** out_scene, DvzTextAtlasFontView* out_primary)
{
    ANN(out_scene);
    ANN(out_primary);
    *out_scene = dvz_scene();
    if (*out_scene == NULL)
        return false;

    DvzFontDesc desc = dvz_font_desc();
    desc.family = "Source Sans 3";
    desc.style = "Regular";
    DvzFont* font = dvz_font(*out_scene, &desc);
    if (font == NULL || !_scene_font_ensure_bytes(font))
    {
        dvz_scene_destroy(*out_scene);
        *out_scene = NULL;
        return false;
    }
    if (font->face_index > INT32_MAX)
    {
        dvz_scene_destroy(*out_scene);
        *out_scene = NULL;
        return false;
    }
    *out_primary = (DvzTextAtlasFontView){
        .bytes = (const uint8_t*)font->ttf_bytes,
        .size = font->ttf_size,
        .face_index = (int32_t)font->face_index,
        .load_flags = 0,
    };
    return true;
}



#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
/**
 * Execute a readback copy from one rendered texture target.
 *
 * @param runtime the DRP2 runtime
 * @param target_id rendered texture id
 * @param width target width
 * @param height target height
 * @param out_pixels output RGBA buffer
 * @return true on success
 */
static bool _download_render_target(
    DvzDrp2Runtime* runtime, uint64_t target_id, uint32_t width, uint32_t height,
    uint8_t* out_pixels)
{
    ANN(runtime);
    ANN(out_pixels);
    ASSERT(target_id != 0);

    const uint64_t readback_buffer_id = 9201;
    const uint64_t encoder_id = 9202;
    const uint64_t command_buffer_id = 9203;
    const uint64_t submission_id = 9204;
    const uint64_t byte_size = (uint64_t)width * height * 4;

    DvzDrp2CommandStream* readback = dvz_drp2_stream();
    ANN(readback);
    bool ok = dvz_drp2_stream_create_buffer(
                  readback, readback_buffer_id, byte_size,
                  DVZ_DRP2_BUFFER_USAGE_COPY_DST | DVZ_DRP2_BUFFER_USAGE_MAP_READ) &&
              dvz_drp2_stream_begin_command_encoder(readback, encoder_id) &&
              dvz_drp2_stream_copy_texture_to_buffer(
                  readback, encoder_id, target_id, readback_buffer_id, 0, width, height,
                  width * 4, height) &&
              dvz_drp2_stream_finish_command_encoder(readback, encoder_id, command_buffer_id) &&
              dvz_drp2_stream_queue_submit(readback, command_buffer_id, submission_id);

    if (ok)
    {
        DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, readback);
        ok = result.ok && result.code == DVZ_DRP2_VALIDATION_OK;
    }
    if (ok)
        ok = _dvz_drp2_runtime_vklite_download_buffer(
            runtime, readback_buffer_id, 0, byte_size, out_pixels);

    _test_scene_stream_destroy(readback);
    return ok;
}
#endif




/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

/**
 * Verify MSDF glyph shaders decode RGB distance with alpha used only as artifact guard.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_text_msdf_shader_uses_rgb_distance(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    const char* glsl = _builtin_shader_glsl(DVZ_SCENE_BUILTIN_SHADER_GLYPH, true);
    const char* wgsl = _builtin_shader_wgsl(DVZ_SCENE_BUILTIN_SHADER_GLYPH, true);
    ANN(glsl);
    ANN(wgsl);

    AT(strstr(glsl, "float median3") != NULL);
    AT(strstr(glsl, "float msdf = median3(texel.r, texel.g, texel.b);") != NULL);
    AT(strstr(glsl, "float sdf = texel.a;") != NULL);
    AT(strstr(glsl, "opacity = distanceOpacity(sd);") != NULL);
    AT(strstr(glsl, "opacity = distanceOpacity(texel.a);") != NULL);

    AT(strstr(wgsl, "fn median3") != NULL);
    AT(strstr(wgsl, "let msdf = median3(texel.r, texel.g, texel.b);") != NULL);
    AT(strstr(wgsl, "let sdf = texel.a;") != NULL);
    AT(strstr(wgsl, "opacity = distanceOpacity(input.uv, sd);") != NULL);
    AT(strstr(wgsl, "opacity = distanceOpacity(input.uv, texel.a);") != NULL);

    return 0;
}


/**
 * Verify Source ASCII MSDF atlases use embedded payloads and extend transactionally at runtime.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_text_source_msdf_uses_embedded_atlas(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

#if defined(DVZ_HAS_ZLIB) && DVZ_HAS_ZLIB
    DvzScene* scene = dvz_scene();
    ANN(scene);

    DvzFontDesc desc = dvz_font_desc();
    DvzFont* font = dvz_font(scene, &desc);
    ANN(font);
    AT(font->source_id == DVZ_FONT_SOURCE_SOURCE_SANS_3_REGULAR);
    AT(font->ttf_bytes == NULL);
    AT(font->ttf_size == 0);

    DvzTextAtlasSpec small_spec =
        _scene_text_atlas_spec(DVZ_TEXT_ATLAS_BACKEND_MSDF, 38.0f);
    AT(_scene_text_atlas_ensure_string(font, &small_spec, "MSDF atlas renderer"));
    AT(font->ttf_bytes == NULL);
    AT(font->ttf_size == 0);
    DvzTextAtlas* small_atlas = _scene_text_atlas_get(font, &small_spec);
    ANN(small_atlas);
    ANN(small_atlas->field);
    AT(small_atlas->backend == DVZ_TEXT_ATLAS_BACKEND_MSDF);
    AT(small_atlas->encoding == DVZ_TEXT_ATLAS_ENCODING_MSDF_RGB);
    AT(small_atlas->glyph_count >= 95);
    DvzTextAtlasGlyph* small_glyph = _scene_text_atlas_glyph(small_atlas, 'A');
    ANN(small_glyph);

    DvzTextAtlasSpec large_spec =
        _scene_text_atlas_spec(DVZ_TEXT_ATLAS_BACKEND_MSDF, 84.0f);
    AT(_scene_text_atlas_ensure_string(font, &large_spec, "Retained text"));
    AT(font->ttf_bytes == NULL);
    AT(font->ttf_size == 0);
    DvzTextAtlas* large = _scene_text_atlas_get(font, &large_spec);
    ANN(large);
    ANN(large->field);
    AT(large->backend == DVZ_TEXT_ATLAS_BACKEND_MSDF);
    AT(large->encoding == DVZ_TEXT_ATLAS_ENCODING_MSDF_RGB);
    AT(large->glyph_count >= 95);
    DvzTextAtlasGlyph* large_glyph = _scene_text_atlas_glyph(large, 'R');
    ANN(large_glyph);

    DvzTextAtlasSpec huge_spec =
        _scene_text_atlas_spec(DVZ_TEXT_ATLAS_BACKEND_MSDF, 128.0f);
    AT(_scene_text_atlas_ensure_string(font, &huge_spec, "Large retained text"));
    AT(font->ttf_bytes == NULL);
    AT(font->ttf_size == 0);
    DvzTextAtlas* huge = _scene_text_atlas_get(font, &huge_spec);
    ANN(huge);
    ANN(huge->field);
    AT(huge->backend == DVZ_TEXT_ATLAS_BACKEND_MSDF);
    AT(huge->encoding == DVZ_TEXT_ATLAS_ENCODING_MSDF_RGB);
    AT(huge->glyph_count >= 95);
    DvzTextAtlasGlyph* huge_glyph = _scene_text_atlas_glyph(huge, 'L');
    ANN(huge_glyph);

    uint64_t small_generation = small_atlas->generation;
    AT(_scene_text_atlas_ensure_string(font, &small_spec, "caf\xC3\xA9"));
    AT(font->ttf_bytes != NULL);
    AT(font->ttf_size > 0);
    AT(_scene_text_atlas_get(font, &small_spec) == small_atlas);
    AT(small_atlas->generation > small_generation);
    DvzTextAtlasGlyph* extended_glyph = _scene_text_atlas_glyph(small_atlas, 0x00E9);
    ANN(extended_glyph);
    AT(extended_glyph->valid);

    dvz_scene_destroy(scene);
#else
    tst_skip(suite, "zlib unavailable");
#endif
    return 0;
}


/**
 * Verify built-in and custom-path fonts have distinct resolved source identities.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_text_font_source_identity(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);

    DvzFontDesc built_in_desc = dvz_font_desc();
    built_in_desc.family = "Source Sans 3";
    built_in_desc.style = "Regular";
    DvzFont* built_in = dvz_font(scene, &built_in_desc);
    ANN(built_in);
    AT(built_in->source_id == DVZ_FONT_SOURCE_SOURCE_SANS_3_REGULAR);

    DvzFontDesc custom_desc = built_in_desc;
    custom_desc.path = "assets/runtime/fonts/SourceSans3-Regular.ttf";
    DvzFont* custom = dvz_font(scene, &custom_desc);
    ANN(custom);
    AT(custom->source_id == DVZ_FONT_SOURCE_CUSTOM_FILE);
    AT(custom->source_id != built_in->source_id);

    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify CPU atlas-product defaults are bounded and deterministic.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_text_atlas_product_defaults(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzTextAtlasProductBudget budget = _text_atlas_product_budget_default();
    AT(budget.max_glyphs == 256u);
    AT(budget.max_dimension == 4096u);
    AT(budget.max_rgba_bytes == 64ull * 1024ull * 1024ull);

    DvzTextAtlasProductParams params = _text_atlas_product_params_default();
    AT(params.thread_count == 8u);
    AT(params.fallback_codepoint == (uint32_t)'?');
    AT(params.edge_coloring_seed == 0);
    AC(params.max_corner_angle, 3.0, 1e-12);
    AC(params.miter_limit, 1.0, 1e-12);
    AT(params.overlap_support);
    AT(params.scanline_pass);
    AT(params.preprocess_geometry);
    AT(params.enable_kerning);
    return 0;
}



/**
 * Verify the CPU product builder validates a complete Source Sans 3 ASCII atlas.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_text_atlas_product_builds_source_ascii(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;
    if (!_text_atlas_product_msdf_available())
    {
        tst_skip(suite, "MSDF atlas generation unavailable");
        return 0;
    }

    DvzScene* scene = NULL;
    DvzTextAtlasFontView primary = {0};
    AT(_text_atlas_product_source_view(&scene, &primary));
    uint32_t codepoints[96] = {0};
    for (uint32_t i = 0; i < 95; i++)
        codepoints[i] = 0x20u + i;
    codepoints[95] = 0x10FFFFu;
    DvzTextAtlasSpec spec = _scene_text_atlas_spec(DVZ_TEXT_ATLAS_BACKEND_MSDF, 32.0f);
    DvzTextAtlasProductBudget budget = _text_atlas_product_budget_default();
    DvzTextAtlasProductParams params = _text_atlas_product_params_default();
    DvzTextAtlasProduct product = {0};

    AT(_text_atlas_product_build_msdf(
        &primary, NULL, &spec, codepoints, 96, &budget, &params, &product));
    AT(_text_atlas_product_validate(&product, &budget));
    AT(product.coverage_count == 96);
    AT(product.fallback_mapping_count == 1);
    AT(product.glyph_count > 0);
    for (uint32_t i = 0; i < 95; i++)
    {
        AT(product.coverage[i].requested_codepoint == codepoints[i]);
        AT(product.coverage[i].resolved_codepoint == codepoints[i]);
        AT(product.coverage[i].kind == DVZ_TEXT_ATLAS_PRODUCT_COVERAGE_EXACT);
        AT(product.coverage[i].font_role == DVZ_TEXT_ATLAS_PRODUCT_FONT_PRIMARY);
    }
    AT(product.coverage[95].requested_codepoint == 0x10FFFFu);
    AT(product.coverage[95].resolved_codepoint == (uint32_t)'?');
    AT(product.coverage[95].kind == DVZ_TEXT_ATLAS_PRODUCT_COVERAGE_FALLBACK);
    AT(product.coverage[95].font_role == DVZ_TEXT_ATLAS_PRODUCT_FONT_PRIMARY);

    _text_atlas_product_destroy(&product);
    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Verify one-thread CPU atlas products are byte-identical across two builds.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_text_atlas_product_is_deterministic(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;
    if (!_text_atlas_product_msdf_available())
    {
        tst_skip(suite, "MSDF atlas generation unavailable");
        return 0;
    }

    DvzScene* scene = NULL;
    DvzTextAtlasFontView primary = {0};
    AT(_text_atlas_product_source_view(&scene, &primary));
    const uint32_t codepoints[] = {0x20u, 0x41u, 0x42u, 0x61u, 0x62u, 0x7Eu};
    DvzTextAtlasSpec spec = _scene_text_atlas_spec(DVZ_TEXT_ATLAS_BACKEND_MSDF, 32.0f);
    DvzTextAtlasProductBudget budget = _text_atlas_product_budget_default();
    DvzTextAtlasProductParams params = _text_atlas_product_params_default();
    params.thread_count = 1;
    DvzTextAtlasProduct first = {0};
    DvzTextAtlasProduct second = {0};

    AT(_text_atlas_product_build_msdf(
        &primary, NULL, &spec, codepoints, 6, &budget, &params, &first));
    AT(_text_atlas_product_build_msdf(
        &primary, NULL, &spec, codepoints, 6, &budget, &params, &second));
    AT(_text_atlas_product_validate(&first, &budget));
    AT(_text_atlas_product_validate(&second, &budget));
    AT(first.width == second.width);
    AT(first.height == second.height);
    AT(first.glyph_count == second.glyph_count);
    AT(first.coverage_count == second.coverage_count);
    AT(first.fallback_mapping_count == second.fallback_mapping_count);
    AT(first.rgba_size == second.rgba_size);
    AC(first.em_px, second.em_px, 1e-6f);
    AC(first.distance_range_px, second.distance_range_px, 1e-6f);
    AC(first.ascent, second.ascent, 1e-6f);
    AC(first.descent, second.descent, 1e-6f);
    AC(first.line_gap, second.line_gap, 1e-6f);
    AC(first.line_height, second.line_height, 1e-6f);
    AT(memcmp(first.rgba, second.rgba, (size_t)first.rgba_size) == 0);
    AT(memcmp(
           first.glyphs, second.glyphs,
           (size_t)first.glyph_count * sizeof(DvzTextAtlasGlyph)) == 0);
    AT(memcmp(
           first.coverage, second.coverage,
           (size_t)first.coverage_count * sizeof(DvzTextAtlasProductCoverage)) == 0);

    _text_atlas_product_destroy(&second);
    _text_atlas_product_destroy(&first);
    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Verify a CPU atlas product budget failure leaves the output object empty.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_text_atlas_product_budget_failure_is_empty(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;
    if (!_text_atlas_product_msdf_available())
    {
        tst_skip(suite, "MSDF atlas generation unavailable");
        return 0;
    }

    DvzScene* scene = NULL;
    DvzTextAtlasFontView primary = {0};
    AT(_text_atlas_product_source_view(&scene, &primary));
    const uint32_t codepoints[] = {0x20u, 0x41u};
    DvzTextAtlasSpec spec = _scene_text_atlas_spec(DVZ_TEXT_ATLAS_BACKEND_MSDF, 32.0f);
    DvzTextAtlasProductBudget budget = _text_atlas_product_budget_default();
    budget.max_dimension = 1;
    DvzTextAtlasProductParams params = _text_atlas_product_params_default();
    DvzTextAtlasProduct product = {0};

    AT(!_text_atlas_product_build_msdf(
        &primary, NULL, &spec, codepoints, 2, &budget, &params, &product));
    AT(product.rgba == NULL);
    AT(product.glyphs == NULL);
    AT(product.coverage == NULL);
    AT(product.width == 0);
    AT(product.height == 0);
    AT(product.glyph_count == 0);
    AT(product.coverage_count == 0);
    AT(product.rgba_size == 0);

    _text_atlas_product_destroy(&product);
    dvz_scene_destroy(scene);
    return 0;
}



/**
 * Verify over-capacity requests fail without mutating an existing atlas realization.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_text_atlas_capacity_failure_rolls_back(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFontDesc desc = dvz_font_desc();
    desc.family = "Source Sans 3";
    desc.style = "Regular";
    DvzFont* font = dvz_font(scene, &desc);
    ANN(font);

    DvzTextAtlasSpec spec =
        _scene_text_atlas_spec(DVZ_TEXT_ATLAS_BACKEND_STB_SDF, 32.0f);
    AT(_scene_text_atlas_ensure_string(font, &spec, "ASCII"));
    DvzTextAtlas* initial = _scene_text_atlas_get(font, &spec);
    ANN(initial);
    const DvzSampledField* initial_field = initial->field;
    uint64_t initial_generation = initial->generation;
    uint32_t initial_glyph_count = initial->glyph_count;
    const DvzTextAtlasGlyph* initial_a = _scene_text_atlas_glyph(initial, 'A');
    ANN(initial_a);
    DvzTextAtlasGlyph a = *initial_a;

    char encoded[200][3] = {{0}};
    const char* strings[200] = {0};
    for (uint32_t i = 0; i < 200; i++)
    {
        uint32_t codepoint = 0x0100u + i;
        encoded[i][0] = (char)(0xC0u | (codepoint >> 6));
        encoded[i][1] = (char)(0x80u | (codepoint & 0x3Fu));
        strings[i] = encoded[i];
    }
    AT_EXPECTED_ERROR_STRICT(
        suite, !_scene_text_atlas_ensure_strings(font, &spec, strings, 200));

    DvzTextAtlas* realized = _scene_text_atlas_get(font, &spec);
    AT(realized == initial);
    AT(realized->field == initial_field);
    AT(realized->generation == initial_generation);
    AT(realized->glyph_count == initial_glyph_count);
    const DvzTextAtlasGlyph* realized_a = _scene_text_atlas_glyph(realized, 'A');
    ANN(realized_a);
    AT(realized_a->glyph_id == a.glyph_id);
    AC(realized_a->advance, a.advance, 1e-6f);

    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify Source scene text falls back to Noto Sans Math for scientific symbols.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_text_scientific_fallback_all_backends(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    ANN(item);

    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFontDesc desc = dvz_font_desc();
    desc.family = "Source Sans 3";
    desc.style = "Regular";
    DvzFont* font = dvz_font(scene, &desc);
    ANN(font);

    const char* scientific = "\xE2\x88\x87 \xE2\x88\x9D \xE2\x88\x88";
    const uint32_t codepoints[] = {0x2207u, 0x221Du, 0x2208u};
    const DvzTextAtlasBackend backends[] = {
        DVZ_TEXT_ATLAS_BACKEND_MSDF,
        DVZ_TEXT_ATLAS_BACKEND_FREETYPE_BITMAP,
        DVZ_TEXT_ATLAS_BACKEND_STB_SDF,
    };
    for (uint32_t i = 0; i < 3; i++)
    {
        DvzTextAtlasSpec spec = _scene_text_atlas_spec(backends[i], 32.0f);
        AT(_scene_text_atlas_ensure_string(font, &spec, scientific));
        DvzTextAtlas* atlas = _scene_text_atlas_get(font, &spec);
        ANN(atlas);
        AT(atlas->missing_glyph_count == 0);
        for (uint32_t j = 0; j < 3; j++)
        {
            DvzTextAtlasGlyph* glyph = _scene_text_atlas_glyph(atlas, codepoints[j]);
            ANN(glyph);
            AT(glyph->codepoint == codepoints[j]);
            AT(glyph->valid);
            AT(glyph->glyph_id != 0);
        }
    }

    dvz_scene_destroy(scene);
    return 0;
}


/**
 * Verify the public font atlas API exposes generated atlas metadata and glyph metrics.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_text_public_font_atlas_api(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

#if defined(DVZ_HAS_ZLIB) && DVZ_HAS_ZLIB
    DvzScene* scene = dvz_scene();
    ANN(scene);

    DvzFontDesc desc = dvz_font_desc();
    desc.family = "Roboto";
    desc.style = "Regular";
    DvzFont* font = dvz_font(scene, &desc);
    ANN(font);

    DvzTextAtlasSpec spec = dvz_text_atlas_spec(DVZ_TEXT_RENDERER_MSDF_ATLAS, 48.0f);
    AT(spec.backend == DVZ_TEXT_ATLAS_BACKEND_MSDF);
    AT(dvz_font_atlas_ensure_string(font, &spec, "Atlas caf" "\xC3" "\xA9"));

    const DvzTextAtlas* atlas = dvz_font_atlas(font, &spec);
    ANN(atlas);
    DvzTextAtlasInfo info = dvz_text_atlas_info(atlas);
    AT(info.backend == DVZ_TEXT_ATLAS_BACKEND_MSDF);
    AT(info.encoding == DVZ_TEXT_ATLAS_ENCODING_MSDF_RGB);
    AT(info.width > 0);
    AT(info.height > 0);
    AT(info.glyph_count >= 95);
    AT(info.channels == 4);

    const DvzSampledField* field = dvz_text_atlas_field(atlas);
    ANN(field);
    DvzSampledFieldDesc field_desc = {0};
    AT(dvz_sampled_field_info(field, &field_desc));
    AT(field_desc.width == info.width);
    AT(field_desc.height == info.height);
    AT(field_desc.format == DVZ_FIELD_FORMAT_RGBA8_UNORM);

    DvzVisual* glyph_visual = dvz_glyph(scene, 0);
    ANN(glyph_visual);
    AT(dvz_glyph_set_atlas(glyph_visual, atlas) == 0);
    AT(_visual_family_state(glyph_visual)->field == field);
    AT(_visual_family_state(glyph_visual)->glyph_atlas_encoding == info.encoding);
    AT(_visual_family_state(glyph_visual)->glyph_distance_range_px == info.distance_range_px);

    const DvzTextAtlasGlyph* glyph = dvz_text_atlas_glyph(atlas, 0x00E9u);
    ANN(glyph);
    AT(glyph->valid);
    AT(glyph->width > 0.0f);
    AT(glyph->height > 0.0f);
    AT(glyph->advance > 0.0f);
    AT(glyph->uv[0] < glyph->uv[2]);
    AT(glyph->uv[1] < glyph->uv[3]);

    const DvzTextAtlasGlyph* fallback = dvz_text_atlas_glyph(atlas, 0x10FFFFu);
    ANN(fallback);
    AT(fallback->codepoint == '?');

    dvz_scene_destroy(scene);
#else
    tst_skip(suite, "zlib unavailable");
#endif
    return 0;
}



/**
 * Verify font-backed UTF-8 text renders through scene DRP2 runtime readback.
 *
 * @param suite the active test suite
 * @param item the active test item
 * @return 0 on success
 */
int test_scene_text_atlas_utf8_runtime_readback(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

#if defined(DVZ_DRP2_HAS_VKLITE) && DVZ_DRP2_HAS_VKLITE
    TST_SCENE_TEXT_ATLAS_REQUIRE_VKLITE(suite);

    DvzGpuCtxConfig gpu_cfg = dvz_testing_gpu_ctx_config(suite);
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    dvz_gpu_ctx_config_features13(&gpu_cfg, &features13);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        log_warn("test_scene_text_atlas_utf8_runtime_readback skipped: GPU context creation failed");
        tst_skip(suite, "GPU context creation failed");
        return 0;
    }

    const uint32_t width = 192;
    const uint32_t height = 72;
    DvzScene* scene = dvz_scene();
    ANN(scene);
    DvzFigure* figure = dvz_figure(scene, width, height, 0);
    ANN(figure);
    DvzPanel* panel = dvz_panel(
        figure, &(DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    ANN(panel);

    DvzVisual* text = _scene_text_visual(scene, 0);
    ANN(text);
    AT(_scene_text_visual_set_renderer(text, DVZ_TEXT_RENDERER_MSDF_ATLAS) == 0);
    const char* strings[1] = {"Atlas UTF-8 caf" "\xC3" "\xA9"};
    vec3 positions[1] = {{8.0f, 16.0f, 0.0f}};
    vec2 text_anchors[1] = {{0.0f, 0.0f}};
    float sizes[1] = {24.0f};
    float angles[1] = {0.0f};
    DvzColor colors[1] = {{0, 255, 0, 255}};
    DvzVisualDataUpdate updates[5] = {
        {.attr_name = "position", .data = positions, .item_count = 1},
        {.attr_name = "anchor", .data = text_anchors, .item_count = 1},
        {.attr_name = "size", .data = sizes, .item_count = 1},
        {.attr_name = "color", .data = colors, .item_count = 1},
        {.attr_name = "angle", .data = angles, .item_count = 1},
    };
    AT(dvz_visual_set_strings(text, "text", strings, 1) == 0);
    AT(dvz_visual_set_data_many(text, updates, 5) == 0);
    AT(dvz_panel_add_visual(
           panel, text,
           &(DvzVisualAttachDesc){DVZ_STRUCT_INIT_FIELDS(DvzVisualAttachDesc),
               .z_layer = 1,
               .controller_mode = DVZ_CONTROLLER_FIXED,
               .coord_space = DVZ_VISUAL_COORD_PANEL_PIXEL}) == 0);

    _scene_prepare_text_visuals(figure);
    AT(_visual_family_state(text)->text.glyph_visual != NULL);
    AT(scene->font_count == 1);
    DvzTextAtlasSpec spec = _scene_text_atlas_spec(DVZ_TEXT_ATLAS_BACKEND_MSDF, sizes[0]);
    DvzTextAtlas* atlas = _scene_text_atlas_get(&scene->fonts[0], &spec);
    ANN(atlas);
    ANN(atlas->field);
    DvzTextAtlasGlyph* utf8_glyph = _scene_text_atlas_glyph(atlas, 0x00E9u);
    ANN(utf8_glyph);
    AT(_visual_family_state(_visual_family_state(text)->text.glyph_visual)->field == atlas->field);

    DvzCapabilitySnapshot caps = dvz_capability_snapshot();
    caps.supports_color_blending = true;
    DvzDiagnosticReport report;
    dvz_diagnostic_report_init(&report);
    DvzFramePlanEmitConfig emit_cfg = dvz_frame_plan_emit_config();
    emit_cfg.shader_format = DVZ_SCENE_SHADER_FORMAT_GLSL;
    emit_cfg.target_width = width;
    emit_cfg.target_height = height;

    DvzDrp2CommandStream* stream = _test_scene_emit_stream_ex(figure, &caps, &report, &emit_cfg);
    if (stream == NULL && dvz_diagnostic_report_count(&report) > 0)
    {
        log_error("%s", dvz_diagnostic_report_get(&report, 0));
    }
    AT(dvz_diagnostic_report_count(&report) == 0);
    ANN(stream);
    uint64_t target_id = _first_render_target_id(stream);
    AT(target_id != 0);

    DvzDrp2RuntimeConfig runtime_cfg =
        dvz_drp2_runtime_vklite_config(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx));
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&runtime_cfg);
    ANN(runtime);

    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    AT(result.ok);
    AT(result.code == DVZ_DRP2_VALIDATION_OK);
    AT(dvz_gpu_ctx_error_count(ctx) == 0);

    uint8_t pixels[192 * 72 * 4] = {0};
    AT(_download_render_target(runtime, target_id, width, height, pixels));
    AT(_green_text_pixel_count(pixels, width, height) > 0);

    dvz_drp2_runtime_destroy(runtime);
    _test_scene_stream_destroy(stream);
    dvz_scene_destroy(scene);
    dvz_gpu_ctx_destroy(ctx);
#else
    tst_skip(suite, "DRP2 vklite runtime unavailable");
#endif
    return 0;
}




/*************************************************************************************************/
/*  Entry-point                                                                                  */
/*************************************************************************************************/

/**
 * Register scene text atlas tests.
 *
 * @param suite the active test suite
 * @return 0 on success
 */
int test_scene_text_atlas(TstSuite* suite)
{
    ANN(suite);
    const char* tags = "scene";

    TST_MODULE(suite, "scene");
    TST_GROUP("text-atlas");

    TST_CASE(test_scene_text_msdf_shader_uses_rgb_distance);
    TST_CASE(test_scene_text_source_msdf_uses_embedded_atlas);
    TST_CASE(test_scene_text_font_source_identity);
    TST_CASE(test_scene_text_atlas_product_defaults);
    TST_CASE(test_scene_text_atlas_product_builds_source_ascii);
    TST_CASE(test_scene_text_atlas_product_is_deterministic);
    TST_CASE(test_scene_text_atlas_product_budget_failure_is_empty);
    TST_CASE(test_scene_text_atlas_capacity_failure_rolls_back);
    TST_CASE(test_scene_text_scientific_fallback_all_backends);
    TST_CASE(test_scene_text_public_font_atlas_api);
    TST_SCENE_TEXT_ATLAS_GPU_CASE(test_scene_text_atlas_utf8_runtime_readback);

    return 0;
}
