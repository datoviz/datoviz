#!/usr/bin/env python3
"""Textured Earth with real CelesTrak debris propagated into a prepared SGP4 ephemeris."""

from __future__ import annotations

import ctypes
import struct
from dataclasses import dataclass
from pathlib import Path

import numpy as np
from PIL import Image

import datoviz as dvz
from examples.python.gallery import common as ex

TEXTURE_WIDTH = 1024
TEXTURE_HEIGHT = 512
EARTH_TEXTURE_PATH = Path('data/assets/textures/world.200412.3x5400x2700.jpg')
SPHERE_RADIUS = 0.92
SPHERE_SECTORS = 96
SPHERE_RINGS = 48
STAR_COUNT = 900
STAR_RADIUS = 48.0
ORBIT_DATA_PATHS = (
    Path('data/examples/orbital_debris/prepared/orbital_debris.bin'),
    Path('.cache/datoviz/examples/orbital_debris/prepared/orbital_debris.bin'),
)
DEBRIS_TIME_SCALE = 60.0
GLOBE_ROTATION_SPEED = 0.035
ORBIT_TRACE_COUNT = 12
ORBIT_TRACE_SAMPLES = 121
SUN_DIR = (-0.80, +0.22, +0.55)
TAU = 2.0 * np.pi
ORBIT_HEADER = struct.Struct('<8sIIIIIddff32s')

PANEL_BG = dvz.DvzColor(2, 2, 4, 255)


@dataclass(frozen=True)
class OrbitModel:
    """Prepared catalog metadata and SGP4 ephemeris."""

    event_ids: np.ndarray
    catalog_ids: np.ndarray
    ephemeris: np.ndarray
    closed_traces: np.ndarray
    snapshot_utc: str
    step_seconds: float
    duration_seconds: float


@dataclass
class GlobeState:
    """Python-owned shared rotation track."""

    rotation: object | None = None

    def destroy(self) -> None:
        """Destroy the shared track after the scene run ends."""
        if self.rotation:
            dvz.dvz_track_destroy(self.rotation)
        self.rotation = None


def _load_orbit_model() -> OrbitModel:
    path = next((candidate for candidate in ORBIT_DATA_PATHS if candidate.exists()), None)
    if path is None:
        raise RuntimeError(
            'missing real orbital-debris ephemeris; run '
            '`uv run tools/data/prepare_orbital_debris.py --force`'
        )
    payload = path.read_bytes()
    if len(payload) < ORBIT_HEADER.size:
        raise RuntimeError(f'truncated orbital-debris ephemeris: {path}')

    (
        magic,
        version,
        object_count,
        frame_count,
        event_count,
        trace_sample_count,
        _start_unix_s,
        step_seconds,
        _earth_radius_km,
        _max_radius,
        snapshot_field,
    ) = ORBIT_HEADER.unpack_from(payload)
    if (
        magic != b'DVZORB1\0'
        or version != 2
        or object_count <= 0
        or frame_count < 2
        or event_count != 3
        or trace_sample_count < 3
        or step_seconds <= 0
    ):
        raise RuntimeError(f'invalid orbital-debris ephemeris header: {path}')

    event_offset = ORBIT_HEADER.size
    catalog_offset = event_offset + object_count
    position_offset = catalog_offset + 4 * object_count
    trace_offset = position_offset + 12 * object_count * frame_count
    expected_size = trace_offset + 12 * object_count * trace_sample_count
    if len(payload) != expected_size:
        raise RuntimeError(f'unexpected orbital-debris ephemeris size: {path}')
    event_ids = np.frombuffer(
        payload, dtype=np.uint8, count=object_count, offset=event_offset
    ).copy()
    catalog_ids = np.frombuffer(
        payload, dtype='<u4', count=object_count, offset=catalog_offset
    ).copy()
    ephemeris = np.frombuffer(
        payload,
        dtype='<f4',
        count=3 * object_count * frame_count,
        offset=position_offset,
    ).reshape(frame_count, object_count, 3)
    closed_traces = np.frombuffer(
        payload,
        dtype='<f4',
        count=3 * object_count * trace_sample_count,
        offset=trace_offset,
    ).reshape(object_count, trace_sample_count, 3)
    snapshot_utc = snapshot_field.split(b'\0', 1)[0].decode('ascii')
    return OrbitModel(
        event_ids,
        catalog_ids,
        ephemeris,
        closed_traces,
        snapshot_utc,
        float(step_seconds),
        float(step_seconds * (frame_count - 1)),
    )


def _orbit_positions(model: OrbitModel, time_s: float) -> np.ndarray:
    wrapped_time = float(time_s) % model.duration_seconds
    frame = wrapped_time / model.step_seconds
    frame0 = min(int(np.floor(frame)), len(model.ephemeris) - 1)
    frame1 = min(frame0 + 1, len(model.ephemeris) - 1)
    alpha = np.float32(frame - frame0 if frame1 > frame0 else 0.0)
    return np.ascontiguousarray(
        model.ephemeris[frame0] + alpha * (model.ephemeris[frame1] - model.ephemeris[frame0]),
        dtype=np.float32,
    )


def _orbit_trace(model: OrbitModel, index: int) -> np.ndarray:
    source = model.closed_traces[index]
    if len(source) == ORBIT_TRACE_SAMPLES:
        trace = np.array(source, dtype=np.float32, copy=True, order='C')
    else:
        source_positions = np.linspace(0, len(source) - 1, ORBIT_TRACE_SAMPLES)
        source0 = np.floor(source_positions).astype(np.int32)
        source1 = np.minimum(source0 + 1, len(source) - 1)
        alpha = (source_positions - source0).astype(np.float32)[:, None]
        trace = np.ascontiguousarray(
            source[source0] + alpha * (source[source1] - source[source0]),
            dtype=np.float32,
        )
    trace[-1] = trace[0]
    return trace


def _earth_texture() -> np.ndarray:
    if EARTH_TEXTURE_PATH.exists():
        with Image.open(EARTH_TEXTURE_PATH) as image:
            image = image.convert('RGBA')
            width, height = image.size
            if width == 2 * height:
                return np.ascontiguousarray(np.asarray(image, dtype=np.uint8))

    y = np.linspace(0.0, 1.0, TEXTURE_HEIGHT, dtype=np.float64)
    x = np.linspace(0.0, 1.0, TEXTURE_WIDTH, dtype=np.float64)
    u, v = np.meshgrid(x, y)
    lat = (0.5 - v) * np.pi
    lon = (u - 0.5) * TAU
    bands = (
        np.sin(12.0 * lon + 1.7 * np.sin(5.0 * lat))
        + 0.72 * np.sin(9.0 * lat + 2.2 * np.cos(3.0 * lon))
        + 0.38 * np.sin(21.0 * (lon + lat))
    )
    ridge = np.sin(34.0 * lon) * np.sin(16.0 * lat)
    land = bands + 0.28 * ridge > 0.38
    ice = np.abs(lat) > 1.22
    grid_u = np.abs(u * 24.0 - np.floor(u * 24.0 + 0.5))
    grid_v = np.abs(v * 12.0 - np.floor(v * 12.0 + 0.5))
    grid = (grid_u < 0.015) | (grid_v < 0.015)

    r = np.full_like(u, 0.05)
    g = np.full_like(u, 0.13)
    b = np.full_like(u, 0.27)

    warm = np.clip(0.5 + 0.5 * np.sin(5.0 * lon - 7.0 * lat), 0.0, 1.0)
    latitude_weight = 1.0 - np.abs(lat) / (0.5 * np.pi)
    r = np.where(land, 0.20 + 0.38 * warm, r)
    g = np.where(land, 0.34 + 0.32 * latitude_weight, g)
    b = np.where(land, 0.16 + 0.12 * (1.0 - warm), b)

    water = 0.5 + 0.5 * np.sin(8.0 * lon + 5.0 * lat)
    r = np.where(~land, 0.03 + 0.05 * water, r)
    g = np.where(~land, 0.20 + 0.18 * water, g)
    b = np.where(~land, 0.44 + 0.22 * water, b)

    r = np.where(ice, 0.84, r)
    g = np.where(ice, 0.90, g)
    b = np.where(ice, 0.93, b)
    r = np.where(grid, 0.92, r)
    g = np.where(grid, 0.94, g)
    b = np.where(grid, 0.86, b)

    rgba = np.empty((TEXTURE_HEIGHT, TEXTURE_WIDTH, 4), dtype=np.uint8)
    rgba[..., 0] = np.clip(255.0 * r + 0.5, 0, 255).astype(np.uint8)
    rgba[..., 1] = np.clip(255.0 * g + 0.5, 0, 255).astype(np.uint8)
    rgba[..., 2] = np.clip(255.0 * b + 0.5, 0, 255).astype(np.uint8)
    rgba[..., 3] = 255
    return rgba


def _planet_transform():
    transform = ((ctypes.c_double * 4) * 4)()
    transform[0][1] = -1.0
    transform[1][2] = +1.0
    transform[2][0] = -1.0
    transform[3][3] = 1.0
    return transform


def _add_star_shell(scene, panel) -> None:
    rng = np.random.default_rng(0xD42024)
    z = 2.0 * rng.random(STAR_COUNT, dtype=np.float32) - 1.0
    phi = TAU * rng.random(STAR_COUNT, dtype=np.float32)
    radius = np.sqrt(np.maximum(0.0, 1.0 - z * z))
    positions = np.column_stack(
        (
            STAR_RADIUS * radius * np.cos(phi),
            STAR_RADIUS * radius * np.sin(phi),
            STAR_RADIUS * z,
        )
    ).astype(np.float32)
    brightness = 0.45 + 0.55 * rng.random(STAR_COUNT, dtype=np.float32)
    colors = np.empty((STAR_COUNT, 4), dtype=np.uint8)
    colors[:, 0] = np.clip(255.0 * 0.82 * brightness + 0.5, 0, 255).astype(np.uint8)
    colors[:, 1] = np.clip(255.0 * 0.88 * brightness + 0.5, 0, 255).astype(np.uint8)
    colors[:, 2] = np.clip(255.0 * brightness + 0.5, 0, 255).astype(np.uint8)
    colors[:, 3] = 255
    sizes = (1.0 + 2.2 * rng.random(STAR_COUNT) * rng.random(STAR_COUNT)).astype(np.float32)

    positions[0] = (STAR_RADIUS * SUN_DIR[0], STAR_RADIUS * SUN_DIR[1], STAR_RADIUS * SUN_DIR[2])
    colors[0] = (255, 244, 214, 255)
    sizes[0] = 14.0

    stars = dvz.dvz_point(scene, 0)
    if not stars:
        raise RuntimeError('dvz_point() failed')
    if (
        dvz.dvz_visual_set_data_many(
            stars,
            {
                'position': positions,
                'color': colors,
                'diameter_px': sizes,
            },
        )
        != 0
    ):
        raise RuntimeError('dvz_visual_set_data_many(stars) failed')
    ex.set_filled_point_style(stars)
    if dvz.dvz_visual_set_depth_test(stars, False) != 0:
        raise RuntimeError('dvz_visual_set_depth_test(stars) failed')
    ex.add_visual(panel, stars)


def _planet_material():
    material = dvz.dvz_phong_material_desc()
    material.light_direction[:] = SUN_DIR
    material.phong.ambient = 0.16
    material.phong.diffuse = 0.92
    material.phong.specular = 0.015
    material.phong.shininess = 18.0
    return material


def _add_planet(scene, panel):
    field = dvz.dvz_sampled_field_from_array(scene, _earth_texture())

    desc = dvz.dvz_geometry_sphere_desc()
    desc.radius = SPHERE_RADIUS
    desc.sectors = SPHERE_SECTORS
    desc.rings = SPHERE_RINGS
    desc.color = ex.WHITE
    geometry = dvz.dvz_geometry_sphere(ctypes.byref(desc))
    if not geometry:
        raise RuntimeError('dvz_geometry_sphere() failed')

    mesh = dvz.dvz_mesh(scene, 0)
    if not mesh:
        dvz.dvz_geometry_destroy(geometry)
        raise RuntimeError('dvz_mesh() failed')

    try:
        if dvz.dvz_geometry_transform(geometry, _planet_transform()) != 0:
            raise RuntimeError('dvz_geometry_transform() failed')
        if dvz.dvz_mesh_set_geometry(mesh, geometry) != 0:
            raise RuntimeError('dvz_mesh_set_geometry() failed')
    finally:
        dvz.dvz_geometry_destroy(geometry)

    material = _planet_material()
    if dvz.dvz_visual_set_material(mesh, ctypes.byref(material)) != 0:
        raise RuntimeError('dvz_visual_set_material() failed')
    if dvz.dvz_visual_set_field(mesh, b'texture', field) != 0:
        raise RuntimeError('dvz_visual_set_field(texture) failed')
    ex.add_visual(panel, mesh)
    return mesh


def _add_orbit_traces(scene, panel, model: OrbitModel):
    indices: list[int] = []
    trace_events: list[int] = []
    selections_per_event = (ORBIT_TRACE_COUNT + 2) // 3
    for trace_index in range(ORBIT_TRACE_COUNT):
        event_id = trace_index % 3
        event_indices = np.flatnonzero(model.event_ids == event_id)
        ordinal = trace_index // 3
        index = event_indices[ordinal * len(event_indices) // selections_per_event]
        indices.append(int(index))
        trace_events.append(event_id)
    positions = np.concatenate([_orbit_trace(model, int(index)) for index in indices])
    sample_count = len(positions)
    palette = np.array(
        [[104, 220, 255, 255], [255, 196, 92, 255], [255, 112, 96, 255]],
        dtype=np.uint8,
    )
    colors = np.concatenate(
        [np.tile(palette[event_id], (ORBIT_TRACE_SAMPLES, 1)) for event_id in trace_events]
    )
    assert len(colors) == sample_count
    widths = np.full(sample_count, 0.85, dtype=np.float32)
    subpaths = np.full(ORBIT_TRACE_COUNT, ORBIT_TRACE_SAMPLES, dtype=np.uint32)

    path = dvz.dvz_path(scene, 0)
    if not path:
        raise RuntimeError('dvz_path() failed')
    if (
        dvz.dvz_visual_set_data_many(
            path,
            {
                'position': positions,
                'color': colors,
                'stroke_width_px': widths,
            },
        )
        != 0
    ):
        raise RuntimeError('dvz_visual_set_data_many(orbits) failed')
    lengths = np.ctypeslib.as_ctypes(subpaths)
    if dvz.dvz_path_set_subpaths(path, ORBIT_TRACE_COUNT, lengths) != 0:
        raise RuntimeError('dvz_path_set_subpaths() failed')
    if dvz.dvz_path_set_join(path, dvz.DVZ_PATH_JOIN_ROUND, 4.0) != 0:
        raise RuntimeError('dvz_path_set_join() failed')
    if dvz.dvz_visual_set_depth_test(path, True) != 0:
        raise RuntimeError('dvz_visual_set_depth_test(orbits) failed')
    ex.add_visual(panel, path)
    return path


def _add_debris(scene, panel, model: OrbitModel):
    positions = _orbit_positions(model, 0.0)
    palette = np.array(
        [[104, 220, 255, 255], [255, 196, 92, 255], [255, 112, 96, 255]],
        dtype=np.uint8,
    )
    colors = palette[model.event_ids]
    sizes = (2.0 + 2.8 * ((37 * model.catalog_ids.astype(np.uint64)) % 101) / 100.0).astype(
        np.float32
    )
    sizes[model.catalog_ids % 79 == 0] = 6.5

    points = dvz.dvz_point(scene, 0)
    if not points:
        raise RuntimeError('dvz_point() failed')
    if (
        dvz.dvz_visual_set_data_many(
            points,
            {
                'position': positions,
                'color': colors,
                'diameter_px': sizes,
            },
        )
        != 0
    ):
        raise RuntimeError('dvz_visual_set_data_many(debris) failed')
    ex.set_filled_point_style(points)
    if dvz.dvz_visual_set_depth_test(points, True) != 0:
        raise RuntimeError('dvz_visual_set_depth_test(debris) failed')
    ex.add_visual(panel, points)
    return points


def _setup_camera(panel) -> None:
    camera = dvz.dvz_camera_desc()
    camera.view.eye[:] = (0.0, 0.0, 3.7)
    camera.view.target[:] = (0.0, 0.0, 0.0)
    camera.view.up[:] = (0.0, 1.0, 0.0)
    camera.projection.fov_y = 0.72
    camera.projection.near_clip = 0.005
    camera.projection.far_clip = 100.0
    if dvz.dvz_panel_set_camera_desc(panel, ctypes.byref(camera)) != 0:
        raise RuntimeError('dvz_panel_set_camera_desc() failed')


def _add_globe_rotation(scene, visuals, state: GlobeState) -> None:
    rotation_desc = dvz.dvz_track_rotation_desc()
    rotation_desc.axis[:] = (0.0, 1.0, 0.0)
    rotation_desc.speed_rad_per_sec = 1.0
    rotation = dvz.dvz_track_rotation(ctypes.byref(rotation_desc))
    if not rotation:
        raise RuntimeError('dvz_track_rotation() failed')
    state.rotation = rotation

    for visual in visuals:
        transform_desc = dvz.dvz_transform_motion_desc()
        transform_desc.rotation = rotation
        animation = dvz.dvz_anim_visual_transform(scene, visual, ctypes.byref(transform_desc))
        if not animation:
            raise RuntimeError('dvz_anim_visual_transform() failed')
        if dvz.dvz_anim_set_speed(animation, GLOBE_ROTATION_SPEED) != 0:
            raise RuntimeError('dvz_anim_set_speed() failed')
        if dvz.dvz_anim_start(animation, 0.0) != 0:
            raise RuntimeError('dvz_anim_start() failed')


def _build_scene():
    scene, figure, panel = ex.scene_panel()
    dvz.dvz_panel_set_background_color(panel, PANEL_BG)
    _setup_camera(panel)
    _add_star_shell(scene, panel)
    mesh = _add_planet(scene, panel)
    orbit_model = _load_orbit_model()
    orbits = _add_orbit_traces(scene, panel, orbit_model)
    debris = _add_debris(scene, panel, orbit_model)
    state = GlobeState()
    _add_globe_rotation(scene, (mesh, orbits, debris), state)
    return scene, figure, panel, mesh, debris, orbit_model, state


def _configure_view(view, panel) -> None:
    desc = dvz.dvz_turntable_desc()
    desc.min_distance = 1.02
    desc.max_distance = 20.0
    desc.zoom_speed = 0.018
    turntable = dvz.dvz_view_turntable(view, panel, ctypes.byref(desc))
    if not turntable:
        raise RuntimeError('dvz_view_turntable() failed')


def main() -> None:
    scene, figure, panel, _mesh, debris, orbit_model, state = _build_scene()

    def configure(view) -> None:
        _configure_view(view, panel)

    def on_frame(_view, _frame_index: int, elapsed: float) -> None:
        positions = _orbit_positions(orbit_model, elapsed * DEBRIS_TIME_SCALE)
        if dvz.dvz_visual_set_data(debris, 'position', positions) != 0:
            raise RuntimeError('dvz_visual_set_data(debris) failed')

    print(
        f'textured_planet: {len(orbit_model.catalog_ids)} real catalogued objects, '
        f'snapshot {orbit_model.snapshot_utc}'
    )
    try:
        ex.run_with_frame_callback(
            scene,
            figure,
            'Textured Planets and Orbital Debris',
            on_frame,
            configure,
        )
    finally:
        state.destroy()


if __name__ == '__main__':
    main()
