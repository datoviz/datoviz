/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Common types                                                                                 */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <stdbool.h>
#include <string.h>

#include "datoviz_enums.h"
#include "datoviz_keycodes.h"
#include "datoviz_math_types.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

#define DvzColor cvec4
#define DvzAlpha uint8_t



typedef struct DvzShape DvzShape;
typedef struct DvzMVP DvzMVP;
typedef struct DvzViewport DvzViewport;
typedef struct _VkViewport _VkViewport;
typedef struct DvzAtlasFont DvzAtlasFont;
typedef struct DvzTime DvzTime;


// Qt.
typedef struct DvzQtApp DvzQtApp;
typedef struct QApplication QApplication;
typedef struct DvzQtWindow DvzQtWindow;

// Recorder.
typedef struct DvzRecorderViewport DvzRecorderViewport;
typedef struct DvzRecorderPush DvzRecorderPush;
typedef struct DvzRecorderDraw DvzRecorderDraw;
typedef struct DvzRecorderDrawIndexed DvzRecorderDrawIndexed;
typedef struct DvzRecorderDrawIndirect DvzRecorderDrawIndirect;
typedef struct DvzRecorderDrawIndexedIndirect DvzRecorderDrawIndexedIndirect;
typedef union DvzRecorderUnion DvzRecorderUnion;
typedef struct DvzRecorderCommand DvzRecorderCommand;

// Forward declarations.
typedef struct DvzTimerItem DvzTimerItem;
typedef struct DvzGuiWindow DvzGuiWindow;
typedef struct DvzApp DvzApp;
typedef struct DvzAtlas DvzAtlas;
typedef struct DvzFont DvzFont;
typedef struct DvzList DvzList;
typedef struct DvzFifo DvzFifo;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzAtlasFont
{
    unsigned long ttf_size;
    unsigned char* ttf_bytes;
    DvzAtlas* atlas;
    DvzFont* font;
    float font_size;
};



struct DvzTime
{
    uint64_t seconds;
    uint64_t nanoseconds;
};
