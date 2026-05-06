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
#include "_base64.h"
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
    if (extra > UINT64_MAX - 1 || builder->count > UINT64_MAX - extra - 1)
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

/* Append a JSON string with escaping for quotes, slashes, and control characters. */
static inline void _json_append_escaped_string(JsonBuilder* builder, const char* value)
{
    if (builder->failed)
        return;
    if (value == NULL)
    {
        _json_append(builder, "null");
        return;
    }

    if (!_json_ensure(builder, 1))
        return;
    builder->data[builder->count++] = '"';
    builder->data[builder->count]   = '\0';

    for (const unsigned char* p = (const unsigned char*)value; *p != '\0'; p++)
    {
        switch (*p)
        {
        case '"':
            _json_append(builder, "\\\"");
            break;
        case '\\':
            _json_append(builder, "\\\\");
            break;
        case '\b':
            _json_append(builder, "\\b");
            break;
        case '\f':
            _json_append(builder, "\\f");
            break;
        case '\n':
            _json_append(builder, "\\n");
            break;
        case '\r':
            _json_append(builder, "\\r");
            break;
        case '\t':
            _json_append(builder, "\\t");
            break;
        default:
            if (*p < 0x20)
            {
                _json_append(builder, "\\u%04x", (unsigned int)*p);
            }
            else
            {
                if (!_json_ensure(builder, 1))
                    return;
                builder->data[builder->count++] = (char)*p;
                builder->data[builder->count]   = '\0';
            }
            break;
        }
    }

    if (!_json_ensure(builder, 1))
        return;
    builder->data[builder->count++] = '"';
    builder->data[builder->count]   = '\0';
}

/* Append base64-encoded bytes directly into the JSON string. */
static inline void
_json_append_base64(JsonBuilder* builder, const uint8_t* data, uint64_t size)
{
    if (builder->failed)
        return;
    uint64_t enc_len = _dvz_b64_encoded_len(size);
    if (!_json_ensure(builder, enc_len + 2)) /* +2 for quotes */
        return;
    builder->data[builder->count++] = '"';
    _dvz_b64_encode(data, size, builder->data + builder->count, builder->capacity - builder->count);
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
