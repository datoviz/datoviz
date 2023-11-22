/*************************************************************************************************/
/*  Labels                                                                                       */
/*************************************************************************************************/


/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/labels.h"
#include "_macros.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Labels functions                                                                             */
/*************************************************************************************************/

DvzLabels* dvz_labels(void)
{
    DvzLabels* labels = (DvzLabels*)calloc(1, sizeof(DvzLabels));
    return labels;
}



uint32_t dvz_labels_generate(DvzLabels* labels, double lmin, double lmax, double lstep)
{
    // returns the number of ticks
    ANN(labels);
    // TODO
    return 0;
}



char* dvz_labels_string(DvzLabels* labels)
{
    // no free, concatenation of all null-terminated strings
    ANN(labels);
    return NULL;
}



uint32_t* dvz_labels_index(DvzLabels* labels)
{
    ANN(labels);
    // no free, the size is the output of generate(), each number is the index of the first
    // character of that tick
    return NULL;
}



uint32_t* dvz_labels_length(DvzLabels* labels)
{
    // no free, the size is the number of ticks, each number is the length of the label
    ANN(labels);
    return NULL;
}



void dvz_labels_destroy(DvzLabels* labels)
{
    ANN(labels);
    FREE(labels);
}
