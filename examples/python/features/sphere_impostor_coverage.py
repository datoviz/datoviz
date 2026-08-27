#!/usr/bin/env python3
"""
Sphere impostor coverage check.

Renders a large sphere offscreen from a fixed direction at decreasing camera distances and
compares the rendered silhouette against a NumPy ray-traced ground truth. Coverage must stay
close to 100% at every distance.

This is a regression guard for lowering sphere impostors to quads. When spheres were drawn as
native point sprites, the impostor was one vertex sized by ``gl_PointSize``. That broke in two
ways once a sphere grew large on screen:

* ``gl_PointSize`` is clamped to ``VkPhysicalDeviceLimits::pointSizeRange`` (commonly 255 or
  2048 px), so the sprite stopped covering the silhouette;
* a point primitive is clipped by its centre vertex, so a sphere whose centre left the frustum
  vanished entirely even while its surface filled the view.

Because a perspective silhouette is not centred on the projected sphere centre, the clamped
sprite also cropped asymmetrically, and the surviving band appeared to slide as the camera
dollied in. Run this against the point-sprite path and coverage collapses (~1-60%) as the
camera closes in; against the quad path it stays at ~100%.

Run: python examples/python/features/sphere_impostor_coverage.py
"""

from __future__ import annotations

import argparse
import math
from pathlib import Path

import numpy as np

import datoviz as dvz

WIDTH = 900
HEIGHT = 700
FOV_Y = math.radians(45.0)
NEAR_CLIP = 0.05
FAR_CLIP = 1000.0
BACKGROUND = (13, 15, 20)

# One dominant sphere plus two small companions. The large one mirrors the case that exposed the
# bug: a wide sphere whose centre sits far off the view axis, so the silhouette is strongly
# off-centre and the centre leaves the frustum well before the surface does.
SPHERES = np.array(
    [
        [0.0, 0.0, -10.5, 10.8],
        [6.0, 1.5, 1.2, 0.5],
        [-5.0, -2.0, 0.8, 0.7],
    ],
    dtype=np.float64,
)

DISTANCES = (60.0, 40.0, 25.0, 20.0, 15.0, 12.0, 9.0, 6.0)
MIN_COVERAGE = 0.97


def _analytic_mask(eye: np.ndarray, target: np.ndarray) -> np.ndarray:
    """Ray-trace the sphere set with the same camera, as ground truth."""
    forward = target - eye
    forward /= np.linalg.norm(forward)
    right = np.cross(forward, np.array([0.0, 0.0, 1.0]))
    right /= np.linalg.norm(right)
    up = np.cross(right, forward)

    half_y = math.tan(FOV_Y / 2.0)
    half_x = half_y * WIDTH / HEIGHT
    xs = (2.0 * (np.arange(WIDTH) + 0.5) / WIDTH - 1.0) * half_x
    ys = (1.0 - 2.0 * (np.arange(HEIGHT) + 0.5) / HEIGHT) * half_y
    grid_x, grid_y = np.meshgrid(xs, ys)
    rays = forward + grid_x[..., None] * right + grid_y[..., None] * up
    rays /= np.linalg.norm(rays, axis=-1, keepdims=True)

    mask = np.zeros((HEIGHT, WIDTH), dtype=bool)
    for cx, cy, cz, radius in SPHERES:
        oc = eye - np.array([cx, cy, cz])
        b = (rays * oc).sum(-1)
        c = oc @ oc - radius * radius
        disc = b * b - c
        near = np.where(disc >= 0.0, -b - np.sqrt(np.maximum(disc, 0.0)), np.inf)
        mask |= (disc >= 0.0) & (near > 1e-4)
    return mask


def _required_sprite_px(eye: np.ndarray) -> float:
    """Point-sprite diameter the old native-point path would have asked for."""
    cx, cy, cz, radius = SPHERES[0]
    distance = float(np.linalg.norm(eye - np.array([cx, cy, cz])))
    if distance <= radius:
        return float('inf')
    return 2.0 * (0.5 * HEIGHT * (radius / distance) / math.tan(FOV_Y / 2.0))


def _build(scene):
    figure = dvz.dvz_figure(scene, WIDTH, HEIGHT, 0)
    if not figure:
        raise RuntimeError('dvz_figure() failed')
    panel = dvz.dvz_panel_full(figure)
    if not panel:
        raise RuntimeError('dvz_panel_full() failed')
    dvz.dvz_panel_set_background_color(panel, dvz.DvzColor(*BACKGROUND, 255))

    desc = dvz.dvz_camera_desc()
    desc.view.eye[:] = (0.0, -40.0, 0.0)
    desc.view.target[:] = (0.0, 0.0, 0.0)
    desc.view.up[:] = (0.0, 0.0, 1.0)
    desc.projection.fov_y = FOV_Y
    desc.projection.near_clip = NEAR_CLIP
    desc.projection.far_clip = FAR_CLIP
    if dvz.dvz_panel_set_camera_desc(panel, desc) != 0:
        raise RuntimeError('dvz_panel_set_camera_desc() failed')
    camera = dvz.dvz_panel_camera(panel)
    if not camera:
        raise RuntimeError('dvz_panel_camera() failed')

    visual = dvz.dvz_sphere(scene, dvz.DVZ_SPHERE_FLAGS_LIGHTING)
    if not visual:
        raise RuntimeError('dvz_sphere() failed')
    count = SPHERES.shape[0]
    payload = {
        'position': np.ascontiguousarray(SPHERES[:, :3], dtype=np.float32),
        'radius': np.ascontiguousarray(SPHERES[:, 3], dtype=np.float32),
        'color': np.tile(np.array([190, 190, 190, 255], dtype=np.uint8), (count, 1)),
    }
    if dvz.dvz_visual_set_data_many(visual, payload) != 0:
        raise RuntimeError('dvz_visual_set_data_many() failed')
    if dvz.dvz_panel_add_visual(panel, visual, None) != 0:
        raise RuntimeError('dvz_panel_add_visual() failed')
    return figure, camera


def _look_from(camera, eye: np.ndarray, target: np.ndarray) -> None:
    view = dvz.dvz_camera_view()
    view.eye[:] = tuple(float(v) for v in eye)
    view.target[:] = tuple(float(v) for v in target)
    view.up[:] = (0.0, 0.0, 1.0)
    if dvz.dvz_camera_set_view(camera, view) != 0:
        raise RuntimeError('dvz_camera_set_view() failed')


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description='Sphere impostor coverage check.')
    parser.add_argument('--dump', type=Path, help='directory to write captured PNG-like .npy')
    args = parser.parse_args(argv)

    scene = dvz.dvz_scene()
    if not scene:
        raise RuntimeError('dvz_scene() failed')
    app = None
    try:
        figure, camera = _build(scene)
        app = dvz.dvz_app(scene)
        if not app:
            print('sphere impostor coverage: SKIP (dvz_app() failed)')
            return 0
        view = dvz.dvz_view_offscreen(app, figure, WIDTH, HEIGHT)
        if not view:
            print('sphere impostor coverage: SKIP (dvz_view_offscreen() failed)')
            return 0

        background = np.array(BACKGROUND, dtype=np.int16)
        target = np.zeros(3)
        direction = np.array([0.3, 1.0, 0.28])
        direction /= np.linalg.norm(direction)

        print(f'{"distance":>9} {"sprite px":>10} {"truth px":>10} {"drawn px":>10} {"coverage":>9}')
        worst = 1.0
        for distance in DISTANCES:
            eye = target + direction * distance
            _look_from(camera, eye, target)
            if dvz.dvz_view_render_once(view) != 0:
                raise RuntimeError('dvz_view_render_once() failed')
            rgba = dvz.dvz_view_capture_rgba(view)
            drawn = np.abs(rgba[:, :, :3].astype(np.int16) - background).sum(-1) > 12
            truth = _analytic_mask(eye, target)
            coverage = float((drawn & truth).sum()) / max(int(truth.sum()), 1)
            worst = min(worst, coverage)
            print(
                f'{distance:9.1f} {_required_sprite_px(eye):10.0f} '
                f'{int(truth.sum()):10d} {int(drawn.sum()):10d} {coverage:8.1%}'
            )
            if args.dump is not None:
                args.dump.mkdir(parents=True, exist_ok=True)
                np.save(args.dump / f'sphere_{int(distance):03d}.npy', rgba)

        if worst < MIN_COVERAGE:
            print(f'sphere impostor coverage: FAIL (worst {worst:.1%} < {MIN_COVERAGE:.0%})')
            return 1
        print(f'sphere impostor coverage: OK (worst {worst:.1%})')
        return 0
    finally:
        if app:
            dvz.dvz_app_destroy(app)
        dvz.dvz_scene_destroy(scene)


if __name__ == '__main__':
    raise SystemExit(main())
