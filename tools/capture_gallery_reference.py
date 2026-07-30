#!/usr/bin/env python3
"""Generate review-only Linux reference screenshot candidates with provenance."""

from __future__ import annotations

import argparse
import hashlib
import html
import json
import os
import platform
import shutil
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path

from PIL import Image, ImageChops

import capture_gallery
import gallery_media


ROOT = gallery_media.ROOT
DEFAULT_OUTPUT_DIR = ROOT / "build/gallery-reference"
REFERENCE_RUN_NAMES = ("candidates", "repeat")
VULKAN_ENV_KEYS = (
    "VK_DRIVER_FILES",
    "VK_ICD_FILENAMES",
    "VK_LAYER_PATH",
    "VK_LOADER_DRIVERS_SELECT",
    "VK_LOADER_DRIVERS_DISABLE",
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, default=gallery_media.DEFAULT_MANIFEST)
    parser.add_argument("--build-dir", type=Path, default=capture_gallery.DEFAULT_BUILD_DIR)
    parser.add_argument("--output-dir", type=Path, default=DEFAULT_OUTPUT_DIR)
    parser.add_argument("--jobs", default="1", help="capture workers; reference default is serial")
    parser.add_argument("--host-id", default=platform.node(), help="stable reference-host label")
    parser.add_argument(
        "--report-only",
        action="store_true",
        help="rebuild reports from existing candidate and repeat captures",
    )
    parser.add_argument(
        "--allow-dirty",
        action="store_true",
        help="record candidates from a dirty source tree instead of refusing",
    )
    return parser.parse_args()


def file_sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as file:
        for chunk in iter(lambda: file.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def text_sha256(value: str) -> str:
    return hashlib.sha256(value.encode("utf8")).hexdigest()


def ensure_build_output(path: Path) -> Path:
    """Resolve an output path and require it to be a proper child of build/."""
    resolved = path.resolve()
    build = (ROOT / "build").resolve()
    if resolved == build or not resolved.is_relative_to(build):
        raise ValueError(f"reference output must be a proper child of {build}: {resolved}")
    return resolved


def run_text(command: list[str], cwd: Path = ROOT) -> str:
    result = subprocess.run(
        command,
        cwd=cwd,
        check=True,
        capture_output=True,
        text=True,
    )
    return result.stdout.strip()


def git_value(*args: str, cwd: Path = ROOT) -> str:
    return run_text(["git", *args], cwd=cwd)


def os_release() -> dict[str, str]:
    path = Path("/etc/os-release")
    if not path.is_file():
        return {}
    values: dict[str, str] = {}
    for line in path.read_text(encoding="utf8").splitlines():
        if "=" not in line:
            continue
        key, value = line.split("=", maxsplit=1)
        values[key] = value.strip().strip('"')
    return values


def parse_vulkan_devices(summary: str) -> list[dict[str, str]]:
    """Parse the stable key/value device section from vulkaninfo --summary."""
    devices: list[dict[str, str]] = []
    current: dict[str, str] | None = None
    in_devices = False
    for raw_line in summary.splitlines():
        line = raw_line.strip()
        if line == "Devices:":
            in_devices = True
            continue
        if not in_devices:
            continue
        if line.startswith("GPU") and line.endswith(":") and line[3:-1].isdigit():
            current = {"index": line[3:-1]}
            devices.append(current)
            continue
        if current is not None and "=" in line:
            key, value = line.split("=", maxsplit=1)
            current[key.strip()] = value.strip()
    return devices


def collect_provenance(host_id: str, commands: list[list[str]]) -> dict:
    if platform.system() != "Linux":
        raise RuntimeError("canonical reference candidates must be generated on Linux")
    if not host_id.strip():
        raise ValueError("--host-id must not be empty")

    vulkan_summary = run_text(["vulkaninfo", "--summary"])
    devices = parse_vulkan_devices(vulkan_summary)
    if not devices:
        raise RuntimeError("vulkaninfo --summary did not report a Vulkan device")

    source_status = git_value("status", "--porcelain")
    data_status = git_value("status", "--porcelain", cwd=ROOT / "data")
    system = {
        "host_id": host_id,
        "hostname": platform.node(),
        "machine": platform.machine(),
        "kernel_release": platform.release(),
        "kernel_version": platform.version(),
        "os_release": os_release(),
    }
    selected_gpu = devices[0]
    fingerprint_payload = {
        "system": system,
        "selected_gpu": selected_gpu,
        "vulkan_summary_sha256": text_sha256(vulkan_summary),
    }
    fingerprint = text_sha256(
        json.dumps(fingerprint_payload, sort_keys=True, separators=(",", ":"))
    )
    return {
        "schema_version": 1,
        "policy": "linux-reference-host",
        "generated_at": datetime.now(timezone.utc).isoformat(),
        "reference_fingerprint": fingerprint,
        "system": system,
        "vulkan": {
            "selected_gpu_index": 0,
            "selected_gpu": selected_gpu,
            "devices": devices,
            "summary": vulkan_summary,
            "summary_sha256": text_sha256(vulkan_summary),
            "environment": {
                key: os.environ[key] for key in VULKAN_ENV_KEYS if key in os.environ
            },
        },
        "source": {
            "commit": git_value("rev-parse", "HEAD"),
            "status": source_status,
            "clean": not source_status,
            "data_commit": git_value("rev-parse", "HEAD", cwd=ROOT / "data"),
            "data_status": data_status,
            "data_clean": not data_status,
            "capture_tool_sha256": file_sha256(Path(__file__)),
            "capture_engine_sha256": file_sha256(ROOT / "tools/capture_gallery.py"),
            "gallery_media_sha256": file_sha256(ROOT / "tools/gallery_media.py"),
        },
        "capture": {
            "commands": commands,
            "runs": list(REFERENCE_RUN_NAMES),
            "jobs": commands[0][commands[0].index("--jobs") + 1],
        },
    }


def reference_examples(manifest_path: Path) -> list[capture_gallery.CaptureExample]:
    manifest = capture_gallery.load_manifest(manifest_path)
    reviewed = capture_gallery.reviewed_example_ids(manifest)
    examples = [
        example
        for example in capture_gallery.collect_examples(manifest)
        if example.id in reviewed and "screenshot" in example.validation
    ]
    return sorted(examples, key=lambda item: (item.lane, item.id))


def capture_command(
    manifest: Path,
    build_dir: Path,
    image_dir: Path,
    jobs: str,
) -> list[str]:
    return [
        sys.executable,
        str(ROOT / "tools/capture_gallery.py"),
        "--manifest",
        str(manifest),
        "--build-dir",
        str(build_dir),
        "--image-dir",
        str(image_dir),
        "--all-screenshot",
        "--force",
        "--jobs",
        jobs,
    ]


def reset_run_dir(path: Path) -> None:
    if path.exists():
        shutil.rmtree(path)
    path.mkdir(parents=True)


def run_captures(
    manifest: Path,
    build_dir: Path,
    output_dir: Path,
    jobs: str,
) -> list[list[str]]:
    commands: list[list[str]] = []
    for name in REFERENCE_RUN_NAMES:
        image_dir = output_dir / name
        reset_run_dir(image_dir)
        command = capture_command(manifest, build_dir, image_dir, jobs)
        commands.append(command)
        result = subprocess.run(command, cwd=ROOT, check=False)
        if result.returncode:
            raise RuntimeError(f"{name} capture failed with exit code {result.returncode}")
    return commands


def pixel_metrics(first: Path, second: Path) -> tuple[int, float]:
    return capture_gallery.png_pixel_difference(first, second)


def classification(max_delta: int, changed_fraction: float) -> str:
    if max_delta == 0 and changed_fraction == 0:
        return "identical"
    if (
        max_delta <= capture_gallery.VERIFY_MAX_CHANNEL_DELTA
        and changed_fraction <= capture_gallery.VERIFY_MAX_CHANGED_COMPONENT_FRACTION
    ):
        return "pixel-equivalent"
    return "different"


def write_enhanced_diff(first: Path, second: Path, target: Path) -> None:
    target.parent.mkdir(parents=True, exist_ok=True)
    with Image.open(first) as first_image, Image.open(second) as second_image:
        first_rgba = first_image.convert("RGBA")
        second_rgba = second_image.convert("RGBA")
        if first_rgba.size != second_rgba.size:
            raise ValueError(f"capture sizes differ: {first_rgba.size} != {second_rgba.size}")
        difference = ImageChops.difference(first_rgba, second_rgba).convert("RGB")
        enhanced = difference.point(lambda value: min(255, value * 8))
        enhanced.save(target, format="PNG")


def image_entry(
    example: capture_gallery.CaptureExample,
    output_dir: Path,
) -> dict:
    canonical = capture_gallery.output_path(example, capture_gallery.DEFAULT_IMAGE_DIR)
    candidate = capture_gallery.output_path(example, output_dir / REFERENCE_RUN_NAMES[0])
    repeat = capture_gallery.output_path(example, output_dir / REFERENCE_RUN_NAMES[1])
    for label, path in (
        ("canonical", canonical),
        ("candidate", candidate),
        ("repeat", repeat),
    ):
        ok, detail = capture_gallery.png_is_nonblank(
            path, (example.expected_width, example.expected_height)
        )
        if not ok:
            raise RuntimeError(f"{example.id}: invalid {label} PNG: {detail}")

    canonical_hash = file_sha256(canonical)
    candidate_hash = file_sha256(candidate)
    repeat_hash = file_sha256(repeat)
    canonical_delta, canonical_fraction = pixel_metrics(candidate, canonical)
    repeat_delta, repeat_fraction = pixel_metrics(candidate, repeat)
    canonical_class = classification(canonical_delta, canonical_fraction)
    repeat_class = classification(repeat_delta, repeat_fraction)
    diff = output_dir / "diffs" / example.lane / f"{example.id}.png"
    if canonical_class != "identical":
        write_enhanced_diff(canonical, candidate, diff)
    else:
        diff.unlink(missing_ok=True)
    return {
        "id": example.id,
        "title": example.title,
        "lane": example.lane,
        "source": example.source,
        "size": f"{example.expected_width}x{example.expected_height}",
        "canonical": {
            "path": canonical.relative_to(ROOT).as_posix(),
            "sha256": canonical_hash,
        },
        "candidate": {
            "path": candidate.relative_to(ROOT).as_posix(),
            "sha256": candidate_hash,
        },
        "repeat": {
            "path": repeat.relative_to(ROOT).as_posix(),
            "sha256": repeat_hash,
            "classification": repeat_class,
            "max_delta": repeat_delta,
            "changed_component_fraction": repeat_fraction,
        },
        "comparison": {
            "classification": canonical_class,
            "max_delta": canonical_delta,
            "changed_component_fraction": canonical_fraction,
            "enhanced_diff": (
                diff.relative_to(ROOT).as_posix() if diff.is_file() else None
            ),
        },
    }


def summarize(entries: list[dict]) -> dict:
    canonical_counts = {
        status: sum(
            entry["comparison"]["classification"] == status for entry in entries
        )
        for status in ("identical", "pixel-equivalent", "different")
    }
    repeat_counts = {
        status: sum(entry["repeat"]["classification"] == status for entry in entries)
        for status in ("identical", "pixel-equivalent", "different")
    }
    return {
        "total": len(entries),
        "canonical_comparison": canonical_counts,
        "repeatability": repeat_counts,
        "all_repeat_identical": repeat_counts["identical"] == len(entries),
    }


def relative_from_report(path: str, output_dir: Path) -> str:
    return os.path.relpath(ROOT / path, output_dir).replace(os.sep, "/")


def stage_committed_review_images(report: dict, output_dir: Path) -> dict[str, str]:
    review_dir = output_dir / "committed"
    if review_dir.exists():
        shutil.rmtree(review_dir)
    staged: dict[str, str] = {}
    for entry in report["entries"]:
        source = ROOT / entry["canonical"]["path"]
        target = review_dir / entry["lane"] / f"{entry['id']}.png"
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source, target)
        staged[entry["id"]] = target.relative_to(output_dir).as_posix()
    return staged


def write_html(report: dict, output_dir: Path) -> None:
    summary = report["summary"]
    provenance = report["provenance"]
    selected_gpu = provenance["vulkan"]["selected_gpu"]
    committed_review_images = stage_committed_review_images(report, output_dir)
    cards: list[str] = []
    order = {"different": 0, "pixel-equivalent": 1, "identical": 2}
    for entry in sorted(
        report["entries"],
        key=lambda item: (
            order[item["comparison"]["classification"]],
            item["lane"],
            item["id"],
        ),
    ):
        comparison = entry["comparison"]
        canonical = committed_review_images[entry["id"]]
        candidate = relative_from_report(entry["candidate"]["path"], output_dir)
        diff_path = comparison["enhanced_diff"]
        diff = (
            f'<img loading="lazy" src="{html.escape(relative_from_report(diff_path, output_dir))}" alt="Enhanced difference">'
            if diff_path
            else '<div class="identical">Byte-identical</div>'
        )
        cards.append(
            f"""
<article class="card {html.escape(comparison["classification"])}">
  <h2>{html.escape(entry["id"])}</h2>
  <p>{html.escape(entry["lane"])} · committed comparison: {html.escape(comparison["classification"])} · repeat: {html.escape(entry["repeat"]["classification"])}</p>
  <p>max delta {comparison["max_delta"]}; changed RGBA components {comparison["changed_component_fraction"]:.4%}</p>
  <div class="images">
    <figure><figcaption>Committed</figcaption><img loading="lazy" src="{html.escape(canonical)}" alt="Committed screenshot"></figure>
    <figure><figcaption>Linux candidate</figcaption><img loading="lazy" src="{html.escape(candidate)}" alt="Linux candidate"></figure>
    <figure><figcaption>Difference ×8</figcaption>{diff}</figure>
  </div>
</article>"""
        )
    document = f"""<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Datoviz Linux reference screenshot review</title>
<style>
body {{ margin: 0; padding: 2rem; color: #e8edf2; background: #111820; font: 15px/1.45 system-ui, sans-serif; }}
header {{ max-width: 90rem; margin: 0 auto 2rem; }}
code {{ color: #7dd3fc; }}
.card {{ max-width: 90rem; margin: 0 auto 2rem; padding: 1rem; border: 1px solid #334155; border-left: 5px solid #ef4444; border-radius: .5rem; background: #18222d; }}
.card.pixel-equivalent {{ border-left-color: #f59e0b; }}
.card.identical {{ border-left-color: #22c55e; }}
.card h2 {{ margin: 0; font-size: 1rem; }}
.card p {{ margin: .35rem 0; color: #bac7d5; }}
.images {{ display: grid; grid-template-columns: repeat(3, minmax(0, 1fr)); gap: .75rem; }}
figure {{ margin: 0; }}
figcaption {{ margin-bottom: .35rem; color: #94a3b8; }}
img, .identical {{ display: block; width: 100%; aspect-ratio: 16 / 9; object-fit: contain; background: #0b1118; }}
.identical {{ display: grid; place-items: center; color: #86efac; }}
@media (max-width: 900px) {{ .images {{ grid-template-columns: 1fr; }} }}
</style>
</head>
<body>
<header>
  <h1>Linux reference screenshot candidates</h1>
  <p>Total {summary["total"]}; committed comparison: {summary["canonical_comparison"]}; repeatability: {summary["repeatability"]}.</p>
  <p>Host <code>{html.escape(provenance["system"]["host_id"])}</code>; selected GPU <code>{html.escape(selected_gpu.get("deviceName", "unknown"))}</code>; driver <code>{html.escape(selected_gpu.get("driverInfo", selected_gpu.get("driverVersion", "unknown")))}</code>; source <code>{html.escape(provenance["source"]["commit"])}</code>.</p>
  <p>Red means materially different, amber means tightly pixel-equivalent, and green means byte-identical. Difference images amplify channel deltas eightfold.</p>
</header>
{"".join(cards)}
</body>
</html>
"""
    (output_dir / "index.html").write_text(document, encoding="utf8")


def build_report(
    manifest: Path,
    output_dir: Path,
    provenance: dict,
) -> dict:
    examples = reference_examples(manifest)
    differences = output_dir / "diffs"
    if differences.exists():
        shutil.rmtree(differences)
    entries = [image_entry(example, output_dir) for example in examples]
    report = {
        "schema_version": 1,
        "provenance": provenance,
        "summary": summarize(entries),
        "entries": entries,
    }
    (output_dir / "report.json").write_text(
        json.dumps(report, indent=2, sort_keys=True) + "\n",
        encoding="utf8",
    )
    write_html(report, output_dir)
    return report


def main() -> int:
    args = parse_args()
    try:
        output_dir = ensure_build_output(args.output_dir)
        if platform.system() != "Linux":
            raise RuntimeError("canonical reference candidates must be generated on Linux")
        source_status = git_value("status", "--porcelain")
        data_status = git_value("status", "--porcelain", cwd=ROOT / "data")
        if not args.allow_dirty and (source_status or data_status):
            raise RuntimeError(
                "reference capture requires clean source and data worktrees; "
                "use --allow-dirty only for diagnostics"
            )

        output_dir.mkdir(parents=True, exist_ok=True)
        if args.report_only:
            provenance_path = output_dir / "provenance.json"
            if not provenance_path.is_file():
                raise RuntimeError(f"missing capture provenance: {provenance_path}")
            provenance = json.loads(provenance_path.read_text(encoding="utf8"))
        else:
            commands = [
                capture_command(
                    args.manifest,
                    args.build_dir,
                    output_dir / name,
                    args.jobs,
                )
                for name in REFERENCE_RUN_NAMES
            ]
            provenance = collect_provenance(args.host_id, commands)
            run_captures(args.manifest, args.build_dir, output_dir, args.jobs)
        (output_dir / "provenance.json").write_text(
            json.dumps(provenance, indent=2, sort_keys=True) + "\n",
            encoding="utf8",
        )
        report = build_report(args.manifest, output_dir, provenance)
    except (OSError, RuntimeError, ValueError, subprocess.CalledProcessError) as exc:
        print(f"gallery reference capture: {exc}", file=sys.stderr)
        return 1

    summary = report["summary"]
    print(
        "gallery reference capture: "
        f"total={summary['total']} "
        f"committed={summary['canonical_comparison']} "
        f"repeat={summary['repeatability']} "
        f"report={output_dir / 'report.json'} "
        f"html={output_dir / 'index.html'}"
    )
    return 0 if summary["all_repeat_identical"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
