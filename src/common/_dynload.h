/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Portable runtime dynamic library loading                                                     */
/*************************************************************************************************/

#pragma once

/*
 * Three macros provide a thin cross-platform shim over the OS loader API:
 *
 *   DvzDynLib handle = dvz_dynlib_open(path);   // NULL on failure
 *   void* sym        = dvz_dynlib_sym(h, name); // NULL on failure
 *   dvz_dynlib_close(handle);
 *
 * These are intentionally macros rather than functions so each call site retains the original
 * file/line in compiler diagnostics without any wrapper overhead.
 */

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

typedef HMODULE DvzDynLib;
#define dvz_dynlib_open(path)       LoadLibraryA(path)
#define dvz_dynlib_sym(handle, sym) ((void*)(uintptr_t)GetProcAddress((handle), (sym)))
#define dvz_dynlib_close(handle)    FreeLibrary(handle)

#else

#include <dlfcn.h>

typedef void* DvzDynLib;
#define dvz_dynlib_open(path)       dlopen((path), RTLD_LAZY | RTLD_LOCAL)
#define dvz_dynlib_sym(handle, sym) dlsym((handle), (sym))
#define dvz_dynlib_close(handle)    dlclose(handle)

#endif
