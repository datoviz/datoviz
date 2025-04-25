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
DEFAULT_INTERPOLATION = 'linear'
DEFAULT_ADDRESS_MODE = 'clamp_to_border'
VEC_TYPES = (dvz.vec3, dvz.vec4, dvz.cvec4)  # TODO: others
DTYPE_FORMATS = {
    ('np.uint8', 4): dvz.FORMAT_R8G8B8A8_UNORM,
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

        'tex': {'type': 'tex'},
    },

    'segment': {
        'position': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 3)},
        'shift': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 4)},
        'color': {'type': np.ndarray, 'dtype': np.uint8, 'shape': (-1, 4)},
        'linewidth': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1,)},

        'cap': {'type': 'enum', 'enum': 'DVZ_CAP'},
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
        'tex': {'type': 'tex'},
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
        'ardius': {'type': float},
        'colormap': {'type': 'enum', 'enum': 'DVZ_CMAP'},

        'tex': {'type': 'tex'},
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

        'tex': {'type': 'tex'},
    },

    'sphere': {
        'position': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 3)},
        'color': {'type': np.ndarray, 'dtype': np.uint8, 'shape': (-1, 4)},
        'size': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1,)},

        'light_pos': {'type': dvz.vec3},
        'light_params': {'type': dvz.vec4},
    },

    'volume': {
        'bounds': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 3)},
        'permutation': {'type': dvz.ivec3},
        'slice': {'type': int},
        'transfer': {'type': dvz.vec4},
        'tex': {'type': 'tex'},
    },

    'slice': {
        'position': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 3)},
        'texcoords': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 4)},
        'alpha': {'type': float},
        'tex': {'type': 'tex'},
    },

}


# -------------------------------------------------------------------------------------------------
# Utils
# -------------------------------------------------------------------------------------------------

def parse_index(idx, total_size=0):
    offset = size = 0
    if isinstance(idx, slice):
        offset = idx.start or 0
        step = idx.step or 1
        size = (idx.stop or total_size - offset) // step
    else:
        raise ValueError(idx)
    return offset, size


def dtype_to_format(dtype, n_channels):
    return DTYPE_FORMATS[dtype, n_channels]


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

    def tex(self,
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
        format = dtype_to_format(dtype)
        shape = dvz.uvec3(*shape)
        tex = dvz.create_tex(self.c_batch, getattr(dvz, f'TEX_{ndim}D'), format, shape, 0).id
        return Tex(tex, interpolation=interpolation, address_mode=address_mode)

    def sampler(self, interpolation, address_mode):
        c_filter = dvz.to_enum(f'filter_{interpolation}')
        c_address_mode = dvz.to_enum(f'sampler_address_mode_{address_mode}')
        sampler = dvz.create_sampler(self.c_batch, c_filter, c_address_mode).id
        return sampler

    # Visuals
    # ---------------------------------------------------------------------------------------------

    def basic(
            self, topology: str = None, position: np.ndarray = None, color: np.ndarray = None,
            group: np.ndarray = None, size: float = None):

        c_topology = dvz.to_enum(f'primitive_topology_{topology}')
        c_visual = dvz.basic(self.c_batch, c_topology, 0)

        visual = Visual(self, c_visual, 'basic')
        visual.position[:] = position
        visual.color[:] = color
        visual.group[:] = group
        visual.size = size

        return visual

    def pixel(self, position: np.ndarray = None, color: np.ndarray = None, size: float = None):

        c_visual = dvz.pixel(self.c_batch, 0)
        visual = Visual(self, c_visual, 'pixel')

        visual.position[:] = position
        visual.color[:] = color
        visual.size = size

        return visual

    def point(self, position: np.ndarray = None, color: np.ndarray = None, size: float = None):

        c_visual = dvz.point(self.c_batch, 0)
        visual = Visual(self, c_visual, 'point')

        visual.position[:] = position
        visual.color[:] = color
        visual.size = size

        return visual

    def marker(
        self,
        position: np.ndarray = None,
        color: np.ndarray = None,
        size: float = None,
        angle: np.ndarray = None,
        edgecolor: tuple = None,
        linewidth: float = None,
        mode: str = None,
        aspect: str = None,
        shape: str = None,
    ):

        c_visual = dvz.marker(self.c_batch, 0)
        visual = Visual(self, c_visual, 'marker')

        visual.position[:] = position
        visual.color[:] = color
        visual.size[:] = size
        visual.angle[:] = angle

        visual.edgecolor = edgecolor
        visual.linewidth = linewidth
        visual.mode = mode
        visual.aspect = aspect
        visual.shape = shape

        return visual


# -------------------------------------------------------------------------------------------------
# Figure
# -------------------------------------------------------------------------------------------------

class Figure:
    c_figure: dvz.DvzFigure = None

    def __init__(self, c_figure: dvz.DvzFigure):
        assert c_figure
        self.c_figure = c_figure

    def panel(self):
        c_panel = dvz.panel_default(self.c_figure)
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

    def __init__(self, app: App, c_visual: dvz.DvzVisual, visual_name: str):
        assert app
        assert visual_name
        assert visual_name in PROPS
        assert c_visual

        # UGLY HACK: we override __setattr__() which only works AFTER self.visual_name has been
        # set, but we can't set self.visual_name the usual way because it calls __setattr__()
        # which requires self.visual_name! So we directly manipulate the __dict__ instead.
        self.__dict__['visual_name'] = visual_name
        self.app = app
        self.c_visual = c_visual
        self._prop_classes = {}

    def set_count(self, count):
        self.count = count

    def get_count(self):
        return self.count

    def set_prop_class(self, prop_name: str, prop_cls: tp.Type):
        self._prop_classes[prop_name] = prop_cls

    def __getattr__(self, prop_name: str):
        prop_type = PROPS[self.visual_name].get(prop_name, {}).get('type', None)
        if prop_type == np.ndarray:
            prop_cls = self._prop_classes.get(prop_name, Prop)
            return prop_cls(self, prop_name)
        else:
            raise Exception(f"Prop '{prop_name}' is not a valid array property for visual {self}")

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
            elif prop_type == 'tex':
                if isinstance(value, np.ndarray):
                    tex = self.app.tex(value)
                c_sampler = self.app.sampler(tex.interpolation, tex.address_mode)
                values = (tex.c_tex, c_sampler)

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
            raise Exception(f"Prop '{prop_name}' is not a valid scalar property for visual {self}")


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

    def prepare_data_array(self, value):
        dtype = self.dtype
        shape = self.shape
        ndim = len(shape)
        pvalue = np.asanyarray(value, dtype=dtype)
        if pvalue.ndim < ndim:
            if pvalue.ndim == 2:
                pvalue = np.atleast_2d(pvalue)
            elif pvalue.ndim == 3:
                pvalue = np.atleast_3d(pvalue)
        elif pvalue.ndim > ndim:
            raise ValueError(
                f"Visual property {self.visual_name}.{self.prop_name} should have shape "
                f"{shape} instead of {pvalue.shape}")
        assert ndim == pvalue.ndim
        for dim in range(ndim):
            if shape[dim] > 0 and pvalue.shape[dim] != shape[dim]:
                raise ValueError(f"Incorrect shape {pvalue.shape[dim]} != {shape[dim]}")
        return pvalue

    def prepare_data_scalar(self, value):
        size = self.visual.get_count()
        if size == 0:
            raise ValueError(
                f"Property {self.visual_name}.{self.prop_name} needs to be set after the position")
        pvalue = np.full(size, value, dtype=self.dtype)
        return pvalue

    def prepare_data(self, value):
        # if doing visual.prop[idx] = scalar, need to create an array
        if not isinstance(value, np.ndarray):
            pvalue = self.prepare_data_scalar(value)
            size = self.visual.get_count()

        # otherwise, just need to prepare the array with the right shape and dtype
        else:
            pvalue = self.prepare_data_array(value)
            size = pvalue.shape[0]

        return pvalue, size

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

        # Convert the data to a ndarray to be passed to the setter function.
        pvalue, size = self.prepare_data(value)

        # extract the offset and length from the index (may be a slice object)
        offset, length = parse_index(idx, size)
        assert offset >= 0
        assert length > 0

        # allocate the data and register the item count
        count = offset + length
        self.allocate(count)

        # call the C property setter
        self.set(offset, length, pvalue)


class Tex:
    c_tex: dvz.DvzId = None
    interpolation: str = DEFAULT_INTERPOLATION
    address_mode: str = DEFAULT_ADDRESS_MODE

    def __init__(
            self, c_tex: dvz.DvzId = None,
            interpolation=DEFAULT_INTERPOLATION,
            address_mode=DEFAULT_ADDRESS_MODE):
        assert c_tex is not None
        self.c_tex = c_tex
        self.interpolation = interpolation
        self.address_mode = address_mode


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

    n = 1_000
    position = np.random.normal(size=(n, 3), scale=.25)
    color = np.random.randint(size=(n, 4), low=100, high=255)
    size = np.random.uniform(size=(n,), low=20, high=40)

    visual = app.marker(
        position=position, color=color, size=size,
        shape='asterisk',
        edgecolor=(255, 255, 255, 255),
        linewidth=1.0,
        aspect='outline')

    panel.add(visual)
    app.run()
    app.destroy()
