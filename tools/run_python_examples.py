#!/usr/bin/env python3
"""Run manifest-declared Python gallery examples as isolated offscreen smokes."""

from __future__ import annotations

import argparse
from collections import Counter
from dataclasses import asdict, dataclass
import json
import os
from pathlib import Path
import py_compile
import re
import subprocess
import sys
import time

import yaml


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_MANIFEST = ROOT / "examples/c/MANIFEST.yaml"
DEFAULT_OUTPUT_DIR = ROOT / "build/python-example-check"


@dataclass(frozen=True)
class Example:
    id: str
    title: str
    stage: str
    source: str | None


@dataclass
class Result:
    id: str
    title: str
    source: str | None
    status: str
    seconds: float
    returncode: int | None = None
    detail: str = ""
    log: str | None = None
    capture: str | None = None


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("filter", nargs="?", default="", help="optional ID/title/source pattern")
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument("--batch", default="", help="review batch name or number alias")
    parser.add_argument(
        "--review-order", action="store_true", help="use all review batches in declared order"
    )
    parser.add_argument("--start-at", default="", metavar="EXAMPLE_ID")
    parser.add_argument("--code-only", action="store_true", help="compile without executing")
    parser.add_argument(
        "--capture-temp",
        action="store_true",
        help="save build-local PNG captures and require nonblank pixels",
    )
    parser.add_argument("--timeout", type=float, default=90.0, help="seconds per example")
    parser.add_argument("--output-dir", type=Path, default=DEFAULT_OUTPUT_DIR)
    parser.add_argument("--list", action="store_true")
    args = parser.parse_args()
    if args.batch and args.review_order:
        parser.error("--batch cannot be combined with --review-order")
    if args.start_at and not (args.batch or args.review_order):
        parser.error("--start-at requires --batch or --review-order")
    if args.timeout <= 0:
        parser.error("--timeout must be positive")
    return args


def load_manifest(path: Path) -> dict:
    return yaml.safe_load(path.read_text(encoding="utf8")) or {}


def entries_by_id(manifest: dict) -> dict[str, dict]:
    return {str(entry["id"]): entry for entry in manifest.get("examples", [])}


def resolve_batch(manifest: dict, value: str) -> str:
    batches = manifest.get("batches") or {}
    if value in batches:
        return value
    if value.isdigit():
        prefix = f"review-{int(value):02d}-"
        matches = [str(name) for name in batches if str(name).startswith(prefix)]
        if len(matches) == 1:
            return matches[0]
    raise ValueError(f"unknown or ambiguous review batch: {value}")


def selected_ids(manifest: dict, args: argparse.Namespace) -> list[str]:
    if args.batch:
        name = resolve_batch(manifest, args.batch)
        args.resolved_batch = name
        ids = [str(value) for value in manifest["batches"][name]]
    elif args.review_order:
        ids = [
            str(value)
            for batch_ids in (manifest.get("batches") or {}).values()
            for value in batch_ids
        ]
    else:
        ids = [
            str(entry["id"])
            for entry in manifest.get("examples", [])
            if entry.get("stage") == "v0.4_required" and (entry.get("python") or {}).get("source")
        ]
    if args.start_at:
        if args.start_at not in ids:
            raise ValueError(f"start example ID is not in the selected entries: {args.start_at}")
        ids = ids[ids.index(args.start_at) :]
    return list(dict.fromkeys(ids))


def examples(manifest: dict, args: argparse.Namespace) -> list[Example]:
    by_id = entries_by_id(manifest)
    selected = selected_ids(manifest, args)
    unknown = [id_ for id_ in selected if id_ not in by_id]
    if unknown:
        raise ValueError(f"review selection contains unknown IDs: {', '.join(unknown)}")
    result = []
    for id_ in selected:
        entry = by_id[id_]
        source = (entry.get("python") or {}).get("source")
        example = Example(
            id=id_,
            title=str(entry.get("title", id_)),
            stage=str(entry.get("stage", "")),
            source=str(source) if source else None,
        )
        text = " ".join((example.id, example.title, example.source or ""))
        if args.filter and not matches(text, args.filter):
            continue
        result.append(example)
    return result


def matches(text: str, pattern: str) -> bool:
    if pattern in text:
        return True
    try:
        return re.search(pattern, text) is not None
    except re.error:
        return False


def module_name(source: str) -> str:
    path = Path(source)
    if path.suffix != ".py":
        raise ValueError(f"Python source is not a .py file: {source}")
    return ".".join(path.with_suffix("").parts)


def run_one(example: Example, args: argparse.Namespace, logs: Path, captures: Path) -> Result:
    if example.source is None:
        return Result(example.id, example.title, None, "skip", 0.0, detail="no-python-source")
    source_path = ROOT / example.source
    log_path = logs / f"{example.id}.log"
    started = time.monotonic()
    if not source_path.is_file():
        return Result(
            example.id,
            example.title,
            example.source,
            "fail",
            0.0,
            detail="missing-source",
        )
    if args.code_only:
        try:
            py_compile.compile(str(source_path), doraise=True)
            return Result(example.id, example.title, example.source, "pass", 0.0)
        except py_compile.PyCompileError as exc:
            log_path.write_text(str(exc) + "\n", encoding="utf8")
            return Result(
                example.id,
                example.title,
                example.source,
                "fail",
                time.monotonic() - started,
                detail="compile-error",
                log=str(log_path.relative_to(ROOT)),
            )

    env = os.environ.copy()
    env["DVZ_PYTHON_GALLERY_SMOKE"] = "1"
    env.setdefault("DVZ_PYTHON_GALLERY_SMOKE_FRAMES", "1")
    capture_path = captures / f"{example.id}.png" if args.capture_temp else None
    if capture_path is not None:
        env["DVZ_PYTHON_GALLERY_CAPTURE"] = str(capture_path)
    command = [sys.executable, "-m", module_name(example.source)]
    try:
        completed = subprocess.run(
            command,
            cwd=ROOT,
            env=env,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            timeout=args.timeout,
            check=False,
        )
    except subprocess.TimeoutExpired as exc:
        output = exc.stdout or ""
        if isinstance(output, bytes):
            output = output.decode(errors="replace")
        log_path.write_text(output, encoding="utf8")
        return Result(
            example.id,
            example.title,
            example.source,
            "timeout",
            time.monotonic() - started,
            detail=f"exceeded {args.timeout:g}s",
            log=str(log_path.relative_to(ROOT)),
        )
    log_path.write_text(completed.stdout, encoding="utf8")
    status = "pass" if completed.returncode == 0 else "fail"
    detail = "" if status == "pass" else f"exit-{completed.returncode}"
    if status == "pass" and capture_path is not None and not capture_path.is_file():
        status = "fail"
        detail = "missing-capture"
    return Result(
        example.id,
        example.title,
        example.source,
        status,
        time.monotonic() - started,
        returncode=completed.returncode,
        detail=detail,
        log=str(log_path.relative_to(ROOT)),
        capture=str(capture_path.relative_to(ROOT)) if capture_path and capture_path.is_file() else None,
    )


def write_reports(output_dir: Path, results: list[Result], elapsed: float, args: argparse.Namespace) -> None:
    counts = Counter(result.status for result in results)
    payload = {
        "mode": "code-only" if args.code_only else "render",
        "capture_temp": bool(args.capture_temp),
        "elapsed_seconds": elapsed,
        "counts": dict(sorted(counts.items())),
        "results": [asdict(result) for result in results],
    }
    (output_dir / "results.json").write_text(json.dumps(payload, indent=2) + "\n", encoding="utf8")
    lines = [
        "# Python example check",
        "",
        f"- Mode: `{payload['mode']}`",
        f"- Elapsed: `{elapsed:.1f}s`",
        f"- Counts: `{', '.join(f'{key}={value}' for key, value in sorted(counts.items()))}`",
        "",
        "| Status | ID | Seconds | Detail | Log |",
        "| --- | --- | ---: | --- | --- |",
    ]
    for result in results:
        log = f"[{result.log}]({result.log})" if result.log else ""
        lines.append(
            f"| {result.status.upper()} | `{result.id}` | {result.seconds:.2f} | "
            f"{result.detail} | {log} |"
        )
    (output_dir / "summary.md").write_text("\n".join(lines) + "\n", encoding="utf8")


def main() -> int:
    args = parse_args()
    try:
        manifest = load_manifest(args.manifest)
        selected = examples(manifest, args)
    except (OSError, ValueError, yaml.YAMLError) as exc:
        print(str(exc), file=sys.stderr)
        return 2
    if not selected:
        print("No matching Python examples.", file=sys.stderr)
        return 2
    print(f"Python examples selected: {len(selected)}")
    for index, example in enumerate(selected, 1):
        source = example.source or "SKIP no-python-source"
        print(f"  {index:3d}. {example.id:42s} {source}")
    if args.list:
        return 0

    output_dir = args.output_dir.resolve()
    logs = output_dir / "logs"
    captures = output_dir / "captures"
    logs.mkdir(parents=True, exist_ok=True)
    if args.capture_temp:
        captures.mkdir(parents=True, exist_ok=True)
    started = time.monotonic()
    results = []
    for index, example in enumerate(selected, 1):
        result = run_one(example, args, logs, captures)
        results.append(result)
        print(
            f"[{index:3d}/{len(selected):3d}] {result.status.upper():7s} "
            f"{example.id:42s} {result.seconds:7.2f}s {result.detail}"
        )
    elapsed = time.monotonic() - started
    write_reports(output_dir, results, elapsed, args)
    counts = Counter(result.status for result in results)
    print("summary: " + ", ".join(f"{key}={value}" for key, value in sorted(counts.items())))
    print(f"reports: {output_dir.relative_to(ROOT)}")
    return 1 if counts["fail"] or counts["timeout"] else 0


if __name__ == "__main__":
    raise SystemExit(main())
