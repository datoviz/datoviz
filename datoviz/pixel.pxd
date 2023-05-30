# WARNING: parts of this file are auto-generated

from ._types cimport *
from .viewset cimport *


cdef extern from "<datoviz/scene/visuals/pixel.h>":
    # ctypedef struct DvzVisual:
    #     pass

    # ctypedef struct DvzRequester:
    #     pass


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
    DvzVisual* dvz_pixel(DvzRequester* rqr, uint32_t item_count, int flags)

    void dvz_pixel_viewport(DvzVisual* pixel, DvzViewport viewport)

    void dvz_pixel_position(DvzVisual* pixel, uint32_t first, uint32_t count, vec3* values, int flags)

    void dvz_pixel_color(DvzVisual* pixel, uint32_t first, uint32_t count, cvec4* values, int flags)

    DvzInstance* dvz_pixel_instance(DvzVisual* pixel, DvzView* view, uint32_t first, uint32_t count)

    void dvz_pixel_create(DvzVisual* pixel)

    void dvz_pixel_update(DvzVisual* pixel)


    # FUNCTION END
