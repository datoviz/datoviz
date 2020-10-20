cimport visky.cyvisky as cv


DEFAULT_WIDTH = 800
DEFAULT_HEIGHT = 800


cdef class App:
    cdef cv.VkyApp* _c_app

    def __cinit__(self):
        self._c_app = cv.vky_create_app(cv.VKY_BACKEND_GLFW, NULL)
        if self._c_app is NULL:
            raise MemoryError()

    def __dealloc__(self):
        if self._c_app is not NULL:
            cv.vky_destroy_app(self._c_app)

    def canvas(self, int width=DEFAULT_WIDTH, int height=DEFAULT_HEIGHT):
        c_canvas = cv.vky_create_canvas(self._c_app, width, height)
        if c_canvas is NULL:
            raise MemoryError()
        c = Canvas()
        c.create(c_canvas)

    def run(self):
        cv.vky_run_app(self._c_app)


cdef class Canvas:
    cdef cv.VkyCanvas* _c_canvas

    cdef create(self, cv.VkyCanvas* c_canvas):
        self._c_canvas = c_canvas
