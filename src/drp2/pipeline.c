/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  DRP2 vklite runtime pipelines                                                                */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "shaderc/shaderc.h"
#include <volk.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_dynload.h"
#include "_log.h"
#include "_runtime.h"
#include "_stream.h"



#if DVZ_DRP2_HAS_VKLITE
/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

/* Lazy-loaded shaderc function-pointer table. Populated once on first GLSL compile call. */
typedef struct
{
    shaderc_compiler_t (*compiler_initialize)(void);
    void (*compiler_release)(shaderc_compiler_t);
    shaderc_compile_options_t (*compile_options_initialize)(void);
    void (*compile_options_release)(shaderc_compile_options_t);
    void (*compile_options_set_source_language)(
        shaderc_compile_options_t, shaderc_source_language);
    void (*compile_options_set_target_env)(
        shaderc_compile_options_t, shaderc_target_env, uint32_t);
    void (*compile_options_set_target_spirv)(
        shaderc_compile_options_t, shaderc_spirv_version);
    shaderc_compilation_result_t (*compile_into_spv)(
        shaderc_compiler_t, const char*, size_t, shaderc_shader_kind, const char*, const char*,
        const shaderc_compile_options_t);
    shaderc_compilation_status (*result_get_compilation_status)(shaderc_compilation_result_t);
    const char* (*result_get_error_message)(shaderc_compilation_result_t);
    const char* (*result_get_bytes)(shaderc_compilation_result_t);
    size_t (*result_get_length)(shaderc_compilation_result_t);
    void (*result_release)(shaderc_compilation_result_t);
} ShadercSyms;

static ShadercSyms g_shaderc = {0};
static bool g_shaderc_loaded = false;
static bool g_shaderc_available = false;

static bool _shaderc_load(void)
{
    if (g_shaderc_loaded)
        return g_shaderc_available;
    g_shaderc_loaded = true;

#ifndef DVZ_SHADERC_LIB_PATH
#define DVZ_SHADERC_LIB_PATH "libshaderc_shared.so.1"
#endif
    DvzDynLib lib = dvz_dynlib_open(DVZ_SHADERC_LIB_PATH);
    if (lib == NULL)
    {
        log_error("shaderc not available: could not load " DVZ_SHADERC_LIB_PATH);
        return false;
    }

    /* POSIX allows void* <-> function pointer via memcpy to avoid -Wpedantic warnings. */
#define _SC_SYM(field, name)                                                                      \
    {                                                                                             \
        void* _p = dvz_dynlib_sym(lib, name);                                                     \
        if (_p == NULL)                                                                           \
        {                                                                                         \
            log_error("shaderc: symbol '%s' not found", name);                                    \
            dvz_dynlib_close(lib);                                                                \
            return false;                                                                         \
        }                                                                                         \
        memcpy(&g_shaderc.field, &_p, sizeof(_p));                                                \
    }

    _SC_SYM(compiler_initialize, "shaderc_compiler_initialize")
    _SC_SYM(compiler_release, "shaderc_compiler_release")
    _SC_SYM(compile_options_initialize, "shaderc_compile_options_initialize")
    _SC_SYM(compile_options_release, "shaderc_compile_options_release")
    _SC_SYM(compile_options_set_source_language, "shaderc_compile_options_set_source_language")
    _SC_SYM(compile_options_set_target_env, "shaderc_compile_options_set_target_env")
    _SC_SYM(compile_options_set_target_spirv, "shaderc_compile_options_set_target_spirv")
    _SC_SYM(compile_into_spv, "shaderc_compile_into_spv")
    _SC_SYM(result_get_compilation_status, "shaderc_result_get_compilation_status")
    _SC_SYM(result_get_error_message, "shaderc_result_get_error_message")
    _SC_SYM(result_get_bytes, "shaderc_result_get_bytes")
    _SC_SYM(result_get_length, "shaderc_result_get_length")
    _SC_SYM(result_release, "shaderc_result_release")
#undef _SC_SYM

    /* Keep the library resident — we do not dlclose it. The handle is intentionally retained
       so the symbols remain valid for the process lifetime without re-opening on each call. */
    g_shaderc_available = true;
    return true;
}


/**
 * Return the shaderc GLSL shader kind for a DRP2 shader stage string.
 *
 * @param stage shader stage string
 * @return shaderc shader kind, or infer-from-source when the stage is unknown
 */
static shaderc_shader_kind _vklite_shader_kind(const char* stage)
{
    ANN(stage);
    if (strcmp(stage, "VERTEX") == 0 || strcmp(stage, "vertex") == 0)
        return shaderc_glsl_vertex_shader;
    if (strcmp(stage, "FRAGMENT") == 0 || strcmp(stage, "fragment") == 0)
        return shaderc_glsl_fragment_shader;
    if (strcmp(stage, "COMPUTE") == 0 || strcmp(stage, "compute") == 0)
        return shaderc_glsl_compute_shader;
    return shaderc_glsl_infer_from_source;
}


/**
 * Return the DRP2 runtime object kind for a shader stage string.
 *
 * @param stage shader stage string
 * @return shader object kind, or DRP2_OBJECT_NONE when the stage is unknown
 */
static Drp2ObjectKind _vklite_shader_object_kind(const char* stage)
{
    ANN(stage);
    if (strcmp(stage, "VERTEX") == 0 || strcmp(stage, "vertex") == 0)
        return DRP2_OBJECT_SHADER_VERTEX;
    if (strcmp(stage, "FRAGMENT") == 0 || strcmp(stage, "fragment") == 0)
        return DRP2_OBJECT_SHADER_FRAGMENT;
    if (strcmp(stage, "COMPUTE") == 0 || strcmp(stage, "compute") == 0)
        return DRP2_OBJECT_SHADER_COMPUTE;
    return DRP2_OBJECT_NONE;
}


/**
 * Compile GLSL source code into SPIR-V with shaderc.
 *
 * @param stage shader stage string
 * @param code GLSL source code
 * @param spv output pointer to aligned SPIR-V words, freed by the caller
 * @param spv_size output SPIR-V byte size
 * @return true when compilation succeeds, false otherwise
 */
bool _vklite_compile_glsl(
    const char* stage, const char* code, uint32_t** spv, uint64_t* spv_size)
{
    ANN(stage);
    ANN(code);
    ANN(spv);
    ANN(spv_size);
    *spv = NULL;
    *spv_size = 0;

    if (!_shaderc_load())
        return false;

    shaderc_compiler_t compiler = g_shaderc.compiler_initialize();
    if (compiler == NULL)
        return false;
    shaderc_compile_options_t options = g_shaderc.compile_options_initialize();
    if (options == NULL)
    {
        g_shaderc.compiler_release(compiler);
        return false;
    }

    g_shaderc.compile_options_set_source_language(options, shaderc_source_language_glsl);
    g_shaderc.compile_options_set_target_env(
        options, shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
    g_shaderc.compile_options_set_target_spirv(options, shaderc_spirv_version_1_6);

    shaderc_shader_kind kind = _vklite_shader_kind(stage);
    shaderc_compilation_result_t result = g_shaderc.compile_into_spv(
        compiler, code, strlen(code), kind, "drp2_scene_fixture.glsl", "main", options);

    g_shaderc.compile_options_release(options);
    g_shaderc.compiler_release(compiler);

    if (result == NULL)
        return false;
    if (g_shaderc.result_get_compilation_status(result) != shaderc_compilation_status_success)
    {
        log_error("GLSL compilation failed: %s", g_shaderc.result_get_error_message(result));
        g_shaderc.result_release(result);
        return false;
    }

    const char* bytes = g_shaderc.result_get_bytes(result);
    uint64_t size = (uint64_t)g_shaderc.result_get_length(result);
    if (bytes == NULL || size == 0 || size % sizeof(uint32_t) != 0)
    {
        g_shaderc.result_release(result);
        return false;
    }

    uint32_t* out = (uint32_t*)dvz_calloc((size_t)(size / sizeof(uint32_t)), sizeof(uint32_t));
    if (out == NULL)
    {
        g_shaderc.result_release(result);
        return false;
    }
    dvz_memcpy(out, (size_t)size, bytes, (size_t)size);
    g_shaderc.result_release(result);

    *spv = out;
    *spv_size = size;
    return true;
}


/**
 * Create a vklite shader module object from a DRP2 CreateShaderModule command.
 *
 * @param state vklite runtime state
 * @param command DRP2 CreateShaderModule command
 * @param command_index command index used for validation reporting
 * @return DRP2 validation result
 */
DvzDrp2ValidationResult _vklite_create_shader_module(
    Drp2VkliteState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);
    const char* fmt = command->u.create_shader_module.format;
    bool is_glsl   = (strcmp(fmt, "glsl")  == 0);
    bool is_spirv  = (strcmp(fmt, "spirv") == 0);
    if (!is_glsl && !is_spirv)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_ARGUMENT, command_index);

    Drp2ObjectKind kind = _vklite_shader_object_kind(command->u.create_shader_module.stage);
    if (kind == DRP2_OBJECT_NONE)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_ARGUMENT, command_index);

    uint32_t* spv_owned = NULL;
    const uint32_t* spv = NULL;
    uint64_t spv_size = 0;

    if (is_glsl)
    {
        if (!_vklite_compile_glsl(
                command->u.create_shader_module.stage, command->u.create_shader_module.code,
                &spv_owned, &spv_size))
            return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_ARGUMENT, command_index);
        spv = spv_owned;
    }
    else
    {
        spv_size = command->u.create_shader_module.spirv_size;
        if (spv_size == 0 || (spv_size % sizeof(uint32_t)) != 0)
            return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_ARGUMENT, command_index);
        spv_owned = (uint32_t*)dvz_calloc(spv_size, 1);
        if (spv_owned == NULL)
            return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
        dvz_memcpy(spv_owned, (size_t)spv_size, command->u.create_shader_module.spirv,
            (size_t)spv_size);
        spv = spv_owned;
    }

    Drp2VkliteObject* object = _vklite_add(state, command->u.create_shader_module.id, kind);
    if (object == NULL)
    {
        dvz_free(spv_owned);
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    }

    DvzShader* shader = dvz_shader_create_wrapper();
    if (shader == NULL)
    {
        dvz_free(spv_owned);
        return _vklite_fail_destroy_object(
            object, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    }
    object->shader = shader;

    int out = dvz_shader(state->runtime->device, spv_size, spv, shader);
    dvz_free(spv_owned);
    if (out != 0)
        return _vklite_fail_destroy_object(
            object, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    return _drp2_ok();
}


/**
 * Create an empty pipeline layout for vklite graphics pipelines without bind groups.
 *
 * @param state vklite runtime state
 * @param command_index command index used for validation reporting
 * @param slots output slots wrapper
 * @return DRP2 validation result
 */
static DvzDrp2ValidationResult _vklite_create_empty_slots(
    Drp2VkliteState* state, uint32_t command_index, DvzSlots** slots)
{
    ANN(state);
    ANN(slots);
    *slots = NULL;

    DvzSlots* out = dvz_slots_create_wrapper();
    if (out == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    dvz_slots(state->runtime->device, out);
    if (dvz_slots_create(out) != 0)
    {
        dvz_slots_destroy(out);
        dvz_slots_free(out);
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    }
    *slots = out;
    return _drp2_ok();
}


/**
 * Create a vklite sampler from a DRP2 CreateSampler command.
 *
 * @param state vklite runtime state
 * @param command DRP2 CreateSampler command
 * @param command_index command index used for validation reporting
 * @return DRP2 validation result
 */
DvzDrp2ValidationResult
_vklite_create_sampler(Drp2VkliteState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    Drp2VkliteObject* object = _vklite_add(state, command->u.create_sampler.id, DRP2_OBJECT_SAMPLER);
    if (object == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    DvzSampler* sampler = dvz_sampler_create_wrapper();
    if (sampler == NULL)
        return _vklite_fail_destroy_object(
            object, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    object->sampler = sampler;

    dvz_sampler(state->runtime->device, sampler);
    dvz_sampler_min_filter(sampler, VK_FILTER_LINEAR);
    dvz_sampler_mag_filter(sampler, VK_FILTER_LINEAR);
    dvz_sampler_address_mode(sampler, DVZ_SAMPLER_AXIS_U, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    dvz_sampler_address_mode(sampler, DVZ_SAMPLER_AXIS_V, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    dvz_sampler_address_mode(sampler, DVZ_SAMPLER_AXIS_W, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    if (dvz_sampler_create(sampler) != 0)
        return _vklite_fail_destroy_object(
            object, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    return _drp2_ok();
}


/**
 * Create a vklite bind-group layout from a DRP2 CreateBindGroupLayout command.
 *
 * @param state vklite runtime state
 * @param command DRP2 CreateBindGroupLayout command
 * @param command_index command index used for validation reporting
 * @return DRP2 validation result
 */
DvzDrp2ValidationResult _vklite_create_bind_group_layout(
    Drp2VkliteState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    Drp2VkliteObject* object =
        _vklite_add(state, command->u.create_bind_group_layout.id, DRP2_OBJECT_BIND_GROUP_LAYOUT);
    if (object == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    DvzSlots* slots = dvz_slots_create_wrapper();
    if (slots == NULL)
        return _vklite_fail_destroy_object(
            object, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    object->slots = slots;

    dvz_slots(state->runtime->device, slots);
    if (command->u.create_bind_group_layout.storage_buffers)
    {
        dvz_slots_binding(
            slots, 0, 0, 1, VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
        dvz_slots_binding(
            slots, 0, 1, 1, VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    }
    else if (command->u.create_bind_group_layout.uniform_buffer)
    {
        dvz_slots_binding(
            slots, 0, 0, 1,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    }
    else
    {
        dvz_slots_binding(
            slots, 0, 0, 1, VK_SHADER_STAGE_FRAGMENT_BIT,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    }
    if (dvz_slots_create(slots) != 0)
        return _vklite_fail_destroy_object(
            object, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    return _drp2_ok();
}


/**
 * Create vklite descriptors from a DRP2 CreateBindGroup command.
 *
 * @param state vklite runtime state
 * @param command DRP2 CreateBindGroup command
 * @param command_index command index used for validation reporting
 * @return DRP2 validation result
 */
DvzDrp2ValidationResult _vklite_create_bind_group(
    Drp2VkliteState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    Drp2VkliteObject* layout = _vklite_find(state, command->u.create_bind_group.bind_group_layout_id);
    if (layout == NULL || layout->kind != DRP2_OBJECT_BIND_GROUP_LAYOUT || layout->slots == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    Drp2VkliteObject* object =
        _vklite_add(state, command->u.create_bind_group.id, DRP2_OBJECT_BIND_GROUP);
    if (object == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    layout = _vklite_find(state, command->u.create_bind_group.bind_group_layout_id);
    if (layout == NULL || layout->kind != DRP2_OBJECT_BIND_GROUP_LAYOUT || layout->slots == NULL)
        return _vklite_fail_destroy_object(
            object, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    DvzDescriptors* descriptors = dvz_descriptors_create_wrapper();
    if (descriptors == NULL)
        return _vklite_fail_destroy_object(
            object, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    object->descriptors = descriptors;
    object->texture_id = command->u.create_bind_group.texture_id;
    object->sampler_id = command->u.create_bind_group.sampler_id;
    dvz_descriptors(layout->slots, descriptors);

    if (command->u.create_bind_group.buffer_size != 0 &&
        command->u.create_bind_group.buffer1_id == 0)
    {
        /* Uniform buffer: single buffer with a sub-allocation offset. */
        Drp2VkliteObject* buffer0 = _vklite_find(state, command->u.create_bind_group.buffer0_id);
        if (buffer0 == NULL || buffer0->buffer == NULL)
            return _vklite_fail_destroy_object(
                object, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
        dvz_descriptors_buffer(
            descriptors, 0, 0, 0, dvz_buffer_handle(buffer0->buffer),
            command->u.create_bind_group.buffer0_offset,
            command->u.create_bind_group.buffer_size);
    }
    else if (command->u.create_bind_group.buffer_size != 0)
    {
        /* Storage buffers: two buffers, no offset. */
        Drp2VkliteObject* buffer0 = _vklite_find(state, command->u.create_bind_group.buffer0_id);
        Drp2VkliteObject* buffer1 = _vklite_find(state, command->u.create_bind_group.buffer1_id);
        if (buffer0 == NULL || buffer0->buffer == NULL || buffer1 == NULL ||
            buffer1->buffer == NULL)
            return _vklite_fail_destroy_object(
                object, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
        dvz_descriptors_buffer(
            descriptors, 0, 0, 0, dvz_buffer_handle(buffer0->buffer), 0,
            command->u.create_bind_group.buffer_size);
        dvz_descriptors_buffer(
            descriptors, 0, 1, 0, dvz_buffer_handle(buffer1->buffer), 0,
            command->u.create_bind_group.buffer_size);
    }
    else
    {
        Drp2VkliteObject* texture = _vklite_find(state, command->u.create_bind_group.texture_id);
        Drp2VkliteObject* sampler = _vklite_find(state, command->u.create_bind_group.sampler_id);
        VkImageView texture_view = _vklite_object_image_view(texture);
        if (texture_view == VK_NULL_HANDLE || sampler == NULL || sampler->sampler == NULL)
            return _vklite_fail_destroy_object(
                object, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
        dvz_descriptors_image(
            descriptors, 0, 0, 0, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, texture_view,
            dvz_sampler_handle(sampler->sampler));
    }
    return _drp2_ok();
}


/**
 * Create a vklite graphics pipeline from a DRP2 CreateRenderPipeline command.
 *
 * @param state vklite runtime state
 * @param command DRP2 CreateRenderPipeline command
 * @param command_index command index used for validation reporting
 * @return DRP2 validation result
 */
DvzDrp2ValidationResult _vklite_create_render_pipeline(
    Drp2VkliteState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    Drp2VkliteObject* vertex =
        _vklite_find(state, command->u.create_render_pipeline.vertex_shader_module_id);
    Drp2VkliteObject* fragment =
        _vklite_find(state, command->u.create_render_pipeline.fragment_shader_module_id);
    if (vertex == NULL || vertex->kind != DRP2_OBJECT_SHADER_VERTEX || vertex->shader == NULL ||
        fragment == NULL || fragment->kind != DRP2_OBJECT_SHADER_FRAGMENT ||
        fragment->shader == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    Drp2VkliteObject* object =
        _vklite_add(state, command->u.create_render_pipeline.id, DRP2_OBJECT_RENDER_PIPELINE);
    if (object == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    vertex = _vklite_find(state, command->u.create_render_pipeline.vertex_shader_module_id);
    fragment = _vklite_find(state, command->u.create_render_pipeline.fragment_shader_module_id);
    if (vertex == NULL || vertex->kind != DRP2_OBJECT_SHADER_VERTEX || vertex->shader == NULL ||
        fragment == NULL || fragment->kind != DRP2_OBJECT_SHADER_FRAGMENT ||
        fragment->shader == NULL)
        return _vklite_fail_destroy_object(
            object, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;

    if (command->u.create_render_pipeline.bind_group_layout_id2 != 0)
    {
        /* Two-descriptor-set pipeline: build a combined pipeline layout. */
        Drp2VkliteObject* layout0 =
            _vklite_find(state, command->u.create_render_pipeline.bind_group_layout_id);
        Drp2VkliteObject* layout1 =
            _vklite_find(state, command->u.create_render_pipeline.bind_group_layout_id2);
        if (layout0 == NULL || layout0->kind != DRP2_OBJECT_BIND_GROUP_LAYOUT ||
            layout0->slots == NULL || layout1 == NULL ||
            layout1->kind != DRP2_OBJECT_BIND_GROUP_LAYOUT || layout1->slots == NULL)
            return _vklite_fail_destroy_object(
                object, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
        pipeline_layout = dvz_slots_combined_pipeline_layout(
            state->runtime->device,
            dvz_slots_set_layout(layout0->slots, 0),
            dvz_slots_set_layout(layout1->slots, 0));
        if (pipeline_layout == VK_NULL_HANDLE)
            return _vklite_fail_destroy_object(
                object, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
        object->combined_pipeline_layout = pipeline_layout;
        object->combined_layout_device   = dvz_device_handle(state->runtime->device);
        /* Borrow first layout's slots pointer for bookkeeping; layout is driven by combined. */
        object->slots = layout0->slots;
        object->borrowed_slots = true;
    }
    else if (command->u.create_render_pipeline.bind_group_layout_id != 0)
    {
        Drp2VkliteObject* layout =
            _vklite_find(state, command->u.create_render_pipeline.bind_group_layout_id);
        if (layout == NULL || layout->kind != DRP2_OBJECT_BIND_GROUP_LAYOUT ||
            layout->slots == NULL)
            return _vklite_fail_destroy_object(
                object, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
        pipeline_layout = dvz_slots_handle(layout->slots);
        object->slots = layout->slots;
        object->borrowed_slots = true;
    }
    else
    {
        DvzDrp2ValidationResult result =
            _vklite_create_empty_slots(state, command_index, &object->slots);
        if (!result.ok)
        {
            _vklite_destroy_object(object);
            return result;
        }
        pipeline_layout = dvz_slots_handle(object->slots);
    }

    DvzGraphics* graphics = dvz_graphics_create_wrapper();
    if (graphics == NULL)
        return _vklite_fail_destroy_object(
            object, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    object->graphics = graphics;

    dvz_graphics(state->runtime->device, graphics);
    dvz_graphics_layout(graphics, pipeline_layout);
    dvz_graphics_shader(graphics, VK_SHADER_STAGE_VERTEX_BIT, dvz_shader_handle(vertex->shader));
    dvz_graphics_shader(
        graphics, VK_SHADER_STAGE_FRAGMENT_BIT, dvz_shader_handle(fragment->shader));
    dvz_graphics_attachment_color(graphics, 0, VK_FORMAT_R8G8B8A8_UNORM);
    dvz_graphics_color_write_mask(
        graphics, 0,
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT);
    if (command->u.create_render_pipeline.has_depth_attachment)
    {
        dvz_graphics_attachment_depth(graphics, VK_FORMAT_D32_SFLOAT);
        dvz_graphics_depth(
            graphics, false, command->u.create_render_pipeline.depth_write_enabled,
            (VkCompareOp)command->u.create_render_pipeline.depth_compare_op,
            DVZ_GRAPHICS_FLAGS_FIXED);
    }

    /* binding_count==0 means old-style call (no vertex layout); use TRIANGLE_LIST as default.
       Otherwise respect the topology set via create_render_pipeline_ex. */
    VkPrimitiveTopology topology =
        command->u.create_render_pipeline.binding_count > 0
            ? (VkPrimitiveTopology)command->u.create_render_pipeline.topology
            : VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    dvz_graphics_primitive(graphics, topology, DVZ_GRAPHICS_FLAGS_FIXED);
    dvz_graphics_viewport(graphics, 0, 0, 1, 1, 0, 1, DVZ_GRAPHICS_FLAGS_DYNAMIC);
    dvz_graphics_scissor(graphics, 0, 0, 1, 1, DVZ_GRAPHICS_FLAGS_DYNAMIC);

    /* Vertex input layout (only when explicitly provided). */
    uint32_t nb = command->u.create_render_pipeline.binding_count;
    for (uint32_t i = 0; i < nb; i++)
    {
        VkVertexInputRate input_rate =
            command->u.create_render_pipeline.binding_step_modes[i] ==
                    DVZ_DRP2_VERTEX_STEP_MODE_INSTANCE
                ? VK_VERTEX_INPUT_RATE_INSTANCE
                : VK_VERTEX_INPUT_RATE_VERTEX;
        dvz_graphics_vertex_binding(
            graphics, i, command->u.create_render_pipeline.binding_strides[i],
            input_rate);
    }
    uint32_t na = command->u.create_render_pipeline.attr_count;
    for (uint32_t i = 0; i < na; i++)
        dvz_graphics_vertex_attr(
            graphics,
            command->u.create_render_pipeline.attr_bindings[i],
            command->u.create_render_pipeline.attr_locations[i],
            (VkFormat)command->u.create_render_pipeline.attr_formats[i],
            command->u.create_render_pipeline.attr_offsets[i]);

    if (dvz_graphics_create(graphics) != 0)
        return _vklite_fail_destroy_object(
            object, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    return _drp2_ok();
}


/**
 * Create a vklite compute pipeline from a DRP2 CreateComputePipeline command.
 *
 * @param state vklite runtime state
 * @param command DRP2 CreateComputePipeline command
 * @param command_index command index used for validation reporting
 * @return DRP2 validation result
 */
DvzDrp2ValidationResult _vklite_create_compute_pipeline(
    Drp2VkliteState* state, const DvzDrp2Command* command, uint32_t command_index)
{
    ANN(state);
    ANN(command);

    Drp2VkliteObject* shader =
        _vklite_find(state, command->u.create_compute_pipeline.compute_shader_module_id);
    if (shader == NULL || shader->kind != DRP2_OBJECT_SHADER_COMPUTE || shader->shader == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    Drp2VkliteObject* object =
        _vklite_add(state, command->u.create_compute_pipeline.id, DRP2_OBJECT_COMPUTE_PIPELINE);
    if (object == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    shader = _vklite_find(state, command->u.create_compute_pipeline.compute_shader_module_id);
    if (shader == NULL || shader->kind != DRP2_OBJECT_SHADER_COMPUTE || shader->shader == NULL)
        return _vklite_fail_destroy_object(
            object, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    if (command->u.create_compute_pipeline.bind_group_layout_id != 0)
    {
        Drp2VkliteObject* layout =
            _vklite_find(state, command->u.create_compute_pipeline.bind_group_layout_id);
        if (layout == NULL || layout->kind != DRP2_OBJECT_BIND_GROUP_LAYOUT ||
            layout->slots == NULL)
            return _vklite_fail_destroy_object(
                object, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
        object->slots = layout->slots;
        object->borrowed_slots = true;
    }
    else
    {
        DvzDrp2ValidationResult result =
            _vklite_create_empty_slots(state, command_index, &object->slots);
        if (!result.ok)
        {
            _vklite_destroy_object(object);
            return result;
        }
    }

    DvzCompute* compute = dvz_compute_create_wrapper();
    if (compute == NULL)
        return _vklite_fail_destroy_object(
            object, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    object->compute = compute;

    dvz_compute(state->runtime->device, compute);
    dvz_compute_shader(compute, dvz_shader_handle(shader->shader));
    dvz_compute_layout(compute, dvz_slots_handle(object->slots));
    if (dvz_compute_create(compute) != 0)
        return _vklite_fail_destroy_object(
            object, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    return _drp2_ok();
}



#endif
