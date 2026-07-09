#!/usr/bin/env python3
"""Prepared Allen mouse brain volume with an embedded slice."""

from __future__ import annotations

import ctypes
import gzip
from pathlib import Path

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


DATA_PATH = Path("data/examples/allen_ibl/prepared/allen_mouse_brain_rgba.npy.gz")
RAW_SHAPE = (528, 456, 320, 4)
DOWNSAMPLE = 2
SLICE_AXIS = dvz.DVZ_VOLUME_AXIS_Y
SLICE_POSITION = 0.50
VOLUME_OPACITY = 0.85
SLICE_OPACITY = 1.0
VOLUME_STEPS = 192
BRAIN_SCALAR_TISSUE_FLOOR = 32
INITIAL_ANGLES = (ctypes.c_float * 3)(-0.42, +0.26, -0.16)

FRAME_BG = dvz.DvzColor(13, 18, 24, 255)
PANEL_BG = dvz.DvzColor(18, 25, 32, 255)
CYAN = dvz.DvzColor(76, 201, 240, 255)
MINT = dvz.DvzColor(128, 255, 219, 255)
TEXT = dvz.DvzColor(217, 226, 236, 255)


def _load_rgba(path: Path = DATA_PATH) -> np.ndarray:
    if not path.exists():
        raise FileNotFoundError(
            f"{path} is missing; run `python tools/data/prepare_brain_volume.py`"
        )
    with gzip.open(path, "rb") as f:
        rgba = np.load(f, allow_pickle=False)
    if rgba.shape != RAW_SHAPE or rgba.dtype != np.uint8:
        raise ValueError(f"unexpected Allen mouse brain volume: {rgba.shape} {rgba.dtype}")
    return np.ascontiguousarray(rgba)


def _downsample_strongest(rgba: np.ndarray, factor: int = DOWNSAMPLE) -> np.ndarray:
    if factor <= 1:
        return rgba
    depth, height, width, channels = rgba.shape
    if channels != 4 or depth % factor != 0 or height % factor != 0 or width % factor != 0:
        raise ValueError("brain volume dimensions must be divisible by the downsample factor")

    out_shape = (depth // factor, height // factor, width // factor, 4)
    best = np.zeros(out_shape, dtype=np.uint8)
    best_score = np.zeros(out_shape[:3], dtype=np.uint32)

    for z in range(factor):
        for y in range(factor):
            for x in range(factor):
                candidate = rgba[z::factor, y::factor, x::factor, :]
                luma = (
                    candidate[..., 0].astype(np.uint32)
                    + candidate[..., 1].astype(np.uint32)
                    + candidate[..., 2].astype(np.uint32)
                )
                score = candidate[..., 3].astype(np.uint32) * np.uint32(1024) + luma
                mask = score > best_score
                best_score[mask] = score[mask]
                best[mask] = candidate[mask]
    return best


def _normalize_alpha(rgba: np.ndarray) -> None:
    alpha = rgba[..., 3]
    max_alpha = int(alpha.max(initial=0))
    if max_alpha == 0 or max_alpha == 255:
        return
    visible = alpha > 0
    scaled = (alpha[visible].astype(np.uint32) * 255 + max_alpha // 2) // max_alpha
    alpha[visible] = np.clip(scaled, 0, 255).astype(np.uint8)


def _scalar_volume(rgba: np.ndarray) -> np.ndarray:
    alpha = rgba[..., 3]
    luma = (
        77 * rgba[..., 0].astype(np.uint32)
        + 150 * rgba[..., 1].astype(np.uint32)
        + 29 * rgba[..., 2].astype(np.uint32)
        + 128
    ) // 256
    scalar = np.where(alpha > 0, luma, 0).astype(np.uint8)
    signal = scalar[scalar > 0]
    if signal.size == 0:
        return scalar
    min_signal = int(signal.min())
    max_signal = int(signal.max())
    if max_signal <= min_signal:
        return scalar

    visible = scalar > 0
    stretched = (
        BRAIN_SCALAR_TISSUE_FLOOR
        + (
            (scalar[visible].astype(np.uint32) - min_signal)
            * (255 - BRAIN_SCALAR_TISSUE_FLOOR)
            + (max_signal - min_signal) // 2
        )
        // (max_signal - min_signal)
    )
    scalar[visible] = np.clip(stretched, 0, 255).astype(np.uint8)
    return np.ascontiguousarray(scalar)


def _load_scalar_volume(path: Path = DATA_PATH) -> np.ndarray:
    rgba = _load_rgba(path)
    downsampled = _downsample_strongest(rgba)
    del rgba
    _normalize_alpha(downsampled)
    return _scalar_volume(downsampled)


def _volume_bounds(width: int, height: int, depth: int):
    max_dim = max(width, height, depth, 1)
    sx = width / max_dim
    sy = height / max_dim
    sz = depth / max_dim
    return (ctypes.c_double * 3)(-sx, -sy, -sz), (ctypes.c_double * 3)(+sx, +sy, +sz)


def _attach_transfer(scene, visual, *, slice_visual: bool) -> None:
    desc = dvz.dvz_scale_desc()
    desc.kind = dvz.DVZ_SCALE_CONTINUOUS
    desc.label = b"brain density"
    scale = dvz.dvz_scale(scene, ctypes.byref(desc))
    if not scale:
        raise RuntimeError("dvz_scale() failed")
    if dvz.dvz_scale_set_domain(scale, 0.0, 1.0) != 0:
        raise RuntimeError("dvz_scale_set_domain() failed")

    colormap = dvz.dvz_colormap(scene, None)
    if not colormap:
        raise RuntimeError("dvz_colormap() failed")
    stop_values = (
        (0.00, (FRAME_BG.r, FRAME_BG.g, FRAME_BG.b, 255)),
        (0.10, (PANEL_BG.r, PANEL_BG.g, PANEL_BG.b, 255)),
        (0.26, (24, 64, 82, 255)),
        (0.48, (CYAN.r, CYAN.g, CYAN.b, 255)),
        (0.70, (MINT.r, MINT.g, MINT.b, 255)),
        (0.90, (MINT.r, MINT.g, MINT.b, 255)),
        (1.00, (TEXT.r, TEXT.g, TEXT.b, 255)),
    )
    stops = (dvz.DvzColormapStop * len(stop_values))()
    for stop, (position, rgba) in zip(stops, stop_values, strict=True):
        stop.position = position
        stop.rgba[:] = rgba
    if dvz.dvz_colormap_set_stops(colormap, stops, len(stops)) != 0:
        raise RuntimeError("dvz_colormap_set_stops() failed")
    if dvz.dvz_scale_set_colormap(scale, colormap) != 0:
        raise RuntimeError("dvz_scale_set_colormap() failed")

    alpha_values = (
        ((0.00, 0.00), (0.01, 0.00), (0.03, 1.00), (1.00, 1.00))
        if slice_visual
        else (
            (0.00, 0.00),
            (0.12, 0.00),
            (0.24, 0.06),
            (0.42, 0.22),
            (0.62, 0.46),
            (0.84, 0.74),
            (1.00, 0.90),
        )
    )
    alpha = (dvz.DvzVolumeAlphaStop * len(alpha_values))()
    for stop, (position, opacity) in zip(alpha, alpha_values, strict=True):
        stop.position = position
        stop.alpha = opacity
    if dvz.dvz_volume_set_alpha_stops(visual, alpha, len(alpha)) != 0:
        raise RuntimeError("dvz_volume_set_alpha_stops() failed")
    if dvz.dvz_visual_set_scale(visual, b"color", scale) != 0:
        raise RuntimeError("dvz_visual_set_scale() failed")


def _configure_volume_pair(volume, slice_visual, scalar: np.ndarray) -> None:
    # Raw texture axes are DV, ML, AP; scene axes are ML, reversed AP, reversed DV.
    axis_order = (ctypes.c_uint32 * 3)(2, 0, 1)
    axis_flip = (ctypes.c_bool * 3)(True, False, True)
    for visual in (volume, slice_visual):
        if dvz.dvz_visual_set_alpha_mode(visual, dvz.DVZ_ALPHA_BLENDED) != 0:
            raise RuntimeError("dvz_visual_set_alpha_mode() failed")
        if dvz.dvz_volume_set_sampling(visual, dvz.DVZ_VOLUME_SAMPLING_LINEAR) != 0:
            raise RuntimeError("dvz_volume_set_sampling() failed")
        if dvz.dvz_volume_set_axis_mapping(visual, axis_order, axis_flip) != 0:
            raise RuntimeError("dvz_volume_set_axis_mapping() failed")

    depth, height, width = scalar.shape
    display_width = height
    display_height = depth
    display_depth = width
    bmin, bmax = _volume_bounds(display_width, display_height, display_depth)
    for visual in (volume, slice_visual):
        if dvz.dvz_volume_set_bounds(visual, bmin, bmax) != 0:
            raise RuntimeError("dvz_volume_set_bounds() failed")

    if dvz.dvz_volume_set_render_mode(volume, dvz.DVZ_VOLUME_RENDER_COMPOSITE) != 0:
        raise RuntimeError("dvz_volume_set_render_mode(volume) failed")
    if dvz.dvz_volume_set_value_range(volume, 0.04, 1.0) != 0:
        raise RuntimeError("dvz_volume_set_value_range(volume) failed")
    if dvz.dvz_volume_set_opacity(volume, VOLUME_OPACITY) != 0:
        raise RuntimeError("dvz_volume_set_opacity(volume) failed")
    if dvz.dvz_volume_set_step_count(volume, VOLUME_STEPS) != 0:
        raise RuntimeError("dvz_volume_set_step_count(volume) failed")
    clip_min = (ctypes.c_double * 3)(0.0, 0.0, 0.0)
    clip_max = (ctypes.c_double * 3)(1.0, SLICE_POSITION, 1.0)
    if dvz.dvz_volume_set_clipping_box(volume, clip_min, clip_max) != 0:
        raise RuntimeError("dvz_volume_set_clipping_box() failed")

    if dvz.dvz_volume_set_render_mode(slice_visual, dvz.DVZ_VOLUME_RENDER_SLICE) != 0:
        raise RuntimeError("dvz_volume_set_render_mode(slice) failed")
    if dvz.dvz_volume_set_value_range(slice_visual, 0.0, 1.0) != 0:
        raise RuntimeError("dvz_volume_set_value_range(slice) failed")
    if dvz.dvz_volume_set_opacity(slice_visual, SLICE_OPACITY) != 0:
        raise RuntimeError("dvz_volume_set_opacity(slice) failed")
    if dvz.dvz_volume_set_slice_axis(slice_visual, SLICE_AXIS) != 0:
        raise RuntimeError("dvz_volume_set_slice_axis() failed")
    if dvz.dvz_volume_set_slice_position(slice_visual, SLICE_POSITION) != 0:
        raise RuntimeError("dvz_volume_set_slice_position() failed")


def _enable_volume_occlusion(panel, volume, slice_visual) -> None:
    desc = dvz.dvz_volume_occlusion_desc()
    desc.enabled = True
    desc.alpha_threshold = 0.000001
    desc.fade_distance = 0.000001
    desc.occluded_alpha = 0.097
    if dvz.dvz_panel_set_volume_occluder(panel, volume, ctypes.byref(desc)) != 0:
        raise RuntimeError("dvz_panel_set_volume_occluder() failed")
    if dvz.dvz_visual_set_scene_occluder(volume, False) != 0:
        raise RuntimeError("dvz_visual_set_scene_occluder() failed")
    if dvz.dvz_visual_set_volume_occluded(slice_visual, True) != 0:
        raise RuntimeError("dvz_visual_set_volume_occluded() failed")
    if dvz.dvz_visual_set_scene_occluded(slice_visual, False) != 0:
        raise RuntimeError("dvz_visual_set_scene_occluded() failed")
    if dvz.dvz_panel_set_scene_occlusion(panel, None) != 0:
        raise RuntimeError("dvz_panel_set_scene_occlusion() failed")


def _set_capabilities(scene) -> None:
    caps = dvz.dvz_capability_snapshot()
    caps.max_color_attachments = 3
    caps.render_target_format_rgba16float = True
    caps.render_target_format_r16float = True
    caps.supports_render_target_sampling = True
    caps.supports_color_blending = True
    if dvz.dvz_scene_set_capabilities(scene, ctypes.byref(caps)) != 0:
        raise RuntimeError("dvz_scene_set_capabilities() failed")


def _setup_camera(panel) -> None:
    camera = dvz.dvz_camera_desc()
    camera.view.eye[:] = (-1.55, 1.58, -2.45)
    camera.view.target[:] = (0.0, 0.0, 0.0)
    camera.view.up[:] = (0.0, 1.0, 0.0)
    camera.projection.fov_y = 0.72
    camera.projection.near_clip = 0.01
    camera.projection.far_clip = 100.0
    if dvz.dvz_panel_set_camera_desc(panel, ctypes.byref(camera)) != 0:
        raise RuntimeError("dvz_panel_set_camera_desc() failed")


def _add_volume_pair(scene, panel, scalar: np.ndarray):
    field = dvz.dvz_sampled_field_from_array(
        scene,
        scalar,
        format=dvz.DVZ_FIELD_FORMAT_R8_UNORM,
        semantic=dvz.DVZ_FIELD_SEMANTIC_SCALAR,
        dim=dvz.DVZ_FIELD_DIM_3D,
    )
    volume = dvz.dvz_volume(scene, 0)
    slice_visual = dvz.dvz_volume(scene, 0)
    if not volume or not slice_visual:
        raise RuntimeError("dvz_volume() failed")
    if dvz.dvz_visual_set_field(volume, b"field", field) != 0:
        raise RuntimeError("dvz_visual_set_field(volume) failed")
    if dvz.dvz_visual_set_field(slice_visual, b"field", field) != 0:
        raise RuntimeError("dvz_visual_set_field(slice) failed")

    _attach_transfer(scene, volume, slice_visual=False)
    _attach_transfer(scene, slice_visual, slice_visual=True)
    _configure_volume_pair(volume, slice_visual, scalar)

    volume_attach = dvz.dvz_visual_attach_desc()
    volume_attach.z_layer = 0
    volume_attach.controller_mode = dvz.DVZ_CONTROLLER_APPLY
    slice_attach = dvz.dvz_visual_attach_desc()
    slice_attach.z_layer = 1
    slice_attach.controller_mode = dvz.DVZ_CONTROLLER_APPLY
    if dvz.dvz_panel_add_visual(panel, volume, ctypes.byref(volume_attach)) != 0:
        raise RuntimeError("dvz_panel_add_visual(volume) failed")
    if dvz.dvz_panel_add_visual(panel, slice_visual, ctypes.byref(slice_attach)) != 0:
        raise RuntimeError("dvz_panel_add_visual(slice) failed")

    _enable_volume_occlusion(panel, volume, slice_visual)
    return volume, slice_visual


def _build_scene(path: Path = DATA_PATH):
    scalar = _load_scalar_volume(path)
    scene = dvz.dvz_scene()
    if not scene:
        raise RuntimeError("dvz_scene() failed")
    _set_capabilities(scene)
    figure = dvz.dvz_figure(scene, ex.WIDTH, ex.HEIGHT, 0)
    if not figure:
        dvz.dvz_scene_destroy(scene)
        raise RuntimeError("dvz_figure() failed")
    panel = dvz.dvz_panel_full(figure)
    if not panel:
        dvz.dvz_scene_destroy(scene)
        raise RuntimeError("dvz_panel_full() failed")
    dvz.dvz_panel_set_background_color(panel, PANEL_BG)
    _setup_camera(panel)
    volume, slice_visual = _add_volume_pair(scene, panel, scalar)
    return scene, figure, panel, volume, slice_visual, scalar.shape


def _configure_view(view, scene, panel) -> None:
    controller = dvz.dvz_arcball(scene, None)
    if not controller:
        raise RuntimeError("dvz_arcball() failed")
    if dvz.dvz_view_bind_controller(view, panel, controller, dvz.DVZ_DIM_MASK_XYZ) != 0:
        raise RuntimeError("dvz_view_bind_controller() failed")
    arcball = dvz.dvz_controller_arcball(controller)
    if not arcball:
        raise RuntimeError("dvz_controller_arcball() failed")
    if dvz.dvz_arcball_set(arcball, INITIAL_ANGLES) != 0:
        raise RuntimeError("dvz_arcball_set() failed")


def main() -> None:
    scene, figure, panel, _volume, _slice, shape = _build_scene()
    print(f"loaded downsampled Allen mouse brain scalar volume {shape[2]}x{shape[1]}x{shape[0]}")

    def configure(view) -> None:
        _configure_view(view, scene, panel)

    ex.run_with_view(scene, figure, "Allen Mouse Brain", configure)


if __name__ == "__main__":
    main()
