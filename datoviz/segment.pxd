# WARNING: parts of this file are auto-generated

from ._types cimport *
from .viewset cimport *


cdef extern from "<datoviz/scene/visuals/segment.h>":

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
    DvzVisual* dvz_segment(DvzRequester* rqr, int flags)

    void dvz_segment_initial(DvzVisual* segment, uint32_t first, uint32_t count, vec3* values, int flags)

    void dvz_segment_terminal(DvzVisual* segment, uint32_t first, uint32_t count, vec3* values, int flags)

    void dvz_segment_shift(DvzVisual* segment, uint32_t first, uint32_t count, vec4* values, int flags)

    void dvz_segment_color(DvzVisual* segment, uint32_t first, uint32_t count, cvec4* values, int flags)

    void dvz_segment_linewidth(DvzVisual* segment, uint32_t first, uint32_t count, float* values, int flags)

    void dvz_segment_initial_cap(DvzVisual* segment, uint32_t first, uint32_t count, DvzCapType* values, int flags)

    void dvz_segment_terminal_cap(DvzVisual* segment, uint32_t first, uint32_t count, DvzCapType* values, int flags)

    void dvz_segment_alloc(DvzVisual* segment, uint32_t item_count)


    # FUNCTION END
