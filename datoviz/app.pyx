# cython: c_string_type=unicode, c_string_encoding=ascii

# -------------------------------------------------------------------------------------------------
# Imports
# -------------------------------------------------------------------------------------------------

# from .renderer cimport Renderer
from . cimport _types as tp
from . cimport app as pt
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
# App
# -------------------------------------------------------------------------------------------------

cdef class App:
    cdef pt.DvzApp * _c_app
    cdef rq.DvzRequester * _c_rqr

    def __init__(self):
        self._c_app = pt.dvz_app()
        self._c_rqr = pt.dvz_app_requester(self._c_app)

    def canvas(self, int width=WIDTH, int height=HEIGHT, int flags=0, DvzId id=0):
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
        # return Canvas(self, id)

    def destroy(self):
        pt.dvz_app_destroy(self._c_app)


# -------------------------------------------------------------------------------------------------
# Entry-point
# -------------------------------------------------------------------------------------------------

def main():
    app = App()

    n = 50
    arr = np.zeros(n, dtype=[('pos', 'f4', 3),
                   ('color', 'u1', 4), ('size', 'f4')])
    t = np.linspace(-1, 1, n, dtype=np.float32)
    arr['pos'] = .75 * \
        np.c_[np.cos(np.pi*t), np.sin(np.pi*t), np.zeros(n)]
    arr['color'][:] = 255
    arr['size'][:] = 10.0

    with app.commands() as cmd:
        c = cmd.Canvas(width=800, height=600)
        g = cmd.Graphics(1, flags=3)  # default MVP and viewport
        vb = cmd.VertexBuffer(arr)
        g.set_vertex_buffer(vb)

        with c.record() as r:
            r.viewport(0, 0, 0, 0)
            r.draw(g, 0, n)
    app.run()


if __name__ == '__main__':
    main()
