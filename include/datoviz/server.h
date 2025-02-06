/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/* Server                                                                                        */
/*************************************************************************************************/

#ifndef DVZ_HEADER_SERVER
#define DVZ_HEADER_SERVER



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_enums.h"
#include "_log.h"
#include "datoviz_math.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzServer DvzServer;

// Forward declarations.
typedef struct DvzHost DvzHost;
typedef struct DvzGpu DvzGpu;
typedef struct DvzRenderer DvzRenderer;
typedef struct DvzBatch DvzBatch;
typedef struct DvzMouse DvzMouse;
typedef struct DvzKeyboard DvzKeyboard;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzServer
{
    DvzHost* host;
    DvzGpu* gpu;
    DvzRenderer* rd;
    DvzBatch* batch;
    DvzMouse* mouse;
    DvzKeyboard* keyboard;
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/



EXTERN_C_OFF

#endif
