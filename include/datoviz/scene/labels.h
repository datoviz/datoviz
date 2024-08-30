/*
* Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
* Licensed under the MIT license. See LICENSE file in the project root for details.
* SPDX-License-Identifier: MIT
*/

/*************************************************************************************************/
/* Labels                                                                                        */
/*************************************************************************************************/

#ifndef DVZ_HEADER_LABELS
#define DVZ_HEADER_LABELS



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_log.h"
#include "datoviz_math.h"
#include "ticks.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_LABELS_MAX_EXPONENT_LENGTH 16
#define DVZ_LABELS_MAX_OFFSET_LENGTH   24



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzLabels DvzLabels;
typedef struct DvzLabelFormat DvzLabelFormat;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzLabels
{
    uint32_t count;   // number of labels
    double* values;   // tick values
    char* labels;     // concatenation of all null-terminated strings
    uint32_t* index;  // index of the first glyph of each label
    uint32_t* length; // the length of each label

    char exponent[DVZ_LABELS_MAX_EXPONENT_LENGTH];
    char offset[DVZ_LABELS_MAX_OFFSET_LENGTH];
};



struct DvzLabelFormat
{
    DvzTicksFormat format;
    uint32_t precision;
    int32_t exponent;
    double offset;
    char tick_format[12];
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzLabelFormat
dvz_label_format(DvzTicksFormat format, uint32_t precision, int32_t exponent, double offset);



DvzLabels* dvz_labels(void);



// Return the total number of glyphs.
uint32_t dvz_labels_generate(
    DvzLabels* labels, DvzTicksFormat format, uint32_t precision, //
    int32_t exponent, double offset,                              //
    double lmin, double lmax, double lstep);



char* dvz_labels_string(DvzLabels* labels);



uint32_t* dvz_labels_index(DvzLabels* labels);



double* dvz_labels_values(DvzLabels* labels);



uint32_t* dvz_labels_length(DvzLabels* labels);



char* dvz_labels_exponent(DvzLabels* labels);



char* dvz_labels_offset(DvzLabels* labels);



void dvz_labels_print(DvzLabels* labels);



void dvz_labels_destroy(DvzLabels* labels);



EXTERN_C_OFF

#endif
