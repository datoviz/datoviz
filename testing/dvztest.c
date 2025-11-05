/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Datoviz test runner                                                                          */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stddef.h>

#include "../src/common/tests/test_common.h"
#include "../src/ds/tests/test_ds.h"
#include "../src/fileio/tests/test_fileio.h"
#include "../src/math/tests/test_math.h"
#include "../src/thread/tests/test_thread.h"
#include "../src/vk/tests/test_vk.h"
#include "../src/vklite/tests/test_vklite.h"
#include "testing.h"



/*************************************************************************************************/
/*  Entry-point                                                                                  */
/*************************************************************************************************/

int main(int argc, char** argv)
{
    TstSuite suite = tst_suite();

    test_common(&suite);
    test_ds(&suite);
    test_fileio(&suite);
    test_math(&suite);
    test_thread(&suite);
    test_vk(&suite);
    test_vklite(&suite);

    tst_suite_run(&suite, argc >= 2 ? argv[1] : NULL);
    tst_suite_destroy(&suite);
    return 0;
}
