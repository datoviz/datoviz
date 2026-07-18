/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing logging                                                                              */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_assertions.h"
#include "_log.h"
#include "test_common.h"
#include "testing.h"



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_log_default_level(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);
    ANN(tstitem);

#if DEBUG
    AT(DVZ_DEFAULT_LOG_LEVEL == LOG_INFO);
#else
    AT(DVZ_DEFAULT_LOG_LEVEL > LOG_FATAL);
#endif
    return 0;
}
