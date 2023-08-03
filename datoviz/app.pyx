# cython: c_string_type=unicode, c_string_encoding=ascii

# -------------------------------------------------------------------------------------------------
# Imports
# -------------------------------------------------------------------------------------------------

# from .renderer cimport Renderer
from . cimport _types as tp
from . cimport viewset as vs
from . cimport pixel as px
from . cimport segment as sg
from . cimport app as pt
from . cimport scene as sc
from . cimport request as rq
from . cimport fileio
from libc.stdlib cimport free
from cython.view cimport array
import numpy as np
import logging
from contextlib import contextmanager

cimport numpy as np


logger = logging.getLogger('datoviz')


# -------------------------------------------------------------------------------------------------
# Enums
# -------------------------------------------------------------------------------------------------

WIDTH = 800
HEIGHT = 600


# -------------------------------------------------------------------------------------------------
# Util functions
# -------------------------------------------------------------------------------------------------


# -------------------------------------------------------------------------------------------------
# Visual
# -------------------------------------------------------------------------------------------------

cdef class Visual:
    cdef pt.DvzApp * _c_app
    cdef rq.DvzRequester * _c_rqr

    cdef sc.DvzVisual * _c_visual
    cdef tp.uint32_t _c_count

    cdef np.ndarray _arr_pos
    cdef np.ndarray _arr_color

    def __init__(self, App app, count):
        self._c_app = app._c_app
        self._c_rqr = app._c_rqr

        self._c_count = count

        self._arr_pos = np.zeros((count, 3), dtype=np.float32)
        self._arr_color = np.zeros((count, 4), dtype=np.uint8)

        # TODO: other visuals
        # self._c_visual = px.dvz_pixel(self._c_rqr, 0)
        self._c_visual = sg.dvz_segment(self._c_rqr, 0)
        sg.dvz_segment_alloc(self._c_visual, count)

    def initial(self, np.ndarray[dtype=float, ndim=2] arr):
        # self._arr_pos[:] = arr
        cdef vec3 * data = <vec3*> & arr.data[0]
        sg.dvz_segment_initial(self._c_visual, 0, self._c_count, data, 0)

    def terminal(self, np.ndarray[dtype=float, ndim=2] arr):
        # self._arr_pos[:] = arr
        cdef vec3 * data = <vec3*> & arr.data[0]
        sg.dvz_segment_terminal(self._c_visual, 0, self._c_count, data, 0)

    def linewidth(self, np.ndarray[dtype=float, ndim=1] arr):
        # self._arr_pos[:] = arr
        cdef float * data = <float*> & arr.data[0]
        sg.dvz_segment_linewidth(self._c_visual, 0, self._c_count, data, 0)

    # def position(self, np.ndarray[dtype=float, ndim=2] arr):
    #     self._arr_pos[:] = arr
    #     cdef vec3 * data = <vec3*> & arr.data[0]
    #     px.dvz_pixel_position(self._c_visual, 0, self._c_count, data, 0)

    def color(self, np.ndarray[dtype=tp.uint8_t, ndim=2] arr):
        # self._arr_color[:] = arr
        cdef cvec4 * data = <cvec4*> & arr.data[0]
        # px.dvz_pixel_color(self._c_visual, 0, self._c_count, data, 0)
        sg.dvz_segment_color(self._c_visual, 0, self._c_count, data, 0)


# -------------------------------------------------------------------------------------------------
# View
# -------------------------------------------------------------------------------------------------

cdef class View:
    cdef pt.DvzApp * _c_app
    cdef rq.DvzRequester * _c_rqr
    cdef vs.DvzViewset * _c_viewset
    cdef vs.DvzView * _c_view

    cdef vec2 _c_offset
    cdef vec2 _c_shape

    def __init__(self, Canvas canvas, offset=(0, 0), shape=(0, 0)):
        # self.app = app
        # self.canvas = canvas

        self._c_app = canvas._c_app
        self._c_rqr = canvas._c_rqr
        self._c_viewset = canvas._c_viewset

        cdef vec2 c_offset
        c_offset[0] = offset[0]
        c_offset[1] = offset[1]

        cdef vec2 c_shape
        c_shape[0] = shape[0]
        c_shape[1] = shape[1]

        self._c_offset = c_offset
        self._c_shape = c_shape

        self._c_view = vs.dvz_view(self._c_viewset, c_offset, c_shape)

    def add(self, Visual visual):
        # TODO: other visuals
        px.dvz_view_add(self._c_view, visual._c_visual, 0, visual._c_count, 0, 1, NULL, 0)


# -------------------------------------------------------------------------------------------------
# Canvas
# -------------------------------------------------------------------------------------------------

cdef class Canvas:
    cdef pt.DvzApp * _c_app
    cdef rq.DvzRequester * _c_rqr
    cdef vs.DvzViewset * _c_viewset
    cdef DvzId _c_id

    def __init__(self, App app, DvzId c_id):
        self._c_app = app._c_app
        self._c_rqr = app._c_rqr
        self._c_id = c_id

        self._c_viewset = vs.dvz_viewset(self._c_rqr, self._c_id)

    def view(self, offset=(0, 0), shape=(0, 0)):
        return View(self, offset, shape)

    def build(self):
        vs.dvz_viewset_build(self._c_viewset)


# -------------------------------------------------------------------------------------------------
# Figure
# -------------------------------------------------------------------------------------------------

cdef class Figure:
    cdef pt.DvzApp * _c_app
    cdef rq.DvzRequester * _c_rqr
    cdef sc.DvzFigure * _c_fig
    cdef sc.DvzPanel * _c_panel
    cdef sc.DvzScene * _c_scene

    def __init__(self, Scene scene, int width, int height, int flags):
        self._c_app = scene._c_app
        self._c_scene = scene._c_scene
        self._c_fig = sc.dvz_figure(self._c_scene, width, height, flags)
        # self.width = width
        # self.height = height
        self._c_panel = sc.dvz_panel_default(self._c_fig)
        cdef sc.DvzPanzoom* pz = sc.dvz_panel_panzoom(self._c_app, self._c_panel)

    def visual(self, Visual visual):
        sc.dvz_panel_visual(self._c_panel, visual._c_visual)


# -------------------------------------------------------------------------------------------------
# Scene
# -------------------------------------------------------------------------------------------------

cdef class Scene:
    cdef pt.DvzApp * _c_app
    cdef rq.DvzRequester * _c_rqr
    cdef sc.DvzScene * _c_scene

    def __init__(self, App app):
        self._c_app = app._c_app
        self._c_rqr = app._c_rqr
        self._c_scene = sc.dvz_scene(self._c_rqr)

    def figure(self, int width=WIDTH, int height=HEIGHT, int flags=0):
        # cdef sc.DvzFigure* c_fig = sc.dvz_figure(self._c_scene, width, height, flags)
        return Figure(self, width, height, flags)

    def run(self, int n = 0):
        sc.dvz_scene_run(self._c_scene, self._c_app, n)


# -------------------------------------------------------------------------------------------------
# App
# -------------------------------------------------------------------------------------------------

cdef class App:
    cdef pt.DvzApp * _c_app
    cdef rq.DvzRequester * _c_rqr

    def __init__(self):
        self._c_app = pt.dvz_app(0)
        self._c_rqr = pt.dvz_app_requester(self._c_app)

    def scene(self):
        return Scene(self)

    def canvas(self, int width=WIDTH, int height=HEIGHT, int flags=0):
        # Background color
        cdef cvec4 c_background
        # if background is None:
        c_background[0] = 0
        c_background[1] = 8
        c_background[2] = 18
        c_background[3] = 255
        # else:
        #     assert len(background) == 3
        #     c_background[0] = <uint8_t>int(background[0])
        #     c_background[1] = <uint8_t>int(background[1])
        #     c_background[2] = <uint8_t>int(background[2])
        #     c_background[3] = 255

        cdef rq.DvzRequest req = rq.dvz_create_canvas(self._c_rqr, width, height, c_background, flags)
        # logger.debug(f"create canvas {width}x{height}, id={id:02x}, flags={flags}")
        return Canvas(self, req.id)

    def visual(self, count):
        return Visual(self, count)

    def run(self):
        pt.dvz_app_run(self._c_app, 0)

    def destroy(self):
        pt.dvz_app_destroy(self._c_app)

    def __dealloc__(self):
        self.destroy()


# -------------------------------------------------------------------------------------------------
# Entry-point
# -------------------------------------------------------------------------------------------------

# def main():
#     app = App()

#     n = 50
#     arr = np.zeros(n, dtype=[('pos', 'f4', 3),
#                    ('color', 'u1', 4), ('size', 'f4')])
#     t = np.linspace(-1, 1, n, dtype=np.float32)
#     arr['pos'] = .75 * \
#         np.c_[np.cos(np.pi*t), np.sin(np.pi*t), np.zeros(n)]
#     arr['color'][:] = 255
#     arr['size'][:] = 10.0

#     with app.commands() as cmd:
#         c = cmd.Canvas(width=800, height=600)
#         g = cmd.Graphics(1, flags=3)  # default MVP and viewport
#         vb = cmd.VertexBuffer(arr)
#         g.set_vertex_buffer(vb)

#         with c.record() as r:
#             r.viewport(0, 0, 0, 0)
#             r.draw(g, 0, n)
#     app.run()


# if __name__ == '__main__':
#     main()
