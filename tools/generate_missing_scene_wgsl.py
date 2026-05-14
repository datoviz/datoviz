#!/usr/bin/env python3
"""Generate missing scene WGSL shader siblings from GLSL sources."""

from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
import tempfile
from dataclasses import dataclass
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_GLSL_DIR = REPO_ROOT / "src" / "scene" / "glsl"
DEFAULT_WGSL_DIR = REPO_ROOT / "src" / "scene" / "wgsl"


@dataclass(frozen=True)
class ShaderJob:
    source: Path
    output: Path
    stage: str


@dataclass(frozen=True)
class Tools:
    glslc: Path | None
    glslang_validator: Path | None
    naga: Path | None


def _discover_tool(cli_path: str | None, env_name: str, executable: str) -> Path | None:
    if cli_path:
        path = Path(cli_path)
        return path if path.exists() else None

    env_path = os.environ.get(env_name)
    if env_path:
        path = Path(env_path)
        return path if path.exists() else None

    found = shutil.which(executable)
    return Path(found) if found else None


def _parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate missing src/scene/wgsl/*.wgsl shaders from src/scene/glsl/*.vert/*.frag."
    )
    parser.add_argument("--glsl-dir", type=Path, default=DEFAULT_GLSL_DIR)
    parser.add_argument("--wgsl-dir", type=Path, default=DEFAULT_WGSL_DIR)
    parser.add_argument("--glslc", help="Path to glslc.")
    parser.add_argument("--glslang-validator", help="Path to glslangValidator.")
    parser.add_argument("--naga", help="Path to naga.")
    parser.add_argument("--tint", help="Reserved for a future Tint backend.")
    parser.add_argument(
        "--force",
        "--update-existing",
        action="store_true",
        dest="force",
        help="Overwrite existing WGSL files.",
    )
    parser.add_argument("--check", action="store_true", help="Fail if any WGSL sibling is missing.")
    parser.add_argument("--dry-run", action="store_true", help="Print planned actions without writing.")
    return parser.parse_args()


def _stage_for_source(path: Path) -> str:
    if path.suffix == ".vert":
        return "vert"
    if path.suffix == ".frag":
        return "frag"
    raise ValueError(f"unsupported shader suffix: {path}")


def _scan_jobs(glsl_dir: Path, wgsl_dir: Path) -> list[ShaderJob]:
    sources = sorted([*glsl_dir.glob("*.vert"), *glsl_dir.glob("*.frag")])
    jobs: list[ShaderJob] = []
    for source in sources:
        output = wgsl_dir / f"{source.name}.wgsl"
        jobs.append(ShaderJob(source=source, output=output, stage=_stage_for_source(source)))
    return jobs


def _run(command: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(command, check=False, capture_output=True, text=True)


def _normalize_generated_text(path: Path) -> None:
    text = path.read_text(encoding="utf8")
    lines = [line.rstrip() for line in text.splitlines()]
    path.write_text("\n".join(lines) + "\n", encoding="utf8")


def _compile_glsl(job: ShaderJob, tools: Tools, spv_path: Path, include_dir: Path) -> str | None:
    if tools.glslc is not None:
        command = [
            str(tools.glslc),
            "-I",
            str(include_dir),
            "-fshader-stage=" + job.stage,
            "-o",
            str(spv_path),
            str(job.source),
        ]
        result = _run(command)
        if result.returncode == 0:
            return None
        if tools.glslang_validator is None:
            return result.stderr.strip() or result.stdout.strip() or "glslc failed"

    if tools.glslang_validator is not None:
        command = [
            str(tools.glslang_validator),
            "-V",
            "-I" + str(include_dir),
            "-S",
            job.stage,
            "-o",
            str(spv_path),
            str(job.source),
        ]
        result = _run(command)
        if result.returncode == 0:
            return None
        return result.stderr.strip() or result.stdout.strip() or "glslangValidator failed"

    return "missing glslc or glslangValidator"


def _generate_job(job: ShaderJob, tools: Tools, include_dir: Path) -> str | None:
    if tools.naga is None:
        return "missing naga"

    job.output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="datoviz-wgsl-") as tmp:
        tmp_dir = Path(tmp)
        spv_path = tmp_dir / f"{job.source.name}.spv"
        wgsl_path = tmp_dir / f"{job.output.stem}.tmp.wgsl"

        error = _compile_glsl(job, tools, spv_path, include_dir)
        if error is not None:
            return error

        result = _run([str(tools.naga), "--input-kind", "spv", str(spv_path), str(wgsl_path)])
        if result.returncode != 0:
            return result.stderr.strip() or result.stdout.strip() or "naga failed"

        _normalize_generated_text(wgsl_path)
        os.replace(wgsl_path, job.output)
    return None


def main() -> int:
    args = _parse_args()
    if args.tint:
        print("warning: --tint is reserved for future use; naga is used for this tool", file=sys.stderr)

    glsl_dir = args.glsl_dir.resolve()
    wgsl_dir = args.wgsl_dir.resolve()
    jobs = _scan_jobs(glsl_dir, wgsl_dir)
    tools = Tools(
        glslc=_discover_tool(args.glslc, "GLSLC", "glslc"),
        glslang_validator=_discover_tool(
            args.glslang_validator, "GLSLANG_VALIDATOR", "glslangValidator"
        ),
        naga=_discover_tool(args.naga, "NAGA", "naga"),
    )

    missing = [job for job in jobs if not job.output.exists()]
    if args.check:
        for job in jobs:
            status = "present" if job.output.exists() else "missing"
            print(f"{status}: {job.output.relative_to(REPO_ROOT)}")
        return 1 if missing else 0

    generated: list[Path] = []
    skipped: list[Path] = []
    failed: list[tuple[Path, str]] = []

    for job in jobs:
        if job.output.exists() and not args.force:
            skipped.append(job.output)
            continue
        if args.dry_run:
            action = "update" if job.output.exists() else "generate"
            print(f"{action}: {job.output.relative_to(REPO_ROOT)}")
            continue

        error = _generate_job(job, tools, glsl_dir)
        if error is None:
            generated.append(job.output)
        else:
            failed.append((job.output, error))

    for path in generated:
        print(f"generated: {path.relative_to(REPO_ROOT)}")
    for path in skipped:
        print(f"skipped: {path.relative_to(REPO_ROOT)}")
    for path, error in failed:
        print(f"failed: {path.relative_to(REPO_ROOT)}: {error}", file=sys.stderr)

    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
