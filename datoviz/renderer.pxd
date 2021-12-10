# WARNING: parts of this file are auto-generated

from ._types cimport *

cdef extern from "<datoviz/renderer.h>":
    # Semi-opaque structs:

    ctypedef struct DvzRequest:
        pass

    ctypedef struct DvzRenderer:
        pass

    ctypedef struct VkPhysicalDeviceFeatures:
        pass

    ctypedef struct DvzHost:
        pass

    ctypedef struct DvzGpu:
        DvzHost * host

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

    # STRUCT END

    # Functions
    # ---------------------------------------------------------------------------------------------

    # FUNCTION START
    DvzGpu* dvz_init_offscreen()

    DvzGpu* dvz_init_glfw()

    DvzHost* dvz_host(DvzBackend backend)

    void dvz_host_wait(DvzHost* host)

    int dvz_host_destroy(DvzHost* host)

    DvzRenderer* dvz_renderer_offscreen(DvzGpu* gpu)

    DvzRenderer* dvz_renderer_glfw(DvzGpu* gpu)

    void dvz_renderer_request(DvzRenderer* rd, DvzRequest req)

    void dvz_renderer_requests(DvzRenderer* rd, uint32_t count, DvzRequest* reqs)

    uint8_t* dvz_renderer_image(DvzRenderer* rd, DvzId board_id, DvzSize* size, uint8_t* rgba)

    void dvz_renderer_destroy(DvzRenderer* rd)


    # FUNCTION END
