/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Version number: single source of truth                                                       */
/*************************************************************************************************/

#ifndef DVZ_HEADER_PUBLIC_VERSION
#define DVZ_HEADER_PUBLIC_VERSION



#define _str(s) #s
#define xstr(s) _str(s)

// The single source of truth for the version number is here.
// All copies of the version numbers in the different files are updated with `just bump $version`

#define DVZ_VERSION_MAJOR 0
#define DVZ_VERSION_MINOR 3
#define DVZ_VERSION_PATCH 3
#define DVZ_VERSION_DEVEL -dev

// DVZ_VERSION
#if DEBUG
#define DVZ_VERSION                                                                               \
    (xstr(DVZ_VERSION_MAJOR) "." xstr(DVZ_VERSION_MINOR) "." xstr(DVZ_VERSION_PATCH) "" xstr(     \
        DVZ_VERSION_DEVEL) " (DEBUG)")
#else
#define DVZ_VERSION                                                                               \
    (xstr(DVZ_VERSION_MAJOR) "." xstr(DVZ_VERSION_MINOR) "." xstr(DVZ_VERSION_PATCH) "" xstr(     \
        DVZ_VERSION_DEVEL))
#endif



#define DVZ_NAME       "Datoviz"
#define DVZ_MAINTAINER "Cyrille Rossant <cyrille.rossant at gmail.com>"



#endif
