# WARNING: parts of this file are auto-generated

from ._types cimport *


cdef extern from "<datoviz/scene/app.h>":
    ctypedef struct DvzApp:
        pass

    ctypedef struct DvzList:
        pass

    ctypedef struct DvzDevice:
        pass

    ctypedef struct DvzRequester:
        pass

    ctypedef struct DvzRequest:
        pass

    # ctypedef struct DvzVisual:
    #     pass

    # ctypedef struct DvzInstance:
    #     pass

    # ctypedef struct DvzView:
    #     pass

    # ctypedef struct DvzViewset:
    #     pass

    # ctypedef struct DvzScene:
    #     pass

    ctypedef struct DvzClient:
        pass

    ctypedef struct DvzTimerItem:
        pass

    ctypedef struct DvzMouse:
        pass

    ctypedef struct DvzKeyboard:
        pass

    ctypedef void (*DvzClientCallback)(DvzClient*, DvzClientEvent)
    ctypedef void (*DvzMouseCallback)(DvzMouse*, DvzMouseEvent)
    ctypedef void (*DvzKeyboardCallback)(DvzKeyboard*, DvzKeyboardEvent)

    # ---------------------------------------------------------------------------------------------
    # ---------------------------------------------------------------------------------------------
    # ---------------------------------------------------------------------------------------------
    # !!!!!!!!!!!!!!!!!!!! AUTOMATICALLY-GENERATED PART: DO NOT EDIT MANUALLY !!!!!!!!!!!!!!!!!!!!
    # ---------------------------------------------------------------------------------------------
    # ---------------------------------------------------------------------------------------------
    # ---------------------------------------------------------------------------------------------

    # Structures
    # ---------------------------------------------------------------------------------------------

    # STRUCT START
    ctypedef struct DvzKeyboardEvent:
        DvzKeyboardEventType type
        DvzKeyCode key
        int mods
        void* user_data

    ctypedef struct DvzMouseMoveEvent:
        vec2 pos

    ctypedef struct DvzMouseButtonEvent:
        DvzMouseButton button

    ctypedef struct DvzMouseWheelEvent:
        vec2 pos
        vec2 dir

    ctypedef struct DvzMouseDragEvent:
        DvzMouseButton button
        vec2 pos
        vec2 press_pos
        vec2 shift

    ctypedef struct DvzMouseClickEvent:
        DvzMouseButton button
        vec2 pos

    ctypedef union DvzMouseEventUnion:
        DvzMouseMoveEvent m
        DvzMouseButtonEvent b
        DvzMouseWheelEvent w
        DvzMouseDragEvent d
        DvzMouseClickEvent c

    ctypedef struct DvzMouseEvent:
        DvzMouseEventType type
        DvzMouseEventUnion content
        int mods
        void* user_data

    ctypedef struct DvzClientWindowEvent:
        uint32_t framebuffer_width
        uint32_t framebuffer_height
        uint32_t screen_width
        uint32_t screen_height
        int flags

    ctypedef struct DvzClientFrameEvent:
        uint64_t frame_idx
        double time

    ctypedef struct DvzClientTimerEvent:
        uint32_t timer_idx
        DvzTimerItem* timer_item
        uint64_t step_idx
        double time

    ctypedef struct DvzClientRequestsEvent:
        uint32_t request_count
        void* requests

    ctypedef union DvzClientEventUnion:
        DvzClientWindowEvent w
        DvzClientFrameEvent f
        DvzClientTimerEvent t
        DvzClientRequestsEvent r
        DvzMouseEvent m
        DvzKeyboardEvent k

    ctypedef struct DvzClientEvent:
        DvzClientEventType type
        DvzId window_id
        DvzClientEventUnion content
        void* user_data

    ctypedef struct DvzClientPayload:
        DvzClient* client
        DvzClientCallback callback
        DvzClientCallbackMode mode
        void* user_data

    ctypedef struct DvzKeyboardPayload:
        DvzKeyboardEventType type
        DvzKeyboardCallback callback
        void* user_data

    ctypedef struct DvzKeyboard:
        DvzList* keys
        int mods
        DvzList* callbacks
        bint is_active

    ctypedef struct DvzMousePayload:
        DvzMouseEventType type
        DvzMouseCallback callback
        void* user_data


    # STRUCT END

    # Functions
    # ---------------------------------------------------------------------------------------------

    # FUNCTION START
    DvzApp* dvz_app()

    DvzRequester* dvz_app_requester(DvzApp* app)

    void dvz_app_frame(DvzApp* app)

    void dvz_app_onmouse(DvzApp* app, DvzClientCallback on_mouse, void* user_data)

    void dvz_app_onkeyboard(DvzApp* app, DvzClientCallback on_keyboard, void* user_data)

    void dvz_app_onresize(DvzApp* app, DvzClientCallback on_resize, void* user_data)

    DvzTimerItem* dvz_app_timer(DvzApp* app, double delay, double period, uint64_t max_count)

    void dvz_app_ontimer(DvzApp* app, DvzClientCallback on_timer, void* user_data)

    void dvz_app_run(DvzApp* app, uint64_t n_frames)

    void dvz_app_destroy(DvzApp* app)


    # FUNCTION END
