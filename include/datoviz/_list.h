/*************************************************************************************************/
/*  List                                                                                         */
/*************************************************************************************************/

#ifndef DVZ_HEADER_LIST
#define DVZ_HEADER_LIST



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "_macros.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_MAX_LIST_CAPACITY 64



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
DVZ_EXPORT DvzList* dvz_list(void);



/**
 * Append an item to a list.
 *
 * @param list the list
 * @param value an pointer to the item (memory exlusively managed by the user)
 */
DVZ_EXPORT void dvz_list_append(DvzList* list, DvzListItem value);



/**
 * Remove an item from a list.
 *
 * @param list the list
 * @param value the value to remove, if it exists
 */
DVZ_EXPORT void dvz_list_remove(DvzList* list, uint64_t index);



DVZ_EXPORT void dvz_list_remove_pointer(DvzList* list, const void* pointer);



DVZ_EXPORT void dvz_list_insert(DvzList* list, uint64_t index, DvzListItem value);



DVZ_EXPORT DvzListItem dvz_list_get(DvzList* list, uint64_t index);



DVZ_EXPORT uint64_t dvz_list_index(DvzList* list, int value);



DVZ_EXPORT bool dvz_list_has(DvzList* list, int value);



DVZ_EXPORT void dvz_list_clear(DvzList* list);



/**
 * Return the number of items in a list.
 *
 * @param list the list
 * @returns the number of items
 */
DVZ_EXPORT uint64_t dvz_list_count(DvzList* list);



/**
 * Destroy a list.
 *
 * @param list the list
 */
DVZ_EXPORT void dvz_list_destroy(DvzList* list);



EXTERN_C_OFF

#endif
