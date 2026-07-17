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
                               "layout(local_size_x = 1) in;\n"
                               "void main() {}\n";

    uint64_t size = 0;
    uint32_t* spv = dvz_compile_glsl("compute", glsl, &size);
    if (spv == NULL)
    {
        fprintf(stderr, "dvz_compile_glsl returned NULL\n");
        return 1;
    }
    if (size < 5 * sizeof(uint32_t) || size % sizeof(uint32_t) != 0)
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
    if (spv[1] != 0x00010600)
    {
        fprintf(stderr, "unexpected SPIR-V version: 0x%08x\n", spv[1]);
        dvz_free(spv);
        return 1;
    }

    const uint32_t word_count = (uint32_t)(size / sizeof(uint32_t));
    for (uint32_t i = 5; i < word_count;)
    {
        const uint32_t instruction_words = spv[i] >> 16;
        const uint32_t opcode = spv[i] & 0xffff;
        if (instruction_words == 0 || instruction_words > word_count - i)
        {
            fprintf(stderr, "invalid SPIR-V instruction at word %u\n", i);
            dvz_free(spv);
            return 1;
        }
        /* OpDecorate <target> BuiltIn WorkgroupSize. */
        if (opcode == 71 && instruction_words >= 4 && spv[i + 2] == 11 && spv[i + 3] == 25)
        {
            fprintf(stderr, "deprecated SPIR-V WorkgroupSize built-in found\n");
            dvz_free(spv);
            return 1;
        }
        i += instruction_words;
    }

    printf("source-build shaderc compute smoke produced %llu bytes\n", (unsigned long long)size);
    dvz_free(spv);
    return 0;
}
