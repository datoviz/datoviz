/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

layout(std140, binding = USER_BINDING) uniform ImageParams
{
    float radius;     // rounded rectangle radius, 0 for sharp corners
    float linewidth; // width of the border, 0 for no border
    vec4 edgecolor;  // color of the border
}
params;
