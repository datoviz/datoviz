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
    DvzDual dual;       // Dual array with a DvzMVP struct.
    DvzTransform* next; // NOTE: transform chaining not implemented yet
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



DVZ_EXPORT DvzMVP* dvz_transform_mvp(DvzTransform* tr);



/**
 *
 */
DVZ_EXPORT void dvz_transform_destroy(DvzTransform* tr);



EXTERN_C_OFF

#endif
