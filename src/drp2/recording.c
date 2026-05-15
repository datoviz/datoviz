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
#include <string.h>

#if defined(_WIN32)
#include <direct.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

#include "_alloc.h"
#include "_assertions.h"
#include "_base64.h"
#include "_compat.h"
#include "_log.h"
#include "_overflow.h"
#include "_stream.h"
#include "datoviz/drp2/recording.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_DRP2_RECORDING_PATH_SIZE 4096
#define DVZ_DRP2_RECORDING_LINE_SIZE 4096



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
};


struct DvzDrp2Recorder
{
    char path[DVZ_DRP2_RECORDING_PATH_SIZE];
    FILE* stream_fp;
    uint32_t blob_index;
    uint64_t command_count;
    bool closed;
};



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

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
                   ",\"height\":%" PRIu32 ",\"depth\":%" PRIu32 ",\"usage\":%" PRIu32 "}\n",
                   index, (int)command->type, command->u.create_texture.id,
                   command->u.create_texture.width, command->u.create_texture.height,
                   command->u.create_texture.depth, command->u.create_texture.usage) > 0;
    case DVZ_DRP2_COMMAND_WRITE_BUFFER:
    {
        char payload_rel[128] = {0};
        uint64_t payload_size = command->u.write_buffer.size;
        if (payload_size > 0 &&
            !_recording_write_payload_ref(
                root, blob_index, command->u.write_buffer.data_raw,
                command->u.write_buffer.data_base64, payload_size, payload_rel,
                sizeof(payload_rel)))
            return false;
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
        if (payload_size > 0 &&
            !_recording_write_payload_ref(
                root, blob_index, command->u.write_texture.data_raw,
                command->u.write_texture.data_base64, payload_size, payload_rel,
                sizeof(payload_rel)))
            return false;
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
    return sscanf(p, "%" SCNu64, out) == 1;
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
    return sscanf(p, "%lf", out) == 1;
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
        if (_recording_owner_add(owner, payload))
            return true;
        command->u.create_shader_module.spirv = NULL;
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

    char payload_rel[128] = {0};
    char payload_path[DVZ_DRP2_RECORDING_PATH_SIZE] = {0};
    if (!_recording_line_string(line, "\"payload_blob\":\"", payload_rel, sizeof(payload_rel)) ||
        !_recording_join(root, payload_rel, payload_path, sizeof(payload_path)))
        return false;
    *out_payload = _recording_read_blob(payload_path, *out_payload_size);
    return *out_payload != NULL;
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
    const char* backend_hint = info != NULL && info->backend_hint != NULL ? info->backend_hint : "";
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
                  "  \"backend_hint\": \"%s\"\n"
                  "}\n",
                  width, height, duration_s, backend_hint) > 0;
    fclose(manifest);
    return ok;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

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
            ok = _recording_read_command(path, line, stream, owner);
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
