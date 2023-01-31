/*************************************************************************************************/
/*  Recorder                                                                                     */
/*************************************************************************************************/

#ifndef DVZ_HEADER_RECORDER
#define DVZ_HEADER_RECORDER



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_obj.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_RECORDER_COMMAND_COUNT 16
#define DVZ_MAX_SWAPCHAIN_IMAGES   4



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    DVZ_RECORDER_NONE,
    DVZ_RECORDER_BEGIN,
    DVZ_RECORDER_DRAW,
    DVZ_RECORDER_DRAW_INDEXED,
    DVZ_RECORDER_DRAW_INDIRECT,
    DVZ_RECORDER_DRAW_INDEXED_INDIRECT,
    DVZ_RECORDER_VIEWPORT,
    DVZ_RECORDER_END,
} DvzRecorderCommandType;



typedef enum
{
    DVZ_RECORDER_FLAGS_NONE = 0x00,
    DVZ_RECORDER_FLAGS_DISABLE_CACHE = 0x01,
} DvzRecorderFlags;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzRecorderCommand DvzRecorderCommand;
typedef struct DvzRecorder DvzRecorder;

// Forward declarations.
typedef uint64_t DvzId;
typedef struct DvzCanvas DvzCanvas;
typedef struct DvzPipe DvzPipe;
typedef struct DvzCommands DvzCommands;
typedef struct DvzRenderer DvzRenderer;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzRecorderCommand
{
    DvzRecorderCommandType type;
    DvzId canvas_id;
    union
    {
        // // Begin recording for canvas.
        // struct
        // {
        //     DvzCanvas* canvas;
        // } c;

        // Viewport.
        struct
        {
            vec2 offset, shape; // in framebuffer pixels
        } v;

        struct
        {
            DvzId pipe_id;
            uint32_t first_vertex, vertex_count;
            uint32_t first_instance, instance_count;
        } draw;

        struct
        {
            DvzId pipe_id;
            uint32_t first_index, vertex_offset, index_count;
            uint32_t first_instance, instance_count;
        } draw_indexed;

        struct
        {
            DvzId pipe_id;
            DvzId dat_indirect_id;
        } draw_indirect;

        struct
        {
            DvzId pipe_id;
            DvzId dat_indirect_id;
        } draw_indexed_indirect;
    } contents;
};



struct DvzRecorder
{
    int flags;
    uint32_t capacity;
    uint32_t count; // number of commands
    DvzRecorderCommand* commands;
    bool dirty[DVZ_MAX_SWAPCHAIN_IMAGES]; // all true initially
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Recorder functions                                                                           */
/*************************************************************************************************/

DVZ_EXPORT DvzRecorder* dvz_recorder(int flags);

DVZ_EXPORT void dvz_recorder_clear(DvzRecorder* recorder);

DVZ_EXPORT void dvz_recorder_append(DvzRecorder* recorder, DvzRecorderCommand rc);

DVZ_EXPORT uint32_t dvz_recorder_size(DvzRecorder* recorder);

DVZ_EXPORT void
dvz_recorder_set(DvzRecorder* recorder, DvzRenderer* rd, DvzCommands* cmds, uint32_t img_idx);

DVZ_EXPORT void dvz_recorder_cache(DvzRecorder* recorder, bool activate);

DVZ_EXPORT bool dvz_recorder_is_dirty(DvzRecorder* recorder, uint32_t img_idx);

DVZ_EXPORT void dvz_recorder_set_dirty(DvzRecorder* recorder);

DVZ_EXPORT void dvz_recorder_destroy(DvzRecorder* recorder);



EXTERN_C_OFF

#endif
