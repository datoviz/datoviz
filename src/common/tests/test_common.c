/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing common                                                                               */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_common.h"
#include "testing.h"
#include <stdio.h>


/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

// #define TEST(test, tags, setup, teardown, flags)
//     tst_suite_add(&suite, #test, tags, test, setup, teardown, NULL, flags);



/*************************************************************************************************/
/*  Entry-point                                                                                  */
/*************************************************************************************************/

int main(int argc, char** argv)
{
    TstSuite suite = tst_suite();
    // DvzTestCtx ctx = {0};
    // suite.context = &ctx;

    char* tags = "";
    int flags = 0;
    TstFunction setup = NULL, teardown = NULL;


    /*********************************************************************************************/
    /*  Utils                                                                                    */
    /*********************************************************************************************/

    // tags = "utils";

    // TEST_NO_FIXTURE(test_thread_1)

    // tst_suite_add(&suite, "test_obj_1", tags, test_obj_1, setup, teardown, NULL, flags);

    // tst_suite_run(&suite, match);
    tst_suite_destroy(&suite);
    return 0;
}
