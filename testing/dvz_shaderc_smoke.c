/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Source-build shaderc smoke                                                                   */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>
#include <stdio.h>

#include "_alloc.h"
#include "datoviz/vk/gpu_ctx.h"



/*************************************************************************************************/
/*  Entry-point                                                                                  */
/*************************************************************************************************/

int main(void)
{
    static const char glsl[] = "#version 450\n"
                               "vec2 positions[3] = vec2[](\n"
                               "    vec2(0.0, -0.5),\n"
                               "    vec2(0.5, 0.5),\n"
                               "    vec2(-0.5, 0.5));\n"
                               "void main() {\n"
                               "    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);\n"
                               "}\n";

    uint64_t size = 0;
    uint32_t* spv = dvz_compile_glsl("vertex", glsl, &size);
    if (spv == NULL)
    {
        fprintf(stderr, "dvz_compile_glsl returned NULL\n");
        return 1;
    }
    if (size < 4 || size % sizeof(uint32_t) != 0)
    {
        fprintf(stderr, "invalid SPIR-V size: %llu\n", (unsigned long long)size);
        dvz_free(spv);
        return 1;
    }
    if (spv[0] != 0x07230203)
    {
        fprintf(stderr, "invalid SPIR-V magic: 0x%08x\n", spv[0]);
        dvz_free(spv);
        return 1;
    }

    printf("source-build shaderc smoke produced %llu bytes\n", (unsigned long long)size);
    dvz_free(spv);
    return 0;
}
