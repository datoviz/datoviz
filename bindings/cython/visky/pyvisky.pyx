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

    def panel(self, int row=0, int col=0):
        c_panel = cv.vky_get_panel(self._c_scene, row, col)
        if c_panel is NULL:
            raise MemoryError()
        p = Panel()
        p.create(c_panel, self._c_scene)
        return p


cdef class Panel:
    cdef cv.VkyPanel* _c_panel
    cdef cv.VkyScene* _c_scene

    cdef create(self, cv.VkyPanel* c_panel, cv.VkyScene* c_scene):
        self._c_panel = c_panel
        self._c_scene = c_scene

    def set_controller(self, str controller_type='axes'):
        c_controller_type = cv.VKY_CONTROLLER_NONE
        if controller_type == 'axes':
            c_controller_type = cv.VKY_CONTROLLER_AXES_2D
        cv.vky_set_controller(self._c_panel, c_controller_type, NULL)

    def visual(self, str visual_type):
        if visual_type == 'marker':
            c_visual_type = cv.VKY_VISUAL_MARKER
        c_visual = cv.vky_visual(self._c_scene, c_visual_type, NULL, NULL)
        cv.vky_add_visual_to_panel(c_visual, self._c_panel, cv.VKY_VIEWPORT_INNER, cv.VKY_VISUAL_PRIORITY_NONE);
        visual = Visual()
        visual.create(c_visual)
        return visual


cdef class Visual:
    cdef cv.VkyVisual* _c_visual

    cdef create(self, cv.VkyVisual* c_visual):
        self._c_visual = c_visual
        if c_visual is NULL:
            raise MemoryError()

    def data(self, str prop):
        # TODO
        pass
