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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if DVZ_HAS_SHADERC
#include "shaderc/shaderc.h"
#else
typedef int shaderc_shader_kind;
#endif
#include <volk.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_dynload.h"
#include "_log.h"
#include "_runtime.h"
#include "_stream.h"
#include "_vk_utils.h"



#if DVZ_DRP2_HAS_VKLITE
/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

#if DVZ_HAS_SHADERC
/* Shaderc function table: statically initialized on Windows or lazy-loaded elsewhere. */
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

#if DVZ_SHADERC_STATIC
static ShadercSyms g_shaderc = {
    .compiler_initialize                 = shaderc_compiler_initialize,
    .compiler_release                    = shaderc_compiler_release,
    .compile_options_initialize           = shaderc_compile_options_initialize,
    .compile_options_release              = shaderc_compile_options_release,
    .compile_options_set_source_language  = shaderc_compile_options_set_source_language,
    .compile_options_set_target_env       = shaderc_compile_options_set_target_env,
    .compile_options_set_target_spirv     = shaderc_compile_options_set_target_spirv,
    .compile_into_spv                     = shaderc_compile_into_spv,
    .result_get_compilation_status        = shaderc_result_get_compilation_status,
    .result_get_error_message             = shaderc_result_get_error_message,
    .result_get_bytes                     = shaderc_result_get_bytes,
    .result_get_length                    = shaderc_result_get_length,
    .result_release                       = shaderc_result_release,
};
#else
static ShadercSyms g_shaderc = {0};
static bool g_shaderc_loaded = false;
static bool g_shaderc_available = false;
#endif
#endif


#if DVZ_HAS_SHADERC && !DVZ_SHADERC_STATIC
static DvzDynLib _shaderc_open_runtime_dirs(const char* filename)
{
    ANN(filename);
    const char* dirs = getenv("DVZ_WHEEL_RUNTIME_DIRS");
    if (dirs == NULL || dirs[0] == '\0')
        return NULL;

    const char* basename = strrchr(filename, '/');
#if defined(_WIN32)
    const char* backslash = strrchr(filename, '\\');
    if (basename == NULL || (backslash != NULL && backslash > basename))
        basename = backslash;
#endif
    filename = basename != NULL ? basename + 1 : filename;
    if (filename[0] == '\0')
        return NULL;

#if defined(_WIN32)
    const char sep = ';';
#else
    const char sep = ':';
#endif

    const char* cursor = dirs;
    while (*cursor != '\0')
    {
        const char* end = strchr(cursor, sep);
        size_t dir_len = end == NULL ? strlen(cursor) : (size_t)(end - cursor);
        if (dir_len > 0)
        {
            char path[4096] = {0};
            int rc = snprintf(path, sizeof(path), "%.*s/%s", (int)dir_len, cursor, filename);
            if (rc > 0 && (size_t)rc < sizeof(path))
            {
                DvzDynLib lib = dvz_dynlib_open(path);
                if (lib != NULL)
                    return lib;
            }
        }
        if (end == NULL)
            break;
        cursor = end + 1;
    }
    return NULL;
}
#endif


static VkShaderStageFlags _vklite_stage_flags(uint32_t visibility)
{
    VkShaderStageFlags out = 0;
    if ((visibility & DVZ_DRP2_SHADER_STAGE_VERTEX) != 0)
        out |= VK_SHADER_STAGE_VERTEX_BIT;
    if ((visibility & DVZ_DRP2_SHADER_STAGE_FRAGMENT) != 0)
        out |= VK_SHADER_STAGE_FRAGMENT_BIT;
    if ((visibility & DVZ_DRP2_SHADER_STAGE_COMPUTE) != 0)
        out |= VK_SHADER_STAGE_COMPUTE_BIT;
    return out != 0 ? out : VK_SHADER_STAGE_ALL;
}


/**
 * Return whether a Vulkan format carries a depth aspect.
 *
 * @param format backend-native texture format enum value
 * @return whether the format is a depth format
 */
static bool _vklite_pipeline_format_has_depth(uint32_t format)
{
    switch ((VkFormat)format)
    {
    case VK_FORMAT_D16_UNORM:
    case VK_FORMAT_D32_SFLOAT:
    case VK_FORMAT_D24_UNORM_S8_UINT:
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
        return true;
    default:
        return false;
    }
}



static VkDescriptorType _vklite_descriptor_type(DvzDrp2BindingType type)
{
    switch (type)
    {
    case DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER:
        return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    case DVZ_DRP2_BINDING_TYPE_STORAGE_BUFFER:
        return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    case DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE:
        return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    case DVZ_DRP2_BINDING_TYPE_STORAGE_TEXTURE:
        return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    case DVZ_DRP2_BINDING_TYPE_SAMPLER:
        return VK_DESCRIPTOR_TYPE_SAMPLER;
    case DVZ_DRP2_BINDING_TYPE_NONE:
    default:
        return VK_DESCRIPTOR_TYPE_MAX_ENUM;
    }
}



static VkPipelineLayout _vklite_combined_pipeline_layout(
    DvzDevice* device, uint32_t count, VkDescriptorSetLayout* set_layouts)
{
    ANN(device);
    ANN(set_layouts);
    VkDevice vkd = dvz_device_handle(device);
    ANNVK(vkd);

    VkPipelineLayoutCreateInfo info = {0};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    info.setLayoutCount = count;
    info.pSetLayouts = set_layouts;

    VkPipelineLayout layout = VK_NULL_HANDLE;
    VkResult res = vkCreatePipelineLayout(vkd, &info, NULL, &layout);
    if (vk_result_check(res, __FILE__, __LINE__) != 0)
        return VK_NULL_HANDLE;
    return layout;
}

static bool _shaderc_load(void)
{
#if DVZ_HAS_SHADERC
#if DVZ_SHADERC_STATIC
    return true;
#else
    if (g_shaderc_loaded)
        return g_shaderc_available;
    g_shaderc_loaded = true;

#ifndef DVZ_SHADERC_LIB_PATH
#define DVZ_SHADERC_LIB_PATH "libshaderc_shared.so.1"
#endif
    const char* env_path = getenv("DVZ_SHADERC_RUNTIME_LIBRARY");
    DvzDynLib lib = env_path != NULL && env_path[0] != '\0' ? dvz_dynlib_open(env_path) : NULL;
    if (lib == NULL)
        lib = dvz_dynlib_open(DVZ_SHADERC_LIB_PATH);
    if (lib == NULL)
        lib = _shaderc_open_runtime_dirs(DVZ_SHADERC_LIB_PATH);
#if defined(__APPLE__)
    if (lib == NULL && strcmp(DVZ_SHADERC_LIB_PATH, "@rpath/libshaderc_shared.1.dylib") == 0)
        lib = dvz_dynlib_open("@loader_path/libshaderc_shared.1.dylib");
#endif
    if (lib == NULL)
    {
        log_error(
            "runtime GLSL compilation unavailable: could not load shaderc runtime "
            DVZ_SHADERC_LIB_PATH);
        log_error(
            "install shaderc or set DVZ_SHADERC_RUNTIME_LIBRARY/DVZ_WHEEL_RUNTIME_DIRS so the "
            "runtime library is packaged or discoverable");
        return false;
    }

    /* POSIX allows void* <-> function pointer via byte copy to avoid -Wpedantic warnings. */
#define _SC_SYM(field, name)                                                                      \
    {                                                                                             \
        void* _p = dvz_dynlib_sym(lib, name);                                                     \
        if (_p == NULL)                                                                           \
        {                                                                                         \
            log_error("shaderc: symbol '%s' not found", name);                                    \
            dvz_dynlib_close(lib);                                                                \
            return false;                                                                         \
        }                                                                                         \
        dvz_memcpy(&g_shaderc.field, sizeof(g_shaderc.field), &_p, sizeof(_p));                   \
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
#endif
#else
    log_error(
        "runtime GLSL compilation unavailable: Datoviz was built without shaderc support");
    log_error(
        "install shaderc development files or the Vulkan SDK and rebuild with "
        "-DDVZ_ENABLE_SHADERC=ON to require this feature");
    return false;
#endif
}


/**
 * Return the shaderc GLSL shader kind for a DRP2 shader stage string.
 *
 * @param stage shader stage string
 * @return shaderc shader kind, or infer-from-source when the stage is unknown
 */
static shaderc_shader_kind _vklite_shader_kind(const char* stage)
{
#if DVZ_HAS_SHADERC
    ANN(stage);
    if (strcmp(stage, "VERTEX") == 0 || strcmp(stage, "vertex") == 0)
        return shaderc_glsl_vertex_shader;
    if (strcmp(stage, "FRAGMENT") == 0 || strcmp(stage, "fragment") == 0)
        return shaderc_glsl_fragment_shader;
    if (strcmp(stage, "COMPUTE") == 0 || strcmp(stage, "compute") == 0)
        return shaderc_glsl_compute_shader;
    return shaderc_glsl_infer_from_source;
#else
    (void)stage;
    return 0;
#endif
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

#if DVZ_HAS_SHADERC
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
    shaderc_shader_kind kind = _vklite_shader_kind(stage);
    if (kind == shaderc_glsl_compute_shader)
    {
        /* SPIR-V 1.6 removes the deprecated WorkgroupSize built-in emitted for compute shaders. */
        g_shaderc.compile_options_set_target_env(
            options, shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
        g_shaderc.compile_options_set_target_spirv(options, shaderc_spirv_version_1_6);
    }
    else
    {
        /* SPIR-V 1.6 maps discard to an optional device capability; retain the graphics policy. */
        g_shaderc.compile_options_set_target_env(
            options, shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_0);
        g_shaderc.compile_options_set_target_spirv(options, shaderc_spirv_version_1_0);
    }

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
#else
    log_error(
        "GLSL shader modules require shaderc support, but Datoviz was built without it");
    return false;
#endif
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

    Drp2VkliteObject* previous = _vklite_find(state, command->u.create_sampler.id);
    bool replaced = false;
    if (previous != NULL)
    {
        if (previous->kind != DRP2_OBJECT_SAMPLER)
            return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
        replaced = true;
        if (state->active_borrowed_command_buffer != VK_NULL_HANDLE)
        {
            if (!_vklite_defer_destroy_object(
                    state, previous, state->active_borrowed_command_buffer))
                return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
        }
        else
            _vklite_destroy_object_slot(state, previous);
    }

    Drp2VkliteObject* object = _vklite_add(state, command->u.create_sampler.id, DRP2_OBJECT_SAMPLER);
    if (object == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    DvzSampler* sampler = dvz_sampler_create_wrapper();
    if (sampler == NULL)
        return _vklite_fail_destroy_object(
            object, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    object->sampler = sampler;

    dvz_sampler(state->runtime->device, sampler);
    VkFilter min_filter = command->u.create_sampler.min_filter == DVZ_DRP2_FILTER_NEAREST ?
                              VK_FILTER_NEAREST :
                              VK_FILTER_LINEAR;
    VkFilter mag_filter = command->u.create_sampler.mag_filter == DVZ_DRP2_FILTER_NEAREST ?
                              VK_FILTER_NEAREST :
                              VK_FILTER_LINEAR;
    dvz_sampler_min_filter(sampler, min_filter);
    dvz_sampler_mag_filter(sampler, mag_filter);
    dvz_sampler_address_mode(sampler, DVZ_SAMPLER_AXIS_U, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    dvz_sampler_address_mode(sampler, DVZ_SAMPLER_AXIS_V, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    dvz_sampler_address_mode(sampler, DVZ_SAMPLER_AXIS_W, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    if (dvz_sampler_create(sampler) != 0)
        return _vklite_fail_destroy_object(
            object, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    if (replaced)
    {
        DvzDrp2ValidationResult result = _vklite_refresh_dependent_bind_groups(
            state, command->u.create_sampler.id, command_index);
        if (!result.ok)
            return result;
    }
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
    object->layout_entry_count = command->u.create_bind_group_layout.entry_count;
    dvz_memcpy(
        object->layout_entries, sizeof(object->layout_entries),
        command->u.create_bind_group_layout.entries,
        command->u.create_bind_group_layout.entry_count *
            sizeof(command->u.create_bind_group_layout.entries[0]));

    dvz_slots(state->runtime->device, slots);
    for (uint32_t i = 0; i < command->u.create_bind_group_layout.entry_count; i++)
    {
        const DvzDrp2BindGroupLayoutEntry* entry =
            &command->u.create_bind_group_layout.entries[i];
        VkDescriptorType type = _vklite_descriptor_type(entry->binding_type);
        if (type == VK_DESCRIPTOR_TYPE_MAX_ENUM)
            return _vklite_fail_destroy_object(
                object, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
        dvz_slots_binding(
            slots, 0, entry->binding, 1, _vklite_stage_flags(entry->visibility), type);
    }
    if (dvz_slots_create(slots) != 0)
        return _vklite_fail_destroy_object(
            object, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    return _drp2_ok();
}


/**
 * Build vklite descriptors from one saved bind-group declaration.
 *
 * @param state vklite runtime state
 * @param bind_group bind-group object carrying saved entries
 * @param command_index command index used for validation reporting
 * @return DRP2 validation result
 */
DvzDrp2ValidationResult _vklite_build_bind_group_descriptors(
    Drp2VkliteState* state, const Drp2VkliteObject* bind_group, uint32_t command_index,
    DvzDescriptors** out)
{
    ANN(state);
    ANN(bind_group);
    ANN(out);
    *out = NULL;

    Drp2VkliteObject* layout = _vklite_find(state, bind_group->bind_group_layout_id);
    if (layout == NULL || layout->kind != DRP2_OBJECT_BIND_GROUP_LAYOUT || layout->slots == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    DvzDescriptors* descriptors = dvz_descriptors_create_wrapper();
    if (descriptors == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    dvz_descriptors(layout->slots, descriptors);

    for (uint32_t i = 0; i < bind_group->bind_group_entry_count; i++)
    {
        const DvzDrp2BindGroupEntry* entry = &bind_group->bind_group_entries[i];
        if (entry->binding_type == DVZ_DRP2_BINDING_TYPE_UNIFORM_BUFFER ||
            entry->binding_type == DVZ_DRP2_BINDING_TYPE_STORAGE_BUFFER)
        {
            Drp2VkliteObject* buffer = _vklite_find(state, entry->resource_id);
            if (buffer == NULL || buffer->buffer == NULL)
            {
                dvz_descriptors_free(descriptors);
                return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
            }
            dvz_descriptors_buffer(
                descriptors, 0, entry->binding, 0, dvz_buffer_handle(buffer->buffer),
                entry->offset, entry->size);
        }
        else if (entry->binding_type == DVZ_DRP2_BINDING_TYPE_SAMPLED_TEXTURE ||
                 entry->binding_type == DVZ_DRP2_BINDING_TYPE_STORAGE_TEXTURE)
        {
            Drp2VkliteObject* texture = _vklite_find(state, entry->resource_id);
            VkImageView texture_view = _vklite_object_image_view(texture);
            if (texture_view == VK_NULL_HANDLE)
            {
                dvz_descriptors_free(descriptors);
                return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
            }

            VkSampler sampler_handle = VK_NULL_HANDLE;
            VkImageLayout image_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            if (entry->binding_type == DVZ_DRP2_BINDING_TYPE_STORAGE_TEXTURE)
                image_layout = VK_IMAGE_LAYOUT_GENERAL;
            else if (_vklite_pipeline_format_has_depth(texture->format))
                image_layout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
            dvz_descriptors_image(
                descriptors, 0, entry->binding, 0, image_layout, texture_view, sampler_handle);
        }
        else if (entry->binding_type == DVZ_DRP2_BINDING_TYPE_SAMPLER)
        {
            Drp2VkliteObject* sampler = _vklite_find(state, entry->resource_id);
            if (sampler == NULL || sampler->sampler == NULL)
            {
                dvz_descriptors_free(descriptors);
                return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
            }
            dvz_descriptors_image(
                descriptors, 0, entry->binding, 0, VK_IMAGE_LAYOUT_UNDEFINED, VK_NULL_HANDLE,
                dvz_sampler_handle(sampler->sampler));
        }
    }

    *out = descriptors;
    return _drp2_ok();
}



/**
 * Return whether a bind group references a backend resource id.
 *
 * @param bind_group bind-group object carrying saved entries
 * @param resource_id backend resource id to match
 * @return whether the bind group depends on the resource
 */
static bool _vklite_bind_group_references_resource(
    const Drp2VkliteObject* bind_group, uint64_t resource_id)
{
    ANN(bind_group);
    if (bind_group->kind != DRP2_OBJECT_BIND_GROUP)
        return false;
    for (uint32_t i = 0; i < bind_group->bind_group_entry_count; i++)
    {
        if (bind_group->bind_group_entries[i].resource_id == resource_id)
            return true;
    }
    return false;
}



/**
 * Retire one descriptor wrapper without mutating the live bind-group object.
 *
 * @param state vklite runtime state
 * @param descriptors descriptor wrapper to retire
 * @return whether the wrapper was retired
 */
static bool _vklite_retire_bind_group_descriptors(
    Drp2VkliteState* state, DvzDescriptors* descriptors)
{
    ANN(state);
    if (descriptors == NULL)
        return true;

    if (state->active_borrowed_command_buffer != VK_NULL_HANDLE)
    {
        Drp2VkliteObject retired = {0};
        retired.kind = DRP2_OBJECT_BIND_GROUP;
        retired.descriptors = descriptors;
        return _vklite_defer_destroy_object(
            state, &retired, state->active_borrowed_command_buffer);
    }

    dvz_descriptors_free(descriptors);
    return true;
}



/**
 * Rebuild bind-group descriptors that reference a recreated backend resource id.
 *
 * @param state vklite runtime state
 * @param resource_id recreated resource id
 * @param command_index command index used for validation reporting
 * @return DRP2 validation result
 */
DvzDrp2ValidationResult _vklite_refresh_dependent_bind_groups(
    Drp2VkliteState* state, uint64_t resource_id, uint32_t command_index)
{
    ANN(state);
    if (resource_id == 0)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_ARGUMENT, command_index);

    for (uint32_t i = 0; i < state->count; i++)
    {
        Drp2VkliteObject* object = &state->objects[i];
        if (object->destroyed || object->kind != DRP2_OBJECT_BIND_GROUP)
            continue;
        if (!_vklite_bind_group_references_resource(object, resource_id))
            continue;

        DvzDescriptors* descriptors = NULL;
        DvzDrp2ValidationResult result =
            _vklite_build_bind_group_descriptors(state, object, command_index, &descriptors);
        if (!result.ok)
            return result;

        DvzDescriptors* retired = object->descriptors;
        if (!_vklite_retire_bind_group_descriptors(state, retired))
        {
            dvz_descriptors_free(descriptors);
            return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
        }
        object->descriptors = descriptors;
    }
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

    Drp2VkliteObject* previous = _vklite_find(state, command->u.create_bind_group.id);
    if (previous != NULL)
    {
        if (previous->kind != DRP2_OBJECT_BIND_GROUP)
            return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
        if (state->active_borrowed_command_buffer != VK_NULL_HANDLE)
        {
            if (!_vklite_defer_destroy_object(
                    state, previous, state->active_borrowed_command_buffer))
                return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
        }
        else
            _vklite_destroy_object_slot(state, previous);
    }

    Drp2VkliteObject* object =
        _vklite_add(state, command->u.create_bind_group.id, DRP2_OBJECT_BIND_GROUP);
    if (object == NULL)
        return _drp2_fail(DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);

    object->bind_group_layout_id = command->u.create_bind_group.bind_group_layout_id;
    object->bind_group_entry_count = command->u.create_bind_group.entry_count;
    dvz_memcpy(
        object->bind_group_entries, sizeof(object->bind_group_entries),
        command->u.create_bind_group.entries,
        command->u.create_bind_group.entry_count *
            sizeof(command->u.create_bind_group.entries[0]));

    DvzDescriptors* descriptors = NULL;
    DvzDrp2ValidationResult result =
        _vklite_build_bind_group_descriptors(state, object, command_index, &descriptors);
    if (!result.ok)
    {
        _vklite_destroy_object(object);
        return result;
    }
    object->descriptors = descriptors;
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

    if (command->u.create_render_pipeline.bind_group_layout_count > 1)
    {
        VkDescriptorSetLayout set_layouts[DVZ_DRP2_MAX_BIND_GROUPS] = {0};
        Drp2VkliteObject* layout0 = NULL;
        for (uint32_t i = 0; i < command->u.create_render_pipeline.bind_group_layout_count; i++)
        {
            Drp2VkliteObject* layout =
                _vklite_find(state, command->u.create_render_pipeline.bind_group_layout_ids[i]);
            if (layout == NULL || layout->kind != DRP2_OBJECT_BIND_GROUP_LAYOUT ||
                layout->slots == NULL)
                return _vklite_fail_destroy_object(
                    object, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
            if (i == 0)
                layout0 = layout;
            set_layouts[i] = dvz_slots_set_layout(layout->slots, 0);
        }
        pipeline_layout = _vklite_combined_pipeline_layout(
            state->runtime->device, command->u.create_render_pipeline.bind_group_layout_count,
            set_layouts);
        if (pipeline_layout == VK_NULL_HANDLE)
            return _vklite_fail_destroy_object(
                object, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
        object->combined_pipeline_layout = pipeline_layout;
        object->combined_layout_device   = dvz_device_handle(state->runtime->device);
        /* Borrow first layout's slots pointer for bookkeeping; layout is driven by combined. */
        object->slots = layout0 != NULL ? layout0->slots : NULL;
        object->borrowed_slots = true;
    }
    else if (command->u.create_render_pipeline.bind_group_layout_count == 1)
    {
        Drp2VkliteObject* layout =
            _vklite_find(state, command->u.create_render_pipeline.bind_group_layout_ids[0]);
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
    uint32_t color_target_count = command->u.create_render_pipeline.color_target_count;
    if (color_target_count == 0)
        color_target_count = 1;
    if (color_target_count > DVZ_DRP2_MAX_COLOR_ATTACHMENTS)
        return _vklite_fail_destroy_object(
            object, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    for (uint32_t i = 0; i < color_target_count; i++)
    {
        const DvzDrp2ColorTarget* target = &command->u.create_render_pipeline.color_targets[i];
        VkFormat format = target->format != 0 ? (VkFormat)target->format :
                                               VK_FORMAT_R8G8B8A8_UNORM;
        uint32_t mask = target->color_write_mask != 0 ? target->color_write_mask :
                                                     (VK_COLOR_COMPONENT_R_BIT |
                                                      VK_COLOR_COMPONENT_G_BIT |
                                                      VK_COLOR_COMPONENT_B_BIT |
                                                      VK_COLOR_COMPONENT_A_BIT);
        dvz_graphics_attachment_color(graphics, i, format);
        dvz_graphics_color_write_mask(graphics, i, mask);
        if (target->blend_enabled)
        {
            dvz_graphics_blend_color(
                graphics, i, (VkBlendFactor)target->src_color_blend_factor,
                (VkBlendFactor)target->dst_color_blend_factor,
                (VkBlendOp)target->color_blend_op, mask);
            dvz_graphics_blend_alpha(
                graphics, i, (VkBlendFactor)target->src_alpha_blend_factor,
                (VkBlendFactor)target->dst_alpha_blend_factor,
                (VkBlendOp)target->alpha_blend_op);
        }
    }
    if (command->u.create_render_pipeline.has_depth_attachment)
    {
        dvz_graphics_attachment_depth(graphics, VK_FORMAT_D32_SFLOAT);
        dvz_graphics_depth(
            graphics, false, command->u.create_render_pipeline.depth_write_enabled,
            (VkCompareOp)command->u.create_render_pipeline.depth_compare_op,
            DVZ_GRAPHICS_FLAGS_FIXED);
    }
    dvz_graphics_multisampling(
        graphics, _vklite_sample_count(command->u.create_render_pipeline.sample_count), 0.0f,
        command->u.create_render_pipeline.alpha_to_coverage_enabled);

    /* binding_count==0 means old-style call (no vertex layout); use TRIANGLE_LIST as default.
       Otherwise respect the topology set via create_render_pipeline_ex. */
    VkPrimitiveTopology topology =
        command->u.create_render_pipeline.binding_count > 0
            ? (VkPrimitiveTopology)command->u.create_render_pipeline.topology
            : VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    dvz_graphics_primitive(graphics, topology, DVZ_GRAPHICS_FLAGS_FIXED);
    if (command->u.create_render_pipeline.has_raster_state)
    {
        dvz_graphics_cull_mode(
            graphics, (VkCullModeFlags)command->u.create_render_pipeline.cull_mode,
            DVZ_GRAPHICS_FLAGS_FIXED);
        dvz_graphics_front_face(
            graphics, (VkFrontFace)command->u.create_render_pipeline.front_face,
            DVZ_GRAPHICS_FLAGS_FIXED);
    }
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

    VkPipelineLayout compute_layout = VK_NULL_HANDLE;
    if (command->u.create_compute_pipeline.bind_group_layout_count > 1)
    {
        VkDescriptorSetLayout set_layouts[DVZ_DRP2_MAX_BIND_GROUPS] = {0};
        Drp2VkliteObject* layout0 = NULL;
        for (uint32_t i = 0; i < command->u.create_compute_pipeline.bind_group_layout_count; i++)
        {
            Drp2VkliteObject* layout =
                _vklite_find(state, command->u.create_compute_pipeline.bind_group_layout_ids[i]);
            if (layout == NULL || layout->kind != DRP2_OBJECT_BIND_GROUP_LAYOUT ||
                layout->slots == NULL)
                return _vklite_fail_destroy_object(
                    object, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
            if (i == 0)
                layout0 = layout;
            set_layouts[i] = dvz_slots_set_layout(layout->slots, 0);
        }
        compute_layout = _vklite_combined_pipeline_layout(
            state->runtime->device, command->u.create_compute_pipeline.bind_group_layout_count,
            set_layouts);
        if (compute_layout == VK_NULL_HANDLE)
            return _vklite_fail_destroy_object(
                object, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
        object->combined_pipeline_layout = compute_layout;
        object->combined_layout_device   = dvz_device_handle(state->runtime->device);
        object->slots = layout0 != NULL ? layout0->slots : NULL;
        object->borrowed_slots = true;
    }
    else if (command->u.create_compute_pipeline.bind_group_layout_count == 1)
    {
        Drp2VkliteObject* layout =
            _vklite_find(state, command->u.create_compute_pipeline.bind_group_layout_ids[0]);
        if (layout == NULL || layout->kind != DRP2_OBJECT_BIND_GROUP_LAYOUT ||
            layout->slots == NULL)
            return _vklite_fail_destroy_object(
                object, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
        object->slots = layout->slots;
        object->borrowed_slots = true;
        compute_layout = dvz_slots_handle(object->slots);
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
        compute_layout = dvz_slots_handle(object->slots);
    }

    DvzCompute* compute = dvz_compute_create_wrapper();
    if (compute == NULL)
        return _vklite_fail_destroy_object(
            object, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    object->compute = compute;

    dvz_compute(state->runtime->device, compute);
    dvz_compute_shader(compute, dvz_shader_handle(shader->shader));
    dvz_compute_layout(compute, compute_layout);
    if (dvz_compute_create(compute) != 0)
        return _vklite_fail_destroy_object(
            object, DVZ_DRP2_VALIDATION_INVALID_STATE, command_index);
    return _drp2_ok();
}



#endif
