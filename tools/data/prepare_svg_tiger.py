#!/usr/bin/env python3
"""Flatten the pinned Glumpy tiger SVG into a compact C-readable cache bundle."""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import re
import struct
import urllib.request
import xml.etree.ElementTree as ET
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, Sequence

try:
    from .common import CACHE_ROOT
except ImportError:
    from common import CACHE_ROOT


EXAMPLE_ID = "svg_tiger"
DEFAULT_OUTPUT = CACHE_ROOT / EXAMPLE_ID
SOURCE_COMMIT = "aedb9212a1e00a68b7c4669405a6a8f754daf283"
SOURCE_URL = (
    "https://raw.githubusercontent.com/glumpy/glumpy/"
    f"{SOURCE_COMMIT}/glumpy/data/tiger.svg"
)
SOURCE_BYTES = 110724
SOURCE_SHA256 = "1bc237707ae0523f0e2115917626bfaceb24bc4a388b5ef659b5604b311ff537"

MAGIC = b"DVZSVG1\0"
VERSION = 2
HEADER = struct.Struct("<8sIIII6d")
PATH_RECORD = struct.Struct("<II4B4B4BfI")
POINT = struct.Struct("<dd")

EXPECTED_PATH_COUNT = 240
EXPECTED_CLOSED_COUNT = 227
EXPECTED_OPEN_COUNT = 13

_NUMBER = r"[-+]?(?:\d+\.?(?:\d*)?|\.\d+)(?:[eE][-+]?\d+)?"
_TOKEN_RE = re.compile(rf"[MmLlCcZz]|{_NUMBER}")
_TRANSFORM_RE = re.compile(r"\s*matrix\s*\(([^)]*)\)\s*")
_SUPPORTED_PATH_COMMANDS = frozenset("MmLlCcZz")
_IGNORED_ELEMENTS = frozenset({"metadata", "defs", "namedview", "title", "desc"})

Matrix = tuple[float, float, float, float, float, float]
Point = tuple[float, float]


class SvgTigerError(ValueError):
    """Raised when the input exceeds the intentionally narrow SVG subset."""


@dataclass(frozen=True)
class Paint:
    """Resolved solid fill and stroke paint for one SVG path."""

    fill: tuple[int, int, int, int] | None
    stroke: tuple[int, int, int, int] | None
    stroke_width: float


@dataclass(frozen=True)
class FlatPath:
    """One flattened SVG path in document coordinates."""

    points: tuple[Point, ...]
    closed: bool
    paint: Paint
    paint_order: int


@dataclass(frozen=True)
class SvgDocument:
    """Flattened path document plus its declared viewport."""

    width: float
    height: float
    paths: tuple[FlatPath, ...]


def _local_name(tag: str) -> str:
    """Return an XML element name without its namespace."""
    return tag.rsplit("}", 1)[-1]


def _sha256(path: Path) -> str:
    """Return the SHA-256 digest of one file."""
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def _parse_style(text: str | None) -> dict[str, str]:
    """Parse one CSS-style declaration list into a property mapping."""
    out: dict[str, str] = {}
    if not text:
        return out
    for declaration in text.split(";"):
        if not declaration.strip():
            continue
        if ":" not in declaration:
            raise SvgTigerError(f"invalid style declaration: {declaration!r}")
        key, value = declaration.split(":", 1)
        out[key.strip()] = value.strip()
    return out


def _element_style(element: ET.Element, inherited: dict[str, str]) -> dict[str, str]:
    """Resolve inherited presentation properties for one element."""
    style = dict(inherited)
    style.update(_parse_style(element.get("style")))
    for key in (
        "fill",
        "stroke",
        "stroke-width",
        "fill-opacity",
        "stroke-opacity",
        "opacity",
        "fill-rule",
        "stroke-linecap",
        "stroke-linejoin",
    ):
        value = element.get(key)
        if value is not None:
            style[key] = value
    return style


def _opacity(style: dict[str, str], key: str) -> float:
    """Return a finite clamped opacity property."""
    try:
        value = float(style.get(key, "1"))
    except ValueError as exc:
        raise SvgTigerError(f"invalid {key}: {style.get(key)!r}") from exc
    if not math.isfinite(value):
        raise SvgTigerError(f"non-finite {key}")
    return min(1.0, max(0.0, value))


def _color(value: str, alpha: float) -> tuple[int, int, int, int] | None:
    """Parse a narrow solid SVG color value."""
    value = value.strip().lower()
    if value == "none":
        return None
    named = {"black": "#000000", "white": "#ffffff"}
    value = named.get(value, value)
    if re.fullmatch(r"#[0-9a-f]{3}", value):
        value = "#" + "".join(channel * 2 for channel in value[1:])
    if not re.fullmatch(r"#[0-9a-f]{6}", value):
        raise SvgTigerError(f"unsupported solid color: {value!r}")
    rgb = tuple(int(value[i : i + 2], 16) for i in (1, 3, 5))
    return rgb + (round(255 * alpha),)


def _paint(style: dict[str, str]) -> Paint:
    """Build resolved solid paint from inherited style properties."""
    opacity = _opacity(style, "opacity")
    fill = _color(style.get("fill", "black"), opacity * _opacity(style, "fill-opacity"))
    stroke = _color(style.get("stroke", "none"), opacity * _opacity(style, "stroke-opacity"))
    try:
        stroke_width = float(style.get("stroke-width", "1"))
    except ValueError as exc:
        raise SvgTigerError(f"invalid stroke width: {style.get('stroke-width')!r}") from exc
    if not math.isfinite(stroke_width) or stroke_width < 0:
        raise SvgTigerError("stroke width must be finite and nonnegative")
    if style.get("fill-rule", "nonzero") not in {"nonzero", "evenodd"}:
        raise SvgTigerError(f"unsupported fill rule: {style['fill-rule']!r}")
    return Paint(fill=fill, stroke=stroke, stroke_width=stroke_width)


def _identity() -> Matrix:
    """Return the identity SVG affine transform."""
    return (1.0, 0.0, 0.0, 1.0, 0.0, 0.0)


def _multiply(left: Matrix, right: Matrix) -> Matrix:
    """Compose two SVG affine transforms as left times right."""
    a, b, c, d, e, f = left
    g, h, i, j, k, l = right
    return (
        a * g + c * h,
        b * g + d * h,
        a * i + c * j,
        b * i + d * j,
        a * k + c * l + e,
        b * k + d * l + f,
    )


def _parse_transform(text: str | None) -> Matrix:
    """Parse the matrix-only transform subset required by the tiger."""
    if text is None or not text.strip():
        return _identity()
    match = _TRANSFORM_RE.fullmatch(text)
    if match is None:
        raise SvgTigerError(f"unsupported transform: {text!r}")
    values = [float(value) for value in re.findall(_NUMBER, match.group(1))]
    if len(values) != 6 or not all(math.isfinite(value) for value in values):
        raise SvgTigerError(f"invalid affine matrix: {text!r}")
    return tuple(values)  # type: ignore[return-value]


def _transform(point: Point, matrix: Matrix) -> Point:
    """Apply one SVG affine transform to a point."""
    x, y = point
    a, b, c, d, e, f = matrix
    return (a * x + c * y + e, b * x + d * y + f)


def _point_line_distance(point: Point, start: Point, end: Point) -> float:
    """Return the perpendicular distance to an infinite chord or a degenerate endpoint."""
    dx = end[0] - start[0]
    dy = end[1] - start[1]
    length = math.hypot(dx, dy)
    if length == 0:
        return math.hypot(point[0] - start[0], point[1] - start[1])
    return abs(dx * (start[1] - point[1]) - (start[0] - point[0]) * dy) / length


def _midpoint(left: Point, right: Point) -> Point:
    """Return the midpoint of two points."""
    return ((left[0] + right[0]) * 0.5, (left[1] + right[1]) * 0.5)


def _flatten_cubic(
    p0: Point,
    p1: Point,
    p2: Point,
    p3: Point,
    tolerance: float,
    out: list[Point],
    depth: int = 0,
) -> None:
    """Append an adaptively flattened cubic endpoint sequence."""
    flatness = max(_point_line_distance(p1, p0, p3), _point_line_distance(p2, p0, p3))
    if flatness <= tolerance or depth >= 24:
        out.append(p3)
        return
    p01 = _midpoint(p0, p1)
    p12 = _midpoint(p1, p2)
    p23 = _midpoint(p2, p3)
    p012 = _midpoint(p01, p12)
    p123 = _midpoint(p12, p23)
    p0123 = _midpoint(p012, p123)
    _flatten_cubic(p0, p01, p012, p0123, tolerance, out, depth + 1)
    _flatten_cubic(p0123, p123, p23, p3, tolerance, out, depth + 1)


def _tokens(path_data: str) -> list[str]:
    """Tokenize path data and reject characters outside the supported grammar."""
    matches = list(_TOKEN_RE.finditer(path_data))
    residue = _TOKEN_RE.sub("", path_data)
    if residue.replace(",", "").strip():
        raise SvgTigerError(f"invalid SVG path syntax near {residue!r}")
    tokens = [match.group(0) for match in matches]
    for token in tokens:
        if token.isalpha() and token not in _SUPPORTED_PATH_COMMANDS:
            raise SvgTigerError(f"unsupported path command: {token}")
    return tokens


def _take_numbers(
    tokens: Sequence[str], index: int, count: int, command: str
) -> tuple[list[float], int]:
    """Read a fixed number of numeric path arguments."""
    if index + count > len(tokens) or any(tokens[i].isalpha() for i in range(index, index + count)):
        raise SvgTigerError(f"path command {command} has incomplete arguments")
    values = [float(tokens[i]) for i in range(index, index + count)]
    if not all(math.isfinite(value) for value in values):
        raise SvgTigerError(f"path command {command} has non-finite arguments")
    return values, index + count


def _deduplicate(points: Iterable[Point]) -> tuple[Point, ...]:
    """Remove consecutive duplicate points from a flattened contour."""
    out: list[Point] = []
    for point in points:
        if not out or point != out[-1]:
            out.append(point)
    if len(out) >= 2 and out[0] == out[-1]:
        out.pop()
    return tuple(out)


def _flatten_path(
    path_data: str, matrix: Matrix, tolerance: float
) -> tuple[tuple[Point, ...], bool]:
    """Parse and flatten one single-subpath SVG path."""
    tokens = _tokens(path_data)
    if not tokens:
        raise SvgTigerError("empty SVG path")

    index = 0
    command: str | None = None
    current = (0.0, 0.0)
    start: Point | None = None
    points: list[Point] = []
    closed = False
    moved = False

    while index < len(tokens):
        token = tokens[index]
        if token.isalpha():
            command = token
            index += 1
        if command is None:
            raise SvgTigerError("path data must begin with a command")

        relative = command.islower()
        upper = command.upper()
        if upper == "Z":
            if start is None:
                raise SvgTigerError("close command without a current subpath")
            closed = True
            current = start
            command = None
            if index != len(tokens):
                raise SvgTigerError("multiple subpaths in one path element are not supported")
            continue

        argument_count = {"M": 2, "L": 2, "C": 6}.get(upper)
        if argument_count is None:
            raise SvgTigerError(f"unsupported path command: {command}")
        values, index = _take_numbers(tokens, index, argument_count, command)

        origin = current if relative else (0.0, 0.0)
        if upper == "M":
            target = (values[0] + origin[0], values[1] + origin[1])
            if moved:
                raise SvgTigerError("multiple subpaths in one path element are not supported")
            current = target
            start = target
            points.append(_transform(target, matrix))
            moved = True
            command = "l" if relative else "L"
        elif upper == "L":
            if start is None:
                raise SvgTigerError("line command before move command")
            target = (values[0] + origin[0], values[1] + origin[1])
            current = target
            points.append(_transform(target, matrix))
        else:
            if start is None:
                raise SvgTigerError("cubic command before move command")
            p0 = _transform(current, matrix)
            p1 = _transform((values[0] + origin[0], values[1] + origin[1]), matrix)
            p2 = _transform((values[2] + origin[0], values[3] + origin[1]), matrix)
            target = (values[4] + origin[0], values[5] + origin[1])
            p3 = _transform(target, matrix)
            _flatten_cubic(p0, p1, p2, p3, tolerance, points)
            current = target

    flattened = _deduplicate(points)
    # Preserve degenerate authored paths in the document statistics and paint ordering. Glumpy does
    # the same and skips them only when building render collections; the tiger contains one such
    # one-point closed path (path242).
    if not flattened:
        raise SvgTigerError("path has no distinct flattened points")
    return flattened, closed


def parse_svg(path: Path, tolerance: float = 0.25) -> SvgDocument:
    """Parse the narrow solid-path SVG subset used by the tiger."""
    if not math.isfinite(tolerance) or tolerance <= 0:
        raise SvgTigerError("flattening tolerance must be positive and finite")
    try:
        root = ET.parse(path).getroot()
    except ET.ParseError as exc:
        raise SvgTigerError(f"invalid XML: {exc}") from exc
    if _local_name(root.tag) != "svg":
        raise SvgTigerError("document root must be <svg>")

    try:
        width = float(root.get("width", "0"))
        height = float(root.get("height", "0"))
    except ValueError as exc:
        raise SvgTigerError("SVG width and height must be numeric") from exc
    if not math.isfinite(width) or not math.isfinite(height) or width <= 0 or height <= 0:
        raise SvgTigerError("SVG width and height must be positive")

    paths: list[FlatPath] = []
    # Match Glumpy's Style defaults rather than the browser SVG default: unspecified paint is
    # disabled. This matters for the tiger's open muzzle strokes, which specify only ``stroke``.
    base_style = {
        "fill": "none",
        "stroke": "none",
        "stroke-width": "1",
        "fill-opacity": "1",
        "stroke-opacity": "1",
        "opacity": "1",
        "fill-rule": "nonzero",
    }

    def visit(element: ET.Element, inherited_style: dict[str, str], parent_matrix: Matrix) -> None:
        name = _local_name(element.tag)
        style = _element_style(element, inherited_style)
        matrix = _multiply(parent_matrix, _parse_transform(element.get("transform")))
        if name == "path":
            data = element.get("d")
            if data is None:
                raise SvgTigerError("path element is missing d data")
            try:
                points, closed = _flatten_path(data, matrix, tolerance)
            except SvgTigerError as exc:
                identifier = element.get("id", f"paint-order-{len(paths)}")
                raise SvgTigerError(f"path {identifier}: {exc}") from exc
            paths.append(FlatPath(points, closed, _paint(style), len(paths)))
            return
        if name not in {"svg", "g"} and name not in _IGNORED_ELEMENTS:
            raise SvgTigerError(f"unsupported SVG element: <{name}>")
        if name in _IGNORED_ELEMENTS:
            return
        for child in element:
            visit(child, style, matrix)

    visit(root, base_style, _identity())
    if not paths:
        raise SvgTigerError("SVG contains no supported paths")
    return SvgDocument(width=width, height=height, paths=tuple(paths))


def _rgba(value: tuple[int, int, int, int] | None) -> tuple[int, int, int, int]:
    """Return transparent black for a disabled paint."""
    return value if value is not None else (0, 0, 0, 0)


def write_bundle(document: SvgDocument, output: Path, source_path: Path, tolerance: float) -> Path:
    """Write deterministic path records and F64 points into the prepared cache."""
    prepared = output / "prepared"
    prepared.mkdir(parents=True, exist_ok=True)
    bundle_path = prepared / "tiger_paths.bin"

    point_count = sum(len(path.points) for path in document.paths)
    all_points = [point for path in document.paths for point in path.points]
    xs = [point[0] for point in all_points]
    ys = [point[1] for point in all_points]
    bounds = (min(xs), min(ys), max(xs), max(ys))

    with bundle_path.open("wb") as stream:
        stream.write(
            HEADER.pack(
                MAGIC,
                VERSION,
                len(document.paths),
                point_count,
                PATH_RECORD.size,
                document.width,
                document.height,
                *bounds,
            )
        )
        offset = 0
        for path in document.paths:
            fill = _rgba(path.paint.fill)
            stroke = _rgba(path.paint.stroke)
            stream.write(
                PATH_RECORD.pack(
                    offset,
                    len(path.points),
                    int(path.closed),
                    int(path.paint.fill is not None),
                    int(path.paint.stroke is not None),
                    0,
                    *fill,
                    *stroke,
                    path.paint.stroke_width,
                    path.paint_order,
                )
            )
            offset += len(path.points)
        for point in all_points:
            stream.write(POINT.pack(*point))

    metadata = {
        "schema": "datoviz.svg-path-cache.v1",
        "source": {
            "url": SOURCE_URL,
            "commit": SOURCE_COMMIT,
            "bytes": source_path.stat().st_size,
            "sha256": _sha256(source_path),
            "attribution": "Nicolas P. Rougier, Glumpy example gallery, classic SVG Tiger example",
            "gallery": "https://glumpy.github.io/gallery.html",
            "redistribution": "maintainer-approved for Datoviz prepared-data publication",
        },
        "processing": {
            "flatten_tolerance_px": tolerance,
            "paint_defaults": "Glumpy-compatible: unspecified fill and stroke are disabled",
            "stroke_width_policy": "unscaled, matching the Glumpy tiger example",
        },
        "document": {
            "width": document.width,
            "height": document.height,
            "path_count": len(document.paths),
            "closed_path_count": sum(path.closed for path in document.paths),
            "open_path_count": sum(not path.closed for path in document.paths),
            "point_count": point_count,
            "renderable_path_count": sum(
                len(path.points) >= (3 if path.closed else 2) for path in document.paths
            ),
            "bounds": bounds,
        },
        "artifact": {
            "path": bundle_path.name,
            "bytes": bundle_path.stat().st_size,
            "sha256": _sha256(bundle_path),
        },
    }
    (prepared / "metadata.json").write_text(
        json.dumps(metadata, indent=2, sort_keys=True) + "\n", encoding="utf8"
    )
    return bundle_path


def _source(args: argparse.Namespace) -> Path:
    """Resolve, optionally download, and verify the pinned source SVG."""
    source = args.input
    if args.download:
        source = args.output / "source" / "tiger.svg"
        source.parent.mkdir(parents=True, exist_ok=True)
        if args.force or not source.exists():
            with urllib.request.urlopen(SOURCE_URL, timeout=120) as response:
                source.write_bytes(response.read())
    if source is None:
        raise SvgTigerError("pass --input PATH or --download")
    if not source.is_file():
        raise SvgTigerError(f"source SVG does not exist: {source}")
    actual_bytes = source.stat().st_size
    actual_sha256 = _sha256(source)
    if actual_bytes != SOURCE_BYTES or actual_sha256 != SOURCE_SHA256:
        raise SvgTigerError(
            "source does not match the pinned Glumpy tiger: "
            f"bytes={actual_bytes}, sha256={actual_sha256}"
        )
    return source


def prepare(args: argparse.Namespace) -> Path:
    """Prepare and validate the cache-local tiger path bundle."""
    source = _source(args)
    document = parse_svg(source, args.tolerance)
    closed_count = sum(path.closed for path in document.paths)
    open_count = len(document.paths) - closed_count
    if (
        len(document.paths) != EXPECTED_PATH_COUNT
        or closed_count != EXPECTED_CLOSED_COUNT
        or open_count != EXPECTED_OPEN_COUNT
    ):
        raise SvgTigerError(
            "unexpected pinned tiger structure: "
            f"paths={len(document.paths)}, closed={closed_count}, open={open_count}"
        )
    return write_bundle(document, args.output, source, args.tolerance)


def _parser() -> argparse.ArgumentParser:
    """Build the command-line parser."""
    parser = argparse.ArgumentParser(description=__doc__)
    source = parser.add_mutually_exclusive_group(required=True)
    source.add_argument("--input", type=Path, help="existing pinned tiger.svg")
    source.add_argument("--download", action="store_true", help="download the pinned Glumpy SVG")
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT, help="cache bundle root")
    parser.add_argument("--tolerance", type=float, default=0.25, help="curve error in SVG pixels")
    parser.add_argument("--force", action="store_true", help="redownload the pinned source")
    return parser


def main() -> int:
    """Prepare the tiger bundle from command-line arguments."""
    args = _parser().parse_args()
    try:
        bundle = prepare(args)
    except (OSError, SvgTigerError) as exc:
        raise SystemExit(f"prepare_svg_tiger: {exc}") from exc
    print(bundle)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
