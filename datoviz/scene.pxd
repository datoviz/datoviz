# WARNING: parts of this file are auto-generated

from ._types cimport *


cdef extern from "<datoviz/scene/scene.h>":
    ctypedef struct DvzScene:
        pass

    ctypedef struct DvzFigure:
        pass

    ctypedef struct DvzPanel:
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

    # STRUCT END

    # Functions
    # ---------------------------------------------------------------------------------------------

    # FUNCTION START
    DvzScene* dvz_scene()

    DvzFigure* dvz_figure(DvzScene* scene, uint32_t width, uint32_t height, uint32_t n_rows, uint32_t n_cols, int flags)

    void dvz_figure_destroy(DvzFigure* figure)

    void dvz_scene_destroy(DvzScene* scene)


    # FUNCTION END
