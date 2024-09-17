/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

layout(std140, binding = USER_BINDING) uniform MeshParams
{
    vec4 light_pos;
    vec4 light_params; // ambient, diffuse, specular, specular expon
    vec4 stroke;       // r, g, b, stroke-width
    int isoline_count; // number of isolines
}
params;
