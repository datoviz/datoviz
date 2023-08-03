# WARNING: parts of this file are auto-generated

from ._types cimport *


cdef extern from "<datoviz/scene/scene.h>":
    ctypedef struct DvzScene:
        pass

    ctypedef struct DvzRequester:
        pass

    ctypedef struct DvzView:
        pass

    ctypedef struct DvzObject:
        pass

    ctypedef struct DvzList:
        pass

    ctypedef struct DvzTransform:
        pass

    ctypedef struct DvzBaker:
        pass

    ctypedef struct DvzParams:
        pass

    ctypedef struct DvzVisual:
        pass

    ctypedef struct DvzFigure:
        pass

    ctypedef struct DvzApp:
        pass

    ctypedef struct DvzScene:
        pass

    ctypedef struct DvzPanel:
        pass

    ctypedef struct DvzCamera:
        pass

    ctypedef struct DvzPanzoom:
        pass

    ctypedef struct DvzArcball:
        pass

    ctypedef struct DvzMVP:
        pass

    ctypedef struct DvzViewport:
        pass

    ctypedef struct DvzMouse:
        pass

    ctypedef struct DvzMouseReference:
        pass

    ctypedef struct DvzMouseEvent:
        pass

    ctypedef struct DvzKeyboard:
        pass

    ctypedef struct DvzKeyboardEvent:
        pass

    ctypedef void (*DvzVisualCallback)(DvzVisual* visual, DvzId canvas,
    uint32_t first, uint32_t count, uint32_t first_instance, uint32_t instance_count)

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
    DvzScene* dvz_scene(DvzRequester* rqr)

    void dvz_scene_destroy(DvzScene* scene)

    DvzFigure* dvz_figure(DvzScene* scene, uint32_t width, uint32_t height, int flags)

    void dvz_figure_resize(DvzFigure* fig, uint32_t width, uint32_t height)

    DvzFigure* dvz_scene_figure(DvzScene* scene, DvzId id)

    void dvz_figure_destroy(DvzFigure* figure)

    DvzPanel* dvz_panel(DvzFigure* fig, float x, float y, float w, float h)

    DvzPanel* dvz_panel_default(DvzFigure* fig)

    void dvz_panel_transform(DvzPanel* panel, DvzTransform* tr)

    void dvz_panel_resize(DvzPanel* panel, float x, float y, float width, float height)

    bint dvz_panel_contains(DvzPanel* panel, vec2 pos)

    DvzPanel* dvz_panel_at(DvzFigure* figure, vec2 pos)

    void dvz_panel_destroy(DvzPanel* panel)

    DvzCamera* dvz_panel_camera(DvzPanel* panel)

    DvzPanzoom* dvz_panel_panzoom(DvzApp* app, DvzPanel* panel)

    DvzArcball* dvz_panel_arcball(DvzApp* app, DvzPanel* panel)

    void dvz_panel_visual(DvzPanel* panel, DvzVisual* visual)

    void dvz_scene_run(DvzScene* scene, DvzApp* app, uint64_t n_frames)

    DvzVisual* dvz_visual(DvzRequester* rqr, DvzPrimitiveTopology primitive, int flags)

    void dvz_visual_update(DvzVisual* visual)

    void dvz_visual_destroy(DvzVisual* visual)

    void dvz_visual_primitive(DvzVisual* visual, DvzPrimitiveTopology primitive)

    void dvz_visual_blend(DvzVisual* visual, DvzBlendType blend_type)

    void dvz_visual_depth(DvzVisual* visual, DvzDepthTest depth_test)

    void dvz_visual_polygon(DvzVisual* visual, DvzPolygonMode polygon_mode)

    void dvz_visual_cull(DvzVisual* visual, DvzCullMode cull_mode)

    void dvz_visual_front(DvzVisual* visual, DvzFrontFace front_face)

    void dvz_visual_spirv(DvzVisual* visual, DvzShaderType type, DvzSize size, const char* buffer)

    void dvz_visual_shader(DvzVisual* visual, const char* name)

    void dvz_visual_resize(DvzVisual* visual, uint32_t item_count, uint32_t vertex_count)

    void dvz_visual_groups(DvzVisual* visual, uint32_t group_count, uint32_t* group_sizes)

    void dvz_visual_attr(DvzVisual* visual, uint32_t attr_idx, DvzSize offset, DvzSize item_size, DvzFormat format, int flags)

    void dvz_visual_stride(DvzVisual* visual, uint32_t binding_idx, DvzSize stride)

    void dvz_visual_slot(DvzVisual* visual, uint32_t slot_idx, DvzSlotType type)

    void dvz_visual_params(DvzVisual* visual, uint32_t slot_idx, DvzParams* params)

    void dvz_visual_tex(DvzVisual* visual, uint32_t slot_idx, DvzId tex, DvzId sampler, uvec3 offset)

    void dvz_visual_alloc(DvzVisual* visual, uint32_t item_count, uint32_t vertex_count, uint32_t index_count)

    void dvz_visual_mvp(DvzVisual* visual, DvzMVP* mvp)

    void dvz_visual_viewport(DvzVisual* visual, DvzViewport* viewport)

    void dvz_visual_data(DvzVisual* visual, uint32_t attr_idx, uint32_t first, uint32_t count, void* data)

    void dvz_visual_quads(DvzVisual* visual, uint32_t attr_idx, uint32_t first, uint32_t count, vec2 quad_size, vec2* positions)

    void dvz_visual_index(DvzVisual* visual, uint32_t first, uint32_t count, DvzIndex* data)

    void dvz_visual_instance(DvzVisual* visual, DvzId canvas, uint32_t first, uint32_t vertex_offset, uint32_t count, uint32_t first_instance, uint32_t instance_count)

    void dvz_visual_indirect(DvzVisual* visual, DvzId canvas, uint32_t draw_count)

    void dvz_visual_record(DvzVisual* visual, DvzId canvas)

    void dvz_visual_callback(DvzVisual* visual, DvzVisualCallback callback)

    void dvz_visual_visible(DvzVisual* visual, bint is_visible)


    # FUNCTION END
