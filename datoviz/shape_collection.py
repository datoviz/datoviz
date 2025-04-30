"""
Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
Licensed under the MIT license. See LICENSE file in the project root for details.
SPDX-License-Identifier: MIT
"""

# Shape

# -------------------------------------------------------------------------------------------------
# Imports
# -------------------------------------------------------------------------------------------------

import ctypes
import typing as tp
import numpy as np
import datoviz as dvz


# -------------------------------------------------------------------------------------------------
# Types
# -------------------------------------------------------------------------------------------------

Color = tp.Tuple[int, int, int, int]
Vec4 = tp.Tuple[float, float, float, float]
Mat4 = tp.Tuple[float, ...]

DEFAULT_SIZE = 100
WHITE = dvz.cvec4(255, 255, 255, 255)


# -------------------------------------------------------------------------------------------------
# Utils
# -------------------------------------------------------------------------------------------------

def _shape_transform(c_shape: dvz.Shape, offset: tp.Tuple[float, float, float] = None, scale: float = None, transform: Mat4 = None):
    dvz.shape_begin(c_shape, 0, 0)
    if scale is not None:
        dvz.shape_scale(c_shape,  dvz.vec3(scale, scale, scale))
    if offset is not None:
        dvz.shape_translate(c_shape, dvz.vec3(*offset))
    # TODO
    # dvz.shape_rotate(c_shape, angle, axis)
    if transform is not None:
        dvz.shape_transform(c_shape, dvz.mat4(*transform))

    dvz.shape_end(c_shape)


def merge_shapes(c_shapes):
    merged = dvz.shape()
    n = len(c_shapes)
    if n == 0:
        return None
    else:
        array_type = ctypes.POINTER(dvz.Shape) * n
        shapes_array = array_type(*(s for s in c_shapes))
        dvz.shape_merge(merged, len(c_shapes), shapes_array)
        return merged


# -------------------------------------------------------------------------------------------------
# Shape collection
# -------------------------------------------------------------------------------------------------

class ShapeCollection:
    c_shapes = None

    def __init__(self):
        self.c_shapes = []
        self.c_merged = None

    def add(self, c_shape: dvz.Shape, offset: tp.Tuple[float, float, float] = None, scale: float = None, transform: Mat4 = None):
        _shape_transform(c_shape, offset=offset, scale=scale, transform=transform)
        self.c_shapes.append(c_shape)

    def add_square(self, offset: tp.Tuple[float, float, float] = None, scale: float = None, transform: Mat4 = None, color: Color = None):
        c_shape = dvz.shape()
        c_color = dvz.cvec4(*color) if color is not None else WHITE
        dvz.shape_square(c_shape, c_color)
        self.add(c_shape, offset=offset, scale=scale, transform=transform)

    def add_disc(self, size: int = None, offset: tp.Tuple[float, float, float] = None, scale: float = None, transform: Mat4 = None, color: Color = None):
        c_shape = dvz.shape()
        c_color = dvz.cvec4(*color) if color is not None else WHITE
        dvz.shape_disc(c_shape, size, c_color)
        self.add(c_shape, offset=offset, scale=scale, transform=transform)

    def add_cube(self, offset: tp.Tuple[float, float, float] = None, scale: float = None, transform: Mat4 = None, color: Color = None):
        c_shape = dvz.shape()
        colors = np.zeros((6, 4), dtype=np.uint8)
        for i in range(6):
            colors[i] = color if color is not None else WHITE
        dvz.shape_cube(c_shape, colors)
        self.add(c_shape, offset=offset, scale=scale, transform=transform)

    def add_sphere(self, rows: int = None, cols: int = None, offset: tp.Tuple[float, float, float] = None, scale: float = None, transform: Mat4 = None, color: Color = None):
        rows = rows or DEFAULT_SIZE
        cols = cols or DEFAULT_SIZE
        c_shape = dvz.shape()
        c_color = dvz.cvec4(*color) if color is not None else WHITE
        dvz.shape_sphere(c_shape, rows, cols, c_color)
        self.add(c_shape, offset=offset, scale=scale, transform=transform)

    def add_cone(self, size: int = None, offset: tp.Tuple[float, float, float] = None, scale: float = None, transform: Mat4 = None, color: Color = None):
        size = size or DEFAULT_SIZE
        c_shape = dvz.shape()
        c_color = dvz.cvec4(*color) if color is not None else WHITE
        dvz.shape_cone(c_shape, size, c_color)
        self.add(c_shape, offset=offset, scale=scale, transform=transform)

    def add_cylinder(self, size: int = None, offset: tp.Tuple[float, float, float] = None, scale: float = None, transform: Mat4 = None, color: Color = None):
        size = size or DEFAULT_SIZE
        c_shape = dvz.shape()
        c_color = dvz.cvec4(*color) if color is not None else WHITE
        dvz.shape_cylinder(c_shape, size, c_color)
        self.add(c_shape, offset=offset, scale=scale, transform=transform)

    def add_tetrahedron(self, offset: tp.Tuple[float, float, float] = None, scale: float = None, transform: Mat4 = None, color: Color = None):
        c_shape = dvz.shape()
        c_color = dvz.cvec4(*color) if color is not None else WHITE
        dvz.shape_tetrahedron(c_shape, c_color)

        # We need to unindex the mesh in order to get nice surface normals
        dvz.shape_unindex(c_shape, 0)

        # We compute the normals after unindexing.
        dvz.shape_normals(c_shape)

        self.add(c_shape, offset=offset, scale=scale, transform=transform)

    def add_hexahedron(self, offset: tp.Tuple[float, float, float] = None, scale: float = None, transform: Mat4 = None, color: Color = None):
        c_shape = dvz.shape()
        c_color = dvz.cvec4(*color) if color is not None else WHITE
        dvz.shape_hexahedron(c_shape, c_color)

        # We need to unindex the mesh in order to get nice surface normals
        dvz.shape_unindex(c_shape, 0)

        # We compute the normals after unindexing.
        dvz.shape_normals(c_shape)

        self.add(c_shape, offset=offset, scale=scale, transform=transform)

    def add_octahedron(self, offset: tp.Tuple[float, float, float] = None, scale: float = None, transform: Mat4 = None, color: Color = None):
        c_shape = dvz.shape()
        c_color = dvz.cvec4(*color) if color is not None else WHITE
        dvz.shape_octahedron(c_shape, c_color)

        # We need to unindex the mesh in order to get nice surface normals
        dvz.shape_unindex(c_shape, 0)

        # We compute the normals after unindexing.
        dvz.shape_normals(c_shape)

        self.add(c_shape, offset=offset, scale=scale, transform=transform)

    def add_dodecahedron(self, offset: tp.Tuple[float, float, float] = None, scale: float = None, transform: Mat4 = None, color: Color = None):
        c_shape = dvz.shape()
        c_color = dvz.cvec4(*color) if color is not None else WHITE
        dvz.shape_dodecahedron(c_shape, c_color)

        # We need to unindex the mesh in order to get nice surface normals
        dvz.shape_unindex(c_shape, 0)

        # We compute the normals after unindexing.
        dvz.shape_normals(c_shape)

        self.add(c_shape, offset=offset, scale=scale, transform=transform)

    def add_icosahedron(self, offset: tp.Tuple[float, float, float] = None, scale: float = None, transform: Mat4 = None, color: Color = None):
        c_shape = dvz.shape()
        c_color = dvz.cvec4(*color) if color is not None else WHITE
        dvz.shape_icosahedron(c_shape, c_color)

        # We need to unindex the mesh in order to get nice surface normals
        dvz.shape_unindex(c_shape, 0)

        # We compute the normals after unindexing.
        dvz.shape_normals(c_shape)

        self.add(c_shape, offset=offset, scale=scale, transform=transform)

    def add_polygon(self, points: np.ndarray, offset: tp.Tuple[float, float, float] = None, scale: float = None, transform: Mat4 = None, color: Color = None):
        assert points.ndim == 2
        assert points.shape[1] == 2
        c_shape = dvz.shape()
        c_color = dvz.cvec4(*color) if color is not None else WHITE
        dvz.shape_polygon(c_shape, points.shape[0], points, c_color)
        self.add(c_shape, offset=offset, scale=scale, transform=transform)

    def add_surface(self, heights: np.ndarray, colors: np.ndarray, u: tp.Tuple[float, float, float] = None, v: tp.Tuple[float, float, float] = None, offset: tp.Tuple[float, float, float] = None, scale: float = None, transform: Mat4 = None):
        row_count, col_count = heights.shape
        offset = offset if offset is not None else (0, 0, 0)
        u = u if u is not None else (0, 0, 2.0 / (col_count - 1))
        v = v if v is not None else (2.0 / (row_count - 1), 0, 0)
        o = dvz.vec3(*offset)
        u = dvz.vec3(*u)
        v = dvz.vec3(*v)
        c_shape = dvz.shape()
        dvz.shape_surface(c_shape, row_count, col_count, heights, colors, o, u, v, 0)
        self.add(c_shape, scale=scale, transform=transform)

    def merge(self):
        self.c_merged = merge_shapes(self.c_shapes)

    def destroy(self):
        for c_shape in self.c_shapes:
            if c_shape:
                # print("destroy shape", c_shape)
                dvz.shape_destroy(c_shape)

        if self.c_merged:
            # print("destroy merged", self.c_merged)
            dvz.shape_destroy(self.c_merged)
