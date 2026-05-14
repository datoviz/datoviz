/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  App trace helpers                                                                            */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_trace.h"

#include <inttypes.h>
#include <stdarg.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "../drp2/_stream.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_APP_TRACE_FNV_OFFSET    1469598103934665603ULL
#define DVZ_APP_TRACE_FNV_PRIME     1099511628211ULL
#define DVZ_APP_TRACE_INITIAL_LINES 32



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct TracePassMap TracePassMap;


struct TracePassMap
{
    uint64_t id;
    uint32_t ordinal;
};



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Extend one FNV-1a hash with a byte span.
 *
 * @param hash current hash
 * @param data byte span pointer
 * @param size byte span size
 * @return updated hash
 */
static uint64_t _trace_hash_bytes(uint64_t hash, const void* data, uint64_t size)
{
    ANN(data);
    const uint8_t* bytes = (const uint8_t*)data;
    for (uint64_t i = 0; i < size; i++)
    {
        hash ^= (uint64_t)bytes[i];
        hash *= DVZ_APP_TRACE_FNV_PRIME;
    }
    return hash;
}



/**
 * Extend one FNV-1a hash with an unsigned 64-bit value.
 *
 * @param hash current hash
 * @param value value to hash
 * @return updated hash
 */
static uint64_t _trace_hash_u64(uint64_t hash, uint64_t value)
{
    return _trace_hash_bytes(hash, &value, sizeof(value));
}



/**
 * Extend one FNV-1a hash with an unsigned 32-bit value.
 *
 * @param hash current hash
 * @param value value to hash
 * @return updated hash
 */
static uint64_t _trace_hash_u32(uint64_t hash, uint32_t value)
{
    return _trace_hash_bytes(hash, &value, sizeof(value));
}



/**
 * Extend one FNV-1a hash with a boolean value.
 *
 * @param hash current hash
 * @param value value to hash
 * @return updated hash
 */
static uint64_t _trace_hash_bool(uint64_t hash, bool value)
{
    return _trace_hash_u32(hash, value ? 1u : 0u);
}



/**
 * Extend one FNV-1a hash with a float value.
 *
 * @param hash current hash
 * @param value value to hash
 * @return updated hash
 */
static uint64_t _trace_hash_f32(uint64_t hash, float value)
{
    return _trace_hash_bytes(hash, &value, sizeof(value));
}



/**
 * Extend one FNV-1a hash with a NUL-terminated string.
 *
 * @param hash current hash
 * @param value value to hash
 * @return updated hash
 */
static uint64_t _trace_hash_string(uint64_t hash, const char* value)
{
    if (value == NULL)
        return _trace_hash_u64(hash, 0);
    uint64_t len = (uint64_t)strlen(value);
    hash = _trace_hash_u64(hash, len);
    if (len > 0)
        hash = _trace_hash_bytes(hash, value, len);
    return hash;
}



/**
 * Extend one FNV-1a hash with one stable command payload.
 *
 * @param hash current hash
 * @param command source command
 * @return updated hash
 */
static uint64_t _trace_hash_command(uint64_t hash, const DvzDrp2Command* command)
{
    ANN(command);
    hash = _trace_hash_u32(hash, (uint32_t)command->type);

    switch (command->type)
    {
    case DVZ_DRP2_COMMAND_HELLO_RENDERER:
    case DVZ_DRP2_COMMAND_RENDERER_HELLO_REPLY:
        return _trace_hash_string(hash, command->u.handshake.name);
    case DVZ_DRP2_COMMAND_CREATE_BUFFER:
        hash = _trace_hash_u64(hash, command->u.create_buffer.id);
        hash = _trace_hash_u64(hash, command->u.create_buffer.size);
        return _trace_hash_u32(hash, command->u.create_buffer.usage);
    case DVZ_DRP2_COMMAND_DESTROY_BUFFER:
        return _trace_hash_u64(hash, command->u.destroy_buffer.buffer_id);
    case DVZ_DRP2_COMMAND_CREATE_TEXTURE:
        hash = _trace_hash_u64(hash, command->u.create_texture.id);
        hash = _trace_hash_u32(hash, command->u.create_texture.width);
        hash = _trace_hash_u32(hash, command->u.create_texture.height);
        hash = _trace_hash_u32(hash, command->u.create_texture.depth);
        return _trace_hash_u32(hash, command->u.create_texture.usage);
    case DVZ_DRP2_COMMAND_DESTROY_TEXTURE:
        return _trace_hash_u64(hash, command->u.destroy_texture.texture_id);
    case DVZ_DRP2_COMMAND_CREATE_SHADER_MODULE:
        hash = _trace_hash_u64(hash, command->u.create_shader_module.id);
        hash = _trace_hash_string(hash, command->u.create_shader_module.stage);
        return _trace_hash_string(hash, command->u.create_shader_module.format);
    case DVZ_DRP2_COMMAND_DESTROY_SHADER_MODULE:
        return _trace_hash_u64(hash, command->u.destroy_shader_module.shader_module_id);
    case DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE:
        hash = _trace_hash_u64(hash, command->u.create_render_pipeline.id);
        hash = _trace_hash_u64(hash, command->u.create_render_pipeline.vertex_shader_module_id);
        hash = _trace_hash_u64(hash, command->u.create_render_pipeline.fragment_shader_module_id);
        hash = _trace_hash_u32(hash, command->u.create_render_pipeline.vertex_buffer_slots);
        hash = _trace_hash_u64(hash, command->u.create_render_pipeline.bind_group_layout_id);
        hash = _trace_hash_u64(hash, command->u.create_render_pipeline.bind_group_layout_id2);
        hash = _trace_hash_bool(hash, command->u.create_render_pipeline.has_depth_attachment);
        hash = _trace_hash_bool(hash, command->u.create_render_pipeline.depth_write_enabled);
        hash = _trace_hash_u32(hash, command->u.create_render_pipeline.depth_compare_op);
        hash = _trace_hash_u32(hash, command->u.create_render_pipeline.topology);
        hash = _trace_hash_u32(hash, command->u.create_render_pipeline.binding_count);
        for (uint32_t i = 0; i < command->u.create_render_pipeline.binding_count && i < 16; i++)
            hash = _trace_hash_u32(hash, command->u.create_render_pipeline.binding_strides[i]);
        hash = _trace_hash_u32(hash, command->u.create_render_pipeline.attr_count);
        for (uint32_t i = 0; i < command->u.create_render_pipeline.attr_count && i < 16; i++)
        {
            hash = _trace_hash_u32(hash, command->u.create_render_pipeline.attr_bindings[i]);
            hash = _trace_hash_u32(hash, command->u.create_render_pipeline.attr_locations[i]);
            hash = _trace_hash_u32(hash, command->u.create_render_pipeline.attr_formats[i]);
            hash = _trace_hash_u32(hash, command->u.create_render_pipeline.attr_offsets[i]);
        }
        return hash;
    case DVZ_DRP2_COMMAND_DESTROY_RENDER_PIPELINE:
        return _trace_hash_u64(hash, command->u.destroy_render_pipeline.render_pipeline_id);
    case DVZ_DRP2_COMMAND_CREATE_COMPUTE_PIPELINE:
        hash = _trace_hash_u64(hash, command->u.create_compute_pipeline.id);
        hash = _trace_hash_u64(hash, command->u.create_compute_pipeline.compute_shader_module_id);
        return _trace_hash_u64(hash, command->u.create_compute_pipeline.bind_group_layout_id);
    case DVZ_DRP2_COMMAND_DESTROY_COMPUTE_PIPELINE:
        return _trace_hash_u64(hash, command->u.destroy_compute_pipeline.compute_pipeline_id);
    case DVZ_DRP2_COMMAND_CREATE_SAMPLER:
        return _trace_hash_u64(hash, command->u.create_sampler.id);
    case DVZ_DRP2_COMMAND_CREATE_BIND_GROUP_LAYOUT:
        hash = _trace_hash_u64(hash, command->u.create_bind_group_layout.id);
        hash = _trace_hash_bool(hash, command->u.create_bind_group_layout.storage_buffers);
        return _trace_hash_bool(hash, command->u.create_bind_group_layout.uniform_buffer);
    case DVZ_DRP2_COMMAND_CREATE_BIND_GROUP:
        hash = _trace_hash_u64(hash, command->u.create_bind_group.id);
        hash = _trace_hash_u64(hash, command->u.create_bind_group.bind_group_layout_id);
        hash = _trace_hash_u64(hash, command->u.create_bind_group.texture_id);
        hash = _trace_hash_u64(hash, command->u.create_bind_group.sampler_id);
        hash = _trace_hash_u64(hash, command->u.create_bind_group.buffer0_id);
        hash = _trace_hash_u64(hash, command->u.create_bind_group.buffer1_id);
        hash = _trace_hash_u64(hash, command->u.create_bind_group.buffer_size);
        return _trace_hash_u64(hash, command->u.create_bind_group.buffer0_offset);
    case DVZ_DRP2_COMMAND_DESTROY_BIND_GROUP_LAYOUT:
        return _trace_hash_u64(hash, command->u.destroy_bind_group_layout.bind_group_layout_id);
    case DVZ_DRP2_COMMAND_DESTROY_BIND_GROUP:
        return _trace_hash_u64(hash, command->u.destroy_bind_group.bind_group_id);
    case DVZ_DRP2_COMMAND_WRITE_BUFFER:
        hash = _trace_hash_u64(hash, command->u.write_buffer.buffer_id);
        hash = _trace_hash_u64(hash, command->u.write_buffer.offset);
        return _trace_hash_u64(hash, command->u.write_buffer.size);
    case DVZ_DRP2_COMMAND_WRITE_TEXTURE:
        hash = _trace_hash_u64(hash, command->u.write_texture.texture_id);
        hash = _trace_hash_u32(hash, command->u.write_texture.mip_level);
        hash = _trace_hash_u32(hash, command->u.write_texture.origin_x);
        hash = _trace_hash_u32(hash, command->u.write_texture.origin_y);
        hash = _trace_hash_u32(hash, command->u.write_texture.origin_z);
        hash = _trace_hash_u32(hash, command->u.write_texture.width);
        hash = _trace_hash_u32(hash, command->u.write_texture.height);
        hash = _trace_hash_u32(hash, command->u.write_texture.depth);
        hash = _trace_hash_u32(hash, command->u.write_texture.bytes_per_row);
        return _trace_hash_u32(hash, command->u.write_texture.rows_per_image);
    case DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS:
        hash = _trace_hash_u64(hash, command->u.begin_render_pass.texture_id);
        hash = _trace_hash_bool(hash, command->u.begin_render_pass.has_depth_attachment);
        hash = _trace_hash_f32(hash, command->u.begin_render_pass.clear_depth);
        for (uint32_t i = 0; i < 4; i++)
            hash = _trace_hash_f32(hash, command->u.begin_render_pass.clear_color[i]);
        for (uint32_t i = 0; i < 4; i++)
            hash = _trace_hash_f32(hash, command->u.begin_render_pass.viewport[i]);
        return _trace_hash_bool(hash, command->u.begin_render_pass.clear);
    case DVZ_DRP2_COMMAND_SET_VIEWPORT:
        for (uint32_t i = 0; i < 4; i++)
            hash = _trace_hash_f32(hash, command->u.set_viewport.viewport[i]);
        return hash;
    case DVZ_DRP2_COMMAND_SET_SCISSOR:
        for (uint32_t i = 0; i < 4; i++)
            hash = _trace_hash_f32(hash, command->u.set_scissor.scissor[i]);
        return hash;
    case DVZ_DRP2_COMMAND_SET_PIPELINE:
        return _trace_hash_u64(hash, command->u.set_pipeline.pipeline_id);
    case DVZ_DRP2_COMMAND_SET_BIND_GROUP:
        hash = _trace_hash_u32(hash, command->u.set_bind_group.slot);
        return _trace_hash_u64(hash, command->u.set_bind_group.bind_group_id);
    case DVZ_DRP2_COMMAND_SET_VERTEX_BUFFER:
        hash = _trace_hash_u32(hash, command->u.set_vertex_buffer.slot);
        hash = _trace_hash_u64(hash, command->u.set_vertex_buffer.buffer_id);
        return _trace_hash_u64(hash, command->u.set_vertex_buffer.offset);
    case DVZ_DRP2_COMMAND_SET_INDEX_BUFFER:
        hash = _trace_hash_u64(hash, command->u.set_index_buffer.buffer_id);
        hash = _trace_hash_string(hash, command->u.set_index_buffer.index_format);
        return _trace_hash_u64(hash, command->u.set_index_buffer.offset);
    case DVZ_DRP2_COMMAND_DRAW:
        hash = _trace_hash_u32(hash, command->u.draw.vertex_count);
        hash = _trace_hash_u32(hash, command->u.draw.instance_count);
        hash = _trace_hash_u32(hash, command->u.draw.first_vertex);
        return _trace_hash_u32(hash, command->u.draw.first_instance);
    case DVZ_DRP2_COMMAND_DRAW_INDEXED:
        hash = _trace_hash_u32(hash, command->u.draw_indexed.index_count);
        hash = _trace_hash_u32(hash, command->u.draw_indexed.instance_count);
        hash = _trace_hash_u32(hash, command->u.draw_indexed.first_index);
        hash = _trace_hash_u32(hash, (uint32_t)command->u.draw_indexed.base_vertex);
        return _trace_hash_u32(hash, command->u.draw_indexed.first_instance);
    case DVZ_DRP2_COMMAND_DISPATCH_WORKGROUPS:
        hash = _trace_hash_u32(hash, command->u.dispatch.x);
        hash = _trace_hash_u32(hash, command->u.dispatch.y);
        return _trace_hash_u32(hash, command->u.dispatch.z);
    case DVZ_DRP2_COMMAND_COPY_BUFFER_TO_BUFFER:
        hash = _trace_hash_u64(hash, command->u.copy_buffer_to_buffer.src_buffer_id);
        hash = _trace_hash_u64(hash, command->u.copy_buffer_to_buffer.src_offset);
        hash = _trace_hash_u64(hash, command->u.copy_buffer_to_buffer.dst_buffer_id);
        hash = _trace_hash_u64(hash, command->u.copy_buffer_to_buffer.dst_offset);
        return _trace_hash_u64(hash, command->u.copy_buffer_to_buffer.size);
    case DVZ_DRP2_COMMAND_COPY_BUFFER_TO_TEXTURE:
        hash = _trace_hash_u64(hash, command->u.copy_buffer_to_texture.src_buffer_id);
        hash = _trace_hash_u64(hash, command->u.copy_buffer_to_texture.src_offset);
        hash = _trace_hash_u32(hash, command->u.copy_buffer_to_texture.bytes_per_row);
        hash = _trace_hash_u32(hash, command->u.copy_buffer_to_texture.rows_per_image);
        hash = _trace_hash_u64(hash, command->u.copy_buffer_to_texture.dst_texture_id);
        hash = _trace_hash_u32(hash, command->u.copy_buffer_to_texture.dst_mip_level);
        hash = _trace_hash_u32(hash, command->u.copy_buffer_to_texture.dst_origin_x);
        hash = _trace_hash_u32(hash, command->u.copy_buffer_to_texture.dst_origin_y);
        hash = _trace_hash_u32(hash, command->u.copy_buffer_to_texture.dst_origin_z);
        hash = _trace_hash_u32(hash, command->u.copy_buffer_to_texture.width);
        hash = _trace_hash_u32(hash, command->u.copy_buffer_to_texture.height);
        return _trace_hash_u32(hash, command->u.copy_buffer_to_texture.depth);
    case DVZ_DRP2_COMMAND_COPY_TEXTURE_TO_BUFFER:
        hash = _trace_hash_u64(hash, command->u.copy_texture_to_buffer.src_texture_id);
        hash = _trace_hash_u64(hash, command->u.copy_texture_to_buffer.dst_buffer_id);
        hash = _trace_hash_u64(hash, command->u.copy_texture_to_buffer.dst_offset);
        hash = _trace_hash_u32(hash, command->u.copy_texture_to_buffer.width);
        hash = _trace_hash_u32(hash, command->u.copy_texture_to_buffer.height);
        hash = _trace_hash_u32(hash, command->u.copy_texture_to_buffer.bytes_per_row);
        return _trace_hash_u32(hash, command->u.copy_texture_to_buffer.rows_per_image);
    case DVZ_DRP2_COMMAND_COPY_TEXTURE_TO_TEXTURE:
        hash = _trace_hash_u64(hash, command->u.copy_texture_to_texture.src_texture_id);
        hash = _trace_hash_u32(hash, command->u.copy_texture_to_texture.src_mip_level);
        hash = _trace_hash_u32(hash, command->u.copy_texture_to_texture.src_origin_x);
        hash = _trace_hash_u32(hash, command->u.copy_texture_to_texture.src_origin_y);
        hash = _trace_hash_u32(hash, command->u.copy_texture_to_texture.src_origin_z);
        hash = _trace_hash_u64(hash, command->u.copy_texture_to_texture.dst_texture_id);
        hash = _trace_hash_u32(hash, command->u.copy_texture_to_texture.dst_mip_level);
        hash = _trace_hash_u32(hash, command->u.copy_texture_to_texture.dst_origin_x);
        hash = _trace_hash_u32(hash, command->u.copy_texture_to_texture.dst_origin_y);
        hash = _trace_hash_u32(hash, command->u.copy_texture_to_texture.dst_origin_z);
        hash = _trace_hash_u32(hash, command->u.copy_texture_to_texture.width);
        hash = _trace_hash_u32(hash, command->u.copy_texture_to_texture.height);
        return _trace_hash_u32(hash, command->u.copy_texture_to_texture.depth);
    case DVZ_DRP2_COMMAND_QUEUE_SUBMIT:
    case DVZ_DRP2_COMMAND_QUEUE_SUBMIT_REPLY:
        hash = _trace_hash_bool(hash, command->u.queue_submit.has_readback);
        hash = _trace_hash_u64(hash, command->u.queue_submit.buffer_id);
        hash = _trace_hash_u64(hash, command->u.queue_submit.offset);
        return _trace_hash_u64(hash, command->u.queue_submit.size);
    case DVZ_DRP2_COMMAND_BEGIN_COMMAND_ENCODER:
    case DVZ_DRP2_COMMAND_BEGIN_COMPUTE_PASS:
    case DVZ_DRP2_COMMAND_END_RENDER_PASS:
    case DVZ_DRP2_COMMAND_END_COMPUTE_PASS:
    case DVZ_DRP2_COMMAND_FINISH_COMMAND_ENCODER:
    case DVZ_DRP2_COMMAND_NONE:
    default:
        return hash;
    }
}



/**
 * Ensure a normalized snapshot can append one more line.
 *
 * @param snapshot the snapshot
 * @return whether the snapshot has append capacity
 */
static bool _trace_snapshot_ensure(DvzAppTraceSnapshot* snapshot)
{
    ANN(snapshot);
    if (snapshot->lines == NULL || snapshot->capacity == 0)
    {
        snapshot->capacity = DVZ_APP_TRACE_INITIAL_LINES;
        snapshot->lines =
            (DvzAppTraceLine*)dvz_calloc(snapshot->capacity, sizeof(DvzAppTraceLine));
        return snapshot->lines != NULL;
    }
    if (snapshot->count < snapshot->capacity)
        return true;
    if (snapshot->capacity > UINT32_MAX / 2)
        return false;

    uint32_t old_capacity = snapshot->capacity;
    uint32_t capacity = old_capacity * 2;
    DvzAppTraceLine* lines =
        (DvzAppTraceLine*)dvz_realloc(snapshot->lines, capacity * sizeof(DvzAppTraceLine));
    if (lines == NULL)
        return false;
    dvz_memset(
        &lines[old_capacity], (capacity - old_capacity) * sizeof(DvzAppTraceLine), 0,
        (capacity - old_capacity) * sizeof(DvzAppTraceLine));
    snapshot->capacity = capacity;
    snapshot->lines = lines;
    return true;
}



/**
 * Append one formatted line to a normalized snapshot.
 *
 * @param snapshot destination snapshot
 * @param format printf-style format
 * @return whether the line was appended
 */
static bool _trace_snapshot_append(DvzAppTraceSnapshot* snapshot, const char* format, ...)
{
    ANN(snapshot);
    ANN(format);
    if (!_trace_snapshot_ensure(snapshot))
        return false;

    va_list args;
    va_start(args, format);
    int written = dvz_vsnprintf(
        snapshot->lines[snapshot->count].text, sizeof(snapshot->lines[snapshot->count].text),
        format, args);
    va_end(args);
    if (written < 0 || (uint32_t)written >= sizeof(snapshot->lines[snapshot->count].text))
        return false;
    snapshot->count++;
    return true;
}


/**
 * Format one compact trace suffix and reject truncation.
 *
 * @param out destination character buffer
 * @param size destination buffer size in bytes
 * @param format printf-style format
 * @return whether the full suffix fit in the destination buffer
 */
static bool _trace_format_suffix(char* out, uint32_t size, const char* format, ...)
{
    ANN(out);
    ANN(format);
    ASSERT(size > 0);

    va_list args;
    va_start(args, format);
    int written = dvz_vsnprintf(out, size, format, args);
    va_end(args);
    return written >= 0 && (uint32_t)written < size;
}



/**
 * Return the display ordinal for a transient pass id.
 *
 * @param passes pass map
 * @param count pass count
 * @param id transient DRP2 pass id
 * @return pass ordinal, or UINT32_MAX if unknown
 */
static uint32_t _trace_pass_ordinal(const TracePassMap* passes, uint32_t count, uint64_t id)
{
    if (passes == NULL)
        return UINT32_MAX;
    for (uint32_t i = 0; i < count; i++)
    {
        if (passes[i].id == id)
            return passes[i].ordinal;
    }
    return UINT32_MAX;
}



/**
 * Return whether a normalized rectangle covers the full render target.
 *
 * @param values normalized x, y, width, height
 * @return true if the rectangle is full target
 */
static bool _trace_rect_full(const float values[4])
{
    ANN(values);
    return values[0] == 0.0f && values[1] == 0.0f && values[2] == 1.0f && values[3] == 1.0f;
}



/**
 * Append a pass-qualified line, using a fallback when no pass ordinal is known.
 *
 * @param snapshot destination snapshot
 * @param pass_kind pass kind label
 * @param ordinal pass ordinal
 * @param suffix line suffix
 * @return whether the line was appended
 */
static bool _trace_snapshot_append_pass_line(
    DvzAppTraceSnapshot* snapshot, const char* pass_kind, uint32_t ordinal, const char* suffix)
{
    ANN(snapshot);
    ANN(pass_kind);
    ANN(suffix);
    if (ordinal == UINT32_MAX)
        return _trace_snapshot_append(snapshot, "%s<?> %s", pass_kind, suffix);
    return _trace_snapshot_append(snapshot, "%s#%" PRIu32 " %s", pass_kind, ordinal, suffix);
}



/**
 * Append the compact normalized representation for one command.
 *
 * @param snapshot destination snapshot
 * @param command source command
 * @param passes transient pass map
 * @param pass_count pass map count
 * @return whether the command was represented successfully
 */
static bool _trace_snapshot_append_command(
    DvzAppTraceSnapshot* snapshot, const DvzDrp2Command* command, const TracePassMap* passes,
    uint32_t pass_count)
{
    ANN(snapshot);
    ANN(command);
    char suffix[160] = {0};
    uint32_t ordinal = UINT32_MAX;

    switch (command->type)
    {
    case DVZ_DRP2_COMMAND_CREATE_BUFFER:
        return _trace_snapshot_append(
            snapshot, "+ buffer id=%" PRIu64 " size=%" PRIu64 " usage=0x%" PRIx32,
            command->u.create_buffer.id, command->u.create_buffer.size,
            command->u.create_buffer.usage);
    case DVZ_DRP2_COMMAND_DESTROY_BUFFER:
        return _trace_snapshot_append(
            snapshot, "- buffer id=%" PRIu64, command->u.destroy_buffer.buffer_id);
    case DVZ_DRP2_COMMAND_CREATE_TEXTURE:
        return _trace_snapshot_append(
            snapshot, "+ texture id=%" PRIu64 " size=%" PRIu32 "x%" PRIu32 "x%" PRIu32
                      " usage=0x%" PRIx32,
            command->u.create_texture.id, command->u.create_texture.width,
            command->u.create_texture.height, command->u.create_texture.depth,
            command->u.create_texture.usage);
    case DVZ_DRP2_COMMAND_DESTROY_TEXTURE:
        return _trace_snapshot_append(
            snapshot, "- texture id=%" PRIu64, command->u.destroy_texture.texture_id);
    case DVZ_DRP2_COMMAND_CREATE_SHADER_MODULE:
        return _trace_snapshot_append(
            snapshot, "+ shader id=%" PRIu64 " stage=%s format=%s",
            command->u.create_shader_module.id, command->u.create_shader_module.stage,
            command->u.create_shader_module.format);
    case DVZ_DRP2_COMMAND_DESTROY_SHADER_MODULE:
        return _trace_snapshot_append(
            snapshot, "- shader id=%" PRIu64, command->u.destroy_shader_module.shader_module_id);
    case DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE:
        return _trace_snapshot_append(
            snapshot, "+ render-pipeline id=%" PRIu64 " shaders=(%" PRIu64 ",%" PRIu64
                      ") vslots=%" PRIu32 " attrs=%" PRIu32 " depth=%s",
            command->u.create_render_pipeline.id,
            command->u.create_render_pipeline.vertex_shader_module_id,
            command->u.create_render_pipeline.fragment_shader_module_id,
            command->u.create_render_pipeline.vertex_buffer_slots,
            command->u.create_render_pipeline.attr_count,
            command->u.create_render_pipeline.has_depth_attachment ? "yes" : "no");
    case DVZ_DRP2_COMMAND_DESTROY_RENDER_PIPELINE:
        return _trace_snapshot_append(
            snapshot, "- render-pipeline id=%" PRIu64,
            command->u.destroy_render_pipeline.render_pipeline_id);
    case DVZ_DRP2_COMMAND_CREATE_COMPUTE_PIPELINE:
        return _trace_snapshot_append(
            snapshot, "+ compute-pipeline id=%" PRIu64 " shader=%" PRIu64,
            command->u.create_compute_pipeline.id,
            command->u.create_compute_pipeline.compute_shader_module_id);
    case DVZ_DRP2_COMMAND_DESTROY_COMPUTE_PIPELINE:
        return _trace_snapshot_append(
            snapshot, "- compute-pipeline id=%" PRIu64,
            command->u.destroy_compute_pipeline.compute_pipeline_id);
    case DVZ_DRP2_COMMAND_CREATE_SAMPLER:
        return _trace_snapshot_append(
            snapshot, "+ sampler id=%" PRIu64, command->u.create_sampler.id);
    case DVZ_DRP2_COMMAND_CREATE_BIND_GROUP_LAYOUT:
        return _trace_snapshot_append(
            snapshot, "+ bind-layout id=%" PRIu64 " storage=%s uniform=%s",
            command->u.create_bind_group_layout.id,
            command->u.create_bind_group_layout.storage_buffers ? "yes" : "no",
            command->u.create_bind_group_layout.uniform_buffer ? "yes" : "no");
    case DVZ_DRP2_COMMAND_CREATE_BIND_GROUP:
        return _trace_snapshot_append(
            snapshot, "+ bind-group id=%" PRIu64 " layout=%" PRIu64 " tex=%" PRIu64
                      " buf0=%" PRIu64 " buf1=%" PRIu64,
            command->u.create_bind_group.id, command->u.create_bind_group.bind_group_layout_id,
            command->u.create_bind_group.texture_id, command->u.create_bind_group.buffer0_id,
            command->u.create_bind_group.buffer1_id);
    case DVZ_DRP2_COMMAND_DESTROY_BIND_GROUP_LAYOUT:
        return _trace_snapshot_append(
            snapshot, "- bind-layout id=%" PRIu64,
            command->u.destroy_bind_group_layout.bind_group_layout_id);
    case DVZ_DRP2_COMMAND_DESTROY_BIND_GROUP:
        return _trace_snapshot_append(
            snapshot, "- bind-group id=%" PRIu64, command->u.destroy_bind_group.bind_group_id);
    case DVZ_DRP2_COMMAND_WRITE_BUFFER:
        return _trace_snapshot_append(
            snapshot, "~ buffer id=%" PRIu64 " off=%" PRIu64 " size=%" PRIu64,
            command->u.write_buffer.buffer_id, command->u.write_buffer.offset,
            command->u.write_buffer.size);
    case DVZ_DRP2_COMMAND_WRITE_TEXTURE:
        return _trace_snapshot_append(
            snapshot, "~ texture id=%" PRIu64 " origin=(%" PRIu32 ",%" PRIu32 ",%" PRIu32
                      ") size=(%" PRIu32 ",%" PRIu32 ",%" PRIu32 ")",
            command->u.write_texture.texture_id, command->u.write_texture.origin_x,
            command->u.write_texture.origin_y, command->u.write_texture.origin_z,
            command->u.write_texture.width, command->u.write_texture.height,
            command->u.write_texture.depth);
    case DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS:
        return _trace_snapshot_append(
            snapshot, "render#%" PRIu32 " target=%" PRIu64 " clear=%s depth=%s area=(%.3g,%.3g %.3gx%.3g)",
            _trace_pass_ordinal(passes, pass_count, command->u.begin_render_pass.id),
            command->u.begin_render_pass.texture_id,
            command->u.begin_render_pass.clear ? "yes" : "load",
            command->u.begin_render_pass.has_depth_attachment ? "yes" : "no",
            (double)command->u.begin_render_pass.viewport[0],
            (double)command->u.begin_render_pass.viewport[1],
            (double)command->u.begin_render_pass.viewport[2],
            (double)command->u.begin_render_pass.viewport[3]);
    case DVZ_DRP2_COMMAND_BEGIN_COMPUTE_PASS:
        return _trace_snapshot_append(
            snapshot, "compute#%" PRIu32 " begin",
            _trace_pass_ordinal(passes, pass_count, command->u.begin_compute_pass.id));
    case DVZ_DRP2_COMMAND_SET_VIEWPORT:
        if (_trace_rect_full(command->u.set_viewport.viewport))
            return true;
        ordinal = _trace_pass_ordinal(passes, pass_count, command->u.set_viewport.pass_id);
        if (!_trace_format_suffix(
            suffix, sizeof(suffix), "viewport=(%.3g,%.3g %.3gx%.3g)",
            (double)command->u.set_viewport.viewport[0],
            (double)command->u.set_viewport.viewport[1],
            (double)command->u.set_viewport.viewport[2],
            (double)command->u.set_viewport.viewport[3]))
            return false;
        return _trace_snapshot_append_pass_line(snapshot, "render", ordinal, suffix);
    case DVZ_DRP2_COMMAND_SET_SCISSOR:
        if (_trace_rect_full(command->u.set_scissor.scissor))
            return true;
        ordinal = _trace_pass_ordinal(passes, pass_count, command->u.set_scissor.pass_id);
        if (!_trace_format_suffix(
            suffix, sizeof(suffix), "scissor=(%.3g,%.3g %.3gx%.3g)",
            (double)command->u.set_scissor.scissor[0],
            (double)command->u.set_scissor.scissor[1],
            (double)command->u.set_scissor.scissor[2],
            (double)command->u.set_scissor.scissor[3]))
            return false;
        return _trace_snapshot_append_pass_line(snapshot, "render", ordinal, suffix);
    case DVZ_DRP2_COMMAND_SET_PIPELINE:
        ordinal = _trace_pass_ordinal(passes, pass_count, command->u.set_pipeline.pass_id);
        if (!_trace_format_suffix(
            suffix, sizeof(suffix), "pipeline=%" PRIu64,
            command->u.set_pipeline.pipeline_id))
            return false;
        return _trace_snapshot_append_pass_line(snapshot, "pass", ordinal, suffix);
    case DVZ_DRP2_COMMAND_SET_BIND_GROUP:
        ordinal = _trace_pass_ordinal(passes, pass_count, command->u.set_bind_group.pass_id);
        if (!_trace_format_suffix(
            suffix, sizeof(suffix), "bind[%" PRIu32 "]=%" PRIu64,
            command->u.set_bind_group.slot, command->u.set_bind_group.bind_group_id))
            return false;
        return _trace_snapshot_append_pass_line(snapshot, "pass", ordinal, suffix);
    case DVZ_DRP2_COMMAND_SET_VERTEX_BUFFER:
        ordinal = _trace_pass_ordinal(passes, pass_count, command->u.set_vertex_buffer.pass_id);
        if (!_trace_format_suffix(
            suffix, sizeof(suffix), "vbuf[%" PRIu32 "]=%" PRIu64 " off=%" PRIu64,
            command->u.set_vertex_buffer.slot, command->u.set_vertex_buffer.buffer_id,
            command->u.set_vertex_buffer.offset))
            return false;
        return _trace_snapshot_append_pass_line(snapshot, "render", ordinal, suffix);
    case DVZ_DRP2_COMMAND_SET_INDEX_BUFFER:
        ordinal = _trace_pass_ordinal(passes, pass_count, command->u.set_index_buffer.pass_id);
        if (!_trace_format_suffix(
            suffix, sizeof(suffix), "ibuf=%" PRIu64 " fmt=%s off=%" PRIu64,
            command->u.set_index_buffer.buffer_id, command->u.set_index_buffer.index_format,
            command->u.set_index_buffer.offset))
            return false;
        return _trace_snapshot_append_pass_line(snapshot, "render", ordinal, suffix);
    case DVZ_DRP2_COMMAND_DRAW:
        ordinal = _trace_pass_ordinal(passes, pass_count, command->u.draw.pass_id);
        if (!_trace_format_suffix(
            suffix, sizeof(suffix), "draw vertices=%" PRIu32 " first=%" PRIu32
                                    " instances=%" PRIu32,
            command->u.draw.vertex_count, command->u.draw.first_vertex,
            command->u.draw.instance_count))
            return false;
        return _trace_snapshot_append_pass_line(snapshot, "render", ordinal, suffix);
    case DVZ_DRP2_COMMAND_DRAW_INDEXED:
        ordinal = _trace_pass_ordinal(passes, pass_count, command->u.draw_indexed.pass_id);
        if (!_trace_format_suffix(
            suffix, sizeof(suffix), "draw-indexed indices=%" PRIu32 " first=%" PRIu32
                                    " base=%" PRId32 " instances=%" PRIu32,
            command->u.draw_indexed.index_count, command->u.draw_indexed.first_index,
            command->u.draw_indexed.base_vertex, command->u.draw_indexed.instance_count))
            return false;
        return _trace_snapshot_append_pass_line(snapshot, "render", ordinal, suffix);
    case DVZ_DRP2_COMMAND_DISPATCH_WORKGROUPS:
        ordinal = _trace_pass_ordinal(passes, pass_count, command->u.dispatch.pass_id);
        if (!_trace_format_suffix(
            suffix, sizeof(suffix), "dispatch=(%" PRIu32 ",%" PRIu32 ",%" PRIu32 ")",
            command->u.dispatch.x, command->u.dispatch.y, command->u.dispatch.z))
            return false;
        return _trace_snapshot_append_pass_line(snapshot, "compute", ordinal, suffix);
    case DVZ_DRP2_COMMAND_COPY_BUFFER_TO_BUFFER:
        return _trace_snapshot_append(
            snapshot, "~ copy buffer %" PRIu64 ":%" PRIu64 " -> %" PRIu64 ":%" PRIu64
                      " size=%" PRIu64,
            command->u.copy_buffer_to_buffer.src_buffer_id,
            command->u.copy_buffer_to_buffer.src_offset,
            command->u.copy_buffer_to_buffer.dst_buffer_id,
            command->u.copy_buffer_to_buffer.dst_offset,
            command->u.copy_buffer_to_buffer.size);
    case DVZ_DRP2_COMMAND_COPY_BUFFER_TO_TEXTURE:
        return _trace_snapshot_append(
            snapshot, "~ copy buffer %" PRIu64 ":%" PRIu64 " -> texture %" PRIu64
                      " size=(%" PRIu32 ",%" PRIu32 ",%" PRIu32 ")",
            command->u.copy_buffer_to_texture.src_buffer_id,
            command->u.copy_buffer_to_texture.src_offset,
            command->u.copy_buffer_to_texture.dst_texture_id,
            command->u.copy_buffer_to_texture.width,
            command->u.copy_buffer_to_texture.height,
            command->u.copy_buffer_to_texture.depth);
    case DVZ_DRP2_COMMAND_COPY_TEXTURE_TO_BUFFER:
        return _trace_snapshot_append(
            snapshot, "~ copy texture %" PRIu64 " -> buffer %" PRIu64 ":%" PRIu64
                      " size=(%" PRIu32 ",%" PRIu32 ")",
            command->u.copy_texture_to_buffer.src_texture_id,
            command->u.copy_texture_to_buffer.dst_buffer_id,
            command->u.copy_texture_to_buffer.dst_offset,
            command->u.copy_texture_to_buffer.width,
            command->u.copy_texture_to_buffer.height);
    case DVZ_DRP2_COMMAND_COPY_TEXTURE_TO_TEXTURE:
        return _trace_snapshot_append(
            snapshot, "~ copy texture %" PRIu64 " -> %" PRIu64 " size=(%" PRIu32
                      ",%" PRIu32 ",%" PRIu32 ")",
            command->u.copy_texture_to_texture.src_texture_id,
            command->u.copy_texture_to_texture.dst_texture_id,
            command->u.copy_texture_to_texture.width,
            command->u.copy_texture_to_texture.height,
            command->u.copy_texture_to_texture.depth);
    case DVZ_DRP2_COMMAND_QUEUE_SUBMIT:
        if (!command->u.queue_submit.has_readback)
            return true;
        return _trace_snapshot_append(
            snapshot, "readback buffer=%" PRIu64 " off=%" PRIu64 " size=%" PRIu64,
            command->u.queue_submit.buffer_id, command->u.queue_submit.offset,
            command->u.queue_submit.size);
    case DVZ_DRP2_COMMAND_HELLO_RENDERER:
    case DVZ_DRP2_COMMAND_RENDERER_HELLO_REPLY:
    case DVZ_DRP2_COMMAND_BEGIN_COMMAND_ENCODER:
    case DVZ_DRP2_COMMAND_END_RENDER_PASS:
    case DVZ_DRP2_COMMAND_END_COMPUTE_PASS:
    case DVZ_DRP2_COMMAND_FINISH_COMMAND_ENCODER:
    case DVZ_DRP2_COMMAND_QUEUE_SUBMIT_REPLY:
    case DVZ_DRP2_COMMAND_NONE:
    default:
        return true;
    }
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Parse the `DVZ_DRP2_TRACE` environment variable into an internal trace mode.
 *
 * @param value environment variable value, or NULL
 * @return the parsed trace mode
 */
DvzAppTraceMode _dvz_app_trace_mode_from_env(const char* value)
{
    if (value == NULL)
        return DVZ_APP_TRACE_NONE;
    if (strcmp(value, "0") == 0 || strcmp(value, "false") == 0 ||
        strcmp(value, "FALSE") == 0 || strcmp(value, "off") == 0 || strcmp(value, "OFF") == 0)
    {
        return DVZ_APP_TRACE_NONE;
    }
    if (strcmp(value, "full") == 0 || strcmp(value, "FULL") == 0)
        return DVZ_APP_TRACE_FULL;
    return DVZ_APP_TRACE_NORMAL;
}



/**
 * Compute the terminal-behavior plan for one trace update.
 *
 * @param mode active trace mode
 * @param status_line_open whether an in-place unchanged line is currently open
 * @param changed whether the newly emitted stream differs from the previous one
 * @return the trace update plan
 */
DvzAppTracePlan
_dvz_app_trace_plan(DvzAppTraceMode mode, bool status_line_open, bool changed)
{
    DvzAppTracePlan plan = {0};
    if (mode == DVZ_APP_TRACE_NONE)
        return plan;

    if (mode == DVZ_APP_TRACE_FULL)
    {
        plan.event_kind = DVZ_APP_TRACE_EVENT_CHANGED;
        plan.prepend_newline = status_line_open;
        return plan;
    }

    if (changed)
    {
        plan.event_kind = DVZ_APP_TRACE_EVENT_CHANGED;
        plan.prepend_newline = status_line_open;
        return plan;
    }

    plan.event_kind = DVZ_APP_TRACE_EVENT_UNCHANGED;
    plan.rewrite_in_place = true;
    plan.status_line_open_after = true;
    return plan;
}



/**
 * Format the stable serializer name used for duplicate detection.
 *
 * @param out destination character buffer
 * @param size destination buffer size in bytes
 * @return true on success, false on error or truncation
 */
bool _dvz_app_trace_fingerprint_name(char* out, uint32_t size)
{
    ANN(out);
    ASSERT(size > 0);
    int written = dvz_snprintf(out, size, "live_frame");
    return written >= 0 && (uint32_t)written < size;
}



/**
 * Compute a stable semantic fingerprint for one emitted DRP2 stream.
 *
 * @param stream the emitted command stream
 * @param out destination fingerprint
 * @return true on success, false on error
 */
bool _dvz_app_trace_fingerprint(const DvzDrp2CommandStream* stream, uint64_t* out)
{
    ANN(stream);
    ANN(out);

    uint64_t hash = DVZ_APP_TRACE_FNV_OFFSET;
    uint32_t command_count = dvz_drp2_stream_count(stream);
    hash = _trace_hash_u32(hash, command_count);
    for (uint32_t i = 0; i < command_count; i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(stream, i);
        if (command == NULL)
            return false;
        hash = _trace_hash_u64(hash, (uint64_t)i);
        hash = _trace_hash_command(hash, command);
    }

    *out = hash;
    return true;
}



/**
 * Initialize an empty normalized trace snapshot.
 *
 * @param snapshot the snapshot to initialize
 */
void _dvz_app_trace_snapshot_init(DvzAppTraceSnapshot* snapshot)
{
    ANN(snapshot);
    dvz_memset(snapshot, sizeof(DvzAppTraceSnapshot), 0, sizeof(DvzAppTraceSnapshot));
}



/**
 * Destroy a normalized trace snapshot.
 *
 * @param snapshot the snapshot to destroy
 */
void _dvz_app_trace_snapshot_destroy(DvzAppTraceSnapshot* snapshot)
{
    if (snapshot == NULL)
        return;
    dvz_free(snapshot->lines);
    _dvz_app_trace_snapshot_init(snapshot);
}



/**
 * Build a compact normalized trace snapshot from one emitted DRP2 stream.
 *
 * @param snapshot destination snapshot
 * @param stream source DRP2 stream
 * @return true on success, false on allocation or stream error
 */
bool _dvz_app_trace_snapshot_build(
    DvzAppTraceSnapshot* snapshot, const DvzDrp2CommandStream* stream)
{
    ANN(snapshot);
    ANN(stream);
    _dvz_app_trace_snapshot_destroy(snapshot);

    uint32_t command_count = dvz_drp2_stream_count(stream);
    uint32_t pass_capacity = 0;
    for (uint32_t i = 0; i < command_count; i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(stream, i);
        if (command == NULL)
            return false;
        if (command->type == DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS ||
            command->type == DVZ_DRP2_COMMAND_BEGIN_COMPUTE_PASS)
        {
            pass_capacity++;
        }
    }

    TracePassMap* passes = NULL;
    if (pass_capacity > 0)
    {
        passes = (TracePassMap*)dvz_calloc(pass_capacity, sizeof(TracePassMap));
        if (passes == NULL)
            return false;
    }

    uint32_t pass_count = 0;
    uint32_t render_count = 0;
    uint32_t compute_count = 0;
    for (uint32_t i = 0; i < command_count; i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(stream, i);
        ANN(command);
        if (command->type == DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS)
        {
            if (passes == NULL || pass_count >= pass_capacity)
            {
                dvz_free(passes);
                return false;
            }
            passes[pass_count++] = (TracePassMap){
                .id = command->u.begin_render_pass.id,
                .ordinal = render_count++,
            };
        }
        else if (command->type == DVZ_DRP2_COMMAND_BEGIN_COMPUTE_PASS)
        {
            if (passes == NULL || pass_count >= pass_capacity)
            {
                dvz_free(passes);
                return false;
            }
            passes[pass_count++] = (TracePassMap){
                .id = command->u.begin_compute_pass.id,
                .ordinal = compute_count++,
            };
        }
    }

    bool ok = true;
    for (uint32_t i = 0; ok && i < command_count; i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(stream, i);
        if (command == NULL)
        {
            ok = false;
            break;
        }
        ok = _trace_snapshot_append_command(snapshot, command, passes, pass_count);
    }

    dvz_free(passes);
    if (!ok)
        _dvz_app_trace_snapshot_destroy(snapshot);
    return ok;
}



/**
 * Return whether two normalized trace snapshots contain identical lines in identical order.
 *
 * @param a first snapshot
 * @param b second snapshot
 * @return true if both snapshots are equal
 */
bool _dvz_app_trace_snapshot_equal(
    const DvzAppTraceSnapshot* a, const DvzAppTraceSnapshot* b)
{
    ANN(a);
    ANN(b);
    if (a->count != b->count)
        return false;
    for (uint32_t i = 0; i < a->count; i++)
    {
        if (strcmp(a->lines[i].text, b->lines[i].text) != 0)
            return false;
    }
    return true;
}



/**
 * Count the occurrences of one normalized line in a snapshot.
 *
 * @param snapshot the snapshot
 * @param text line text
 * @return occurrence count
 */
uint32_t _dvz_app_trace_snapshot_line_count(
    const DvzAppTraceSnapshot* snapshot, const char* text)
{
    ANN(snapshot);
    ANN(text);
    uint32_t count = 0;
    for (uint32_t i = 0; i < snapshot->count; i++)
    {
        if (strcmp(snapshot->lines[i].text, text) == 0)
            count++;
    }
    return count;
}
