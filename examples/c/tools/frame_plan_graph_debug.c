/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  FramePlan terminal graph debug tool                                                          */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "_compat.h"
#include "frame_plan/frame_plan.h"
#include "techniques/_technique.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Emit one graph-backed WBOIT FramePlan and print its terminal graph.
 *
 * @param argc argument count
 * @param argv argument values
 * @return process exit code
 */
int main(int argc, char** argv)
{
    uint32_t flags = DVZ_FRAME_PLAN_ASCII_VERBOSE;
    if (argc > 1 && strcmp(argv[1], "--ascii") == 0)
        flags |= DVZ_FRAME_PLAN_ASCII_ASCII_ONLY;

    DvzFramePlan* plan = dvz_frame_plan("debug.wboit", 0);
    if (plan == NULL)
    {
        dvz_fprintf(stderr, "failed to create FramePlan\n");
        return 1;
    }

    bool ok = _scene_technique_emit_wboit_frame_graph(plan, "panel0", true, true);
    if (!ok)
    {
        dvz_fprintf(stderr, "failed to emit WBOIT frame graph\n");
        dvz_frame_plan_destroy(plan);
        return 1;
    }

    DvzDiagnosticReport report = {0};
    dvz_diagnostic_report_init(&report);
    ok = dvz_frame_plan_graph_validate(plan, &report);
    if (!ok)
    {
        for (uint32_t i = 0; i < dvz_diagnostic_report_count(&report); i++)
        {
            const char* message = dvz_diagnostic_report_get(&report, i);
            dvz_fprintf(stderr, "%s\n", message != NULL ? message : "unknown graph diagnostic");
        }
        dvz_frame_plan_destroy(plan);
        return 1;
    }

    char* graph = dvz_frame_plan_graph_ascii(plan, flags);
    if (graph == NULL)
    {
        dvz_fprintf(stderr, "failed to render FramePlan terminal graph\n");
        dvz_frame_plan_destroy(plan);
        return 1;
    }

    dvz_fprintf(stdout, "%s", graph);
    dvz_frame_plan_graph_ascii_destroy(graph);
    dvz_frame_plan_destroy(plan);
    return 0;
}
