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
typedef struct DvzBatch DvzBatch;
// typedef struct DvzDual DvzDual;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    DVZ_TRANSFORM_FLAGS_LINEAR = 0x0000,
    DVZ_TRANSFORM_FLAGS_PANEL = 0x0010,
} DvzTransformFlags;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzTransform
{
    int flags;
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
DVZ_EXPORT DvzTransform* dvz_transform(DvzBatch* batch, int flags);



/**
 *
 */
DVZ_EXPORT void dvz_transform_set(DvzTransform* tr, DvzMVP mvp);



/**
 *
 */
DVZ_EXPORT void dvz_transform_update(DvzTransform* tr);



/**
 *
 */
DVZ_EXPORT void dvz_transform_next(DvzTransform* tr, DvzTransform* next);



/**
 *
 */
DVZ_EXPORT DvzMVP* dvz_transform_mvp(DvzTransform* tr);



/**
 *
 */
DVZ_EXPORT void dvz_transform_destroy(DvzTransform* tr);



EXTERN_C_OFF

#endif
