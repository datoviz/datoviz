/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Platform external handles                                                                    */
/*************************************************************************************************/

#include "datoviz/common/functions.h"

#if OS_WINDOWS
#include <windows.h>
#elif OS_UNIX
#include <unistd.h>
#endif



void dvz_external_handle_close(DvzExternalHandle handle)
{
    if (handle == DVZ_EXTERNAL_HANDLE_INVALID)
    {
        return;
    }
#if OS_WINDOWS
    HANDLE win32_handle = (HANDLE)(intptr_t)handle;
    if (win32_handle != NULL && win32_handle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(win32_handle);
    }
#elif OS_UNIX
    close((int)handle);
#else
    (void)handle;
#endif
}
