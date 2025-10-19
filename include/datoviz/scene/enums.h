/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene enums                                                                                  */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Panel link flags.
typedef enum
{
    DVZ_PANEL_LINK_FLAGS_NONE = 0x00,
    DVZ_PANEL_LINK_FLAGS_MODEL = 0x01,
    DVZ_PANEL_LINK_FLAGS_VIEW = 0x02,
    DVZ_PANEL_LINK_FLAGS_PROJECTION = 0x04,
} DvzPanelLinkFlags;
