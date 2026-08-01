#!/usr/bin/env python3
"""Compare Datoviz presentation benchmarks between two commits on one machine."""

from __future__ import annotations

import argparse
import json
import math
import os
import platform
import random
import re
import shlex
import shutil
import statistics
import subprocess
import sys
import tempfile
import time
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Any, Sequence


SCHEMA_VERSION = 1
DEFAULT_WORKLOADS = (
    "blank",
    "scene-drp2",
    "scene-drp2-cached-plan",
    "scene-drp2-cached-stream",
    "scene-drp2-10k",
    "scatter-1",
    "scatter",
    "scatter-panzoom",
)
EXTERNAL_SUBMODULES = (
    "external/cglm",
    "external/cimgui",
    "external/glfw",
    "external/kvazaar",
    "external/mimalloc",
    "external/msdf-atlas-gen",
)
LIVE_SUMMARY_RE = re.compile(
    r"benchmark: frames=(?P<frames>\d+) warmup=(?P<warmup>\d+) samples=(?P<samples>\d+) "
    r"elapsed=(?P<elapsed>[0-9.]+)s fps=(?P<fps>[0-9.]+) avg_ms=(?P<avg>[0-9.]+)"
)
LIVE_PERCENTILES_RE = re.compile(
    r"benchmark: frame_ms min=(?P<minimum>[0-9.]+) p50=(?P<p50>[0-9.]+) "
    r"p90=(?P<p90>[0-9.]+) p95=(?P<p95>[0-9.]+) p99=(?P<p99>[0-9.]+) "
    r"max=(?P<maximum>[0-9.]+)"
)
LIVE_RECREATE_RE = re.compile(
    r"benchmark: swapchain recreates total=(?P<total>\d+) steady=(?P<steady>\d+)"
)
SCENARIO_SUMMARY_RE = re.compile(
    r"scenario_benchmark: scenario=(?P<scenario>\S+) frames=(?P<frames>\d+) "
    r"warmup=(?P<warmup>\d+) elapsed=(?P<elapsed>[0-9.]+)s fps=(?P<fps>[0-9.]+)"
)
APP_FRAME_TIMING_RE = re.compile(r"^app_frame_timing: (?P<fields>.+)$", re.MULTILINE)


class CompareError(RuntimeError):
    """Expected comparison failure with a concise user-facing message."""


@dataclass(frozen=True)
class BenchmarkMetrics:
    frames: int
    warmup: int
    samples: int
    elapsed_s: float
    fps: float
    ms_per_frame: float
    avg_ms: float | None = None
    p50_ms: float | None = None
    p95_ms: float | None = None
    p99_ms: float | None = None
    recreate_total: int | None = None
    recreate_steady: int | None = None
    phase_ms: dict[str, float] | None = None


@dataclass(frozen=True)
class RunResult:
    workload: str
    pair: int
    order: int
    revision: str
    commit: str
    command: list[str]
    elapsed_wall_s: float
    exit_code: int
    stdout_path: str
    stderr_path: str
    metrics: BenchmarkMetrics


@dataclass(frozen=True)
class WorkloadSummary:
    workload: str
    pairs: int
    base_median_ms: float
    candidate_median_ms: float
    paired_delta_pct_median: float
    ci95_pct: tuple[float, float]
    threshold_pct: float
    verdict: str


@dataclass(frozen=True)
class Revision:
    name: str
    ref: str
    commit: str
    worktree: Path
    build_dir: Path


def _run(
    command: Sequence[str],
    *,
    cwd: Path,
    env: dict[str, str] | None = None,
    stdout_path: Path | None = None,
    stderr_path: Path | None = None,
    check: bool = True,
) -> subprocess.CompletedProcess[str]:
    shared_log = stdout_path is not None and stdout_path == stderr_path
    stdout_file = stdout_path.open("w", encoding="utf8") if stdout_path is not None else None
    stderr_file = (
        None
        if shared_log
        else stderr_path.open("w", encoding="utf8") if stderr_path is not None else None
    )
    try:
        completed = subprocess.run(
            list(command),
            cwd=cwd,
            env=env,
            text=True,
            stdout=stdout_file if stdout_file is not None else subprocess.PIPE,
            stderr=(
                subprocess.STDOUT
                if shared_log
                else stderr_file if stderr_file is not None else subprocess.PIPE
            ),
            check=False,
        )
    finally:
        if stdout_file is not None:
            stdout_file.close()
        if stderr_file is not None:
            stderr_file.close()
    if check and completed.returncode != 0:
        rendered = shlex.join(command)
        raise CompareError(f"command failed ({completed.returncode}): {rendered}")
    return completed


def resolve_commit(repo: Path, ref: str) -> str:
    completed = _run(["git", "rev-parse", f"{ref}^{{commit}}"], cwd=repo, check=False)
    if completed.returncode != 0:
        raise CompareError(f"unable to resolve Git reference '{ref}'")
    commit = completed.stdout.strip()
    if not re.fullmatch(r"[0-9a-f]{40}", commit):
        raise CompareError(f"Git reference '{ref}' did not resolve to a commit")
    return commit


def parse_benchmark_output(workload: str, stdout: str, stderr: str) -> BenchmarkMetrics:
    combined = f"{stdout}\n{stderr}"
    if workload == "blank" or workload.startswith("scene-drp2"):
        summary = LIVE_SUMMARY_RE.search(combined)
        percentiles = LIVE_PERCENTILES_RE.search(combined)
        recreate = LIVE_RECREATE_RE.search(combined)
        if summary is None or percentiles is None or recreate is None:
            raise CompareError(f"{workload}: incomplete dvz_live_canvas benchmark output")
        frames = int(summary.group("frames"))
        samples = int(summary.group("samples"))
        elapsed_s = float(summary.group("elapsed"))
        if frames < 2 or samples < 1 or elapsed_s <= 0:
            raise CompareError(f"{workload}: invalid benchmark counts or elapsed time")
        steady = int(recreate.group("steady"))
        if steady != 0:
            raise CompareError(f"{workload}: observed {steady} steady swapchain recreations")
        expected_points = 10_000 if workload == "scene-drp2-10k" else 1
        if workload.startswith("scene-drp2") and f"benchmark: scene_points={expected_points}" not in combined:
            raise CompareError(f"{workload}: expected scene_points={expected_points}")
        return BenchmarkMetrics(
            frames=frames,
            warmup=int(summary.group("warmup")),
            samples=samples,
            elapsed_s=elapsed_s,
            fps=float(summary.group("fps")),
            ms_per_frame=1000.0 * elapsed_s / samples,
            avg_ms=float(summary.group("avg")),
            p50_ms=float(percentiles.group("p50")),
            p95_ms=float(percentiles.group("p95")),
            p99_ms=float(percentiles.group("p99")),
            recreate_total=int(recreate.group("total")),
            recreate_steady=steady,
        )

    summary = SCENARIO_SUMMARY_RE.search(combined)
    if summary is None:
        raise CompareError(f"{workload}: incomplete scenario benchmark output")
    frames = int(summary.group("frames"))
    elapsed_s = float(summary.group("elapsed"))
    if frames < 2 or elapsed_s <= 0:
        raise CompareError(f"{workload}: invalid benchmark counts or elapsed time")
    if summary.group("scenario") != "start_scatter":
        raise CompareError(f"{workload}: unexpected scenario '{summary.group('scenario')}'")
    if workload == "scatter-panzoom" and "scenario_benchmark_workload: panzoom-v1" not in combined:
        raise CompareError("scatter-panzoom: commit does not implement panzoom-v1")
    expected_points = 1 if workload == "scatter-1" else 10_000
    if f"scenario_benchmark_points: {expected_points}" not in combined:
        raise CompareError(f"{workload}: expected {expected_points} scatter points")
    timing_matches = list(APP_FRAME_TIMING_RE.finditer(combined))
    phase_ms = None
    if timing_matches:
        phase_ms = {}
        for field in timing_matches[-1].group("fields").split():
            key, separator, value = field.partition("=")
            if separator and key not in {"view", "frames"}:
                phase_ms[key] = float(value)
    return BenchmarkMetrics(
        frames=frames,
        warmup=int(summary.group("warmup")),
        samples=frames,
        elapsed_s=elapsed_s,
        fps=float(summary.group("fps")),
        ms_per_frame=1000.0 * elapsed_s / frames,
        phase_ms=phase_ms,
    )


def _percentile(sorted_values: Sequence[float], quantile: float) -> float:
    if not sorted_values:
        raise ValueError("percentile requires at least one value")
    if len(sorted_values) == 1:
        return sorted_values[0]
    position = quantile * (len(sorted_values) - 1)
    lower = math.floor(position)
    upper = math.ceil(position)
    if lower == upper:
        return sorted_values[lower]
    weight = position - lower
    return sorted_values[lower] * (1.0 - weight) + sorted_values[upper] * weight


def bootstrap_median_ci(
    values: Sequence[float], *, seed: int, samples: int = 10_000
) -> tuple[float, float]:
    if not values:
        raise ValueError("bootstrap requires at least one value")
    rng = random.Random(seed)
    medians = []
    count = len(values)
    for _ in range(samples):
        medians.append(statistics.median(values[rng.randrange(count)] for _ in range(count)))
    medians.sort()
    return _percentile(medians, 0.025), _percentile(medians, 0.975)


def summarize_pairs(
    workload: str,
    base_ms: Sequence[float],
    candidate_ms: Sequence[float],
    *,
    threshold_pct: float,
    seed: int,
    bootstrap_samples: int = 10_000,
) -> WorkloadSummary:
    if len(base_ms) != len(candidate_ms) or not base_ms:
        raise ValueError("paired summaries require equal non-empty samples")
    if any(value <= 0 or not math.isfinite(value) for value in (*base_ms, *candidate_ms)):
        raise ValueError("frame times must be finite and positive")
    deltas = [100.0 * (candidate / base - 1.0) for base, candidate in zip(base_ms, candidate_ms)]
    median_delta = statistics.median(deltas)
    ci_low, ci_high = bootstrap_median_ci(deltas, seed=seed, samples=bootstrap_samples)
    if ci_low > threshold_pct:
        verdict = "regression"
    elif ci_high < -threshold_pct:
        verdict = "improvement"
    elif abs(median_delta) <= threshold_pct:
        verdict = "no-material-change"
    else:
        verdict = "inconclusive"
    return WorkloadSummary(
        workload=workload,
        pairs=len(base_ms),
        base_median_ms=statistics.median(base_ms),
        candidate_median_ms=statistics.median(candidate_ms),
        paired_delta_pct_median=median_delta,
        ci95_pct=(ci_low, ci_high),
        threshold_pct=threshold_pct,
        verdict=verdict,
    )


def workload_command(revision: Revision, workload: str, frames: int) -> tuple[list[str], dict[str, str]]:
    env = os.environ.copy()
    env["DVZ_PRESENT_MODE"] = "immediate"
    env["DVZ_APP_SCHEDULE"] = "continuous"
    if workload == "blank" or workload.startswith("scene-drp2"):
        draw = "clear" if workload == "blank" else "scene-drp2"
        command = [
            str(revision.build_dir / "testing" / "dvz_live_canvas"),
            "--benchmark",
            "--frames",
            str(frames),
            "--draw",
            draw,
            "--present",
            "immediate",
        ]
        if workload == "scene-drp2-cached-plan":
            command.extend(["--scene-path", "cached-plan"])
        elif workload == "scene-drp2-cached-stream":
            command.extend(["--scene-path", "cached-stream"])
        elif workload == "scene-drp2-10k":
            command.extend(["--scene-points", "10000"])
    elif workload in ("scatter-1", "scatter", "scatter-panzoom"):
        env["DVZ_APP_FRAME_TIMING"] = "1"
        command = [
            str(revision.build_dir / "examples" / "c" / "start" / "scatter"),
            "--benchmark",
            str(frames),
        ]
        if workload == "scatter-panzoom":
            env["DVZ_SCATTER_BENCHMARK"] = "panzoom-v1"
        else:
            env.pop("DVZ_SCATTER_BENCHMARK", None)
        if workload == "scatter-1":
            env["DVZ_SCATTER_POINT_COUNT"] = "1"
        else:
            env.pop("DVZ_SCATTER_POINT_COUNT", None)
    else:
        raise CompareError(f"unknown workload '{workload}'")
    return command, env


def _probe(command: Sequence[str], cwd: Path) -> str | None:
    try:
        completed = subprocess.run(
            list(command), cwd=cwd, text=True, stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT, timeout=10, check=False
        )
    except (OSError, subprocess.TimeoutExpired):
        return None
    output = completed.stdout.strip()
    return output if output else None


def machine_fingerprint(repo: Path) -> dict[str, Any]:
    env_keys = sorted(key for key in os.environ if key.startswith("DVZ_") or key in {"DISPLAY", "WAYLAND_DISPLAY"})
    cpu = None
    cpuinfo = Path("/proc/cpuinfo")
    if cpuinfo.is_file():
        for line in cpuinfo.read_text(encoding="utf8", errors="replace").splitlines():
            if line.startswith("model name") and ":" in line:
                cpu = line.split(":", 1)[1].strip()
                break
    return {
        "platform": platform.platform(),
        "system": platform.system(),
        "release": platform.release(),
        "machine": platform.machine(),
        "cpu": cpu or platform.processor() or None,
        "logical_cpus": os.cpu_count(),
        "display": os.environ.get("DISPLAY"),
        "wayland_display": os.environ.get("WAYLAND_DISPLAY"),
        "nvidia_smi": _probe(
            ["nvidia-smi", "--query-gpu=name,driver_version,pci.bus_id", "--format=csv,noheader"], repo
        ),
        "vulkaninfo_summary": _probe(["vulkaninfo", "--summary"], repo),
        "xrandr": _probe(["xrandr", "--current"], repo),
        "environment": {key: os.environ[key] for key in env_keys},
    }


def _gitlink(repo: Path, commit: str, path: str) -> str | None:
    completed = _run(["git", "ls-tree", commit, path], cwd=repo, check=False)
    fields = completed.stdout.strip().split()
    return fields[2] if len(fields) >= 3 and fields[0] == "160000" else None


class WorktreePair:
    def __init__(self, repo: Path, base_ref: str, candidate_ref: str, *, keep: bool = False):
        self.repo = repo
        self.base_ref = base_ref
        self.candidate_ref = candidate_ref
        self.base_commit = resolve_commit(repo, base_ref)
        self.candidate_commit = resolve_commit(repo, candidate_ref)
        self.keep = keep
        self.root = Path(tempfile.mkdtemp(prefix="dvz-compare."))
        self.created: list[Path] = []

    def __enter__(self) -> tuple[Revision, Revision]:
        try:
            base = self._add("base", self.base_ref, self.base_commit)
            candidate = self._add("candidate", self.candidate_ref, self.candidate_commit)
            return base, candidate
        except Exception:
            self.cleanup()
            raise

    def _add(self, name: str, ref: str, commit: str) -> Revision:
        path = self.root / name
        _run(["git", "worktree", "add", "--detach", str(path), commit], cwd=self.repo)
        self.created.append(path)
        self._link_data(path, commit)
        existing = [submodule for submodule in EXTERNAL_SUBMODULES if (path / submodule).exists()]
        if existing:
            _run(["git", "submodule", "update", "--init", "--recursive", "--", *existing], cwd=path)
        return Revision(name=name, ref=ref, commit=commit, worktree=path, build_dir=path / "build-compare")

    def _link_data(self, worktree: Path, commit: str) -> None:
        expected = _gitlink(self.repo, commit, "data")
        source = self.repo / "data"
        if expected is None:
            raise CompareError(f"{commit[:12]} does not contain the data gitlink")
        if not source.is_dir():
            raise CompareError("the repository data submodule is not checked out")
        current = _run(["git", "rev-parse", "HEAD"], cwd=source, check=False)
        if current.returncode != 0 or current.stdout.strip() != expected:
            observed = current.stdout.strip()[:12] if current.returncode == 0 else "unavailable"
            raise CompareError(
                f"{commit[:12]} requires data {expected[:12]}, but the checkout has {observed}"
            )
        dirty = _run(["git", "status", "--porcelain"], cwd=source, check=False)
        if dirty.returncode != 0 or dirty.stdout.strip():
            raise CompareError("the checked-out data submodule must be clean for a reproducible comparison")
        destination = worktree / "data"
        destination.rmdir()
        destination.symlink_to(source.resolve(), target_is_directory=True)

    def cleanup(self) -> None:
        if self.keep:
            print(f"comparison worktrees retained at {self.root}", file=sys.stderr)
            return
        for path in reversed(self.created):
            _run(["git", "worktree", "remove", "--force", str(path)], cwd=self.repo, check=False)
        if self.root.parent == Path(tempfile.gettempdir()) and self.root.name.startswith("dvz-compare."):
            shutil.rmtree(self.root, ignore_errors=True)

    def __exit__(self, exc_type: object, exc: object, traceback: object) -> None:
        self.cleanup()


def build_revision(
    revision: Revision,
    *,
    jobs: int,
    cmake_args: Sequence[str],
    log_dir: Path,
) -> None:
    configure_log = log_dir / f"build-{revision.name}-configure.log"
    build_log = log_dir / f"build-{revision.name}.log"
    command = [
        "cmake", "-S", str(revision.worktree), "-B", str(revision.build_dir), "-GNinja",
        "-DCMAKE_BUILD_TYPE=RelWithDebInfo", "-DDVZ_ENABLE_CUDA=OFF",
        "-DDVZ_ENABLE_KVAZAAR=OFF", "-DDVZ_ENABLE_QT_BRIDGE=OFF", *cmake_args,
    ]
    _run(command, cwd=revision.worktree, stdout_path=configure_log, stderr_path=configure_log)
    _run(
        ["cmake", "--build", str(revision.build_dir), "--target", "dvz_live_canvas", "scatter", "--parallel", str(jobs)],
        cwd=revision.worktree, stdout_path=build_log, stderr_path=build_log,
    )


def execute_benchmark(
    revision: Revision,
    workload: str,
    frames: int,
    *,
    pair: int,
    order: int,
    log_dir: Path,
    label: str,
) -> RunResult:
    command, env = workload_command(revision, workload, frames)
    stem = f"{workload}-pair{pair:02d}-order{order}-{label}"
    stdout_path = log_dir / f"{stem}.stdout.log"
    stderr_path = log_dir / f"{stem}.stderr.log"
    started = time.perf_counter()
    completed = _run(
        command, cwd=revision.worktree, env=env, stdout_path=stdout_path,
        stderr_path=stderr_path, check=False,
    )
    elapsed_wall_s = time.perf_counter() - started
    stdout = stdout_path.read_text(encoding="utf8", errors="replace")
    stderr = stderr_path.read_text(encoding="utf8", errors="replace")
    if completed.returncode != 0:
        raise CompareError(
            f"{workload}: {revision.name} benchmark failed ({completed.returncode}); see {stderr_path}"
        )
    if re.search(r"Validation Error|VUID-|scene-drp2: failed|canvas (?:frame|submit) error", f"{stdout}\n{stderr}"):
        raise CompareError(f"{workload}: {revision.name} reported a rendering or validation failure")
    metrics = parse_benchmark_output(workload, stdout, stderr)
    if metrics.frames != frames:
        raise CompareError(f"{workload}: expected {frames} frames, observed {metrics.frames}")
    return RunResult(
        workload=workload, pair=pair, order=order, revision=revision.name,
        commit=revision.commit, command=command, elapsed_wall_s=elapsed_wall_s,
        exit_code=completed.returncode, stdout_path=str(stdout_path), stderr_path=str(stderr_path),
        metrics=metrics,
    )


def _report_markdown(report: dict[str, Any]) -> str:
    lines = [
        "# Present benchmark comparison",
        "",
        f"Reference: `{report['invocation']['base_ref']}` (`{report['invocation']['base_commit'][:12]}`)",
        "",
        f"Candidate: `{report['invocation']['candidate_ref']}` (`{report['invocation']['candidate_commit'][:12]}`)",
        "",
        "| Workload | Base ms/frame | Candidate ms/frame | Paired delta | 95% CI | Verdict |",
        "| --- | ---: | ---: | ---: | ---: | --- |",
    ]
    for summary in report["summaries"]:
        ci = summary["ci95_pct"]
        lines.append(
            f"| {summary['workload']} | {summary['base_median_ms']:.4f} | "
            f"{summary['candidate_median_ms']:.4f} | {summary['paired_delta_pct_median']:+.2f}% | "
            f"[{ci[0]:+.2f}%, {ci[1]:+.2f}%] | {summary['verdict']} |"
        )
    lines.extend(["", "The verdict uses paired frame-time deltas; positive values are slower.", ""])
    return "\n".join(lines)


def _parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("reference", help="reference Git revision")
    parser.add_argument("candidate", nargs="?", default="HEAD", help="candidate Git revision")
    parser.add_argument("--runs", type=int, default=9, help="paired measured runs per workload")
    parser.add_argument("--frames", type=int, default=1200, help="measured frames per run")
    parser.add_argument("--threshold-pct", type=float, default=3.0, help="practical regression threshold")
    parser.add_argument("--seed", type=int, default=20260801, help="order/bootstrap random seed")
    parser.add_argument("--bootstrap-samples", type=int, default=10_000)
    parser.add_argument("--jobs", type=int, default=max(1, os.cpu_count() or 1))
    parser.add_argument("--workload", action="append", choices=DEFAULT_WORKLOADS, dest="workloads")
    parser.add_argument("--cmake-arg", action="append", default=[])
    parser.add_argument("--output", type=Path, help="report directory")
    parser.add_argument("--fail-on-regression", action="store_true")
    parser.add_argument("--keep-worktrees", action="store_true")
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    args = _parser().parse_args(argv)
    if args.runs < 3:
        raise CompareError("--runs must be at least three")
    if args.frames < 2:
        raise CompareError("--frames must be at least two")
    if args.threshold_pct < 0 or not math.isfinite(args.threshold_pct):
        raise CompareError("--threshold-pct must be finite and non-negative")
    if args.bootstrap_samples < 100:
        raise CompareError("--bootstrap-samples must be at least 100")
    if args.jobs < 1:
        raise CompareError("--jobs must be positive")

    repo_completed = subprocess.run(
        ["git", "rev-parse", "--show-toplevel"], text=True, stdout=subprocess.PIPE,
        stderr=subprocess.PIPE, check=False
    )
    if repo_completed.returncode != 0:
        raise CompareError("run this command inside a Git repository")
    repo = Path(repo_completed.stdout.strip()).resolve()
    workloads = tuple(args.workloads or DEFAULT_WORKLOADS)
    base_commit = resolve_commit(repo, args.reference)
    candidate_commit = resolve_commit(repo, args.candidate)
    timestamp = time.strftime("%Y%m%d-%H%M%S")
    output = args.output or repo / "build" / "perf-comparisons" / f"{base_commit[:12]}..{candidate_commit[:12]}-{timestamp}"
    output = output.resolve()
    log_dir = output / "logs"
    log_dir.mkdir(parents=True, exist_ok=True)

    report: dict[str, Any] = {
        "schema_version": SCHEMA_VERSION,
        "status": "running",
        "invocation": {
            "base_ref": args.reference, "base_commit": base_commit,
            "candidate_ref": args.candidate, "candidate_commit": candidate_commit,
            "seed": args.seed, "runs": args.runs, "frames": args.frames,
            "threshold_pct": args.threshold_pct, "bootstrap_samples": args.bootstrap_samples,
            "workloads": list(workloads), "cmake_args": list(args.cmake_arg),
        },
        "machine": machine_fingerprint(repo),
        "gitlinks": {
            "base_data": _gitlink(repo, base_commit, "data"),
            "candidate_data": _gitlink(repo, candidate_commit, "data"),
        },
        "runs": [],
        "summaries": [],
    }

    report_json = output / "report.json"
    report_md = output / "report.md"
    cmake_args = [*shlex.split(os.environ.get("DVZ_CMAKE_ARGS", "")), *args.cmake_arg]
    try:
        with WorktreePair(repo, args.reference, args.candidate, keep=args.keep_worktrees) as revisions:
            base, candidate = revisions
            build_revision(base, jobs=args.jobs, cmake_args=cmake_args, log_dir=log_dir)
            build_revision(candidate, jobs=args.jobs, cmake_args=cmake_args, log_dir=log_dir)

            for workload_index, workload in enumerate(workloads):
                print(f"preflight: {workload}")
                execute_benchmark(base, workload, args.frames, pair=-1, order=0, log_dir=log_dir, label="base-preflight")
                execute_benchmark(candidate, workload, args.frames, pair=-1, order=1, log_dir=log_dir, label="candidate-preflight")

                rng = random.Random(args.seed + workload_index)
                pairs: list[dict[str, RunResult]] = []
                for pair_index in range(args.runs):
                    order = [base, candidate]
                    if rng.randrange(2):
                        order.reverse()
                    pair_results: dict[str, RunResult] = {}
                    print(f"measure: {workload} pair {pair_index + 1}/{args.runs} order={order[0].name},{order[1].name}")
                    for order_index, revision in enumerate(order):
                        result = execute_benchmark(
                            revision, workload, args.frames, pair=pair_index, order=order_index,
                            log_dir=log_dir, label=revision.name,
                        )
                        pair_results[revision.name] = result
                        report["runs"].append(asdict(result))
                    pairs.append(pair_results)

                summary = summarize_pairs(
                    workload,
                    [pair["base"].metrics.ms_per_frame for pair in pairs],
                    [pair["candidate"].metrics.ms_per_frame for pair in pairs],
                    threshold_pct=args.threshold_pct,
                    seed=args.seed + 1000 + workload_index,
                    bootstrap_samples=args.bootstrap_samples,
                )
                report["summaries"].append(asdict(summary))
    except Exception as exc:
        report["status"] = "failed"
        report["error"] = str(exc)
        report_json.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf8")
        raise

    report["status"] = "complete"
    report_json.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf8")
    report_md.write_text(_report_markdown(report), encoding="utf8")
    print(_report_markdown(report))
    print(f"report: {report_md}")
    has_regression = any(summary["verdict"] == "regression" for summary in report["summaries"])
    return 1 if args.fail_on_regression and has_regression else 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except CompareError as exc:
        print(f"error: {exc}", file=sys.stderr)
        raise SystemExit(2) from exc
