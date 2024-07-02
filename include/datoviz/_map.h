/*************************************************************************************************/
/*  Map                                                                                          */
/*************************************************************************************************/

#ifndef DVZ_HEADER_MAP
#define DVZ_HEADER_MAP



/*************************************************************************************************/
/*  Includes                                                                                    */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "_macros.h"
#include "datoviz_math.h"



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

typedef struct DvzMap DvzMap;

// Forward declarations.



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Create a map, storing key-values pairs.
 *
 * @returns a map
 */
DvzMap* dvz_map(void);



/**
 * Return whether a key exists in the map.
 *
 * @param map the map
 * @param key the key
 * @returns a boolean
 */
bool dvz_map_exists(DvzMap* map, DvzId key);



/**
 * Add an item to a map.
 *
 * @param map the map
 * @param key the key
 * @param type the type of the item
 * @param value an pointer to the item (memory exlusively managed by the user)
 */
void dvz_map_add(DvzMap* map, DvzId key, int type, void* value);



/**
 * Remove an item from a map.
 *
 * @param map the map
 * @param key the key
 */
void dvz_map_remove(DvzMap* map, DvzId key);



/**
 * Get an item in the map.
 *
 * @param map the map
 * @param key the key
 * @returns a pointer to the item associated to the key (memory exlusively managed by the user)
 */
void* dvz_map_get(DvzMap* map, DvzId key);



/**
 * Get the type of an item in the map.
 *
 * @param map the map
 * @param key the key
 * @returns the type
 */
int dvz_map_type(DvzMap* map, DvzId key);



/**
 * Get the number of items in the map of a given type, or of any type (if using 0).
 *
 * @param map the map
 * @param type the type of the item, or 0 (count all items)
 * @returns the number of items
 */
uint64_t dvz_map_count(DvzMap* map, int type);



/**
 * Get the first element of a given type.
 *
 * @param map the map
 * @param type the type of the item
 * @returns a pointer to the item (memory exlusively managed by the user)
 */
void* dvz_map_first(DvzMap* map, int type);



/**
 * Get the last element of a given type.
 *
 * @param map the map
 * @param type the type of the item
 * @returns a pointer to the item (memory exlusively managed by the user)
 */
void* dvz_map_last(DvzMap* map, int type);



/**
 * Destroy a map.
 *
 * @param map the map
 */
void dvz_map_destroy(DvzMap* map);



EXTERN_C_OFF

#endif
