/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/* Vulkan types                                                                                  */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "../vk/types.h"
#include "datoviz/common/obj.h"
#include "datoviz/math/types.h"
#include "datoviz/vk/enums.h"
#include "datoviz/vklite/buffers.h"
#include "datoviz/vklite/images.h"
#include <volk.h>



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzDevice DvzDevice;
typedef struct DvzQueue DvzQueue;

typedef struct DvzCommands DvzCommands;
typedef struct DvzSampler DvzSampler;
typedef struct DvzCompute DvzCompute;
typedef struct DvzPush DvzPush;
typedef struct DvzSlots DvzSlots;
typedef struct DvzDescriptors DvzDescriptors;
typedef struct DvzBuffer DvzBuffer;
typedef struct DvzBufferViews DvzBufferViews;
typedef struct DvzImages DvzImages;
typedef struct DvzImageViews DvzImageViews;
typedef struct VkBufferImageCopy2 DvzImageRegion;
typedef struct DvzVertexBinding DvzVertexBinding;
typedef struct DvzVertexAttr DvzVertexAttr;
typedef struct DvzSpecialization DvzSpecialization;
typedef struct VkRenderingAttachmentInfo DvzAttachment;
typedef struct DvzRendering DvzRendering;

typedef struct VkMemoryBarrier2 DvzBarrierMemory;
typedef struct VkBufferMemoryBarrier2 DvzBarrierBuffer;
typedef struct VkImageMemoryBarrier2 DvzBarrierImage;
typedef struct DvzBarriers DvzBarriers;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/
