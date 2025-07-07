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
from typing import Tuple, Union

import numpy as np

from . import _constants as cst
from . import _ctypes as dvz
from . import visuals as vs
from ._event import Event
from ._figure import Figure
from ._texture import Texture
from .shape_collection import ShapeCollection
from .utils import dtype_to_format, image_flags, mesh_flags, sphere_flags, to_enum

# -------------------------------------------------------------------------------------------------
# App
# -------------------------------------------------------------------------------------------------


class App:
    """
    Main application class for managing figures, textures, visuals, and events.

    Attributes
    ----------
    c_flags : int
        Datoviz flags for the application.
    c_app : dvz.DvzApp
        Internal application instance.
    c_batch : dvz.DvzBatch
        Internal batch instance.
    c_scene : dvz.DvzScene
        Internal scene instance.
    """

    c_flags: int = 0
    c_app: dvz.DvzApp = None
    c_batch: dvz.DvzBatch = None
    c_scene: dvz.DvzScene = None

    def __init__(
        self, c_flags: int = 0, offscreen: bool = False, background: tp.Optional[str] = None
    ) -> None:
        """
        Initialize the App instance.

        Parameters
        ----------
        c_flags : int, optional
            Datoviz flags for the application, by default 0.
        offscreen : bool, optional
            Whether to run in offscreen mode, by default False.
        background : str or None, optional
            Background color ('white' or None), by default None (black).

        Warnings
        --------

        The `background` parameter is likely to change in future versions.
        """
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

    def figure(
        self,
        width: int = cst.DEFAULT_WIDTH,
        height: int = cst.DEFAULT_HEIGHT,
        c_flags: int = 0,
        gui: bool = False,
        fullscreen: bool = False,
    ) -> Figure:
        """
        Create a new figure.

        Parameters
        ----------
        width : int, optional
            Width of the figure, by default cst.DEFAULT_WIDTH.
        height : int, optional
            Height of the figure, by default cst.DEFAULT_HEIGHT.
        c_flags : int, optional
            Flags for the figure, by default 0.
        gui : bool, optional
            Whether to enable GUI, by default False.
        fullscreen : bool, optional
            Open figure in fullscreen mode.

        Returns
        -------
        Figure
            The created figure instance.

        Warnings
        --------
        The `gui` parameter *must* be `True` when a GUI is used in the figure, and *must* be
        `False` otherwise. This may change in versions.
        """
        if gui:
            c_flags |= dvz.CANVAS_FLAGS_IMGUI
        if fullscreen:
            c_flags |= dvz.CANVAS_FLAGS_FULLSCREEN
        c_figure = dvz.figure(self.c_scene, width, height, c_flags)
        return Figure(c_figure, app=self)

    def run(self, frame_count: int = 0) -> None:
        """
        Run the application.

        Parameters
        ----------
        frame_count : int, optional
            Number of frames to run. 0 for infinite, by default 0.
        """
        dvz.scene_run(self.c_scene, self.c_app, frame_count)

    def screenshot(self, figure: Figure, png_path: str) -> None:
        """
        Take a screenshot of the given figure.

        This function first runs one frame before saving the screenshot.

        Parameters
        ----------
        figure : Figure
            The figure to capture.
        png_path : str
            Path to save the screenshot.
        """
        self.run(1)
        dvz.app_screenshot(self.c_app, figure.figure_id(), png_path)

    def stop(self):
        """Stop the application."""
        dvz.app_stop(self.c_app)

    def __del__(self) -> None:
        self.destroy()

    def destroy(self) -> None:
        """
        Destroy the application and release resources.
        """
        if self.c_app is not None:
            dvz.scene_destroy(self.c_scene)
            dvz.app_destroy(self.c_app)
            self.c_app = None

    # Textures
    # ---------------------------------------------------------------------------------------------

    def texture(
        self,
        image: tp.Optional[np.ndarray] = None,
        ndim: int = 2,
        shape: tp.Optional[Tuple[int, ...]] = None,
        n_channels: tp.Optional[int] = None,
        dtype: tp.Optional[np.dtype] = None,
        interpolation: tp.Optional[str] = None,
        address_mode: tp.Optional[str] = None,
    ) -> Texture:
        """
        Create a texture, either with or without initial image data. If without, all texture
        parameters must be specified.

        Parameters
        ----------
        image : np.ndarray, optional
            Image data for the texture, by default None.
        ndim : int, optional
            Number of texture dimensions (1, 2, or 3), by default 2.
        shape : tuple of int, optional
            Shape of the texture, inferred from `image` if set.
        n_channels : int, optional
            Number of color channels, inferred from `image` if set.
        dtype : np.dtype, optional
            Data type of the texture, inferred from `image` if set.
        interpolation : str, optional
            Interpolation mode, `nearest` (default) or `linear`.
        address_mode : str, optional
            Address mode: `repeat`, `mirrored_repeat`, `clamp_to_edge`,
            `clamp_to_border` (default), `mirror_clamp_to_edge`.

        Returns
        -------
        Texture
            The created texture instance.
        """
        if image is not None:
            if image.ndim == 4:
                ndim = 3
            # NOTE: ambiguity if image.ndim == 3, may be 2D rgba or 3D single channel
            if shape is None:
                shape = image.shape[:ndim]
                # WARNING: when inferring a shape from an image, the width and height are reversed
                shape = (shape[1], shape[0]) + shape[2:]
            n_channels = n_channels or (image.shape[-1] if ndim == image.ndim - 1 else 1)
            dtype = dtype or image.dtype
            assert 0 <= image.ndim - ndim <= 1

        assert n_channels > 0
        c_format = dtype_to_format(np.dtype(dtype).name, n_channels)
        shape = dvz.uvec3(*shape)
        width, height, depth = shape

        interpolation = interpolation or cst.DEFAULT_INTERPOLATION
        c_filter = to_enum(f'filter_{interpolation}')
        address_mode = address_mode or cst.DEFAULT_ADDRESS_MODE
        c_address_mode = to_enum(f'sampler_address_mode_{address_mode}')

        if ndim == 1:
            c_texture = dvz.texture_1D(
                self.c_batch, c_format, c_filter, c_address_mode, width, image, 0
            )
        elif ndim == 2:
            c_texture = dvz.texture_2D(
                self.c_batch, c_format, c_filter, c_address_mode, width, height, image, 0
            )
        elif ndim == 3:
            c_texture = dvz.texture_3D(
                self.c_batch, c_format, c_filter, c_address_mode, width, height, depth, image, 0
            )

        return Texture(c_texture, c_batch=self.c_batch, ndim=ndim)

    def texture_1D(
        self,
        data: np.ndarray,
        interpolation: tp.Optional[str] = None,
        address_mode: tp.Optional[str] = None,
    ) -> Texture:
        """
        Create a 1D texture.

        Parameters
        ----------
        data : np.ndarray
            Data for the texture.
        interpolation : str, optional
            Interpolation mode (see `texture()`)
        address_mode : str, optional
            Address mode (see `texture()`)

        Returns
        -------
        Texture
            The created 1D texture instance.
        """
        return self.texture(data, ndim=1, interpolation=interpolation, address_mode=address_mode)

    def texture_2D(
        self,
        image: np.ndarray,
        interpolation: tp.Optional[str] = None,
        address_mode: tp.Optional[str] = None,
    ) -> Texture:
        """
        Create a 2D texture.

        Parameters
        ----------
        image : np.ndarray
            Image data for the texture.
        interpolation : str, optional
            Interpolation mode (see `texture()`)
        address_mode : str, optional
            Address mode (see `texture()`)

        Returns
        -------
        Texture
            The created 2D texture instance.
        """
        return self.texture(image, ndim=2, interpolation=interpolation, address_mode=address_mode)

    def texture_3D(
        self,
        volume: np.ndarray,
        shape: tp.Optional[Tuple[int, ...]] = None,
        interpolation: tp.Optional[str] = None,
        address_mode: tp.Optional[str] = None,
    ) -> Texture:
        """
        Create a 3D texture.

        Parameters
        ----------
        volume : np.ndarray
            Volume data for the texture.
        shape : tuple of int, optional
            Shape of the texture, by default None.
        interpolation : str, optional
            Interpolation mode (see `texture()`)
        address_mode : str, optional
            Address mode (see `texture()`)

        Returns
        -------
        Texture
            The created 3D texture instance.
        """
        return self.texture(
            volume, ndim=3, shape=shape, interpolation=interpolation, address_mode=address_mode
        )

    # Visuals
    # ---------------------------------------------------------------------------------------------

    def _visual(
        self,
        fn: tp.Callable = None,
        cls: type = None,
        c_visual=None,
        c_flags: int = 0,
        fixed=None,
        **kwargs,
    ) -> vs.Visual:
        """
        Create a visual.

        Parameters
        ----------
        fn : Callable, optional
            Function to create the visual.
        cls : type, optional
            Class of the visual.
        c_visual : optional
            Visual instance.
        c_flags : int, optional
            Flags for the visual, by default 0.
        fixed : optional
            Fixed data for the visual, by default None.
        **kwargs
            Additional keyword arguments for the visual.

        Returns
        -------
        vs.Visual
            The created visual instance.
        """
        c_visual = c_visual or fn(self.c_batch, c_flags)
        visual = cls(c_visual)
        kwargs_f = {k: v for k, v in kwargs.items() if v is not None}
        visual.set_data(**kwargs_f)
        if fixed is not None:
            visual.fixed(fixed)
        return visual

    def basic(
        self,
        topology: str = None,
        position: np.ndarray = None,
        color: np.ndarray = None,
        group: np.ndarray = None,
        size: float = None,
        shape: ShapeCollection = None,
        depth_test: bool = None,
        cull: str = None,
    ) -> vs.Basic:
        """
        Create a basic visual.

        Parameters
        ----------
        topology : str
            Topology type: `point_list`, `line_list`, `line_strip`,
            `triangle_list`, `triangle_strip`.
        position : ndarray
            Point 3D positions in normalized device coordinates.
        color : ndarray
            Point RGBA colors in range 0–255.
        group : ndarray
            Group indices of all points (optional).
        size : float
            Point size in pixels, when using the `point_list` topology.
        shape : ShapeCollection, optional
            Create a basic visual from a shape collection.
        depth_test : bool, optional
            Whether to enable depth testing.
        cull : str, optional
            The culling mode, `None`, `front`, or `back`.

        Returns
        -------
        vs.Basic
            The created basic visual instance.
        """
        if not shape and topology not in cst.TOPOLOGY_OPTIONS:
            raise ValueError(f'Topology must be one of {cst.TOPOLOGY_OPTIONS} and not {topology}')
        c_topology = to_enum(f'primitive_topology_{topology}')
        assert c_topology is not None
        if shape is None:
            c_visual = dvz.basic(self.c_batch, c_topology, 0)
        else:
            if not shape.c_merged:
                shape.merge()
            c_visual = dvz.basic_shape(self.c_batch, shape.c_merged, c_topology, 0)
        return self._visual(
            cls=vs.Basic,
            c_visual=c_visual,
            position=position,
            color=color,
            group=group,
            size=size,
            depth_test=depth_test,
            cull=cull,
        )

    def pixel(
        self,
        position: np.ndarray = None,
        color: np.ndarray = None,
        size: float = None,
        depth_test: bool = None,
        cull: str = None,
    ) -> vs.Pixel:
        """
        Create a pixel visual.

        Parameters
        ----------
        position : ndarray
            Point 3D positions in normalized device coordinates.
        color : ndarray
            Point RGBA colors in range 0–255.
        size : float
            Point size in pixels, when using the `point_list` topology.
        depth_test : bool, optional
            Whether to enable depth testing.
        cull : str, optional
            The culling mode, `None`, `front`, or `back`.

        Returns
        -------
        vs.Pixel
            The created pixel visual instance.
        """
        return self._visual(
            dvz.pixel,
            vs.Pixel,
            position=position,
            color=color,
            size=size,
            depth_test=depth_test,
            cull=cull,
        )

    def point(
        self,
        position: np.ndarray = None,
        color: np.ndarray = None,
        size: np.ndarray = None,
        depth_test: bool = None,
        cull: str = None,
    ) -> vs.Point:
        """
        Create a point visual.

        Parameters
        ----------
        position : ndarray
            Point 3D positions in normalized device coordinates.
        color : ndarray
            Point RGBA colors in range 0–255.
        size : ndarray
            Point size in pixels.
        depth_test : bool, optional
            Whether to enable depth testing.
        cull : str, optional
            The culling mode, `None`, `front`, or `back`.

        Returns
        -------
        vs.Point
            The created point visual instance.
        """
        return self._visual(
            dvz.point,
            vs.Point,
            position=position,
            color=color,
            size=size,
            depth_test=depth_test,
            cull=cull,
        )

    def marker(
        self,
        position: np.ndarray = None,
        color: np.ndarray = None,
        size: np.ndarray = None,
        angle: np.ndarray = None,
        edgecolor: Tuple[int, int, int, int] = None,
        linewidth: float = None,
        tex_scale: float = None,
        mode: str = None,
        aspect: str = None,
        shape: str = None,
        texture: tp.Optional[Texture] = None,
        depth_test: bool = None,
        cull: str = None,
    ) -> vs.Marker:
        """
        Create a marker visual.

        Parameters
        ----------
        position : ndarray
            Marker 3D positions in normalized device coordinates.
        color : ndarray
            Marker RGBA colors in range 0–255.
        size : ndarray
            Marker size in pixels.
        angle : ndarray
            Marker rotation angle in radians.
        edgecolor : cvec4
            Marker edge color in RGBA format.
        linewidth : float
            Marker edge line width in pixels.
        tex_scale : float
            Marker texture scale.
        mode : str, optional
            Marker mode, one of `code`, `bitmap`, `sdf`, `msdf`.
        aspect : str, optional
            Marker aspect, one of `fill`, `stroke`, `outline`.
        shape : str, optional
            Marker shape, when using the `code` mode, one of `disc`, `asterisk`, etc.
            See the documentation for the full list.
        texture : Texture, optional
            Texture for the marker when using another mode than `code`.
        depth_test : bool, optional
            Whether to enable depth testing.
        cull : str, optional
            The culling mode, `None`, `front`, or `back`.

        Returns
        -------
        vs.Marker
            The created marker visual instance.
        """
        return self._visual(
            dvz.marker,
            vs.Marker,
            position=position,
            color=color,
            size=size,
            angle=angle,
            edgecolor=edgecolor,
            linewidth=linewidth,
            tex_scale=tex_scale,
            mode=mode,
            aspect=aspect,
            shape=shape,
            texture=texture,
            depth_test=depth_test,
            cull=cull,
        )

    def segment(
        self,
        initial: np.ndarray = None,
        terminal: np.ndarray = None,
        shift: np.ndarray = None,
        color: np.ndarray = None,
        linewidth: np.ndarray = None,
        cap: str = None,
        depth_test: bool = None,
        cull: str = None,
    ) -> vs.Segment:
        """
        Create a segment visual.

        Parameters
        ----------
        initial : ndarray
            3D positions of the initial end of each segment, in normalized device coordinates.
        terminal : ndarray
            3D positions of the terminal end of each segment, in normalized device coordinates.
        color : ndarray
            Segment RGBA colors in range 0–255.
        shift : ndarray, optional
            Shift vector for each segment, in pixels. Each row of this 2D array contains the pixel
            shift of the initial and terminal end of each segment (x0,y0,x1,y1), in framebuffer
            coordinates.
        linewidth : ndarray, optional
            Line width for each segment, in pixels.
        cap : str, optional
            Cap style for the segment, one of `butt`, `round`, `square`, `triangle_in`,
            `triangle_out`.
        depth_test : bool, optional
            Whether to enable depth testing.
        cull : str, optional
            The culling mode, `None`, `front`, or `back`.

        Returns
        -------
        vs.Segment
            The created segment visual instance.
        """
        return self._visual(
            dvz.segment,
            vs.Segment,
            position=(initial, terminal),
            shift=shift,
            color=color,
            linewidth=linewidth,
            cap=cap,
            depth_test=depth_test,
            cull=cull,
        )

    def path(
        self,
        position: np.ndarray = None,
        color: np.ndarray = None,
        linewidth: np.ndarray = None,
        cap: str = None,
        join: str = None,
        depth_test: bool = None,
        cull: str = None,
    ) -> vs.Path:
        """
        Create a path visual.

        Parameters
        ----------
        position : ndarray, or list of ndarray
            An array of positions, or a list of arrays representing the positions of the paths.
        color : ndarray
            Point RGBA colors in range 0–255.
        linewidth : ndarray
            Uniform or varying line width for each path point.
        cap : str, optional
            Cap style for all paths, one of `butt`, `round`, `square`, `triangle_in`,
            `triangle_out`.
        join : str, optional
            Join style for all paths, one of `square`, `round`.
        depth_test : bool, optional
            Whether to enable depth testing.
        cull : str, optional
            The culling mode, `None`, `front`, or `back`.

        Returns
        -------
        vs.Path
            The created path visual instance.
        """
        return self._visual(
            dvz.path,
            vs.Path,
            position=position,
            color=color,
            linewidth=linewidth,
            cap=cap,
            join=join,
            depth_test=depth_test,
            cull=cull,
        )

    def glyph(
        self,
        font_size: int = cst.DEFAULT_FONT_SIZE,
        position: np.ndarray = None,
        axis: np.ndarray = None,
        size: np.ndarray = None,
        anchor: np.ndarray = None,
        shift: np.ndarray = None,
        texcoords: np.ndarray = None,
        group_size: np.ndarray = None,
        scale: np.ndarray = None,
        angle: np.ndarray = None,
        color: np.ndarray = None,
        bgcolor: tp.Optional[Tuple[int, int, int, int]] = None,
        texture: tp.Optional[Texture] = None,
        depth_test: bool = None,
        cull: str = None,
    ) -> vs.Glyph:
        """
        Create a glyph visual.

        Parameters
        ----------
        font_size : int, optional
            Font size for the glyph, by default cst.DEFAULT_FONT_SIZE.
        position : ndarray
            Glyph 3D positions in normalized device coordinates.
        axis : ndarray, optional
            Glyph axes in 3D space, used for rotation (not implemented yet).
        size : ndarray, optional
            Glyph sizes in pixels, each row contains the width and height of each glyph.
        anchor : ndarray, optional
            Glyph anchor points in pixels, each row contains the x and y anchor of each glyph.
        shift : ndarray, optional
            Glyph shifts in pixels, each row contains the x and y shift of each glyph.
        texcoords : ndarray, optional
            Glyph texture coordinates, each row contains the u0,v0,u1,v1 coordinates of each glyph
            within the font atlas texture.
        group_size : ndarray, optional
            Glyph group sizes, each row contains the width and height of the string
            (i.e., the group of glyphs) the glyph belongs to.
        scale : ndarray, optional
            Glyph scales, each row contains the scale factor for each glyph.
        angle : ndarray, optional
            Glyph rotation angles in radians, each row contains the angle for each glyph.
        color : ndarray
            Glyph RGBA colors in range 0–255.
        bgcolor : Tuple[int, int, int, int], optional
            Background color for the glyph, in RGBA format.
        texture : Texture, optional
            Texture for the glyph, typically a font atlas texture.
        depth_test : bool, optional
            Whether to enable depth testing.
        cull : str, optional
            The culling mode, `None`, `front`, or `back`.

        Returns
        -------
        vs.Glyph
            The created glyph visual instance.
        """
        c_visual = dvz.glyph(self.c_batch, 0)
        visual = vs.Glyph(c_visual, font_size=font_size)
        visual.set_data(
            position=position,
            axis=axis,
            size=size,
            anchor=anchor,
            shift=shift,
            texcoords=texcoords,
            group_size=group_size,
            scale=scale,
            angle=angle,
            color=color,
            bgcolor=bgcolor,
            texture=texture,
            depth_test=depth_test,
            cull=cull,
        )
        return visual

    def image(
        self,
        position: np.ndarray = None,
        size: np.ndarray = None,
        anchor: np.ndarray = None,
        texcoords: np.ndarray = None,
        facecolor: np.ndarray = None,
        edgecolor: tp.Optional[Tuple[int, int, int, int]] = None,
        permutation: Tuple[int, int] = None,
        linewidth: float = None,
        radius: float = None,
        colormap: tp.Optional[str] = None,
        texture: tp.Optional[Texture] = None,
        unit: str = None,
        mode: str = None,
        rescale: str = None,
        border: bool = None,
        depth_test: bool = None,
        cull: str = None,
    ) -> vs.Image:
        """
        Create an image visual.

        Parameters
        ----------
        position : ndarray
            Image 3D positions in normalized device coordinates.
        size : ndarray
            Image sizes in pixels or NDC, each row contains the width and height of each image.
        anchor : ndarray
            Image anchor in normalized coordinates.
        texcoords : ndarray
            Image texture coordinates, each row contains the u0,v0,u1,v1 coordinates of each image
            within the texture.
        facecolor : ndarray
            Image face colors in RGBA format, each row contains the RGBA color of each image.
        edgecolor : Tuple[int, int, int, int], optional
            Image edge color in RGBA format, used for the border.
        permutation : Tuple[int, int], optional
            Permutation of the image texture coordinates, e.g., (0, 1) for normal orientation.
        linewidth : float, optional
            Line width for the image border, in pixels.
        radius : float, optional
            Radius for the image border, in pixels, or 0 for a square border.
        colormap : str, optional
            Colormap to apply to the image, when using the `colormap` mode.
        texture : Texture, optional
            Texture for the image, typically a 2D texture containing the image data.
        unit : str, optional
            Specifies the unit for the image size. Can be:

            - `pixels` (default): Image size is specified in pixels.
            - `ndc`: Image size depends on the normalized device coordinates (NDC) of the panel.
        mode : str, optional
            Specifies the image mode. Can be:

            - `rgba` (default): RGBA image mode.
            - `colormap`: Single-channel image with a colormap applied.
            - `fill`: Uniform color fill mode.
        rescale : str, optional
            Specifies how the image should be rescaled with transformations. Can be:

            - `None` (default): No rescaling.
            - `rescale`: Rescale the image with the panel size.
            - `keep_ratio`: Rescale the image while maintaining its aspect ratio.
        border : bool, optional
            Indicates whether to display a border around the image. Defaults to `False`.
        depth_test : bool, optional
            Whether to enable depth testing.
        cull : str, optional
            The culling mode, `None`, `front`, or `back`.

        Returns
        -------
        vs.Image
            The created image visual instance.
        """
        if linewidth is not None or radius is not None or edgecolor is not None:
            border = True
        if colormap is not None:
            mode = 'colormap'
        c_flags = image_flags(
            unit=unit,
            mode=mode,
            rescale=rescale,
            border=border,
        )
        return self._visual(
            dvz.image,
            vs.Image,
            c_flags=c_flags,
            position=position,
            size=size,
            anchor=anchor,
            texcoords=texcoords,
            facecolor=facecolor,
            edgecolor=edgecolor,
            permutation=permutation,
            linewidth=linewidth,
            radius=radius,
            colormap=colormap,
            texture=texture,
            depth_test=depth_test,
            cull=cull,
        )

    def wiggle(
        self,
        bounds: tp.Optional[Tuple[Tuple[float, float], Tuple[float, float]]] = None,
        negative_color: tp.Optional[Tuple[int, int, int, int]] = None,
        positive_color: tp.Optional[Tuple[int, int, int, int]] = None,
        edgecolor: tp.Optional[Tuple[int, int, int, int]] = None,
        xrange: tp.Optional[Tuple[float, float]] = None,
        scale: tp.Optional[float] = None,
        texture: tp.Optional[Texture] = None,
    ) -> vs.Wiggle:
        """
        Create a Wiggle visual.

        Parameters
        ----------
        bounds : tuple of tuples, optional
            Bounds for the wiggle plot, in the form ((xmin, xmax), (ymin, ymax)).
        negative_color : Tuple[int, int, int, int], optional
            Color for the negative wiggle values, in RGBA format.
        positive_color : Tuple[int, int, int, int], optional
            Color for the positive wiggle values, in RGBA format.
        edgecolor : Tuple[int, int, int, int], optional
            Line color in RGBA format.
        xrange : tuple of float, optional
            Range of the x-axis for the wiggle plot, in the form (x0, xl).
        scale : float, optional
            Scale factor for the wiggle plot, applied to the wiggle values.
        texture : Texture, optional
            Texture for the wiggle, a 2D texture containing the data.

        Returns
        -------
        vs.Wiggle
            The created wiggle visual instance.
        """
        return self._visual(
            dvz.wiggle,
            vs.Wiggle,
            bounds=bounds,
            color=(negative_color, positive_color),
            edgecolor=edgecolor,
            xrange=xrange,
            scale=scale,
            texture=texture,
        )

    def mesh(
        self,
        shape: ShapeCollection = None,
        position: np.ndarray = None,
        color: np.ndarray = None,
        texcoords: np.ndarray = None,
        normal: np.ndarray = None,
        isoline: np.ndarray = None,
        left: np.ndarray = None,
        right: np.ndarray = None,
        contour: Union[np.ndarray, bool] = None,
        index: np.ndarray = None,
        light_pos: Tuple[float, float, float, float] = None,
        light_color: Tuple[int, int, int, int] = None,
        ambient_params: tp.Optional[Tuple[float, float, float]] = None,
        diffuse_params: tp.Optional[Tuple[float, float, float]] = None,
        specular_params: tp.Optional[Tuple[float, float, float]] = None,
        emission_params: tp.Optional[Tuple[float, float, float]] = None,
        shine: float = None,
        emit: float = None,
        edgecolor: tp.Optional[Tuple[int, int, int, int]] = None,
        linewidth: float = None,
        density: int = None,
        texture: tp.Optional[Texture] = None,
        indexed: tp.Optional[bool] = None,
        lighting: tp.Optional[bool] = None,
        depth_test: bool = None,
        cull: str = None,
    ) -> vs.Mesh:
        """
        Create a mesh visual.

        Parameters
        ----------
        shape : ShapeCollection, optional
            Create a mesh from a shape collection.
        position : ndarray
            Vertex 3D positions in normalized device coordinates.
        color : ndarray
            Vertex RGBA colors in range 0–255.
        texcoords : ndarray
            Vertex texture coordinates, each row contains the u0,v0,u1,v1 coordinates of each
            vertex within the texture.
        normal : ndarray
            Vertex normals in normalized device coordinates.
        isoline : ndarray, optional
            Scalar field, one value per vertex, is showing isolines.
        left : ndarray, optional
            Left values for contours (not documented yet).
        right : ndarray, optional
            Right values for contours (not documented yet).
        contour : ndarray or bool, optional
            Contour values for the mesh (not documented yet), or a boolean indicating whether to
            use contours.
        index : ndarray, optional
            Vertex indices for indexed meshes (three integers per triangle).
        light_pos : Tuple[float, float, float, float], optional
            Light position in normalized device coordinates, in the form (x, y, z, w).
            If `w` is 0, the light is directional; if `w` is 1, the light is positional.
        light_color : Tuple[int, int, int, int], optional
            Light color in RGBA format, in the form (r, g, b, a).
        ambient_params : Tuple[float, float, float], optional
            Material ambient parameters for the mesh, in the form (r, g, b).
        diffuse_params : Tuple[float, float, float], optional
            Material diffuse parameters for the mesh, in the form (r, g, b).
        specular_params : Tuple[float, float, float], optional
            Material specular parameters for the mesh, in the form (r, g, b).
        emission_params : Tuple[float, float, float], optional
            Material emission parameters for the mesh, in the form (r, g, b).
        shine : float, optional
            Material shine factor for the mesh, in the range [0, 1].
        emit : float, optional
            Material emission factor for the mesh, in the range [0, 1].
        edgecolor : Tuple[int, int, int, int], optional
            Edge color for the mesh, in RGBA format, when showing contours or isolines.
        linewidth : float, optional
            Line width for the mesh edges, in pixels, when showing contours or isolines.
        density : int, optional
            Density of isolines, in pixels, when showing isolines.
        texture : Texture, optional
            Texture for the mesh, when using textured mesh.
        indexed : bool, optional
            Whether the mesh is indexed. If `True`, the mesh will use indices for vertices.
        lighting : bool
            Whether lighting is enabled.
        depth_test : bool, optional
            Whether to enable depth testing.
        cull : str, optional
            The culling mode, `None`, `front`, or `back`.

        Returns
        -------
        vs.Mesh
            The created mesh visual instance.
        """
        # has_contour flag.
        has_contour = False
        if contour is not None or linewidth is not None or edgecolor is not None:
            has_contour = True

        # has_isoline flag.
        has_isoline = False
        if isoline is not None or density is not None:
            has_isoline = True

        # has_index flag.
        if indexed is not None:
            has_index = indexed
        else:
            has_index = True if index is not None else False

        # has_texture flag.
        has_texture = True if texture is not None else False

        if shape:
            if not shape.c_merged:
                shape.merge()
            c_merged = shape.c_merged

            # Allocate the visual with the right number of vertices and indices.
            nv = dvz.shape_vertex_count(c_merged)
            ni = dvz.shape_index_count(c_merged)
            if ni == 0:
                has_index = False

        c_flags = mesh_flags(
            indexed=has_index,
            textured=has_texture,
            lighting=lighting,
            contour=has_contour,
            isoline=has_isoline,
        )

        # Initialization with a ShapeCollection
        kwargs = {}
        if shape:
            c_visual = dvz.mesh_shape(self.c_batch, c_merged, c_flags)
            kwargs['vertex_count'] = nv
            kwargs['index_count'] = ni

        # Initialization with raw position, color etc.
        else:
            c_visual = dvz.mesh(self.c_batch, c_flags)

        return self._visual(
            c_visual=c_visual,
            cls=vs.Mesh,
            position=position,
            color=color,
            texcoords=texcoords,
            normal=normal,
            isoline=isoline,
            # NOTE: these are not properly documented yet, so just skip them for now
            # left=left,
            # right=right,
            # contour=contour,
            index=index,
            light_pos=light_pos,
            light_color=light_color,
            ambient_params=ambient_params,
            diffuse_params=diffuse_params,
            specular_params=specular_params,
            emission_params=emission_params,
            shine=shine,
            emit=emit,
            edgecolor=edgecolor,
            linewidth=linewidth,
            density=density,
            texture=texture,
            depth_test=depth_test,
            cull=cull,
            **kwargs,
        )

    def sphere(
        self,
        position: np.ndarray = None,
        color: np.ndarray = None,
        size: np.ndarray = None,
        light_pos: tp.Optional[Tuple[float, float, float, float]] = None,
        light_color: tp.Optional[Tuple[int, int, int, int]] = None,
        ambient_params: tp.Optional[Tuple[float, float, float]] = None,
        diffuse_params: tp.Optional[Tuple[float, float, float]] = None,
        specular_params: tp.Optional[Tuple[float, float, float]] = None,
        emission_params: tp.Optional[Tuple[float, float, float]] = None,
        shine: tp.Optional[float] = None,
        emit: tp.Optional[float] = None,
        texture: tp.Optional[Texture] = None,
        equal_rectangular: tp.Optional[bool] = None,
        lighting: tp.Optional[bool] = None,
        size_pixels: tp.Optional[bool] = None,
        depth_test: bool = None,
        cull: str = None,
    ) -> vs.Sphere:
        """
        Create a sphere visual.

        Parameters
        ----------
        position : ndarray
            Sphere 3D positions in normalized device coordinates.
        color : ndarray
            Sphere RGBA colors in range 0–255.
        size : ndarray
            Sphere sizes in pixels or NDC, depending on `size_pixels`.
        light_pos : Tuple[float, float, float, float], optional
            Light position in normalized device coordinates, in the form (x, y, z, w).
            If `w` is 0, the light is directional; if `w` is 1, the light is positional.
        light_color : Tuple[int, int, int, int], optional
            Light color in RGBA format, in the form (r, g, b, a).
        ambient_params : Tuple[float, float, float], optional
            Material ambient parameters for the sphere, in the form (r, g, b).
        diffuse_params : Tuple[float, float, float], optional
            Material diffuse parameters for the sphere, in the form (r, g, b).
        specular_params : Tuple[float, float, float], optional
            Material specular parameters for the sphere, in the form (r, g, b).
        emission_params : Tuple[float, float, float], optional
            Material emission parameters for the sphere, in the form (r, g, b).
        shine : float, optional
            Material shine factor for the sphere, in the range [0, 1].
        emit : float, optional
            Material emission factor for the sphere, in the range [0, 1].
        texture : Texture, optional
            Texture for the sphere, when using a textured sphere.
        equal_rectangular : bool
            Texture is equal rectangular.
        lighting : bool
            Whether lighting is enabled.
        size_pixels : bool
            Whether to specify the sphere size in pixels rather than NDC.
        depth_test : bool, optional
            Whether to enable depth testing.
        cull : str, optional
            The culling mode, `None`, `front`, or `back`.

        Returns
        -------
        vs.Sphere
            The created sphere visual instance.
        """
        has_texture = True if texture is not None else False
        c_flags = sphere_flags(
            textured=has_texture,
            lighting=lighting,
            size_pixels=size_pixels,
            equal_rectangular=equal_rectangular,
        )
        return self._visual(
            dvz.sphere,
            vs.Sphere,
            c_flags=c_flags,
            position=position,
            color=color,
            size=size,
            light_pos=light_pos,
            light_color=light_color,
            ambient_params=ambient_params,
            diffuse_params=diffuse_params,
            specular_params=specular_params,
            emission_params=emission_params,
            shine=shine,
            emit=emit,
            texture=texture,
            depth_test=depth_test,
            cull=cull,
        )

    def volume(
        self,
        bounds: Tuple[Tuple[float, float], Tuple[float, float], Tuple[float, float]] = None,
        permutation: Tuple[int, int, int] = None,
        slice: int = None,
        transfer: Tuple[float, float, float, float] = None,
        texture: tp.Optional[Texture] = None,
        mode: str = 'colormap',
    ) -> vs.Volume:
        """
        Create a volume visual.

        Parameters
        ----------
        bounds : Tuple[Tuple[float, float], Tuple[float, float], Tuple[float, float]]
            Bounds of the volume in normalized device coordinates, as three tuples
            (xmin, xmax), (ymin, ymax), (zmin, zmax).
        permutation : Tuple[int, int, int], optional
            Permutation of the volume axes, e.g., (0, 1, 2) for normal orientation.
        slice : int, optional
            Slice index to display (not implemented yet).
        transfer : Tuple[float, float, float, float], optional
            Transfer function parameters for the volume (only the first value is used for now).
        texture : Texture, optional
            Texture for the volume, typically a 3D texture containing the volume data.
        mode : str, optional
            Volume mode ('rgba', 'colormap'), by default 'colormap'.

        Returns
        -------
        vs.Volume
            The created volume visual instance.
        """
        assert mode in cst.VOLUME_MODES
        c_flags = to_enum(f'volume_flags_{mode}')
        return self._visual(
            dvz.volume,
            vs.Volume,
            c_flags=c_flags,
            bounds=bounds,
            permutation=permutation,
            slice=slice,
            transfer=transfer,
            texture=texture,
        )

    def slice(
        self,
    ) -> vs.Slice:
        """Not implemented yet."""
        raise NotImplementedError()

    # Events
    # ---------------------------------------------------------------------------------------------

    def on_mouse(self, figure: Figure) -> tp.Callable:
        """
        Register a mouse event handler for the given figure.

        Parameters
        ----------
        figure : Figure
            The figure to attach the handler to.

        Returns
        -------
        Callable
            A decorator for the mouse event handler.
        """
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

    def on_keyboard(self, figure: Figure) -> tp.Callable:
        """
        Register a keyboard event handler for the given figure.

        Parameters
        ----------
        figure : Figure
            The figure to attach the handler to.

        Returns
        -------
        Callable
            A decorator for the keyboard event handler.
        """
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

    def on_resize(self, figure: Figure) -> tp.Callable:
        """
        Register a resize event handler for the given figure.

        Parameters
        ----------
        figure : Figure
            The figure to attach the handler to.

        Returns
        -------
        Callable
            A decorator for the resize event handler.
        """
        assert figure

        def decorator(fun):
            @dvz.on_resize
            def on_resize(app, window_id, ev_):
                if dvz.figure_id(figure.c_figure) == window_id:
                    ev = ev_.contents
                    fun(Event(ev, 'resize'))

            dvz.app_on_resize(self.c_app, on_resize, None)
            self._callbacks.append(on_resize)
            return fun

        return decorator

    def on_frame(self, figure: Figure) -> tp.Callable:
        """
        Register a frame event handler for the given figure.

        Parameters
        ----------
        figure : Figure
            The figure to attach the handler to.

        Returns
        -------
        Callable
            A decorator for the frame event handler.
        """
        assert figure

        def decorator(fun):
            @dvz.on_frame
            def on_frame(app, window_id, ev_):
                if dvz.figure_id(figure.c_figure) == window_id:
                    ev = ev_.contents
                    fun(Event(ev, 'frame'))

            dvz.app_on_frame(self.c_app, on_frame, None)
            self._callbacks.append(on_frame)
            return fun

        return decorator

    def on_gui(self, figure: Figure) -> tp.Callable:
        """
        Register a GUI event handler for the given figure.

        Parameters
        ----------
        figure : Figure
            The figure to attach the handler to.

        Returns
        -------
        Callable
            A decorator for the GUI event handler.
        """
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

    def connect(self, figure: Figure) -> tp.Callable:
        """
        Connect an event handler to the given figure, using the decorated function name to
        determine the event to attach it to. The name of the decorated function can be `on_mouse`,
        `on_keyboard`, or `on_gui`.

        Parameters
        ----------
        figure : Figure
            The figure to connect handlers to.

        Returns
        -------
        Callable
            A decorator for event handlers.
        """
        assert figure

        def decorator(fun):
            if fun.__name__ == 'on_mouse':
                return self.on_mouse(figure)(fun)
            elif fun.__name__ == 'on_keyboard':
                return self.on_keyboard(figure)(fun)
            elif fun.__name__ == 'on_frame':
                return self.on_frame(figure)(fun)
            elif fun.__name__ == 'on_gui':
                return self.on_gui(figure)(fun)
            elif fun.__name__ == 'on_resize':
                return self.on_resize(figure)(fun)
            # TODO: on_frame

        return decorator

    def timer(self, delay: float = 0.0, period: float = 1.0, max_count: int = 0) -> tp.Callable:
        """
        Register a timer event.

        Parameters
        ----------
        delay : float, optional
            Initial delay before the timer starts, by default 0.0.
        period : float, optional
            Period of the timer in seconds, by default 1.0 second.
        max_count : int, optional
            Maximum number of timer events, or 0 for unlimited timer ticks, by default 0.

        Returns
        -------
        Callable
            A decorator for the timer event handler.
        """

        def decorator(fun):
            @dvz.on_timer
            def on_timer(app, window_id, ev_):
                ev = ev_.contents
                fun(Event(ev, 'timer'))

            dvz.app_on_timer(self.c_app, on_timer, None)
            self._callbacks.append(on_timer)
            return fun

        dvz.app_timer(self.c_app, delay, period, max_count)
        return decorator

    def clear_timers(self):
        """
        Stop and clear all timers.
        """
        dvz.app_timer_clear(self.c_app)

    def timestamps(self, figure: Figure, count: int) -> Tuple[np.ndarray, np.ndarray]:
        """
        Get presentation timestamps for the given figure.

        Parameters
        ----------
        figure : Figure
            The figure to get timestamps for.
        count : int
            Number of timestamps.

        Returns
        -------
        tuple of np.ndarray
            Seconds and nanoseconds arrays.
        """
        assert figure
        seconds = np.zeros(count, dtype=np.uint64)  # epoch, in seconds
        nanoseconds = np.zeros(count, dtype=np.uint64)  # number of ns within the second
        dvz.app_timestamps(self.c_app, figure.figure_id(), count, seconds, nanoseconds)
        return seconds, nanoseconds
