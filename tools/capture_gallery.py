#!/usr/bin/env python3
"""Capture v0.4 C gallery screenshots into the MkDocs media tree."""

from __future__ import annotations

import argparse
import os
import platform
import subprocess
import sys
import zlib
from dataclasses import dataclass
from pathlib import Path
from struct import unpack

import yaml


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_MANIFEST = ROOT / "examples/c/MANIFEST.yaml"
DEFAULT_BUILD_DIR = ROOT / "build"
DEFAULT_IMAGE_DIR = ROOT / "docs/images/gallery"
PUBLIC_LANES = ("visuals", "features", "composites", "showcases")
CATEGORY_TO_LANE = {
    "visual": "visuals",
    "feature": "features",
    "composite": "composites",
    "showcase": "showcases",
}
LANDING_IDS = (
    "point_cloud",
    "protein_arcball_viewer",
    "brain_volume_mesh",
    "showcase_wind_field",
    "us_state_choropleth",
)


@dataclass(frozen=True)
class CaptureExample:
    id: str
    title: str
    lane: str
    source: str
    validation: str
    capture_mode: str
    capture_reason: str
    expected_width: int
    expected_height: int

    @property
    def rel_executable(self) -> str:
        rel = Path(self.source).relative_to("examples/c")
        return rel.with_suffix("").as_posix()


def split_values(values: list[str]) -> set[str]:
    return {
        part.strip()
        for value in values
        for part in value.split(",")
        if part.strip()
    }


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument("--build-dir", type=Path, default=DEFAULT_BUILD_DIR)
    parser.add_argument("--image-dir", type=Path, default=DEFAULT_IMAGE_DIR)
    parser.add_argument("--id", action="append", default=[], help="example id; repeat or comma-separate")
    parser.add_argument(
        "--lane",
        action="append",
        default=[],
        help="gallery lane such as visuals, features, composites, or showcases",
    )
    parser.add_argument(
        "--landing",
        action="store_true",
        help="capture the native C examples used by the landing-page proof cards",
    )
    parser.add_argument(
        "--all-screenshot",
        action="store_true",
        help="capture every public manifest entry declaring screenshot validation",
    )
    parser.add_argument("--list", action="store_true", help="list matching captures without running")
    parser.add_argument("--dry-run", action="store_true", help="print commands without running")
    parser.add_argument(
        "--skip-nonblank-check",
        action="store_true",
        help="do not inspect captured PNG dimensions and pixel variation",
    )
    return parser.parse_args()


def load_manifest(path: Path) -> dict:
    with path.open("r", encoding="utf8") as f:
        manifest = yaml.safe_load(f) or {}
    if not isinstance(manifest.get("examples"), list):
        raise ValueError(f"{path} does not contain an examples list")
    return manifest


def parse_capture_size(value: object, fallback: tuple[int, int]) -> tuple[int, int]:
    if value is None:
        return fallback
    if isinstance(value, str):
        parts = value.lower().split("x")
        if len(parts) == 2 and parts[0].strip().isdigit() and parts[1].strip().isdigit():
            return int(parts[0]), int(parts[1])
    if isinstance(value, dict):
        width = value.get("width")
        height = value.get("height")
        if isinstance(width, int) and isinstance(height, int):
            return width, height
        size = value.get("size")
        if size is not None:
            return parse_capture_size(size, fallback)
        return fallback
    raise ValueError(f"invalid capture size: {value!r}")


def entry_lane(entry: dict) -> str:
    raw_category = entry.get("category")
    if raw_category is not None:
        category = str(raw_category)
        return CATEGORY_TO_LANE.get(category, category)
    return str(entry.get("lane", ""))


def collect_examples(manifest: dict) -> list[CaptureExample]:
    examples: list[CaptureExample] = []
    defaults = manifest.get("defaults") or {}
    default_size = parse_capture_size(defaults.get("public_capture"), (1600, 1200))
    for entry in manifest["examples"]:
        source = str(entry.get("source", ""))
        lane = entry_lane(entry)
        stage = str(entry.get("stage", ""))
        if not source or lane not in PUBLIC_LANES or stage == "lab":
            continue
        capture = entry.get("capture") or {}
        if not isinstance(capture, dict):
            raise ValueError(f"{entry['id']} capture metadata must be a mapping")
        expected_width, expected_height = parse_capture_size(capture, default_size)
        examples.append(
            CaptureExample(
                id=str(entry["id"]),
                title=str(entry.get("title", entry["id"])),
                lane=lane,
                source=source,
                validation=str(entry.get("validation", "")),
                capture_mode=str(capture.get("mode", "scenario")),
                capture_reason=str(capture.get("reason", "")),
                expected_width=expected_width,
                expected_height=expected_height,
            )
        )
    return examples


def matches_filter(example: CaptureExample, ids: set[str], lanes: set[str], args: argparse.Namespace) -> bool:
    selected = False
    if ids:
        selected = example.id in ids
    if lanes:
        selected = (selected or not ids) and example.lane in lanes
    if args.landing:
        selected = selected or example.id in LANDING_IDS
    if args.all_screenshot:
        selected = selected or "screenshot" in example.validation
    return selected


def apply_runtime_env(env: dict[str, str]) -> None:
    if platform.system() != "Darwin":
        return
    vulkan_sdk = env.get("VULKAN_SDK", "")
    candidates = []
    if vulkan_sdk:
        candidates.append(Path(vulkan_sdk) / "lib")
    candidates.append(ROOT / "libs" / "vulkan" / "macos")
    for candidate in candidates:
        if not candidate.is_dir():
            continue
        old = env.get("DYLD_FALLBACK_LIBRARY_PATH")
        env["DYLD_FALLBACK_LIBRARY_PATH"] = str(candidate) if not old else f"{candidate}:{old}"
        icd = candidate / "MoltenVK_icd.json"
        if icd.exists() and "VK_DRIVER_FILES" not in env:
            env["VK_DRIVER_FILES"] = str(icd)
        break


def output_path(example: CaptureExample, image_dir: Path) -> Path:
    return image_dir / example.lane / f"{example.id}.png"


def executable_path(example: CaptureExample, build_dir: Path) -> Path:
    return build_dir / "examples" / "c" / example.rel_executable


def png_is_nonblank(path: Path, expected_size: tuple[int, int]) -> tuple[bool, str]:
    if not path.exists():
        return False, "missing"
    if path.stat().st_size < 1024:
        return False, "too small"

    try:
        width, height, extrema = png_extrema(path)
    except ValueError as exc:
        return False, str(exc)
    if width < 32 or height < 32:
        return False, f"unexpected size {width}x{height}"
    expected_width, expected_height = expected_size
    if width != expected_width or height != expected_height:
        return False, f"expected {expected_width}x{expected_height}, got {width}x{height}"

    color_spread = max(high - low for low, high in extrema["color"])
    alpha = extrema.get("alpha")
    if alpha is not None and alpha[1] == 0:
        return False, "fully transparent"
    if color_spread <= 2 and (alpha is None or alpha[1] - alpha[0] <= 2):
        return False, "flat pixels"
    return True, f"{width}x{height}"


def png_extrema(path: Path) -> tuple[int, int, dict[str, list[tuple[int, int]] | tuple[int, int]]]:
    data = path.read_bytes()
    if not data.startswith(b"\x89PNG\r\n\x1a\n"):
        raise ValueError("not a PNG")

    offset = 8
    width = height = bit_depth = color_type = interlace = None
    idat = bytearray()
    while offset + 8 <= len(data):
        length = unpack(">I", data[offset : offset + 4])[0]
        kind = data[offset + 4 : offset + 8]
        start = offset + 8
        end = start + length
        if end + 4 > len(data):
            raise ValueError("truncated PNG chunk")
        payload = data[start:end]
        offset = end + 4

        if kind == b"IHDR":
            if length != 13:
                raise ValueError("invalid IHDR")
            width, height, bit_depth, color_type, _, _, interlace = unpack(">IIBBBBB", payload)
        elif kind == b"IDAT":
            idat.extend(payload)
        elif kind == b"IEND":
            break

    if width is None or height is None or bit_depth is None or color_type is None or interlace is None:
        raise ValueError("missing PNG IHDR")
    if bit_depth != 8 or interlace != 0:
        raise ValueError("unsupported PNG format for validation")

    channels_by_type = {0: 1, 2: 3, 4: 2, 6: 4}
    channels = channels_by_type.get(color_type)
    if channels is None:
        raise ValueError("unsupported PNG color type for validation")

    raw = zlib.decompress(bytes(idat))
    row_bytes = width * channels
    expected = height * (row_bytes + 1)
    if len(raw) < expected:
        raise ValueError("truncated PNG data")

    previous = bytearray(row_bytes)
    color_count = 1 if color_type in (0, 4) else 3
    color_extrema = [(255, 0) for _ in range(color_count)]
    alpha_extrema = (255, 0) if color_type in (4, 6) else None
    pos = 0
    for _ in range(height):
        filter_type = raw[pos]
        pos += 1
        scanline = bytearray(raw[pos : pos + row_bytes])
        pos += row_bytes
        unfilter_scanline(scanline, previous, channels, filter_type)

        for x in range(width):
            base = x * channels
            for c in range(color_count):
                value = scanline[base + c]
                low, high = color_extrema[c]
                color_extrema[c] = (min(low, value), max(high, value))
            if alpha_extrema is not None:
                alpha_index = 1 if color_type == 4 else 3
                value = scanline[base + alpha_index]
                alpha_extrema = (min(alpha_extrema[0], value), max(alpha_extrema[1], value))

        previous = scanline

    extrema: dict[str, list[tuple[int, int]] | tuple[int, int]] = {"color": color_extrema}
    if alpha_extrema is not None:
        extrema["alpha"] = alpha_extrema
    return width, height, extrema


def unfilter_scanline(
    scanline: bytearray, previous: bytearray, bytes_per_pixel: int, filter_type: int
) -> None:
    if filter_type == 0:
        return
    if filter_type not in (1, 2, 3, 4):
        raise ValueError("unsupported PNG filter")

    for i, value in enumerate(scanline):
        left = scanline[i - bytes_per_pixel] if i >= bytes_per_pixel else 0
        up = previous[i]
        up_left = previous[i - bytes_per_pixel] if i >= bytes_per_pixel else 0
        if filter_type == 1:
            predictor = left
        elif filter_type == 2:
            predictor = up
        elif filter_type == 3:
            predictor = (left + up) // 2
        else:
            predictor = paeth(left, up, up_left)
        scanline[i] = (value + predictor) & 0xFF


def paeth(left: int, up: int, up_left: int) -> int:
    p = left + up - up_left
    pa = abs(p - left)
    pb = abs(p - up)
    pc = abs(p - up_left)
    if pa <= pb and pa <= pc:
        return left
    if pb <= pc:
        return up
    return up_left


def command_for(example: CaptureExample, build_dir: Path) -> list[str]:
    exe = str(executable_path(example, build_dir))
    if example.capture_mode == "scenario":
        return [exe, "--png"]
    return [exe]


def uses_scenario_runner(example: CaptureExample) -> bool:
    try:
        source = (ROOT / example.source).read_text(encoding="utf8", errors="replace")
    except OSError:
        return False
    return "dvz_scenario_run_native_cli" in source


def capture_one(example: CaptureExample, args: argparse.Namespace) -> tuple[bool, str]:
    exe = executable_path(example, args.build_dir)
    png = output_path(example, args.image_dir)
    if not exe.is_file():
        return False, f"missing executable: {exe.relative_to(ROOT)}"
    if example.capture_mode == "scenario" and not uses_scenario_runner(example):
        return False, "public gallery captures must use dvz_scenario_run_native_cli()"
    if example.capture_mode != "scenario" and not example.capture_reason:
        return False, "custom capture mode requires capture.reason in the manifest"

    env = os.environ.copy()
    apply_runtime_env(env)
    png.parent.mkdir(parents=True, exist_ok=True)
    env["DVZ_CAPTURE_DIR"] = str(png.parent)
    env["DVZ_CAPTURE_BASENAME"] = png.stem

    cmd = command_for(example, args.build_dir)
    if args.dry_run:
        rel_png = png.relative_to(ROOT)
        return True, f"dry-run: {' '.join(cmd)} -> {rel_png}"

    result = subprocess.run(cmd, cwd=ROOT, env=env, check=False)
    if result.returncode != 0:
        return False, f"exit {result.returncode}"

    if args.skip_nonblank_check:
        return True, str(png.relative_to(ROOT))

    ok, detail = png_is_nonblank(png, (example.expected_width, example.expected_height))
    if not ok:
        return False, detail
    return True, detail


def print_examples(examples: list[CaptureExample], args: argparse.Namespace) -> None:
    for example in examples:
        png = output_path(example, args.image_dir).relative_to(ROOT)
        exe = executable_path(example, args.build_dir)
        exe_display = exe.relative_to(ROOT) if exe.is_relative_to(ROOT) else exe
        size = f"{example.expected_width}x{example.expected_height}"
        print(f"{example.id:32} {example.lane:10} {size:10} {exe_display} -> {png}")


def main() -> int:
    args = parse_args()
    ids = split_values(args.id)
    lanes = split_values(args.lane)
    if not (ids or lanes or args.landing or args.all_screenshot):
        print("No capture selection provided; use --id, --lane, --landing, or --all-screenshot.", file=sys.stderr)
        return 2

    try:
        manifest = load_manifest(args.manifest)
        examples = collect_examples(manifest)
    except (OSError, ValueError, yaml.YAMLError) as exc:
        print(str(exc), file=sys.stderr)
        return 2

    selected = [example for example in examples if matches_filter(example, ids, lanes, args)]
    selected.sort(key=lambda item: (item.lane, item.id))
    if not selected:
        print("No matching gallery examples.", file=sys.stderr)
        return 1

    if args.list:
        print_examples(selected, args)
        return 0

    failures = 0
    for index, example in enumerate(selected, 1):
        print(f"[{index}/{len(selected)}] {example.id} ({example.lane})")
        ok, detail = capture_one(example, args)
        marker = "ok" if ok else "fail"
        print(f"  {marker}: {detail}")
        if not ok:
            failures += 1

    if args.landing:
        print("note: the WebGPU landing card needs a separate browser-smoke screenshot.")
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
