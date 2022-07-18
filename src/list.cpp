/*************************************************************************************************/
/*  List                                                                                         */
/*************************************************************************************************/

#include "list.h"
#include "_log.h"

#include <list>
#include <numeric>
#include <utility>



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

extern "C" struct DvzList
{
    std::list<void*> _list;
    std::list<void*>::iterator _iter;
    int64_t _iter_idx;
};



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzList* dvz_list(void)
{
    DvzList* list = new DvzList();
    list->_list = std::list<void*>();
    list->_iter_idx = -1;
    return list;
}



void dvz_list_append(DvzList* list, void* value)
{
    ASSERT(list != NULL);
    ASSERT(value != NULL);

    log_trace("append value %u to list", (uint64_t)value);
    list->_list.push_back(value);
}



void dvz_list_remove(DvzList* list, void* value)
{
    ASSERT(list != NULL);
    ASSERT(value != NULL);

    list->_list.remove(value);
}



void* dvz_list_iter(DvzList* list)
{
    ASSERT(list != NULL);
    void* item = NULL;
    if (list->_iter == list->_list.end())
    {
        list->_iter_idx = -1;
        return NULL;
    }
    if (list->_iter_idx == -1)
        list->_iter = list->_list.begin();
    list->_iter_idx++;
    item = *list->_iter;
    list->_iter++;
    return item;
}



uint64_t dvz_list_count(DvzList* list)
{
    ASSERT(list != NULL);
    return list->_list.size();
}



void dvz_list_destroy(DvzList* list)
{
    if (list != NULL)
    {
        delete list;
    }
}
