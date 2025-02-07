/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Qt app                                                                                       */
/*************************************************************************************************/

#ifndef DVZ_HEADER_QTAPP
#define DVZ_HEADER_QTAPP



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_obj.h"
#include "datoviz.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzHost DvzHost;
typedef struct DvzGpu DvzGpu;
typedef struct DvzRenderer DvzRenderer;
typedef struct DvzBatch DvzBatch;
typedef struct DvzRenderpass DvzRenderpass;
typedef struct QApplication QApplication;
typedef struct QVulkanInstance QVulkanInstance;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

EXTERN_C_ON

struct DvzQtApp
{
    QApplication* qapp;
    QVulkanInstance* inst;
    DvzHost* host;
    DvzGpu* gpu;
    DvzRenderer* rd;
    DvzRenderpass* renderpass;
    DvzBatch* batch;
};



EXTERN_C_OFF

#endif
