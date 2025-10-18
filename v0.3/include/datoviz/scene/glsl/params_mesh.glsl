/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

layout(std140, binding = USER_BINDING) uniform MeshParams
{
    mat4 light_dir;    /* x, y, z, *** */
    mat4 light_color;  /* r, g, b, *** */
    mat4 light_params; /* ambient, diffuse, specular, exponent */
    vec4 edgecolor;       // r, g, b, a
    float linewidth;       // contour line width
    int isoline_count; // number of isolines
}
params;
