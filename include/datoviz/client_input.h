/*
* Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
* Licensed under the MIT license. See LICENSE file in the project root for details.
* SPDX-License-Identifier: MIT
*/

/*************************************************************************************************/
/*  Client input                                                                                 */
/*************************************************************************************************/

#ifndef DVZ_HEADER_CLIENT_INPUT
#define DVZ_HEADER_CLIENT_INPUT



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "common.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

// Forward declarations.
typedef struct DvzClient DvzClient;
typedef struct DvzWindow DvzWindow;



/*************************************************************************************************/
/*  Client input functions                                                                       */
/*************************************************************************************************/

void dvz_window_input(DvzWindow* window);

void dvz_client_input(DvzClient* client);



#endif
