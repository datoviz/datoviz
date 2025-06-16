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
from typing import List, Optional, Tuple, Union

import numpy as np

from . import _constants as cst
from . import _ctypes as dvz
from ._props import PROPS
from ._texture import Texture
from .utils import (
    get_fixed_flag,
    get_size,
    is_enumerable,
    prepare_data_array,
    prepare_data_scalar,
    to_enum,
)

# -------------------------------------------------------------------------------------------------
# Base Visual class
# -------------------------------------------------------------------------------------------------


class Visual:
    """
    Base class for all visuals.

    Attributes
    ----------
    c_visual : dvz.DvzVisual
        The underlying C visual object.
    visual_name : str
        The name of the visual's elements.
    count : int
        The number of elements in the visual.
    _prop_classes : dict
        A dictionary mapping property names to their classes.
    _fn_alloc : Callable
        The allocation function for the visual.
    """

    c_visual: dvz.DvzVisual = None
    visual_name: str = ''
    count: int = 0

    _prop_classes: dict = None
    _fn_alloc: tp.Callable = None

    def __init__(self, c_visual: dvz.DvzVisual, visual_name: str = None) -> None:
        """
        Initialize a Visual instance.

        Parameters
        ----------
        c_visual : dvz.DvzVisual
            The underlying C visual object.
        visual_name : str, optional
            The name of the visual, by default None.
        """
        assert c_visual

        if visual_name:
            assert visual_name in PROPS
            self.__dict__['visual_name'] = visual_name

        self.c_visual = c_visual
        self._prop_classes = {}

        self._fn_alloc = getattr(dvz, f'{self.visual_name}_alloc', None)

        self.set_prop_classes()

    # Internals
    # ---------------------------------------------------------------------------------------------

    def set_data(self, depth_test: bool = None, cull: str = None, **kwargs) -> None:
        """
        Set data for the visual.

        Parameters
        ----------
        depth_test : bool, optional
            Whether to enable depth testing.
        cull : str, optional
            The culling mode, `None`, `front`, or `back`.
        **kwargs
            Additional data to set.
        """
        if depth_test is not None:
            dvz.visual_depth(self.c_visual, depth_test)

        if cull is not None:
            dvz.visual_cull(self.c_visual, to_enum(f'CULL_MODE_{cull}'))

        for key, value in kwargs.items():
            fn = getattr(self, f'set_{key}', None)
            if fn:
                if value is not None:
                    fn(value)
            else:
                raise ValueError(f"Method '{self.__class__.__name__}.set_{key}' not found")

    def set_prop_class(self, prop_name: str, prop_cls: type) -> None:
        """
        Set the class for a property.

        Parameters
        ----------
        prop_name : str
            The name of the property.
        prop_cls : type
            The class of the property.
        """
        self._prop_classes[prop_name] = prop_cls

    def set_prop_classes(self) -> None:
        """
        Set the classes for all properties.
        """

    def __getattr__(self, prop_name: str) -> tp.Any:
        """
        Get a property of the visual's elements.

        Parameters
        ----------
        prop_name : str
            The name of the property.

        Returns
        -------
        Any
            The property value.
        """
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

    def __setattr__(self, prop_name: str, value: tp.Any) -> None:
        """
        Set a property of the visual's elements.

        Parameters
        ----------
        prop_name : str
            The name of the property.
        value : Any
            The value to set.
        """
        prop_info = PROPS.get(self.visual_name, {}).get(prop_name, {})
        prop_type = prop_info.get('type', None)
        if not prop_type:
            # print(f'Prop {prop_name} not found in visual {self.visual_name}')
            return super().__setattr__(prop_name, value)

        elif prop_type != np.ndarray:
            prop_cls = self._prop_classes.get(prop_name, Prop)
            prop = prop_cls(self, prop_name)

            if value is None:
                return

            if prop_type == 'enum':
                enum_prefix = prop_info['enum']
                enum_prefix = enum_prefix.replace('DVZ_', '')
                value = to_enum(f'{enum_prefix}_{value}')
                values = (value,)

            elif prop_type == 'texture':
                assert isinstance(value, Texture)
                values = (value.c_tex, value.c_sampler)

            elif prop_type in dvz.VEC_TYPES:
                assert hasattr(value, '__len__')
                value = prop_type(*value)
                values = (value,)

            else:
                value = prop_type(value)
                values = (value,)

            prop.call(self.c_visual, *values)

        else:
            raise Exception(
                f"Prop '{prop_name}' is not a valid scalar property "
                f"for visual '{self.visual_name}'"
            )

    # Public properties
    # ---------------------------------------------------------------------------------------------

    def update(self) -> None:
        """
        Update the visual.
        """
        dvz.visual_update(self.c_visual)

    def show(self, is_visible: bool = True) -> None:
        """
        Show or hide the visual.

        Parameters
        ----------
        is_visible : bool, optional
            Whether to show the visual, by default True.
        """
        dvz.visual_show(self.c_visual, is_visible)

    def hide(self) -> None:
        """
        Hide the visual.
        """
        self.show(False)

    def clip(self, clip: str) -> None:
        """
        Set the clipping mode for the visual.

        Parameters
        ----------
        clip : str
            The clipping mode:
            - `inner` (clip everything inside the internal viewport)
            - `outer` (clip everything outside the interval viewport)
            - `bottom` (clip everything below the inferior border of the internal viewport)
            - `left` (clip everything to the left of the internal viewport)
        """
        c_clip = to_enum(f'viewport_clip_{clip}')
        dvz.visual_clip(self.c_visual, c_clip)

    def fixed(self, fixed: tp.Union[bool, str]) -> None:
        """
        Set whether the visual is fixed along certain axes.

        Parameters
        ----------
        fixed : bool or str
            Use True to fix all x, y, z dimensions, or `x` or `x,y` etc to fix only some of the
            axes.
        """
        dvz.visual_fixed(self.c_visual, get_fixed_flag(fixed))

    # Elements
    # ---------------------------------------------------------------------------------------------

    def allocate(self, count: int) -> None:
        """
        Allocate memory for the visual.

        Parameters
        ----------
        count : int
            The number of elements to allocate.
        """
        self._fn_alloc(self.c_visual, count)
        self.set_count(count)

    def set_count(self, count: int) -> None:
        """
        Set the number of elements in the visual.

        Parameters
        ----------
        count : int
            The number of elements.
        """
        self.count = count

    def get_count(self) -> int:
        """
        Get the number of elements in the visual.

        Returns
        -------
        int
            The number of elements.
        """
        return self.count


# -------------------------------------------------------------------------------------------------
# Base Prop class
# -------------------------------------------------------------------------------------------------


class Prop:
    """
    Represents a property of a visual.

    Attributes
    ----------
    visual : Visual
        The visual to which the property belongs.
    visual_name : str
        The name of the visual's elements.
    prop_name : str
        The name of the property.
    _fn : Callable
        The function to set the property.
    """

    visual: Visual = None
    visual_name: str = ''
    prop_name: str = ''
    _fn: tp.Callable = None

    def __init__(self, visual: Visual, prop_name: str) -> None:
        """
        Initialize a Prop instance.

        Parameters
        ----------
        visual : Visual
            The visual to which the property belongs.
        prop_name : str
            The name of the property.
        """
        assert visual
        assert prop_name

        self.visual = visual
        self.visual_name = visual.visual_name
        self.prop_name = prop_name

        self._fn = getattr(dvz, f'{visual.visual_name}_{prop_name}', None)

    @property
    def dtype(self) -> tp.Optional[type]:
        """
        Get the data type of the property.

        Returns
        -------
        type or None
            The data type of the property.
        """
        info = PROPS[self.visual_name][self.prop_name]
        return info.get('dtype', None)

    @property
    def shape(self) -> tp.Optional[Tuple[int, ...]]:
        """
        Get the shape of the property.

        Returns
        -------
        tuple of int or None
            The shape of the property.
        """
        info = PROPS[self.visual_name][self.prop_name]
        return info.get('shape', None)

    @property
    def size(self) -> int:
        """
        Get the size of the property.

        Returns
        -------
        int
            The size of the property.
        """
        return self.visual.get_count()

    @property
    def name(self) -> str:
        """
        Get the full name of the property.

        Returns
        -------
        str
            The full name of the property.
        """
        return f'{self.visual_name}.{self.prop_name}'

    def prepare_data(self, value: tp.Any, size: int) -> np.ndarray:
        """
        Prepare data for the property.

        Parameters
        ----------
        value : Any
            The value to prepare.
        size : int
            The size of the data.

        Returns
        -------
        np.ndarray
            The prepared data.
        """
        if isinstance(value, list):
            return self.prepare_data(np.asanyarray(value), size)
        elif not isinstance(value, np.ndarray):
            return prepare_data_scalar(self.name, self.dtype, size, value)
        else:
            return prepare_data_array(self.name, self.dtype, self.shape, value)

    def set(self, offset: int, length: int, pvalue: np.ndarray, c_flags: int = 0) -> None:
        """
        Set the property value.

        Parameters
        ----------
        offset : int
            The offset at which to start setting the value.
        length : int
            The length of the value.
        pvalue : np.ndarray
            The value to set.
        c_flags : int, optional
            Additional flags, by default 0.
        """
        self.call(self.visual.c_visual, offset, length, pvalue, c_flags)

    def call(self, *args) -> tp.Any:
        """
        Call the property setter function.

        Parameters
        ----------
        *args
            The arguments to pass to the function.

        Returns
        -------
        Any
            The result of the function call.
        """
        return self._fn(*args)

    def allocate(self, count: int) -> None:
        """
        Allocate memory for the property's visual.

        Parameters
        ----------
        count : int
            The number of elements to allocate.
        """
        self.visual.allocate(count)

    def __setitem__(self, idx: tp.Union[int, slice], value: tp.Any) -> None:
        """
        Set a value for a slice of the property.

        Parameters
        ----------
        idx : int or slice
            The index or slice to set.
        value : Any
            The value to set.
        """
        if value is None:
            return

        offset = idx.start if isinstance(idx, slice) else 0
        size = get_size(idx, value, total_size=self.size)
        count = offset + size
        assert offset >= 0
        assert count > 0

        pvalue = self.prepare_data(value, size)

        self.allocate(count)

        self.set(offset, size, pvalue)


# -------------------------------------------------------------------------------------------------
# Basic visual
# -------------------------------------------------------------------------------------------------


class Basic(Visual):
    """
    A basic visual using the default graphical primitives (points, lines, triangles).
    """

    visual_name = 'basic'

    def set_position(self, array: np.ndarray, offset: int = 0) -> None:
        """
        Set the position of the visual's elements.

        Parameters
        ----------
        array : np.ndarray
            The position array.
        offset : int, optional
            The offset at which to start setting the position, by default 0.
        """
        self.position[offset:] = array

    def set_color(self, array: np.ndarray, offset: int = 0) -> None:
        """
        Set the color of the visual's elements.

        Parameters
        ----------
        array : np.ndarray
            The color array.
        offset : int, optional
            The offset at which to start setting the color, by default 0.
        """
        self.color[offset:] = array

    def set_group(self, array: np.ndarray, offset: int = 0) -> None:
        """
        Set the groups.

        Parameters
        ----------
        array : np.ndarray
            The group array.
        offset : int, optional
            The offset at which to start setting the group, by default 0.
        """
        self.group[offset:] = array

    def set_size(self, value: float) -> None:
        """
        Set the common size of the visual's elements.

        Parameters
        ----------
        value : float
            The size value.
        """
        self.size = value


# -------------------------------------------------------------------------------------------------
# Pixel visual
# -------------------------------------------------------------------------------------------------


class Pixel(Visual):
    """
    A visual for rendering individual pixels.
    """

    visual_name = 'pixel'

    def set_position(self, array: np.ndarray, offset: int = 0) -> None:
        """
        Set the position of the pixels.

        Parameters
        ----------
        array : np.ndarray
            The position array.
        offset : int, optional
            The offset at which to start setting the position, by default 0.
        """
        self.position[offset:] = array

    def set_color(self, array: np.ndarray, offset: int = 0) -> None:
        """
        Set the color of the pixels.

        Parameters
        ----------
        array : np.ndarray
            The color array.
        offset : int, optional
            The offset at which to start setting the color, by default 0.
        """
        self.color[offset:] = array

    def set_size(self, value: float) -> None:
        """
        Set the size of the pixels.

        Parameters
        ----------
        value : float
            The size value.
        """
        self.size = value


# -------------------------------------------------------------------------------------------------
# Point visual
# -------------------------------------------------------------------------------------------------


class Point(Visual):
    """
    A visual for rendering points.
    """

    visual_name = 'point'

    def set_position(self, array: np.ndarray, offset: int = 0) -> None:
        """
        Set the position of points.

        Parameters
        ----------
        array : np.ndarray
            The position array.
        offset : int, optional
            The offset at which to start setting the position, by default 0.
        """
        self.position[offset:] = array

    def set_color(self, array: np.ndarray, offset: int = 0) -> None:
        """
        Set the color of points.

        Parameters
        ----------
        array : np.ndarray
            The color array.
        offset : int, optional
            The offset at which to start setting the color, by default 0.
        """
        self.color[offset:] = array

    def set_size(self, array: np.ndarray, offset: int = 0) -> None:
        """
        Set the size of points.

        Parameters
        ----------
        array : np.ndarray
            The size array, in pixels.
        offset : int, optional
            The offset at which to start setting the size, by default 0.
        """
        self.size[offset:] = array


# -------------------------------------------------------------------------------------------------
# Marker visual
# -------------------------------------------------------------------------------------------------


class Marker(Visual):
    """
    A visual for rendering markers.
    """

    visual_name = 'marker'

    def set_position(self, array: np.ndarray, offset: int = 0) -> None:
        """
        Set the position of the markers.

        Parameters
        ----------
        array : np.ndarray
            The position array.
        offset : int, optional
            The offset at which to start setting the position, by default 0.
        """
        self.position[offset:] = array

    def set_color(self, array: np.ndarray, offset: int = 0) -> None:
        """
        Set the color of the markers.

        Parameters
        ----------
        array : np.ndarray
            The color array.
        offset : int, optional
            The offset at which to start setting the color, by default 0.
        """
        self.color[offset:] = array

    def set_size(self, array: np.ndarray, offset: int = 0) -> None:
        """
        Set the size of the markers.

        Parameters
        ----------
        array : np.ndarray
            The size array.
        offset : int, optional
            The offset at which to start setting the size, by default 0.
        """
        self.size[offset:] = array

    def set_angle(self, array: np.ndarray, offset: int = 0) -> None:
        """
        Set the angle of the markers.

        Parameters
        ----------
        array : np.ndarray
            The angle array.
        offset : int, optional
            The offset at which to start setting the angle, by default 0.
        """
        self.angle[offset:] = array

    def set_linewidth(self, value: float) -> None:
        """
        Set the common linewidth of the markers.

        Parameters
        ----------
        value : float
            The linewidth value.
        """
        self.linewidth = value

    def set_edgecolor(self, value: tuple) -> None:
        """
        Set the common edge color of the markers.

        Parameters
        ----------
        value : tuple
            The edge color value.
        """
        self.edgecolor = value

    def set_mode(self, value: str) -> None:
        """
        Set the marker mode.

        Parameters
        ----------
        value : str
            The mode value:
            - `code` (the shape is defined in the shader code)
            - `bitmap` (the marker is defined in a bitmap texture)
            - `sdf` (the marker shape is defined as a signed distance field)
            - `msdf` (the marker shaped is defined as a multichannel signed distance field)
        """
        self.mode = value

    def set_aspect(self, value: str) -> None:
        """
        Set the rendering style of the marker.

        This controls how the marker uses color, edgecolor, and linewidth.

        Parameters
        ----------
        value : str
            One of:
            - 'filled': fill only (no border)
            - 'stroke': border only (transparent interior)
            - 'outline': fill with border

        """
        self.aspect = value

    def set_shape(self, value: str) -> None:
        """
        Set the shape of the visual's elements.

        Parameters
        ----------
        value : str
            The shape value, one of:
            - `disc`
            - `asterisk`
            - `chevron`
            - `clover`
            - `club`
            - `cross`
            - `diamond`
            - `arrow`
            - `ellipse`
            - `hbar`
            - `heart`
            - `infinity`
            - `pin`
            - `ring`
            - `spade`
            - `square`
            - `tag`
            - `triangle`
            - `vbar`
            - `rounded_rect`
        """
        self.shape = value

    def set_tex_scale(self, value: float) -> None:
        """
        Set the common texture scale.

        Parameters
        ----------
        value : float
            The texture scale value.
        """
        self.tex_scale = value

    def set_texture(self, texture: Texture) -> None:
        """
        Set the texture with the bitmap, SDF, or MSDF data.

        Parameters
        ----------
        texture : Texture
            The texture object.
        """
        dvz.marker_texture(self.c_visual, texture.c_texture)


# -------------------------------------------------------------------------------------------------
# Segment visual
# -------------------------------------------------------------------------------------------------


class SegmentProp(Prop):
    """
    A property for segment visuals.
    """

    def prepare_data(
        self, value: Tuple[np.ndarray, np.ndarray], size: int
    ) -> Tuple[np.ndarray, np.ndarray]:
        """
        Prepare data for the segment property.

        Parameters
        ----------
        value : tuple of np.ndarray
            The initial and terminal positions.
        size : int
            The size of the data.

        Returns
        -------
        tuple of np.ndarray
            The prepared data.
        """
        initial, terminal = value
        pinitial = super().prepare_data(initial, size)
        pterminal = super().prepare_data(terminal, size)
        return pinitial, pterminal

    def set(
        self, offset: int, length: int, pvalue: Tuple[np.ndarray, np.ndarray], flags: int = 0
    ) -> None:
        """
        Set the segment property value.

        Parameters
        ----------
        offset : int
            The offset at which to start setting the value.
        length : int
            The length of the value.
        pvalue : tuple of np.ndarray
            The value to set.
        flags : int, optional
            Additional flags, by default 0.
        """
        initial, terminal = pvalue
        self.call(self.visual.c_visual, offset, length, initial, terminal, flags)


class Segment(Visual):
    """
    A visual for rendering line segments.
    """

    visual_name = 'segment'

    def set_prop_classes(self) -> None:
        """
        Set the property classes for the segment visual.
        """
        self.set_prop_class('position', SegmentProp)

    def set_position(
        self, initial: np.ndarray, terminal: np.ndarray = None, offset: int = 0
    ) -> None:
        """
        Set the position of the line segments.

        Parameters
        ----------
        initial : np.ndarray
            The initial positions.
        terminal : np.ndarray
            The terminal positions.
        offset : int, optional
            The offset at which to start setting the position, by default 0.
        """
        if terminal is None:
            assert isinstance(initial, tuple) and len(initial) == 2
            initial, terminal = initial
        n = initial.shape[0]
        self.set_count(offset + n)
        self.position[offset:] = initial, terminal

    def set_color(self, array: np.ndarray, offset: int = 0) -> None:
        """
        Set the color of line segments.

        Parameters
        ----------
        array : np.ndarray
            The color array.
        offset : int, optional
            The offset at which to start setting the color, by default 0.
        """
        self.color[offset:] = array

    def set_linewidth(self, array: np.ndarray, offset: int = 0) -> None:
        """
        Set the linewidth of line segments.

        Parameters
        ----------
        array : np.ndarray
            The linewidth array.
        offset : int, optional
            The offset at which to start setting the linewidth, by default 0.
        """
        self.linewidth[offset:] = array

    def set_shift(self, array: np.ndarray, offset: int = 0) -> None:
        """
        Set the pixel shift of line segments.

        Parameters
        ----------
        array : np.ndarray
            The shift array.
        offset : int, optional
            The offset at which to start setting the shift, by default 0.
        """
        self.shift[offset:] = array

    def set_cap(self, initial: str, terminal: str = None) -> None:
        """
        Set the cap of line segments:

        - `round`
        - `triangle_in`
        - `triangle_out`
        - `square`
        - `butt`

        Parameters
        ----------
        initial : str
            The initial cap.
        terminal : str
            The terminal cap.

        """
        if terminal is None:
            assert isinstance(initial, tuple) and len(initial) == 2
            initial, terminal = initial
        prop_info = PROPS[self.visual_name].get('cap', {})
        enum_prefix = prop_info['enum']
        enum_prefix = enum_prefix.replace('DVZ_', '')
        initial = to_enum(f'{enum_prefix}_{initial}')
        terminal = to_enum(f'{enum_prefix}_{terminal}')
        dvz.segment_cap(self.c_visual, initial, terminal)


# -------------------------------------------------------------------------------------------------
# Path visual
# -------------------------------------------------------------------------------------------------


class Path(Visual):
    """
    A visual for rendering paths.

    Attributes
    ----------
    visual_name : str
        The name of the visual, set to 'path'.
    """

    visual_name = 'path'

    def set_position(
        self,
        position: Union[np.ndarray, List[np.ndarray]],
        groups: Union[int, np.ndarray] = 0,
        offset: int = 0,
    ) -> None:
        """
        Set the position of the paths.

        Parameters
        ----------
        position : ndarray, or list of ndarray
            A list of arrays representing the positions of the paths.
        groups : int or ndarray, optional
            The number of uniformly-sized groups, by default 0, or the sizes of each group.
        offset : int, optional
            The offset at which to start setting the position, by default 0.
        """
        assert position is not None
        if isinstance(position, np.ndarray):
            if position.ndim == 3:
                position = list(position)
            elif groups is None:
                # By default: a single group
                position = [position]
            elif isinstance(groups, int):
                k = position.shape[0] // groups
                position = [position[i * k: (i + 1) * k] for i in range(groups)]
            elif is_enumerable(groups):
                indices = np.cumsum([0] + list(groups))
                position = [position[indices[i]: indices[i + 1]] for i in range(len(groups))]

        # Ensure we get a list of positions in the end.
        assert isinstance(position, list)
        point_count = sum(map(len, position))
        path_count = len(position)
        path_lengths = np.array([len(p) for p in position], dtype=np.uint32)

        # Concatenation of all positions.
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

    def set_color(self, array: np.ndarray, offset: int = 0) -> None:
        """
        Set the color of the individual points in each path.

        Parameters
        ----------
        array : np.ndarray
            The color array.
        offset : int, optional
            The offset at which to start setting the color, by default 0.
        """
        self.color[offset:] = array

    def set_linewidth(self, array: np.ndarray, offset: int = 0) -> None:
        """
        Set the linewidth of the individual points in each path.

        Parameters
        ----------
        array : np.ndarray
            The linewidth array.
        offset : int, optional
            The offset at which to start setting the linewidth, by default 0.
        """
        self.linewidth[offset:] = array

    def set_cap(self, value: str) -> None:
        """
        Set the common cap style of all paths, one of the following:

        - `round`
        - `triangle_in`
        - `triangle_out`
        - `square`
        - `butt`

        Parameters
        ----------
        value : str
            The cap style.
        """
        self.cap = value

    def set_join(self, value: str) -> None:
        """
        Set the common join style of all paths, one of the following:

        - `square`
        - `round`

        Parameters
        ----------
        value : str
            The join style.
        """
        self.join = value


# -------------------------------------------------------------------------------------------------
# Glyph visual
# -------------------------------------------------------------------------------------------------


class Glyph(Visual):
    """
    A visual for rendering glyphs.

    Attributes
    ----------
    visual_name : str
        The name of the visual, set to 'glyph'.
    """

    visual_name = 'glyph'
    _af = None

    def __init__(self, *args, font_size: int = None, **kwargs) -> None:
        """
        Initialize a Glyph visual.

        Parameters
        ----------
        args : tuple
            Positional arguments for the parent class `Visual`.
        font_size : int, optional
            The font size, by default None.
        kwargs : dict
            Keyword arguments for the parent class `Visual`.
        """
        super().__init__(*args, **kwargs)
        self._af = dvz.AtlasFont()
        dvz.atlas_font(font_size, self._af)
        dvz.glyph_atlas_font(self.c_visual, self._af)

    def set_strings(
        self,
        strings: List[str],
        string_pos: np.ndarray = None,
        scales: np.ndarray = None,
        color: tuple = cst.DEFAULT_GLYPH_COLOR,
        anchor: tuple = (0, 0),
        offset: tuple = (0, 0),
    ) -> None:
        """
        Set the strings to render as glyphs.

        This is a helper function to set several strings at different locations and scales but
        with the same color, anchor, and offset. To use different values for each string, use the
        other methods.

        Parameters
        ----------
        strings : list of str
            The list of strings to render.
        string_pos : np.ndarray, optional
            The positions of each string, by default None.
        scales : np.ndarray, optional
            The scales of each string, by default None.
        color : tuple, optional
            The common color of all glyphs, by default cst.DEFAULT_GLYPH_COLOR.
        anchor : tuple, optional
            The common anchor point of all strings, by default (0, 0).
        offset : tuple, optional
            The common offset of all strings, by default (0, 0).
        """
        assert strings

        string_count = len(strings)

        if string_pos is None:
            string_pos = np.zeros((string_count, 3), dtype=np.float32)

        if scales is None:
            scales = np.ones(string_count, dtype=np.float32)

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

    def set_position(self, array: np.ndarray, offset: int = 0) -> None:
        """
        Set the position of each glyph.

        Parameters
        ----------
        array : np.ndarray
            The position array.
        offset : int, optional
            The offset at which to start setting the position, by default 0.
        """
        self.position[offset:] = array

    def set_axis(self, array: np.ndarray, offset: int = 0) -> None:
        """
        Set the 3D rotation axis of the glyphs.

        Parameters
        ----------
        array : np.ndarray
            The axis array.
        offset : int, optional
            The offset at which to start setting the axis, by default 0.

        Warnings:
        --------
        This is not implemented yet.

        """
        self.axis[offset:] = array

    def set_size(self, array: np.ndarray, offset: int = 0) -> None:
        """
        Set the size of each glyph.

        Parameters
        ----------
        array : np.ndarray
            The size array.
        offset : int, optional
            The offset at which to start setting the size, by default 0.
        """
        self.size[offset:] = array

    def set_anchor(self, array: np.ndarray, offset: int = 0) -> None:
        """
        Set the anchor of each glyph.

        Parameters
        ----------
        array : np.ndarray
            The anchor array.
        offset : int, optional
            The offset at which to start setting the anchor, by default 0.
        """
        self.anchor[offset:] = array

    def set_shift(self, array: np.ndarray, offset: int = 0) -> None:
        """
        Set the pixel shift of each glyph.

        Parameters
        ----------
        array : np.ndarray
            The shift array.
        offset : int, optional
            The offset at which to start setting the shift, by default 0.
        """
        self.shift[offset:] = array

    def set_texcoords(self, array: np.ndarray, offset: int = 0) -> None:
        """
        Set the texture coordinates of each glyph (relative to the font atlas texture).

        Parameters
        ----------
        array : np.ndarray
            The texture coordinates array.
        offset : int, optional
            The offset at which to start setting the texture coordinates, by default 0.
        """
        self.texcoords[offset:] = array

    def set_group_size(self, array: np.ndarray, offset: int = 0) -> None:
        """
        Set the string size of each glyph (the value should be the same across all glyphs of
        each string).

        Parameters
        ----------
        array : np.ndarray
            The group size array.
        offset : int, optional
            The offset at which to start setting the group size, by default 0.
        """
        self.group_size[offset:] = array

    def set_scale(self, array: np.ndarray, offset: int = 0) -> None:
        """
        Set the scale of each glyph.

        Parameters
        ----------
        array : np.ndarray
            The scale array.
        offset : int, optional
            The offset at which to start setting the scale, by default 0.
        """
        self.scale[offset:] = array

    def set_angle(self, array: np.ndarray, offset: int = 0) -> None:
        """
        Set the angle of each glyph.

        Parameters
        ----------
        array : np.ndarray
            The angle array.
        offset : int, optional
            The offset at which to start setting the angle, by default 0.
        """
        self.angle[offset:] = array

    def set_color(self, array: np.ndarray, offset: int = 0) -> None:
        """
        Set the color of each glyph.

        Parameters
        ----------
        array : np.ndarray
            The color array.
        offset : int, optional
            The offset at which to start setting the color, by default 0.
        """
        self.color[offset:] = array

    def set_bgcolor(self, value: tuple) -> None:
        """
        Set the common background color for all glyphs.

        Parameters
        ----------
        value : tuple
            The background color value.
        """
        self.bgcolor = value

    def set_texture(self, texture: Texture) -> None:
        """
        Set the texture with the font atlas.

        Parameters
        ----------
        texture : Texture
            The texture object.
        """
        dvz.glyph_texture(self.c_visual, texture.c_texture)


# -------------------------------------------------------------------------------------------------
# Image visual
# -------------------------------------------------------------------------------------------------


class Image(Visual):
    """
    A visual for displaying images.

    Attributes
    ----------
    visual_name : str
        The name of the visual, set to 'image'.
    """

    visual_name = 'image'

    def set_position(self, array: np.ndarray, offset: int = 0) -> None:
        """
        Set the position of each image.

        Parameters
        ----------
        array : np.ndarray
            The position array.
        offset : int, optional
            The offset at which to start setting the position, by default 0.
        """
        self.position[offset:] = array

    def set_size(self, array: np.ndarray, offset: int = 0) -> None:
        """
        Set the pixel size of each image.

        Parameters
        ----------
        array : np.ndarray
            The size array.
        offset : int, optional
            The offset at which to start setting the size, by default 0.
        """
        self.size[offset:] = array

    def set_anchor(self, array: np.ndarray, offset: int = 0) -> None:
        """
        Set the anchor of each image.

        Parameters
        ----------
        array : np.ndarray
            The anchor array.
        offset : int, optional
            The offset at which to start setting the anchor, by default 0.
        """
        self.anchor[offset:] = array

    def set_texcoords(self, array: np.ndarray, offset: int = 0) -> None:
        """
        Set the texture coordinates of each image.

        Parameters
        ----------
        array : np.ndarray
            The texture coordinates array.
        offset : int, optional
            The offset at which to start setting the texture coordinates, by default 0.
        """
        self.texcoords[offset:] = array

    def set_facecolor(self, array: np.ndarray, offset: int = 0) -> None:
        """
        Set the face color of the image (when using a fill mode instead of texture mode).

        Parameters
        ----------
        array : np.ndarray
            The face color array.
        offset : int, optional
            The offset at which to start setting the face color, by default 0.
        """
        self.facecolor[offset:] = array

    def set_edgecolor(self, value: tuple) -> None:
        """
        Set the common edge color of all images.

        Parameters
        ----------
        value : tuple
            The edge color value.
        """
        self.edgecolor = value

    def set_permutation(self, value: tuple) -> None:
        """
        Set the permutation axes of the image.

        Parameters
        ----------
        value : tuple
            The permutation value.
        """
        self.permutation = value

    def set_linewidth(self, value: float) -> None:
        """
        Set the common linewidth of all images when using an image border.

        Parameters
        ----------
        value : float
            The linewidth value.
        """
        self.linewidth = value

    def set_radius(self, value: float) -> None:
        """
        Set the common border radius of all images when using an image border.

        Parameters
        ----------
        value : float
            The radius value.
        """
        self.radius = value

    def set_colormap(self, value: str) -> None:
        """
        Set the colormap of the image when using the `colormap` image mode.

        Parameters
        ----------
        value : str
            The colormap value.
        """
        self.colormap = value

    def set_texture(self, texture: Texture) -> None:
        """
        Set the image texture.

        Parameters
        ----------
        texture : Texture
            The texture object.
        """
        dvz.image_texture(self.c_visual, texture.c_texture)


# -------------------------------------------------------------------------------------------------
# Wiggle visual
# -------------------------------------------------------------------------------------------------


class Wiggle(Visual):
    """
    A visual for displaying a wiggle plot.

    Attributes
    ----------
    visual_name : str
        The name of the visual, set to 'wiggle'.
    """

    visual_name = 'wiggle'

    def set_bounds(self, xlim: tuple, ylim: tuple = None) -> None:
        """
        Set the bounds of the wiggle plot.

        Parameters
        ----------
        xlim : tuple
            The x-axis bounds.
        ylim : tuple
            The y-axis bounds.
        """
        if ylim is None:
            xlim, ylim = xlim
        dvz.wiggle_bounds(self.c_visual, dvz.vec2(*xlim), dvz.vec2(*ylim))

    def set_color(
        self,
        negative: Tuple[float, float, float, float],
        positive: Optional[Tuple[float, float, float, float]] = None,
    ) -> None:
        """
        Set the color of the wiggle plot.

        Parameters
        ----------
        negative : tuple
            The color for negative values.
        positive : tuple
            The color for positive values.
        """
        if hasattr(negative[0], '__len__'):
            negative, positive = negative
        dvz.wiggle_color(self.c_visual, dvz.cvec4(*negative), dvz.cvec4(*positive))

    def set_edgecolor(self, value: Tuple[int, int, int, int]) -> None:
        """
        Set the line color.

        Parameters
        ----------
        value : tuple
            The edge color value.
        """
        self.edgecolor = value

    def set_xrange(self, xrange: Tuple[float, float]) -> None:
        """
        Set the x-axis range of the wiggle plot.

        Parameters
        ----------
        xrange : tuple
            The x-axis range.
        """
        self.xrange = xrange

    def set_scale(self, scale: float) -> None:
        """
        Set the wiggle scale.

        Parameters
        ----------
        scale : float
            The scale factor for the wiggle plot.
        """
        self.scale = scale

    def set_texture(self, texture: Texture) -> None:
        """
        Set the wiggle texture.

        Parameters
        ----------
        texture : Texture
            The texture object.
        """
        dvz.wiggle_texture(self.c_visual, texture.c_texture)


# -------------------------------------------------------------------------------------------------
# Mesh visual
# -------------------------------------------------------------------------------------------------


class MeshIndexProp(Prop):
    """
    A property for mesh indices.
    """

    def allocate(self, count: int) -> None:
        """
        Allocate memory for the mesh indices.

        Parameters
        ----------
        count : int
            The number of elements to allocate.
        """
        self.visual.allocate(self.visual.count, count)


class Mesh(Visual):
    """
    A visual for rendering meshes.

    Attributes
    ----------
    visual_name : str
        The name of the visual, set to 'mesh'.
    index_count : int
        The number of indices in the mesh.
    """

    visual_name = 'mesh'
    index_count: int = None

    def set_prop_classes(self) -> None:
        """
        Set the property classes for the mesh visual.
        """
        self.set_prop_class('index', MeshIndexProp)

    def set_data(
        self,
        vertex_count: int = None,
        index_count: int = None,
        compute_normals: bool = None,
        **kwargs,
    ) -> None:
        """
        Set data for the mesh.

        Parameters
        ----------
        vertex_count : int, optional
            The number of vertices, by default None.
        index_count : int, optional
            The number of indices, by default None.
        compute_normals : bool, optional
            Whether to compute normals, by default None.
        **kwargs
            Additional data to set.
        """
        if 'position' in kwargs and 'index' in kwargs:
            nv, ni = kwargs['position'].shape[0], kwargs['index'].size
            self.allocate(nv, ni)

            # Automatic normal computation.
            if compute_normals:
                normals = np.zeros((nv, 3), dtype=np.float32)
                dvz.compute_normals(
                    nv,
                    ni,
                    kwargs['position'].astype(np.float32),
                    kwargs['index'].astype(np.uint32),
                    normals,
                )
                kwargs['normal'] = normals

        if vertex_count is not None and index_count is not None:
            self.allocate(vertex_count, index_count)

        super().set_data(**kwargs)

    def allocate(self, count: int, index_count: int = None) -> None:
        """
        Allocate memory for the mesh.

        Parameters
        ----------
        count : int
            The number of vertices to allocate.
        index_count : int, optional
            The number of indices to allocate, by default None.
        """
        if index_count is not None:
            dvz.mesh_alloc(self.c_visual, count, index_count)
            self.set_count(count, index_count)

    def set_count(self, count: int, index_count: int = None) -> None:
        """
        Set the number of vertices and indices in the mesh.

        Parameters
        ----------
        count : int
            The number of vertices.
        index_count : int, optional
            The number of indices, by default None.
        """
        self.count = count
        self.index_count = index_count

    def get_index_count(self) -> int:
        """
        Get the number of indices in the mesh.

        Returns
        -------
        int
            The number of indices.
        """
        return self.index_count

    def set_position(self, array: np.ndarray, offset: int = 0) -> None:
        """
        Set the position of the mesh vertices.

        Parameters
        ----------
        array : np.ndarray
            The position array.
        offset : int, optional
            The offset at which to start setting the position, by default 0.
        """
        self.position[offset:] = array

    def set_color(self, array: np.ndarray, offset: int = 0) -> None:
        """
        Set the color of the mesh vertices.

        Parameters
        ----------
        array : np.ndarray
            The color array.
        offset : int, optional
            The offset at which to start setting the color, by default 0.
        """
        self.color[offset:] = array

    def set_texcoords(self, array: np.ndarray, offset: int = 0) -> None:
        """
        Set the texture coordinates of the mesh vertices.

        Parameters
        ----------
        array : np.ndarray
            The texture coordinates array.
        offset : int, optional
            The offset at which to start setting the texture coordinates, by default 0.
        """
        self.texcoords[offset:] = array

    def set_normal(self, array: np.ndarray, offset: int = 0) -> None:
        """
        Set the normals of the mesh vertices.

        Parameters
        ----------
        array : np.ndarray
            The normals array.
        offset : int, optional
            The offset at which to start setting the normals, by default 0.
        """
        self.normal[offset:] = array

    def set_isoline(self, array: np.ndarray, offset: int = 0) -> None:
        """
        Set the isolines of the mesh vertices.

        Parameters
        ----------
        array : np.ndarray
            The isolines array.
        offset : int, optional
            The offset at which to start setting the isolines, by default 0.
        """
        self.isoline[offset:] = array

    def set_index(self, array: np.ndarray, offset: int = 0) -> None:
        """
        Set the indices of the mesh.

        Parameters
        ----------
        array : np.ndarray
            The indices array.
        offset : int, optional
            The offset at which to start setting the indices, by default 0.
        """
        self.index_count = array.size
        self.allocate(self.count, self.index_count)
        self.index[offset:] = array

    def set_light_pos(self, value: tuple, idx: int = 0) -> None:
        """
        Set the direction of the light.

        Parameters
        ----------
        value : tuple
            The light direction value.
        idx : int, optional
            The index of the light, by default 0.
        """
        value = value if value is not None else cst.DEFAULT_LIGHT_POS
        dvz.mesh_light_pos(self.c_visual, idx, dvz.vec4(*value))

    def set_light_color(self, value: tuple, idx: int = 0) -> None:
        """
        Set the color of the light.

        Parameters
        ----------
        value : tuple
            The light color value.
        idx : int, optional
            The index of the light, by default 0.
        """
        value = value if value is not None else cst.DEFAULT_LIGHT_COLOR
        dvz.mesh_light_color(self.c_visual, idx, dvz.cvec4(*value))

    def set_ambient_params(self, value: tuple) -> None:
        """
        Set the ambient parameters of the material.

        Parameters
        ----------
        value : tuple
            The material ambient parameters (r, g, b).
        """
        value = value if value is not None else cst.DEFAULT_AMBIENT_PARAMS
        dvz.mesh_material_params(self.c_visual, 0, dvz.vec3(*value))

    def set_diffuse_params(self, value: tuple) -> None:
        """
        Set the diffuse parameters of the material.

        Parameters
        ----------
        value : tuple
            The material diffuse parameters (r, g, b).
        """
        value = value if value is not None else cst.DEFAULT_DIFFUSE_PARAMS
        dvz.mesh_material_params(self.c_visual, 1, dvz.vec3(*value))

    def set_specular_params(self, value: tuple) -> None:
        """
        Set the specular parameters of the material.

        Parameters
        ----------
        value : tuple
            The material specular parameters (r, g, b).
        """
        value = value if value is not None else cst.DEFAULT_SPECULAR_PARAMS
        dvz.mesh_material_params(self.c_visual, 2, dvz.vec3(*value))

    def set_emission_params(self, value: tuple) -> None:
        """
        Set the emission parameters of the material.

        Parameters
        ----------
        value : tuple
            The material emission parameters (r, g, b).
        """
        value = value if value is not None else cst.DEFAULT_EMISSION_PARAMS
        dvz.mesh_material_params(self.c_visual, 3, dvz.vec3(*value))

    def set_shine(self, value: float) -> None:
        """
        Set the surface shininess.

        Parameters
        ----------
        value : float
            The shine value.
        """
        self.shine = value

    def set_emit(self, value: float) -> None:
        """
        Set the surface emission level.

        Parameters
        ----------
        value : float
            The emit value.
        """
        self.emit = value

    def set_edgecolor(self, value: tuple) -> None:
        """
        Set the edge color of the contour, if showing a contour.

        Parameters
        ----------
        value : tuple
            The edge color value.
        """
        self.edgecolor = value

    def set_linewidth(self, value: float) -> None:
        """
        Set the line width of the contour, if showing a contour.

        Parameters
        ----------
        value : float
            The linewidth value.
        """
        self.linewidth = value

    def set_density(self, value: int) -> None:
        """
        Set the density of the isolines, if showing isolines.

        Parameters
        ----------
        value : int
            The density value.
        """
        self.density = value

    def set_texture(self, texture: Texture) -> None:
        """
        Set the mesh texture.

        Parameters
        ----------
        texture : Texture
            The texture object.
        """
        dvz.mesh_texture(self.c_visual, texture.c_texture)


# -------------------------------------------------------------------------------------------------
# 3D sphere visual
# -------------------------------------------------------------------------------------------------


class Sphere(Visual):
    """
    A visual for rendering spheres.

    Attributes
    ----------
    visual_name : str
        The name of the visual, set to 'sphere'.
    """

    visual_name = 'sphere'

    def set_position(self, array: np.ndarray, offset: int = 0) -> None:
        """
        Set the position of the spheres.

        Parameters
        ----------
        array : np.ndarray
            The position array.
        offset : int, optional
            The offset at which to start setting the position, by default 0.
        """
        self.position[offset:] = array

    def set_color(self, array: np.ndarray, offset: int = 0) -> None:
        """
        Set the color of the spheres.

        Parameters
        ----------
        array : np.ndarray
            The color array.
        offset : int, optional
            The offset at which to start setting the color, by default 0.
        """
        self.color[offset:] = array

    def set_size(self, array: np.ndarray, offset: int = 0) -> None:
        """
        Set the size of the spheres.

        Parameters
        ----------
        array : np.ndarray
            The size array.
        offset : int, optional
            The offset at which to start setting the size, by default 0.
        """
        self.size[offset:] = array

    def set_shine(self, value) -> None:
        """
        Set the size of the spheres.

        Parameters
        ----------
        value : float
            The amount of surface shininess.
        """
        self.shine = value

    def set_emit(self, value) -> None:
        """
        Set the size of the spheres.

        Parameters
        ----------
        value : float
            The amount of surface light emission.
        """
        self.emit = value

    def set_light_pos(self, value: tuple, idx: int = 0) -> None:
        """
        Set the position of light idx.

        Parameters
        ----------
        value : tuple
            The light position value.
        idx : int, optional
            The index of the light, by default 0.
        """
        value = value if value is not None else cst.DEFAULT_LIGHT_POS
        dvz.sphere_light_pos(self.c_visual, idx, dvz.vec4(*value))

    def set_light_color(self, value: tuple, idx: int = 0) -> None:
        """
        Set the color of the light idx.

        Parameters
        ----------
        value : tuple
            The light color value.
        idx : int, optional
            The index of the light, by default 0.
        """
        value = value if value is not None else cst.DEFAULT_LIGHT_COLOR
        dvz.sphere_light_color(self.c_visual, idx, dvz.cvec4(*value))

    def set_ambient_params(self, value: tuple) -> None:
        """
        Set the ambient parameters of the material.

        Parameters
        ----------
        value : tuple
            The material ambient parameters (r, g, b).
        """
        value = value if value is not None else cst.DEFAULT_MATERIAL_PARAMS
        dvz.sphere_material_params(self.c_visual, 0, dvz.vec3(*value))

    def set_diffuse_params(self, value: tuple) -> None:
        """
        Set the diffuse parameters of the material.

        Parameters
        ----------
        value : tuple
            The material diffuse parameters (r, g, b).
        """
        value = value if value is not None else cst.DEFAULT_MATERIAL_PARAMS
        dvz.sphere_material_params(self.c_visual, 1, dvz.vec3(*value))

    def set_specular_params(self, value: tuple) -> None:
        """
        Set the specular parameters of the material.

        Parameters
        ----------
        value : tuple
            The material specular parameters (r, g, b).
        """
        value = value if value is not None else cst.DEFAULT_MATERIAL_PARAMS
        dvz.sphere_material_params(self.c_visual, 2, dvz.vec3(*value))

    def set_emission_params(self, value: tuple) -> None:
        """
        Set the emission parameters of the material.

        Parameters
        ----------
        value : tuple
            The material emission parameters (r, g, b).
        """
        value = value if value is not None else cst.DEFAULT_MATERIAL_PARAMS
        dvz.sphere_material_params(self.c_visual, 3, dvz.vec3(*value))

    def set_texture(self, texture: Texture) -> None:
        """
        Set the sphere texture.

        Parameters
        ----------
        texture : Texture
            The texture object.
        """
        dvz.sphere_texture(self.c_visual, texture.c_texture)


# -------------------------------------------------------------------------------------------------
# Volume visual
# -------------------------------------------------------------------------------------------------


class Volume(Visual):
    """
    A visual for rendering volumetric data.

    Attributes
    ----------
    visual_name : str
        The name of the visual, set to 'volume'.
    """

    visual_name = 'volume'

    def set_bounds(self, xlim: tuple, ylim: tuple = None, zlim: tuple = None) -> None:
        """
        Set the bounds of the volume.

        Parameters
        ----------
        xlim : tuple
            The x-axis bounds.
        ylim : tuple
            The y-axis bounds.
        zlim : tuple
            The z-axis bounds.
        """
        if ylim is None and zlim is None:
            xlim, ylim, zlim = xlim
        dvz.volume_bounds(self.c_visual, dvz.vec2(*xlim), dvz.vec2(*ylim), dvz.vec2(*zlim))

    def set_texcoords(self, uvw0: tuple, uvw1: tuple) -> None:
        """
        Set the texture coordinates of the volume.

        Parameters
        ----------
        uvw0 : tuple
            The texture coordinates of the point `(xlim[0], ylim[0], zlim[0])`.
        uvw1 : tuple
            The texture coordinates of the point `(xlim[1], ylim[1], zlim[1])`.
        """
        dvz.volume_texcoords(self.c_visual, dvz.vec3(uvw0), dvz.vec3(uvw1))

    def set_permutation(self, value: tuple) -> None:
        """
        Set the axis permutation of the volume 3D array.

        Parameters
        ----------
        value : tuple
            The permutation of the axes, by default (0, 1, 2) (corresponding to u, v, w).
        """
        self.permutation = value

    def set_slice(self, value: int) -> None:
        """
        Set the slice index for the volume.

        Parameters
        ----------
        value : int
            The slice index.

        Warnings:
        --------
        This is not implemented yet.
        """
        self.slice = value

    def set_transfer(self, value: tuple) -> None:
        """
        Set the transfer function for the volume.

        Parameters
        ----------
        value : tuple
            The transfer function parameters.
        """
        self.transfer = value

    def set_texture(self, texture: Texture) -> None:
        """
        Set the texture for the volume.

        Parameters
        ----------
        texture : Texture
            The texture object.
        """
        dvz.volume_texture(self.c_visual, texture.c_texture)


# -------------------------------------------------------------------------------------------------
# Slice visual
# -------------------------------------------------------------------------------------------------


class Slice(Visual):
    """
    A visual for rendering 2D slices of volumetric data.

    Attributes
    ----------
    visual_name : str
        The name of the visual, set to 'slice'.
    """

    visual_name = 'slice'

    def set_position(self, array: np.ndarray, offset: int = 0) -> None:
        """
        Set the position of the slice.

        Parameters
        ----------
        array : np.ndarray
            The position array.
        offset : int, optional
            The offset at which to start setting the position, by default 0.
        """
        self.position[offset:] = array

    def set_texcoords(self, array: np.ndarray, offset: int = 0) -> None:
        """
        Set the texture coordinates of the slice.

        Parameters
        ----------
        array : np.ndarray
            The texture coordinates array.
        offset : int, optional
            The offset at which to start setting the texture coordinates, by default 0.
        """
        self.texcoords[offset:] = array

    def set_alpha(self, value: float) -> None:
        """
        Set the alpha transparency of the slice.

        Parameters
        ----------
        value : float
            The alpha transparency value.
        """
        self.alpha = value

    def set_texture(self, texture: Texture) -> None:
        """
        Set the texture for the slice.

        Parameters
        ----------
        texture : Texture
            The texture object.
        """
        dvz.slice_texture(self.c_visual, texture.c_texture)
