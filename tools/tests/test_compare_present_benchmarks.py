#!/usr/bin/env python3
"""Focused tests for same-machine presentation benchmark comparison."""

from __future__ import annotations

import sys
from pathlib import Path
from tempfile import TemporaryDirectory
from unittest import TestCase, main


TOOLS_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TOOLS_DIR))

import compare_present_benchmarks as compare  # noqa: E402


class PresentBenchmarkComparisonTest(TestCase):
    def test_run_can_merge_stdout_and_stderr_into_one_log(self) -> None:
        with TemporaryDirectory() as temporary:
            log = Path(temporary) / "command.log"
            result = compare._run(
                [
                    sys.executable,
                    "-c",
                    "import sys; print('stdout'); print('stderr', file=sys.stderr)",
                ],
                cwd=TOOLS_DIR,
                stdout_path=log,
                stderr_path=log,
            )
            self.assertEqual(result.returncode, 0)
            self.assertEqual(set(log.read_text(encoding="utf8").splitlines()), {"stdout", "stderr"})

    def test_parse_live_canvas_metrics(self) -> None:
        stderr = """
benchmark: frames=120 warmup=12 samples=108 elapsed=0.010800s fps=10000.00 avg_ms=0.1000
benchmark: frame_ms min=0.0500 p50=0.0900 p90=0.1200 p95=0.1400 p99=0.2000 max=0.2500
benchmark: swapchain recreates total=1 steady=0
"""
        metrics = compare.parse_benchmark_output("blank", "", stderr)
        self.assertEqual(metrics.frames, 120)
        self.assertEqual(metrics.samples, 108)
        self.assertAlmostEqual(metrics.ms_per_frame, 0.1)
        self.assertEqual(metrics.recreate_steady, 0)

    def test_parse_live_canvas_rejects_recreation(self) -> None:
        stderr = """
benchmark: frames=20 warmup=2 samples=18 elapsed=0.018000s fps=1000.00 avg_ms=1.0000
benchmark: frame_ms min=0.5000 p50=0.9000 p90=1.1000 p95=1.2000 p99=1.3000 max=1.4000
benchmark: swapchain recreates total=3 steady=2
"""
        with self.assertRaisesRegex(compare.CompareError, "steady swapchain recreations"):
            compare.parse_benchmark_output("scene-drp2", "", stderr)

    def test_parse_panzoom_requires_version_marker(self) -> None:
        output = (
            "scenario_benchmark: scenario=start_scatter frames=60 warmup=6 "
            "elapsed=0.120000s fps=500.00\n"
        )
        with self.assertRaisesRegex(compare.CompareError, "does not implement panzoom-v1"):
            compare.parse_benchmark_output("scatter-panzoom", output, "")
        metrics = compare.parse_benchmark_output(
            "scatter-panzoom", "scenario_benchmark_workload: panzoom-v1\n" + output, ""
        )
        self.assertAlmostEqual(metrics.ms_per_frame, 2.0)

    def test_paired_summary_classifies_regression(self) -> None:
        summary = compare.summarize_pairs(
            "scatter", [1.0] * 9, [1.10] * 9, threshold_pct=3.0, seed=1,
            bootstrap_samples=200,
        )
        self.assertEqual(summary.verdict, "regression")
        self.assertAlmostEqual(summary.paired_delta_pct_median, 10.0)

    def test_paired_summary_classifies_improvement(self) -> None:
        summary = compare.summarize_pairs(
            "scatter", [1.0] * 9, [0.90] * 9, threshold_pct=3.0, seed=1,
            bootstrap_samples=200,
        )
        self.assertEqual(summary.verdict, "improvement")

    def test_paired_summary_classifies_small_change(self) -> None:
        summary = compare.summarize_pairs(
            "blank", [1.0] * 5, [1.01] * 5, threshold_pct=3.0, seed=2,
            bootstrap_samples=200,
        )
        self.assertEqual(summary.verdict, "no-material-change")

    def test_workload_commands_use_fixed_environment(self) -> None:
        revision = compare.Revision(
            name="base", ref="base", commit="0" * 40,
            worktree=Path("/tmp/base"), build_dir=Path("/tmp/base/build-compare"),
        )
        command, env = compare.workload_command(revision, "scatter-panzoom", 120)
        self.assertEqual(command[-2:], ["--benchmark", "120"])
        self.assertEqual(env["DVZ_PRESENT_MODE"], "immediate")
        self.assertEqual(env["DVZ_APP_SCHEDULE"], "continuous")
        self.assertEqual(env["DVZ_SCATTER_BENCHMARK"], "panzoom-v1")


if __name__ == "__main__":
    main()
