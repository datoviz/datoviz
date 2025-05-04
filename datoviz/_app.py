"""
Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
Licensed under the MIT license. See LICENSE file in the project root for details.
SPDX-License-Identifier: MIT
"""

# App

# -------------------------------------------------------------------------------------------------
# Imports
# -------------------------------------------------------------------------------------------------

import typing as tp

import numpy as np

from . import _ctypes as dvz
from . import _constants as cst
from . import visuals as vs
from ._event import Event
from ._texture import Texture
from ._figure import Figure
from .utils import mesh_flags, to_enum,  dtype_to_format
from .shape_collection import ShapeCollection


# -------------------------------------------------------------------------------------------------
# App
# -------------------------------------------------------------------------------------------------

class App:
    c_flags: int = 0
    c_app: dvz.DvzApp = None
    c_batch: dvz.DvzBatch = None
    c_scene: dvz.DvzScene = None

    def __init__(self, c_flags: int = 0, offscreen: bool = False, background: str = None):
        if offscreen:
            c_flags |= dvz.APP_FLAGS_OFFSCREEN

        # HACK: this will change in the next version
        if background == 'white':
            c_flags |= dvz.APP_FLAGS_WHITE_BACKGROUND

        self.c_flags = c_flags
        self.c_app = dvz.app(c_flags)
        self.c_batch = dvz.app_batch(self.c_app)
        self.c_scene = dvz.scene(self.c_batch)

        # NOTE: keep a reference to callbacks defined inside functions to avoid them being
        # garbage-collected, resulting in a segfault.
        self._callbacks = []

    def figure(self, width: int = cst.DEFAULT_WIDTH, height: int = cst.DEFAULT_HEIGHT, c_flags: int = 0, gui: bool = False):
        if gui:
            c_flags |= dvz.CANVAS_FLAGS_IMGUI
        c_figure = dvz.figure(self.c_scene, width, height, c_flags)
        return Figure(c_figure)

    def on_timer(self):
        pass

    def on_frame(self):
        pass

    def run(self, frame_count: int = 0):
        dvz.scene_run(self.c_scene, self.c_app, frame_count)

    def screenshot(self, figure: Figure, png_path: str):
        self.run(1)
        dvz.app_screenshot(self.c_app, figure.figure_id(), png_path)

    def __del__(self):
        self.destroy()

    def destroy(self):
        if self.c_app is not None:
            dvz.scene_destroy(self.c_scene)
            dvz.app_destroy(self.c_app)
            self.c_app = None

    # Textures
    # ---------------------------------------------------------------------------------------------

    def texture(
        self,
        image: np.ndarray = None,
        ndim: int = 2,
        shape: tuple = None,
        n_channels: int = None,
        dtype: np.dtype = None,
        interpolation: str = None,
        address_mode: str = None,
    ):
        if image is not None:
            if image.ndim == 4:
                ndim = 3
            # NOTE: ambiguity if image.ndim == 3, may be 2D rgba or 3D single channel
            shape = shape or image.shape[:ndim]
            n_channels = n_channels or (image.shape[-1] if ndim == image.ndim - 1 else 1)
            dtype = dtype or image.dtype
            assert 0 <= image.ndim - ndim <= 1

        assert n_channels > 0
        c_format = dtype_to_format(dtype.name, n_channels)
        shape = dvz.uvec3(*shape)
        width, height, depth = shape

        interpolation = interpolation or cst.DEFAULT_INTERPOLATION
        c_filter = to_enum(f'filter_{interpolation}')
        address_mode = address_mode or cst.DEFAULT_ADDRESS_MODE
        c_address_mode = to_enum(f'sampler_address_mode_{address_mode}')

        if ndim == 1:
            c_texture = dvz.texture_1D(
                self.c_batch, c_format, c_filter, c_address_mode, width, image, 0)
        elif ndim == 2:
            c_texture = dvz.texture_2D(
                self.c_batch, c_format, c_filter, c_address_mode, width, height, image, 0)
        elif ndim == 3:
            c_texture = dvz.texture_3D(
                self.c_batch, c_format, c_filter, c_address_mode, width, height, depth, image, 0)

        return Texture(c_texture)

    def texture_1D(
        self, data: np.ndarray,
        interpolation: str = None,
        address_mode: str = None,
    ):
        return self.texture(data, ndim=1, interpolation=interpolation, address_mode=address_mode)

    def texture_2D(
        self, image: np.ndarray,
        interpolation: str = None,
        address_mode: str = None,
    ):
        return self.texture(image, ndim=2, interpolation=interpolation, address_mode=address_mode)

    def texture_3D(
        self, volume: np.ndarray,
        shape: tuple = None,
        interpolation: str = None,
        address_mode: str = None,
    ):
        return self.texture(
            volume, ndim=3, shape=shape, interpolation=interpolation, address_mode=address_mode)

    # Visuals
    # ---------------------------------------------------------------------------------------------

    # TODO: put the actual keywords instead of kwargs in the signatures, and write the docstrings

    def _visual(self, fn: tp.Callable = None, cls: tp.Type = None, c_visual=None, c_flags: int = 0, **kwargs):
        c_visual = c_visual or fn(self.c_batch, c_flags)
        visual = cls(c_visual)
        visual.set_data(**kwargs)
        return visual

    def basic(self, topology: str = None, **kwargs):
        c_topology = to_enum(f'primitive_topology_{topology}')
        assert c_topology
        c_visual = dvz.basic(self.c_batch, c_topology, 0)
        return self._visual(cls=vs.Basic, c_visual=c_visual, **kwargs)

    def pixel(self, **kwargs):
        return self._visual(dvz.pixel, vs.Pixel, **kwargs)

    def point(self, **kwargs):
        return self._visual(dvz.point, vs.Point, **kwargs)

    def marker(self, **kwargs):
        return self._visual(dvz.marker, vs.Marker, **kwargs)

    def segment(self, **kwargs):
        return self._visual(dvz.segment, vs.Segment, **kwargs)

    def path(self, **kwargs):
        return self._visual(dvz.path, vs.Path, **kwargs)

    def glyph(self, font_size: int = cst.DEFAULT_FONT_SIZE, **kwargs):
        c_visual = dvz.glyph(self.c_batch, 0)
        visual = vs.Glyph(c_visual, font_size=font_size)
        visual.set_data(**kwargs)
        return visual

    def image(self, **kwargs):
        return self._visual(dvz.image, vs.Image, **kwargs)

    def _mesh(self, c_visual, vertex_count: int = None, index_count: int = None, **kwargs):
        return self._visual(c_visual=c_visual, cls=vs.Mesh, vertex_count=vertex_count, index_count=index_count, **kwargs)

    def mesh(self, indexed: bool = None, lighting: bool = None, contour: bool = False, **kwargs):
        c_flags = mesh_flags(indexed=indexed, lighting=lighting, contour=contour)
        return self._mesh(dvz.mesh(self.c_batch, c_flags), **kwargs)

    def mesh_shape(self, shape: ShapeCollection, indexed: bool = None, lighting: bool = None, contour: bool = False, **kwargs):
        if not shape.c_merged:
            shape.merge()
        c_merged = shape.c_merged

        # Allocate the visual with the right number of vertices and indices.
        nv = dvz.shape_vertex_count(c_merged)
        ni = dvz.shape_index_count(c_merged)
        if ni == 0:
            indexed = False

        # Force contour flag.
        if kwargs.get('linewidth', None) is not None or kwargs.get('edgecolor', None) is not None:
            contour = True

        c_flags = mesh_flags(indexed=indexed, lighting=lighting, contour=contour)
        return self._mesh(dvz.mesh_shape(self.c_batch, c_merged, c_flags), vertex_count=nv, index_count=ni, **kwargs)

    def sphere(self, **kwargs):
        return self._visual(dvz.sphere, vs.Sphere, **kwargs)

    def volume(self, mode: str = 'colormap', **kwargs):
        assert mode in cst.VOLUME_MODES
        c_flags = to_enum(f'volume_flags_{mode}')
        return self._visual(dvz.volume, vs.Volume, c_flags=c_flags, **kwargs)

    def slice(self, **kwargs):
        return self._visual(dvz.slice, vs.Slice, **kwargs)

    # GUI
    # ---------------------------------------------------------------------------------------------

    def arcball_gui(self, panel, arcball):
        c_figure = panel.c_figure
        dvz.arcball_gui(arcball.c_arcball, self.c_app, dvz.figure_id(c_figure), panel.c_panel)

    # Events
    # ---------------------------------------------------------------------------------------------

    def on_mouse(self, figure: Figure):
        assert figure

        def decorator(fun):
            @dvz.on_mouse
            def on_mouse(app, window_id, ev_):
                if dvz.figure_id(figure.c_figure) == window_id:
                    ev = ev_.contents
                    fun(Event(ev, 'mouse'))
            dvz.app_on_mouse(self.c_app, on_mouse, None)
            self._callbacks.append(on_mouse)
            return fun

        return decorator

    def on_keyboard(self, figure: Figure):
        assert figure

        def decorator(fun):
            @dvz.on_keyboard
            def on_keyboard(app, window_id, ev_):
                if dvz.figure_id(figure.c_figure) == window_id:
                    ev = ev_.contents
                    fun(Event(ev, 'keyboard'))
            dvz.app_on_keyboard(self.c_app, on_keyboard, None)
            self._callbacks.append(on_keyboard)
            return fun

        return decorator

    def on_gui(self, figure: Figure):
        assert figure
        fid = dvz.figure_id(figure.c_figure)

        def decorator(fun):
            @dvz.on_gui
            def on_gui(app, window_id, ev_):
                if fid == window_id:
                    ev = ev_.contents
                    fun(ev)
            dvz.app_gui(self.c_app, fid, on_gui, None)
            self._callbacks.append(on_gui)
            return fun

        return decorator

    def connect(self, figure: Figure):
        assert figure

        def decorator(fun):
            if fun.__name__ == 'on_mouse':
                return self.on_mouse(figure)(fun)
            elif fun.__name__ == 'on_keyboard':
                return self.on_keyboard(figure)(fun)
            elif fun.__name__ == 'on_gui':
                return self.on_gui(figure)(fun)
        return decorator

    def timer(self, delay: float = 0.0, period: float = 1.0, max_count: int = 0):
        def decorator(fun):
            @dvz.on_timer
            def on_timer(app, window_id, ev_):
                ev = ev_.contents
                fun(ev)
            dvz.app_on_timer(self.c_app, on_timer, None)
            self._callbacks.append(on_timer)
            return fun

        dvz.app_timer(self.c_app, delay, period, max_count)
        return decorator

    def timestamps(self, figure: Figure, count: int):
        assert figure
        seconds = np.zeros(count, dtype=np.uint64)  # epoch, in seconds
        nanoseconds = np.zeros(count, dtype=np.uint64)  # number of ns within the second
        dvz.app_timestamps(self.c_app, figure.figure_id(), count, seconds, nanoseconds)
        return seconds, nanoseconds
