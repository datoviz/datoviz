/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Debugging macros                                                                             */
/*************************************************************************************************/

#ifndef DVZ_HEADER_DEBUG
#define DVZ_HEADER_DEBUG



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stddef.h>

#include <b64/b64.h>



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#define Pu(x) printf("%s=%" PRIu64 "\n", (#x), (x));
#define Pd(x) printf("%s=%" PRId64 "\n", (#x), (x));
#define Pf(x) printf("%s=%.8f\n", (#x), (double)(x));
#define Ps(x) printf("%s=%s\n", (#x), (x));



/*************************************************************************************************/
/*  Debugging functions                                                                          */
/*************************************************************************************************/

/* */
/*
 */
static bool dvz_is_empty(size_t size, const uint8_t* buf)
{
    return buf[0] == 0 && !memcmp(buf, buf + 1, size - 1);
}



/* */
/*
 */
static void dvz_show_base64(size_t size, const void* buffer)
{
    char* encoded = b64_encode((const unsigned char*)buffer, size);
    printf("base64 is: %s\n", encoded);
    free(encoded);
}



static void _show_line(uint32_t group_size, uint32_t cols)
{
    uint32_t n = cols * 2 + 2 * (cols / group_size) + 1;

    for (uint32_t i = 0; i < n; i++)
    {
        if (i % (2 * (group_size + 1)) == 0)
            printf("+");
        else
            printf("-");
    }
    printf("\n");
}

static void dvz_show_buffer(uint32_t group_size, uint32_t cols, DvzSize size, void* data)
{
    ANN(data);
    ASSERT(size > 0);
    ASSERT(group_size > 0);
    ASSERT(cols > 0);

    printf("buffer with size %s:\n", pretty_size(size));

    _show_line(group_size, cols);

    for (uint32_t i = 0; i < size; i++)
    {
        if (i % group_size == 0)
            printf("| ");

        printf("%hhu ", ((char*)data)[i]);

        if ((i > 0) && (i % cols == cols - 1))
            printf("|\n");
    }

    _show_line(group_size, cols);
}



#endif
