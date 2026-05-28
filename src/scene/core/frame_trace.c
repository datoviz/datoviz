/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene FramePlan tracing                                                                      */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_frame_plan.h"
#include "_log.h"
#include "_scene.h"


/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Resolve the stable emitted figure identifier for one scene figure.
 *
 * @param figure the figure
 * @param out the destination string buffer
 * @param size the destination buffer size in bytes
 */
void _scene_figure_id(const DvzFigure* figure, char* out, uint32_t size)
{
    ANN(figure);
    ANN(out);
    ASSERT(size > 0);
    dvz_strlcpy(out, "fig0", size);
    if (figure->scene == NULL)
        return;
    for (uint32_t i = 0; i < figure->scene->figure_count; i++)
    {
        if (&figure->scene->figures[i] == figure)
        {
            dvz_snprintf(out, size, "fig%u", i);
            return;
        }
    }
}


/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Parse the `DVZ_FRAME_PLAN_TRACE` environment variable into a trace mode.
 *
 * @param value environment variable value, or NULL
 * @return the parsed FramePlan trace mode
 */
DvzFramePlanTraceMode _scene_frame_plan_trace_mode_from_env(const char* value)
{
    if (value == NULL)
        return DVZ_FRAME_PLAN_TRACE_NONE;
    if (strcmp(value, "0") == 0 || strcmp(value, "false") == 0 ||
        strcmp(value, "FALSE") == 0 || strcmp(value, "off") == 0 || strcmp(value, "OFF") == 0)
    {
        return DVZ_FRAME_PLAN_TRACE_NONE;
    }
    if (
        strcmp(value, "full") == 0 || strcmp(value, "FULL") == 0 ||
        strcmp(value, "ascii-full") == 0 || strcmp(value, "ASCII-FULL") == 0 ||
        strcmp(value, "full-ascii") == 0 || strcmp(value, "FULL-ASCII") == 0)
    {
        return DVZ_FRAME_PLAN_TRACE_FULL;
    }
    return DVZ_FRAME_PLAN_TRACE_NORMAL;
}



/**
 * Parse the `DVZ_FRAME_PLAN_TRACE` environment variable into terminal graph flags.
 *
 * @param value environment variable value, or NULL
 * @return FramePlan terminal graph flags
 */
uint32_t _scene_frame_plan_trace_flags_from_env(const char* value)
{
    uint32_t flags = DVZ_FRAME_PLAN_ASCII_VERBOSE;
    if (value == NULL)
        return flags;
    if (
        strcmp(value, "ascii") == 0 || strcmp(value, "ASCII") == 0 ||
        strcmp(value, "ascii-full") == 0 || strcmp(value, "ASCII-FULL") == 0 ||
        strcmp(value, "full-ascii") == 0 || strcmp(value, "FULL-ASCII") == 0)
    {
        flags |= DVZ_FRAME_PLAN_ASCII_ASCII_ONLY;
    }
    return flags;
}



/**
 * Return whether a FramePlan terminal graph should be printed for this figure.
 *
 * @param mode active trace mode
 * @param figure the figure owning the previous trace cache
 * @param graph the newly rendered terminal graph
 * @return whether the trace should be printed
 */
bool _scene_frame_plan_trace_should_print(
    DvzFramePlanTraceMode mode, const DvzFigure* figure, const char* graph)
{
    if (mode == DVZ_FRAME_PLAN_TRACE_NONE)
        return false;
    if (mode == DVZ_FRAME_PLAN_TRACE_FULL)
        return true;
    if (figure == NULL || graph == NULL)
        return false;
    if (!figure->has_last_frame_plan_trace || figure->last_frame_plan_trace == NULL)
        return true;
    return strcmp(figure->last_frame_plan_trace, graph) != 0;
}



/**
 * Reset the cached FramePlan terminal graph trace for one figure.
 *
 * @param figure the figure
 */
void _scene_figure_frame_plan_trace_reset(DvzFigure* figure)
{
    if (figure == NULL)
        return;
    dvz_free(figure->last_frame_plan_trace);
    figure->last_frame_plan_trace = NULL;
    figure->has_last_frame_plan_trace = false;
}



/**
 * Optionally print a FramePlan terminal graph according to `DVZ_FRAME_PLAN_TRACE`.
 *
 * @param figure the figure owning the FramePlan
 * @param plan the emitted FramePlan
 */
void _scene_frame_plan_trace(DvzFigure* figure, const DvzFramePlan* plan)
{
    ANN(figure);
    ANN(plan);

    const char* trace_env = getenv("DVZ_FRAME_PLAN_TRACE");
    DvzFramePlanTraceMode mode = _scene_frame_plan_trace_mode_from_env(trace_env);
    if (mode == DVZ_FRAME_PLAN_TRACE_NONE)
        return;

    uint32_t flags = _scene_frame_plan_trace_flags_from_env(trace_env);
    char* graph = dvz_frame_plan_graph_ascii(plan, flags);
    if (graph == NULL)
    {
        log_error("failed to render FramePlan terminal graph trace");
        return;
    }

    bool changed =
        _scene_frame_plan_trace_should_print(DVZ_FRAME_PLAN_TRACE_NORMAL, figure, graph);
    bool should_print = _scene_frame_plan_trace_should_print(mode, figure, graph);
    if (should_print)
    {
        dvz_fprintf(
            stderr, "\n[DVZ_FRAME_PLAN_TRACE %s]\n%s", changed ? "changed" : "full", graph);
    }

    if (changed)
    {
        _scene_figure_frame_plan_trace_reset(figure);
        figure->last_frame_plan_trace = graph;
        figure->has_last_frame_plan_trace = true;
    }
    else
    {
        dvz_frame_plan_graph_ascii_destroy(graph);
    }
}
