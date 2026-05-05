/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Minimal JSON string builder — shared internal header                                         */
/*************************************************************************************************/

#pragma once

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

#include "_alloc.h"
#include "_compat.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct JsonBuilder JsonBuilder;

struct JsonBuilder
{
    char*    data;
    uint64_t count;
    uint64_t capacity;
    bool     failed;
};



/*************************************************************************************************/
/*  Base64 encoder                                                                               */
/*************************************************************************************************/

static const char _B64_CHARS[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/* Returns the number of base64 characters needed to encode `size` bytes (excluding NUL). */
static inline uint64_t _base64_encoded_len(uint64_t size)
{
    return ((size + 2) / 3) * 4;
}

/* Encode `size` bytes from `data` into `out` (must have room for `_base64_encoded_len(size)+1`).
 * Returns false if out_size is too small. */
static inline bool
_encode_base64(const uint8_t* data, uint64_t size, char* out, uint64_t out_size)
{
    if (data == NULL || out == NULL)
        return false;
    uint64_t needed = _base64_encoded_len(size) + 1;
    if (needed > out_size)
        return false;

    uint64_t i = 0, j = 0;
    for (; i + 2 < size; i += 3)
    {
        out[j++] = _B64_CHARS[(data[i] >> 2) & 0x3F];
        out[j++] = _B64_CHARS[((data[i] & 0x3) << 4) | ((data[i + 1] >> 4) & 0xF)];
        out[j++] = _B64_CHARS[((data[i + 1] & 0xF) << 2) | ((data[i + 2] >> 6) & 0x3)];
        out[j++] = _B64_CHARS[data[i + 2] & 0x3F];
    }
    if (i < size)
    {
        out[j++] = _B64_CHARS[(data[i] >> 2) & 0x3F];
        if (i + 1 < size)
        {
            out[j++] = _B64_CHARS[((data[i] & 0x3) << 4) | ((data[i + 1] >> 4) & 0xF)];
            out[j++] = _B64_CHARS[((data[i + 1] & 0xF) << 2)];
        }
        else
        {
            out[j++] = _B64_CHARS[((data[i] & 0x3) << 4)];
            out[j++] = '=';
        }
        out[j++] = '=';
    }
    out[j] = '\0';
    return true;
}



/*************************************************************************************************/
/*  Builder helpers                                                                              */
/*************************************************************************************************/

#define JSON_INITIAL_CAPACITY 4096

static inline bool _json_init(JsonBuilder* builder)
{
    builder->capacity = JSON_INITIAL_CAPACITY;
    builder->count    = 0;
    builder->failed   = false;
    builder->data     = (char*)dvz_calloc(builder->capacity, 1);
    if (builder->data == NULL)
    {
        builder->failed = true;
        return false;
    }
    return true;
}

static inline bool _json_ensure(JsonBuilder* builder, uint64_t extra)
{
    if (builder->failed || builder->data == NULL)
    {
        builder->failed = true;
        return false;
    }
    uint64_t required = builder->count + extra + 1;
    if (required <= builder->capacity)
        return true;

    uint64_t cap = builder->capacity;
    while (required > cap)
    {
        if (cap > UINT64_MAX / 2)
        {
            builder->failed = true;
            return false;
        }
        cap *= 2;
    }
    char* d = (char*)dvz_realloc(builder->data, cap);
    if (d == NULL)
    {
        builder->failed = true;
        return false;
    }
    builder->capacity = cap;
    builder->data     = d;
    return true;
}

static inline void _json_append(JsonBuilder* builder, const char* fmt, ...)
{
    if (builder->failed)
        return;
    while (true)
    {
        uint64_t avail = builder->capacity - builder->count;
        va_list  args;
        va_start(args, fmt);
        int w = dvz_vsnprintf(builder->data + builder->count, (size_t)avail, fmt, args);
        va_end(args);
        if (w < 0)
        {
            if (!_json_ensure(builder, builder->capacity))
                return;
            continue;
        }
        uint64_t wu = (uint64_t)w;
        if (wu < avail)
        {
            builder->count += wu;
            return;
        }
        if (!_json_ensure(builder, wu))
            return;
    }
}

/* Append base64-encoded bytes directly into the JSON string. */
static inline void
_json_append_base64(JsonBuilder* builder, const uint8_t* data, uint64_t size)
{
    if (builder->failed)
        return;
    uint64_t enc_len = _base64_encoded_len(size);
    if (!_json_ensure(builder, enc_len + 2)) /* +2 for quotes */
        return;
    builder->data[builder->count++] = '"';
    _encode_base64(data, size, builder->data + builder->count, builder->capacity - builder->count);
    builder->count += enc_len;
    builder->data[builder->count++] = '"';
    builder->data[builder->count]   = '\0';
}

static inline char* _json_finish(JsonBuilder* builder)
{
    if (builder->failed)
    {
        dvz_free(builder->data);
        builder->data = NULL;
    }
    return builder->data;
}
