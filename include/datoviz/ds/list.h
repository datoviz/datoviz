/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  List                                                                                         */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "datoviz/common/macros.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_LIST_INITIAL_CAPACITY 64



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzList DvzList;
typedef union DvzListItem DvzListItem;

// Forward declarations.



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

union DvzListItem
{
    int i;
    void* p;
};



struct DvzList
{
    uint64_t capacity;
    uint64_t count;
    DvzListItem* values;
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Create a list storing pointers.
 *
 * @returns a list
 */
DvzList* dvz_list(void);



/**
 * Append an item to a list.
 *
 * @param list the list
 * @param value an pointer to the item (memory exlusively managed by the user)
 */
void dvz_list_append(DvzList* list, DvzListItem value);



/**
 * Remove an item from a list.
 *
 * @param list the list
 * @param value the value to remove, if it exists
 */
void dvz_list_remove(DvzList* list, uint64_t index);



void dvz_list_remove_pointer(DvzList* list, const void* pointer);



void dvz_list_insert(DvzList* list, uint64_t index, DvzListItem value);



DvzListItem dvz_list_get(DvzList* list, uint64_t index);



uint64_t dvz_list_index(DvzList* list, int value);



bool dvz_list_has(DvzList* list, int value);



void dvz_list_clear(DvzList* list);



/**
 * Return the number of items in a list.
 *
 * @param list the list
 * @returns the number of items
 */
uint64_t dvz_list_count(DvzList* list);



/**
 * Destroy a list.
 *
 * @param list the list
 */
void dvz_list_destroy(DvzList* list);



EXTERN_C_OFF
