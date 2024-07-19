/*************************************************************************************************/
/*  Version number: single source of truth                                                       */
/*************************************************************************************************/

#ifndef DVZ_HEADER_PUBLIC_VERSION
#define DVZ_HEADER_PUBLIC_VERSION



#define _str(s) #s
#define xstr(s) _str(s)

// The single source of truth for the version number is here. All other copies of the version
// numbers in the other files, in the development scripts etc are updated by `just bump $version`.

#define DVZ_VERSION_MAJOR 0
#define DVZ_VERSION_MINOR 2
#define DVZ_VERSION_PATCH 0

#define DVZ_VERSION                                                                               \
    (xstr(DVZ_VERSION_MAJOR) "." xstr(DVZ_VERSION_MINOR) "." xstr(DVZ_VERSION_PATCH))

#define DVZ_NAME        "Datoviz"
#define DVZ_MAINTAINER  "Cyrille Rossant <cyrille.rossant at gmail.com>"
#define DVZ_DESCRIPTION "A C library for high-performance GPU scientific visualization"



#endif
