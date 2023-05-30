/*************************************************************************************************/
/* Transform                                                                                     */
/*************************************************************************************************/

#ifndef DVZ_HEADER_TRANSFORM
#define DVZ_HEADER_TRANSFORM



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "dual.h"
#include "mvp.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzTransform DvzTransform;

// Forward declarations.
typedef struct DvzRequester DvzRequester;
// typedef struct DvzDual DvzDual;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzTransform
{
    DvzDual dual;
    DvzTransform* next; // NOTE: not implemented yet
    // DvzMVP mvp; // NOTE: this is redundant because the structure is stored in the dual (array)
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Transform                                                                                    */
/*************************************************************************************************/

/**
 *
 */
DVZ_EXPORT DvzTransform* dvz_transform(DvzRequester* rqr);



/**
 *
 */
DVZ_EXPORT void dvz_transform_update(DvzTransform* tr, DvzMVP mvp);



/**
 *
 */
DVZ_EXPORT void dvz_transform_destroy(DvzTransform* tr);



EXTERN_C_OFF

#endif
