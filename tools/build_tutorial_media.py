#!/usr/bin/env python3
"""Generate build-local previews from the canonical Vulkan course programs."""

from __future__ import annotations

import argparse
from dataclasses import dataclass
from pathlib import Path
import shutil
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_EXECUTABLES_DIR = ROOT / "build" / "examples" / "c" / "vulkan"
DEFAULT_OUTPUT_DIR = ROOT / "build" / "vulkan-course-media"
SIZE = (800, 600)
DEFAULT_QUALITY = 90
ANIMATION_FPS = 6
ANIMATION_TIMES = tuple(i / ANIMATION_FPS for i in range(12))
EXPECTED_STEP03_RGBA = (
    (89, 97, 118, 255),
    (100, 113, 127, 255),
    (109, 125, 136, 255),
    (116, 132, 143, 255),
    (121, 134, 149, 255),
    (123, 133, 154, 255),
    (123, 126, 158, 255),
    (121, 116, 160, 255),
    (117, 101, 162, 255),
    (110, 82, 162, 255),
    (101, 59, 161, 255),
    (90, 32, 158, 255),
)
EXPECTED_OUTPUTS = (
    "01-setup.webp",
    "02-window.webp",
    "03-frame-still.webp",
    "03-frame.webp",
)


@dataclass(frozen=True)
class TutorialMediaResult:
    generated: int = 0
    skipped: int = 0
    missing: int = 0
    invalid: int = 0


def _executable(directory: Path, name: str) -> Path:
    path = directory / name
    if path.is_file():
        return path
    windows = path.with_suffix(".exe")
    return windows if windows.is_file() else path


def _run_step(executable: Path, arguments: list[str]) -> str:
    result = subprocess.run(
        [str(executable), *arguments],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if result.returncode != 0:
        raise RuntimeError(
            f"{executable.name} failed with status {result.returncode}:\n{result.stdout}"
        )
    return result.stdout


def _pillow():
    try:
        from PIL import Image, ImageDraw, ImageFont
    except ImportError as e:
        raise RuntimeError("Pillow is required for Vulkan course preview generation") from e
    return Image, ImageDraw, ImageFont


def _render_terminal_card(output: str, path: Path) -> None:
    Image, ImageDraw, ImageFont = _pillow()
    image = Image.new("RGBA", SIZE, (13, 18, 25, 255))
    draw = ImageDraw.Draw(image)
    draw.rounded_rectangle(
        (48, 64, 752, 536),
        radius=18,
        fill=(24, 31, 42, 255),
        outline=(55, 70, 86, 255),
        width=2,
    )
    draw.rounded_rectangle((48, 64, 752, 118), radius=18, fill=(34, 43, 56, 255))
    draw.rectangle((48, 98, 752, 118), fill=(34, 43, 56, 255))
    controls = (
        (78, (244, 93, 101, 255)),
        (106, (244, 190, 85, 255)),
        (134, (74, 201, 126, 255)),
    )
    for x, color in controls:
        draw.ellipse((x - 8, 83 - 8, x + 8, 83 + 8), fill=color)
    font = ImageFont.load_default(size=24)
    small = ImageFont.load_default(size=18)
    draw.text((174, 75), "Vulkan course · setup", font=small, fill=(178, 190, 204, 255))
    draw.text((86, 166), "$ ./step01", font=font, fill=(92, 219, 229, 255))
    lines = [line for line in output.strip().splitlines() if line]
    y = 218
    for line in lines[:8]:
        draw.text((86, y), line, font=font, fill=(225, 232, 240, 255))
        y += 38
    image.save(path)


def _validate_exact_rgba(path: Path, expected: tuple[int, int, int, int]) -> None:
    Image, _, _ = _pillow()
    with Image.open(path) as source:
        image = source.convert("RGBA")
        if image.size != SIZE:
            raise RuntimeError(f"{path}: expected {SIZE[0]}x{SIZE[1]}, got {image.size}")
        extrema = image.getextrema()
        exact = tuple((channel, channel) for channel in expected)
        if extrema != exact:
            actual = image.getpixel((0, 0))
            raise RuntimeError(f"{path}: expected exact RGBA {expected}, got {actual} / {extrema}")


def _encode_static(source: Path, output: Path, quality: int) -> None:
    cwebp = shutil.which("cwebp")
    if cwebp is None:
        raise RuntimeError("cwebp not found; install the WebP tools package")
    subprocess.run(
        [cwebp, "-quiet", "-q", str(quality), str(source), "-o", str(output)], check=True
    )


def _encode_animation(frames: list[Path], output: Path, quality: int) -> None:
    img2webp = shutil.which("img2webp")
    if img2webp is None:
        raise RuntimeError("img2webp not found; install the WebP tools package")
    delay_ms = round(1000 / ANIMATION_FPS)
    command = [img2webp, "-loop", "0"]
    for frame in frames:
        command.extend(["-d", str(delay_ms), "-lossy", "-q", str(quality), str(frame)])
    command.extend(["-o", str(output)])
    subprocess.run(command, check=True)


def _current(executables: list[Path], outputs: list[Path], force: bool) -> bool:
    if force or any(not output.is_file() for output in outputs):
        return False
    input_times = [Path(__file__).stat().st_mtime_ns]
    input_times.extend(path.stat().st_mtime_ns for path in executables)
    newest_input = max(input_times)
    return min(output.stat().st_mtime_ns for output in outputs) >= newest_input


def generate_tutorial_media(
    *,
    executables_dir: Path = DEFAULT_EXECUTABLES_DIR,
    output_dir: Path = DEFAULT_OUTPUT_DIR,
    quality: int = DEFAULT_QUALITY,
    force: bool = False,
    strict: bool = False,
) -> tuple[int, TutorialMediaResult]:
    if not 0 <= quality <= 100:
        print("--quality must be between 0 and 100")
        return 2, TutorialMediaResult(invalid=1)

    executables = [_executable(executables_dir, f"step0{i}") for i in range(1, 4)]
    missing = [path for path in executables if not path.is_file()]
    if missing:
        for path in missing:
            print(f"missing Vulkan course executable: {path}")
        print("Run: just build")
        result = TutorialMediaResult(missing=len(missing))
        return (2 if strict else 0), result

    outputs = [output_dir / name for name in EXPECTED_OUTPUTS]
    if _current(executables, outputs, force):
        result = TutorialMediaResult(skipped=len(outputs))
        print(f"Vulkan course media: generated=0 skipped={result.skipped} missing=0 invalid=0")
        return 0, result

    output_dir.mkdir(parents=True, exist_ok=True)
    try:
        with tempfile.TemporaryDirectory(prefix="datoviz-vulkan-course-media-") as scratch:
            temporary = Path(scratch)

            step01_output = _run_step(executables[0], [])
            if not any(line.startswith("Datoviz ") for line in step01_output.splitlines()):
                raise RuntimeError("step01 output does not contain the Datoviz version")
            terminal_png = temporary / "01-setup.png"
            _render_terminal_card(step01_output, terminal_png)
            _encode_static(terminal_png, outputs[0], quality)

            step02_png = temporary / "02-window.png"
            step02_output = _run_step(executables[1], ["--png", str(step02_png)])
            if "validation errors: 0" not in step02_output:
                raise RuntimeError("step02 reported Vulkan validation errors")
            _validate_exact_rgba(step02_png, (89, 97, 118, 255))
            _encode_static(step02_png, outputs[1], quality)

            frames = []
            for index, (time_s, expected_rgba) in enumerate(
                zip(ANIMATION_TIMES, EXPECTED_STEP03_RGBA, strict=True)
            ):
                frame = temporary / f"03-frame-{index:03d}.png"
                step03_output = _run_step(
                    executables[2], ["--png", str(frame), "--time", f"{time_s:.9g}"]
                )
                if "validation errors: 0" not in step03_output:
                    raise RuntimeError(f"step03 frame {index} reported Vulkan validation errors")
                _validate_exact_rgba(frame, expected_rgba)
                frames.append(frame)
            if len({path.read_bytes() for path in frames}) != len(frames):
                raise RuntimeError("step03 fixed-time captures are not all distinct")
            _encode_static(frames[3], outputs[2], quality)
            _encode_animation(frames, outputs[3], quality)
    except (OSError, RuntimeError, subprocess.CalledProcessError) as e:
        print(f"Vulkan course media: {e}")
        return 2, TutorialMediaResult(invalid=1)

    result = TutorialMediaResult(generated=len(outputs))
    print(f"Vulkan course media: generated={result.generated} skipped=0 missing=0 invalid=0")
    return 0, result


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--executables-dir", type=Path, default=DEFAULT_EXECUTABLES_DIR)
    parser.add_argument("--output-dir", type=Path, default=DEFAULT_OUTPUT_DIR)
    parser.add_argument("--quality", type=int, default=DEFAULT_QUALITY)
    parser.add_argument("--force", action="store_true")
    parser.add_argument("--strict", action="store_true")
    args = parser.parse_args()
    rc, _ = generate_tutorial_media(
        executables_dir=args.executables_dir,
        output_dir=args.output_dir,
        quality=args.quality,
        force=args.force,
        strict=args.strict,
    )
    return rc


if __name__ == "__main__":
    raise SystemExit(main())
