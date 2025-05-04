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

import numpy as np

from . import _constants as cst
from . import _ctypes as dvz
from .utils import to_enum

# -------------------------------------------------------------------------------------------------
# Types
# -------------------------------------------------------------------------------------------------

Color = tuple[int, int, int, int]
Vec4 = tuple[float, float, float, float]
Mat4 = tuple[float, ...]

DEFAULT_SIZE = 100
WHITE = dvz.cvec4(255, 255, 255, 255)


# -------------------------------------------------------------------------------------------------
# Utils
# -------------------------------------------------------------------------------------------------


def _shape_transform(
    c_shape: dvz.Shape,
    offset: tuple[float, float, float] = None,
    scale: float = None,
    transform: Mat4 = None,
) -> None:
    """
    Apply transformations to a shape.

    Parameters
    ----------
    c_shape : dvz.Shape
        The shape to transform.
    offset : tuple of float, optional
        The (x, y, z) offset to apply, by default None.
    scale : float, optional
        The scale factor to apply, by default None.
    transform : Mat4, optional
        A 4x4 transformation matrix, by default None.
    """
    dvz.shape_begin(c_shape, 0, 0)
    if scale is not None:
        dvz.shape_scale(c_shape, dvz.vec3(scale, scale, scale))
    if offset is not None:
        dvz.shape_translate(c_shape, dvz.vec3(*offset))
    # TODO
    # dvz.shape_rotate(c_shape, angle, axis)
    if transform is not None:
        dvz.shape_transform(c_shape, dvz.mat4(*transform))

    dvz.shape_end(c_shape)


def merge_shapes(c_shapes: list[dvz.Shape]) -> dvz.Shape:
    """
    Merge multiple shapes into a single shape.

    Parameters
    ----------
    c_shapes : list of dvz.Shape
        The list of shapes to merge.

    Returns
    -------
    dvz.Shape
        The merged shape.
    """
    merged = dvz.shape()
    n = len(c_shapes)
    if n == 0:
        return None
    else:
        array_type = ctypes.POINTER(dvz.Shape) * n
        shapes_array = array_type(*(s for s in c_shapes))
        dvz.shape_merge(merged, len(c_shapes), shapes_array)
        return merged


def unindex(
    c_shape: dvz.Shape,
    contour: str = None,
    indexing: str = None,
) -> None:
    """
    Unindex a shape, optionally applying contour and indexing flags.

    This takes an indexed mesh and converts it into a non-indexed one, which is necessary with
    contours.

    Parameters
    ----------
    c_shape : dvz.Shape
        The shape to unindex.
    contour : str, optional
        The contour type to apply, `edges`, `joints` (default), `full`.
    indexing : str, optional
        The indexing type to apply, `earcut` (when using polygons) or `surfaces` (when using
        surfaces).
    """
    c_flags = 0

    if contour is not None:
        # NOTE: contour can be a boolean, in which case it defaults to the default contour
        if contour is True:
            contour = None
        contour = contour or cst.DEFAULT_CONTOUR
        c_contour = to_enum(f'contour_{contour}')
        c_flags |= c_contour

    if indexing is not None:
        indexing = indexing or cst.DEFAULT_INDEXING
        c_indexing = to_enum(f'indexing_{indexing}')
        c_flags |= c_indexing

    dvz.shape_unindex(c_shape, c_flags)


# -------------------------------------------------------------------------------------------------
# Shape collection
# -------------------------------------------------------------------------------------------------


class ShapeCollection:
    """
    A collection of shapes that can be transformed, merged, and rendered.

    Attributes
    ----------
    c_shapes : list of dvz.Shape
        The list of shapes in the collection.
    c_merged : dvz.Shape or None
        The merged shape, if applicable.
    """

    c_shapes: list[dvz.Shape] = None
    c_merged: dvz.Shape = None

    def __init__(self) -> None:
        """
        Initialize an empty ShapeCollection.
        """
        self.c_shapes = []
        self.c_merged = None

    def add(
        self,
        c_shape: dvz.Shape,
        offset: tuple[float, float, float] = None,
        scale: float = None,
        transform: Mat4 = None,
    ) -> None:
        """
        Add a shape to the collection with optional transformations.

        Parameters
        ----------
        c_shape : dvz.Shape
            The shape to add.
        offset : tuple of float, optional
            The (x, y, z) offset to apply, by default None.
        scale : float, optional
            The scale factor to apply, by default None.
        transform : Mat4, optional
            A 4x4 transformation matrix, by default None.
        """
        _shape_transform(c_shape, offset=offset, scale=scale, transform=transform)
        self.c_shapes.append(c_shape)

    def add_square(
        self,
        offset: tuple[float, float, float] = None,
        scale: float = None,
        transform: Mat4 = None,
        color: Color = None,
    ) -> None:
        """
        Add a square shape to the collection.

        Parameters
        ----------
        offset : tuple of float, optional
            The (x, y, z) offset to apply, by default None.
        scale : float, optional
            The scale factor to apply, by default None.
        transform : Mat4, optional
            A 4x4 transformation matrix, by default None.
        color : Color, optional
            The color of the square, by default None.
        """
        c_shape = dvz.shape()
        c_color = dvz.cvec4(*color) if color is not None else WHITE
        dvz.shape_square(c_shape, c_color)
        self.add(c_shape, offset=offset, scale=scale, transform=transform)

    def add_disc(
        self,
        size: int = None,
        offset: tuple[float, float, float] = None,
        scale: float = None,
        transform: Mat4 = None,
        color: Color = None,
    ) -> None:
        """
        Add a disc shape to the collection.

        Parameters
        ----------
        size : int, optional
            The size of the disc, by default None.
        offset : tuple of float, optional
            The (x, y, z) offset to apply, by default None.
        scale : float, optional
            The scale factor to apply, by default None.
        transform : Mat4, optional
            A 4x4 transformation matrix, by default None.
        color : Color, optional
            The color of the disc, by default None.
        """
        c_shape = dvz.shape()
        c_color = dvz.cvec4(*color) if color is not None else WHITE
        dvz.shape_disc(c_shape, size, c_color)
        self.add(c_shape, offset=offset, scale=scale, transform=transform)

    def add_cube(
        self,
        offset: tuple[float, float, float] = None,
        scale: float = None,
        transform: Mat4 = None,
        color: Color = None,
    ) -> None:
        """
        Add a cube shape to the collection.

        Parameters
        ----------
        offset : tuple of float, optional
            The (x, y, z) offset to apply, by default None.
        scale : float, optional
            The scale factor to apply, by default None.
        transform : Mat4, optional
            A 4x4 transformation matrix, by default None.
        color : Color, optional
            The color of the cube, by default None.
        """
        c_shape = dvz.shape()
        colors = np.zeros((6, 4), dtype=np.uint8)
        for i in range(6):
            colors[i] = color if color is not None else WHITE
        dvz.shape_cube(c_shape, colors)
        self.add(c_shape, offset=offset, scale=scale, transform=transform)

    def add_sphere(
        self,
        rows: int = None,
        cols: int = None,
        offset: tuple[float, float, float] = None,
        scale: float = None,
        transform: Mat4 = None,
        color: Color = None,
    ) -> None:
        """
        Add a sphere shape to the collection.

        Parameters
        ----------
        rows : int, optional
            The number of rows in the sphere, by default None.
        cols : int, optional
            The number of columns in the sphere, by default None.
        offset : tuple of float, optional
            The (x, y, z) offset to apply, by default None.
        scale : float, optional
            The scale factor to apply, by default None.
        transform : Mat4, optional
            A 4x4 transformation matrix, by default None.
        color : Color, optional
            The color of the sphere, by default None.
        """
        rows = rows or DEFAULT_SIZE
        cols = cols or DEFAULT_SIZE
        c_shape = dvz.shape()
        c_color = dvz.cvec4(*color) if color is not None else WHITE
        dvz.shape_sphere(c_shape, rows, cols, c_color)
        self.add(c_shape, offset=offset, scale=scale, transform=transform)

    def add_cone(
        self,
        size: int = None,
        offset: tuple[float, float, float] = None,
        scale: float = None,
        transform: Mat4 = None,
        color: Color = None,
    ) -> None:
        """
        Add a cone shape to the collection.

        Parameters
        ----------
        size : int, optional
            The size of the cone, by default None.
        offset : tuple of float, optional
            The (x, y, z) offset to apply, by default None.
        scale : float, optional
            The scale factor to apply, by default None.
        transform : Mat4, optional
            A 4x4 transformation matrix, by default None.
        color : Color, optional
            The color of the cone, by default None.

        Warnings:
        --------
        .. warning::
            This method is not yet implemented.
        """
        size = size or DEFAULT_SIZE
        c_shape = dvz.shape()
        c_color = dvz.cvec4(*color) if color is not None else WHITE
        dvz.shape_cone(c_shape, size, c_color)
        self.add(c_shape, offset=offset, scale=scale, transform=transform)

    def add_cylinder(
        self,
        size: int = None,
        offset: tuple[float, float, float] = None,
        scale: float = None,
        transform: Mat4 = None,
        color: Color = None,
    ) -> None:
        """
        Add a cylinder shape to the collection.

        Parameters
        ----------
        size : int, optional
            The size of the cylinder, by default None.
        offset : tuple of float, optional
            The (x, y, z) offset to apply, by default None.
        scale : float, optional
            The scale factor to apply, by default None.
        transform : Mat4, optional
            A 4x4 transformation matrix, by default None.
        color : Color, optional
            The color of the cylinder, by default None.

        Warnings:
        --------
        .. warning::
            This method is not yet implemented.
        """
        size = size or DEFAULT_SIZE
        c_shape = dvz.shape()
        c_color = dvz.cvec4(*color) if color is not None else WHITE
        dvz.shape_cylinder(c_shape, size, c_color)
        self.add(c_shape, offset=offset, scale=scale, transform=transform)

    def add_tetrahedron(
        self,
        offset: tuple[float, float, float] = None,
        scale: float = None,
        transform: Mat4 = None,
        color: Color = None,
    ) -> None:
        """
        Add a tetrahedron shape to the collection.

        Parameters
        ----------
        offset : tuple of float, optional
            The (x, y, z) offset to apply, by default None.
        scale : float, optional
            The scale factor to apply, by default None.
        transform : Mat4, optional
            A 4x4 transformation matrix, by default None.
        color : Color, optional
            The color of the tetrahedron, by default None.
        """
        c_shape = dvz.shape()
        c_color = dvz.cvec4(*color) if color is not None else WHITE
        dvz.shape_tetrahedron(c_shape, c_color)

        # We need to unindex the mesh in order to get nice surface normals
        dvz.shape_unindex(c_shape, 0)

        # We compute the normals after unindexing.
        dvz.shape_normals(c_shape)

        self.add(c_shape, offset=offset, scale=scale, transform=transform)

    def add_hexahedron(
        self,
        offset: tuple[float, float, float] = None,
        scale: float = None,
        transform: Mat4 = None,
        color: Color = None,
    ) -> None:
        """
        Add a hexahedron shape to the collection.

        Parameters
        ----------
        offset : tuple of float, optional
            The (x, y, z) offset to apply, by default None.
        scale : float, optional
            The scale factor to apply, by default None.
        transform : Mat4, optional
            A 4x4 transformation matrix, by default None.
        color : Color, optional
            The color of the hexahedron, by default None.
        """
        c_shape = dvz.shape()
        c_color = dvz.cvec4(*color) if color is not None else WHITE
        dvz.shape_hexahedron(c_shape, c_color)

        # We need to unindex the mesh in order to get nice surface normals
        dvz.shape_unindex(c_shape, 0)

        # We compute the normals after unindexing.
        dvz.shape_normals(c_shape)

        self.add(c_shape, offset=offset, scale=scale, transform=transform)

    def add_octahedron(
        self,
        offset: tuple[float, float, float] = None,
        scale: float = None,
        transform: Mat4 = None,
        color: Color = None,
    ) -> None:
        """
        Add an octahedron shape to the collection.

        Parameters
        ----------
        offset : tuple of float, optional
            The (x, y, z) offset to apply, by default None.
        scale : float, optional
            The scale factor to apply, by default None.
        transform : Mat4, optional
            A 4x4 transformation matrix, by default None.
        color : Color, optional
            The color of the octahedron, by default None.
        """
        c_shape = dvz.shape()
        c_color = dvz.cvec4(*color) if color is not None else WHITE
        dvz.shape_octahedron(c_shape, c_color)

        # We need to unindex the mesh in order to get nice surface normals
        dvz.shape_unindex(c_shape, 0)

        # We compute the normals after unindexing.
        dvz.shape_normals(c_shape)

        self.add(c_shape, offset=offset, scale=scale, transform=transform)

    def add_dodecahedron(
        self,
        offset: tuple[float, float, float] = None,
        scale: float = None,
        transform: Mat4 = None,
        color: Color = None,
    ) -> None:
        """
        Add a dodecahedron shape to the collection.

        Parameters
        ----------
        offset : tuple of float, optional
            The (x, y, z) offset to apply, by default None.
        scale : float, optional
            The scale factor to apply, by default None.
        transform : Mat4, optional
            A 4x4 transformation matrix, by default None.
        color : Color, optional
            The color of the dodecahedron, by default None.
        """
        c_shape = dvz.shape()
        c_color = dvz.cvec4(*color) if color is not None else WHITE
        dvz.shape_dodecahedron(c_shape, c_color)

        # We need to unindex the mesh in order to get nice surface normals
        dvz.shape_unindex(c_shape, 0)

        # We compute the normals after unindexing.
        dvz.shape_normals(c_shape)

        self.add(c_shape, offset=offset, scale=scale, transform=transform)

    def add_icosahedron(
        self,
        offset: tuple[float, float, float] = None,
        scale: float = None,
        transform: Mat4 = None,
        color: Color = None,
    ) -> None:
        """
        Add an icosahedron shape to the collection.

        Parameters
        ----------
        offset : tuple of float, optional
            The (x, y, z) offset to apply, by default None.
        scale : float, optional
            The scale factor to apply, by default None.
        transform : Mat4, optional
            A 4x4 transformation matrix, by default None.
        color : Color, optional
            The color of the icosahedron, by default None.
        """
        c_shape = dvz.shape()
        c_color = dvz.cvec4(*color) if color is not None else WHITE
        dvz.shape_icosahedron(c_shape, c_color)

        # We need to unindex the mesh in order to get nice surface normals
        dvz.shape_unindex(c_shape, 0)

        # We compute the normals after unindexing.
        dvz.shape_normals(c_shape)

        self.add(c_shape, offset=offset, scale=scale, transform=transform)

    def add_polygon(
        self,
        points: np.ndarray,
        offset: tuple[float, float, float] = None,
        scale: float = None,
        transform: Mat4 = None,
        color: Color = None,
        contour: str = None,
        indexing: str = None,
    ) -> None:
        """
        Add a polygon shape to the collection.

        This function uses earcut to triangulate the polygon. More sophisticated triangulations
        methods will be added in a future version.

        Parameters
        ----------
        points : np.ndarray
            The points defining the polygon.
        offset : tuple of float, optional
            The (x, y, z) offset to apply, by default None.
        scale : float, optional
            The scale factor to apply, by default None.
        transform : Mat4, optional
            A 4x4 transformation matrix, by default None.
        color : Color, optional
            The color of the polygon, by default None.
        contour : str, optional
            The contour type to apply, by default None.
        indexing : str, optional
            The indexing type to apply, by default None.

        Warnings:
        --------
        .. warning::
            This method is not yet implemented.
        """
        assert points.ndim == 2
        assert points.shape[1] == 2
        c_shape = dvz.shape()
        c_color = dvz.cvec4(*color) if color is not None else WHITE
        dvz.shape_polygon(c_shape, points.shape[0], points, c_color)

        if contour or indexing:
            unindex(c_shape, contour=contour, indexing=indexing)

        self.add(c_shape, offset=offset, scale=scale, transform=transform)

    def add_surface(
        self,
        heights: np.ndarray,
        colors: np.ndarray,
        contour: str = None,
        indexing: str = None,
        u: tuple[float, float, float] = None,
        v: tuple[float, float, float] = None,
        offset: tuple[float, float, float] = None,
        scale: float = None,
        transform: Mat4 = None,
    ) -> None:
        """
        Add a surface shape to the collection.

        Parameters
        ----------
        heights : np.ndarray
            The height values of the surface.
        colors : np.ndarray
            The color values of the surface.
        contour : str, optional
            The contour type to apply, by default None.
        indexing : str, optional
            The indexing type to apply, by default None.
        u : tuple of float, optional
            The u vector for the surface, by default None.
        v : tuple of float, optional
            The v vector for the surface, by default None.
        offset : tuple of float, optional
            The (x, y, z) offset to apply, by default None.
        scale : float, optional
            The scale factor to apply, by default None.
        transform : Mat4, optional
            A 4x4 transformation matrix, by default None.
        """
        heights = np.asanyarray(heights, dtype=np.float32)
        row_count, col_count = heights.shape
        offset = offset if offset is not None else (-1, 0, -1)
        u = u if u is not None else (0, 0, 2.0 / (col_count - 1))
        v = v if v is not None else (2.0 / (row_count - 1), 0, 0)
        o = dvz.vec3(*offset)
        u = dvz.vec3(*u)
        v = dvz.vec3(*v)
        c_shape = dvz.shape()
        dvz.shape_surface(c_shape, row_count, col_count, heights.ravel(), colors, o, u, v, 0)

        if contour or indexing:
            indexing = indexing or 'surface'
            unindex(c_shape, contour=contour, indexing=indexing)

        self.add(c_shape, scale=scale, transform=transform)

    def add_obj(
        self,
        file_path: str,
        contour: str = None,
        offset: tuple[float, float, float] = None,
        scale: float = None,
        transform: Mat4 = None,
    ) -> None:
        """
        Add a shape from an OBJ file to the collection.

        Parameters
        ----------
        file_path : str
            The path to the OBJ file.
        contour : str, optional
            The contour type to apply, by default None.
        offset : tuple of float, optional
            The (x, y, z) offset to apply, by default None.
        scale : float, optional
            The scale factor to apply, by default None.
        transform : Mat4, optional
            A 4x4 transformation matrix, by default None.
        """
        c_shape = dvz.shape()
        dvz.shape_obj(c_shape, file_path)
        dvz.shape_unindex(c_shape, to_enum(f'contour_{contour}'))
        self.add(c_shape, offset=offset, scale=scale, transform=transform)

    def merge(self) -> None:
        """
        Merge all shapes in the collection into a single shape.
        """
        self.c_merged = merge_shapes(self.c_shapes)

    def destroy(self) -> None:
        """
        Destroy all shapes in the collection and release resources.
        """
        for c_shape in self.c_shapes:
            if c_shape:
                # print("destroy shape", c_shape)
                dvz.shape_destroy(c_shape)

        if self.c_merged:
            # print("destroy merged", self.c_merged)
            dvz.shape_destroy(self.c_merged)
