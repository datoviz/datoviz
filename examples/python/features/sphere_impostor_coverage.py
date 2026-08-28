#!/usr/bin/env python3
"""
Sphere impostor coverage check.

Renders large spheres offscreen and compares each rendered silhouette against a NumPy ray-traced
ground truth. The comparison is a set overlap -- intersection over union, plus the symmetric
difference relative to the truth mask -- so a silhouette that is too small and one that is too
large both fail. A one-sided coverage ratio would pass an impostor that floods the viewport.

This is a regression guard for lowering sphere impostors to quads. When spheres were drawn as
native point sprites, the impostor was one vertex sized by ``gl_PointSize``. That broke in two
ways once a sphere grew large on screen:

* ``gl_PointSize`` is clamped to ``VkPhysicalDeviceLimits::pointSizeRange`` (commonly 255 or
  2048 px), so the sprite stopped covering the silhouette;
* a point primitive is clipped by its centre vertex, so a sphere whose centre left the frustum
  vanished entirely even while its surface filled the view.

Because a perspective silhouette is not centred on the projected sphere centre, the clamped
sprite also cropped asymmetrically, and the surviving band appeared to slide as the camera
dollied in. Run this against the point-sprite path and the overlap collapses as the camera closes
in -- 88.7% at distance 15, 10.8% at 12, 2.5% at 9 on an RTX 5090 -- while the quad path holds
above 99% at every camera.

The perspective cases sweep the camera in along a fixed off-axis direction. The orthographic case
covers the other branch of ``sphereSilhouetteNdc()``: under an orthographic projection the
silhouette is a circle centred on the projected centre, and the eye ray varies in origin rather
than in direction.

``testing/test_sphere_impostor_coverage.py`` drives the same cases as a pytest regression test.

Run: python examples/python/features/sphere_impostor_coverage.py
"""

from __future__ import annotations

import argparse
import json
import math
from dataclasses import asdict, dataclass
from pathlib import Path

import numpy as np

import datoviz as dvz

WIDTH = 900
HEIGHT = 700
FOV_Y = math.radians(45.0)
NEAR_CLIP = 0.05
FAR_CLIP = 1000.0
BACKGROUND = (13, 15, 20)

# Channel-sum distance from the background above which a pixel counts as drawn.
DRAWN_THRESHOLD = 12

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

# Fixed off-axis view direction, shared by every case.
VIEW_DIRECTION = np.array([0.3, 1.0, 0.28])
VIEW_DIRECTION = VIEW_DIRECTION / np.linalg.norm(VIEW_DIRECTION)
VIEW_TARGET = np.zeros(3)

MIN_IOU = 0.97
MAX_SYMMETRIC_DIFFERENCE = 0.05

# Exit codes, so a test driver can tell a real regression from a machine with no GPU.
EXIT_FAILURE = 1
EXIT_UNAVAILABLE = 2


class CoverageUnavailableError(RuntimeError):
    """Raised when no GPU device is available to render the cases."""


@dataclass(frozen=True)
class Case:
    """One camera configuration to render and score."""

    name: str
    distance: float
    ortho_height: float | None = None

    @property
    def orthographic(self) -> bool:
        """Whether this case uses an orthographic projection."""
        return self.ortho_height is not None

    @property
    def eye(self) -> np.ndarray:
        """Camera position in world space."""
        return VIEW_TARGET + VIEW_DIRECTION * self.distance


@dataclass(frozen=True)
class Metrics:
    """Set-overlap scores between a rendered mask and its ray-traced truth."""

    truth_px: int
    drawn_px: int
    intersection_px: int
    union_px: int
    iou: float
    missing: float
    excess: float

    @property
    def symmetric_difference(self) -> float:
        """Symmetric difference between the two masks, relative to the truth mask."""
        return self.missing + self.excess

    @property
    def passed(self) -> bool:
        """Whether both overlap gates hold."""
        return self.iou >= MIN_IOU and self.symmetric_difference <= MAX_SYMMETRIC_DIFFERENCE


CASES = (
    Case('perspective d=60', 60.0),
    Case('perspective d=40', 40.0),
    Case('perspective d=25', 25.0),
    Case('perspective d=20', 20.0),
    Case('perspective d=15', 15.0),
    Case('perspective d=12', 12.0),
    Case('perspective d=9', 9.0),
    Case('perspective d=6', 6.0),
    # Orthographic silhouettes do not shrink with distance, so this case sits at a distance
    # where a perspective projection would render a visibly larger, off-centre silhouette:
    # a silent fallback to the perspective branch scores well under 60% IoU here.
    Case('orthographic h=34', 18.0, ortho_height=34.0),
)


def mask_metrics(drawn: np.ndarray, truth: np.ndarray) -> Metrics:
    """Score a rendered silhouette mask against its ray-traced truth mask."""
    truth_px = int(truth.sum())
    drawn_px = int(drawn.sum())
    intersection_px = int((drawn & truth).sum())
    union_px = int((drawn | truth).sum())
    # An empty truth mask means the case renders nothing; any drawn pixel is then pure excess.
    denominator = max(truth_px, 1)
    return Metrics(
        truth_px=truth_px,
        drawn_px=drawn_px,
        intersection_px=intersection_px,
        union_px=union_px,
        iou=intersection_px / union_px if union_px else 1.0,
        missing=(truth_px - intersection_px) / denominator,
        excess=(drawn_px - intersection_px) / denominator,
    )


def drawn_mask(rgba: np.ndarray) -> np.ndarray:
    """Mask of pixels that differ from the panel background."""
    background = np.array(BACKGROUND, dtype=np.int16)
    return np.abs(rgba[:, :, :3].astype(np.int16) - background).sum(-1) > DRAWN_THRESHOLD


def _camera_rays(case: Case) -> tuple[np.ndarray, np.ndarray]:
    """Per-pixel ray origins and unit directions matching the case camera."""
    eye = case.eye
    forward = VIEW_TARGET - eye
    forward /= np.linalg.norm(forward)
    right = np.cross(forward, np.array([0.0, 0.0, 1.0]))
    right /= np.linalg.norm(right)
    up = np.cross(right, forward)

    # Pixel centres in NDC, y pointing up.
    ndc_x = 2.0 * (np.arange(WIDTH) + 0.5) / WIDTH - 1.0
    ndc_y = 1.0 - 2.0 * (np.arange(HEIGHT) + 0.5) / HEIGHT
    grid_x, grid_y = np.meshgrid(ndc_x, ndc_y)

    if case.orthographic:
        # dvz_camera_mvp() builds the ortho box from a vertical world extent and the aspect
        # ratio, so every ray shares the view direction and differs only in origin.
        half_y = 0.5 * float(case.ortho_height)
        half_x = half_y * WIDTH / HEIGHT
        origins = eye + grid_x[..., None] * half_x * right + grid_y[..., None] * half_y * up
        directions = np.broadcast_to(forward, origins.shape).copy()
        return origins, directions

    half_y = math.tan(FOV_Y / 2.0)
    half_x = half_y * WIDTH / HEIGHT
    directions = forward + grid_x[..., None] * half_x * right + grid_y[..., None] * half_y * up
    directions /= np.linalg.norm(directions, axis=-1, keepdims=True)
    origins = np.broadcast_to(eye, directions.shape).copy()
    return origins, directions


def analytic_mask(case: Case) -> np.ndarray:
    """Ray-trace the sphere set with the same camera, as ground truth."""
    origins, directions = _camera_rays(case)
    mask = np.zeros((HEIGHT, WIDTH), dtype=bool)
    for cx, cy, cz, radius in SPHERES:
        oc = origins - np.array([cx, cy, cz])
        b = (directions * oc).sum(-1)
        c = (oc * oc).sum(-1) - radius * radius
        disc = b * b - c
        near = -b - np.sqrt(np.maximum(disc, 0.0))
        far = -b + np.sqrt(np.maximum(disc, 0.0))
        # The visible root is the near one unless the camera sits inside the sphere.
        hit = np.where(near > NEAR_CLIP, near, far)
        mask |= (disc >= 0.0) & (hit > NEAR_CLIP) & (hit < FAR_CLIP)
    return mask


def required_sprite_px(case: Case) -> float:
    """Point-sprite diameter the old native-point path would have asked for."""
    cx, cy, cz, radius = SPHERES[0]
    if case.orthographic:
        return 2.0 * radius * HEIGHT / float(case.ortho_height)
    distance = float(np.linalg.norm(case.eye - np.array([cx, cy, cz])))
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


def _apply_case(camera, case: Case) -> None:
    view = dvz.dvz_camera_view()
    view.eye[:] = tuple(float(v) for v in case.eye)
    view.target[:] = tuple(float(v) for v in VIEW_TARGET)
    view.up[:] = (0.0, 0.0, 1.0)
    if dvz.dvz_camera_set_view(camera, view) != 0:
        raise RuntimeError('dvz_camera_set_view() failed')
    if case.orthographic:
        rc = dvz.dvz_camera_set_orthographic(camera, float(case.ortho_height), NEAR_CLIP, FAR_CLIP)
        if rc != 0:
            raise RuntimeError('dvz_camera_set_orthographic() failed')
    elif dvz.dvz_camera_set_perspective(camera, FOV_Y, NEAR_CLIP, FAR_CLIP) != 0:
        raise RuntimeError('dvz_camera_set_perspective() failed')


def render_cases(cases=CASES, dump: Path | None = None) -> dict[str, Metrics]:
    """Render every case offscreen and score it against the ray-traced truth.

    Raises CoverageUnavailableError when no GPU device can serve the offscreen view.
    """
    scene = dvz.dvz_scene()
    if not scene:
        raise RuntimeError('dvz_scene() failed')
    app = None
    try:
        figure, camera = _build(scene)
        # Offscreen rendering needs no swapchain, so skip the GLFW surface extensions: they pull
        # in a window system this test never uses.
        config = dvz.dvz_app_config()
        config.enable_glfw_extensions = False
        app = dvz.dvz_app_with_config(scene, config)
        if not app:
            raise CoverageUnavailableError('dvz_app_with_config() failed')
        view = dvz.dvz_view_offscreen(app, figure, WIDTH, HEIGHT)
        if not view:
            raise CoverageUnavailableError('dvz_view_offscreen() failed')

        results = {}
        for case in cases:
            _apply_case(camera, case)
            if dvz.dvz_view_render_once(view) != 0:
                raise RuntimeError('dvz_view_render_once() failed')
            rgba = dvz.dvz_view_capture_rgba(view)
            results[case.name] = mask_metrics(drawn_mask(rgba), analytic_mask(case))
            if dump is not None:
                dump.mkdir(parents=True, exist_ok=True)
                np.save(dump / f'{case.name.replace(" ", "_")}.npy', rgba)
        return results
    finally:
        if app:
            dvz.dvz_app_destroy(app)
        dvz.dvz_scene_destroy(scene)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description='Sphere impostor coverage check.')
    parser.add_argument('--dump', type=Path, help='directory to write captured PNG-like .npy')
    parser.add_argument('--json', type=Path, help='file to write the per-case metrics to')
    args = parser.parse_args(argv)

    try:
        results = render_cases(dump=args.dump)
    except CoverageUnavailableError as exc:
        print(f'sphere impostor coverage: SKIP ({exc})')
        return EXIT_UNAVAILABLE

    if args.json is not None:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(
            json.dumps({name: asdict(m) for name, m in results.items()}, indent=2)
        )

    header = (
        f'{"case":>18} {"sprite px":>10} {"truth px":>10} {"drawn px":>10} '
        f'{"IoU":>8} {"missing":>8} {"excess":>8}'
    )
    print(header)
    failed = []
    for case in CASES:
        m = results[case.name]
        print(
            f'{case.name:>18} {required_sprite_px(case):10.0f} {m.truth_px:10d} '
            f'{m.drawn_px:10d} {m.iou:7.1%} {m.missing:7.1%} {m.excess:7.1%}'
        )
        if not m.passed:
            failed.append(case.name)

    if failed:
        print(f'sphere impostor coverage: FAIL ({", ".join(failed)})')
        return EXIT_FAILURE
    worst = min(m.iou for m in results.values())
    print(f'sphere impostor coverage: OK (worst IoU {worst:.1%})')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
