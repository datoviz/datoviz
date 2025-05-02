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
from . import _ctypes as dvz
from . import _constants as cst
from .base import Visual, Prop, Texture
from .utils import prepare_data_array


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

    def set_position(self, position: tp.List[np.ndarray], n_groups: int = 0, offset: int = 0):
        if isinstance(position, np.ndarray):
            if n_groups is not None:
                k = position.shape[0] // n_groups
                position = [position[i*k: (i+1)*k] for i in range(n_groups)]
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
            self.visual_name, np.float32, (-1, 3), position_concat)

        dvz.path_alloc(self.c_visual, point_count)
        dvz.path_position(self.c_visual, offset, point_count,
                          position_concat, path_count, path_lengths, 0)

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
            self, strings: tp.List[str],
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

    def set_data(self, vertex_count: int = None, index_count: int = None, compute_normals: bool = None, **kwargs):
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
