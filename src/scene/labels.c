/*
* Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
* Licensed under the MIT license. See LICENSE file in the project root for details.
* SPDX-License-Identifier: MIT
*/

/*************************************************************************************************/
/*  Labels                                                                                       */
/*************************************************************************************************/


/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/labels.h"
#include "_macros.h"
#include "labels_utils.h"
#include "scene/ticks.h"



/*************************************************************************************************/
/*  Label format                                                                                 */
/*************************************************************************************************/

DvzLabelFormat
dvz_label_format(DvzTicksFormat format, uint32_t precision, int32_t exponent, double offset)
{
    DvzLabelFormat f = {
        .format = format,
        .precision = precision,
        .exponent = exponent,
        .offset = offset,
    };
    _get_tick_format(format, precision, f.tick_format);
    return f;
}



/*************************************************************************************************/
/*  Labels functions                                                                             */
/*************************************************************************************************/

DvzLabels* dvz_labels(void)
{
    DvzLabels* labels = (DvzLabels*)calloc(1, sizeof(DvzLabels));

    // NOTE: each label is 0-terminated.
    labels->labels = (char*)calloc(MAX_LABELS * MAX_GLYPHS_PER_LABEL, sizeof(char));
    labels->values = (double*)calloc(MAX_LABELS, sizeof(double));
    labels->index = (uint32_t*)calloc(MAX_LABELS, sizeof(uint32_t));
    labels->length = (uint32_t*)calloc(MAX_LABELS, sizeof(uint32_t));

    return labels;
}



uint32_t dvz_labels_generate(
    DvzLabels* labels,
    // label format
    DvzTicksFormat format, uint32_t precision, int32_t exponent, double offset, //
    // ticks positions spec
    double lmin, double lmax, double lstep)
{
    ANN(labels);

    DvzLabelFormat fmt = dvz_label_format(format, precision, exponent, offset);

    ASSERT(lstep > 0);
    double x0 = lmin;
    double x = x0;
    uint32_t count = tick_count(lmin, lmax, lstep);
    labels->count = count; // tick count.
    if (count == 0)
        return 0;
    ASSERT(count > 0);

    char* s = NULL;
    uint32_t k = 0, n = 0;
    uint32_t glyph_count = 0;
    for (uint32_t i = 0; i < count; i++)
    {
        // Tick value, assuming linear range.
        x = x0 + i * lstep;

        // Pointer to the current tick string.
        s = &labels->labels[k];

        // Generate the label string.
        n = format_label(&fmt, x, s);

        labels->length[i] = n;
        labels->index[i] = k;
        labels->values[i] = x;

        // Keep track of the total number of glyphs for all labels.
        glyph_count += n;
        k += (n + 1); // NOTE: 0-terminated strings
    }

    // Display the exponent.
    if (_is_format_factored(format))
    {
        // Exponent.

        if (exponent != 0)
            _tick_label(pow(10, exponent), "x%g", labels->exponent);
        else
            labels->exponent[0] = 0;

        // Offset.
        if (offset != 0)
            _tick_label(offset, fmt.tick_format, labels->offset);
        else
            labels->offset[0] = 0;
    }

    return glyph_count;
}



// the output must not be freed
char* dvz_labels_string(DvzLabels* labels)
{
    ANN(labels);
    // concatenation of all null-terminated strings
    return labels->labels;
}



// the output must not be freed
uint32_t* dvz_labels_index(DvzLabels* labels)
{
    ANN(labels);
    // the size is the output of generate(), each number is the index of the first
    // character of that tick
    return labels->index;
}



// the output must not be freed
double* dvz_labels_values(DvzLabels* labels)
{
    ANN(labels);
    return labels->values;
}



// the output must not be freed
uint32_t* dvz_labels_length(DvzLabels* labels)
{
    // the size is the number of ticks, each number is the length of the label
    ANN(labels);
    return labels->length;
}



// the output must not be freed
char* dvz_labels_exponent(DvzLabels* labels)
{
    ANN(labels);
    return labels->exponent;
}



// the output must not be freed
char* dvz_labels_offset(DvzLabels* labels)
{
    ANN(labels);
    return labels->offset;
}



void dvz_labels_print(DvzLabels* labels)
{
    ANN(labels);
    for (uint32_t i = 0; i < labels->count; i++)
    {
        printf("%02d\t%s\n", i, &labels->labels[labels->index[i]]);
    }
    printf("\n");

    // Display the exponent if there is one.
    if (labels->exponent[0])
        printf("exponent : %s\n", labels->exponent);

    // Display the offset if there is one.
    if (labels->offset[0])
        printf("offset   : %s\n", labels->offset);
}



void dvz_labels_destroy(DvzLabels* labels)
{
    ANN(labels);

    if (labels->labels)
    {
        FREE(labels->labels);
    }

    if (labels->index)
    {
        FREE(labels->index);
    }

    if (labels->length)
    {
        FREE(labels->length);
    }

    if (labels->values)
    {
        FREE(labels->values);
    }

    FREE(labels);
}
