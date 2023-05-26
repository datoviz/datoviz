# WARNING: parts of this file are auto-generated

from ._types cimport *


cdef extern from "<datoviz/scene/viewset.h>":
    ctypedef struct DvzViewset:
        pass

    ctypedef struct DvzRequester:
        pass

    ctypedef struct DvzView:
        pass

    ctypedef struct DvzInstance:
        pass

    ctypedef struct DvzVisual:
        pass


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
    ctypedef struct _VkViewport:
        float x
        float y
        float width
        float height
        float minDepth
        float maxDepth

    ctypedef struct DvzViewport:
        _VkViewport viewport
        vec4 margins
        uvec2 offset_screen
        uvec2 size_screen
        uvec2 offset_framebuffer
        uvec2 size_framebuffer
        DvzViewportClip clip
        int32_t interact_axis


    # STRUCT END

    # Functions
    # ---------------------------------------------------------------------------------------------

    # FUNCTION START
    DvzViewport dvz_viewport(vec2 offset, vec2 shape)

    DvzViewport dvz_viewport_default(uint32_t width, uint32_t height)

    DvzViewset* dvz_viewset(DvzRequester* rqr, DvzId canvas_id)

    void dvz_viewset_clear(DvzViewset* viewset)

    void dvz_viewset_build(DvzViewset* viewset)

    void dvz_viewset_destroy(DvzViewset* viewset)

    DvzView* dvz_view(DvzViewset* viewset, vec2 offset, vec2 shape)

    void dvz_view_clear(DvzView* view)

    void dvz_view_resize(DvzView* view, vec2 offset, vec2 shape)

    void dvz_view_destroy(DvzView* view)

    DvzInstance* dvz_view_instance(DvzView* view, DvzVisual* visual, uint32_t first, uint32_t vertex_offset, uint32_t count, uint32_t first_instance, uint32_t instance_count)


    # FUNCTION END
