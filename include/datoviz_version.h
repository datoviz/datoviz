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
#define DVZ_VERSION_MINOR 2
#define DVZ_VERSION_PATCH 1
#define DVZ_VERSION_DEVEL -dev

#define DVZ_VERSION                                                                               \
    (xstr(DVZ_VERSION_MAJOR) "." xstr(DVZ_VERSION_MINOR) "." xstr(DVZ_VERSION_PATCH) "" xstr(     \
        DVZ_VERSION_DEVEL))

#define DVZ_NAME       "Datoviz"
#define DVZ_MAINTAINER "Cyrille Rossant <cyrille.rossant at gmail.com>"



#endif
