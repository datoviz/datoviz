/*************************************************************************************************/
/*  Testing dual                                                                                 */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/test_dual.h"
#include "_map.h"
#include "request.h"
#include "scene/array.h"
#include "scene/dual.h"
#include "test.h"
#include "testing.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Dual tests                                                                                   */
/*************************************************************************************************/

int test_dual_1(TstSuite* suite)
{
    DvzRequester* rqr = dvz_requester();
    dvz_requester_begin(rqr);
    DvzArray* array = dvz_array(16, DVZ_DTYPE_CHAR);
    DvzId dat = 1;

    DvzDual dual = dvz_dual(rqr, array, dat);

    dvz_dual_dirty(&dual, 2, 3);
    AT(dual.dirty_first == 2);
    AT(dual.dirty_last == 2 + 3);

    dvz_dual_dirty(&dual, 7, 2);
    AT(dual.dirty_first == 2);
    AT(dual.dirty_last == 9);

    dvz_dual_dirty(&dual, 3, 7);
    AT(dual.dirty_first == 2);
    AT(dual.dirty_last == 10);

    dvz_dual_clear(&dual);
    char data[16] = {0};
    for (uint32_t i = 0; i < 16; i++)
    {
        data[i] = i;
    }
    dvz_dual_data(&dual, 4, 8, &data[4]);
    dvz_dual_update(&dual);

    // Check the array has been updated.
    for (uint32_t i = 0; i < 16; i++)
    {
        if (i < 4)
        {
            AT(*((char*)dvz_array_item(array, i)) == 0);
        }
        else if (i < 12)
        {
            AT(*((char*)dvz_array_item(array, i)) == (char)i);
        }
        else
        {
            AT(*((char*)dvz_array_item(array, i)) == 0);
        }
    }

    AT(rqr->count == 1);
    DvzRequest* rq = &rqr->requests[0];
    AT(rq->action == DVZ_REQUEST_ACTION_UPLOAD);
    AT(rq->type == DVZ_REQUEST_OBJECT_DAT);
    AT(rq->id == dat);
    AT(rq->content.dat_upload.offset == 4);
    AT(rq->content.dat_upload.size == 8);
    AT(*(char*)rq->content.dat_upload.data == 4);

    dvz_dual_destroy(&dual);
    dvz_array_destroy(array);
    dvz_requester_destroy(rqr);

    return 0;
}
