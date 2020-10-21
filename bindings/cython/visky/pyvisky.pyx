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
    'uv': cv.VKY_VISUAL_PROP_TEXTURE_COORDS,
}

_PROP_NAMES = list(_PROPS.keys())

_VISUALS = {
    'marker': cv.VKY_VISUAL_MARKER,
    'image': cv.VKY_VISUAL_IMAGE,
    'image_cmap': cv.VKY_VISUAL_IMAGE_CMAP,
}

_CONTROLLERS = {
    'axes': cv.VKY_CONTROLLER_AXES_2D,
    'arcball': cv.VKY_CONTROLLER_ARCBALL,
}


def _get_prop(name):
    prop = _PROPS.get(name, None)
    if prop is None:
        raise NotImplementedError("prop %s not implemented yet" % name)
    return prop


def _get_visual(name):
    visual = _VISUALS.get(name, None)
    if visual is None:
        raise NotImplementedError("visual %s not implemented yet" % name)
    return visual


def _get_controller(name):
    controller = _CONTROLLERS.get(name, None)
    if controller is None:
        raise NotImplementedError("controller %s not implemented yet" % name)
    return controller


_APP = None
def app(*args, **kwargs):
    global _APP
    if _APP is None:
        _APP = App(*args, **kwargs)
    assert _APP
    return _APP


def canvas(*args, **kwargs):
    return app().canvas(*args, **kwargs)


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
        c_controller_type = _get_controller(name)
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
        c_visual_type = _get_visual(name)
        c_visual = cv.vky_visual(
            self._c_panel.scene, c_visual_type, NULL, NULL)
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

    def imshow_cmap(self, np.ndarray image):
        cdef cv.VkyImageCmapParams params
        cdef cv.VkyTextureParams tex_params
        tex_params = cv.vky_default_texture_params(
            image.shape[1],
            1 if image.ndim <= 1 else image.shape[0],
            1)
        tex_params.format_bytes = 1
        tex_params.format = cv.VK_FORMAT_R8_UNORM

        params.cmap = cv.VKY_CMAP_VIRIDIS
        params.scaling = 1
        params.alpha = 1
        params.tex_params = &tex_params

        visual = Image()
        c_visual_type = _get_visual('image_cmap')
        c_visual = cv.vky_visual(
            self._c_panel.scene, c_visual_type, &params, NULL)
        visual.create(c_visual)
        cv.vky_add_visual_to_panel(
            c_visual, self._c_panel, cv.VKY_VIEWPORT_INNER, cv.VKY_VISUAL_PRIORITY_NONE)

        visual.data('pos', np.array([-1, -1, 0], dtype=np.float32), idx=0)
        visual.data('pos', np.array([+1, +1, 0], dtype=np.float32), idx=1)
        visual.data('uv', np.array([0, 0], dtype=np.float32), idx=0)
        visual.data('uv', np.array([1, 1], dtype=np.float32), idx=1)
        visual.set_image(image)
        return visual

    def imshow(self, np.ndarray image):
        cdef cv.VkyTextureParams tex_params
        tex_params = cv.vky_default_texture_params(
            image.shape[1],
            1 if image.ndim <= 1 else image.shape[0],
            1)

        visual = Image()
        c_visual_type = _get_visual('image')
        c_visual = cv.vky_visual(
            self._c_panel.scene, c_visual_type, &tex_params, NULL)
        visual.create(c_visual)
        cv.vky_add_visual_to_panel(
            c_visual, self._c_panel, cv.VKY_VIEWPORT_INNER, cv.VKY_VISUAL_PRIORITY_NONE)

        visual.data('pos', np.array([-1, +1, 0], dtype=np.float32), idx=0)
        visual.data('pos', np.array([+1, -1, 0], dtype=np.float32), idx=1)
        visual.data('uv', np.array([0, 0], dtype=np.float32), idx=0)
        visual.data('uv', np.array([1, 1], dtype=np.float32), idx=1)
        visual.set_image(image)
        return visual



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


cdef class Image(Visual):
    def set_image(self, np.ndarray image):
        cv.vky_visual_image_upload(self._c_visual, &image.data[0])
