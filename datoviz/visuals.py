"""
Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
Licensed under the MIT license. See LICENSE file in the project root for details.
SPDX-License-Identifier: MIT
"""

# Visuals

# -------------------------------------------------------------------------------------------------
# Imports
# -------------------------------------------------------------------------------------------------

import typing as tp

import numpy as np

from . import _constants as cst
from . import _ctypes as dvz
from ._constants import PROPS, VEC_TYPES
from ._texture import Texture
from .utils import get_fixed_params, get_size, prepare_data_array, prepare_data_scalar, to_enum

# -------------------------------------------------------------------------------------------------
# Visual
# -------------------------------------------------------------------------------------------------


class Visual:
    c_visual: dvz.DvzVisual = None
    visual_name: str = ''
    count: int = 0

    _prop_classes: dict = None
    _fn_alloc: tp.Callable = None

    def __init__(self, c_visual: dvz.DvzVisual, visual_name: str = None):
        # assert app
        assert c_visual

        # UGLY HACK: we override __setattr__() which only works AFTER self.visual_name has been
        # set, but we can't set self.visual_name the usual way because it calls __setattr__()
        # which requires self.visual_name! So we directly manipulate the __dict__ instead.
        if visual_name:
            assert visual_name in PROPS
            self.__dict__['visual_name'] = visual_name

        # self.app = app
        self.c_visual = c_visual
        self._prop_classes = {}

        self._fn_alloc = getattr(dvz, f'{self.visual_name}_alloc', None)

        self.set_prop_classes()

    def show(self, is_visible: bool = True):
        dvz.visual_show(self.c_visual, is_visible)

    def hide(self):
        self.show(False)

    def clip(self, clip: str):
        c_clip = to_enum(f'viewport_clip_{clip}')
        dvz.visual_clip(self.c_visual, c_clip)

    def fixed(self, fixed):
        dvz.visual_fixed(self.c_visual, *get_fixed_params(fixed))

    # Counts
    # ---------------------------------------------------------------------------------------------

    def allocate(
        self,
        count: int,
    ):
        self._fn_alloc(self.c_visual, count)
        self.set_count(count)

    def set_count(self, count: int):
        self.count = count

    def get_count(self):
        return self.count

    # Internal
    # ---------------------------------------------------------------------------------------------

    def set_data(self, depth_test: bool = None, cull: str = None, **kwargs):
        if depth_test is not None:
            dvz.visual_depth(self.c_visual, depth_test)

        if cull is not None:
            dvz.visual_cull(self.c_visual, to_enum(f'CULL_MODE_{cull}'))

        for key, value in kwargs.items():
            fn = getattr(self, f'set_{key}', None)
            if fn:
                fn(value)
            else:
                raise ValueError(f"Method '{self.__class__.__name__}.set_{key}' not found")

    def set_prop_class(self, prop_name: str, prop_cls: type):
        self._prop_classes[prop_name] = prop_cls

    def set_prop_classes(self):
        pass

    def __getattr__(self, prop_name: str):
        # assert not prop_name.startswith('set_')
        # print(f"Calling __getattr__() with {self.visual_name}.{prop_name}")
        prop_type = PROPS[self.visual_name].get(prop_name, {}).get('type', None)
        if prop_type is None:
            print(f'Prop type {prop_name} not found')
            return super().__getattr__(prop_name)
        if prop_type == np.ndarray:
            prop_cls = self._prop_classes.get(prop_name, Prop)
            return prop_cls(self, prop_name)
        else:
            raise Exception(
                f"Prop '{prop_name}' is not a valid array property for visual {self.visual_name}"
            )

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
                assert isinstance(value, Texture)
                # if isinstance(value, np.ndarray):
                #     texture = self.app.texture(value)
                values = (value.c_tex, value.c_sampler)

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
                f"for visual '{self.visual_name}'"
            )


class Prop:
    visual: Visual = None
    visual_name: str = ''
    prop_name: str = ''
    _fn: tp.Callable = None

    def __init__(self, visual: Visual, prop_name: str):
        assert visual
        assert prop_name

        self.visual = visual
        self.visual_name = visual.visual_name
        self.prop_name = prop_name

        self._fn = getattr(dvz, f'{visual.visual_name}_{prop_name}', None)

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

    def prepare_data(self, value, size: int):
        if not isinstance(value, np.ndarray):
            # if doing visual.prop[idx] = scalar, need to create an array
            return prepare_data_scalar(self.name, self.dtype, size, value)
        else:
            # otherwise, just need to prepare the array with the right shape and dtype
            return prepare_data_array(self.name, self.dtype, self.shape, value)

    def set(self, offset, length, pvalue, c_flags: int = 0):
        self.call(self.visual.c_visual, offset, length, pvalue, c_flags)

    def call(self, *args):
        return self._fn(*args)

    def allocate(self, count: int):
        self.visual.allocate(count)

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

    def set_size(self, value: float):
        self.size = value


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

    def set_position(self, position: list[np.ndarray], n_groups: int = 0, offset: int = 0):
        if isinstance(position, np.ndarray):
            if n_groups is not None:
                k = position.shape[0] // n_groups
                position = [position[i * k : (i + 1) * k] for i in range(n_groups)]
            elif position.ndim == 2:
                position = list(position)
            elif position.ndim == 3:
                position = list(position)
        point_count = sum(map(len, position))
        path_count = len(position)
        path_lengths = np.array([len(p) for p in position], dtype=np.uint32)

        position_concat = np.vstack(position).astype(np.float32)
        assert position_concat.ndim == 2
        assert position_concat.shape[1] == 3
        position_concat = prepare_data_array(
            self.visual_name, np.float32, (-1, 3), position_concat
        )

        dvz.path_alloc(self.c_visual, point_count)
        dvz.path_position(
            self.c_visual, offset, point_count, position_concat, path_count, path_lengths, 0
        )

    def set_color(self, array: np.ndarray, offset: int = 0):
        self.color[offset:] = array

    def set_linewidth(self, array: np.ndarray, offset: int = 0):
        self.linewidth[offset:] = array

    def set_cap(self, value: str):
        self.cap = value

    def set_join(self, value: str):
        self.join = value


class Glyph(Visual):
    visual_name = 'glyph'
    _af = None

    def __init__(self, *args, font_size: int = None, **kwargs):
        super().__init__(*args, **kwargs)
        self._af = dvz.AtlasFont()
        dvz.atlas_font(font_size, self._af)
        dvz.glyph_atlas_font(self.c_visual, self._af)

    def set_strings(
        self,
        strings: list[str],
        string_pos: np.ndarray = None,
        scales: np.ndarray = None,
        color: tuple = cst.DEFAULT_GLYPH_COLOR,
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


class MeshIndexProp(Prop):
    def allocate(self, count: int):
        # NOTE: set_index() results in allocating the index buffer, not the vertex buffer.
        self.visual.allocate(self.visual.count, count)


class Mesh(Visual):
    visual_name = 'mesh'
    index_count: int = None

    def set_prop_classes(self):
        self.set_prop_class('index', MeshIndexProp)

    def set_data(
        self,
        vertex_count: int = None,
        index_count: int = None,
        compute_normals: bool = None,
        **kwargs,
    ):
        if 'position' in kwargs and 'index' in kwargs:
            nv, ni = kwargs['position'].shape[0], kwargs['index'].size
            self.allocate(nv, ni)

            # Automatic normal computation.
            if compute_normals:
                normals = np.zeros((nv, 3), dtype=np.float32)
                dvz.compute_normals(nv, ni, kwargs['position'], kwargs['index'], normals)
                kwargs['normal'] = normals

        if vertex_count is not None and index_count is not None:
            self.allocate(vertex_count, index_count)

        super().set_data(**kwargs)

    def allocate(self, count: int, index_count: int = None):
        if index_count is not None:
            dvz.mesh_alloc(self.c_visual, count, index_count)
            self.set_count(count, index_count)

    def set_count(self, count: int, index_count: int = None):
        self.count = count
        self.index_count = index_count

    def get_index_count(self):
        return self.index_count

    # Setters
    # ---------------------------------------------------------------------------------------------

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
        self.index_count = array.size
        self.allocate(self.count, self.index_count)
        self.index[offset:] = array

    def set_light_dir(self, value: tuple, idx: int = 0):
        value = value if value is not None else cst.DEFAULT_LIGHT_DIR
        dvz.mesh_light_dir(self.c_visual, idx, dvz.vec3(*value))

    def set_light_color(self, value: tuple, idx: int = 0):
        value = value if value is not None else cst.DEFAULT_LIGHT_COLOR
        dvz.mesh_light_color(self.c_visual, idx, dvz.cvec4(*value))

    def set_light_params(self, value: tuple, idx: int = 0):
        value = value if value is not None else cst.DEFAULT_LIGHT_PARAMS
        dvz.mesh_light_params(self.c_visual, idx, dvz.vec4(*value))

    def set_edgecolor(self, value: tuple):
        self.edgecolor = value

    def set_linewidth(self, value: float):
        self.linewidth = value

    def set_density(self, value: int):
        self.density = value

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
        dvz.volume_bounds(self.c_visual, dvz.vec2(*xlim), dvz.vec2(*ylim), dvz.vec2(*zlim))

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
