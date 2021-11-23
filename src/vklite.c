/*************************************************************************************************/
/*  Vklite                                                                                       */
/*************************************************************************************************/

#include "vklite.h"
#include "common.h"



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#define CMD_START                                                                                 \
    ASSERT(cmds != NULL);                                                                         \
    VkCommandBuffer cb = {0};                                                                     \
    uint32_t i = idx;                                                                             \
    cb = cmds->cmds[i];


#define CMD_START_CLIP(cnt)                                                                       \
    ASSERT(cmds != NULL);                                                                         \
    ASSERT(cnt > 0);                                                                              \
    if (!((cnt) == 1 || (cnt) == cmds->count))                                                    \
        log_debug("mismatch between image count and cmd buf count");                              \
    VkCommandBuffer cb = {0};                                                                     \
    uint32_t iclip = 0;                                                                           \
    uint32_t i = idx;                                                                             \
    iclip = (cnt) == 1 ? 0 : (MIN(i, (cnt)-1));                                                   \
    ASSERT(iclip < (cnt));                                                                        \
    cb = cmds->cmds[i];


#define CMD_END //
