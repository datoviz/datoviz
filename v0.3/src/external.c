/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  External                                                                                     */
/*************************************************************************************************/

#include "datoviz_external.h"
#include "renderer.h"
#include "resources.h"
#include "scene/baker.h"
#include "scene/dual.h"
#include "scene/visual.h"
#include "vklite.h"
#include "vklite_utils.h"



/*************************************************************************************************/
/*  External functions                                                                           */
/*************************************************************************************************/

int dvz_external_vertex(DvzRenderer* rd, DvzVisual* visual, uint32_t binding_idx, DvzSize* offset)
{
    ANN(rd);

    VkDevice device = rd->gpu->device;
    ASSERT(device != VK_NULL_HANDLE);


    // HACK: find the id of the dat backing the vertex binding.
    // Should be refactored in visual.c dvz_visual_vertex_id() etc.
    DvzBaker* baker = visual->baker;
    ANN(baker);

    ASSERT(binding_idx < DVZ_MAX_VERTEX_BINDINGS);
    DvzBakerVertex* bv = &baker->vertex_bindings[binding_idx];
    ANN(bv);

    DvzDual dual = bv->dual;
    DvzId dat_id = dual.dat;
    ASSERT(dat_id != DVZ_ID_NONE);

    // Now, get the DvzBuffer and finally the VkDeviceMemory for the underlying buffer.
    DvzDat* dat = dvz_renderer_dat(rd, dat_id);
    ANN(dat);

    DvzBuffer* buffer = dat->br.buffer;
    ANN(buffer);

    VkDeviceMemory memory = dat->br.buffer->vma.info.deviceMemory;
    ASSERT(memory != VK_NULL_HANDLE);


    // Now we call the Vulkan function to get the external memory handle on that buffer.
    VkMemoryGetFdInfoKHR getFdInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR,
        .memory = memory,
        .handleType = rd->gpu->external_memory_handle_type,
    };

    PFN_vkGetMemoryFdKHR vkGetMemoryFdKHR_ =
        (PFN_vkGetMemoryFdKHR)vkGetDeviceProcAddr(device, "vkGetMemoryFdKHR");
    if (!vkGetMemoryFdKHR_)
    {
        log_error(
            "Vulkan function vkGetMemoryFdKHR not found. Ensure Vulkan supports external memory.");
        return -1;
    }

    int fd = 0;
    VK_CHECK_RESULT(vkGetMemoryFdKHR_(device, &getFdInfo, &fd));

    // TODO: offset

    return fd;
}



int dvz_external_index(DvzRenderer* rd, DvzVisual* visual, DvzSize* offset)
{
    ANN(rd);
    int fd = 0;
    return fd;
}



int dvz_external_dat(DvzRenderer* rd, DvzVisual* visual, uint32_t slot_idx, DvzSize* offset)
{
    ANN(rd);
    int fd = 0;
    return fd;
}



int dvz_external_tex(DvzRenderer* rd, DvzVisual* visual, uint32_t slot_idx, DvzSize* offset)
{
    ANN(rd);
    int fd = 0;
    return fd;
}
