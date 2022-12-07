# WARNING: parts of this file are auto-generated

from ._types cimport *


cdef extern from "<datoviz/scene/scene.h>":
    ctypedef struct DvzScene:
        pass

    ctypedef struct DvzFigure:
        pass

    ctypedef struct DvzPanel:
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

    # STRUCT END

    # Functions
    # ---------------------------------------------------------------------------------------------

    # FUNCTION START
    DvzScene* dvz_scene()

    DvzFigure* dvz_figure(DvzScene* scene, uint32_t width, uint32_t height, uint32_t n_rows, uint32_t n_cols, int flags)

    DvzPanel* dvz_panel(DvzFigure* fig, uint32_t row, uint32_t col, DvzPanelType type, int flags)

    DvzVisual* dvz_visual(DvzScene* scene, DvzVisualType vtype, int flags)

    void dvz_visual_data(DvzVisual* visual, DvzPropType ptype, uint64_t index, uint64_t count, void* data)

    void dvz_panel_visual(DvzPanel* panel, DvzVisual* visual, int pos)

    void dvz_visual_destroy(DvzVisual* visual)

    void dvz_panel_destroy(DvzPanel* panel)

    void dvz_figure_destroy(DvzFigure* figure)

    void dvz_scene_destroy(DvzScene* scene)


    # FUNCTION END
