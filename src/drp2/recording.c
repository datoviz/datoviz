/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  DRP2 linear recording                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <vulkan/vulkan_core.h>

#if defined(_WIN32)
#include <direct.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/utsname.h>
#endif

#include "_alloc.h"
#include "_assertions.h"
#include "_base64.h"
#include "_compat.h"
#include "_log.h"
#include "_overflow.h"
#include "_stream.h"
#include "_time_utils.h"
#include "datoviz/common/version.h"
#include "datoviz/drp2/recording.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_DRP2_RECORDING_PATH_SIZE 4096
#define DVZ_DRP2_RECORDING_LINE_SIZE 4096
#define DVZ_DRP2_RECORDING_INFO_KNOWN_FLAGS 0u
#define DVZ_DRP2_RECORDING_INLINE_PAYLOAD_MAX_SIZE 1024



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct DvzDrp2RecordingOwner DvzDrp2RecordingOwner;

struct DvzDrp2RecordingOwner
{
    uint32_t count;
    void** blobs;
};


struct DvzDrp2Recording
{
    DvzDrp2CommandStream* stream;
    DvzDrp2RecordedFrame* frames;
    uint32_t frame_count;
    uint32_t frame_capacity;
    DvzDrp2RawFallback* raw_fallbacks;
    uint32_t raw_fallback_count;
    uint32_t raw_fallback_capacity;
};


struct DvzDrp2Recorder
{
    char path[DVZ_DRP2_RECORDING_PATH_SIZE];
    FILE* stream_fp;
    DvzDrp2RecordingInfo info;
    uint32_t blob_index;
    uint64_t command_count;
    bool closed;
};



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static bool _recording_info_validate(const DvzDrp2RecordingInfo* info)
{
    if (info == NULL)
        return true;
    if (!DVZ_STRUCT_VALID(info, DvzDrp2RecordingInfo, DVZ_DRP2_RECORDING_INFO_KNOWN_FLAGS))
    {
        log_error("invalid DvzDrp2RecordingInfo ABI prologue");
        return false;
    }
    return true;
}



/**
 * Create one directory if it does not already exist.
 *
 * @param path directory path
 * @return whether the directory exists or was created
 */
static bool _recording_mkdir(const char* path)
{
    ANN(path);
#if defined(_WIN32)
    int rc = _mkdir(path);
#else
    int rc = mkdir(path, 0777);
#endif
    if (rc == 0 || errno == EEXIST)
        return true;
    log_error("failed to create DRP2 recording directory '%s'", path);
    return false;
}



/**
 * Join a recording path with one relative child path.
 *
 * @param base base directory
 * @param child child path
 * @param out output buffer
 * @param out_size output buffer size
 * @return whether the joined path fit in the output buffer
 */
static bool _recording_join(
    const char* base, const char* child, char* out, uint64_t out_size)
{
    ANN(base);
    ANN(child);
    ANN(out);
    int rc = dvz_snprintf(out, (size_t)out_size, "%s/%s", base, child);
    return rc >= 0 && (uint64_t)rc < out_size;
}



/**
 * Create a DRP2 validation result.
 *
 * @param ok whether validation succeeded
 * @param code validation code
 * @param command_index command index associated with the result
 * @return the validation result
 */
static DvzDrp2ValidationResult _recording_result(
    bool ok, DvzDrp2ValidationCode code, uint32_t command_index)
{
    DvzDrp2ValidationResult result = {0};
    result.ok = ok;
    result.code = code;
    result.command_index = command_index;
    return result;
}



/**
 * Shift a frame-local validation command index to recording command coordinates.
 *
 * @param result validation result
 * @param first_command first command index in the recorded frame
 */
static void _recording_result_offset(
    DvzDrp2ValidationResult* result, uint32_t first_command)
{
    ANN(result);
    if (result->ok || result->command_index == UINT32_MAX)
        return;
    if (result->command_index > UINT32_MAX - first_command)
        result->command_index = UINT32_MAX;
    else
        result->command_index += first_command;
}



/**
 * Write one binary blob file.
 *
 * @param path blob path
 * @param data blob bytes
 * @param size blob byte size
 * @return whether the file was written
 */
static bool _recording_write_blob(const char* path, const void* data, uint64_t size)
{
    ANN(path);
    if (size > 0 && data == NULL)
        return false;
    if (size > SIZE_MAX)
        return false;
    FILE* fp = fopen(path, "wb");
    if (fp == NULL)
    {
        log_error("failed to open DRP2 recording blob '%s' for writing", path);
        return false;
    }
    bool ok = true;
    if (size > 0 && fwrite(data, 1, (size_t)size, fp) != (size_t)size)
        ok = false;
    fclose(fp);
    return ok;
}



/**
 * Read one binary blob file.
 *
 * @param path blob path
 * @param expected_size expected byte size
 * @return owned blob bytes, or NULL on error
 */
static void* _recording_read_blob(const char* path, uint64_t expected_size)
{
    ANN(path);
    if (expected_size == 0)
        return NULL;
    if (expected_size > SIZE_MAX)
        return NULL;
    FILE* fp = fopen(path, "rb");
    if (fp == NULL)
    {
        log_error("failed to open DRP2 recording blob '%s' for reading", path);
        return NULL;
    }
    void* data = dvz_calloc((size_t)expected_size, 1);
    if (data == NULL)
    {
        fclose(fp);
        return NULL;
    }
    bool ok = fread(data, 1, (size_t)expected_size, fp) == (size_t)expected_size;
    fclose(fp);
    if (!ok)
    {
        dvz_free(data);
        return NULL;
    }
    return data;
}



/**
 * Append an owned blob pointer to a recording-owned stream.
 *
 * @param owner recording owner
 * @param blob owned blob pointer
 * @return whether the blob was retained
 */
static bool _recording_owner_add(DvzDrp2RecordingOwner* owner, void* blob)
{
    ANN(owner);
    ANN(blob);
    if (owner->count == UINT32_MAX)
        return false;
    uint64_t bytes = 0;
    if (_dvz_mul_u64_overflows(owner->count + 1, sizeof(void*), &bytes))
        return false;
    void** blobs = (void**)dvz_realloc(owner->blobs, bytes);
    if (blobs == NULL)
        return false;
    owner->blobs = blobs;
    owner->blobs[owner->count++] = blob;
    return true;
}



/**
 * Copy a payload into recording-owned stream storage.
 *
 * @param owner recording owner
 * @param data source bytes
 * @param size byte size
 * @return owned byte copy, or NULL on error
 */
static void* _recording_owner_copy(DvzDrp2RecordingOwner* owner, const void* data, uint64_t size)
{
    ANN(owner);
    if (size == 0)
        return NULL;
    if (data == NULL || size > SIZE_MAX)
        return NULL;
    void* copy = dvz_calloc((size_t)size, 1);
    if (copy == NULL)
        return NULL;
    dvz_memcpy(copy, (size_t)size, data, (size_t)size);
    if (_recording_owner_add(owner, copy))
        return copy;
    dvz_free(copy);
    return NULL;
}



/**
 * Copy a NUL-terminated string for stream-owned command fields.
 *
 * @param str source string
 * @return owned string copy, or NULL on error
 */
static char* _recording_strdup(const char* str)
{
    ANN(str);
    size_t len = strlen(str) + 1;
    char* copy = (char*)dvz_calloc(len, 1);
    if (copy == NULL)
        return NULL;
    dvz_memcpy(copy, len, str, len);
    return copy;
}



/**
 * Release all recording-owned stream blobs.
 *
 * @param ptr recording owner pointer
 */
static void _recording_owner_release(void* ptr)
{
    DvzDrp2RecordingOwner* owner = (DvzDrp2RecordingOwner*)ptr;
    if (owner == NULL)
        return;
    for (uint32_t i = 0; i < owner->count; i++)
        dvz_free(owner->blobs[i]);
    dvz_free(owner->blobs);
    dvz_free(owner);
}



/**
 * Ensure a reconstructed stream has space for one more command.
 *
 * @param stream the command stream
 * @return whether capacity is available
 */
static bool _recording_stream_ensure_capacity(DvzDrp2CommandStream* stream)
{
    ANN(stream);
    if (stream->count < stream->capacity)
        return true;
    if (stream->capacity > UINT32_MAX / 2)
        return false;
    uint32_t capacity = stream->capacity * 2;
    uint64_t bytes = 0;
    if (_dvz_mul_u64_overflows(capacity, sizeof(DvzDrp2Command), &bytes))
        return false;
    DvzDrp2Command* commands = (DvzDrp2Command*)dvz_realloc(stream->commands, bytes);
    if (commands == NULL)
        return false;
    stream->capacity = capacity;
    stream->commands = commands;
    return true;
}



/**
 * Append one already-decoded raw command to a stream.
 *
 * @param stream the command stream
 * @param command command to append
 * @return whether the command was appended
 */
static bool _recording_stream_append(DvzDrp2CommandStream* stream, const DvzDrp2Command* command)
{
    ANN(stream);
    ANN(command);
    if (!_recording_stream_ensure_capacity(stream))
        return false;
    DvzDrp2Command* dst = &stream->commands[stream->count++];
    dvz_memcpy(dst, sizeof(DvzDrp2Command), command, sizeof(DvzDrp2Command));
    return true;
}



/**
 * Ensure a loaded recording has space for one more frame record.
 *
 * @param recording loaded recording
 * @return whether capacity is available
 */
static bool _recording_frames_ensure_capacity(DvzDrp2Recording* recording)
{
    ANN(recording);
    if (recording->frame_count < recording->frame_capacity)
        return true;
    uint32_t capacity = recording->frame_capacity == 0 ? 16 : recording->frame_capacity * 2;
    if (capacity <= recording->frame_capacity)
        return false;
    uint64_t bytes = 0;
    if (_dvz_mul_u64_overflows(capacity, sizeof(DvzDrp2RecordedFrame), &bytes))
        return false;
    DvzDrp2RecordedFrame* frames = (DvzDrp2RecordedFrame*)dvz_realloc(
        recording->frames, bytes);
    if (frames == NULL)
        return false;
    recording->frames = frames;
    recording->frame_capacity = capacity;
    return true;
}



/**
 * Append one frame record to a loaded recording.
 *
 * @param recording loaded recording
 * @param frame frame record
 * @return whether the frame was appended
 */
static bool _recording_frame_append(
    DvzDrp2Recording* recording, const DvzDrp2RecordedFrame* frame)
{
    ANN(recording);
    ANN(frame);
    if (!_recording_frames_ensure_capacity(recording))
        return false;
    DvzDrp2RecordedFrame* dst = &recording->frames[recording->frame_count++];
    dvz_memcpy(dst, sizeof(DvzDrp2RecordedFrame), frame, sizeof(DvzDrp2RecordedFrame));
    return true;
}



/**
 * Append one raw fallback record to a loaded recording.
 *
 * @param recording loaded recording
 * @param fallback raw fallback record
 * @return whether the record was appended
 */
static bool
_recording_raw_fallback_append(DvzDrp2Recording* recording, const DvzDrp2RawFallback* fallback)
{
    ANN(recording);
    ANN(fallback);
    if (recording->raw_fallback_count == recording->raw_fallback_capacity)
    {
        uint32_t capacity =
            recording->raw_fallback_capacity == 0 ? 4 : recording->raw_fallback_capacity * 2;
        if (capacity <= recording->raw_fallback_capacity)
            return false;
        uint64_t bytes = 0;
        if (_dvz_mul_u64_overflows(capacity, sizeof(DvzDrp2RawFallback), &bytes))
            return false;
        DvzDrp2RawFallback* raw_fallbacks =
            (DvzDrp2RawFallback*)dvz_realloc(recording->raw_fallbacks, bytes);
        if (raw_fallbacks == NULL)
            return false;
        recording->raw_fallbacks = raw_fallbacks;
        recording->raw_fallback_capacity = capacity;
    }
    recording->raw_fallbacks[recording->raw_fallback_count++] = *fallback;
    return true;
}



/**
 * Return the byte size implied by a texture write payload.
 *
 * @param command write-texture command
 * @param out_size payload byte size
 * @return whether the size calculation succeeded
 */
static bool _recording_texture_payload_size(const DvzDrp2Command* command, uint64_t* out_size)
{
    ANN(command);
    ANN(out_size);
    uint64_t rows = 0;
    if (_dvz_mul_u64_overflows(
            command->u.write_texture.depth, command->u.write_texture.rows_per_image, &rows))
        return false;
    return !_dvz_mul_u64_overflows(rows, command->u.write_texture.bytes_per_row, out_size);
}



/**
 * Release directly owned command fields after a failed frame-stream append.
 *
 * @param command command whose direct fields should be released
 */
static void _recording_command_release_direct(DvzDrp2Command* command)
{
    ANN(command);
    if (command->type == DVZ_DRP2_COMMAND_WRITE_BUFFER)
        dvz_free(command->u.write_buffer.data_base64);
    else if (command->type == DVZ_DRP2_COMMAND_WRITE_TEXTURE)
        dvz_free(command->u.write_texture.data_base64);
    else if (command->type == DVZ_DRP2_COMMAND_CREATE_SHADER_MODULE)
        dvz_free(command->u.create_shader_module.code);
}



/**
 * Copy command payload pointers so one frame stream can outlive its source recording.
 *
 * @param owner destination stream owner
 * @param command copied command to patch
 * @param source source command
 * @return whether payload fields were copied
 */
static bool _recording_command_copy_payloads(
    DvzDrp2RecordingOwner* owner, DvzDrp2Command* command, const DvzDrp2Command* source)
{
    ANN(owner);
    ANN(command);
    ANN(source);
    if (source->type == DVZ_DRP2_COMMAND_WRITE_BUFFER)
    {
        command->u.write_buffer.data_raw = NULL;
        command->u.write_buffer.data_raw_owned = false;
        command->u.write_buffer.data_base64 = NULL;
        if (source->u.write_buffer.data_base64 != NULL)
        {
            command->u.write_buffer.data_base64 = _recording_strdup(
                source->u.write_buffer.data_base64);
            return command->u.write_buffer.data_base64 != NULL;
        }
        if (source->u.write_buffer.data_raw == NULL || source->u.write_buffer.size == 0)
            return true;
        command->u.write_buffer.data_raw = _recording_owner_copy(
            owner, source->u.write_buffer.data_raw, source->u.write_buffer.size);
        return command->u.write_buffer.data_raw != NULL;
    }
    if (source->type == DVZ_DRP2_COMMAND_WRITE_TEXTURE)
    {
        command->u.write_texture.data_raw = NULL;
        command->u.write_texture.data_base64 = NULL;
        if (source->u.write_texture.data_base64 != NULL)
        {
            command->u.write_texture.data_base64 = _recording_strdup(
                source->u.write_texture.data_base64);
            return command->u.write_texture.data_base64 != NULL;
        }
        uint64_t payload_size = 0;
        if (!_recording_texture_payload_size(source, &payload_size))
            return false;
        if (source->u.write_texture.data_raw == NULL || payload_size == 0)
            return true;
        command->u.write_texture.data_raw = _recording_owner_copy(
            owner, source->u.write_texture.data_raw, payload_size);
        return command->u.write_texture.data_raw != NULL;
    }
    if (source->type == DVZ_DRP2_COMMAND_CREATE_SHADER_MODULE)
    {
        command->u.create_shader_module.code = NULL;
        command->u.create_shader_module.spirv = NULL;
        if (source->u.create_shader_module.code != NULL)
        {
            command->u.create_shader_module.code = _recording_strdup(
                source->u.create_shader_module.code);
            return command->u.create_shader_module.code != NULL;
        }
        if (source->u.create_shader_module.spirv == NULL ||
            source->u.create_shader_module.spirv_size == 0)
            return true;
        command->u.create_shader_module.spirv = (const unsigned char*)_recording_owner_copy(
            owner, source->u.create_shader_module.spirv,
            source->u.create_shader_module.spirv_size);
        return command->u.create_shader_module.spirv != NULL;
    }
    return true;
}



/**
 * Write one payload blob for a command with borrowed bytes or base64 bytes.
 *
 * @param path blob path
 * @param raw raw byte pointer, if available
 * @param base64 base64 string, if available
 * @param size expected payload byte size
 * @return whether the payload was written
 */
static bool _recording_write_payload_blob(
    const char* path, const void* raw, const char* base64, uint64_t size)
{
    ANN(path);
    if (size == 0)
        return _recording_write_blob(path, "", 0);
    if (raw != NULL)
        return _recording_write_blob(path, raw, size);
    if (base64 == NULL)
        return false;
    uint8_t* decoded = NULL;
    if (!_dvz_b64_decode_exact(base64, size, &decoded))
        return false;
    bool ok = _recording_write_blob(path, decoded, size);
    dvz_free(decoded);
    return ok;
}


/**
 * Write one payload blob and return its relative path.
 *
 * @param root recording root directory
 * @param blob_index running blob index
 * @param raw raw byte pointer, if available
 * @param base64 base64 string, if available
 * @param size expected payload byte size
 * @param out_rel output relative blob path
 * @param out_rel_size output path capacity
 * @return whether the payload blob was written
 */
static bool _recording_write_payload_ref(
    const char* root, uint32_t* blob_index, const void* raw, const char* base64, uint64_t size,
    char* out_rel, uint64_t out_rel_size)
{
    ANN(root);
    ANN(blob_index);
    ANN(out_rel);
    if (*blob_index == UINT32_MAX)
        return false;
    char payload_path[DVZ_DRP2_RECORDING_PATH_SIZE] = {0};
    int rc =
        dvz_snprintf(out_rel, (size_t)out_rel_size, "blobs/%08" PRIu32 ".bin", (*blob_index)++);
    if (rc < 0 || (uint64_t)rc >= out_rel_size)
        return false;
    if (!_recording_join(root, out_rel, payload_path, sizeof(payload_path)))
        return false;
    return _recording_write_payload_blob(payload_path, raw, base64, size);
}



/**
 * Return an owned base64 string for a small inline payload.
 *
 * @param raw raw payload bytes, if available
 * @param base64 existing base64 payload, if available
 * @param size expected payload byte size
 * @return owned base64 payload, or NULL on error
 */
static char* _recording_inline_payload_base64(const void* raw, const char* base64, uint64_t size)
{
    if (size == 0)
        return _recording_strdup("");
    if (base64 != NULL)
        return _recording_strdup(base64);
    if (raw == NULL)
        return NULL;

    uint64_t encoded_size = _dvz_b64_encoded_len(size) + 1;
    char* encoded = (char*)dvz_calloc((size_t)encoded_size, 1);
    if (encoded == NULL)
        return NULL;
    if (!_dvz_b64_encode((const uint8_t*)raw, size, encoded, encoded_size))
    {
        dvz_free(encoded);
        return NULL;
    }
    return encoded;
}



/**
 * Return whether a string can be emitted as a simple unescaped JSON string.
 *
 * @param str string to inspect
 * @return whether the string is safe to write without escaping
 */
static bool _recording_json_string_safe(const char* str)
{
    ANN(str);
    const unsigned char* p = (const unsigned char*)str;
    while (*p != '\0')
    {
        if (*p < 0x20 || *p == '"' || *p == '\\')
            return false;
        p++;
    }
    return true;
}



/**
 * Copy a best-effort UTC timestamp string.
 *
 * @param out output buffer
 * @param out_size output buffer size
 */
static void _recording_created_at(char* out, uint64_t out_size)
{
    ANN(out);
    if (out_size == 0)
        return;
    out[0] = '\0';
    time_t now = time(NULL);
    struct tm tm_utc = {0};
#if defined(_WIN32)
    if (gmtime_s(&tm_utc, &now) != 0)
        return;
#else
    if (gmtime_r(&now, &tm_utc) == NULL)
        return;
#endif
    strftime(out, (size_t)out_size, "%Y-%m-%dT%H:%M:%SZ", &tm_utc);
}



/**
 * Copy the first line of a command's stdout.
 *
 * @param command shell command
 * @param out output buffer
 * @param out_size output buffer size
 * @return whether a non-empty line was copied
 */
static bool _recording_command_first_line(const char* command, char* out, uint64_t out_size)
{
    ANN(command);
    ANN(out);
    if (out_size == 0)
        return false;
    out[0] = '\0';
#if defined(_WIN32)
    FILE* fp = _popen(command, "r");
#else
    FILE* fp = popen(command, "r");
#endif
    if (fp == NULL)
        return false;
    bool ok = fgets(out, (int)out_size, fp) != NULL;
#if defined(_WIN32)
    (void)_pclose(fp);
#else
    (void)pclose(fp);
#endif
    if (!ok)
        return false;
    size_t len = strlen(out);
    while (len > 0 && (out[len - 1] == '\n' || out[len - 1] == '\r'))
        out[--len] = '\0';
    return len > 0 && _recording_json_string_safe(out);
}



/**
 * Write CreateShaderModule as a portable JSON command record.
 *
 * @param root recording root directory
 * @param stream_fp stream JSONL file
 * @param command command to write
 * @param index command index
 * @param blob_index running blob index
 * @return whether the command record was written
 */
static bool _recording_write_create_shader_module(
    const char* root, FILE* stream_fp, const DvzDrp2Command* command, uint32_t index,
    uint32_t* blob_index)
{
    ANN(root);
    ANN(stream_fp);
    ANN(command);
    ANN(blob_index);
    if (!_recording_json_string_safe(command->u.create_shader_module.stage) ||
        !_recording_json_string_safe(command->u.create_shader_module.format) ||
        !_recording_json_string_safe(command->u.create_shader_module.builtin_family) ||
        !_recording_json_string_safe(command->u.create_shader_module.builtin_variant))
        return false;

    const void* payload = NULL;
    uint64_t payload_size = 0;
    const char* payload_kind = "bytes";
    if (command->u.create_shader_module.spirv != NULL &&
        command->u.create_shader_module.spirv_size > 0)
    {
        payload = command->u.create_shader_module.spirv;
        payload_size = command->u.create_shader_module.spirv_size;
    }
    else if (command->u.create_shader_module.code != NULL)
    {
        payload = command->u.create_shader_module.code;
        payload_size = strlen(command->u.create_shader_module.code) + 1;
        payload_kind = "shader_code";
    }

    char payload_rel[128] = {0};
    if (payload_size > 0 &&
        !_recording_write_payload_ref(
            root, blob_index, payload, NULL, payload_size, payload_rel, sizeof(payload_rel)))
        return false;

    if (dvz_fprintf(
            stream_fp,
            "{\"type\":\"command\",\"index\":%" PRIu32 ",\"cmd_type\":%d,"
            "\"op\":\"CreateShaderModule\",\"id\":%" PRIu64 ",\"stage\":\"%s\","
            "\"format\":\"%s\",\"payload_blob\":\"%s\",\"payload_size\":%" PRIu64
            ",\"payload_kind\":\"%s\"",
            index, (int)command->type, command->u.create_shader_module.id,
            command->u.create_shader_module.stage, command->u.create_shader_module.format,
            payload_rel, payload_size, payload_kind) <= 0)
        return false;

    if (command->u.create_shader_module.builtin_family[0] != '\0')
    {
        if (dvz_fprintf(
                stream_fp,
                ",\"builtin_family\":\"%s\",\"builtin_variant\":\"%s\","
                "\"builtin_version\":%" PRIu32,
                command->u.create_shader_module.builtin_family,
                command->u.create_shader_module.builtin_variant,
                command->u.create_shader_module.builtin_version) <= 0)
            return false;
    }
    return dvz_fprintf(stream_fp, "}\n") > 0;
}



/**
 * Write a CreateRenderPipeline command as indexed scalar JSON fields.
 *
 * @param stream_fp stream JSONL file
 * @param command command to write
 * @param index command index
 * @return whether the command record was written
 */
static bool _recording_write_create_render_pipeline(
    FILE* stream_fp, const DvzDrp2Command* command, uint32_t index)
{
    ANN(stream_fp);
    ANN(command);
    if (!_recording_json_string_safe(command->u.create_render_pipeline.builtin_pipeline))
        return false;

    if (dvz_fprintf(
            stream_fp,
            "{\"type\":\"command\",\"index\":%" PRIu32 ",\"cmd_type\":%d,"
            "\"op\":\"CreateRenderPipeline\",\"id\":%" PRIu64
            ",\"vertex_shader_module_id\":%" PRIu64
            ",\"fragment_shader_module_id\":%" PRIu64 ",\"vertex_buffer_slots\":%" PRIu32
            ",\"bind_group_layout_count\":%" PRIu32 ",\"has_depth_attachment\":%u,"
            "\"depth_write_enabled\":%u,\"depth_compare_op\":%" PRIu32
            ",\"has_raster_state\":%u,\"cull_mode\":%" PRIu32
            ",\"front_face\":%" PRIu32 ",\"color_target_count\":%" PRIu32
            ",\"sample_count\":%" PRIu32 ",\"alpha_to_coverage_enabled\":%u"
            ",\"topology\":%" PRIu32
            ",\"binding_count\":%" PRIu32 ",\"attr_count\":%" PRIu32,
            index, (int)command->type, command->u.create_render_pipeline.id,
            command->u.create_render_pipeline.vertex_shader_module_id,
            command->u.create_render_pipeline.fragment_shader_module_id,
            command->u.create_render_pipeline.vertex_buffer_slots,
            command->u.create_render_pipeline.bind_group_layout_count,
            command->u.create_render_pipeline.has_depth_attachment ? 1u : 0u,
            command->u.create_render_pipeline.depth_write_enabled ? 1u : 0u,
            command->u.create_render_pipeline.depth_compare_op,
            command->u.create_render_pipeline.has_raster_state ? 1u : 0u,
            command->u.create_render_pipeline.cull_mode,
            command->u.create_render_pipeline.front_face,
            command->u.create_render_pipeline.color_target_count,
            command->u.create_render_pipeline.sample_count != 0 ?
                command->u.create_render_pipeline.sample_count :
                1,
            command->u.create_render_pipeline.alpha_to_coverage_enabled ? 1u : 0u,
            command->u.create_render_pipeline.topology,
            command->u.create_render_pipeline.binding_count,
            command->u.create_render_pipeline.attr_count) <= 0)
        return false;

    if (command->u.create_render_pipeline.builtin_pipeline[0] != '\0')
    {
        if (dvz_fprintf(
                stream_fp, ",\"builtin_pipeline\":\"%s\",\"builtin_version\":%" PRIu32,
                command->u.create_render_pipeline.builtin_pipeline,
                command->u.create_render_pipeline.builtin_version) <= 0)
            return false;
    }

    for (uint32_t i = 0; i < DVZ_DRP2_MAX_BIND_GROUPS; i++)
    {
        if (dvz_fprintf(
                stream_fp, ",\"bgl%" PRIu32 "_id\":%" PRIu64, i,
                command->u.create_render_pipeline.bind_group_layout_ids[i]) <= 0)
            return false;
    }
    for (uint32_t i = 0; i < DVZ_DRP2_MAX_COLOR_ATTACHMENTS; i++)
    {
        const DvzDrp2ColorTarget* t = &command->u.create_render_pipeline.color_targets[i];
        if (dvz_fprintf(
                stream_fp,
                ",\"ct%" PRIu32 "_format\":%" PRIu32 ",\"ct%" PRIu32 "_blend_enabled\":%u,"
                "\"ct%" PRIu32 "_src_color_blend_factor\":%" PRIu32
                ",\"ct%" PRIu32 "_dst_color_blend_factor\":%" PRIu32
                ",\"ct%" PRIu32 "_color_blend_op\":%" PRIu32
                ",\"ct%" PRIu32 "_src_alpha_blend_factor\":%" PRIu32
                ",\"ct%" PRIu32 "_dst_alpha_blend_factor\":%" PRIu32
                ",\"ct%" PRIu32 "_alpha_blend_op\":%" PRIu32
                ",\"ct%" PRIu32 "_color_write_mask\":%" PRIu32,
                i, t->format, i, t->blend_enabled ? 1u : 0u, i, t->src_color_blend_factor, i,
                t->dst_color_blend_factor, i, t->color_blend_op, i,
                t->src_alpha_blend_factor, i, t->dst_alpha_blend_factor, i,
                t->alpha_blend_op, i, t->color_write_mask) <= 0)
            return false;
    }
    for (uint32_t i = 0; i < 16; i++)
    {
        if (dvz_fprintf(
                stream_fp,
                ",\"binding%" PRIu32 "_stride\":%" PRIu32
                ",\"binding%" PRIu32 "_step_mode\":%" PRIu32,
                i, command->u.create_render_pipeline.binding_strides[i], i,
                command->u.create_render_pipeline.binding_step_modes[i]) <= 0)
            return false;
    }
    for (uint32_t i = 0; i < 16; i++)
    {
        if (dvz_fprintf(
                stream_fp,
                ",\"attr%" PRIu32 "_binding\":%" PRIu32
                ",\"attr%" PRIu32 "_location\":%" PRIu32
                ",\"attr%" PRIu32 "_format\":%" PRIu32
                ",\"attr%" PRIu32 "_offset\":%" PRIu32,
                i, command->u.create_render_pipeline.attr_bindings[i], i,
                command->u.create_render_pipeline.attr_locations[i], i,
                command->u.create_render_pipeline.attr_formats[i], i,
                command->u.create_render_pipeline.attr_offsets[i]) <= 0)
            return false;
    }
    return dvz_fprintf(stream_fp, "}\n") > 0;
}



/**
 * Write a CreateComputePipeline command as indexed scalar JSON fields.
 *
 * @param stream_fp stream JSONL file
 * @param command command to write
 * @param index command index
 * @return whether the command record was written
 */
static bool _recording_write_create_compute_pipeline(
    FILE* stream_fp, const DvzDrp2Command* command, uint32_t index)
{
    ANN(stream_fp);
    ANN(command);
    if (dvz_fprintf(
            stream_fp,
            "{\"type\":\"command\",\"index\":%" PRIu32 ",\"cmd_type\":%d,"
            "\"op\":\"CreateComputePipeline\",\"id\":%" PRIu64
            ",\"compute_shader_module_id\":%" PRIu64
            ",\"bind_group_layout_count\":%" PRIu32,
            index, (int)command->type, command->u.create_compute_pipeline.id,
            command->u.create_compute_pipeline.compute_shader_module_id,
            command->u.create_compute_pipeline.bind_group_layout_count) <= 0)
        return false;
    for (uint32_t i = 0; i < DVZ_DRP2_MAX_BIND_GROUPS; i++)
    {
        if (dvz_fprintf(
                stream_fp, ",\"bgl%" PRIu32 "_id\":%" PRIu64, i,
                command->u.create_compute_pipeline.bind_group_layout_ids[i]) <= 0)
            return false;
    }
    return dvz_fprintf(stream_fp, "}\n") > 0;
}



/**
 * Write CreateBindGroupLayout as indexed scalar JSON fields.
 *
 * @param stream_fp stream JSONL file
 * @param command command to write
 * @param index command index
 * @return whether the command record was written
 */
static bool _recording_write_create_bind_group_layout(
    FILE* stream_fp, const DvzDrp2Command* command, uint32_t index)
{
    ANN(stream_fp);
    ANN(command);
    if (dvz_fprintf(
            stream_fp,
            "{\"type\":\"command\",\"index\":%" PRIu32 ",\"cmd_type\":%d,"
            "\"op\":\"CreateBindGroupLayout\",\"id\":%" PRIu64
            ",\"entry_count\":%" PRIu32,
            index, (int)command->type, command->u.create_bind_group_layout.id,
            command->u.create_bind_group_layout.entry_count) <= 0)
        return false;
    for (uint32_t i = 0; i < DVZ_DRP2_MAX_BINDINGS; i++)
    {
        const DvzDrp2BindGroupLayoutEntry* e =
            &command->u.create_bind_group_layout.entries[i];
        if (dvz_fprintf(
                stream_fp,
                ",\"entry%" PRIu32 "_binding\":%" PRIu32
                ",\"entry%" PRIu32 "_binding_type\":%d"
                ",\"entry%" PRIu32 "_visibility\":%" PRIu32
                ",\"entry%" PRIu32 "_access\":%d"
                ",\"entry%" PRIu32 "_has_dynamic_offset\":%u",
                i, e->binding, i, (int)e->binding_type, i, e->visibility, i, (int)e->access, i,
                e->has_dynamic_offset ? 1u : 0u) <= 0)
            return false;
    }
    return dvz_fprintf(stream_fp, "}\n") > 0;
}



/**
 * Write CreateBindGroup as indexed scalar JSON fields.
 *
 * @param stream_fp stream JSONL file
 * @param command command to write
 * @param index command index
 * @return whether the command record was written
 */
static bool _recording_write_create_bind_group(
    FILE* stream_fp, const DvzDrp2Command* command, uint32_t index)
{
    ANN(stream_fp);
    ANN(command);
    if (dvz_fprintf(
            stream_fp,
            "{\"type\":\"command\",\"index\":%" PRIu32 ",\"cmd_type\":%d,"
            "\"op\":\"CreateBindGroup\",\"id\":%" PRIu64
            ",\"bind_group_layout_id\":%" PRIu64 ",\"entry_count\":%" PRIu32,
            index, (int)command->type, command->u.create_bind_group.id,
            command->u.create_bind_group.bind_group_layout_id,
            command->u.create_bind_group.entry_count) <= 0)
        return false;
    for (uint32_t i = 0; i < DVZ_DRP2_MAX_BINDINGS; i++)
    {
        const DvzDrp2BindGroupEntry* e = &command->u.create_bind_group.entries[i];
        if (dvz_fprintf(
                stream_fp,
                ",\"entry%" PRIu32 "_binding\":%" PRIu32
                ",\"entry%" PRIu32 "_binding_type\":%d"
                ",\"entry%" PRIu32 "_resource_kind\":%d"
                ",\"entry%" PRIu32 "_resource_id\":%" PRIu64
                ",\"entry%" PRIu32 "_offset\":%" PRIu64
                ",\"entry%" PRIu32 "_size\":%" PRIu64,
                i, e->binding, i, (int)e->binding_type, i, (int)e->resource_kind, i,
                e->resource_id, i, e->offset, i, e->size) <= 0)
            return false;
    }
    return dvz_fprintf(stream_fp, "}\n") > 0;
}



/**
 * Write a BeginRenderPass command as indexed scalar JSON fields.
 *
 * @param stream_fp stream JSONL file
 * @param command command to write
 * @param index command index
 * @return whether the command record was written
 */
static bool _recording_write_begin_render_pass(
    FILE* stream_fp, const DvzDrp2Command* command, uint32_t index)
{
    ANN(stream_fp);
    ANN(command);
    if (dvz_fprintf(
            stream_fp,
            "{\"type\":\"command\",\"index\":%" PRIu32 ",\"cmd_type\":%d,"
            "\"op\":\"BeginRenderPass\",\"id\":%" PRIu64 ",\"encoder_id\":%" PRIu64
            ",\"texture_id\":%" PRIu64 ",\"color_attachment_count\":%" PRIu32
            ",\"has_depth_attachment\":%u,\"depth_texture_id\":%" PRIu64
            ",\"depth_load_op\":%" PRIu32
            ",\"depth_store_op\":%" PRIu32 ",\"depth_access\":%" PRIu32
            ",\"depth_ops_explicit\":%u,\"clear_depth\":%.9g,"
            "\"clear_color0\":%.9g,\"clear_color1\":%.9g,\"clear_color2\":%.9g,"
            "\"clear_color3\":%.9g,\"viewport0\":%.9g,\"viewport1\":%.9g,"
            "\"viewport2\":%.9g,\"viewport3\":%.9g,\"clear\":%u",
            index, (int)command->type, command->u.begin_render_pass.id,
            command->u.begin_render_pass.encoder_id, command->u.begin_render_pass.texture_id,
            command->u.begin_render_pass.color_attachment_count,
            command->u.begin_render_pass.has_depth_attachment ? 1u : 0u,
            command->u.begin_render_pass.depth_texture_id,
            (uint32_t)command->u.begin_render_pass.depth_load_op,
            (uint32_t)command->u.begin_render_pass.depth_store_op,
            (uint32_t)command->u.begin_render_pass.depth_access,
            command->u.begin_render_pass.depth_ops_explicit ? 1u : 0u,
            (double)command->u.begin_render_pass.clear_depth,
            (double)command->u.begin_render_pass.clear_color[0],
            (double)command->u.begin_render_pass.clear_color[1],
            (double)command->u.begin_render_pass.clear_color[2],
            (double)command->u.begin_render_pass.clear_color[3],
            (double)command->u.begin_render_pass.viewport[0],
            (double)command->u.begin_render_pass.viewport[1],
            (double)command->u.begin_render_pass.viewport[2],
            (double)command->u.begin_render_pass.viewport[3],
            command->u.begin_render_pass.clear ? 1u : 0u) <= 0)
        return false;

    for (uint32_t i = 0; i < DVZ_DRP2_MAX_COLOR_ATTACHMENTS; i++)
    {
        const DvzDrp2ColorAttachment* a = &command->u.begin_render_pass.color_attachments[i];
        if (dvz_fprintf(
                stream_fp,
                ",\"ca%" PRIu32 "_texture_id\":%" PRIu64 ",\"ca%" PRIu32 "_clear\":%u,"
                "\"ca%" PRIu32 "_resolve_texture_id\":%" PRIu64
                ",\"ca%" PRIu32 "_resolve_mode\":%" PRIu32 ","
                "\"ca%" PRIu32 "_load_op\":%" PRIu32 ",\"ca%" PRIu32 "_store_op\":%" PRIu32 ","
                "\"ca%" PRIu32 "_access\":%" PRIu32 ","
                "\"ca%" PRIu32 "_clear_color0\":%.9g,\"ca%" PRIu32 "_clear_color1\":%.9g,"
                "\"ca%" PRIu32 "_clear_color2\":%.9g,\"ca%" PRIu32 "_clear_color3\":%.9g",
                i, a->texture_id, i, a->clear ? 1u : 0u, i, a->resolve_texture_id, i,
                a->resolve_mode, i, (uint32_t)a->load_op, i,
                (uint32_t)a->store_op, i, (uint32_t)a->access, i, (double)a->clear_color[0], i,
                (double)a->clear_color[1], i, (double)a->clear_color[2], i,
                (double)a->clear_color[3]) <= 0)
            return false;
    }
    return dvz_fprintf(stream_fp, "}\n") > 0;
}



/**
 * Write one portable JSON command record when the command is in the MVP subset.
 *
 * @param root recording root directory
 * @param stream_fp stream JSONL file
 * @param command command to write
 * @param index command index
 * @param blob_index running blob index
 * @param out_supported whether the command was in the portable subset
 * @return whether the command record was written
 */
static bool _recording_write_portable_command(
    const char* root, FILE* stream_fp, const DvzDrp2Command* command, uint32_t index,
    uint32_t* blob_index, bool* out_supported)
{
    ANN(root);
    ANN(stream_fp);
    ANN(command);
    ANN(blob_index);
    ANN(out_supported);
    *out_supported = true;

    switch (command->type)
    {
    case DVZ_DRP2_COMMAND_HELLO_RENDERER:
        if (!_recording_json_string_safe(command->u.handshake.name))
        {
            *out_supported = false;
            return true;
        }
        return dvz_fprintf(
                   stream_fp,
                   "{\"type\":\"command\",\"index\":%" PRIu32 ",\"cmd_type\":%d,"
                   "\"op\":\"HelloRenderer\",\"name\":\"%s\"}\n",
                   index, (int)command->type, command->u.handshake.name) > 0;
    case DVZ_DRP2_COMMAND_RENDERER_HELLO_REPLY:
        if (!_recording_json_string_safe(command->u.handshake.name))
        {
            *out_supported = false;
            return true;
        }
        return dvz_fprintf(
                   stream_fp,
                   "{\"type\":\"command\",\"index\":%" PRIu32 ",\"cmd_type\":%d,"
                   "\"op\":\"RendererHelloReply\",\"name\":\"%s\"}\n",
                   index, (int)command->type, command->u.handshake.name) > 0;
    case DVZ_DRP2_COMMAND_CREATE_BUFFER:
        return dvz_fprintf(
                   stream_fp,
                   "{\"type\":\"command\",\"index\":%" PRIu32 ",\"cmd_type\":%d,"
                   "\"op\":\"CreateBuffer\",\"id\":%" PRIu64 ",\"size\":%" PRIu64
                   ",\"usage\":%" PRIu32 "}\n",
                   index, (int)command->type, command->u.create_buffer.id,
                   command->u.create_buffer.size, command->u.create_buffer.usage) > 0;
    case DVZ_DRP2_COMMAND_CREATE_TEXTURE:
        return dvz_fprintf(
                   stream_fp,
                   "{\"type\":\"command\",\"index\":%" PRIu32 ",\"cmd_type\":%d,"
                   "\"op\":\"CreateTexture\",\"id\":%" PRIu64 ",\"width\":%" PRIu32
                   ",\"height\":%" PRIu32 ",\"depth\":%" PRIu32 ",\"format\":%" PRIu32
                   ",\"usage\":%" PRIu32 ",\"sample_count\":%" PRIu32 "}\n",
                   index, (int)command->type, command->u.create_texture.id,
                   command->u.create_texture.width, command->u.create_texture.height,
                   command->u.create_texture.depth, command->u.create_texture.format,
                   command->u.create_texture.usage,
                   command->u.create_texture.sample_count != 0 ?
                       command->u.create_texture.sample_count :
                       1) > 0;
    case DVZ_DRP2_COMMAND_CREATE_SHADER_MODULE:
        if (!_recording_write_create_shader_module(
                root, stream_fp, command, index, blob_index))
        {
            *out_supported = false;
            return true;
        }
        return true;
    case DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE:
        return _recording_write_create_render_pipeline(stream_fp, command, index);
    case DVZ_DRP2_COMMAND_CREATE_COMPUTE_PIPELINE:
        return _recording_write_create_compute_pipeline(stream_fp, command, index);
    case DVZ_DRP2_COMMAND_CREATE_SAMPLER:
        return dvz_fprintf(
                   stream_fp,
                   "{\"type\":\"command\",\"index\":%" PRIu32 ",\"cmd_type\":%d,"
                   "\"op\":\"CreateSampler\",\"id\":%" PRIu64 "}\n",
                   index, (int)command->type, command->u.create_sampler.id) > 0;
    case DVZ_DRP2_COMMAND_CREATE_BIND_GROUP_LAYOUT:
        return _recording_write_create_bind_group_layout(stream_fp, command, index);
    case DVZ_DRP2_COMMAND_CREATE_BIND_GROUP:
        return _recording_write_create_bind_group(stream_fp, command, index);
    case DVZ_DRP2_COMMAND_WRITE_BUFFER:
    {
        char payload_rel[128] = {0};
        uint64_t payload_size = command->u.write_buffer.size;
        char* inline_payload = NULL;
        if (payload_size <= DVZ_DRP2_RECORDING_INLINE_PAYLOAD_MAX_SIZE)
        {
            inline_payload = _recording_inline_payload_base64(
                command->u.write_buffer.data_raw, command->u.write_buffer.data_base64,
                payload_size);
            if (inline_payload == NULL)
                return false;
        }
        if (payload_size > 0 &&
            inline_payload == NULL &&
            !_recording_write_payload_ref(
                root, blob_index, command->u.write_buffer.data_raw,
                command->u.write_buffer.data_base64, payload_size, payload_rel,
                sizeof(payload_rel)))
            return false;
        bool ok = false;
        if (inline_payload != NULL)
        {
            ok = dvz_fprintf(
                     stream_fp,
                     "{\"type\":\"command\",\"index\":%" PRIu32 ",\"cmd_type\":%d,"
                     "\"op\":\"WriteBuffer\",\"buffer_id\":%" PRIu64 ",\"offset\":%" PRIu64
                     ",\"size\":%" PRIu64 ",\"payload_base64\":\"%s\","
                     "\"payload_size\":%" PRIu64 "}\n",
                     index, (int)command->type, command->u.write_buffer.buffer_id,
                     command->u.write_buffer.offset, payload_size, inline_payload,
                     payload_size) > 0;
            dvz_free(inline_payload);
            return ok;
        }
        return dvz_fprintf(
                   stream_fp,
                   "{\"type\":\"command\",\"index\":%" PRIu32 ",\"cmd_type\":%d,"
                   "\"op\":\"WriteBuffer\",\"buffer_id\":%" PRIu64 ",\"offset\":%" PRIu64
                   ",\"size\":%" PRIu64 ",\"payload_blob\":\"%s\","
                   "\"payload_size\":%" PRIu64 "}\n",
                   index, (int)command->type, command->u.write_buffer.buffer_id,
                   command->u.write_buffer.offset, payload_size, payload_rel, payload_size) > 0;
    }
    case DVZ_DRP2_COMMAND_WRITE_TEXTURE:
    {
        uint64_t payload_size = 0;
        if (!_recording_texture_payload_size(command, &payload_size))
            return false;
        char payload_rel[128] = {0};
        char* inline_payload = NULL;
        if (payload_size <= DVZ_DRP2_RECORDING_INLINE_PAYLOAD_MAX_SIZE)
        {
            inline_payload = _recording_inline_payload_base64(
                command->u.write_texture.data_raw, command->u.write_texture.data_base64,
                payload_size);
            if (inline_payload == NULL)
                return false;
        }
        if (payload_size > 0 &&
            inline_payload == NULL &&
            !_recording_write_payload_ref(
                root, blob_index, command->u.write_texture.data_raw,
                command->u.write_texture.data_base64, payload_size, payload_rel,
                sizeof(payload_rel)))
            return false;
        if (inline_payload != NULL)
        {
            bool ok = dvz_fprintf(
                          stream_fp,
                          "{\"type\":\"command\",\"index\":%" PRIu32 ",\"cmd_type\":%d,"
                          "\"op\":\"WriteTexture\",\"texture_id\":%" PRIu64
                          ",\"mip_level\":%" PRIu32 ",\"origin_x\":%" PRIu32
                          ",\"origin_y\":%" PRIu32 ",\"origin_z\":%" PRIu32
                          ",\"width\":%" PRIu32 ",\"height\":%" PRIu32
                          ",\"depth\":%" PRIu32 ",\"bytes_per_row\":%" PRIu32
                          ",\"rows_per_image\":%" PRIu32 ",\"payload_base64\":\"%s\","
                          "\"payload_size\":%" PRIu64 "}\n",
                          index, (int)command->type, command->u.write_texture.texture_id,
                          command->u.write_texture.mip_level, command->u.write_texture.origin_x,
                          command->u.write_texture.origin_y, command->u.write_texture.origin_z,
                          command->u.write_texture.width, command->u.write_texture.height,
                          command->u.write_texture.depth, command->u.write_texture.bytes_per_row,
                          command->u.write_texture.rows_per_image, inline_payload, payload_size) >
                      0;
            dvz_free(inline_payload);
            return ok;
        }
        return dvz_fprintf(
                   stream_fp,
                   "{\"type\":\"command\",\"index\":%" PRIu32 ",\"cmd_type\":%d,"
                   "\"op\":\"WriteTexture\",\"texture_id\":%" PRIu64
                   ",\"mip_level\":%" PRIu32 ",\"origin_x\":%" PRIu32
                   ",\"origin_y\":%" PRIu32 ",\"origin_z\":%" PRIu32
                   ",\"width\":%" PRIu32 ",\"height\":%" PRIu32 ",\"depth\":%" PRIu32
                   ",\"bytes_per_row\":%" PRIu32 ",\"rows_per_image\":%" PRIu32
                   ",\"payload_blob\":\"%s\",\"payload_size\":%" PRIu64 "}\n",
                   index, (int)command->type, command->u.write_texture.texture_id,
                   command->u.write_texture.mip_level, command->u.write_texture.origin_x,
                   command->u.write_texture.origin_y, command->u.write_texture.origin_z,
                   command->u.write_texture.width, command->u.write_texture.height,
                   command->u.write_texture.depth, command->u.write_texture.bytes_per_row,
                   command->u.write_texture.rows_per_image, payload_rel, payload_size) > 0;
    }
    case DVZ_DRP2_COMMAND_BEGIN_COMMAND_ENCODER:
        return dvz_fprintf(
                   stream_fp,
                   "{\"type\":\"command\",\"index\":%" PRIu32 ",\"cmd_type\":%d,"
                   "\"op\":\"BeginCommandEncoder\",\"id\":%" PRIu64 "}\n",
                   index, (int)command->type, command->u.begin_command_encoder.id) > 0;
    case DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS:
        return _recording_write_begin_render_pass(stream_fp, command, index);
    case DVZ_DRP2_COMMAND_SET_VIEWPORT:
        return dvz_fprintf(
                   stream_fp,
                   "{\"type\":\"command\",\"index\":%" PRIu32 ",\"cmd_type\":%d,"
                   "\"op\":\"SetViewport\",\"pass_id\":%" PRIu64
                   ",\"x\":%.9g,\"y\":%.9g,\"width\":%.9g,\"height\":%.9g}\n",
                   index, (int)command->type, command->u.set_viewport.pass_id,
                   (double)command->u.set_viewport.viewport[0],
                   (double)command->u.set_viewport.viewport[1],
                   (double)command->u.set_viewport.viewport[2],
                   (double)command->u.set_viewport.viewport[3]) > 0;
    case DVZ_DRP2_COMMAND_SET_SCISSOR:
        return dvz_fprintf(
                   stream_fp,
                   "{\"type\":\"command\",\"index\":%" PRIu32 ",\"cmd_type\":%d,"
                   "\"op\":\"SetScissor\",\"pass_id\":%" PRIu64
                   ",\"x\":%.9g,\"y\":%.9g,\"width\":%.9g,\"height\":%.9g}\n",
                   index, (int)command->type, command->u.set_scissor.pass_id,
                   (double)command->u.set_scissor.scissor[0],
                   (double)command->u.set_scissor.scissor[1],
                   (double)command->u.set_scissor.scissor[2],
                   (double)command->u.set_scissor.scissor[3]) > 0;
    case DVZ_DRP2_COMMAND_SET_PIPELINE:
        return dvz_fprintf(
                   stream_fp,
                   "{\"type\":\"command\",\"index\":%" PRIu32 ",\"cmd_type\":%d,"
                   "\"op\":\"SetPipeline\",\"pass_id\":%" PRIu64 ",\"pipeline_id\":%" PRIu64
                   "}\n",
                   index, (int)command->type, command->u.set_pipeline.pass_id,
                   command->u.set_pipeline.pipeline_id) > 0;
    case DVZ_DRP2_COMMAND_SET_BIND_GROUP:
        if (dvz_fprintf(
                stream_fp,
                "{\"type\":\"command\",\"index\":%" PRIu32 ",\"cmd_type\":%d,"
                "\"op\":\"SetBindGroup\",\"pass_id\":%" PRIu64 ",\"slot\":%" PRIu32
                ",\"bind_group_id\":%" PRIu64 ",\"dynamic_offset_count\":%" PRIu32,
                index, (int)command->type, command->u.set_bind_group.pass_id,
                command->u.set_bind_group.slot, command->u.set_bind_group.bind_group_id,
                command->u.set_bind_group.dynamic_offset_count) <= 0)
            return false;
        for (uint32_t i = 0; i < DVZ_DRP2_MAX_BINDINGS; i++)
        {
            if (dvz_fprintf(
                    stream_fp, ",\"dynamic_offset%" PRIu32 "\":%" PRIu64, i,
                    command->u.set_bind_group.dynamic_offsets[i]) <= 0)
                return false;
        }
        return dvz_fprintf(stream_fp, "}\n") > 0;
    case DVZ_DRP2_COMMAND_SET_VERTEX_BUFFER:
        return dvz_fprintf(
                   stream_fp,
                   "{\"type\":\"command\",\"index\":%" PRIu32 ",\"cmd_type\":%d,"
                   "\"op\":\"SetVertexBuffer\",\"pass_id\":%" PRIu64 ",\"slot\":%" PRIu32
                   ",\"buffer_id\":%" PRIu64 ",\"offset\":%" PRIu64 "}\n",
                   index, (int)command->type, command->u.set_vertex_buffer.pass_id,
                   command->u.set_vertex_buffer.slot, command->u.set_vertex_buffer.buffer_id,
                   command->u.set_vertex_buffer.offset) > 0;
    case DVZ_DRP2_COMMAND_SET_INDEX_BUFFER:
        if (!_recording_json_string_safe(command->u.set_index_buffer.index_format))
        {
            *out_supported = false;
            return true;
        }
        return dvz_fprintf(
                   stream_fp,
                   "{\"type\":\"command\",\"index\":%" PRIu32 ",\"cmd_type\":%d,"
                   "\"op\":\"SetIndexBuffer\",\"pass_id\":%" PRIu64
                   ",\"buffer_id\":%" PRIu64 ",\"index_format\":\"%s\",\"offset\":%" PRIu64
                   "}\n",
                   index, (int)command->type, command->u.set_index_buffer.pass_id,
                   command->u.set_index_buffer.buffer_id,
                   command->u.set_index_buffer.index_format,
                   command->u.set_index_buffer.offset) > 0;
    case DVZ_DRP2_COMMAND_DRAW:
        return dvz_fprintf(
                   stream_fp,
                   "{\"type\":\"command\",\"index\":%" PRIu32 ",\"cmd_type\":%d,"
                   "\"op\":\"Draw\",\"pass_id\":%" PRIu64 ",\"vertex_count\":%" PRIu32
                   ",\"instance_count\":%" PRIu32 ",\"first_vertex\":%" PRIu32
                   ",\"first_instance\":%" PRIu32 "}\n",
                   index, (int)command->type, command->u.draw.pass_id,
                   command->u.draw.vertex_count, command->u.draw.instance_count,
                   command->u.draw.first_vertex, command->u.draw.first_instance) > 0;
    case DVZ_DRP2_COMMAND_DRAW_INDEXED:
        return dvz_fprintf(
                   stream_fp,
                   "{\"type\":\"command\",\"index\":%" PRIu32 ",\"cmd_type\":%d,"
                   "\"op\":\"DrawIndexed\",\"pass_id\":%" PRIu64 ",\"index_count\":%" PRIu32
                   ",\"instance_count\":%" PRIu32 ",\"first_index\":%" PRIu32
                   ",\"base_vertex\":%" PRId32 ",\"first_instance\":%" PRIu32 "}\n",
                   index, (int)command->type, command->u.draw_indexed.pass_id,
                   command->u.draw_indexed.index_count, command->u.draw_indexed.instance_count,
                   command->u.draw_indexed.first_index, command->u.draw_indexed.base_vertex,
                   command->u.draw_indexed.first_instance) > 0;
    case DVZ_DRP2_COMMAND_END_RENDER_PASS:
        return dvz_fprintf(
                   stream_fp,
                   "{\"type\":\"command\",\"index\":%" PRIu32 ",\"cmd_type\":%d,"
                   "\"op\":\"EndRenderPass\",\"pass_id\":%" PRIu64 "}\n",
                   index, (int)command->type, command->u.end_render_pass.pass_id) > 0;
    case DVZ_DRP2_COMMAND_BEGIN_COMPUTE_PASS:
        return dvz_fprintf(
                   stream_fp,
                   "{\"type\":\"command\",\"index\":%" PRIu32 ",\"cmd_type\":%d,"
                   "\"op\":\"BeginComputePass\",\"id\":%" PRIu64 ",\"encoder_id\":%" PRIu64
                   "}\n",
                   index, (int)command->type, command->u.begin_compute_pass.id,
                   command->u.begin_compute_pass.encoder_id) > 0;
    case DVZ_DRP2_COMMAND_DISPATCH_WORKGROUPS:
        return dvz_fprintf(
                   stream_fp,
                   "{\"type\":\"command\",\"index\":%" PRIu32 ",\"cmd_type\":%d,"
                   "\"op\":\"DispatchWorkgroups\",\"pass_id\":%" PRIu64 ",\"x\":%" PRIu32
                   ",\"y\":%" PRIu32 ",\"z\":%" PRIu32 "}\n",
                   index, (int)command->type, command->u.dispatch.pass_id,
                   command->u.dispatch.x, command->u.dispatch.y, command->u.dispatch.z) > 0;
    case DVZ_DRP2_COMMAND_END_COMPUTE_PASS:
        return dvz_fprintf(
                   stream_fp,
                   "{\"type\":\"command\",\"index\":%" PRIu32 ",\"cmd_type\":%d,"
                   "\"op\":\"EndComputePass\",\"pass_id\":%" PRIu64 "}\n",
                   index, (int)command->type, command->u.end_compute_pass.pass_id) > 0;
    case DVZ_DRP2_COMMAND_COPY_BUFFER_TO_BUFFER:
        return dvz_fprintf(
                   stream_fp,
                   "{\"type\":\"command\",\"index\":%" PRIu32 ",\"cmd_type\":%d,"
                   "\"op\":\"CopyBufferToBuffer\",\"encoder_id\":%" PRIu64
                   ",\"src_buffer_id\":%" PRIu64 ",\"src_offset\":%" PRIu64
                   ",\"dst_buffer_id\":%" PRIu64 ",\"dst_offset\":%" PRIu64
                   ",\"size\":%" PRIu64 "}\n",
                   index, (int)command->type, command->u.copy_buffer_to_buffer.encoder_id,
                   command->u.copy_buffer_to_buffer.src_buffer_id,
                   command->u.copy_buffer_to_buffer.src_offset,
                   command->u.copy_buffer_to_buffer.dst_buffer_id,
                   command->u.copy_buffer_to_buffer.dst_offset,
                   command->u.copy_buffer_to_buffer.size) > 0;
    case DVZ_DRP2_COMMAND_COPY_BUFFER_TO_TEXTURE:
        return dvz_fprintf(
                   stream_fp,
                   "{\"type\":\"command\",\"index\":%" PRIu32 ",\"cmd_type\":%d,"
                   "\"op\":\"CopyBufferToTexture\",\"encoder_id\":%" PRIu64
                   ",\"src_buffer_id\":%" PRIu64 ",\"src_offset\":%" PRIu64
                   ",\"bytes_per_row\":%" PRIu32 ",\"rows_per_image\":%" PRIu32
                   ",\"dst_texture_id\":%" PRIu64 ",\"dst_mip_level\":%" PRIu32
                   ",\"dst_origin_x\":%" PRIu32 ",\"dst_origin_y\":%" PRIu32
                   ",\"dst_origin_z\":%" PRIu32 ",\"width\":%" PRIu32
                   ",\"height\":%" PRIu32 ",\"depth\":%" PRIu32 "}\n",
                   index, (int)command->type, command->u.copy_buffer_to_texture.encoder_id,
                   command->u.copy_buffer_to_texture.src_buffer_id,
                   command->u.copy_buffer_to_texture.src_offset,
                   command->u.copy_buffer_to_texture.bytes_per_row,
                   command->u.copy_buffer_to_texture.rows_per_image,
                   command->u.copy_buffer_to_texture.dst_texture_id,
                   command->u.copy_buffer_to_texture.dst_mip_level,
                   command->u.copy_buffer_to_texture.dst_origin_x,
                   command->u.copy_buffer_to_texture.dst_origin_y,
                   command->u.copy_buffer_to_texture.dst_origin_z,
                   command->u.copy_buffer_to_texture.width,
                   command->u.copy_buffer_to_texture.height,
                   command->u.copy_buffer_to_texture.depth) > 0;
    case DVZ_DRP2_COMMAND_COPY_TEXTURE_TO_BUFFER:
        return dvz_fprintf(
                   stream_fp,
                   "{\"type\":\"command\",\"index\":%" PRIu32 ",\"cmd_type\":%d,"
                   "\"op\":\"CopyTextureToBuffer\",\"encoder_id\":%" PRIu64
                   ",\"src_texture_id\":%" PRIu64 ",\"dst_buffer_id\":%" PRIu64
                   ",\"dst_offset\":%" PRIu64 ",\"width\":%" PRIu32
                   ",\"height\":%" PRIu32 ",\"bytes_per_row\":%" PRIu32
                   ",\"rows_per_image\":%" PRIu32 "}\n",
                   index, (int)command->type, command->u.copy_texture_to_buffer.encoder_id,
                   command->u.copy_texture_to_buffer.src_texture_id,
                   command->u.copy_texture_to_buffer.dst_buffer_id,
                   command->u.copy_texture_to_buffer.dst_offset,
                   command->u.copy_texture_to_buffer.width,
                   command->u.copy_texture_to_buffer.height,
                   command->u.copy_texture_to_buffer.bytes_per_row,
                   command->u.copy_texture_to_buffer.rows_per_image) > 0;
    case DVZ_DRP2_COMMAND_COPY_TEXTURE_TO_TEXTURE:
        return dvz_fprintf(
                   stream_fp,
                   "{\"type\":\"command\",\"index\":%" PRIu32 ",\"cmd_type\":%d,"
                   "\"op\":\"CopyTextureToTexture\",\"encoder_id\":%" PRIu64
                   ",\"src_texture_id\":%" PRIu64 ",\"src_mip_level\":%" PRIu32
                   ",\"src_origin_x\":%" PRIu32 ",\"src_origin_y\":%" PRIu32
                   ",\"src_origin_z\":%" PRIu32 ",\"dst_texture_id\":%" PRIu64
                   ",\"dst_mip_level\":%" PRIu32 ",\"dst_origin_x\":%" PRIu32
                   ",\"dst_origin_y\":%" PRIu32 ",\"dst_origin_z\":%" PRIu32
                   ",\"width\":%" PRIu32 ",\"height\":%" PRIu32 ",\"depth\":%" PRIu32
                   "}\n",
                   index, (int)command->type, command->u.copy_texture_to_texture.encoder_id,
                   command->u.copy_texture_to_texture.src_texture_id,
                   command->u.copy_texture_to_texture.src_mip_level,
                   command->u.copy_texture_to_texture.src_origin_x,
                   command->u.copy_texture_to_texture.src_origin_y,
                   command->u.copy_texture_to_texture.src_origin_z,
                   command->u.copy_texture_to_texture.dst_texture_id,
                   command->u.copy_texture_to_texture.dst_mip_level,
                   command->u.copy_texture_to_texture.dst_origin_x,
                   command->u.copy_texture_to_texture.dst_origin_y,
                   command->u.copy_texture_to_texture.dst_origin_z,
                   command->u.copy_texture_to_texture.width,
                   command->u.copy_texture_to_texture.height,
                   command->u.copy_texture_to_texture.depth) > 0;
    case DVZ_DRP2_COMMAND_FINISH_COMMAND_ENCODER:
        return dvz_fprintf(
                   stream_fp,
                   "{\"type\":\"command\",\"index\":%" PRIu32 ",\"cmd_type\":%d,"
                   "\"op\":\"FinishCommandEncoder\",\"encoder_id\":%" PRIu64
                   ",\"command_buffer_id\":%" PRIu64 "}\n",
                   index, (int)command->type, command->u.finish_command_encoder.encoder_id,
                   command->u.finish_command_encoder.command_buffer_id) > 0;
    case DVZ_DRP2_COMMAND_QUEUE_SUBMIT:
        return dvz_fprintf(
                   stream_fp,
                   "{\"type\":\"command\",\"index\":%" PRIu32 ",\"cmd_type\":%d,"
                   "\"op\":\"QueueSubmit\",\"command_buffer_id\":%" PRIu64
                   ",\"submission_id\":%" PRIu64 ",\"has_readback\":%u,"
                   "\"buffer_id\":%" PRIu64 ",\"offset\":%" PRIu64 ",\"size\":%" PRIu64
                   "}\n",
                   index, (int)command->type, command->u.queue_submit.command_buffer_id,
                   command->u.queue_submit.submission_id,
                   command->u.queue_submit.has_readback ? 1u : 0u,
                   command->u.queue_submit.buffer_id, command->u.queue_submit.offset,
                   command->u.queue_submit.size) > 0;
    default:
        *out_supported = false;
        return true;
    }
}



/**
 * Write a command blob and optional payload blob.
 *
 * @param root recording root directory
 * @param stream_fp stream JSONL file
 * @param command command to write
 * @param index command index
 * @param blob_index running blob index
 * @return whether the command record was written
 */
static bool _recording_write_command(
    const char* root, FILE* stream_fp, const DvzDrp2Command* command, uint32_t index,
    uint32_t* blob_index)
{
    ANN(root);
    ANN(stream_fp);
    ANN(command);
    ANN(blob_index);

    bool portable_supported = false;
    if (!_recording_write_portable_command(
            root, stream_fp, command, index, blob_index, &portable_supported))
        return false;
    if (portable_supported)
        return true;

    char command_rel[128] = {0};
    char command_path[DVZ_DRP2_RECORDING_PATH_SIZE] = {0};
    dvz_snprintf(command_rel, sizeof(command_rel), "blobs/%08" PRIu32 ".cmd", (*blob_index)++);
    if (!_recording_join(root, command_rel, command_path, sizeof(command_path)))
        return false;

    DvzDrp2Command stored = *command;
    const void* payload = NULL;
    const char* payload_base64 = NULL;
    uint64_t payload_size = 0;
    bool has_payload = false;
    bool shader_code_payload = false;

    if (stored.type == DVZ_DRP2_COMMAND_WRITE_BUFFER)
    {
        payload = command->u.write_buffer.data_raw;
        payload_base64 = command->u.write_buffer.data_base64;
        payload_size = command->u.write_buffer.size;
        stored.u.write_buffer.data_raw = NULL;
        stored.u.write_buffer.data_base64 = NULL;
        has_payload = payload_size > 0;
    }
    else if (stored.type == DVZ_DRP2_COMMAND_WRITE_TEXTURE)
    {
        payload = command->u.write_texture.data_raw;
        payload_base64 = command->u.write_texture.data_base64;
        if (!_recording_texture_payload_size(command, &payload_size))
            return false;
        stored.u.write_texture.data_raw = NULL;
        stored.u.write_texture.data_base64 = NULL;
        has_payload = payload_size > 0;
    }
    else if (stored.type == DVZ_DRP2_COMMAND_CREATE_SHADER_MODULE)
    {
        if (command->u.create_shader_module.spirv != NULL &&
            command->u.create_shader_module.spirv_size > 0)
        {
            payload = command->u.create_shader_module.spirv;
            payload_size = command->u.create_shader_module.spirv_size;
            stored.u.create_shader_module.spirv = NULL;
            has_payload = true;
        }
        else if (command->u.create_shader_module.code != NULL)
        {
            payload = command->u.create_shader_module.code;
            payload_size = strlen(command->u.create_shader_module.code) + 1;
            stored.u.create_shader_module.code = NULL;
            has_payload = true;
            shader_code_payload = true;
        }
    }

    if (!_recording_write_blob(command_path, &stored, sizeof(DvzDrp2Command)))
        return false;

    if (!has_payload)
    {
        return dvz_fprintf(
                   stream_fp,
                   "{\"type\":\"command\",\"index\":%" PRIu32 ",\"cmd_type\":%d,"
                   "\"command_blob\":\"%s\",\"command_size\":%zu}\n",
                   index, (int)command->type, command_rel, sizeof(DvzDrp2Command)) > 0;
    }

    char payload_rel[128] = {0};
    char payload_path[DVZ_DRP2_RECORDING_PATH_SIZE] = {0};
    dvz_snprintf(payload_rel, sizeof(payload_rel), "blobs/%08" PRIu32 ".bin", (*blob_index)++);
    if (!_recording_join(root, payload_rel, payload_path, sizeof(payload_path)))
        return false;
    if (!_recording_write_payload_blob(payload_path, payload, payload_base64, payload_size))
        return false;

    return dvz_fprintf(
               stream_fp,
               "{\"type\":\"command\",\"index\":%" PRIu32 ",\"cmd_type\":%d,"
               "\"command_blob\":\"%s\",\"command_size\":%zu,\"payload_blob\":\"%s\","
               "\"payload_size\":%" PRIu64 ",\"payload_kind\":\"%s\"}\n",
               index, (int)command->type, command_rel, sizeof(DvzDrp2Command), payload_rel,
               payload_size, shader_code_payload ? "shader_code" : "bytes") > 0;
}



/**
 * Parse one unsigned integer field from a JSONL record.
 *
 * @param line JSONL record
 * @param key field key including surrounding quotes and trailing colon
 * @param out parsed value
 * @return whether the field was found and parsed
 */
static bool _recording_line_u64(const char* line, const char* key, uint64_t* out)
{
    ANN(line);
    ANN(key);
    ANN(out);
    const char* p = strstr(line, key);
    if (p == NULL)
        return false;
    p += strlen(key);
    if (*p == '-')
        return false;
    char* end = NULL;
    errno = 0;
    unsigned long long value = strtoull(p, &end, 10);
    if (end == p || errno == ERANGE || value > UINT64_MAX)
        return false;
    *out = (uint64_t)value;
    return true;
}



/**
 * Parse one double field from a JSONL record.
 *
 * @param line JSONL record
 * @param key field key including surrounding quotes and trailing colon
 * @param out parsed value
 * @return whether the field was found and parsed
 */
static bool _recording_line_double(const char* line, const char* key, double* out)
{
    ANN(line);
    ANN(key);
    ANN(out);
    const char* p = strstr(line, key);
    if (p == NULL)
        return false;
    p += strlen(key);
    char* end = NULL;
    errno = 0;
    double value = strtod(p, &end);
    if (end == p || errno == ERANGE)
        return false;
    *out = value;
    return true;
}



/**
 * Parse one unsigned 32-bit integer field from a JSONL record.
 *
 * @param line JSONL record
 * @param key field key including surrounding quotes and trailing colon
 * @param out parsed value
 * @return whether the field was found and parsed
 */
static bool _recording_line_u32(const char* line, const char* key, uint32_t* out)
{
    ANN(line);
    ANN(key);
    ANN(out);
    uint64_t value = 0;
    if (!_recording_line_u64(line, key, &value) || value > UINT32_MAX)
        return false;
    *out = (uint32_t)value;
    return true;
}



/**
 * Parse one signed 32-bit integer field from a JSONL record.
 *
 * @param line JSONL record
 * @param key field key including surrounding quotes and trailing colon
 * @param out parsed value
 * @return whether the field was found and parsed
 */
static bool _recording_line_i32(const char* line, const char* key, int32_t* out)
{
    ANN(line);
    ANN(key);
    ANN(out);
    const char* p = strstr(line, key);
    if (p == NULL)
        return false;
    p += strlen(key);
    char* end = NULL;
    errno = 0;
    long value = strtol(p, &end, 10);
    if (end == p || errno == ERANGE || value < INT32_MIN || value > INT32_MAX)
        return false;
    *out = (int32_t)value;
    return true;
}



/**
 * Parse one float field from a JSONL record.
 *
 * @param line JSONL record
 * @param key field key including surrounding quotes and trailing colon
 * @param out parsed value
 * @return whether the field was found and parsed
 */
static bool _recording_line_float(const char* line, const char* key, float* out)
{
    ANN(line);
    ANN(key);
    ANN(out);
    const char* p = strstr(line, key);
    if (p == NULL)
        return false;
    p += strlen(key);
    char* end = NULL;
    errno = 0;
    float value = strtof(p, &end);
    if (end == p || errno == ERANGE)
        return false;
    *out = value;
    return true;
}



/**
 * Parse one bool field encoded as 0 or 1 from a JSONL record.
 *
 * @param line JSONL record
 * @param key field key including surrounding quotes and trailing colon
 * @param out parsed value
 * @return whether the field was found and parsed
 */
static bool _recording_line_bool(const char* line, const char* key, bool* out)
{
    ANN(line);
    ANN(key);
    ANN(out);
    uint32_t value = 0;
    if (!_recording_line_u32(line, key, &value) || value > 1)
        return false;
    *out = value != 0;
    return true;
}



/**
 * Build an indexed field name.
 *
 * @param out output field buffer
 * @param out_size output field buffer size
 * @param prefix field prefix
 * @param index array index
 * @param suffix field suffix
 * @return whether the field name fit in the output buffer
 */
static bool _recording_field_name(
    char* out, uint64_t out_size, const char* prefix, uint32_t index, const char* suffix)
{
    ANN(out);
    ANN(prefix);
    ANN(suffix);
    int rc = suffix[0] == '\0'
                 ? dvz_snprintf(out, (size_t)out_size, "\"%s%" PRIu32 "\":", prefix, index)
                 : dvz_snprintf(
                       out, (size_t)out_size, "\"%s%" PRIu32 "_%s\":", prefix, index, suffix);
    return rc >= 0 && (uint64_t)rc < out_size;
}



/**
 * Parse one indexed unsigned 64-bit field from a JSONL record.
 *
 * @param line JSONL record
 * @param prefix field prefix
 * @param index array index
 * @param suffix field suffix
 * @param out parsed value
 * @return whether the field was found and parsed
 */
static bool _recording_line_indexed_u64(
    const char* line, const char* prefix, uint32_t index, const char* suffix, uint64_t* out)
{
    char key[64] = {0};
    return _recording_field_name(key, sizeof(key), prefix, index, suffix) &&
           _recording_line_u64(line, key, out);
}



/**
 * Parse one indexed unsigned 32-bit field from a JSONL record.
 *
 * @param line JSONL record
 * @param prefix field prefix
 * @param index array index
 * @param suffix field suffix
 * @param out parsed value
 * @return whether the field was found and parsed
 */
static bool _recording_line_indexed_u32(
    const char* line, const char* prefix, uint32_t index, const char* suffix, uint32_t* out)
{
    char key[64] = {0};
    return _recording_field_name(key, sizeof(key), prefix, index, suffix) &&
           _recording_line_u32(line, key, out);
}



/**
 * Parse one indexed bool field encoded as 0 or 1 from a JSONL record.
 *
 * @param line JSONL record
 * @param prefix field prefix
 * @param index array index
 * @param suffix field suffix
 * @param out parsed value
 * @return whether the field was found and parsed
 */
static bool _recording_line_indexed_bool(
    const char* line, const char* prefix, uint32_t index, const char* suffix, bool* out)
{
    char key[64] = {0};
    return _recording_field_name(key, sizeof(key), prefix, index, suffix) &&
           _recording_line_bool(line, key, out);
}



/**
 * Parse one indexed float field from a JSONL record.
 *
 * @param line JSONL record
 * @param prefix field prefix
 * @param index array index
 * @param suffix field suffix
 * @param out parsed value
 * @return whether the field was found and parsed
 */
static bool _recording_line_indexed_float(
    const char* line, const char* prefix, uint32_t index, const char* suffix, float* out)
{
    char key[64] = {0};
    return _recording_field_name(key, sizeof(key), prefix, index, suffix) &&
           _recording_line_float(line, key, out);
}



/**
 * Parse one string field from a JSONL record.
 *
 * @param line JSONL record
 * @param key field key including surrounding quotes and trailing colon and quote
 * @param out output string
 * @param out_size output string capacity
 * @return whether the field was found and parsed
 */
static bool _recording_line_string(
    const char* line, const char* key, char* out, uint64_t out_size)
{
    ANN(line);
    ANN(key);
    ANN(out);
    const char* p = strstr(line, key);
    if (p == NULL)
        return false;
    p += strlen(key);
    const char* q = strchr(p, '"');
    if (q == NULL || q < p)
        return false;
    uint64_t len = (uint64_t)(q - p);
    if (len >= out_size)
        return false;
    dvz_memcpy(out, (size_t)out_size, p, (size_t)len);
    out[len] = '\0';
    return true;
}



/**
 * Attach one payload blob to a decoded command.
 *
 * @param owner stream owner
 * @param command decoded command
 * @param payload owned payload bytes
 * @param payload_size payload byte size
 * @param payload_kind optional payload kind
 * @return whether the payload was attached
 */
static bool _recording_attach_payload(
    DvzDrp2RecordingOwner* owner, DvzDrp2Command* command, void* payload,
    uint64_t payload_size, const char* payload_kind)
{
    ANN(owner);
    ANN(command);
    if (payload == NULL || payload_size == 0)
        return true;
    switch (command->type)
    {
    case DVZ_DRP2_COMMAND_WRITE_BUFFER:
        command->u.write_buffer.data_raw = payload;
        command->u.write_buffer.data_raw_owned = false;
        if (_recording_owner_add(owner, payload))
            return true;
        command->u.write_buffer.data_raw = NULL;
        dvz_free(payload);
        return false;
    case DVZ_DRP2_COMMAND_WRITE_TEXTURE:
        command->u.write_texture.data_raw = payload;
        if (_recording_owner_add(owner, payload))
            return true;
        command->u.write_texture.data_raw = NULL;
        dvz_free(payload);
        return false;
    case DVZ_DRP2_COMMAND_CREATE_SHADER_MODULE:
        if (payload_kind != NULL && strcmp(payload_kind, "shader_code") == 0)
        {
            ((char*)payload)[payload_size - 1] = '\0';
            command->u.create_shader_module.code = (char*)payload;
            return true;
        }
        command->u.create_shader_module.spirv = (const unsigned char*)payload;
        command->u.create_shader_module.spirv_size = payload_size;
        if (_recording_owner_add(owner, payload))
            return true;
        command->u.create_shader_module.spirv = NULL;
        command->u.create_shader_module.spirv_size = 0;
        dvz_free(payload);
        return false;
    default:
        dvz_free(payload);
        return false;
    }
}



/**
 * Load a portable-command payload blob when present.
 *
 * @param root recording root directory
 * @param line JSONL command record
 * @param out_payload output owned payload bytes, or NULL for an empty payload
 * @param out_payload_size output payload byte size
 * @return whether the payload fields were valid and the blob was loaded
 */
static bool _recording_read_payload_ref(
    const char* root, const char* line, void** out_payload, uint64_t* out_payload_size)
{
    ANN(root);
    ANN(line);
    ANN(out_payload);
    ANN(out_payload_size);
    *out_payload = NULL;
    *out_payload_size = 0;
    if (!_recording_line_u64(line, "\"payload_size\":", out_payload_size))
        return false;
    if (*out_payload_size == 0)
        return true;

    char payload_base64[DVZ_DRP2_RECORDING_LINE_SIZE] = {0};
    if (_recording_line_string(
            line, "\"payload_base64\":\"", payload_base64, sizeof(payload_base64)))
    {
        uint8_t* decoded = NULL;
        if (!_dvz_b64_decode_exact(payload_base64, *out_payload_size, &decoded))
            return false;
        *out_payload = decoded;
        return true;
    }

    char payload_rel[128] = {0};
    char payload_path[DVZ_DRP2_RECORDING_PATH_SIZE] = {0};
    if (!_recording_line_string(line, "\"payload_blob\":\"", payload_rel, sizeof(payload_rel)) ||
        !_recording_join(root, payload_rel, payload_path, sizeof(payload_path)))
        return false;
    *out_payload = _recording_read_blob(payload_path, *out_payload_size);
    return *out_payload != NULL;
}



/**
 * Decode CreateRenderPipeline indexed fields.
 *
 * @param line JSONL command record
 * @param command command to fill
 * @return whether the fields were decoded
 */
static bool _recording_read_create_render_pipeline(const char* line, DvzDrp2Command* command)
{
    ANN(line);
    ANN(command);
    command->type = DVZ_DRP2_COMMAND_CREATE_RENDER_PIPELINE;
    command->u.create_render_pipeline.has_raster_state = false;
    command->u.create_render_pipeline.cull_mode = VK_CULL_MODE_NONE;
    command->u.create_render_pipeline.front_face = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    command->u.create_render_pipeline.sample_count = 1;
    command->u.create_render_pipeline.alpha_to_coverage_enabled = false;
    if (!_recording_line_u64(line, "\"id\":", &command->u.create_render_pipeline.id) ||
        !_recording_line_u64(
            line, "\"vertex_shader_module_id\":",
            &command->u.create_render_pipeline.vertex_shader_module_id) ||
        !_recording_line_u64(
            line, "\"fragment_shader_module_id\":",
            &command->u.create_render_pipeline.fragment_shader_module_id) ||
        !_recording_line_u32(
            line, "\"vertex_buffer_slots\":",
            &command->u.create_render_pipeline.vertex_buffer_slots) ||
        !_recording_line_u32(
            line, "\"bind_group_layout_count\":",
            &command->u.create_render_pipeline.bind_group_layout_count) ||
        !_recording_line_bool(
            line, "\"has_depth_attachment\":",
            &command->u.create_render_pipeline.has_depth_attachment) ||
        !_recording_line_bool(
            line, "\"depth_write_enabled\":",
            &command->u.create_render_pipeline.depth_write_enabled) ||
        !_recording_line_u32(
            line, "\"depth_compare_op\":",
            &command->u.create_render_pipeline.depth_compare_op) ||
        !_recording_line_u32(
            line, "\"color_target_count\":",
            &command->u.create_render_pipeline.color_target_count) ||
        !_recording_line_u32(line, "\"topology\":", &command->u.create_render_pipeline.topology) ||
        !_recording_line_u32(
            line, "\"binding_count\":", &command->u.create_render_pipeline.binding_count) ||
        !_recording_line_u32(line, "\"attr_count\":", &command->u.create_render_pipeline.attr_count))
        return false;
    (void)_recording_line_bool(
        line, "\"has_raster_state\":", &command->u.create_render_pipeline.has_raster_state);
    (void)_recording_line_u32(line, "\"cull_mode\":", &command->u.create_render_pipeline.cull_mode);
    (void)_recording_line_u32(
        line, "\"front_face\":", &command->u.create_render_pipeline.front_face);
    (void)_recording_line_u32(
        line, "\"sample_count\":", &command->u.create_render_pipeline.sample_count);
    (void)_recording_line_bool(
        line, "\"alpha_to_coverage_enabled\":",
        &command->u.create_render_pipeline.alpha_to_coverage_enabled);
    (void)_recording_line_string(
        line, "\"builtin_pipeline\":\"", command->u.create_render_pipeline.builtin_pipeline,
        sizeof(command->u.create_render_pipeline.builtin_pipeline));
    (void)_recording_line_u32(
        line, "\"builtin_version\":", &command->u.create_render_pipeline.builtin_version);

    for (uint32_t i = 0; i < DVZ_DRP2_MAX_BIND_GROUPS; i++)
    {
        if (!_recording_line_indexed_u64(
                line, "bgl", i, "id",
                &command->u.create_render_pipeline.bind_group_layout_ids[i]))
            return false;
    }
    for (uint32_t i = 0; i < DVZ_DRP2_MAX_COLOR_ATTACHMENTS; i++)
    {
        DvzDrp2ColorTarget* t = &command->u.create_render_pipeline.color_targets[i];
        if (!_recording_line_indexed_u32(line, "ct", i, "format", &t->format) ||
            !_recording_line_indexed_bool(line, "ct", i, "blend_enabled", &t->blend_enabled) ||
            !_recording_line_indexed_u32(
                line, "ct", i, "src_color_blend_factor", &t->src_color_blend_factor) ||
            !_recording_line_indexed_u32(
                line, "ct", i, "dst_color_blend_factor", &t->dst_color_blend_factor) ||
            !_recording_line_indexed_u32(line, "ct", i, "color_blend_op", &t->color_blend_op) ||
            !_recording_line_indexed_u32(
                line, "ct", i, "src_alpha_blend_factor", &t->src_alpha_blend_factor) ||
            !_recording_line_indexed_u32(
                line, "ct", i, "dst_alpha_blend_factor", &t->dst_alpha_blend_factor) ||
            !_recording_line_indexed_u32(line, "ct", i, "alpha_blend_op", &t->alpha_blend_op) ||
            !_recording_line_indexed_u32(line, "ct", i, "color_write_mask", &t->color_write_mask))
            return false;
    }
    for (uint32_t i = 0; i < 16; i++)
    {
        if (!_recording_line_indexed_u32(
                line, "binding", i, "stride",
                &command->u.create_render_pipeline.binding_strides[i]) ||
            !_recording_line_indexed_u32(
                line, "binding", i, "step_mode",
                &command->u.create_render_pipeline.binding_step_modes[i]))
            return false;
    }
    for (uint32_t i = 0; i < 16; i++)
    {
        if (!_recording_line_indexed_u32(
                line, "attr", i, "binding",
                &command->u.create_render_pipeline.attr_bindings[i]) ||
            !_recording_line_indexed_u32(
                line, "attr", i, "location",
                &command->u.create_render_pipeline.attr_locations[i]) ||
            !_recording_line_indexed_u32(
                line, "attr", i, "format",
                &command->u.create_render_pipeline.attr_formats[i]) ||
            !_recording_line_indexed_u32(
                line, "attr", i, "offset",
                &command->u.create_render_pipeline.attr_offsets[i]))
            return false;
    }
    return true;
}



/**
 * Decode CreateComputePipeline indexed fields.
 *
 * @param line JSONL command record
 * @param command command to fill
 * @return whether the fields were decoded
 */
static bool _recording_read_create_compute_pipeline(const char* line, DvzDrp2Command* command)
{
    ANN(line);
    ANN(command);
    command->type = DVZ_DRP2_COMMAND_CREATE_COMPUTE_PIPELINE;
    if (!_recording_line_u64(line, "\"id\":", &command->u.create_compute_pipeline.id) ||
        !_recording_line_u64(
            line, "\"compute_shader_module_id\":",
            &command->u.create_compute_pipeline.compute_shader_module_id) ||
        !_recording_line_u32(
            line, "\"bind_group_layout_count\":",
            &command->u.create_compute_pipeline.bind_group_layout_count))
        return false;
    for (uint32_t i = 0; i < DVZ_DRP2_MAX_BIND_GROUPS; i++)
    {
        if (!_recording_line_indexed_u64(
                line, "bgl", i, "id",
                &command->u.create_compute_pipeline.bind_group_layout_ids[i]))
            return false;
    }
    return true;
}



/**
 * Decode CreateBindGroupLayout indexed fields.
 *
 * @param line JSONL command record
 * @param command command to fill
 * @return whether the fields were decoded
 */
static bool _recording_read_create_bind_group_layout(const char* line, DvzDrp2Command* command)
{
    ANN(line);
    ANN(command);
    command->type = DVZ_DRP2_COMMAND_CREATE_BIND_GROUP_LAYOUT;
    if (!_recording_line_u64(line, "\"id\":", &command->u.create_bind_group_layout.id) ||
        !_recording_line_u32(
            line, "\"entry_count\":", &command->u.create_bind_group_layout.entry_count))
        return false;
    for (uint32_t i = 0; i < DVZ_DRP2_MAX_BINDINGS; i++)
    {
        DvzDrp2BindGroupLayoutEntry* e = &command->u.create_bind_group_layout.entries[i];
        uint32_t binding_type = 0;
        uint32_t access = 0;
        if (!_recording_line_indexed_u32(line, "entry", i, "binding", &e->binding) ||
            !_recording_line_indexed_u32(line, "entry", i, "binding_type", &binding_type) ||
            !_recording_line_indexed_u32(line, "entry", i, "visibility", &e->visibility) ||
            !_recording_line_indexed_u32(line, "entry", i, "access", &access) ||
            !_recording_line_indexed_bool(
                line, "entry", i, "has_dynamic_offset", &e->has_dynamic_offset))
            return false;
        e->binding_type = (DvzDrp2BindingType)binding_type;
        e->access = (DvzDrp2BindingAccess)access;
    }
    return true;
}



/**
 * Decode CreateBindGroup indexed fields.
 *
 * @param line JSONL command record
 * @param command command to fill
 * @return whether the fields were decoded
 */
static bool _recording_read_create_bind_group(const char* line, DvzDrp2Command* command)
{
    ANN(line);
    ANN(command);
    command->type = DVZ_DRP2_COMMAND_CREATE_BIND_GROUP;
    if (!_recording_line_u64(line, "\"id\":", &command->u.create_bind_group.id) ||
        !_recording_line_u64(
            line, "\"bind_group_layout_id\":",
            &command->u.create_bind_group.bind_group_layout_id) ||
        !_recording_line_u32(line, "\"entry_count\":", &command->u.create_bind_group.entry_count))
        return false;
    for (uint32_t i = 0; i < DVZ_DRP2_MAX_BINDINGS; i++)
    {
        DvzDrp2BindGroupEntry* e = &command->u.create_bind_group.entries[i];
        uint32_t binding_type = 0;
        uint32_t resource_kind = 0;
        if (!_recording_line_indexed_u32(line, "entry", i, "binding", &e->binding) ||
            !_recording_line_indexed_u32(line, "entry", i, "binding_type", &binding_type) ||
            !_recording_line_indexed_u32(line, "entry", i, "resource_kind", &resource_kind) ||
            !_recording_line_indexed_u64(line, "entry", i, "resource_id", &e->resource_id) ||
            !_recording_line_indexed_u64(line, "entry", i, "offset", &e->offset) ||
            !_recording_line_indexed_u64(line, "entry", i, "size", &e->size))
            return false;
        e->binding_type = (DvzDrp2BindingType)binding_type;
        e->resource_kind = (DvzDrp2BindingResourceKind)resource_kind;
    }
    return true;
}



/**
 * Decode BeginRenderPass indexed fields.
 *
 * @param line JSONL command record
 * @param command command to fill
 * @return whether the fields were decoded
 */
static bool _recording_read_begin_render_pass(const char* line, DvzDrp2Command* command)
{
    ANN(line);
    ANN(command);
    command->type = DVZ_DRP2_COMMAND_BEGIN_RENDER_PASS;
    if (!_recording_line_u64(line, "\"id\":", &command->u.begin_render_pass.id) ||
        !_recording_line_u64(line, "\"encoder_id\":", &command->u.begin_render_pass.encoder_id) ||
        !_recording_line_u64(line, "\"texture_id\":", &command->u.begin_render_pass.texture_id) ||
        !_recording_line_u32(
            line, "\"color_attachment_count\":",
            &command->u.begin_render_pass.color_attachment_count) ||
        !_recording_line_bool(
            line, "\"has_depth_attachment\":",
            &command->u.begin_render_pass.has_depth_attachment) ||
        !_recording_line_float(
            line, "\"clear_depth\":", &command->u.begin_render_pass.clear_depth) ||
        !_recording_line_float(
            line, "\"clear_color0\":", &command->u.begin_render_pass.clear_color[0]) ||
        !_recording_line_float(
            line, "\"clear_color1\":", &command->u.begin_render_pass.clear_color[1]) ||
        !_recording_line_float(
            line, "\"clear_color2\":", &command->u.begin_render_pass.clear_color[2]) ||
        !_recording_line_float(
            line, "\"clear_color3\":", &command->u.begin_render_pass.clear_color[3]) ||
        !_recording_line_float(line, "\"viewport0\":", &command->u.begin_render_pass.viewport[0]) ||
        !_recording_line_float(line, "\"viewport1\":", &command->u.begin_render_pass.viewport[1]) ||
        !_recording_line_float(line, "\"viewport2\":", &command->u.begin_render_pass.viewport[2]) ||
        !_recording_line_float(line, "\"viewport3\":", &command->u.begin_render_pass.viewport[3]) ||
        !_recording_line_bool(line, "\"clear\":", &command->u.begin_render_pass.clear))
        return false;

    command->u.begin_render_pass.depth_load_op =
        command->u.begin_render_pass.clear ? DVZ_DRP2_ATTACHMENT_LOAD_CLEAR :
                                             DVZ_DRP2_ATTACHMENT_LOAD_LOAD;
    command->u.begin_render_pass.depth_store_op = DVZ_DRP2_ATTACHMENT_STORE_STORE;
    command->u.begin_render_pass.depth_access = DVZ_DRP2_ATTACHMENT_ACCESS_READ_WRITE;
    command->u.begin_render_pass.depth_ops_explicit = false;
    command->u.begin_render_pass.depth_texture_id = 0;
    (void)_recording_line_u64(
        line, "\"depth_texture_id\":", &command->u.begin_render_pass.depth_texture_id);
    uint32_t depth_load_op = (uint32_t)command->u.begin_render_pass.depth_load_op;
    uint32_t depth_store_op = (uint32_t)command->u.begin_render_pass.depth_store_op;
    uint32_t depth_access = (uint32_t)command->u.begin_render_pass.depth_access;
    if (_recording_line_u32(line, "\"depth_load_op\":", &depth_load_op))
        command->u.begin_render_pass.depth_load_op = (DvzDrp2AttachmentLoadOp)depth_load_op;
    if (_recording_line_u32(line, "\"depth_store_op\":", &depth_store_op))
        command->u.begin_render_pass.depth_store_op = (DvzDrp2AttachmentStoreOp)depth_store_op;
    if (_recording_line_u32(line, "\"depth_access\":", &depth_access))
        command->u.begin_render_pass.depth_access = (DvzDrp2AttachmentAccess)depth_access;
    (void)_recording_line_bool(
        line, "\"depth_ops_explicit\":", &command->u.begin_render_pass.depth_ops_explicit);

    for (uint32_t i = 0; i < DVZ_DRP2_MAX_COLOR_ATTACHMENTS; i++)
    {
        DvzDrp2ColorAttachment* a = &command->u.begin_render_pass.color_attachments[i];
        if (!_recording_line_indexed_u64(line, "ca", i, "texture_id", &a->texture_id) ||
            !_recording_line_indexed_bool(line, "ca", i, "clear", &a->clear) ||
            !_recording_line_indexed_float(line, "ca", i, "clear_color0", &a->clear_color[0]) ||
            !_recording_line_indexed_float(line, "ca", i, "clear_color1", &a->clear_color[1]) ||
            !_recording_line_indexed_float(line, "ca", i, "clear_color2", &a->clear_color[2]) ||
            !_recording_line_indexed_float(line, "ca", i, "clear_color3", &a->clear_color[3]))
            return false;
        a->load_op = a->clear ? DVZ_DRP2_ATTACHMENT_LOAD_CLEAR : DVZ_DRP2_ATTACHMENT_LOAD_LOAD;
        a->store_op = DVZ_DRP2_ATTACHMENT_STORE_STORE;
        a->access = DVZ_DRP2_ATTACHMENT_ACCESS_WRITE;
        uint32_t load_op = (uint32_t)a->load_op;
        uint32_t store_op = (uint32_t)a->store_op;
        uint32_t access = (uint32_t)a->access;
        (void)_recording_line_indexed_u64(
            line, "ca", i, "resolve_texture_id", &a->resolve_texture_id);
        (void)_recording_line_indexed_u32(line, "ca", i, "resolve_mode", &a->resolve_mode);
        if (_recording_line_indexed_u32(line, "ca", i, "load_op", &load_op))
            a->load_op = (DvzDrp2AttachmentLoadOp)load_op;
        if (_recording_line_indexed_u32(line, "ca", i, "store_op", &store_op))
            a->store_op = (DvzDrp2AttachmentStoreOp)store_op;
        if (_recording_line_indexed_u32(line, "ca", i, "access", &access))
            a->access = (DvzDrp2AttachmentAccess)access;
    }
    return true;
}



/**
 * Decode one portable JSON command record and append it to a stream.
 *
 * @param root recording root directory
 * @param line JSONL command record
 * @param op portable operation name
 * @param stream output stream
 * @param owner output stream owner
 * @return whether the command was appended
 */
static bool _recording_read_portable_command(
    const char* root, const char* line, const char* op, DvzDrp2CommandStream* stream,
    DvzDrp2RecordingOwner* owner)
{
    ANN(root);
    ANN(line);
    ANN(op);
    ANN(stream);
    ANN(owner);

    DvzDrp2Command command = {0};
    if (strcmp(op, "HelloRenderer") == 0 || strcmp(op, "RendererHelloReply") == 0)
    {
        command.type = strcmp(op, "HelloRenderer") == 0 ? DVZ_DRP2_COMMAND_HELLO_RENDERER
                                                        : DVZ_DRP2_COMMAND_RENDERER_HELLO_REPLY;
        if (!_recording_line_string(
                line, "\"name\":\"", command.u.handshake.name,
                sizeof(command.u.handshake.name)))
            return false;
    }
    else if (strcmp(op, "CreateBuffer") == 0)
    {
        command.type = DVZ_DRP2_COMMAND_CREATE_BUFFER;
        if (!_recording_line_u64(line, "\"id\":", &command.u.create_buffer.id) ||
            !_recording_line_u64(line, "\"size\":", &command.u.create_buffer.size) ||
            !_recording_line_u32(line, "\"usage\":", &command.u.create_buffer.usage))
            return false;
    }
    else if (strcmp(op, "CreateTexture") == 0)
    {
        command.type = DVZ_DRP2_COMMAND_CREATE_TEXTURE;
        if (!_recording_line_u64(line, "\"id\":", &command.u.create_texture.id) ||
            !_recording_line_u32(line, "\"width\":", &command.u.create_texture.width) ||
            !_recording_line_u32(line, "\"height\":", &command.u.create_texture.height) ||
            !_recording_line_u32(line, "\"depth\":", &command.u.create_texture.depth) ||
            !_recording_line_u32(line, "\"usage\":", &command.u.create_texture.usage))
            return false;
        (void)_recording_line_u32(line, "\"format\":", &command.u.create_texture.format);
        command.u.create_texture.sample_count = 1;
        (void)_recording_line_u32(
            line, "\"sample_count\":", &command.u.create_texture.sample_count);
    }
    else if (strcmp(op, "CreateShaderModule") == 0)
    {
        command.type = DVZ_DRP2_COMMAND_CREATE_SHADER_MODULE;
        if (!_recording_line_u64(line, "\"id\":", &command.u.create_shader_module.id) ||
            !_recording_line_string(
                line, "\"stage\":\"", command.u.create_shader_module.stage,
                sizeof(command.u.create_shader_module.stage)) ||
            !_recording_line_string(
                line, "\"format\":\"", command.u.create_shader_module.format,
                sizeof(command.u.create_shader_module.format)))
            return false;
        (void)_recording_line_string(
            line, "\"builtin_family\":\"", command.u.create_shader_module.builtin_family,
            sizeof(command.u.create_shader_module.builtin_family));
        (void)_recording_line_string(
            line, "\"builtin_variant\":\"", command.u.create_shader_module.builtin_variant,
            sizeof(command.u.create_shader_module.builtin_variant));
        (void)_recording_line_u32(
            line, "\"builtin_version\":", &command.u.create_shader_module.builtin_version);

        char payload_kind[64] = {0};
        (void)_recording_line_string(line, "\"payload_kind\":\"", payload_kind, sizeof(payload_kind));
        void* payload = NULL;
        uint64_t payload_size = 0;
        if (!_recording_read_payload_ref(root, line, &payload, &payload_size))
            return false;
        if (!_recording_attach_payload(owner, &command, payload, payload_size, payload_kind))
            return false;
    }
    else if (strcmp(op, "CreateRenderPipeline") == 0)
    {
        if (!_recording_read_create_render_pipeline(line, &command))
            return false;
    }
    else if (strcmp(op, "CreateComputePipeline") == 0)
    {
        if (!_recording_read_create_compute_pipeline(line, &command))
            return false;
    }
    else if (strcmp(op, "CreateSampler") == 0)
    {
        command.type = DVZ_DRP2_COMMAND_CREATE_SAMPLER;
        if (!_recording_line_u64(line, "\"id\":", &command.u.create_sampler.id))
            return false;
        command.u.create_sampler.mag_filter = strstr(line, "\"mag_filter\":\"nearest\"") != NULL ||
                                                     strstr(line, "\"mag_filter\": \"nearest\"") != NULL ?
                                                 DVZ_DRP2_FILTER_NEAREST :
                                                 DVZ_DRP2_FILTER_LINEAR;
        command.u.create_sampler.min_filter = strstr(line, "\"min_filter\":\"nearest\"") != NULL ||
                                                     strstr(line, "\"min_filter\": \"nearest\"") != NULL ?
                                                 DVZ_DRP2_FILTER_NEAREST :
                                                 DVZ_DRP2_FILTER_LINEAR;
    }
    else if (strcmp(op, "CreateBindGroupLayout") == 0)
    {
        if (!_recording_read_create_bind_group_layout(line, &command))
            return false;
    }
    else if (strcmp(op, "CreateBindGroup") == 0)
    {
        if (!_recording_read_create_bind_group(line, &command))
            return false;
    }
    else if (strcmp(op, "WriteBuffer") == 0)
    {
        command.type = DVZ_DRP2_COMMAND_WRITE_BUFFER;
        if (!_recording_line_u64(line, "\"buffer_id\":", &command.u.write_buffer.buffer_id) ||
            !_recording_line_u64(line, "\"offset\":", &command.u.write_buffer.offset) ||
            !_recording_line_u64(line, "\"size\":", &command.u.write_buffer.size))
            return false;

        void* payload = NULL;
        uint64_t payload_size = 0;
        if (!_recording_read_payload_ref(root, line, &payload, &payload_size))
            return false;
        if (payload_size != command.u.write_buffer.size)
        {
            dvz_free(payload);
            return false;
        }
        if (!_recording_attach_payload(owner, &command, payload, payload_size, "bytes"))
            return false;
    }
    else if (strcmp(op, "WriteTexture") == 0)
    {
        command.type = DVZ_DRP2_COMMAND_WRITE_TEXTURE;
        if (!_recording_line_u64(line, "\"texture_id\":", &command.u.write_texture.texture_id) ||
            !_recording_line_u32(line, "\"mip_level\":", &command.u.write_texture.mip_level) ||
            !_recording_line_u32(line, "\"origin_x\":", &command.u.write_texture.origin_x) ||
            !_recording_line_u32(line, "\"origin_y\":", &command.u.write_texture.origin_y) ||
            !_recording_line_u32(line, "\"origin_z\":", &command.u.write_texture.origin_z) ||
            !_recording_line_u32(line, "\"width\":", &command.u.write_texture.width) ||
            !_recording_line_u32(line, "\"height\":", &command.u.write_texture.height) ||
            !_recording_line_u32(line, "\"depth\":", &command.u.write_texture.depth) ||
            !_recording_line_u32(
                line, "\"bytes_per_row\":", &command.u.write_texture.bytes_per_row) ||
            !_recording_line_u32(
                line, "\"rows_per_image\":", &command.u.write_texture.rows_per_image))
            return false;

        uint64_t expected_size = 0;
        if (!_recording_texture_payload_size(&command, &expected_size))
            return false;
        void* payload = NULL;
        uint64_t payload_size = 0;
        if (!_recording_read_payload_ref(root, line, &payload, &payload_size))
            return false;
        if (payload_size != expected_size)
        {
            dvz_free(payload);
            return false;
        }
        if (!_recording_attach_payload(owner, &command, payload, payload_size, "bytes"))
            return false;
    }
    else if (strcmp(op, "BeginCommandEncoder") == 0)
    {
        command.type = DVZ_DRP2_COMMAND_BEGIN_COMMAND_ENCODER;
        if (!_recording_line_u64(line, "\"id\":", &command.u.begin_command_encoder.id))
            return false;
    }
    else if (strcmp(op, "BeginRenderPass") == 0)
    {
        if (!_recording_read_begin_render_pass(line, &command))
            return false;
    }
    else if (strcmp(op, "SetViewport") == 0)
    {
        command.type = DVZ_DRP2_COMMAND_SET_VIEWPORT;
        if (!_recording_line_u64(line, "\"pass_id\":", &command.u.set_viewport.pass_id) ||
            !_recording_line_float(line, "\"x\":", &command.u.set_viewport.viewport[0]) ||
            !_recording_line_float(line, "\"y\":", &command.u.set_viewport.viewport[1]) ||
            !_recording_line_float(line, "\"width\":", &command.u.set_viewport.viewport[2]) ||
            !_recording_line_float(line, "\"height\":", &command.u.set_viewport.viewport[3]))
            return false;
    }
    else if (strcmp(op, "SetScissor") == 0)
    {
        command.type = DVZ_DRP2_COMMAND_SET_SCISSOR;
        if (!_recording_line_u64(line, "\"pass_id\":", &command.u.set_scissor.pass_id) ||
            !_recording_line_float(line, "\"x\":", &command.u.set_scissor.scissor[0]) ||
            !_recording_line_float(line, "\"y\":", &command.u.set_scissor.scissor[1]) ||
            !_recording_line_float(line, "\"width\":", &command.u.set_scissor.scissor[2]) ||
            !_recording_line_float(line, "\"height\":", &command.u.set_scissor.scissor[3]))
            return false;
    }
    else if (strcmp(op, "SetPipeline") == 0)
    {
        command.type = DVZ_DRP2_COMMAND_SET_PIPELINE;
        if (!_recording_line_u64(line, "\"pass_id\":", &command.u.set_pipeline.pass_id) ||
            !_recording_line_u64(line, "\"pipeline_id\":", &command.u.set_pipeline.pipeline_id))
            return false;
    }
    else if (strcmp(op, "SetBindGroup") == 0)
    {
        command.type = DVZ_DRP2_COMMAND_SET_BIND_GROUP;
        if (!_recording_line_u64(line, "\"pass_id\":", &command.u.set_bind_group.pass_id) ||
            !_recording_line_u32(line, "\"slot\":", &command.u.set_bind_group.slot) ||
            !_recording_line_u64(
                line, "\"bind_group_id\":", &command.u.set_bind_group.bind_group_id) ||
            !_recording_line_u32(
                line, "\"dynamic_offset_count\":",
                &command.u.set_bind_group.dynamic_offset_count))
            return false;
        for (uint32_t i = 0; i < DVZ_DRP2_MAX_BINDINGS; i++)
        {
            if (!_recording_line_indexed_u64(
                    line, "dynamic_offset", i, "", &command.u.set_bind_group.dynamic_offsets[i]))
                return false;
        }
    }
    else if (strcmp(op, "SetVertexBuffer") == 0)
    {
        command.type = DVZ_DRP2_COMMAND_SET_VERTEX_BUFFER;
        if (!_recording_line_u64(line, "\"pass_id\":", &command.u.set_vertex_buffer.pass_id) ||
            !_recording_line_u32(line, "\"slot\":", &command.u.set_vertex_buffer.slot) ||
            !_recording_line_u64(line, "\"buffer_id\":", &command.u.set_vertex_buffer.buffer_id) ||
            !_recording_line_u64(line, "\"offset\":", &command.u.set_vertex_buffer.offset))
            return false;
    }
    else if (strcmp(op, "SetIndexBuffer") == 0)
    {
        command.type = DVZ_DRP2_COMMAND_SET_INDEX_BUFFER;
        if (!_recording_line_u64(line, "\"pass_id\":", &command.u.set_index_buffer.pass_id) ||
            !_recording_line_u64(line, "\"buffer_id\":", &command.u.set_index_buffer.buffer_id) ||
            !_recording_line_string(
                line, "\"index_format\":\"", command.u.set_index_buffer.index_format,
                sizeof(command.u.set_index_buffer.index_format)) ||
            !_recording_line_u64(line, "\"offset\":", &command.u.set_index_buffer.offset))
            return false;
    }
    else if (strcmp(op, "Draw") == 0)
    {
        command.type = DVZ_DRP2_COMMAND_DRAW;
        if (!_recording_line_u64(line, "\"pass_id\":", &command.u.draw.pass_id) ||
            !_recording_line_u32(line, "\"vertex_count\":", &command.u.draw.vertex_count) ||
            !_recording_line_u32(line, "\"instance_count\":", &command.u.draw.instance_count) ||
            !_recording_line_u32(line, "\"first_vertex\":", &command.u.draw.first_vertex) ||
            !_recording_line_u32(line, "\"first_instance\":", &command.u.draw.first_instance))
            return false;
    }
    else if (strcmp(op, "DrawIndexed") == 0)
    {
        command.type = DVZ_DRP2_COMMAND_DRAW_INDEXED;
        if (!_recording_line_u64(line, "\"pass_id\":", &command.u.draw_indexed.pass_id) ||
            !_recording_line_u32(line, "\"index_count\":", &command.u.draw_indexed.index_count) ||
            !_recording_line_u32(
                line, "\"instance_count\":", &command.u.draw_indexed.instance_count) ||
            !_recording_line_u32(line, "\"first_index\":", &command.u.draw_indexed.first_index) ||
            !_recording_line_i32(line, "\"base_vertex\":", &command.u.draw_indexed.base_vertex) ||
            !_recording_line_u32(
                line, "\"first_instance\":", &command.u.draw_indexed.first_instance))
            return false;
    }
    else if (strcmp(op, "EndRenderPass") == 0)
    {
        command.type = DVZ_DRP2_COMMAND_END_RENDER_PASS;
        if (!_recording_line_u64(line, "\"pass_id\":", &command.u.end_render_pass.pass_id))
            return false;
    }
    else if (strcmp(op, "BeginComputePass") == 0)
    {
        command.type = DVZ_DRP2_COMMAND_BEGIN_COMPUTE_PASS;
        if (!_recording_line_u64(line, "\"id\":", &command.u.begin_compute_pass.id) ||
            !_recording_line_u64(line, "\"encoder_id\":", &command.u.begin_compute_pass.encoder_id))
            return false;
    }
    else if (strcmp(op, "DispatchWorkgroups") == 0)
    {
        command.type = DVZ_DRP2_COMMAND_DISPATCH_WORKGROUPS;
        if (!_recording_line_u64(line, "\"pass_id\":", &command.u.dispatch.pass_id) ||
            !_recording_line_u32(line, "\"x\":", &command.u.dispatch.x) ||
            !_recording_line_u32(line, "\"y\":", &command.u.dispatch.y) ||
            !_recording_line_u32(line, "\"z\":", &command.u.dispatch.z))
            return false;
    }
    else if (strcmp(op, "EndComputePass") == 0)
    {
        command.type = DVZ_DRP2_COMMAND_END_COMPUTE_PASS;
        if (!_recording_line_u64(line, "\"pass_id\":", &command.u.end_compute_pass.pass_id))
            return false;
    }
    else if (strcmp(op, "CopyBufferToBuffer") == 0)
    {
        command.type = DVZ_DRP2_COMMAND_COPY_BUFFER_TO_BUFFER;
        if (!_recording_line_u64(
                line, "\"encoder_id\":", &command.u.copy_buffer_to_buffer.encoder_id) ||
            !_recording_line_u64(
                line, "\"src_buffer_id\":", &command.u.copy_buffer_to_buffer.src_buffer_id) ||
            !_recording_line_u64(
                line, "\"src_offset\":", &command.u.copy_buffer_to_buffer.src_offset) ||
            !_recording_line_u64(
                line, "\"dst_buffer_id\":", &command.u.copy_buffer_to_buffer.dst_buffer_id) ||
            !_recording_line_u64(
                line, "\"dst_offset\":", &command.u.copy_buffer_to_buffer.dst_offset) ||
            !_recording_line_u64(line, "\"size\":", &command.u.copy_buffer_to_buffer.size))
            return false;
    }
    else if (strcmp(op, "CopyBufferToTexture") == 0)
    {
        command.type = DVZ_DRP2_COMMAND_COPY_BUFFER_TO_TEXTURE;
        if (!_recording_line_u64(
                line, "\"encoder_id\":", &command.u.copy_buffer_to_texture.encoder_id) ||
            !_recording_line_u64(
                line, "\"src_buffer_id\":", &command.u.copy_buffer_to_texture.src_buffer_id) ||
            !_recording_line_u64(
                line, "\"src_offset\":", &command.u.copy_buffer_to_texture.src_offset) ||
            !_recording_line_u32(
                line, "\"bytes_per_row\":", &command.u.copy_buffer_to_texture.bytes_per_row) ||
            !_recording_line_u32(
                line, "\"rows_per_image\":", &command.u.copy_buffer_to_texture.rows_per_image) ||
            !_recording_line_u64(
                line, "\"dst_texture_id\":", &command.u.copy_buffer_to_texture.dst_texture_id) ||
            !_recording_line_u32(
                line, "\"dst_mip_level\":", &command.u.copy_buffer_to_texture.dst_mip_level) ||
            !_recording_line_u32(
                line, "\"dst_origin_x\":", &command.u.copy_buffer_to_texture.dst_origin_x) ||
            !_recording_line_u32(
                line, "\"dst_origin_y\":", &command.u.copy_buffer_to_texture.dst_origin_y) ||
            !_recording_line_u32(
                line, "\"dst_origin_z\":", &command.u.copy_buffer_to_texture.dst_origin_z) ||
            !_recording_line_u32(line, "\"width\":", &command.u.copy_buffer_to_texture.width) ||
            !_recording_line_u32(line, "\"height\":", &command.u.copy_buffer_to_texture.height) ||
            !_recording_line_u32(line, "\"depth\":", &command.u.copy_buffer_to_texture.depth))
            return false;
    }
    else if (strcmp(op, "CopyTextureToBuffer") == 0)
    {
        command.type = DVZ_DRP2_COMMAND_COPY_TEXTURE_TO_BUFFER;
        if (!_recording_line_u64(
                line, "\"encoder_id\":", &command.u.copy_texture_to_buffer.encoder_id) ||
            !_recording_line_u64(
                line, "\"src_texture_id\":", &command.u.copy_texture_to_buffer.src_texture_id) ||
            !_recording_line_u64(
                line, "\"dst_buffer_id\":", &command.u.copy_texture_to_buffer.dst_buffer_id) ||
            !_recording_line_u64(
                line, "\"dst_offset\":", &command.u.copy_texture_to_buffer.dst_offset) ||
            !_recording_line_u32(line, "\"width\":", &command.u.copy_texture_to_buffer.width) ||
            !_recording_line_u32(line, "\"height\":", &command.u.copy_texture_to_buffer.height) ||
            !_recording_line_u32(
                line, "\"bytes_per_row\":", &command.u.copy_texture_to_buffer.bytes_per_row) ||
            !_recording_line_u32(
                line, "\"rows_per_image\":", &command.u.copy_texture_to_buffer.rows_per_image))
            return false;
    }
    else if (strcmp(op, "CopyTextureToTexture") == 0)
    {
        command.type = DVZ_DRP2_COMMAND_COPY_TEXTURE_TO_TEXTURE;
        if (!_recording_line_u64(
                line, "\"encoder_id\":", &command.u.copy_texture_to_texture.encoder_id) ||
            !_recording_line_u64(
                line, "\"src_texture_id\":", &command.u.copy_texture_to_texture.src_texture_id) ||
            !_recording_line_u32(
                line, "\"src_mip_level\":", &command.u.copy_texture_to_texture.src_mip_level) ||
            !_recording_line_u32(
                line, "\"src_origin_x\":", &command.u.copy_texture_to_texture.src_origin_x) ||
            !_recording_line_u32(
                line, "\"src_origin_y\":", &command.u.copy_texture_to_texture.src_origin_y) ||
            !_recording_line_u32(
                line, "\"src_origin_z\":", &command.u.copy_texture_to_texture.src_origin_z) ||
            !_recording_line_u64(
                line, "\"dst_texture_id\":", &command.u.copy_texture_to_texture.dst_texture_id) ||
            !_recording_line_u32(
                line, "\"dst_mip_level\":", &command.u.copy_texture_to_texture.dst_mip_level) ||
            !_recording_line_u32(
                line, "\"dst_origin_x\":", &command.u.copy_texture_to_texture.dst_origin_x) ||
            !_recording_line_u32(
                line, "\"dst_origin_y\":", &command.u.copy_texture_to_texture.dst_origin_y) ||
            !_recording_line_u32(
                line, "\"dst_origin_z\":", &command.u.copy_texture_to_texture.dst_origin_z) ||
            !_recording_line_u32(line, "\"width\":", &command.u.copy_texture_to_texture.width) ||
            !_recording_line_u32(line, "\"height\":", &command.u.copy_texture_to_texture.height) ||
            !_recording_line_u32(line, "\"depth\":", &command.u.copy_texture_to_texture.depth))
            return false;
    }
    else if (strcmp(op, "FinishCommandEncoder") == 0)
    {
        command.type = DVZ_DRP2_COMMAND_FINISH_COMMAND_ENCODER;
        if (!_recording_line_u64(
                line, "\"encoder_id\":", &command.u.finish_command_encoder.encoder_id) ||
            !_recording_line_u64(
                line, "\"command_buffer_id\":",
                &command.u.finish_command_encoder.command_buffer_id))
            return false;
    }
    else if (strcmp(op, "QueueSubmit") == 0)
    {
        command.type = DVZ_DRP2_COMMAND_QUEUE_SUBMIT;
        if (!_recording_line_u64(
                line, "\"command_buffer_id\":", &command.u.queue_submit.command_buffer_id) ||
            !_recording_line_u64(line, "\"submission_id\":", &command.u.queue_submit.submission_id) ||
            !_recording_line_bool(line, "\"has_readback\":", &command.u.queue_submit.has_readback) ||
            !_recording_line_u64(line, "\"buffer_id\":", &command.u.queue_submit.buffer_id) ||
            !_recording_line_u64(line, "\"offset\":", &command.u.queue_submit.offset) ||
            !_recording_line_u64(line, "\"size\":", &command.u.queue_submit.size))
            return false;
    }
    else
    {
        return false;
    }

    return _recording_stream_append(stream, &command);
}



/**
 * Decode one command record and append it to a stream.
 *
 * @param root recording root directory
 * @param line JSONL command record
 * @param stream output stream
 * @param owner output stream owner
 * @return whether the command was appended
 */
static bool _recording_read_command(
    const char* root, const char* line, DvzDrp2CommandStream* stream,
    DvzDrp2RecordingOwner* owner)
{
    ANN(root);
    ANN(line);
    ANN(stream);
    ANN(owner);

    char op[64] = {0};
    if (_recording_line_string(line, "\"op\":\"", op, sizeof(op)))
        return _recording_read_portable_command(root, line, op, stream, owner);

    char command_rel[128] = {0};
    char command_path[DVZ_DRP2_RECORDING_PATH_SIZE] = {0};
    uint64_t command_size = 0;
    if (!_recording_line_string(line, "\"command_blob\":\"", command_rel, sizeof(command_rel)) ||
        !_recording_line_u64(line, "\"command_size\":", &command_size) ||
        command_size != sizeof(DvzDrp2Command) ||
        !_recording_join(root, command_rel, command_path, sizeof(command_path)))
        return false;

    DvzDrp2Command* command =
        (DvzDrp2Command*)_recording_read_blob(command_path, sizeof(DvzDrp2Command));
    if (command == NULL)
        return false;

    char payload_rel[128] = {0};
    char payload_path[DVZ_DRP2_RECORDING_PATH_SIZE] = {0};
    char payload_kind[64] = {0};
    uint64_t payload_size = 0;
    if (_recording_line_string(line, "\"payload_blob\":\"", payload_rel, sizeof(payload_rel)))
    {
        if (!_recording_line_u64(line, "\"payload_size\":", &payload_size) ||
            !_recording_join(root, payload_rel, payload_path, sizeof(payload_path)))
        {
            dvz_free(command);
            return false;
        }
        _recording_line_string(line, "\"payload_kind\":\"", payload_kind, sizeof(payload_kind));
        void* payload = _recording_read_blob(payload_path, payload_size);
        if (!_recording_attach_payload(owner, command, payload, payload_size, payload_kind))
        {
            dvz_free(command);
            return false;
        }
    }

    bool ok = _recording_stream_append(stream, command);
    if (!ok && command->type == DVZ_DRP2_COMMAND_CREATE_SHADER_MODULE &&
        command->u.create_shader_module.code != NULL)
    {
        dvz_free(command->u.create_shader_module.code);
    }
    dvz_free(command);
    return ok;
}



/**
 * Decode one frame record and append it to a loaded recording.
 *
 * @param line JSONL frame record
 * @param recording loaded recording
 * @return whether the frame was appended
 */
static bool _recording_read_frame(const char* line, DvzDrp2Recording* recording)
{
    ANN(line);
    ANN(recording);
    DvzDrp2RecordedFrame frame = {0};
    uint64_t first_command = 0;
    if (!_recording_line_double(line, "\"t_present\":", &frame.t_present) ||
        !_recording_line_u64(line, "\"first_command\":", &first_command) ||
        !_recording_line_u32(line, "\"command_count\":", &frame.command_count) ||
        first_command > UINT32_MAX)
        return false;
    frame.first_command = (uint32_t)first_command;
    if (frame.first_command > recording->stream->count ||
        frame.command_count > recording->stream->count - frame.first_command)
        return false;
    return _recording_frame_append(recording, &frame);
}



/**
 * Write the recording manifest.
 *
 * @param path recording directory path
 * @param info optional recording metadata
 * @return whether the manifest was written
 */
static bool _recording_write_manifest(const char* path, const DvzDrp2RecordingInfo* info)
{
    ANN(path);
    char manifest_path[DVZ_DRP2_RECORDING_PATH_SIZE] = {0};
    if (!_recording_join(path, "manifest.json", manifest_path, sizeof(manifest_path)))
        return false;
    FILE* manifest = fopen(manifest_path, "wb");
    if (manifest == NULL)
        return false;
    uint32_t width = info != NULL ? info->width : 0;
    uint32_t height = info != NULL ? info->height : 0;
    double duration_s = info != NULL ? info->duration_s : 0.0;
    double fps_cap = info != NULL ? info->fps_cap : 0.0;
    const char* backend_hint = info != NULL && info->backend_hint != NULL ? info->backend_hint : "";
    if (!_recording_json_string_safe(backend_hint))
        backend_hint = "";

    char created_at[32] = {0};
    char git_commit[64] = {0};
    char git_dirty[8] = {0};
    char os_name[128] = {0};
    char arch[128] = {0};
    _recording_created_at(created_at, sizeof(created_at));
    (void)_recording_command_first_line(
        "git rev-parse --short=12 HEAD 2>/dev/null", git_commit, sizeof(git_commit));
    (void)_recording_command_first_line(
        "git diff --quiet 2>/dev/null && git diff --cached --quiet 2>/dev/null && echo false || "
        "echo true",
        git_dirty, sizeof(git_dirty));
#if defined(_WIN32)
    dvz_snprintf(os_name, sizeof(os_name), "Windows");
#if defined(_M_X64) || defined(_M_AMD64)
    dvz_snprintf(arch, sizeof(arch), "x86_64");
#elif defined(_M_ARM64)
    dvz_snprintf(arch, sizeof(arch), "arm64");
#else
    dvz_snprintf(arch, sizeof(arch), "unknown");
#endif
#else
    struct utsname uts = {0};
    if (uname(&uts) == 0)
    {
        dvz_snprintf(os_name, sizeof(os_name), "%s %s", uts.sysname, uts.release);
        dvz_snprintf(arch, sizeof(arch), "%s", uts.machine);
    }
#endif
#if defined(__clang__)
    const char* compiler = "clang " __clang_version__;
#elif defined(__GNUC__)
    const char* compiler = "gcc " __VERSION__;
#elif defined(_MSC_VER)
    const char* compiler = "msvc";
#else
    const char* compiler = "unknown";
#endif
    const char* version = dvz_version();
    if (version == NULL || !_recording_json_string_safe(version))
        version = "";
    if (!_recording_json_string_safe(os_name))
        os_name[0] = '\0';
    if (!_recording_json_string_safe(arch))
        arch[0] = '\0';
    if (!_recording_json_string_safe(compiler))
        compiler = "";

    bool ok = dvz_fprintf(
                  manifest,
                  "{\n"
                  "  \"format\": \"datoviz-drp-recording\",\n"
                  "  \"version\": 1,\n"
                  "  \"encoding\": \"jsonl-command-v0-with-raw-fallback\",\n"
                  "  \"drp_version\": \"2.0\",\n"
                  "  \"width\": %" PRIu32 ",\n"
                  "  \"height\": %" PRIu32 ",\n"
                  "  \"duration_s\": %.17g,\n"
                  "  \"backend_hint\": \"%s\",\n"
                  "  \"recording\": {\n"
                  "    \"created_at\": \"%s\",\n"
                  "    \"fps_cap\": %.17g\n"
                  "  },\n"
                  "  \"datoviz\": {\n"
                  "    \"version\": \"%s\",\n"
                  "    \"git_commit\": \"%s\",\n"
                  "    \"git_dirty\": %s\n"
                  "  },\n"
                  "  \"system\": {\n"
                  "    \"os\": \"%s\",\n"
                  "    \"arch\": \"%s\",\n"
                  "    \"compiler\": \"%s\"\n"
                  "  }\n"
                  "}\n",
                  width, height, duration_s, backend_hint, created_at, fps_cap, version,
                  git_commit, strcmp(git_dirty, "true") == 0 ? "true" : "false", os_name, arch,
                  compiler) > 0;
    fclose(manifest);
    return ok;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzDrp2RecordingInfo dvz_drp2_recording_info(void)
{
    return (DvzDrp2RecordingInfo){DVZ_STRUCT_INIT_FIELDS(DvzDrp2RecordingInfo)};
}



/**
 * Open a linear DRP2 recorder.
 *
 * @param path recording directory path
 * @param info optional recording metadata
 * @return the recorder, or NULL on error
 */
DvzDrp2Recorder* dvz_drp2_recorder_open(
    const char* path, const DvzDrp2RecordingInfo* info)
{
    if (path == NULL)
        return NULL;
    if (!_recording_info_validate(info))
        return NULL;
    if (!_recording_mkdir(path))
        return NULL;
    char blobs_path[DVZ_DRP2_RECORDING_PATH_SIZE] = {0};
    if (!_recording_join(path, "blobs", blobs_path, sizeof(blobs_path)) ||
        !_recording_mkdir(blobs_path))
        return NULL;
    if (!_recording_write_manifest(path, info))
        return NULL;

    char stream_path[DVZ_DRP2_RECORDING_PATH_SIZE] = {0};
    if (!_recording_join(path, "stream.jsonl", stream_path, sizeof(stream_path)))
        return NULL;
    FILE* stream_fp = fopen(stream_path, "wb");
    if (stream_fp == NULL)
        return NULL;

    DvzDrp2Recorder* recorder = (DvzDrp2Recorder*)dvz_calloc(1, sizeof(DvzDrp2Recorder));
    if (recorder == NULL)
    {
        fclose(stream_fp);
        return NULL;
    }
    dvz_strlcpy(recorder->path, path, sizeof(recorder->path));
    if (info != NULL)
        recorder->info = *info;
    recorder->stream_fp = stream_fp;
    bool ok = dvz_fprintf(
                  stream_fp,
                  "{\"type\":\"begin\",\"version\":1,\"drp_version\":\"2.0\","
                  "\"command_count\":0}\n") > 0;
    if (!ok)
    {
        dvz_drp2_recorder_close(recorder);
        return NULL;
    }
    return recorder;
}



/**
 * Append one timestamped command stream to a linear DRP2 recorder.
 *
 * @param recorder the recorder
 * @param t_present presentation timestamp for this stream
 * @param stream the command stream to append
 * @return whether the stream was appended
 */
bool dvz_drp2_recorder_write_stream(
    DvzDrp2Recorder* recorder, double t_present, const DvzDrp2CommandStream* stream)
{
    if (recorder == NULL || recorder->stream_fp == NULL || recorder->closed || stream == NULL)
        return false;
    bool ok = true;
    uint64_t start_index = recorder->command_count;
    for (uint32_t i = 0; ok && i < stream->count; i++)
    {
        if (recorder->command_count > UINT32_MAX)
            return false;
        ok = _recording_write_command(
            recorder->path, recorder->stream_fp, &stream->commands[i],
            (uint32_t)recorder->command_count, &recorder->blob_index);
        if (ok)
            recorder->command_count++;
    }
    if (!ok)
        return false;
    if (t_present > recorder->info.duration_s)
        recorder->info.duration_s = t_present;
    recorder->info.t_present = t_present;
    return dvz_fprintf(
               recorder->stream_fp,
               "{\"type\":\"frame\",\"t_present\":%.17g,\"first_command\":%" PRIu64
               ",\"command_count\":%" PRIu32 "}\n",
               t_present, start_index, stream->count) > 0;
}



/**
 * Close a linear DRP2 recorder.
 *
 * @param recorder the recorder
 * @return whether the recorder was closed cleanly
 */
bool dvz_drp2_recorder_close(DvzDrp2Recorder* recorder)
{
    if (recorder == NULL)
        return false;
    bool ok = true;
    if (!recorder->closed && recorder->stream_fp != NULL)
        ok = dvz_fprintf(recorder->stream_fp, "{\"type\":\"end\"}\n") > 0;
    if (recorder->stream_fp != NULL)
        fclose(recorder->stream_fp);
    ok = _recording_write_manifest(recorder->path, &recorder->info) && ok;
    recorder->stream_fp = NULL;
    recorder->closed = true;
    dvz_free(recorder);
    return ok;
}



/**
 * Write a linear DRP2 recording directory.
 *
 * @param path recording directory path
 * @param stream the command stream to record
 * @param info optional recording metadata
 * @return whether the recording was written
 */
bool dvz_drp2_recording_write_stream(
    const char* path, const DvzDrp2CommandStream* stream, const DvzDrp2RecordingInfo* info)
{
    if (path == NULL || stream == NULL)
        return false;
    if (!_recording_info_validate(info))
        return false;
    DvzDrp2Recorder* recorder = dvz_drp2_recorder_open(path, info);
    if (recorder == NULL)
        return false;
    double t_present = info != NULL ? info->t_present : 0.0;
    bool ok = dvz_drp2_recorder_write_stream(recorder, t_present, stream);
    return dvz_drp2_recorder_close(recorder) && ok;
}



/**
 * Open a linear DRP2 recording directory for indexed playback.
 *
 * @param path recording directory path
 * @return the loaded recording, or NULL on error
 */
DvzDrp2Recording* dvz_drp2_recording_open(const char* path)
{
    if (path == NULL)
        return NULL;
    char stream_path[DVZ_DRP2_RECORDING_PATH_SIZE] = {0};
    if (!_recording_join(path, "stream.jsonl", stream_path, sizeof(stream_path)))
        return NULL;
    FILE* fp = fopen(stream_path, "rb");
    if (fp == NULL)
        return NULL;

    DvzDrp2Recording* recording = (DvzDrp2Recording*)dvz_calloc(1, sizeof(DvzDrp2Recording));
    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    DvzDrp2RecordingOwner* owner =
        (DvzDrp2RecordingOwner*)dvz_calloc(1, sizeof(DvzDrp2RecordingOwner));
    if (recording == NULL || stream == NULL || owner == NULL)
    {
        fclose(fp);
        dvz_drp2_stream_destroy(stream);
        dvz_free(owner);
        dvz_free(recording);
        return NULL;
    }
    recording->stream = stream;
    stream->owner = owner;
    stream->owner_release = _recording_owner_release;

    bool ok = true;
    char line[DVZ_DRP2_RECORDING_LINE_SIZE] = {0};
    while (fgets(line, sizeof(line), fp) != NULL)
    {
        if (strstr(line, "\"type\":\"command\"") != NULL)
        {
            bool raw_fallback = strstr(line, "\"command_blob\":\"") != NULL;
            DvzDrp2RawFallback fallback = {0};
            uint64_t command_index = 0;
            uint32_t command_type = 0;
            if (raw_fallback)
            {
                if (!_recording_line_u64(line, "\"index\":", &command_index) ||
                    !_recording_line_u32(line, "\"cmd_type\":", &command_type) ||
                    command_index > UINT32_MAX)
                {
                    ok = false;
                    break;
                }
                fallback.command_index = (uint32_t)command_index;
                fallback.command_type = (DvzDrp2CommandType)command_type;
            }
            ok = _recording_read_command(path, line, stream, owner);
            if (ok && raw_fallback)
                ok = _recording_raw_fallback_append(recording, &fallback);
        }
        else if (strstr(line, "\"type\":\"frame\"") != NULL)
            ok = _recording_read_frame(line, recording);
        if (!ok)
        {
            break;
        }
    }
    fclose(fp);
    if (!ok)
    {
        dvz_drp2_recording_close(recording);
        return NULL;
    }
    return recording;
}



/**
 * Close a loaded DRP2 recording.
 *
 * @param recording loaded recording
 */
void dvz_drp2_recording_close(DvzDrp2Recording* recording)
{
    if (recording == NULL)
        return;
    dvz_drp2_stream_destroy(recording->stream);
    dvz_free(recording->frames);
    dvz_free(recording->raw_fallbacks);
    dvz_free(recording);
}



/**
 * Return the full reconstructed command stream owned by a loaded recording.
 *
 * @param recording loaded recording
 * @return the full command stream, valid until the recording is closed
 */
const DvzDrp2CommandStream* dvz_drp2_recording_stream(const DvzDrp2Recording* recording)
{
    if (recording == NULL)
        return NULL;
    return recording->stream;
}



/**
 * Return the number of frame records in a loaded recording.
 *
 * @param recording loaded recording
 * @return the frame count
 */
uint32_t dvz_drp2_recording_frame_count(const DvzDrp2Recording* recording)
{
    return recording != NULL ? recording->frame_count : 0;
}



/**
 * Return one frame record from a loaded recording.
 *
 * @param recording loaded recording
 * @param frame_index frame index
 * @return the frame record, valid until the recording is closed, or NULL
 */
const DvzDrp2RecordedFrame* dvz_drp2_recording_frame(
    const DvzDrp2Recording* recording, uint32_t frame_index)
{
    if (recording == NULL || frame_index >= recording->frame_count)
        return NULL;
    return &recording->frames[frame_index];
}



/**
 * Return the number of raw fallback records in a loaded recording.
 *
 * @param recording loaded recording
 * @return raw fallback count
 */
uint32_t dvz_drp2_recording_raw_fallback_count(const DvzDrp2Recording* recording)
{
    return recording != NULL ? recording->raw_fallback_count : 0;
}



/**
 * Return one raw fallback record.
 *
 * @param recording loaded recording
 * @param fallback_index raw fallback index
 * @return raw fallback record, or NULL
 */
const DvzDrp2RawFallback* dvz_drp2_recording_raw_fallback(
    const DvzDrp2Recording* recording, uint32_t fallback_index)
{
    if (recording == NULL || fallback_index >= recording->raw_fallback_count)
        return NULL;
    return &recording->raw_fallbacks[fallback_index];
}



/**
 * Return a newly allocated command stream for one recorded frame.
 *
 * @param recording loaded recording
 * @param frame_index frame index
 * @return a newly allocated frame command stream, or NULL
 */
DvzDrp2CommandStream* dvz_drp2_recording_frame_stream(
    const DvzDrp2Recording* recording, uint32_t frame_index)
{
    const DvzDrp2RecordedFrame* frame = dvz_drp2_recording_frame(recording, frame_index);
    if (frame == NULL || recording->stream == NULL)
        return NULL;
    DvzDrp2CommandStream* stream = dvz_drp2_stream();
    DvzDrp2RecordingOwner* owner =
        (DvzDrp2RecordingOwner*)dvz_calloc(1, sizeof(DvzDrp2RecordingOwner));
    if (stream == NULL || owner == NULL)
    {
        dvz_drp2_stream_destroy(stream);
        dvz_free(owner);
        return NULL;
    }
    stream->owner = owner;
    stream->owner_release = _recording_owner_release;

    for (uint32_t i = 0; i < frame->command_count; i++)
    {
        const DvzDrp2Command* source =
            &recording->stream->commands[frame->first_command + i];
        DvzDrp2Command command = *source;
        if (!_recording_command_copy_payloads(owner, &command, source) ||
            !_recording_stream_append(stream, &command))
        {
            _recording_command_release_direct(&command);
            dvz_drp2_stream_destroy(stream);
            return NULL;
        }
    }
    return stream;
}



/**
 * Execute one recorded frame against an existing DRP2 runtime.
 *
 * @param recording loaded recording
 * @param runtime the runtime
 * @param frame_index frame index
 * @return the validation result after frame execution
 */
DvzDrp2ValidationResult dvz_drp2_recording_execute_frame(
    const DvzDrp2Recording* recording, DvzDrp2Runtime* runtime, uint32_t frame_index)
{
    if (recording == NULL || runtime == NULL)
        return _recording_result(false, DVZ_DRP2_VALIDATION_INVALID_ARGUMENT, 0);
    const DvzDrp2RecordedFrame* frame = dvz_drp2_recording_frame(recording, frame_index);
    if (frame == NULL)
        return _recording_result(false, DVZ_DRP2_VALIDATION_INVALID_ARGUMENT, 0);
    DvzDrp2CommandStream* stream = dvz_drp2_recording_frame_stream(recording, frame_index);
    if (stream == NULL)
        return _recording_result(
            false, DVZ_DRP2_VALIDATION_INVALID_STATE, frame->first_command);
    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, stream);
    _recording_result_offset(&result, frame->first_command);
    dvz_drp2_stream_destroy(stream);
    return result;
}



/**
 * Execute all recorded frames in order against an existing DRP2 runtime.
 *
 * @param recording loaded recording
 * @param runtime the runtime
 * @return the first failing validation result, or OK after all frames execute
 */
DvzDrp2ValidationResult
dvz_drp2_recording_execute_all(const DvzDrp2Recording* recording, DvzDrp2Runtime* runtime)
{
    if (recording == NULL || runtime == NULL)
        return _recording_result(false, DVZ_DRP2_VALIDATION_INVALID_ARGUMENT, 0);
    for (uint32_t i = 0; i < recording->frame_count; i++)
    {
        DvzDrp2ValidationResult result =
            dvz_drp2_recording_execute_frame(recording, runtime, i);
        if (!result.ok)
            return result;
    }
    return _recording_result(true, DVZ_DRP2_VALIDATION_OK, UINT32_MAX);
}



/**
 * Play a recording frame by frame, optionally pacing execution by recorded timestamps.
 *
 * @param recording loaded recording
 * @param runtime the runtime
 * @param paced whether to wait for each frame timestamp before execution
 * @return the first failing validation result, or OK after playback completes
 */
DvzDrp2ValidationResult dvz_drp2_recording_playback(
    const DvzDrp2Recording* recording, DvzDrp2Runtime* runtime, bool paced)
{
    if (recording == NULL || runtime == NULL)
        return _recording_result(false, DVZ_DRP2_VALIDATION_INVALID_ARGUMENT, 0);

    DvzClock clock = dvz_clock();
    for (uint32_t i = 0; i < recording->frame_count; i++)
    {
        const DvzDrp2RecordedFrame* frame = &recording->frames[i];
        if (paced && frame->t_present > 0)
        {
            double now = dvz_clock_get(&clock);
            double delay = frame->t_present - now;
            if (delay > 0)
            {
                double delay_us = delay * 1000000.0;
                int sleep_us = delay_us > (double)INT32_MAX ? INT32_MAX : (int)delay_us;
                dvz_sleep_us(sleep_us);
            }
        }

        DvzDrp2ValidationResult result =
            dvz_drp2_recording_execute_frame(recording, runtime, i);
        if (!result.ok)
            return result;
    }
    return _recording_result(true, DVZ_DRP2_VALIDATION_OK, UINT32_MAX);
}



/**
 * Read a linear DRP2 recording directory.
 *
 * @param path recording directory path
 * @return a reconstructed command stream, or NULL on error
 */
DvzDrp2CommandStream* dvz_drp2_recording_read_stream(const char* path)
{
    DvzDrp2Recording* recording = dvz_drp2_recording_open(path);
    if (recording == NULL)
        return NULL;
    DvzDrp2CommandStream* stream = recording->stream;
    recording->stream = NULL;
    dvz_drp2_recording_close(recording);
    return stream;
}
