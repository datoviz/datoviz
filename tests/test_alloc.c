/*************************************************************************************************/
/*  Testing alloc                                                                                */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdio.h>

#include "alloc.h"
#include "test.h"
#include "test_alloc.h"
#include "test_fifo.h"
#include "testing.h"



/*************************************************************************************************/
/*  Alloc tests                                                                                  */
/*************************************************************************************************/

int test_alloc_1(TstSuite* suite)
{
    DvzSize size = 32;
    DvzSize alignment = 4;
    DvzSize offset = 0;
    DvzSize resized = 0;

    DvzAlloc* alloc = dvz_alloc(size, alignment);
    // [----|----|----|----|----|----|...

    offset = dvz_alloc_new(alloc, 2, &resized);
    // [XX--|----|----|----|----|----|...
    AT(offset == 0);
    AT(!resized);
    AT(dvz_alloc_get(alloc, offset) == 4);

    offset = dvz_alloc_new(alloc, 2, &resized);
    // [XX--|XX--|----|----|----|----|...
    AT(offset == 4);
    AT(!resized);
    AT(dvz_alloc_get(alloc, offset) == 4);

    offset = dvz_alloc_new(alloc, 1, &resized);
    // [XX--|XX--|XX--|----|----|----|...
    AT(offset == 8);
    AT(!resized);
    AT(dvz_alloc_get(alloc, offset) == 4);

    dvz_alloc_free(alloc, 4);
    // [XX--|----|XX--|----|----|----|...

    offset = dvz_alloc_new(alloc, 3, &resized);
    // [XX--|XXX-|XX--|----|----|----|...
    AT(offset == 4);
    AT(!resized);
    AT(dvz_alloc_get(alloc, offset) == 4);

    offset = dvz_alloc_new(alloc, 13, &resized);
    // [XX--|XXX-|XX--|XXXX|XXXX|XXXX|X---|----]
    AT(offset == 12);
    AT(!resized);
    AT(dvz_alloc_get(alloc, offset) == 16);

    offset = dvz_alloc_new(alloc, 5, &resized);
    // [XX--|XXX-|XX--|XXXX|XXXX|XXXX|X---|XXXX] [X---|...
    AT(resized);
    AT(offset == 4 * 7);

    offset = dvz_alloc_new(alloc, 256, &resized);
    // [XX--|XXX-|XX--|XXXX|XXXX|XXXX|X---|XXXX] [X---|...
    AT(resized);
    AT(offset == 36);

    dvz_alloc_destroy(alloc);
    return 0;
}



int test_alloc_2(TstSuite* suite)
{
    DvzSize size = 1024;
    DvzSize alignment = 16;
    DvzAlloc* alloc = dvz_alloc(size, alignment);

    uint32_t n = 0;
    DvzSize offset = 0;
    for (uint32_t i = 0; i < 1000; i++)
    {
        n = (uint32_t)abs(dvz_rand_int()) % 16256;
        if (n % 2 == 0)
            offset = dvz_alloc_new(alloc, n, NULL);
        else
            dvz_alloc_free(alloc, offset);
    }
    dvz_alloc_stats(alloc);

    dvz_alloc_destroy(alloc);
    return 0;
}



int test_alloc_3(TstSuite* suite)
{
    DvzSize size = 128;
    DvzSize alignment = 16;
    DvzSize offset = 0;
    DvzSize resized = 0;
    ASSERT(size % alignment == 0);

    DvzAlloc* alloc = dvz_alloc(2 * size, alignment);

    offset = dvz_alloc_new(alloc, size, &resized);
    AT(offset == 0);

    offset = dvz_alloc_new(alloc, size, &resized);
    AT(offset == size);

    dvz_alloc_free(alloc, offset);

    offset = dvz_alloc_new(alloc, 2 * size, &resized);
    AT(offset == size);

    dvz_alloc_destroy(alloc);
    return 0;
}
