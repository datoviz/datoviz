#!/usr/bin/env python3
"""Animated NumPy density-wave spiral galaxy.

The density-wave model is adapted from Nicolas P. Rougier's Glumpy example.

Copyright (c) 2014 Nicolas P. Rougier.

Redistribution and use in source and binary forms, with or without modification, are permitted
provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this list of conditions
  and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice, this list of
  conditions and the following disclaimer in the documentation and/or other materials provided
  with the distribution.
* Neither Nicolas P. Rougier's name nor the names of contributors may be used to endorse or promote
  products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
"""

from __future__ import annotations

import ctypes
from dataclasses import dataclass

import numpy as np

import datoviz as dvz
from examples.python.gallery import common as ex


STAR_COUNT = 28_000
DUST_COUNT = 18_000
BACKGROUND_COUNT = 1_200
INITIAL_ANGLES = (ctypes.c_float * 3)(-0.302710, +0.044938, -0.017917)
BLACK_BODY = np.array(
    [[1.00, 0.23, 0.01], [1.00, 0.55, 0.25], [1.00, 0.78, 0.62], [1.00, 0.94, 0.99], [0.76, 0.80, 1.00], [0.60, 0.69, 1.00]],
    dtype=np.float32,
)


@dataclass
class Galaxy:
    radius: np.ndarray
    ratio: np.ndarray
    orientation: np.ndarray
    phase: np.ndarray
    height: np.ndarray
    colors: np.ndarray
    sizes: np.ndarray

    def positions(self, elapsed: float) -> np.ndarray:
        theta = self.phase + 0.095 * elapsed
        x = self.radius * np.cos(theta)
        y = self.radius * self.ratio * np.sin(theta)
        c, s = np.cos(self.orientation), np.sin(self.orientation)
        return np.column_stack((c * x - s * y, self.height, s * x + c * y)).astype(np.float32)


def _model() -> Galaxy:
    rng = np.random.default_rng(0x6A09E667)
    star_radius = np.abs(0.47 * rng.normal(size=STAR_COUNT))
    dust_radius = np.sqrt(rng.uniform(0.03, 1.0, DUST_COUNT))
    radius = np.concatenate((star_radius, dust_radius)).astype(np.float32)
    ratio = np.clip(1.0 - 0.10 * np.minimum(radius / 0.30, 1.0), 0.90, 1.0)
    orientation = np.where(np.arange(len(radius)) < STAR_COUNT, np.pi / 2 - 5.2 * radius, 5.2 * radius)
    phase = rng.uniform(0.0, 2.0 * np.pi, len(radius)).astype(np.float32)
    scale_height = np.where(np.arange(len(radius)) < STAR_COUNT, 0.012 + 0.055 * np.exp(-radius / 0.20), 0.007)
    height = (scale_height * rng.normal(size=len(radius))).astype(np.float32)

    temperature = np.concatenate((rng.uniform(3000, 9000, STAR_COUNT), 6000 + 3000 * dust_radius))
    coordinate = np.clip((temperature - 3000) / 6000 * (len(BLACK_BODY) - 1), 0, len(BLACK_BODY) - 1)
    lo = coordinate.astype(np.int32)
    hi = np.minimum(lo + 1, len(BLACK_BODY) - 1)
    rgb = (1.0 - (coordinate - lo)[:, None]) * BLACK_BODY[lo] + (coordinate - lo)[:, None] * BLACK_BODY[hi]
    alpha = np.concatenate((rng.uniform(45, 125, STAR_COUNT), rng.uniform(3, 18, DUST_COUNT)))
    colors = np.column_stack((np.clip(255 * rgb, 0, 255).astype(np.uint8), alpha.astype(np.uint8)))
    sizes = np.concatenate((rng.uniform(1.2, 3.2, STAR_COUNT), rng.uniform(5.0, 13.0, DUST_COUNT))).astype(np.float32)
    return Galaxy(radius, ratio, orientation.astype(np.float32), phase, height, colors, sizes)


def _background():
    rng = np.random.default_rng(42)
    directions = rng.normal(size=(BACKGROUND_COUNT, 3))
    directions /= np.linalg.norm(directions, axis=1, keepdims=True)
    positions = (8.0 * directions).astype(np.float32)
    brightness = rng.uniform(0.35, 1.0, BACKGROUND_COUNT)
    colors = np.column_stack((150 * brightness, 175 * brightness, 220 * brightness, np.full(BACKGROUND_COUNT, 255))).astype(np.uint8)
    sizes = rng.uniform(0.7, 2.0, BACKGROUND_COUNT).astype(np.float32)
    return positions, colors, sizes


def _build_scene():
    model = _model()
    scene, figure, panel = ex.scene_panel()
    dvz.dvz_panel_set_background_color(panel, dvz.DvzColor(0, 0, 8, 255))
    camera = dvz.dvz_camera_desc()
    camera.view.eye[:] = (0.0, -3.0, +4.8)
    camera.view.target[:] = (0.0, 0.0, 0.0)
    camera.view.up[:] = (0.0, 1.0, 0.0)
    camera.projection.fov_y = 0.55
    camera.projection.near_clip = 0.05
    camera.projection.far_clip = 100.0
    dvz.dvz_panel_set_camera_desc(panel, ctypes.byref(camera))

    background = dvz.dvz_point(scene, 0)
    positions, colors, sizes = _background()
    dvz.dvz_visual_set_data_many(background, {"position": positions, "color": colors, "diameter_px": sizes})
    ex.set_filled_point_style(background)
    dvz.dvz_visual_set_depth_test(background, False)
    ex.add_visual(panel, background)

    particles = dvz.dvz_point(scene, 0)
    dvz.dvz_visual_set_data_many(particles, {"position": model.positions(0.0), "color": model.colors, "diameter_px": model.sizes})
    ex.set_filled_point_style(particles)
    dvz.dvz_visual_set_alpha_mode(particles, dvz.DVZ_ALPHA_BLENDED)
    dvz.dvz_visual_set_blend_mode(particles, dvz.DVZ_BLEND_ADDITIVE)
    dvz.dvz_visual_set_depth_test(particles, False)
    ex.add_visual(panel, particles)
    return scene, figure, panel, particles, model


def main() -> None:
    scene, figure, panel, particles, model = _build_scene()

    def configure(view) -> None:
        arcball = dvz.dvz_view_arcball(view, panel, None)
        if not arcball or dvz.dvz_arcball_set(arcball, INITIAL_ANGLES) != 0:
            raise RuntimeError("arcball setup failed")
        dvz.dvz_arcball_zoom(arcball, 7.389051)

    def on_frame(_view, _frame_index: int, elapsed: float) -> None:
        if dvz.dvz_visual_set_data(particles, "position", model.positions(elapsed)) != 0:
            raise RuntimeError("galaxy position update failed")

    ex.run_with_frame_callback(scene, figure, "Density-Wave Galaxy", on_frame, configure)


if __name__ == "__main__":
    main()
