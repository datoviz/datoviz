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
    DvzVisual* dvz_pixel(DvzRequester* rqr, int flags)

    void dvz_pixel_position(DvzVisual* pixel, uint32_t first, uint32_t count, vec3* values, int flags)

    void dvz_pixel_color(DvzVisual* pixel, uint32_t first, uint32_t count, cvec4* values, int flags)

    void dvz_pixel_alloc(DvzVisual* pixel, uint32_t item_count)


    # FUNCTION END
