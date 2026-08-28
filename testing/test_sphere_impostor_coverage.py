"""Regression test for the sphere impostor silhouette.

Sphere impostors used to be native point sprites, which the device clamped to
``VkPhysicalDeviceLimits::pointSizeRange`` and clipped by their centre vertex, cropping any
sphere that grew large on screen. They are now instanced quads spanning the exact NDC silhouette
bounds. This test renders that silhouette offscreen and compares it against a NumPy ray-traced
ground truth with a set-overlap score, so both an impostor that is too small and one that
overdraws the viewport fail.

The rendering half runs the example script in a subprocess: creating a Vulkan device is the one
step that can abort the interpreter on a broken driver stack, and an abort there must not take
down the rest of the pytest session.
"""

from __future__ import annotations

import importlib.util
import json
import os
import subprocess
import sys
from pathlib import Path

import numpy as np
import pytest

ROOT_DIR = Path(__file__).resolve().parents[1]
SCRIPT_PATH = ROOT_DIR / 'examples' / 'python' / 'features' / 'sphere_impostor_coverage.py'
EXIT_FAILURE = 1
EXIT_UNAVAILABLE = 2


def _library_exists() -> bool:
    names = ('libdatoviz.so', 'libdatoviz.dylib', 'libdatoviz.dll')
    return any((ROOT_DIR / 'build' / 'src' / name).exists() for name in names) or any(
        (ROOT_DIR / 'build' / name).exists() for name in names
    )


if not _library_exists():
    pytest.skip('libdatoviz has not been built', allow_module_level=True)


def _load_script():
    """Import the example script as a module, reusing it across tests."""
    name = 'sphere_impostor_coverage'
    cached = sys.modules.get(name)
    if cached is not None:
        return cached
    spec = importlib.util.spec_from_file_location(name, SCRIPT_PATH)
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    # Register before exec so the module's dataclasses can resolve their own module.
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


def _child_env() -> dict[str, str]:
    """Environment for the render subprocess, with the repository importable."""
    env = dict(os.environ)
    existing = env.get('PYTHONPATH')
    env['PYTHONPATH'] = f'{ROOT_DIR}{os.pathsep}{existing}' if existing else str(ROOT_DIR)
    return env


@pytest.fixture(scope='module')
def coverage_metrics(tmp_path_factory) -> dict[str, dict[str, float]]:
    """Render every case in a subprocess and return the per-case metrics."""
    report = tmp_path_factory.mktemp('sphere_impostor') / 'metrics.json'
    completed = subprocess.run(  # noqa: S603
        [sys.executable, str(SCRIPT_PATH), '--json', str(report)],
        cwd=ROOT_DIR,
        env=_child_env(),
        capture_output=True,
        text=True,
        check=False,
    )
    if completed.returncode == EXIT_UNAVAILABLE:
        pytest.skip(f'no GPU device for the offscreen view: {completed.stdout.strip()}')
    if completed.returncode not in (0, EXIT_FAILURE):
        # A device that dies before the first frame says nothing about the impostor geometry.
        pytest.skip(
            f'sphere impostor render did not run (rc={completed.returncode}): '
            f'{completed.stderr.strip()[-500:]}'
        )
    # The script writes the report before applying its own gates, so a rendered regression comes
    # back as metrics and fails per case below rather than as a fixture error.
    assert report.exists(), completed.stderr
    return json.loads(report.read_text())


@pytest.mark.parametrize('case_name', [case.name for case in _load_script().CASES])
def test_sphere_impostor_matches_ray_traced_silhouette(coverage_metrics, case_name) -> None:
    """Every camera must render a silhouette that neither crops nor overdraws the truth."""
    script = _load_script()
    metrics = coverage_metrics[case_name]
    symmetric_difference = metrics['missing'] + metrics['excess']
    assert metrics['truth_px'] > 0, 'the ray-traced truth mask is empty'
    assert metrics['iou'] >= script.MIN_IOU, metrics
    assert symmetric_difference <= script.MAX_SYMMETRIC_DIFFERENCE, metrics


def test_orthographic_case_is_one_a_perspective_fallback_would_fail() -> None:
    """Guard the orthographic case against being a camera perspective would render alike."""
    script = _load_script()
    ortho = next(case for case in script.CASES if case.orthographic)
    perspective = script.Case(f'{ortho.name} as perspective', ortho.distance)
    metrics = script.mask_metrics(script.analytic_mask(ortho), script.analytic_mask(perspective))
    assert metrics.iou < 0.75, metrics


def test_mask_metrics_rejects_a_cropped_silhouette() -> None:
    """A silhouette that is too small must fail both gates."""
    script = _load_script()
    truth = np.zeros((64, 64), dtype=bool)
    truth[8:56, 8:56] = True
    drawn = np.zeros_like(truth)
    drawn[8:40, 8:40] = True

    metrics = script.mask_metrics(drawn, truth)

    assert metrics.excess == 0.0
    assert metrics.missing > 0.5
    assert metrics.iou < script.MIN_IOU
    assert metrics.symmetric_difference > script.MAX_SYMMETRIC_DIFFERENCE
    assert not metrics.passed


def test_mask_metrics_rejects_an_overdrawn_silhouette() -> None:
    """A silhouette that floods the viewport must fail too, unlike a one-sided coverage ratio."""
    script = _load_script()
    truth = np.zeros((64, 64), dtype=bool)
    truth[8:56, 8:56] = True
    drawn = np.ones_like(truth)

    metrics = script.mask_metrics(drawn, truth)

    # Every truth pixel is drawn, so one-sided coverage would report a perfect 100%.
    assert metrics.missing == 0.0
    assert metrics.excess > 0.5
    assert metrics.iou < script.MIN_IOU
    assert metrics.symmetric_difference > script.MAX_SYMMETRIC_DIFFERENCE
    assert not metrics.passed


def test_mask_metrics_scores_an_exact_match() -> None:
    """An identical mask pair must score a perfect overlap."""
    script = _load_script()
    truth = np.zeros((64, 64), dtype=bool)
    truth[8:56, 8:56] = True

    metrics = script.mask_metrics(truth.copy(), truth)

    assert metrics.iou == 1.0
    assert metrics.symmetric_difference == 0.0
    assert metrics.passed
