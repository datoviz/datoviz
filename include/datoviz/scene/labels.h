/*************************************************************************************************/
/* Labels                                                                                        */
/*************************************************************************************************/

#ifndef DVZ_HEADER_LABELS
#define DVZ_HEADER_LABELS



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_log.h"
#include "_math.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzLabels DvzLabels;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzLabels
{
    uint32_t count;   // number of labels
    char* labels;     // concatenation of all null-terminated strings
    uint32_t* index;  // index of the first glyph of each label
    uint32_t* length; // the length of each label
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DVZ_EXPORT DvzLabels* dvz_labels(void);



DVZ_EXPORT uint32_t dvz_labels_generate(DvzLabels* labels, double lmin, double lmax, double lstep);



DVZ_EXPORT char* dvz_labels_string(DvzLabels* labels);



DVZ_EXPORT uint32_t* dvz_labels_index(DvzLabels* labels);



DVZ_EXPORT uint32_t* dvz_labels_length(DvzLabels* labels);



DVZ_EXPORT void dvz_labels_print(DvzLabels* labels);



DVZ_EXPORT void dvz_labels_destroy(DvzLabels* labels);



EXTERN_C_OFF

#endif
