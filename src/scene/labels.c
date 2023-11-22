/*************************************************************************************************/
/*  Labels                                                                                       */
/*************************************************************************************************/


/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/labels.h"
#include "_macros.h"
#include "scene/ticks.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/


/*************************************************************************************************/
/*  Util functions                                                                               */
/*************************************************************************************************/


static inline void _get_tick_format(DvzTicksFormat format, uint32_t precision, char* fmt)
{
    uint32_t offset = 4;
    strcpy(fmt, "%s%.XF"); // [2] = precision, [3] = f or e
    snprintf(&fmt[offset], 4, "%u", precision);
    switch (format)
    {
    case DVZ_TICKS_FORMAT_DECIMAL:
        fmt[offset + 1] = 'f';
        break;
    case DVZ_TICKS_FORMAT_SCIENTIFIC:
        fmt[offset + 1] = 'e';
        break;
    default:
        log_error("unknown tick format %d", format);
        break;
    }
}



static inline void _tick_label(double x, char* tick_format, char* out)
{
    if (x == 0)
    {
        snprintf(out, 2, "0");
        return;
    }
    char sign[2] = {0};
    sign[0] = x < 0 ? '-' : '+';
    snprintf(out, MAX_GLYPHS_PER_LABEL, tick_format, sign, fabs(x));
    ASSERT(strnlen(out, 2 * MAX_GLYPHS_PER_LABEL) < MAX_GLYPHS_PER_LABEL);
}



/*************************************************************************************************/
/*  Labels functions                                                                             */
/*************************************************************************************************/

DvzLabels* dvz_labels(void)
{
    DvzLabels* labels = (DvzLabels*)calloc(1, sizeof(DvzLabels));

    // NOTE: each label is 0-terminated.
    labels->labels = (char*)calloc(MAX_LABELS * MAX_GLYPHS_PER_LABEL, sizeof(char));
    labels->index = (uint32_t*)calloc(MAX_LABELS, sizeof(uint32_t));
    labels->length = (uint32_t*)calloc(MAX_LABELS, sizeof(uint32_t));

    return labels;
}



uint32_t dvz_labels_generate(DvzLabels* labels, double lmin, double lmax, double lstep)
{
    ANN(labels);

    // TODO
    DvzTicksFormat format = DVZ_TICKS_FORMAT_DECIMAL;
    uint32_t precision = 3;

    char tick_format[12] = {0};
    _get_tick_format(format, precision, tick_format);

    ASSERT(lstep > 0);
    double x0 = lmin;
    double x = x0;
    uint32_t count = tick_count(lmin, lmax, lstep);
    labels->count = count;
    if (count == 0)
        return 0;
    ASSERT(count > 0);

    char* s = NULL;
    uint32_t k = 0, n = 0;
    for (uint32_t i = 0; i < count; i++)
    {
        x = x0 + i * lstep;
        s = &labels->labels[k];
        _tick_label(x, tick_format, s);

        n = strnlen(s, MAX_GLYPHS_PER_LABEL * 2);
        ASSERT(n < MAX_GLYPHS_PER_LABEL);

        labels->length[i] = n;
        labels->index[i] = k;
        k += (n + 1); // NOTE: 0-terminated strings
    }
    return count;
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
uint32_t* dvz_labels_length(DvzLabels* labels)
{
    // the size is the number of ticks, each number is the length of the label
    ANN(labels);
    return labels->length;
}



void dvz_labels_print(DvzLabels* labels)
{
    ANN(labels);
    for (uint32_t i = 0; i < labels->count; i++)
    {
        printf("%02d\t%s\n", i, &labels->labels[labels->index[i]]);
    }
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

    FREE(labels);
}
