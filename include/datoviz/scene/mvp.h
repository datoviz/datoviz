/*************************************************************************************************/
/* MVP                                                                                           */
/*************************************************************************************************/

#ifndef DVZ_HEADER_MVP
#define DVZ_HEADER_MVP



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz_math.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

// NOTE: must correspond to values in common.glsl
#define DVZ_SPECIALIZATION_TRANSFORM 16
#define DVZ_TRANSFORM_FIXED_X        0x1
#define DVZ_TRANSFORM_FIXED_Y        0x2
#define DVZ_TRANSFORM_FIXED_Z        0x4
#define DVZ_TRANSFORM_FIXED_ALL      0x7

#define DVZ_SPECIALIZATION_VIEWPORT 17



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

// Forward declarations.
typedef struct DvzMVP DvzMVP;



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return a default DvzMVP struct
 *
 * @returns the DvzMVP struct
 */
DvzMVP dvz_mvp_default(void);



/**
 */
void dvz_mvp_apply(DvzMVP* mvp, vec4 point, vec4 out);



/**
 */
void dvz_mvp_print(DvzMVP* mvp);



EXTERN_C_OFF

#endif
