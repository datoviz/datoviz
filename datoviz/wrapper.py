"""
Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
Licensed under the MIT license. See LICENSE file in the project root for details.
SPDX-License-Identifier: MIT
"""

# Python wrapper

# -------------------------------------------------------------------------------------------------
# Imports
# -------------------------------------------------------------------------------------------------

import typing as tp
import numpy as np
import datoviz as dvz


# -------------------------------------------------------------------------------------------------
# Globals
# -------------------------------------------------------------------------------------------------

DEFAULT_WIDTH = 800
DEFAULT_HEIGHT = 600
DEFAULT_FONT_SIZE = 30
DEFAULT_GLYPH_COLOR = (0, 0, 0, 255)
DEFAULT_INTERPOLATION = 'linear'
DEFAULT_ADDRESS_MODE = 'clamp_to_border'
VEC_TYPES = (dvz.vec3, dvz.vec4, dvz.cvec4)  # TODO: others
DTYPE_FORMATS = {
    ('uint8', 4): dvz.FORMAT_R8G8B8A8_UNORM,
    ('float32', 4): dvz.FORMAT_R32G32B32A32_SFLOAT,
}
PROPS = {
    'basic': {
        'position': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 3)},
        'color': {'type': np.ndarray, 'dtype': np.uint8, 'shape': (-1, 4)},
        'group': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1,)},
        'size': {'type': float},
    },

    'pixel': {
        'position': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 3)},
        'color': {'type': np.ndarray, 'dtype': np.uint8, 'shape': (-1, 4)},
        'size': {'type': float},
    },

    'point': {
        'position': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 3)},
        'color': {'type': np.ndarray, 'dtype': np.uint8, 'shape': (-1, 4)},
        'size': {'type': float},
    },

    'marker': {
        'position': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 3)},
        'size': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1,)},
        'angle': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1,)},
        'color': {'type': np.ndarray, 'dtype': np.uint8, 'shape': (-1, 4)},

        'edgecolor': {'type': dvz.cvec4},
        'linewidth': {'type': float},
        'tex_scale': {'type': float},

        'mode': {'type': 'enum', 'enum': 'DVZ_MARKER_MODE'},
        'aspect': {'type': 'enum', 'enum': 'DVZ_MARKER_ASPECT'},
        'shape': {'type': 'enum', 'enum': 'DVZ_MARKER_SHAPE'},

        'texture': {'type': 'texture'},
    },

    'segment': {
        'position': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 3)},
        'shift': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 4)},
        'color': {'type': np.ndarray, 'dtype': np.uint8, 'shape': (-1, 4)},
        'linewidth': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1,)},
        'cap': {'type': np.ndarray, 'dtype': np.int32, 'shape': (-1,)},
    },

    'path': {
        'position': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 3)},
        'shift': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 4)},
        'color': {'type': np.ndarray, 'dtype': np.uint8, 'shape': (-1, 4)},
        'linewidth': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1,)},

        'cap': {'type': 'enum', 'enum': 'DVZ_CAP'},
        'join': {'type': 'enum', 'enum': 'DVZ_JOIN'},
    },

    'glyph': {
        'position': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 3)},
        'axis': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 3)},
        'size': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 2)},
        'anchor': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 2)},
        'shift': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 2)},
        'texcoords': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 4)},
        'group_size': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 2)},
        'scale': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1,)},
        'angle': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1,)},
        'color': {'type': np.ndarray, 'dtype': np.uint8, 'shape': (-1, 4)},

        'bgcolor': {'type': dvz.vec4},
        'texture': {'type': 'texture'},
    },

    'image': {
        'position': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 3)},
        'size': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 2)},
        'anchor': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 2)},
        'texcoords': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 4)},
        'facecolor': {'type': np.ndarray, 'dtype': np.uint8, 'shape': (-1, 4)},

        'edgecolor': {'type': dvz.vec4},
        'permutation': {'type': dvz.ivec2},
        'linewidth': {'type': float},
        'radius': {'type': float},
        'colormap': {'type': 'enum', 'enum': 'DVZ_CMAP'},

        'texture': {'type': 'texture'},
    },

    'mesh': {
        'position': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 3)},
        'color': {'type': np.ndarray, 'dtype': np.uint8, 'shape': (-1, 4)},
        'texcoords': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 4)},
        'normal': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 3)},
        'isoline': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1,)},
        'left': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 3)},
        'right': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 3)},
        'contour': {'type': np.ndarray, 'dtype': np.uint8, 'shape': (-1, 4)},
        'index': {'type': np.ndarray, 'dtype': np.uint32, 'shape': (-1,)},

        'light_dir': {'type': dvz.vec3},
        'light_color': {'type': dvz.cvec4},
        'light_params': {'type': dvz.vec4},
        'edgecolor': {'type': dvz.cvec4},
        'linewidth': {'type': float},
        'density': {'type': int},

        'texture': {'type': 'texture'},
    },

    'sphere': {
        'position': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 3)},
        'color': {'type': np.ndarray, 'dtype': np.uint8, 'shape': (-1, 4)},
        'size': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1,)},

        'light_pos': {'type': dvz.vec3},
        'light_params': {'type': dvz.vec4},
    },

    'volume': {
        'permutation': {'type': dvz.ivec3},
        'slice': {'type': int},
        'transfer': {'type': dvz.vec4},
        'texture': {'type': 'texture'},
    },

    'slice': {
        'position': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 3)},
        'texcoords': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 4)},
        'alpha': {'type': float},
        'texture': {'type': 'texture'},
    },

}


# -------------------------------------------------------------------------------------------------
# Utils
# -------------------------------------------------------------------------------------------------

def get_size(idx, value, total_size=0):
    if isinstance(value, np.ndarray):
        return value.shape[0]
    elif isinstance(idx, slice):
        offset = idx.start or 0
        step = idx.step or 1
        return (idx.stop or total_size - offset) // step
    else:
        return total_size


def dtype_to_format(dtype, n_channels):
    return DTYPE_FORMATS[dtype, n_channels]


def prepare_data_array(name, dtype, shape, value):
    ndim = len(shape)
    pvalue = np.asanyarray(value, dtype=dtype)
    if pvalue.ndim < ndim:
        if pvalue.ndim == 2:
            pvalue = np.atleast_2d(pvalue)
        elif pvalue.ndim == 3:
            pvalue = np.atleast_3d(pvalue)
    elif pvalue.ndim > ndim:
        raise ValueError(
            f"Visual property {name} should have shape {shape} instead of {pvalue.shape}")
    assert ndim == pvalue.ndim
    for dim in range(ndim):
        if shape[dim] > 0 and pvalue.shape[dim] != shape[dim]:
            raise ValueError(f"Incorrect shape {pvalue.shape[dim]} != {shape[dim]}")
    return pvalue


def prepare_data_scalar(name, dtype, size, value):
    if size == 0:
        raise ValueError(
            f"Property {name} needs to be set after the position")
    pvalue = np.full(size, value, dtype=dtype)
    return pvalue


# -------------------------------------------------------------------------------------------------
# App
# -------------------------------------------------------------------------------------------------

class App:
    _flags: int = 0
    c_app: dvz.DvzApp = None
    c_batch: dvz.DvzBatch = None
    c_scene: dvz.DvzScene = None

    def __init__(self, flags: int = 0):
        self._flags = flags
        self.c_app = dvz.app(flags)
        self.c_batch = dvz.app_batch(self.c_app)
        self.c_scene = dvz.scene(self.c_batch)

    def figure(self, width: int = DEFAULT_WIDTH, height: int = DEFAULT_HEIGHT):
        c_figure = dvz.figure(self.c_scene, width, height, 0)
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

    def texture(self,
                image: np.ndarray = None,
                ndim: int = 2,
                shape: tuple = None,
                n_channels: int = None,
                dtype: np.dtype = None,
                interpolation: str = 'nearest',
                address_mode: str = 'clamp_to_border',
                ):
        if image is not None:
            shape = shape or image.shape[:ndim]
            n_channels = n_channels or (image.shape[-1] if ndim == image.ndim - 1 else 1)
            dtype = dtype or image.dtype
        assert n_channels > 0
        c_format = dtype_to_format(dtype.name, n_channels)
        shape = dvz.uvec3(*shape)
        width, height, _ = shape
        c_filter = dvz.to_enum(f'filter_{interpolation}')
        c_address_mode = dvz.to_enum(f'sampler_address_mode_{address_mode}')
        c_texture = dvz.texture_image(
            self.c_batch, c_format, c_filter, c_address_mode, width, height, image, 0)
        return Texture(c_texture)

    # Visuals
    # ---------------------------------------------------------------------------------------------

    def basic(self, topology: str = None, **kwargs):
        c_topology = dvz.to_enum(f'primitive_topology_{topology}')
        c_visual = dvz.basic(self.c_batch, c_topology, 0)
        visual = Basic(self, c_visual)
        visual.set_data(**kwargs)
        return visual

    def pixel(self, **kwargs):
        c_visual = dvz.pixel(self.c_batch, 0)
        visual = Pixel(self, c_visual)
        visual.set_data(**kwargs)
        return visual

    def point(self, **kwargs):
        c_visual = dvz.point(self.c_batch, 0)
        visual = Point(self, c_visual)
        visual.set_data(**kwargs)
        return visual

    def marker(self, **kwargs):
        c_visual = dvz.marker(self.c_batch, 0)
        visual = Marker(self, c_visual)
        visual.set_data(**kwargs)
        return visual

    def segment(self, **kwargs):
        c_visual = dvz.segment(self.c_batch, 0)
        visual = Segment(self, c_visual)
        visual.set_data(**kwargs)
        return visual

    def path(self, **kwargs):
        c_visual = dvz.path(self.c_batch, 0)
        visual = Path(self, c_visual)
        visual.set_data(**kwargs)
        return visual

    def glyph(self, font_size: int = DEFAULT_FONT_SIZE, **kwargs):
        c_visual = dvz.glyph(self.c_batch, 0)
        visual = Glyph(self, c_visual, font_size=font_size)
        visual.set_data(**kwargs)
        return visual

    def image(self, **kwargs):
        c_visual = dvz.image(self.c_batch, 0)
        visual = Image(self, c_visual)
        visual.set_data(**kwargs)
        return visual

    def mesh(self, **kwargs):
        c_visual = dvz.mesh(self.c_batch, 0)
        visual = Mesh(self, c_visual)
        visual.set_data(**kwargs)
        return visual

    def sphere(self, **kwargs):
        c_visual = dvz.sphere(self.c_batch, 0)
        visual = Sphere(self, c_visual)
        visual.set_data(**kwargs)
        return visual

    def volume(self, **kwargs):
        c_visual = dvz.volume(self.c_batch, 0)
        visual = Volume(self, c_visual)
        visual.set_data(**kwargs)
        return visual

    def slice(self, **kwargs):
        c_visual = dvz.slice(self.c_batch, 0)
        visual = Slice(self, c_visual)
        visual.set_data(**kwargs)
        return visual


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
        return Panel(c_panel)

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

    def set_prop_class(self, prop_name: str, prop_cls: tp.Type):
        self._prop_classes[prop_name] = prop_cls

    def set_prop_classes(self):
        pass

    def __getattr__(self, prop_name: str):
        print(f"Calling __getattr__() on {self.visual_name}.{prop_name}")
        prop_type = PROPS[self.visual_name].get(prop_name, {}).get('type', None)
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
                value = dvz.to_enum(f'{enum_prefix}_{value}')
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

        self._fn = getattr(dvz, f"{visual.visual_name}_{prop_name}")
        self._fn_alloc = getattr(dvz, f"{visual.visual_name}_alloc")

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
# Visuals
# -------------------------------------------------------------------------------------------------

class Basic(Visual):
    visual_name = 'basic'

    def set_position(self, array: np.ndarray, offset: int = 0):
        self.position[offset:] = array

    def set_color(self, array: np.ndarray, offset: int = 0):
        self.color[offset:] = array

    def set_group(self, array: np.ndarray, offset: int = 0):
        self.group[offset:] = array

    def set_size(self, value: float):
        self.size = value


class Pixel(Visual):
    visual_name = 'pixel'

    def set_position(self, array: np.ndarray, offset: int = 0):
        self.position[offset:] = array

    def set_color(self, array: np.ndarray, offset: int = 0):
        self.color[offset:] = array

    def set_size(self, array: np.ndarray, offset: int = 0):
        self.size[offset:] = array


class Point(Visual):
    visual_name = 'point'

    def set_position(self, array: np.ndarray, offset: int = 0):
        self.position[offset:] = array

    def set_color(self, array: np.ndarray, offset: int = 0):
        self.color[offset:] = array

    def set_size(self, array: np.ndarray, offset: int = 0):
        self.size[offset:] = array


class Marker(Visual):
    visual_name = 'marker'

    def set_position(self, array: np.ndarray, offset: int = 0):
        self.position[offset:] = array

    def set_color(self, array: np.ndarray, offset: int = 0):
        self.color[offset:] = array

    def set_size(self, array: np.ndarray, offset: int = 0):
        self.size[offset:] = array

    def set_angle(self, array: np.ndarray, offset: int = 0):
        self.angle[offset:] = array

    def set_linewidth(self, value: float):
        self.linewidth = value

    def set_edgecolor(self, value: tuple):
        self.edgecolor = value

    def set_mode(self, value: str):
        self.mode = value

    def set_aspect(self, value: str):
        self.aspect = value

    def set_shape(self, value: str):
        self.shape = value

    def set_tex_scale(self, value: float):
        self.tex_scale = value

    def set_texture(self, texture: Texture):
        dvz.marker_texture(self.c_visual, texture.c_texture)


class SegmentProp(Prop):
    def prepare_data(self, value, size):
        initial, terminal = value
        pinitial = super().prepare_data(initial, size)
        pterminal = super().prepare_data(terminal, size)
        return pinitial, pterminal

    def set(self, offset, length, pvalue, flags: int = 0):
        initial, terminal = pvalue
        self.call(self.visual.c_visual, offset, length, initial, terminal, flags)


class Segment(Visual):
    visual_name = 'segment'

    def set_prop_classes(self):
        self.set_prop_class('position', SegmentProp)
        self.set_prop_class('cap', SegmentProp)

    def set_position(self, initial: np.ndarray, terminal: np.ndarray, offset: int = 0):
        n = initial.shape[0]
        self.set_count(offset + n)
        self.position[offset:] = initial, terminal

    def set_color(self, array: np.ndarray, offset: int = 0):
        self.color[offset:] = array

    def set_linewidth(self, array: np.ndarray, offset: int = 0):
        self.linewidth[offset:] = array

    def set_shift(self, array: np.ndarray, offset: int = 0):
        self.shift[offset:] = array

    def set_cap(self, initial: np.ndarray, terminal: np.ndarray, offset: int = 0):
        self.cap[offset:] = (initial, terminal)


class Path(Visual):
    visual_name = 'path'

    def set_position(self, positions: tp.List[np.ndarray], offset: int = 0):
        if isinstance(positions, np.ndarray):
            if positions.ndim == 2:
                positions = list(positions)
            elif positions.ndim == 3:
                positions = list(positions)
        point_count = sum(map(len, positions))
        path_count = len(positions)
        path_lengths = np.array([len(p) for p in positions], dtype=np.uint32)

        positions_concat = np.concatenate(positions, axis=0).astype(np.float32)
        assert positions_concat.ndim == 2
        assert positions_concat.shape[1] == 3
        positions_concat = prepare_data_array(self.name, np.float32, (-1, 3), positions_concat)
        dvz.path_position(self, offset, point_count, positions_concat, path_count, path_lengths, 0)

    def set_shift(self, array: np.ndarray, offset: int = 0):
        self.shift[offset:] = array

    def set_color(self, array: np.ndarray, offset: int = 0):
        self.color[offset:] = array

    def set_linewidth(self, array: np.ndarray, offset: int = 0):
        self.linewidth[offset:] = array

    def set_cap(self, value: str):
        self.cap = value

    def set_joint(self, value: str):
        self.joint = value


class Glyph(Visual):
    visual_name = 'glyph'
    _af = None

    def __init__(self, *args, font_size: int = None, **kwargs):
        super().__init__(*args, **kwargs)
        self._af = dvz.AtlasFont()
        dvz.atlas_font(font_size, self._af)
        dvz.glyph_atlas_font(self.c_visual, self._af)

    def set_strings(
            self, strings: tp.List[str],
            string_pos: np.ndarray = None,
            scales: np.ndarray = None,
            color: tuple = DEFAULT_GLYPH_COLOR,
            anchor: tuple = (0, 0),
            offset: tuple = (0, 0),
    ):
        assert strings
        assert string_pos is not None
        assert scales is not None
        string_count = len(strings)
        dvz.glyph_strings(
            self.c_visual,
            string_count,
            strings,
            string_pos,
            scales,
            dvz.cvec4(*color),
            dvz.vec2(*offset),
            dvz.vec2(*anchor),
        )

    def set_position(self, array: np.ndarray, offset: int = 0):
        self.position[offset:] = array

    def set_axis(self, array: np.ndarray, offset: int = 0):
        self.axis[offset:] = array

    def set_size(self, array: np.ndarray, offset: int = 0):
        self.size[offset:] = array

    def set_anchor(self, array: np.ndarray, offset: int = 0):
        self.anchor[offset:] = array

    def set_shift(self, array: np.ndarray, offset: int = 0):
        self.shift[offset:] = array

    def set_texcoords(self, array: np.ndarray, offset: int = 0):
        self.texcoords[offset:] = array

    def set_group_size(self, array: np.ndarray, offset: int = 0):
        self.group_size[offset:] = array

    def set_scale(self, array: np.ndarray, offset: int = 0):
        self.scale[offset:] = array

    def set_angle(self, array: np.ndarray, offset: int = 0):
        self.angle[offset:] = array

    def set_color(self, array: np.ndarray, offset: int = 0):
        self.color[offset:] = array

    def set_bgcolor(self, value: tuple):
        self.bgcolor = value

    def set_texture(self, texture: Texture):
        dvz.glyph_texture(self.c_visual, texture.c_texture)


class Image(Visual):
    visual_name = 'image'

    def set_position(self, array: np.ndarray, offset: int = 0):
        self.position[offset:] = array

    def set_size(self, array: np.ndarray, offset: int = 0):
        self.size[offset:] = array

    def set_anchor(self, array: np.ndarray, offset: int = 0):
        self.anchor[offset:] = array

    def set_texcoords(self, array: np.ndarray, offset: int = 0):
        self.texcoords[offset:] = array

    def set_facecolor(self, array: np.ndarray, offset: int = 0):
        self.facecolor[offset:] = array

    def set_edgecolor(self, value: tuple):
        self.edgecolor = value

    def set_permutation(self, value: tuple):
        self.permutation = value

    def set_linewidth(self, value: float):
        self.linewidth = value

    def set_radius(self, value: float):
        self.radius = value

    def set_colormap(self, value: str):
        self.colormap = value

    def set_texture(self, texture: Texture):
        dvz.image_texture(self.c_visual, texture.c_texture)


class Mesh(Visual):
    visual_name = 'mesh'

    def set_position(self, array: np.ndarray, offset: int = 0):
        self.position[offset:] = array

    def set_color(self, array: np.ndarray, offset: int = 0):
        self.color[offset:] = array

    def set_texcoords(self, array: np.ndarray, offset: int = 0):
        self.texcoords[offset:] = array

    def set_normal(self, array: np.ndarray, offset: int = 0):
        self.normal[offset:] = array

    def set_isoline(self, array: np.ndarray, offset: int = 0):
        self.isoline[offset:] = array

    def set_index(self, array: np.ndarray, offset: int = 0):
        self.index[offset:] = array

    def set_light_dir(self, value: tuple):
        self.light_dir = value

    def set_light_color(self, value: tuple):
        self.light_color = value

    def set_light_params(self, value: tuple):
        self.light_params = value

    def set_light_edgecolor(self, value: tuple):
        self.light_edgecolor = value

    def set_light_linewidth(self, value: float):
        self.light_linewidth = value

    def set_light_density(self, value: int):
        self.light_density = value

    def set_texture(self, texture: Texture):
        dvz.image_texture(self.c_visual, texture.c_texture)


class Sphere(Visual):
    visual_name = 'sphere'

    def set_position(self, array: np.ndarray, offset: int = 0):
        self.position[offset:] = array

    def set_color(self, array: np.ndarray, offset: int = 0):
        self.color[offset:] = array

    def set_size(self, array: np.ndarray, offset: int = 0):
        self.size[offset:] = array

    def set_light_pos(self, value: tuple):
        self.light_pos = value

    def set_light_params(self, value: tuple):
        self.light_params = value


class Volume(Visual):
    visual_name = 'volume'

    def set_bounds(self, xlim: tuple, ylim: tuple, zlim: tuple):
        dvz.volume_bounds(self.c_visual, dvz.vec2(xlim), dvz.vec2(ylim), dvz.vec2(zlim))

    def set_texcoords(self, uvw0: tuple, uvw1: tuple):
        dvz.volume_texcoords(self.c_visual, dvz.vec3(uvw0), dvz.vec3(uvw1))

    def set_permutation(self, value: tuple):
        self.permutation = value

    def set_slice(self, value: int):
        self.slice = value

    def set_transfer(self, value: tuple):
        self.transfer = value

    def set_texture(self, texture: Texture):
        dvz.image_texture(self.c_visual, texture.c_texture)


class Slice(Visual):
    visual_name = 'slice'

    def set_position(self, array: np.ndarray, offset: int = 0):
        self.position[offset:] = array

    def set_texcoords(self, array: np.ndarray, offset: int = 0):
        self.texcoords[offset:] = array

    def set_alpha(self, value: float):
        self.alpha = value

    def set_texture(self, texture: Texture):
        dvz.image_texture(self.c_visual, texture.c_texture)


# -------------------------------------------------------------------------------------------------
# Panel
# -------------------------------------------------------------------------------------------------

class Panzoom:
    c_panzoom: dvz.DvzPanzoom = None

    def __init__(self, c_panzoom: dvz.DvzPanzoom):
        assert c_panzoom
        self.c_panzoom = c_panzoom


class Panel:
    c_panel: dvz.DvzPanel = None

    def __init__(self, c_panel: dvz.DvzPanel):
        assert c_panel
        self.c_panel = c_panel

    def add(self, visual: Visual):
        assert visual
        dvz.panel_visual(self.c_panel, visual.c_visual, 0)

    def panzoom(self, flags: int = None):
        c_panzoom = dvz.panel_panzoom(self.c_panel, flags)
        return Panzoom(c_panzoom)


if __name__ == '__main__':
    app = App()
    fig = app.figure()
    panel = fig.panel()
    panzoom = panel.panzoom()

    n = 100
    position = np.random.normal(size=(n, 3), scale=.25)
    position_2 = np.random.normal(size=(n, 3), scale=.25)
    color = np.random.randint(size=(n, 4), low=100, high=255)
    size = np.random.uniform(size=(n,), low=20, high=40)

    visual = app.segment()
    visual.set_position(initial=position, terminal=position_2)
    visual.set_color(color)
    visual.set_linewidth(20.0)
    visual.set_cap(2, 3)

    panel.add(visual)
    app.run()
    app.destroy()
