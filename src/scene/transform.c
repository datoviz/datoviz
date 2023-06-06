/*************************************************************************************************/
/*  Transform                                                                                    */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/transform.h"
#include "common.h"
#include "request.h"
#include "scene/array.h"
#include "scene/dual.h"
#include "scene/visual.h"



/*************************************************************************************************/
/*  Transform                                                                                    */
/*************************************************************************************************/

DvzTransform* dvz_transform(DvzRequester* rqr)
{
    ANN(rqr);

    DvzTransform* tr = (DvzTransform*)calloc(1, sizeof(DvzTransform));

    // TODO: chaining of transforms, in which case there should really be only one dual.

    // NOTE: the transform holds the DvzMVP dual.
    tr->dual = dvz_dual_dat(rqr, sizeof(DvzMVP), DVZ_DAT_FLAGS_MAPPABLE);

    // Initialize the MVP on initialization.
    dvz_transform_update(tr, dvz_mvp_default());

    return tr;
}



void dvz_transform_update(DvzTransform* tr, DvzMVP mvp)
{
    // This function updates the DvzMVP structure on the CPU, and emits a dat upload request with
    // the new DvzMVP value.

    ANN(tr);

    // NOTE: this is safe, &mvp is immediately copied internally in the dual.
    dvz_dual_data(&tr->dual, 0, 1, &mvp);

    // Emit the dat upload request.
    dvz_dual_update(&tr->dual);
}



DvzMVP* dvz_transform_mvp(DvzTransform* tr)
{
    ANN(tr);
    ANN(tr->dual.array);

    DvzMVP* mvp = (DvzMVP*)dvz_array_item(tr->dual.array, 0);
    ANN(mvp);

    return mvp;
}



void dvz_transform_destroy(DvzTransform* tr)
{
    ANN(tr);
    log_trace("destroy transform");
    if (tr->dual.array != NULL)
        dvz_dual_destroy(&tr->dual);
    FREE(tr);
}
