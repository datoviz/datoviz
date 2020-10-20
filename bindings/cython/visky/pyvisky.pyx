cimport visky.cyvisky as cv


DEFAULT_WIDTH = 800
DEFAULT_HEIGHT = 800


cdef class App:
    cdef cv.VkyApp* _c_app

    def __cinit__(self):
        cv.log_set_level_env()
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
        return c

    def run(self):
        cv.vky_run_app(self._c_app)


cdef class Canvas:
    cdef cv.VkyCanvas* _c_canvas

    cdef create(self, cv.VkyCanvas* c_canvas):
        self._c_canvas = c_canvas

    def scene(self, int rows=1, int cols=1):
        cdef cv.VkyColor clear_color
        clear_color.rgb[0] = 255
        clear_color.rgb[1] = 255
        clear_color.rgb[2] = 255
        clear_color.alpha = 255

        c_scene = cv.vky_create_scene(self._c_canvas, clear_color, rows, cols)
        if c_scene is NULL:
            raise MemoryError()
        s = Scene()
        s.create(c_scene)
        return s


cdef class Scene:
    cdef cv.VkyScene* _c_scene

    cdef create(self, cv.VkyScene* c_scene):
        self._c_scene = c_scene
