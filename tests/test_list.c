/*************************************************************************************************/
/*  Testing list */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdio.h>

#include "list.h"
#include "test.h"
#include "test_list.h"
#include "testing.h"



/*************************************************************************************************/
/*  List tests */
/*************************************************************************************************/

int test_list_1(TstSuite* suite)
{
    DvzList* list = dvz_list();


    int a = 10;
    int b = 20;
    int c = 30;

    AT(dvz_list_count(list) == 0);
    dvz_list_append(list, &a);
    AT(dvz_list_count(list) == 1);
    dvz_list_append(list, &b);
    AT(dvz_list_count(list) == 2);
    dvz_list_append(list, &c);
    AT(dvz_list_count(list) == 3);

    void* item = NULL;
    uint64_t i = 0;
    // NOTE: this is an assignment used as truth value.
    while ((item = dvz_list_iter(list)))
    {
        log_debug("item #%d %u", i, (uint64_t)item);
        i++;
    }
    AT(i == 3);

    // dvz_list_remove

    // Destroy the list.
    dvz_list_destroy(list);
    return 0;
}
