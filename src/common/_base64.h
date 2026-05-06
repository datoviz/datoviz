/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Base64 encode/decode (header-only)                                                           */
/*                                                                                               */
/*  Single canonical implementation; previously duplicated across:                               */
/*    - src/scene/_json.h          (encode, inline)                                              */
/*    - src/drp2/stream.c          (encode, file-static)                                         */
/*    - src/drp2/runtime.c         (decode, file-static)                                         */
/*************************************************************************************************/

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"



static const char _DVZ_B64_CHARS[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";



/**
 * Number of base64 characters needed to encode `size` bytes (excluding NUL).
 */
static inline uint64_t _dvz_b64_encoded_len(uint64_t size)
{
    return ((size + 2) / 3) * 4;
}



/**
 * Encode `size` bytes from `data` into `out`. `out_cap` must be at least
 * `_dvz_b64_encoded_len(size) + 1`. Writes a trailing NUL.
 *
 * @return false on bad arguments or insufficient capacity, true on success
 */
static inline bool
_dvz_b64_encode(const uint8_t* data, uint64_t size, char* out, uint64_t out_cap)
{
    if (data == NULL || out == NULL)
        return false;
    if (_dvz_b64_encoded_len(size) + 1 > out_cap)
        return false;

    uint64_t i = 0, j = 0;
    for (; i + 2 < size; i += 3)
    {
        out[j++] = _DVZ_B64_CHARS[(data[i] >> 2) & 0x3F];
        out[j++] = _DVZ_B64_CHARS[((data[i] & 0x3) << 4) | ((data[i + 1] >> 4) & 0xF)];
        out[j++] = _DVZ_B64_CHARS[((data[i + 1] & 0xF) << 2) | ((data[i + 2] >> 6) & 0x3)];
        out[j++] = _DVZ_B64_CHARS[data[i + 2] & 0x3F];
    }
    if (i < size)
    {
        out[j++] = _DVZ_B64_CHARS[(data[i] >> 2) & 0x3F];
        if (i + 1 < size)
        {
            out[j++] = _DVZ_B64_CHARS[((data[i] & 0x3) << 4) | ((data[i + 1] >> 4) & 0xF)];
            out[j++] = _DVZ_B64_CHARS[((data[i + 1] & 0xF) << 2)];
        }
        else
        {
            out[j++] = _DVZ_B64_CHARS[(data[i] & 0x3) << 4];
            out[j++] = '=';
        }
        out[j++] = '=';
    }
    out[j] = '\0';
    return true;
}



static inline int _dvz_b64_value(char c)
{
    if (c >= 'A' && c <= 'Z')
        return c - 'A';
    if (c >= 'a' && c <= 'z')
        return c - 'a' + 26;
    if (c >= '0' && c <= '9')
        return c - '0' + 52;
    if (c == '+')
        return 62;
    if (c == '/')
        return 63;
    return -1;
}



/**
 * Decode `src` (NUL-terminated, padded base64, whitespace-tolerant) into a
 * freshly heap-allocated byte buffer of exactly `expected_size` bytes.
 *
 * Rejects size==0, mid-stream pad characters, invalid chars, or any output
 * length not exactly equal to `expected_size`.
 *
 * @param src             input base64 string (NUL-terminated)
 * @param expected_size   exact decoded byte count required
 * @param out             receives a fresh dvz_calloc'd buffer on success
 * @return true on exact-size decode, false otherwise (out untouched/freed)
 */
static inline bool
_dvz_b64_decode_exact(const char* src, uint64_t expected_size, uint8_t** out)
{
    ANN(src);
    ANN(out);
    *out = NULL;
    if (expected_size == 0)
        return false;

    uint8_t* decoded = (uint8_t*)dvz_calloc(expected_size, sizeof(uint8_t));
    if (decoded == NULL)
        return false;

    uint32_t quad[4] = {0};
    uint32_t quad_count = 0;
    uint64_t written = 0;
    bool padded = false;
    for (uint32_t i = 0; src[i] != '\0'; i++)
    {
        char c = src[i];
        if (c == ' ' || c == '\n' || c == '\r' || c == '\t')
            continue;
        if (padded && c != '=')
        {
            dvz_free(decoded);
            return false;
        }

        if (c == '=')
        {
            quad[quad_count++] = 64;
            padded = true;
        }
        else
        {
            int value = _dvz_b64_value(c);
            if (value < 0)
            {
                dvz_free(decoded);
                return false;
            }
            quad[quad_count++] = (uint32_t)value;
        }

        if (quad_count != 4)
            continue;

        if (written < expected_size)
            decoded[written++] = (uint8_t)((quad[0] << 2) | (quad[1] >> 4));
        if (quad[2] != 64 && written < expected_size)
            decoded[written++] = (uint8_t)(((quad[1] & 0x0f) << 4) | (quad[2] >> 2));
        if (quad[3] != 64 && written < expected_size)
            decoded[written++] = (uint8_t)(((quad[2] & 0x03) << 6) | quad[3]);

        quad_count = 0;
    }

    if (quad_count != 0 || written != expected_size)
    {
        dvz_free(decoded);
        return false;
    }

    *out = decoded;
    return true;
}
