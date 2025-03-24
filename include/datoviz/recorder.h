/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Recorder                                                                                     */
/*************************************************************************************************/

#ifndef DVZ_HEADER_RECORDER
#define DVZ_HEADER_RECORDER



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_obj.h"
#include "datoviz_types.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_RECORDER_COMMAND_COUNT 16

// HACK: repeats value from vklite.h
#define DVZ_MAX_SWAPCHAIN_IMAGES 4



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    DVZ_RECORDER_FLAGS_NONE = 0x00,
    DVZ_RECORDER_FLAGS_DISABLE_CACHE = 0x01,
} DvzRecorderFlags;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzRecorder DvzRecorder;

// Forward declarations.
typedef uint64_t DvzId;
typedef struct DvzCanvas DvzCanvas;
typedef struct DvzPipe DvzPipe;
typedef struct DvzCommands DvzCommands;
typedef struct DvzRenderer DvzRenderer;

typedef void (*DvzRecorderCallback)(
    DvzRecorder* recorder, DvzRenderer* rd, DvzCommands* cmds, uint32_t img_idx, //
    DvzRecorderCommand* record, void* user_data);



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/


struct DvzRecorder
{
    int flags;
    uint32_t capacity;
    uint32_t count; // number of commands
    DvzRecorderCommand* commands;

    bool dirty[DVZ_MAX_SWAPCHAIN_IMAGES]; // all true initially

    // callbacks for processing the recorder commands, one per recorder type
    DvzRecorderCallback callbacks[DVZ_RECORDER_COUNT];
    void* callback_user_data[DVZ_RECORDER_COUNT];

    void* to_free; // HACK: free push constant once all command buffers have been set
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Recorder functions                                                                           */
/*************************************************************************************************/

DvzRecorder* dvz_recorder(int flags);

void dvz_recorder_clear(DvzRecorder* recorder);

void dvz_recorder_append(DvzRecorder* recorder, DvzRecorderCommand rc);

void dvz_recorder_register(
    DvzRecorder* recorder, DvzRecorderCommandType ctype, DvzRecorderCallback cb, void* user_data);

uint32_t dvz_recorder_size(DvzRecorder* recorder);

void dvz_recorder_set(DvzRecorder* recorder, DvzRenderer* rd, DvzCommands* cmds, uint32_t img_idx);

void dvz_recorder_cache(DvzRecorder* recorder, bool activate);

bool dvz_recorder_is_dirty(DvzRecorder* recorder, uint32_t img_idx);

void dvz_recorder_set_dirty(DvzRecorder* recorder);

void dvz_recorder_destroy(DvzRecorder* recorder);



EXTERN_C_OFF

#endif
