"""
Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
Licensed under the MIT license. See LICENSE file in the project root for details.
SPDX-License-Identifier: MIT
"""

# Base classes

# -------------------------------------------------------------------------------------------------
# Imports
# -------------------------------------------------------------------------------------------------

import typing as tp
import numpy as np
from . import _ctypes as dvz
from . import _constants as cst
from ._constants import Vec3, PROPS, VEC_TYPES
from .utils import mesh_flags, to_enum, get_size, prepare_data_array, dtype_to_format
from .shape_collection import ShapeCollection

# -------------------------------------------------------------------------------------------------
# Constants
# -------------------------------------------------------------------------------------------------

VOLUME_MODES = ('colormap', 'rgba')


# -------------------------------------------------------------------------------------------------
# App
# -------------------------------------------------------------------------------------------------

class App:
    c_flags: int = 0
    c_app: dvz.DvzApp = None
    c_batch: dvz.DvzBatch = None
    c_scene: dvz.DvzScene = None

    def __init__(self, c_flags: int = 0):
        self.c_flags = c_flags
        self.c_app = dvz.app(c_flags)
        self.c_batch = dvz.app_batch(self.c_app)
        self.c_scene = dvz.scene(self.c_batch)

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

    def __del__(self):
        self.destroy()

    def destroy(self):
        if self.c_app is not None:
            dvz.scene_destroy(self.c_scene)
            dvz.app_destroy(self.c_app)
            self.c_app = None

    # Objects
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

    def basic(self, topology: str = None, **kwargs):
        from .visuals import Basic
        c_topology = to_enum(f'primitive_topology_{topology}')
        assert c_topology
        c_visual = dvz.basic(self.c_batch, c_topology, 0)
        visual = Basic(self, c_visual)
        visual.set_data(**kwargs)
        return visual

    def pixel(self, **kwargs):
        from .visuals import Pixel
        c_visual = dvz.pixel(self.c_batch, 0)
        visual = Pixel(self, c_visual)
        visual.set_data(**kwargs)
        return visual

    def point(self, **kwargs):
        from .visuals import Point
        c_visual = dvz.point(self.c_batch, 0)
        visual = Point(self, c_visual)
        visual.set_data(**kwargs)
        return visual

    def marker(self, **kwargs):
        from .visuals import Marker
        c_visual = dvz.marker(self.c_batch, 0)
        visual = Marker(self, c_visual)
        visual.set_data(**kwargs)
        return visual

    def segment(self, **kwargs):
        from .visuals import Segment
        c_visual = dvz.segment(self.c_batch, 0)
        visual = Segment(self, c_visual)
        visual.set_data(**kwargs)
        return visual

    def path(self, **kwargs):
        from .visuals import Path
        c_visual = dvz.path(self.c_batch, 0)
        visual = Path(self, c_visual)
        visual.set_data(**kwargs)
        return visual

    def glyph(self, font_size: int = cst.DEFAULT_FONT_SIZE, **kwargs):
        from .visuals import Glyph
        c_visual = dvz.glyph(self.c_batch, 0)
        visual = Glyph(self, c_visual, font_size=font_size)
        visual.set_data(**kwargs)
        return visual

    def image(self, **kwargs):
        from .visuals import Image
        c_visual = dvz.image(self.c_batch, 0)
        visual = Image(self, c_visual)
        visual.set_data(**kwargs)
        return visual

    def _mesh(self, c_visual, **kwargs):
        from .visuals import Mesh
        visual = Mesh(self, c_visual)
        visual.set_data(**kwargs)
        return visual

    def mesh(self, lighting: bool = True, contour: bool = False, **kwargs):
        c_flags = mesh_flags(lighting=lighting, contour=contour)
        return self._mesh(dvz.mesh(self.c_batch, c_flags), **kwargs)

    def mesh_shape(self, shape: ShapeCollection, lighting: bool = True, contour: bool = False, **kwargs):
        if not shape.c_merged:
            shape.merge()
        c_merged = shape.c_merged

        # Force contour flag.
        if kwargs.get('linewidth', None) is not None or kwargs.get('edgecolor', None) is not None:
            contour = True

        c_flags = mesh_flags(lighting=lighting, contour=contour)
        return self._mesh(dvz.mesh_shape(self.c_batch, c_merged, c_flags), **kwargs)

    def sphere(self, **kwargs):
        from .visuals import Sphere
        c_visual = dvz.sphere(self.c_batch, 0)
        visual = Sphere(self, c_visual)
        visual.set_data(**kwargs)
        return visual

    def volume(self, mode: str = 'colormap', **kwargs):
        from .visuals import Volume
        assert mode in VOLUME_MODES
        c_flags = to_enum(f'volume_flags_{mode}')
        c_visual = dvz.volume(self.c_batch, c_flags)
        visual = Volume(self, c_visual)
        visual.set_data(**kwargs)
        return visual

    def slice(self, **kwargs):
        from .visuals import Slice
        c_visual = dvz.slice(self.c_batch, 0)
        visual = Slice(self, c_visual)
        visual.set_data(**kwargs)
        return visual

    # GUI
    # ---------------------------------------------------------------------------------------------

    def arcball_gui(self, panel, arcball):
        c_figure = panel.c_figure
        dvz.arcball_gui(arcball.c_arcball, self.c_app, dvz.figure_id(c_figure), panel.c_panel)


# -------------------------------------------------------------------------------------------------
# Figure
# -------------------------------------------------------------------------------------------------

class Figure:
    c_figure: dvz.DvzFigure = None

    def __init__(self, c_figure: dvz.DvzFigure):
        assert c_figure
        self.c_figure = c_figure

    def panel(self, offset: tuple = None, size: tuple = None):
        if not offset and not size:
            c_panel = dvz.panel_default(self.c_figure)
        else:
            x, y = offset
            w, h = size
            c_panel = dvz.panel(self.c_figure, x, y, w, h)
        return Panel(c_panel, c_figure=self.c_figure)

    def on_mouse(self):
        pass

    def on_keyboard(self):
        pass

    def on_gui(self):
        pass


# -------------------------------------------------------------------------------------------------
# Visual
# -------------------------------------------------------------------------------------------------

class Visual:
    app: App = None
    c_visual: dvz.DvzVisual = None
    visual_name: str = ''
    count: int = 0
    _prop_classes: dict = None

    def __init__(self, app: App, c_visual: dvz.DvzVisual, visual_name: str = None):
        assert app
        assert c_visual

        # UGLY HACK: we override __setattr__() which only works AFTER self.visual_name has been
        # set, but we can't set self.visual_name the usual way because it calls __setattr__()
        # which requires self.visual_name! So we directly manipulate the __dict__ instead.
        if visual_name:
            assert visual_name in PROPS
            self.__dict__['visual_name'] = visual_name

        self.app = app
        self.c_visual = c_visual
        self._prop_classes = {}

        self.set_prop_classes()

    def set_count(self, count):
        self.count = count

    def get_count(self):
        return self.count

    def set_data(self, **kwargs):
        for key, value in kwargs.items():
            fn = getattr(self, f'set_{key}', None)
            if fn:
                fn(value)
            else:
                raise ValueError(f"Method '{self.__class__.__name__}.set_{key}' not found")

    def set_prop_class(self, prop_name: str, prop_cls: tp.Type):
        self._prop_classes[prop_name] = prop_cls

    def set_prop_classes(self):
        pass

    def __getattr__(self, prop_name: str):
        # assert not prop_name.startswith('set_')
        # print(f"Calling __getattr__() with {self.visual_name}.{prop_name}")
        prop_type = PROPS[self.visual_name].get(prop_name, {}).get('type', None)
        if prop_type is None:
            return super().__getattr__(prop_name)
        if prop_type == np.ndarray:
            prop_cls = self._prop_classes.get(prop_name, Prop)
            return prop_cls(self, prop_name)
        else:
            raise Exception(
                f"Prop '{prop_name}' is not a valid array property for visual {self.visual_name}")

    def __setattr__(self, prop_name: str, value: object):
        # handle visual.prop = value
        prop_info = PROPS[self.visual_name].get(prop_name, {})
        prop_type = prop_info.get('type', None)
        if not prop_type:
            return super().__setattr__(prop_name, value)

        # case where value is not an array
        elif prop_type != np.ndarray:
            # generic or custom Prop class
            prop_cls = self._prop_classes.get(prop_name, Prop)

            # instantiate the Prop
            prop = prop_cls(self, prop_name)

            # do nothing if the value is None
            if value is None:
                return

            # enum props
            if prop_type == 'enum':
                enum_prefix = prop_info['enum']
                enum_prefix = enum_prefix.replace('DVZ_', '')
                value = to_enum(f'{enum_prefix}_{value}')
                values = (value,)

            # texture
            elif prop_type == 'texture':
                if isinstance(value, np.ndarray):
                    texture = self.app.texture(value)
                values = (texture.c_tex, texture.c_sampler)

            elif prop_type in VEC_TYPES:
                assert hasattr(value, '__len__')
                value = prop_type(*value)
                values = (value,)

            # Python type props
            else:
                value = prop_type(value)
                values = (value,)

            # call the prop function
            prop.call(self.c_visual, *values)

        else:
            raise Exception(
                f"Prop '{prop_name}' is not a valid scalar property "
                f"for visual '{self.visual_name}'")


class Prop:
    visual: Visual = None
    visual_name: str = ''
    prop_name: str = ''
    _fn: tp.Callable = None
    _fn_alloc: tp.Callable = None

    def __init__(self, visual: Visual, prop_name: str):
        assert visual
        assert prop_name

        self.visual = visual
        self.visual_name = visual.visual_name
        self.prop_name = prop_name

        self._fn = getattr(dvz, f"{visual.visual_name}_{prop_name}", None)
        self._fn_alloc = getattr(dvz, f"{visual.visual_name}_alloc", None)

    @property
    def dtype(self):
        info = PROPS[self.visual_name][self.prop_name]
        return info.get('dtype', None)

    @property
    def shape(self):
        info = PROPS[self.visual_name][self.prop_name]
        return info.get('shape', None)

    @property
    def size(self):
        return self.visual.get_count()

    @property
    def name(self):
        return f'{self.visual_name}.{self.prop_name}'

    def prepare_data(self, value, size):
        if not isinstance(value, np.ndarray):
            # if doing visual.prop[idx] = scalar, need to create an array
            return prepare_data_scalar(self.name, self.dtype, size, value)
        else:
            # otherwise, just need to prepare the array with the right shape and dtype
            return prepare_data_array(self.name, self.dtype, self.shape, value)

    def allocate(self, count):
        self._fn_alloc(self.visual.c_visual, count)
        self.visual.set_count(count)

    def set(self, offset, length, pvalue, flags: int = 0):
        self.call(self.visual.c_visual, offset, length, pvalue, 0)

    def call(self, *args):
        return self._fn(*args)

    def __setitem__(self, idx, value):
        if value is None:
            return

        # Find the offset and size.
        offset = idx.start if isinstance(idx, slice) else 0
        size = get_size(idx, value, total_size=self.size)
        count = offset + size
        assert offset >= 0
        assert count > 0

        # Convert the data to a ndarray to be passed to the setter function.
        pvalue = self.prepare_data(value, size)

        # Allocate the data and register the item count.
        self.allocate(count)

        # Call the C property setter.
        self.set(offset, size, pvalue)


# -------------------------------------------------------------------------------------------------
# Texture
# -------------------------------------------------------------------------------------------------

class Texture:
    c_texture: dvz.DvzTexture = None

    def __init__(self, c_texture: dvz.DvzTexture):
        assert c_texture is not None
        self.c_texture = c_texture


# -------------------------------------------------------------------------------------------------
# Panel
# -------------------------------------------------------------------------------------------------

class Panzoom:
    c_panzoom: dvz.DvzPanzoom = None

    def __init__(self, c_panzoom: dvz.DvzPanzoom):
        assert c_panzoom
        self.c_panzoom = c_panzoom


class Arcball:
    c_arcball: dvz.DvzArcball = None

    def __init__(self, c_arcball: dvz.DvzArcball):
        assert c_arcball
        self.c_arcball = c_arcball


class Camera:
    c_camera: dvz.DvzCamera = None

    def __init__(self, c_camera: dvz.DvzCamera):
        assert c_camera
        self.c_camera = c_camera


class Panel:
    c_panel: dvz.DvzPanel = None
    c_figure: dvz.DvzFigure = None

    def __init__(self, c_panel: dvz.DvzPanel, c_figure: dvz.DvzFigure = None):
        assert c_panel
        self.c_panel = c_panel
        self.c_figure = c_figure

    def add(self, visual: Visual):
        assert visual
        dvz.panel_visual(self.c_panel, visual.c_visual, 0)

    def panzoom(self, flags: int = 0):
        c_panzoom = dvz.panel_panzoom(self.c_panel, flags)
        return Panzoom(c_panzoom)

    def arcball(self, initial: Vec3 = None, flags: int = 0):
        c_arcball = dvz.panel_arcball(self.c_panel, flags)
        if initial is not None:
            dvz.arcball_initial(c_arcball, dvz.vec3(*initial))
            self.update()
        return Arcball(c_arcball)

    def camera(self, initial: Vec3 = None, initial_lookat: Vec3 = None, initial_up: Vec3 = None, flags: int = 0):
        c_camera = dvz.panel_camera(self.c_panel, flags)
        pos = initial if initial is not None else cst.DEFAULT_CAMERA_POS
        lookat = initial_lookat if initial_lookat is not None else cst.DEFAULT_CAMERA_LOOKAT
        up = initial_up if initial_up is not None else cst.DEFAULT_CAMERA_UP
        if initial is not None:
            dvz.camera_initial(c_camera, dvz.vec3(*pos), dvz.vec3(*lookat), dvz.vec3(*up))
            self.update()
        return Camera(c_camera)

    def update(self):
        dvz.panel_update(self.c_panel)
