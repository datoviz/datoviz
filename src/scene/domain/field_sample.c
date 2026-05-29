/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene field sample helpers                                                                   */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdint.h>

#include "_assertions.h"
#include "_scene.h"
#include "field_internal.h"



/*************************************************************************************************/
/*  Function prototypes                                                                          */
/*************************************************************************************************/

static double _half_to_double(uint16_t bits);



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Convert an IEEE 754 binary16 payload to double precision.
 *
 * @param bits the half-float bit pattern
 * @return the converted double value
 */
static double _half_to_double(uint16_t bits)
{
    uint32_t sign = (bits >> 15) & 0x1u;
    uint32_t exp = (bits >> 10) & 0x1fu;
    uint32_t frac = bits & 0x3ffu;

    if (exp == 0)
    {
        if (frac == 0)
            return sign ? -0.0 : 0.0;
        double value = ldexp((double)frac / 1024.0, -14);
        return sign ? -value : value;
    }
    if (exp == 31)
    {
        if (frac == 0)
            return sign ? -INFINITY : INFINITY;
        return NAN;
    }

    double value = ldexp(1.0 + (double)frac / 1024.0, (int32_t)exp - 15);
    return sign ? -value : value;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Read one scalar sampled-field value as a double.
 *
 * @param field the sampled field
 * @param sample_index the scalar sample index
 * @param out_value the output value
 * @return whether the field format was supported
 */
bool _field_read_scalar(
    const DvzSampledField* field, uint64_t sample_index, double* out_value)
{
    ANN(field);
    ANN(out_value);
    ANN(field->data);
    const uint8_t* bytes = (const uint8_t*)field->data;
    switch (field->desc.format)
    {
    case DVZ_FIELD_FORMAT_R8_UNORM:
        *out_value = (double)bytes[sample_index] / 255.0;
        return true;
    case DVZ_FIELD_FORMAT_R8_SNORM:
    {
        int8_t v = ((const int8_t*)field->data)[sample_index];
        *out_value = v == INT8_MIN ? -1.0 : (double)v / 127.0;
        return true;
    }
    case DVZ_FIELD_FORMAT_R8_UINT:
        *out_value = (double)((const uint8_t*)field->data)[sample_index];
        return true;
    case DVZ_FIELD_FORMAT_R8_SINT:
        *out_value = (double)((const int8_t*)field->data)[sample_index];
        return true;
    case DVZ_FIELD_FORMAT_R16_UNORM:
        *out_value = (double)((const uint16_t*)field->data)[sample_index] / 65535.0;
        return true;
    case DVZ_FIELD_FORMAT_R16_SNORM:
    {
        int16_t v = ((const int16_t*)field->data)[sample_index];
        *out_value = v == INT16_MIN ? -1.0 : (double)v / 32767.0;
        return true;
    }
    case DVZ_FIELD_FORMAT_R16_UINT:
        *out_value = (double)((const uint16_t*)field->data)[sample_index];
        return true;
    case DVZ_FIELD_FORMAT_R16_SINT:
        *out_value = (double)((const int16_t*)field->data)[sample_index];
        return true;
    case DVZ_FIELD_FORMAT_R16_FLOAT:
        *out_value = _half_to_double(((const uint16_t*)field->data)[sample_index]);
        return true;
    case DVZ_FIELD_FORMAT_R32_UINT:
        *out_value = (double)((const uint32_t*)field->data)[sample_index];
        return true;
    case DVZ_FIELD_FORMAT_R32_SINT:
        *out_value = (double)((const int32_t*)field->data)[sample_index];
        return true;
    case DVZ_FIELD_FORMAT_R32_FLOAT:
        *out_value = (double)((const float*)field->data)[sample_index];
        return true;
    default:
        return false;
    }
}
