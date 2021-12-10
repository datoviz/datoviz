# cython: c_string_type=unicode, c_string_encoding=ascii

# -------------------------------------------------------------------------------------------------
# Imports
# -------------------------------------------------------------------------------------------------

from . cimport renderer as rd
import logging


logger = logging.getLogger('datoviz')


# -------------------------------------------------------------------------------------------------
# Renderer
# -------------------------------------------------------------------------------------------------

cdef class Renderer:
    cdef rd.DvzRenderer * _c_rd
    cdef rd.DvzGpu * _c_gpu

    def __cinit__(self):
        self._c_gpu = rd.dvz_init_offscreen()
        self._c_rd = rd.dvz_renderer_offscreen(self._c_gpu)

    def __dealloc__(self):
        self.destroy()

    def destroy(self):
        """Destroy the renderer."""
        rd.dvz_renderer_destroy(self._c_rd)
        rd.dvz_host_destroy(self._c_gpu.host)
