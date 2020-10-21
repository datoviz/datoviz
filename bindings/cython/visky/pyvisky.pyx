import atexit

cimport numpy as np
import numpy as np

cimport visky.cyvisky as cv


DEFAULT_WIDTH = 800
DEFAULT_HEIGHT = 800


_PROPS = {
    'pos': cv.VKY_VISUAL_PROP_POS_GPU,
    'color': cv.VKY_VISUAL_PROP_COLOR,
    'size': cv.VKY_VISUAL_PROP_SIZE,
}

_PROP_NAMES = list(_PROPS.keys())

def _get_prop(name):
    prop = _PROPS.get(name, None)
    if prop is None:
        raise NotImplementedError("prop %s not implemented yet" % prop)
    return prop


_APP = None
def app():
    global _APP
    if _APP is None:
        _APP = App()
    assert _APP
    return _APP


def canvas():
    return app().canvas()


def run():
    app().run()


@atexit.register
def destroy():
    global _APP
    if _APP:
        del _APP
    _APP = None



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

    def canvas(self, int rows=1, int cols=1, int width=DEFAULT_WIDTH, int height=DEFAULT_HEIGHT):
        c_canvas = cv.vky_create_canvas(self._c_app, width, height)
        if c_canvas is NULL:
            raise MemoryError()
        c = Canvas()
        c.create(c_canvas, rows, cols)
        return c

    def run(self):
        cv.vky_run_app(self._c_app)


cdef class Canvas:
    cdef cv.VkyCanvas* _c_canvas
    cdef cv.VkyScene* _c_scene

    cdef create(self, cv.VkyCanvas* c_canvas, int rows, int cols):
        self._c_canvas = c_canvas

        # TODO: customizable clear color
        cdef cv.VkyColor clear_color
        clear_color.rgb[0] = 255
        clear_color.rgb[1] = 255
        clear_color.rgb[2] = 255
        clear_color.alpha = 255

        self._c_scene = cv.vky_create_scene(self._c_canvas, clear_color, rows, cols)
        if self._c_scene is NULL:
            raise MemoryError()

    def panel(self, int row=0, int col=0):
        c_panel = cv.vky_get_panel(self._c_scene, row, col)
        if c_panel is NULL:
            raise MemoryError()
        p = Panel()
        p.create(c_panel)
        return p

    def __getitem__(self, idx):
        if type(idx) == int:
            idx = (idx, idx)
        if len(idx) == 2:
            return self.panel(int(idx[0]), int(idx[1]))
        raise ValueError("panel idx is invalid %s" % str(idx))



cdef class Panel:
    cdef cv.VkyPanel* _c_panel

    cdef create(self, cv.VkyPanel* c_panel):
        self._c_panel = c_panel

    def controller(self, str name='axes'):
        c_controller_type = cv.VKY_CONTROLLER_NONE
        if name == 'axes':
            c_controller_type = cv.VKY_CONTROLLER_AXES_2D
        elif name == 'arcball':
            c_controller_type = cv.VKY_CONTROLLER_ARCBALL
        cv.vky_set_controller(self._c_panel, c_controller_type, NULL)

    def axes(self):
        self.controller('axes')
        return self

    @property
    def row_col(self):
        index = cv.vky_get_panel_index(self._c_panel)
        return (index.row, index.col)

    @property
    def row(self):
        index = cv.vky_get_panel_index(self._c_panel)
        return index.row

    @property
    def col(self):
        index = cv.vky_get_panel_index(self._c_panel)
        return index.col

    def visual(self, str name, **kwargs):
        if name == 'marker':
            c_visual_type = cv.VKY_VISUAL_MARKER
        # TODO: visual params as kwargs.pop()
        c_visual = cv.vky_visual(self._c_panel.scene, c_visual_type, NULL, NULL)
        visual = Visual()
        visual.create(c_visual)
        cv.vky_add_visual_to_panel(
            c_visual, self._c_panel, cv.VKY_VIEWPORT_INNER, cv.VKY_VISUAL_PRIORITY_NONE)
        # Set data, if any.
        if set(kwargs.keys()).intersection(set(_PROP_NAMES)):
            visual.data(**{n: v for n, v in kwargs.items() if n in _PROP_NAMES})
        return visual

    def markers(self, **kwargs):
        return self.visual('marker', **kwargs)


cdef class Visual:
    cdef cv.VkyVisual* _c_visual
    cdef int item_count

    cdef create(self, cv.VkyVisual* c_visual):
        self._c_visual = c_visual
        if c_visual is NULL:
            raise MemoryError()

    def set_size(self, int item_count):
        # TODO: groups
        if self.item_count == item_count:
            return
        cv.vky_visual_data_set_size(self._c_visual, item_count, 1, NULL, NULL)
        self.item_count = item_count

    def data(self, str name=None, np.ndarray values=None, int idx=0, **kwargs):
        if kwargs:
            for name, values in kwargs.items():
                self.data(name=name, values=values)
            return
        prop = _get_prop(name)
        assert prop
        item_count = values.shape[0]
        self.set_size(item_count)
        cdef void* buf = &(values.data[0])
        cv.vky_visual_data(
            self._c_visual, prop, idx, item_count, buf)
