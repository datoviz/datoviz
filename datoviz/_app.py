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

from . import _constants as cst
from . import _ctypes as dvz
from . import visuals as vs
from ._event import Event
from ._figure import Figure
from ._texture import Texture
from .shape_collection import ShapeCollection
from .utils import dtype_to_format, image_flags, mesh_flags, to_enum

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
        .. warning::
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

        Returns
        -------
        Figure
            The created figure instance.

        Warnings
        --------
        .. warning::
            The `gui` parameter *must* be `True` when a GUI is used in the figure, and *must* be
            `False` otherwise. This may change in versions.
        """
        if gui:
            c_flags |= dvz.CANVAS_FLAGS_IMGUI
        c_figure = dvz.figure(self.c_scene, width, height, c_flags)
        return Figure(c_figure)

    def on_timer(self) -> None:
        """
        Handle timer events.

        Warnings:
        --------
        .. warning::
            This method is not yet implemented.
        """
        raise NotImplementedError()

    def on_frame(self) -> None:
        """
        Handle frame events.

        Warnings:
        --------
        .. warning::
            This method is not yet implemented.
        """
        raise NotImplementedError()

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
        shape: tp.Optional[tuple[int, ...]] = None,
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
        shape: tp.Optional[tuple[int, ...]] = None,
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
        visual.set_data(**kwargs)
        if fixed is not None:
            visual.fixed(fixed)
        return visual

    def basic(self, topology: str, **kwargs) -> vs.Basic:
        """
        Create a basic visual.

        Parameters
        ----------
        topology : str
            Topology type: `point_list`, `line_list`, `line_strip`, `triangle_list`,
            `triangle_strip`, `triangle_fan`.
        **kwargs
            Additional keyword arguments for the visual.

        Returns
        -------
        vs.Basic
            The created basic visual instance.
        """
        c_topology = to_enum(f'primitive_topology_{topology}')
        assert c_topology
        c_visual = dvz.basic(self.c_batch, c_topology, 0)
        return self._visual(cls=vs.Basic, c_visual=c_visual, **kwargs)

    def pixel(self, **kwargs) -> vs.Pixel:
        """
        Create a pixel visual.

        Parameters
        ----------
        **kwargs
            Additional keyword arguments for the visual.

        Returns
        -------
        vs.Pixel
            The created pixel visual instance.
        """
        return self._visual(dvz.pixel, vs.Pixel, **kwargs)

    def point(self, **kwargs) -> vs.Point:
        """
        Create a point visual.

        Parameters
        ----------
        **kwargs
            Additional keyword arguments for the visual.

        Returns
        -------
        vs.Point
            The created point visual instance.
        """
        return self._visual(dvz.point, vs.Point, **kwargs)

    def marker(self, **kwargs) -> vs.Marker:
        """
        Create a marker visual.

        Parameters
        ----------
        **kwargs
            Additional keyword arguments for the visual.

        Returns
        -------
        vs.Marker
            The created marker visual instance.
        """
        return self._visual(dvz.marker, vs.Marker, **kwargs)

    def segment(self, **kwargs) -> vs.Segment:
        """
        Create a segment visual.

        Parameters
        ----------
        **kwargs
            Additional keyword arguments for the visual.

        Returns
        -------
        vs.Segment
            The created segment visual instance.
        """
        return self._visual(dvz.segment, vs.Segment, **kwargs)

    def path(self, **kwargs) -> vs.Path:
        """
        Create a path visual.

        Parameters
        ----------
        **kwargs
            Additional keyword arguments for the visual.

        Returns
        -------
        vs.Path
            The created path visual instance.
        """
        return self._visual(dvz.path, vs.Path, **kwargs)

    def glyph(self, font_size: int = cst.DEFAULT_FONT_SIZE, **kwargs) -> vs.Glyph:
        """
        Create a glyph visual.

        Parameters
        ----------
        font_size : int, optional
            Font size for the glyph, by default cst.DEFAULT_FONT_SIZE.
        **kwargs
            Additional keyword arguments for the visual.

        Returns
        -------
        vs.Glyph
            The created glyph visual instance.
        """
        c_visual = dvz.glyph(self.c_batch, 0)
        visual = vs.Glyph(c_visual, font_size=font_size)
        visual.set_data(**kwargs)
        return visual

    def image(
        self,
        unit: str = None,
        mode: str = None,
        rescale: str = None,
        border: bool = None,
        **kwargs,
    ) -> vs.Image:
        """
        Create an image visual.

        Parameters
        ----------
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

        **kwargs
            Additional keyword arguments for the visual.

        Returns
        -------
        vs.Image
            The created image visual instance.
        """
        if kwargs.get('linewidth', 0) or kwargs.get('radius', 0) or kwargs.get('edgecolor', 0):
            border = True
        c_flags = image_flags(
            unit=unit,
            mode=mode,
            rescale=rescale,
            border=border,
        )
        return self._visual(dvz.image, vs.Image, c_flags=c_flags, **kwargs)

    def _mesh(
        self,
        c_visual,
        vertex_count: tp.Optional[int] = None,
        index_count: tp.Optional[int] = None,
        **kwargs,
    ) -> vs.Mesh:
        """
        Create a mesh visual.

        Parameters
        ----------
        c_visual : optional
            Visual instance, by default None.
        vertex_count : int, optional
            Number of vertices, by default None.
        index_count : int, optional
            Number of indices, by default None.
        **kwargs
            Additional keyword arguments for the visual.

        Returns
        -------
        vs.Mesh
            The created mesh visual instance.
        """
        return self._visual(
            c_visual=c_visual,
            cls=vs.Mesh,
            vertex_count=vertex_count,
            index_count=index_count,
            **kwargs,
        )

    def mesh(
        self,
        indexed: tp.Optional[bool] = None,
        textured: tp.Optional[bool] = None,
        lighting: tp.Optional[bool] = None,
        contour: tp.Optional[bool] = None,
        isoline: tp.Optional[bool] = None,
        **kwargs,
    ) -> vs.Mesh:
        """
        Create a mesh visual.

        Parameters
        ----------
        indexed : bool
            Whether the mesh is indexed.
        textured : bool
            Whether to use a texture for the mesh.
        lighting : bool
            Whether lighting is enabled.
        contour : bool
            Whether contour is enabled.
        isoline : bool
            Whether to show isolines.
        **kwargs
            Additional keyword arguments for the visual.

        Returns
        -------
        vs.Mesh
            The created mesh visual instance.
        """
        c_flags = mesh_flags(
            indexed=indexed, textured=textured, lighting=lighting, contour=contour, isoline=isoline
        )
        return self._mesh(dvz.mesh(self.c_batch, c_flags), **kwargs)

    def mesh_shape(
        self,
        shape: ShapeCollection,
        indexed: tp.Optional[bool] = None,
        textured: tp.Optional[bool] = None,
        lighting: tp.Optional[bool] = None,
        contour: tp.Optional[bool] = None,
        isoline: tp.Optional[bool] = None,
        **kwargs,
    ) -> vs.Mesh:
        """
        Create a mesh visual from a shape collection.

        Parameters
        ----------
        shape : ShapeCollection
            Shape collection.
        indexed : bool
            Whether the mesh is indexed.
        textured : bool
            Whether to use a texture for the mesh.
        lighting : bool
            Whether lighting is enabled.
        contour : bool
            Whether contour is enabled.
        isoline : bool
            Whether to show isolines.
        **kwargs
            Additional keyword arguments for the visual.

        Returns
        -------
        vs.Mesh
            The created mesh visual instance.
        """
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

        c_flags = mesh_flags(
            indexed=indexed, textured=textured, lighting=lighting, contour=contour, isoline=isoline
        )
        return self._mesh(
            dvz.mesh_shape(self.c_batch, c_merged, c_flags),
            vertex_count=nv,
            index_count=ni,
            **kwargs,
        )

    def sphere(self, **kwargs) -> vs.Sphere:
        """
        Create a sphere visual.

        Parameters
        ----------
        **kwargs
            Additional keyword arguments for the visual.

        Returns
        -------
        vs.Sphere
            The created sphere visual instance.
        """
        return self._visual(dvz.sphere, vs.Sphere, **kwargs)

    def volume(self, mode: str = 'colormap', **kwargs) -> vs.Volume:
        """
        Create a volume visual.

        Parameters
        ----------
        mode : str, optional
            Volume mode ('colormap', 'mip', etc.), by default 'colormap'.
        **kwargs
            Additional keyword arguments for the visual.

        Returns
        -------
        vs.Volume
            The created volume visual instance.
        """
        assert mode in cst.VOLUME_MODES
        c_flags = to_enum(f'volume_flags_{mode}')
        return self._visual(dvz.volume, vs.Volume, c_flags=c_flags, **kwargs)

    def slice(self, **kwargs) -> vs.Slice:
        """
        Create a slice visual.

        Parameters
        ----------
        **kwargs
            Additional keyword arguments for the visual.

        Returns
        -------
        vs.Slice
            The created slice visual instance.
        """
        return self._visual(dvz.slice, vs.Slice, **kwargs)

    # GUI
    # ---------------------------------------------------------------------------------------------

    def arcball_gui(self, panel, arcball) -> None:
        """
        Attach an arcball GUI to a panel.

        Parameters
        ----------
        panel : Panel
            Panel instance.
        arcball : Arcball
            Arcball instance.
        """
        c_figure = panel.c_figure
        dvz.arcball_gui(arcball.c_arcball, self.c_app, dvz.figure_id(c_figure), panel.c_panel)

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
                fun(ev)

            dvz.app_on_timer(self.c_app, on_timer, None)
            self._callbacks.append(on_timer)
            return fun

        dvz.app_timer(self.c_app, delay, period, max_count)
        return decorator

    def timestamps(self, figure: Figure, count: int) -> tuple[np.ndarray, np.ndarray]:
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
