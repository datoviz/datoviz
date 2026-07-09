#!/usr/bin/env python3
"""Integer sampled field rendered as categorical image labels."""

from __future__ import annotations

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


FIELD_WIDTH = 512
FIELD_HEIGHT = 384
LABEL_IDS = (3, 8, 13, 21, 34, 55)
TAU = 2.0 * np.pi


def _label_field():
    x = np.linspace(0.0, 1.0, FIELD_WIDTH, dtype=np.float32)
    y = np.linspace(0.0, 1.0, FIELD_HEIGHT, dtype=np.float32)
    u, v = np.meshgrid(x, y)

    labels = np.zeros((FIELD_HEIGHT, FIELD_WIDTH), dtype=np.int32)
    dx = u - 0.50
    dy = v - 0.50
    tissue = dx * dx / 0.42 + dy * dy / 0.30
    mask = tissue <= 1.0

    wave = 0.5 + 0.5 * np.sin(TAU * (2.1 * u + 1.4 * v))
    bands = np.floor(3.0 * u + 2.0 * v + 0.8 * wave).astype(np.int32) % len(LABEL_IDS)
    ids = np.array(LABEL_IDS, dtype=np.int32)
    labels[mask] = ids[bands[mask]]

    island_dx = u - 0.68
    island_dy = v - 0.38
    island = island_dx * island_dx + 1.8 * island_dy * island_dy < 0.018
    labels[island & mask] = LABEL_IDS[4]
    return labels


def _labels_scale(scene):
    categories = (
        (LABEL_IDS[0], b"region", dvz.DvzColor(ex.CYAN.r, ex.CYAN.g, ex.CYAN.b, 220)),
        (LABEL_IDS[1], b"region", dvz.DvzColor(ex.GREEN.r, ex.GREEN.g, ex.GREEN.b, 220)),
        (LABEL_IDS[2], b"region", dvz.DvzColor(ex.YELLOW.r, ex.YELLOW.g, ex.YELLOW.b, 220)),
        (LABEL_IDS[3], b"region", dvz.DvzColor(ex.RED.r, ex.RED.g, ex.RED.b, 220)),
        (LABEL_IDS[4], b"region", dvz.DvzColor(ex.TEXT.r, ex.TEXT.g, ex.TEXT.b, 220)),
        (LABEL_IDS[5], b"region", dvz.DvzColor(ex.BLUE.r, ex.BLUE.g, ex.BLUE.b, 220)),
    )
    return ex.categorical_scale(scene, categories, b"labels")


def _add_labels(scene, panel, scale, labels: np.ndarray) -> None:
    field = dvz.dvz_sampled_field_from_array(
        scene,
        labels,
        format=dvz.DVZ_FIELD_FORMAT_R32_SINT,
        semantic=dvz.DVZ_FIELD_SEMANTIC_LABEL,
    )

    visual = dvz.dvz_labels(scene, 0)
    if not visual:
        raise RuntimeError("dvz_labels() failed")
    positions = np.array([[0.0, 0.0, 0.0]], dtype=np.float32)
    extents = np.array([[1.72, 1.28]], dtype=np.float32)
    if dvz.dvz_visual_set_data_many(visual, {"position": positions, "extent": extents}) != 0:
        raise RuntimeError("dvz_visual_set_data_many(labels) failed")
    if dvz.dvz_visual_set_field(visual, b"field", field) != 0:
        raise RuntimeError("dvz_visual_set_field(labels) failed")
    if dvz.dvz_visual_set_scale(visual, b"labels", scale) != 0:
        raise RuntimeError("dvz_visual_set_scale(labels) failed")
    if dvz.dvz_labels_set_opacity(visual, 0.92) != 0:
        raise RuntimeError("dvz_labels_set_opacity() failed")
    if dvz.dvz_labels_set_background(visual, 0) != 0:
        raise RuntimeError("dvz_labels_set_background() failed")
    if dvz.dvz_labels_set_boundary(visual, True, 1.5, ex.BG) != 0:
        raise RuntimeError("dvz_labels_set_boundary() failed")
    if dvz.dvz_visual_set_depth_test(visual, False) != 0:
        raise RuntimeError("dvz_visual_set_depth_test(labels) failed")
    if dvz.dvz_visual_set_alpha_mode(visual, dvz.DVZ_ALPHA_BLENDED) != 0:
        raise RuntimeError("dvz_visual_set_alpha_mode(labels) failed")
    ex.add_visual(panel, visual)


def _build_scene():
    scene, figure, panel = ex.scene_panel()
    labels = _label_field()
    scale = _labels_scale(scene)
    _add_labels(scene, panel, scale, labels)
    return scene, figure, panel


def _configure_view(view, scene, panel) -> None:
    ex.bind_panzoom(view, scene, panel, dvz.DVZ_DIM_MASK_XY)


def main() -> None:
    scene, figure, panel = _build_scene()

    def configure(view) -> None:
        _configure_view(view, scene, panel)

    ex.run_with_view(scene, figure, "Labels", configure)


if __name__ == "__main__":
    main()
