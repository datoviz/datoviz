/*
* Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
* Licensed under the MIT license. See LICENSE file in the project root for details.
* SPDX-License-Identifier: MIT
*/

/*************************************************************************************************/
/*  MVP                                                                                          */
/*************************************************************************************************/


/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/mvp.h"
#include "_cglm.h"
#include "datoviz.h"



/*************************************************************************************************/
/*  MVP functions                                                                                */
/*************************************************************************************************/

DvzMVP dvz_mvp(mat4 model, mat4 view, mat4 proj)
{
    DvzMVP mvp = {0};
    glm_mat4_copy(model, mvp.model);
    glm_mat4_copy(view, mvp.view);
    glm_mat4_copy(proj, mvp.proj);
    return mvp;
}



DvzMVP dvz_mvp_default(void)
{
    DvzMVP mvp = {0};
    glm_mat4_identity(mvp.model);
    glm_mat4_identity(mvp.view);
    glm_mat4_identity(mvp.proj);
    return mvp;
}



void dvz_mvp_apply(DvzMVP* mvp, vec4 point, vec4 out)
{
    ANN(mvp);

    glm_mat4_mulv(mvp->model, point, out);
    glm_mat4_mulv(mvp->view, out, out);
    glm_mat4_mulv(mvp->proj, out, out);
}



void dvz_mvp_print(DvzMVP* mvp)
{
    ANN(mvp);

    glm_mat4_print(mvp->model, stdout);
    glm_mat4_print(mvp->view, stdout);
    glm_mat4_print(mvp->proj, stdout);
}
