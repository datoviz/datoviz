import numpy as np

from visky.wrap import viskylib as vl, upload_data, pointer, array_pointer, get_const
from visky import _constants as const
from visky import _types as tp


class App:
    def __init__(self):
        self._app = vl.vky_create_app(const.BACKEND_GLFW, None)
        self._canvases = []

    def canvas(self):
        c = Canvas(self._app)
        self._canvases.append(c)
        return c

    def run(self):
        vl.vky_run_app(self._app)
        for c in self._canvases:
            vl.vky_destroy_scene(c._scene)
        vl.vky_destroy_app(self._app)


class Canvas:
    def __init__(self, app, shape=(1, 1), width=800, height=600, background=None):
        self._canvas = vl.vky_create_canvas(app, width, height)
        self._scene = vl.vky_create_scene(
            self._canvas, get_const(background, 'white'), shape[0], shape[1])

    def __getitem__(self, shape):
        assert len(shape) == 2
        return Panel(self, row=shape[0], col=shape[1])

    # def on_key(self, f):
    #     @tp.canvas_callback
    #     def callback(p_canvas):
    #         # key = vl.vky_event_key(p_canvas)
    #         # f(key)
    #     vl.vky_add_frame_callback(self._canvas, callback)


class Panel:
    def __init__(self, canvas, row=0, col=0, controller_type=None, params=None):
        self._canvas = canvas._canvas
        self._scene = canvas._scene
        self.row, self.col = row, col
        self._panel = vl.vky_get_panel(self._scene, row, col)
        controller_type = get_const(controller_type, 'controller_axes_2D')
        self.set_controller(controller_type, params=params)
        self._axes = vl.vky_get_axes(self._panel)

    def set_controller(self, controller_type, params=None):
        vl.vky_set_controller(self._panel, controller_type, params)

    def axes_range(self, x0, x1, y0, y1):
        vl.vky_axes_set_range(self._axes, x0, x1, y0, y1)

    def visual(self, visual_type, params=None, obj=None):
        visual_type = get_const(visual_type)
        assert visual_type
        p_visual = vl.vky_visual(self._scene, visual_type, params, obj)
        vl.vky_add_visual_to_panel(
            p_visual, self._panel, get_const('viewport_inner'), get_const('visual_priority_none'))
        cls = Visual
        if visual_type == const.VISUAL_IMAGE:
            cls = Image
        return cls(self, p_visual)

    def image(self, width, height):
        tex_params = vl.vky_default_texture_params(
            tp.T_IVEC3(width, height, 1))
        visual = self.visual('visual_image', pointer(tex_params))

        # Image vertices.
        vertices = np.zeros((1,), dtype=[
            ('p0', 'f4', 3),
            ('p1', 'f4', 3),
            ('uv0', 'f4', 2),
            ('uv1', 'f4', 2)
        ])
        vertices['p0'][0] = (-1, -1, 0)
        vertices['p1'][0] = (+1, +1, 0)
        vertices['uv0'][0] = (0, 1)
        vertices['uv1'][0] = (1, 0)
        visual.upload(vertices)
        return visual


class Visual:
    def __init__(self, panel, p_visual):
        self.panel = panel
        self._scene = panel._scene
        self._visual = p_visual

    def upload(self, vertices):
        upload_data(self._visual, vertices)


class Image(Visual):
    def upload_image(self, image):
        vl.vky_visual_image_upload(self._visual, array_pointer(image))
