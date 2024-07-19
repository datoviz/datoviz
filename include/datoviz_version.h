/*************************************************************************************************/
/*  Version number: single source of truth                                                       */
/*************************************************************************************************/

#ifndef DVZ_HEADER_PUBLIC_VERSION
#define DVZ_HEADER_PUBLIC_VERSION



#define _str(s) #s
#define xstr(s) _str(s)

#define DVZ_VERSION_MAJOR 0
#define DVZ_VERSION_MINOR 2
#define DVZ_VERSION_PATCH 0

#define DVZ_VERSION                                                                               \
    (xstr(DVZ_VERSION_MAJOR) "." xstr(DVZ_VERSION_MINOR) "." xstr(DVZ_VERSION_PATCH))



#define DVZ_NAME "Datoviz"



#endif
