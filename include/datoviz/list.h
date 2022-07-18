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

#include "_macros.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_ID_NONE 0



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzList DvzList;
typedef uint64_t DvzId;

// Forward declarations.



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/



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
DVZ_EXPORT void dvz_list_append(DvzList* list, void* value);



/**
 * Remove an item from a list.
 *
 * @param list the list
 * @param value the value to remove, if it exists
 */
DVZ_EXPORT void dvz_list_remove(DvzList* list, void* value);



/**
 * Go through all items in a list. To use in a while loop.
 *
 * @param list the list
 * @returns a pointer to the current item in the iteration, or NULL if the iteration is finished
 */
DVZ_EXPORT void* dvz_list_iter(DvzList* list);



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
