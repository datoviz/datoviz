/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "internal.h"

#include <string.h>

#include "_assertions.h"
#include "_technique.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return the graph pass matching one render node.
 *
 * @param plan the FramePlan
 * @param render the render node
 * @return the graph pass, or NULL when the render node has no graph pass
 */
const DvzFrameGraphPass* _contract_graph_pass_for_render(
    const DvzFramePlan* plan, const DvzFramePlanNode* render)
{
    ANN(plan);
    ANN(render);
    if (render->type != DVZ_FRAME_PLAN_NODE_RENDER)
        return NULL;
    const char* work_label = _scene_render_role_work_label(render->u.render.pass_role);
    if (work_label[0] == '\0')
        return NULL;

    uint32_t ordinal = 0;
    for (uint32_t i = 0; i < plan->count; i++)
    {
        const DvzFramePlanNode* candidate = &plan->nodes[i];
        if (candidate == render)
            break;
        if (candidate->type != DVZ_FRAME_PLAN_NODE_RENDER)
            continue;
        const char* candidate_label =
            _scene_render_role_work_label(candidate->u.render.pass_role);
        if (candidate_label[0] != '\0' &&
            strcmp(candidate->u.render.panel_id, render->u.render.panel_id) == 0 &&
            strcmp(candidate_label, work_label) == 0)
            ordinal++;
    }

    uint32_t seen = 0;
    for (uint32_t i = 0; i < dvz_frame_plan_graph_pass_count(plan); i++)
    {
        const DvzFrameGraphPass* pass = dvz_frame_plan_graph_pass_get(plan, i);
        if (pass == NULL || strcmp(pass->panel_id, render->u.render.panel_id) != 0 ||
            strcmp(pass->work_label, work_label) != 0)
            continue;
        if (seen == ordinal)
            return pass;
        seen++;
    }
    return NULL;
}



/**
 * Validate that every graph-backed render node has a matching graph pass.
 *
 * @param plan the FramePlan to inspect
 * @param report optional diagnostic report
 * @return whether every graph-backed render role has a graph pass
 */
bool _contract_validate_graph_backed_render_nodes(
    const DvzFramePlan* plan, DvzDiagnosticReport* report)
{
    ANN(plan);
    bool ok = true;
    for (uint32_t i = 0; i < plan->count; i++)
    {
        const DvzFramePlanNode* render = &plan->nodes[i];
        if (render->type != DVZ_FRAME_PLAN_NODE_RENDER)
            continue;
        if (!_scene_render_role_requires_graph_pass(render->u.render.pass_role))
            continue;
        if (_contract_graph_pass_for_render(plan, render) != NULL)
            continue;
        _contract_report(report, "graph-backed render node has no matching graph pass");
        ok = false;
    }
    return ok;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/



/**
 * Validate all graph-backed render contracts in one FramePlan.
 *
 * @param figure the figure that produced the FramePlan
 * @param plan the completed FramePlan
 * @param caps the active capability snapshot, or NULL to preserve requested sample counts
 * @param report optional diagnostic report
 * @return whether all graph-backed render contracts are valid
 */
bool _scene_frame_plan_contracts_validate_ex(
    const DvzFigure* figure, const DvzFramePlan* plan, const DvzCapabilitySnapshot* caps,
    DvzDiagnosticReport* report)
{
    ANN(figure);
    ANN(plan);
    bool ok = _contract_validate_graph_backed_render_nodes(plan, report);
    for (uint32_t i = 0; i < plan->count; i++)
    {
        const DvzFramePlanNode* render = &plan->nodes[i];
        if (render->type != DVZ_FRAME_PLAN_NODE_RENDER)
            continue;

        const DvzFrameGraphPass* graph_pass = _contract_graph_pass_for_render(plan, render);
        if (graph_pass == NULL)
            continue;

        const DvzPanel* panel = _contract_panel_for_render(figure, plan, render);
        if (panel == NULL)
        {
            _contract_report(report, "render contract has no matching panel");
            ok = false;
            continue;
        }

        DvzScenePassContract contract = {0};
        if (!_scene_pass_contract_from_render_ex(plan, panel, render, graph_pass, caps, &contract))
        {
            _contract_report(report, "render contract resolution failed");
            ok = false;
            continue;
        }
        if (!_scene_pass_contract_validate(&contract, report))
            ok = false;
    }
    return ok;
}



/**
 * Validate all graph-backed render contracts in one FramePlan.
 *
 * @param figure the figure that produced the FramePlan
 * @param plan the completed FramePlan
 * @param report optional diagnostic report
 * @return whether all graph-backed render contracts are valid
 */
bool _scene_frame_plan_contracts_validate(
    const DvzFigure* figure, const DvzFramePlan* plan, DvzDiagnosticReport* report)
{
    const DvzCapabilitySnapshot* caps =
        figure != NULL && figure->scene != NULL ? &figure->scene->caps : NULL;
    return _scene_frame_plan_contracts_validate_ex(figure, plan, caps, report);
}
