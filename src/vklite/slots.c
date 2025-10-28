/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Slots                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include "../src/vk/macros.h"
#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "datoviz/common/macros.h"
#include "datoviz/common/obj.h"
#include "datoviz/vk/device.h"
#include "datoviz/vk/queues.h"
#include "datoviz/vklite/slots.h"
#include "types.h"
#include "vulkan/vulkan_core.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

void dvz_slots(DvzDevice* device, DvzSlots* slots)
{
    ANN(device);
    ANN(slots);

    slots->device = device;
    dvz_obj_init(&slots->obj);
}



void dvz_slots_binding(DvzSlots* slots, uint32_t idx, VkDescriptorType type) { ANN(slots); }



void dvz_slots_push(
    DvzSlots* slots, VkShaderStageFlagBits stages, VkDeviceSize offset, VkDeviceSize size)
{
    ANN(slots);
}



void dvz_slots_create(DvzSlots* slots) { ANN(slots); }



void dvz_slots_destroy(DvzSlots* slots) { ANN(slots); }
