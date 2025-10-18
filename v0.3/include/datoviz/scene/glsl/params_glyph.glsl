/*
* Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
* Licensed under the MIT license. See LICENSE file in the project root for details.
* SPDX-License-Identifier: MIT
*/

layout(std140, binding = USER_BINDING) uniform GlyphParams
{
    vec2 size; // glyph size in pixels
    vec4 bgcolor;
}
params;
