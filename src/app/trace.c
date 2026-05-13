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

#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "../drp2/_stream.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_APP_TRACE_FNV_OFFSET 1469598103934665603ULL
#define DVZ_APP_TRACE_FNV_PRIME  1099511628211ULL



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
 * Return a sanitized command copy suitable for stable trace fingerprinting.
 *
 * @param command source command
 * @return sanitized command value
 */
static DvzDrp2Command _trace_stable_command(const DvzDrp2Command* command)
{
    ANN(command);
    DvzDrp2Command stable = *command;

    switch (stable.type)
    {
    case DVZ_DRP2_COMMAND_CREATE_SHADER_MODULE:
        stable.u.create_shader_module.code = NULL;
        stable.u.create_shader_module.spirv = NULL;
        stable.u.create_shader_module.spirv_size = 0;
        break;
    case DVZ_DRP2_COMMAND_WRITE_BUFFER:
        stable.u.write_buffer.data_raw = NULL;
        stable.u.write_buffer.data_base64 = NULL;
        break;
    case DVZ_DRP2_COMMAND_WRITE_TEXTURE:
        stable.u.write_texture.data_raw = NULL;
        stable.u.write_texture.data_base64 = NULL;
        break;
    case DVZ_DRP2_COMMAND_FINISH_COMMAND_ENCODER:
        stable.u.finish_command_encoder.command_buffer_id = 0;
        break;
    case DVZ_DRP2_COMMAND_QUEUE_SUBMIT:
        stable.u.queue_submit.command_buffer_id = 0;
        stable.u.queue_submit.submission_id = 0;
        dvz_memset(
            stable.u.queue_submit.data_base64, sizeof(stable.u.queue_submit.data_base64), 0,
            sizeof(stable.u.queue_submit.data_base64));
        break;
    case DVZ_DRP2_COMMAND_QUEUE_SUBMIT_REPLY:
        stable.u.queue_submit.submission_id = 0;
        dvz_memset(
            stable.u.queue_submit.data_base64, sizeof(stable.u.queue_submit.data_base64), 0,
            sizeof(stable.u.queue_submit.data_base64));
        break;
    default:
        break;
    }
    return stable;
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
 * Format one in-place unchanged status line.
 *
 * @param frame_index 0-based frame index
 * @param command_count emitted command count
 * @param out destination character buffer
 * @param size destination buffer size in bytes
 * @return true on success, false on error or truncation
 */
bool _dvz_app_trace_status_line(
    uint64_t frame_index, uint32_t command_count, char* out, uint32_t size)
{
    ANN(out);
    ASSERT(size > 0);
    int written = dvz_snprintf(
        out, size, "\r\x1b[2Kframe %08llu | unchanged | %u cmds",
        (unsigned long long)frame_index, command_count);
    return written >= 0 && (uint32_t)written < size;
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
    hash = _trace_hash_u32(hash, dvz_drp2_stream_count(stream));
    for (uint32_t i = 0; i < dvz_drp2_stream_count(stream); i++)
    {
        const DvzDrp2Command* command = dvz_drp2_stream_get(stream, i);
        if (command == NULL)
            return false;

        DvzDrp2Command stable = _trace_stable_command(command);
        hash = _trace_hash_u64(hash, (uint64_t)i);
        hash = _trace_hash_bytes(hash, &stable, sizeof(stable));
    }

    *out = hash;
    return true;
}
