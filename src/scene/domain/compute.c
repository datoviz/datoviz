/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene compute                                                                                */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_scene.h"
#include "core/scene_notify_internal.h"
#include "domain/compute_internal.h"
#include "_visual_internal.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static void _scene_compute_notify(DvzSceneCompute* compute)
{
    if (compute == NULL || compute->scene == NULL)
        return;
    DvzScene* scene = compute->scene;
    for (uint32_t i = 0; i < scene->figure_count; i++)
    {
        DvzFigure* figure = &scene->figures[i];
        for (uint32_t j = 0; j < figure->compute_count; j++)
        {
            if (figure->computes[j] == compute)
            {
                _scene_notify_request_frame(figure);
                break;
            }
        }
    }
}


static void _scene_compute_detach_all(DvzSceneCompute* compute)
{
    if (compute == NULL || compute->scene == NULL)
        return;
    DvzScene* scene = compute->scene;
    for (uint32_t fi = 0; fi < scene->figure_count; fi++)
    {
        DvzFigure* figure = &scene->figures[fi];
        for (uint32_t ci = 0; ci < figure->compute_count;)
        {
            if (figure->computes[ci] != compute)
            {
                ci++;
                continue;
            }
            for (uint32_t j = ci + 1; j < figure->compute_count; j++)
                figure->computes[j - 1] = figure->computes[j];
            figure->compute_count--;
        }
    }
}


void _scene_compute_reset(DvzSceneCompute* compute)
{
    if (compute == NULL)
        return;
    _scene_compute_detach_all(compute);
    dvz_memset(compute, sizeof(DvzSceneCompute), 0, sizeof(DvzSceneCompute));
}


static DvzSceneCompute* _scene_alloc_compute_slot(DvzScene* scene)
{
    ANN(scene);
    for (uint32_t i = 0; i < DVZ_SCENE_MAX_COMPUTES; i++)
    {
        DvzSceneCompute* compute = &scene->computes[i];
        if (compute->scene != NULL)
            continue;
        dvz_memset(compute, sizeof(DvzSceneCompute), 0, sizeof(DvzSceneCompute));
        compute->scene = scene;
        if (i + 1 > scene->compute_count)
            scene->compute_count = i + 1;
        return compute;
    }
    return NULL;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzSceneCompute* dvz_scene_compute(DvzScene* scene, const DvzSceneComputeDesc* desc)
{
    ANN(scene);
    ANN(desc);
    if (desc->shader_source == NULL || desc->shader_source[0] == '\0')
    {
        log_error("scene compute shader source is required");
        return NULL;
    }
    if (desc->entry_point != NULL && strcmp(desc->entry_point, "main") != 0)
    {
        log_error("scene compute currently supports the 'main' entry point only");
        return NULL;
    }
    if (desc->dispatch[0] == 0 || desc->dispatch[1] == 0 || desc->dispatch[2] == 0)
    {
        log_error("scene compute dispatch dimensions must be non-zero");
        return NULL;
    }
    if (scene->compute_count >= DVZ_SCENE_MAX_COMPUTES)
    {
        log_error("maximum scene compute count reached");
        return NULL;
    }

    DvzSceneCompute* compute = _scene_alloc_compute_slot(scene);
    if (compute == NULL)
    {
        log_error("maximum scene compute count reached");
        return NULL;
    }

    compute->desc = *desc;
    compute->dispatch[0] = desc->dispatch[0];
    compute->dispatch[1] = desc->dispatch[1];
    compute->dispatch[2] = desc->dispatch[2];
    dvz_strlcpy(
        compute->label, desc->label != NULL && desc->label[0] != '\0' ? desc->label : "compute",
        sizeof(compute->label));
    return compute;
}


void dvz_scene_compute_destroy(DvzSceneCompute* compute)
{
    if (compute == NULL)
        return;
    if (!_scene_visual_mutation_allowed(compute->scene, "destroy scene compute"))
        return;
    _scene_compute_reset(compute);
}


bool dvz_scene_compute_set_dispatch(DvzSceneCompute* compute, uint32_t x, uint32_t y, uint32_t z)
{
    ANN(compute);
    if (x == 0 || y == 0 || z == 0)
    {
        log_error("scene compute dispatch dimensions must be non-zero");
        return false;
    }
    compute->dispatch[0] = x;
    compute->dispatch[1] = y;
    compute->dispatch[2] = z;
    _scene_compute_notify(compute);
    return true;
}


bool dvz_scene_compute_set_buffer(
    DvzSceneCompute* compute, uint32_t binding, DvzSceneBuffer* buffer,
    DvzSceneComputeAccess access, uint64_t byte_offset, uint64_t byte_size)
{
    ANN(compute);
    if (!_scene_visual_mutation_allowed(compute->scene, "bind scene compute buffer"))
        return false;

    if (buffer == NULL)
    {
        for (uint32_t i = 0; i < compute->binding_count; i++)
        {
            if (compute->bindings[i].active && compute->bindings[i].binding == binding)
            {
                compute->bindings[i].active = false;
                _scene_compute_notify(compute);
                return true;
            }
        }
        return true;
    }

    if (buffer->scene != compute->scene)
    {
        log_error("scene compute buffer belongs to another scene");
        return false;
    }
    if ((buffer->desc.usage & DVZ_SCENE_BUFFER_USAGE_STORAGE) == 0)
    {
        log_error("scene compute buffer requires STORAGE usage");
        return false;
    }
    if (buffer->desc.byte_size == 0)
    {
        log_error("scene compute buffer byte_size must be non-zero");
        return false;
    }
    if (byte_offset > buffer->desc.byte_size)
    {
        log_error("scene compute buffer binding range exceeds buffer size");
        return false;
    }
    uint64_t range = byte_size != 0 ? byte_size : buffer->desc.byte_size - byte_offset;
    if (range > buffer->desc.byte_size - byte_offset)
    {
        log_error("scene compute buffer binding range exceeds buffer size");
        return false;
    }

    DvzSceneComputeBinding* slot = NULL;
    for (uint32_t i = 0; i < compute->binding_count; i++)
    {
        if (compute->bindings[i].active && compute->bindings[i].binding == binding)
        {
            slot = &compute->bindings[i];
            break;
        }
    }
    if (slot == NULL)
    {
        if (compute->binding_count >= DVZ_SCENE_MAX_NODE_RESOURCES)
        {
            log_error("maximum scene compute binding count reached");
            return false;
        }
        slot = &compute->bindings[compute->binding_count++];
    }

    slot->active = true;
    slot->binding = binding;
    slot->buffer = buffer;
    slot->access = access;
    slot->byte_offset = byte_offset;
    slot->byte_size = range;
    _scene_compute_notify(compute);
    return true;
}


bool dvz_figure_add_compute(DvzFigure* figure, DvzSceneCompute* compute)
{
    ANN(figure);
    ANN(compute);
    if (compute->scene != figure->scene)
    {
        log_error("scene compute belongs to another scene");
        return false;
    }
    for (uint32_t i = 0; i < figure->compute_count; i++)
    {
        if (figure->computes[i] == compute)
            return true;
    }
    if (figure->compute_count >= DVZ_SCENE_MAX_COMPUTES)
    {
        log_error("maximum figure compute count reached");
        return false;
    }
    figure->computes[figure->compute_count++] = compute;
    _scene_notify_request_frame(figure);
    return true;
}


bool dvz_figure_remove_compute(DvzFigure* figure, DvzSceneCompute* compute)
{
    ANN(figure);
    ANN(compute);
    for (uint32_t i = 0; i < figure->compute_count; i++)
    {
        if (figure->computes[i] != compute)
            continue;
        for (uint32_t j = i + 1; j < figure->compute_count; j++)
            figure->computes[j - 1] = figure->computes[j];
        figure->compute_count--;
        _scene_notify_request_frame(figure);
        return true;
    }
    return true;
}
