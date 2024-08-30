/*
* Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
* Licensed under the MIT license. See LICENSE file in the project root for details.
* SPDX-License-Identifier: MIT
*/

layout(std140, binding = USER_BINDING) uniform MonoglyphParams
{
    vec2 anchor; /* glyph anchor */
    float size;  /* glyph relative size */
}
params;
