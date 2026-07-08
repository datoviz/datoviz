#!/usr/bin/env python3
"""Generate the v0.4 public example gallery from the C example manifest."""

from __future__ import annotations

import argparse
import posixpath
import re
import shutil
from dataclasses import dataclass
from pathlib import Path
from textwrap import dedent

import gallery_media


ROOT = gallery_media.ROOT
DEFAULT_MANIFEST = gallery_media.DEFAULT_MANIFEST
DEFAULT_DOCS_DIR = ROOT / "docs/examples"
DEFAULT_IMAGE_DIR = ROOT / "data/gallery/v0.4"
DEFAULT_IMAGE_URL_BASE = "/assets/gallery/v0.4"
DEFAULT_IMAGE_FORMAT = "webp"
SOURCE_BASE_URL = "https://github.com/datoviz/datoviz/blob/v0.4-dev"
PUBLIC_LANES = gallery_media.DOC_LANES
STATUS_ORDER = ("supported", "experimental", "prototype", "advanced/unstable", "deferred")
DEFAULT_STATUS = "supported"
SOURCE_LANGUAGE_BY_SUFFIX = {
    ".c": ("C", "c"),
    ".h": ("C", "c"),
    ".cpp": ("C++", "cpp"),
    ".cc": ("C++", "cpp"),
    ".cxx": ("C++", "cpp"),
    ".hpp": ("C++", "cpp"),
    ".py": ("Python", "python"),
    ".js": ("JavaScript", "javascript"),
    ".mjs": ("JavaScript", "javascript"),
}
SOURCE_LABEL_BY_LANGUAGE = {
    "c": "C",
    "cpp": "C++",
    "python": "Python",
    "javascript": "JavaScript",
}

# Editorial showcase groups. The 2D/3D split describes the primary gallery
# presentation, not a strict mathematical classification.
SHOWCASE_GROUPS = (
    ("2D", [
        "showcases_scientific_plotting",
        "showcases_panel_linked_axes",
        "showcases_linked_probe_colorbar",
        "showcases_scalebar_measurement",
        "showcases_choropleth",
        "showcases_wind_field",
        "showcases_gpu_particle_smoke",
    ]),
    ("3D", [
        "showcases_brain_volume",
        "showcases_protein",
        "showcases_point_cloud",
        "showcases_surface_grid",
        "showcases_textured_planet",
    ]),
)
SHOWCASE_ORDER = tuple(id_ for _, ids in SHOWCASE_GROUPS for id_ in ids)

# Visuals grouped by dimensionality; each tuple is (subheading, [ids]).
INDEX_VISUAL_GROUPS = (
    ("0D — point-like", ["visuals_point", "visuals_pixel", "visuals_marker", "visuals_splat"]),
    ("1D — line-like",  ["visuals_segment", "visuals_path", "visuals_vector", "visuals_primitive"]),
    ("2D — planar",     ["visuals_image", "visuals_image_rgba", "visuals_text", "visuals_glyph", "visuals_labels"]),
    ("3D — volumetric", ["visuals_mesh", "visuals_sphere", "visuals_volume"]),
    ("Composites",      ["composites_polygon", "composites_graph"]),
)

VISUAL_REFERENCE_BY_ID = {
    "visuals_point": "point",
    "visuals_pixel": "pixel",
    "visuals_marker": "marker",
    "visuals_splat": "splat",
    "visuals_segment": "segment",
    "visuals_path": "path",
    "visuals_vector": "vector",
    "visuals_primitive": "primitive",
    "visuals_image": "image",
    "visuals_image_rgba": "image",
    "visuals_text": "text",
    "visuals_glyph": "glyph",
    "visuals_labels": "labels",
    "visuals_mesh": "mesh",
    "visuals_sphere": "sphere",
    "visuals_volume": "visuals_volume",
}

# Full feature grouping used on the features page — covers all 71 public features.
FEATURE_PAGE_GROUPS = (
    ("Scene & Layout", [
        "features_basic_scene",
        "features_coordinate_system",
        "features_panel_single",
        "features_panel_grid",
        "features_panel_multi",
        "features_panel_linked",
        "features_panel_view2d",
        "features_panel_background",
        "features_user_scale",
        "features_view_size_policies",
        "features_visual_transform",
        "features_visibility",
    ]),
    ("Navigation", [
        "features_camera_manual",
        "features_panzoom",
        "features_controller_arcball",
        "features_controller_turntable",
        "features_controller_fly",
        "features_orientation_gizmo",
        "features_reference_grid",
    ]),
    ("Adornments", [
        "features_axis_labels",
        "features_axes_2d",
        "features_guide_lines",
        "features_guide_spans",
        "features_bars_bands",
        "features_scalebar",
        "features_scalebar_units",
        "features_colorbar",
        "features_colormap_scale",
        "features_legend_categorical",
        "features_annotation_readout",
        "features_text_block",
        "features_overlay_card",
        "features_probe_labels",
    ]),
    ("Shapes & Geometry", [
        "features_builtin_shapes_2d",
        "features_builtin_shapes_3d",
        "features_marker_symbols",
        "features_bezier_curve_path",
        "features_path_join",
        "features_obj_loading",
    ]),
    ("Scientific", [
        "features_sampled_field_update",
        "features_isolines",
        "features_datetime_axis",
        "features_image_probe",
    ]),
    ("3D Rendering", [
        "features_lighting",
        "features_mesh_texture",
        "features_material_mesh",
        "features_volume_occlusion",
        "features_technique_edl",
        "features_technique_ssao",
        "features_technique_depth_cue",
        "features_technique_msaa",
        "features_technique_transparency",
        "features_alpha_blending",
        "features_technique_depth_test",
        "features_bounds_overlay",
    ]),
    ("Interaction & Selection", [
        "features_picking",
        "features_selection_pixel",
        "features_selection_sphere",
        "features_selection_mesh_instances",
    ]),
    ("Animation & Updates", [
        "features_animation_tracks",
        "features_timer_animation",
        "features_compute_buffer_animation",
        "features_update_partial",
        "features_update_visual_data",
    ]),
    ("GUI", [
        "features_gui_controls",
        "features_gui_viewport",
        "features_gui_cimgui",
    ]),
    ("Input & Diagnostics", [
        "features_input_events",
        "features_json_export",
    ]),
)

RUNTIME_PAGE_GROUPS = (
    ("Windows & Hosting", [
        "runtime_app_glfw",
        "runtime_multi_window",
    ]),
    ("Capture, Export & Replay", [
        "runtime_offscreen_capture",
        "runtime_video_export",
        "runtime_record_replay",
    ]),
)

# Flagship feature IDs shown on the examples index; the rest are reachable via the full gallery.
INDEX_FEATURE_GROUPS = (
    ("Layout", [
        "features_panel_grid",
        "features_panel_multi",
        "features_panel_linked",
    ]),
    ("Adornments", [
        "features_axis_labels",
        "features_colorbar",
        "features_scalebar",
        "features_colormap_scale",
    ]),
    ("Navigation", [
        "features_panzoom",
        "features_controller_arcball",
        "features_controller_fly",
        "features_controller_turntable",
    ]),
    ("Scientific", [
        "features_sampled_field_update",
        "features_isolines",
        "features_marker_symbols",
    ]),
    ("3D Rendering", [
        "features_lighting",
        "features_mesh_texture",
        "features_technique_ssao",
        "features_technique_depth_cue",
    ]),
    ("Animation & Interaction", [
        "features_animation_tracks",
        "features_timer_animation",
        "features_image_probe",
        "features_selection_sphere",
    ]),
    ("GUI", [
        "features_gui_controls",
        "features_gui_viewport",
        "features_gui_cimgui",
    ]),
)

CATEGORY_TO_LANE = gallery_media.CATEGORY_TO_LANE
LANE_TO_CATEGORY = gallery_media.LANE_TO_CATEGORY

PAGE_CONFIG = {
    "advanced.md": {
        "title": "Advanced Examples",
        "lanes": ("advanced",),
        "intro": (
            "Browse advanced runtime and host-integration examples. "
            "These are useful after you are comfortable with ordinary scene code."
        ),
    },
    "runtime.md": {
        "title": "Runtime & Capture",
        "lanes": ("runtime",),
        "intro": (
            "Browse examples for opening windows, rendering offscreen, recording, replaying, "
            "and exporting media."
        ),
    },
}



@dataclass(frozen=True)
class SourceTab:
    label: str
    language: str
    path: str


@dataclass(frozen=True)
class Example:
    id: str
    legacy_id: str | None
    title: str
    category: str
    lane: str
    stage: str
    source: str
    validation: str
    status: str
    tags: tuple[str, ...]
    summary: str
    description: tuple[str, ...]
    primary: str
    data: dict
    dataset: dict
    encoding: dict
    webgpu: dict
    agent_copy_safe: bool | None
    source_label: str
    source_language: str
    extra_sources: tuple[SourceTab, ...]
    python_source: str | None
    python_status: str | None

    @property
    def source_path(self) -> Path:
        return ROOT / self.source

    @property
    def rel_executable(self) -> str:
        if not self.source.startswith("examples/c/"):
            raise ValueError(f"{self.source} is not a C example source")
        rel = Path(self.source).relative_to("examples/c")
        return rel.with_suffix("").as_posix()

    @property
    def page_path(self) -> str:
        return f"gallery/{self.lane}/{self.id}.md"

    @property
    def image_path(self) -> Path:
        return DEFAULT_IMAGE_DIR / self.lane / f"{self.id}.png"

    @property
    def image_ref(self) -> str:
        return f"{DEFAULT_IMAGE_URL_BASE}/{self.lane}/{self.id}.png"

    @property
    def python_source_path(self) -> Path | None:
        if self.python_source is None:
            return None
        return ROOT / self.python_source

    @property
    def webgpu_status(self) -> str:
        return str(self.webgpu.get("status", "unclassified"))

    @property
    def webgpu_route(self) -> str:
        return str(self.webgpu.get("route", ""))

    @property
    def webgpu_site_route(self) -> str:
        if not self.webgpu_route:
            return ""
        return f"/{self.webgpu_route}"

    @property
    def webgpu_requirements(self) -> tuple[str, ...]:
        requirements = self.webgpu.get("requirements") or ()
        return tuple(str(requirement) for requirement in requirements)

    @property
    def screenshot_expected(self) -> bool:
        return "screenshot" in self.validation

    @property
    def webgpu_reason(self) -> str:
        return str(self.webgpu.get("reason", ""))


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument("--docs-dir", type=Path, default=DEFAULT_DOCS_DIR)
    parser.add_argument("--image-dir", type=Path, default=DEFAULT_IMAGE_DIR)
    parser.add_argument("--image-url-base", default=DEFAULT_IMAGE_URL_BASE)
    parser.add_argument("--image-format", choices=("png", "webp"), default=DEFAULT_IMAGE_FORMAT)
    return parser.parse_args()


def load_manifest(path: Path) -> dict:
    return gallery_media.load_manifest(path)


def reviewed_example_ids(manifest: dict) -> set[str]:
    """Return example IDs that are approved for public website generation."""
    return gallery_media.reviewed_example_ids(manifest)


def status_from_entry(entry: dict) -> str:
    gallery = entry.get("gallery") or {}
    if gallery.get("status_label"):
        return str(gallery["status_label"])
    stage = str(entry.get("stage", ""))
    if "experimental" in stage:
        return "experimental"
    if "deferred" in stage:
        return "deferred"
    return "supported"


def normalize_summary(text: str) -> str:
    text = re.sub(r"\s+", " ", text).strip()
    if not text:
        return "Current C-first Datoviz example."
    text = re.sub(r"^(what to look for|what this shows):\s*", "", text, flags=re.IGNORECASE)
    text = re.sub(r"\bsmallest runner-backed retained scene\b", "smallest scene", text)
    text = re.sub(r"\bretained\s+", "", text)
    if re.match(r"python(?:3(?:\.\d+)?)?\s+", text):
        return text
    if text and text[0].islower():
        text = text[0].upper() + text[1:]
    return text


URL_RE = re.compile(r"(?<!\]\()https?://[^\s<>)|]+")
COMMAND_START_RE = re.compile(r"(?<!`)\bpython(?:3(?:\.\d+)?)?\s+", flags=re.IGNORECASE)
COMMAND_LABEL_RE = re.compile(
    r"\s+(?:Data|Source|Terms|Prepare|Promote|Build|Run|Smoke|Options|Debug):"
)


def _outside_code_span(text: str, index: int) -> bool:
    return text[:index].count("`") % 2 == 0


def markdown_links(text: str) -> str:
    """Wrap raw URLs in Markdown links."""

    def replace(match: re.Match[str]) -> str:
        if not _outside_code_span(text, match.start()):
            return match.group(0)
        url = match.group(0)
        trailing = ""
        while url and url[-1] in ".,;":
            trailing = url[-1] + trailing
            url = url[:-1]
        return f"[{url}]({url}){trailing}"

    return URL_RE.sub(replace, text)


def markdown_python_commands(text: str) -> str:
    """Wrap raw Python command invocations in inline code spans."""
    matches = [
        match
        for match in COMMAND_START_RE.finditer(text)
        if _outside_code_span(text, match.start())
    ]
    if not matches:
        return text

    chunks: list[str] = []
    cursor = 0
    for i, match in enumerate(matches):
        start = match.start()
        if start < cursor:
            continue
        next_start = matches[i + 1].start() if i + 1 < len(matches) else len(text)
        label = COMMAND_LABEL_RE.search(text, match.end())
        end = min(next_start, label.start() if label else len(text))
        command = text[start:end].rstrip()
        trailing = text[start + len(command):end]
        while command and command[-1] in ".,;":
            trailing = command[-1] + trailing
            command = command[:-1]
        chunks.append(text[cursor:start])
        chunks.append(f"`{command}`")
        chunks.append(trailing)
        cursor = end
    chunks.append(text[cursor:])
    return "".join(chunks)


def format_markdown_inline(value: object) -> str:
    """Format manifest/comment prose for generated Markdown pages."""
    text = str(value)
    text = markdown_links(text)
    text = markdown_python_commands(text)
    return text


def infer_source_language(path: str) -> tuple[str, str]:
    suffix = Path(path).suffix.lower()
    return SOURCE_LANGUAGE_BY_SUFFIX.get(suffix, ("Source", "text"))


def source_language_label(language: str, path: str) -> str:
    if language in SOURCE_LABEL_BY_LANGUAGE:
        return SOURCE_LABEL_BY_LANGUAGE[language]
    inferred_label, _ = infer_source_language(path)
    return inferred_label


def normalize_source_tab(raw: dict) -> SourceTab:
    path = str(raw.get("path") or raw.get("source") or "")
    if not path:
        raise ValueError("extra_sources entries must declare path or source")
    inferred_label, inferred_language = infer_source_language(path)
    language = str(raw.get("language") or inferred_language)
    label = str(raw.get("label") or source_language_label(language, path) or inferred_label)
    return SourceTab(label=label, language=language, path=path)


def extract_c_description(path: Path) -> tuple[str, tuple[str, ...]]:
    if not path.exists():
        return "", ()
    content = path.read_text(encoding="utf8", errors="replace")
    for match in re.finditer(r"/\*(.*?)\*/", content, flags=re.DOTALL):
        block = match.group(1)
        if "Copyright" in block or "SPDX-License" in block:
            continue
        lines = [re.sub(r"^\s*\*\s?", "", line).strip() for line in block.splitlines()]
        while lines and not lines[0]:
            lines.pop(0)
        while lines and not lines[-1]:
            lines.pop()
        if not lines:
            continue
        first = lines[0]
        if " - " in first:
            first = first.split(" - ", 1)[1]

        paragraphs: list[str] = []
        current: list[str] = []
        in_editorial = False
        skip_labels = ("Scenario", "Style", "Build", "Run", "Smoke", "DVZR", "Video")
        metadata_labels = ("Data", "Source", "Terms", "Prepare", "Promote", "Options", "Debug")

        def flush_current() -> None:
            nonlocal current
            if current:
                paragraphs.append(format_markdown_inline(normalize_summary(" ".join(current))))
                current = []

        for raw in lines[1:]:
            if not raw:
                flush_current()
                continue
            editorial = re.match(r"^(What to look for|What this shows):\s*(.*)$", raw, re.IGNORECASE)
            if editorial:
                flush_current()
                in_editorial = True
                first_line = editorial.group(2).strip()
                if first_line:
                    current.append(first_line)
                continue
            if re.match(rf"^({'|'.join(skip_labels)}):\s", raw):
                flush_current()
                continue
            if re.match(rf"^({'|'.join(metadata_labels)}):\s", raw):
                flush_current()
                current.append(raw)
                continue
            if not in_editorial:
                continue
            current.append(raw)
        flush_current()

        return normalize_summary(first), tuple(paragraph for paragraph in paragraphs if paragraph)
    return "", ()


def extract_c_summary(path: Path) -> str:
    summary, _ = extract_c_description(path)
    return summary


def collect_examples(manifest: dict) -> list[Example]:
    examples: list[Example] = []
    reviewed_ids = reviewed_example_ids(manifest)
    for entry in manifest["examples"]:
        id_ = str(entry["id"])
        if reviewed_ids and id_ not in reviewed_ids:
            continue
        lane = gallery_media.lane_for_entry(entry)
        category = gallery_media.category_for_entry(entry)
        stage = str(entry.get("stage", ""))
        source = str(entry.get("source", ""))
        if lane not in PUBLIC_LANES or stage == "lab" or not source:
            continue
        inferred_source_label, inferred_source_language = infer_source_language(source)
        source_language = str(entry.get("source_language") or inferred_source_language)
        source_label = str(
            entry.get("source_label")
            or source_language_label(source_language, source)
            or inferred_source_label
        )
        extra_sources = tuple(normalize_source_tab(raw) for raw in entry.get("extra_sources") or [])
        python = entry.get("python") or {}
        python_source = python.get("source")
        if python_source is not None:
            python_source = str(python_source)
        python_status = python.get("status")
        if python_status is not None:
            python_status = str(python_status)
        tags = entry.get("tags")
        if tags is None:
            tags = entry.get("features", [])
        summary, description = extract_c_description(ROOT / source)
        example = Example(
            id=str(entry["id"]),
            legacy_id=str(entry["legacy_id"]) if entry.get("legacy_id") else None,
            title=str(entry.get("title", entry["id"])),
            category=category,
            lane=lane,
            stage=stage,
            source=source,
            validation=str(entry.get("validation", "")),
            status=status_from_entry(entry),
            tags=tuple(str(tag) for tag in tags),
            summary=summary,
            description=description,
            primary=str(
                entry.get("primary_visual")
                or entry.get("primary_feature")
                or entry.get("primary_runtime")
                or entry.get("primary_composite")
                or entry.get("id")
            ),
            data=entry.get("data") or {},
            dataset=entry.get("dataset") or {},
            encoding=entry.get("encoding") or {},
            webgpu=entry.get("webgpu") or {},
            agent_copy_safe=entry.get("agent_copy_safe"),
            source_label=source_label,
            source_language=source_language,
            extra_sources=extra_sources,
            python_source=python_source,
            python_status=python_status,
        )
        examples.append(example)
    return examples


def validate_showcase_groups(examples: list[Example]) -> None:
    expected = {example.id for example in examples if example.lane == "showcases"}
    listed: list[str] = []
    for _, group_ids in SHOWCASE_GROUPS:
        listed.extend(group_ids)

    duplicates = sorted({id_ for id_ in listed if listed.count(id_) > 1})
    if duplicates:
        raise ValueError(f"Duplicate showcase IDs in SHOWCASE_GROUPS: {', '.join(duplicates)}")

    listed_set = set(listed)
    missing = sorted(expected - listed_set)
    extra = sorted(listed_set - expected)
    if missing:
        raise ValueError(f"Missing showcase IDs in SHOWCASE_GROUPS: {', '.join(missing)}")
    if extra:
        raise ValueError(f"Unknown showcase IDs in SHOWCASE_GROUPS: {', '.join(extra)}")


def status_counts(examples: list[Example]) -> str:
    counts: dict[str, int] = {}
    for example in examples:
        counts[example.status] = counts.get(example.status, 0) + 1
    ordered = [status for status in STATUS_ORDER if status in counts]
    ordered.extend(sorted(status for status in counts if status not in ordered))
    return ", ".join(f"{counts[status]} {status}" for status in ordered)


def lane_title(lane: str) -> str:
    return {
        "visuals": "Visuals",
        "features": "Features",
        "runtime": "Runtime & Capture",
        "workflows": "Workflows",
        "composites": "Composites",
        "showcases": "Showcases",
        "advanced": "Advanced",
        "scientific": "Scientific",
    }.get(lane, lane.capitalize())


def source_url(example: Example) -> str:
    return f"{SOURCE_BASE_URL}/{example.source}"


def source_path_url(source_path: str) -> str:
    return f"{SOURCE_BASE_URL}/{source_path}"


def site_relative_url(page_path: str | Path, target: str) -> str:
    if re.match(r"^[a-z][a-z0-9+.-]*:", target):
        return target
    normalized = target.lstrip("/")
    page_parent = Path(page_path).parent.as_posix()
    if page_parent in ("", "."):
        return normalized
    return posixpath.relpath(normalized, page_parent)


def site_html_relative_url(page_path: str | Path, target: str) -> str:
    if re.match(r"^[a-z][a-z0-9+.-]*:", target):
        return target
    normalized = target.lstrip("/")
    page = Path(page_path)
    page_dir = page.with_suffix("").as_posix() if page.suffix == ".md" else page.as_posix()
    if page_dir in ("", "."):
        return normalized
    return posixpath.relpath(normalized, page_dir)


def docs_site_path(docs_dir: Path, page_path: str | Path) -> str:
    full_path = docs_dir / page_path
    try:
        return full_path.relative_to(ROOT / "docs").as_posix()
    except ValueError:
        return Path(page_path).as_posix()


def image_url(
    page_path: str | Path,
    example: Example,
    image_url_base: str,
    image_format: str = DEFAULT_IMAGE_FORMAT,
) -> str:
    target = f"{image_url_base.rstrip('/')}/{example.lane}/{example.id}.{image_format}"
    return site_relative_url(page_path, target)


def source_image_path(example: Example, image_dir: Path) -> Path:
    image = image_dir / example.lane / f"{example.id}.png"
    if image.exists() or example.legacy_id is None:
        return image
    legacy = image_dir / example.lane / f"{example.legacy_id}.png"
    return legacy if legacy.exists() else image


def html_link(href: str, label: str, code: bool = False) -> str:
    content = f"<code>{label}</code>" if code else label
    return f'<a href="{href}">{content}</a>'


def webgpu_status_label(status: str) -> str:
    return {
        "webgpu-live": "Live in browser",
        "webgpu-planned": "Planned",
        "webgpu-deferred": "Deferred",
        "native-only": "Native only",
        "browser-only": "Browser only",
    }.get(status, status or "Unclassified")


def media_block(
    page_path: str | Path,
    example: Example,
    image_dir: Path,
    image_url_base: str,
    image_format: str = DEFAULT_IMAGE_FORMAT,
) -> str:
    image = source_image_path(example, image_dir)
    if image.exists():
        return f"![{example.title}]({image_url(page_path, example, image_url_base, image_format)})"
    label = "Screenshot pending" if example.screenshot_expected else "No screenshot"
    modifier = "pending" if example.screenshot_expected else "not-required"
    return (
        f'<div class="dvz-gallery-placeholder dvz-gallery-placeholder--{modifier}" '
        f'role="img" aria-label="{label} for {example.title}">'
        f"<span>{label}</span>"
        "</div>"
    )


def render_source_tabs(example: Example) -> list[str]:
    lines = ["## Source", ""]
    tabs = [
        SourceTab(
            label=example.source_label,
            language=example.source_language,
            path=example.source,
        ),
        *example.extra_sources,
    ]
    for tab in tabs:
        lines.extend([f'=== "{tab.label}"', "", f"    ```{tab.language}"])
        lines.append(f'    --8<-- "{tab.path}"')
        lines.extend(["    ```", ""])

    if example.python_source is not None and example.python_source_path is not None:
        lines.extend(['=== "Python"', "", "    ```python"])
        lines.append(f'    --8<-- "{example.python_source}"')
        lines.extend(["    ```", ""])

    return lines


def indent_markdown(lines: list[str], spaces: int = 4) -> list[str]:
    prefix = " " * spaces
    return [f"{prefix}{line}" if line else "" for line in lines]


def render_preview(
    example: Example,
    page_path: str | Path,
    image_dir: Path,
    image_url_base: str,
    image_format: str = DEFAULT_IMAGE_FORMAT,
) -> list[str]:
    screenshot = media_block(page_path, example, image_dir, image_url_base, image_format).splitlines()
    if example.webgpu_status != "webgpu-live" or not example.webgpu_site_route:
        return ["## Preview", "", *screenshot, ""]

    route = site_html_relative_url(page_path, example.webgpu_site_route)
    live_lines = [
        '<div class="dvz-webgpu-live" markdown="1">',
        f'<iframe src="{route}" title="{example.title} WebGPU live example" '
        'loading="lazy" allow="fullscreen; webgpu"></iframe>',
        "</div>",
        "",
        f'{html_link(route, "Open the live WebGPU example")}.',
    ]
    return [
        "## Preview",
        "",
        '=== "Screenshot"',
        "",
        *indent_markdown(screenshot),
        "",
        '=== "Live WebGPU"',
        "",
        *indent_markdown(live_lines),
        "",
    ]


def render_example_explanation(example: Example) -> list[str]:
    if not example.description:
        return []

    lines = ["## What To Look For", ""]
    for paragraph in example.description:
        lines.extend([paragraph, ""])
    return lines


def append_detail_table(lines: list[str], title: str, values: dict) -> None:
    if not values:
        return
    lines.extend([f"### {title}", "", "| Field | Value |", "| --- | --- |"])
    for key, value in values.items():
        lines.append(f"| `{key}` | {format_markdown_inline(value)} |")
    lines.append("")


def render_example_details(example: Example, page_path: str | Path) -> list[str]:
    metadata = [
        f"- ID: `{example.id}`",
        f"- Category: `{example.category}`",
        f"- Lane: `{example.lane}`",
        f"- Source: [`{example.source}`]({source_url(example)})",
    ]
    if example.status != DEFAULT_STATUS:
        metadata.insert(3, f"- Status: `{example.status}`")
    visual_reference = VISUAL_REFERENCE_BY_ID.get(example.id)
    if visual_reference is not None:
        metadata.append(
            "- Reference: "
            f"[{example.title} visual family]"
            f"({site_relative_url(page_path, f'reference/visual-families/{visual_reference}.md')})"
        )
    if example.python_source is not None:
        metadata.append(
            f"- Python source: [`{example.python_source}`]({SOURCE_BASE_URL}/{example.python_source})",
        )
    for tab in example.extra_sources:
        metadata.append(
            f"- {tab.label} source: [`{tab.path}`]({source_path_url(tab.path)})",
        )
    if example.webgpu:
        metadata.append(f"- Browser support: {webgpu_status_label(example.webgpu_status)}")
        if example.webgpu_route:
            metadata.append(
                f"- WebGPU live route: "
                f"{html_link(site_html_relative_url(page_path, example.webgpu_site_route), example.webgpu_route, code=True)}"
            )
        if example.webgpu_reason:
            metadata.append(f"- Browser note: {example.webgpu_reason}")
        if example.webgpu_requirements:
            requirements = ", ".join(f"`{requirement}`" for requirement in example.webgpu_requirements)
            metadata.append(f"- Browser capability tags: {requirements}")
    if example.validation:
        metadata.append(f"- Validation: `{example.validation}`")
    detail_lines = [*metadata, ""]
    if example.tags:
        detail_lines.extend(["### Tags", "", ", ".join(f"`{tag}`" for tag in example.tags), ""])
    append_detail_table(detail_lines, "Data", example.data)
    append_detail_table(detail_lines, "Dataset", example.dataset)
    append_detail_table(detail_lines, "Encoding", example.encoding)
    return ['??? info "Example details"', "", *indent_markdown(detail_lines), ""]


def render_card(
    example: Example,
    page_path: str | Path,
    image_dir: Path,
    image_url_base: str,
    image_format: str = DEFAULT_IMAGE_FORMAT,
    show_tags: bool = True,
    show_status: bool = True,
    title_heading: bool = True,
) -> str:
    page = Path(example.page_path)
    href = page.as_posix()
    media = media_block(page_path, example, image_dir, image_url_base, image_format)
    if media.startswith("!["):
        media = f"[{media}]({href})"
    tag_line = ""
    if show_tags:
        tags = ", ".join(f"`{tag}`" for tag in example.tags[:5])
        if len(example.tags) > 5:
            tags += ", ..."
        tag_line = f"<br><span>{tags}</span>" if tags else ""
    status = f"`{example.status}` " if show_status and example.status != DEFAULT_STATUS else ""
    meta_line = f"{status}`{example.lane}`{tag_line}" if show_status else tag_line
    meta_block = f"\n{meta_line}\n" if meta_line else ""
    title = (
        f"### [{example.title}]({href})"
        if title_heading
        else f"**[{example.title}]({href})**"
    )
    return f"""\
<div class="card" markdown="1">

{title}

{media}
{meta_block}
{example.summary}

</div>
"""


def lane_overview(lane: str) -> tuple[str, str]:
    return {
        "start": ("Examples", "index.md"),
        "visuals": ("Visuals & Composites", "visuals.md"),
        "features": ("Features", "features.md"),
        "runtime": ("Runtime & Capture", "runtime.md"),
        "composites": ("Visuals & Composites", "visuals.md"),
        "showcases": ("Showcases", "showcases.md"),
        "advanced": ("Advanced Examples", "advanced.md"),
    }.get(lane, ("Examples", "index.md"))


def ordered_lane_examples(examples: list[Example], lane: str) -> list[Example]:
    lane_examples = [example for example in examples if example.lane == lane]
    by_id = {example.id: example for example in examples}
    if lane == "features":
        ordered: list[Example] = []
        seen: set[str] = set()
        for _, group_ids in FEATURE_PAGE_GROUPS:
            for id_ in group_ids:
                example = by_id.get(id_)
                if example is not None and example.lane == lane:
                    ordered.append(example)
                    seen.add(id_)
        ordered.extend(
            sorted((e for e in lane_examples if e.id not in seen), key=lambda e: e.title.lower())
        )
        return ordered
    if lane == "runtime":
        ordered = []
        seen: set[str] = set()
        for _, group_ids in RUNTIME_PAGE_GROUPS:
            for id_ in group_ids:
                example = by_id.get(id_)
                if example is not None and example.lane == lane:
                    ordered.append(example)
                    seen.add(id_)
        ordered.extend(
            sorted((e for e in lane_examples if e.id not in seen), key=lambda e: e.title.lower())
        )
        return ordered
    if lane in ("visuals", "composites"):
        group_ids = [id_ for _, ids in INDEX_VISUAL_GROUPS for id_ in ids]
        ordered = [by_id[id_] for id_ in group_ids if id_ in by_id and by_id[id_].lane == lane]
        seen = {example.id for example in ordered}
        ordered.extend(
            sorted((e for e in lane_examples if e.id not in seen), key=lambda e: e.title.lower())
        )
        return ordered
    if lane == "showcases":
        return semantic_sort(lane_examples, SHOWCASE_ORDER)
    return sorted(lane_examples, key=lambda e: e.title.lower())


def example_neighbors(examples: list[Example]) -> dict[str, tuple[Example | None, Example | None]]:
    neighbors: dict[str, tuple[Example | None, Example | None]] = {}
    for lane in PUBLIC_LANES:
        ordered = ordered_lane_examples(examples, lane)
        for i, example in enumerate(ordered):
            previous = ordered[i - 1] if i > 0 else None
            next_ = ordered[i + 1] if i + 1 < len(ordered) else None
            neighbors[example.id] = (previous, next_)
    return neighbors


def render_example_nav(
    example: Example,
    page_path: str | Path,
    previous: Example | None,
    next_: Example | None,
    location: str = "top",
) -> list[str]:
    overview_label, overview_path = lane_overview(example.lane)
    overview_site_path = "examples/" if overview_path == "index.md" else f"examples/{overview_path[:-3]}/"
    examples_href = site_html_relative_url(page_path, "examples/")
    overview_href = site_html_relative_url(page_path, overview_site_path)
    previous_href = site_html_relative_url(page_path, f"examples/{previous.page_path[:-3]}/") if previous else ""
    next_href = site_html_relative_url(page_path, f"examples/{next_.page_path[:-3]}/") if next_ else ""
    if location == "top":
        return [
            '<nav class="dvz-example-breadcrumbs" aria-label="Breadcrumbs">',
            f'<a href="{examples_href}">Examples</a>',
            '<span>/</span>',
            f'<a href="{overview_href}">{overview_label}</a>',
            '<span>/</span>',
            f'<span>{example.title}</span>',
            "</nav>",
            "",
        ]

    previous_link = (
        f'<a href="{previous_href}">Previous: {previous.title}</a>'
        if previous
        else ""
    )
    next_link = (
        f'<a href="{next_href}">Next: {next_.title}</a>'
        if next_
        else ""
    )
    sibling_links = [link for link in (previous_link, next_link) if link]
    return [
        f'<nav class="dvz-example-nav dvz-example-nav--{location}" aria-label="Example navigation">',
        '<div class="dvz-example-nav__trail">',
        f'<a href="{examples_href}">Examples</a>',
        '<span>/</span>',
        f'<a href="{overview_href}">{overview_label}</a>',
        "</div>",
        '<div class="dvz-example-nav__siblings">',
        " · ".join(sibling_links),
        "</div>",
        "</nav>",
        "",
    ]


def semantic_sort(examples: list[Example], order: tuple[str, ...]) -> list[Example]:
    """Sort examples by a fixed ID order; unrecognised IDs fall to the end."""
    rank = {id_: i for i, id_ in enumerate(order)}
    return sorted(examples, key=lambda e: rank.get(e.id, len(order)))


def write_text(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text.rstrip() + "\n", encoding="utf8")


def generated_header(title: str) -> list[str]:
    return [
        f"# {title}",
        "",
        "<!-- WARNING: generated by tools/build_gallery.py from examples/c/MANIFEST.yaml -->",
        "",
    ]


def render_page_intro(summary: str, coverage: str) -> list[str]:
    return [
        summary,
        "",
        coverage,
        "",
        "Each card links to a detail page with preview media, source code, and example metadata.",
        "",
    ]


def render_lane_section(
    examples: list[Example],
    lane: str,
    page_path: str | Path,
    image_dir: Path,
    image_url_base: str,
    image_format: str = DEFAULT_IMAGE_FORMAT,
) -> list[str]:
    lane_examples = [example for example in examples if example.lane == lane]
    if not lane_examples:
        return []
    lines = [f"## {lane_title(lane)}", ""]
    lines.append('<div class="grid cards" markdown="1">')
    lines.append("")
    for example in sorted(lane_examples, key=lambda item: item.title.lower()):
        lines.append(render_card(example, page_path, image_dir, image_url_base, image_format))
    lines.append("</div>")
    lines.append("")
    return lines


def render_gallery_page(
    filename: str,
    config: dict,
    examples: list[Example],
    docs_dir: Path,
    image_dir: Path,
    image_url_base: str,
    image_format: str = DEFAULT_IMAGE_FORMAT,
) -> None:
    page_examples = [example for example in examples if example.lane in config["lanes"]]
    lines = generated_header(config["title"])
    lines.extend(
        render_page_intro(
            dedent(config["intro"]).strip(),
            f"Coverage: {len(page_examples)} examples ({status_counts(page_examples)}).",
        )
    )
    page_path = docs_site_path(docs_dir, filename)
    for lane in config["lanes"]:
        lines.extend(
            render_lane_section(page_examples, lane, page_path, image_dir, image_url_base, image_format)
        )
    write_text(docs_dir / filename, "\n".join(lines))


def render_showcases_page(
    examples: list[Example],
    docs_dir: Path,
    image_dir: Path,
    image_url_base: str,
    image_format: str = DEFAULT_IMAGE_FORMAT,
) -> None:
    showcase_examples = [example for example in examples if example.lane == "showcases"]
    by_id = {example.id: example for example in showcase_examples}
    page_path = docs_site_path(docs_dir, "showcases.md")
    lines = generated_header("Showcases")
    lines.extend(
        render_page_intro(
            "Browse composed scenes demonstrating scientific workflows, real data, and polished demos.",
            f"Coverage: {len(showcase_examples)} examples ({status_counts(showcase_examples)}).",
        )
    )
    for group_label, group_ids in SHOWCASE_GROUPS:
        group_examples = [by_id[id_] for id_ in group_ids if id_ in by_id]
        if not group_examples:
            continue
        lines.extend([f"## {group_label}", ""])
        lines.append('<div class="grid cards" markdown="1">')
        lines.append("")
        for example in group_examples:
            lines.append(render_card(example, page_path, image_dir, image_url_base, image_format))
        lines.append("</div>")
        lines.append("")
    write_text(docs_dir / "showcases.md", "\n".join(lines))


def render_index(
    examples: list[Example],
    docs_dir: Path,
    image_dir: Path = DEFAULT_IMAGE_DIR,
    image_url_base: str = DEFAULT_IMAGE_URL_BASE,
    image_format: str = DEFAULT_IMAGE_FORMAT,
) -> None:
    by_lane = {lane: [e for e in examples if e.lane == lane] for lane in PUBLIC_LANES}
    showcase_examples = semantic_sort(by_lane["showcases"], SHOWCASE_ORDER)
    visual_examples = by_lane["visuals"]
    composite_examples = by_lane["composites"]
    feature_examples = by_lane["features"]
    runtime_examples = by_lane["runtime"]
    by_id = {e.id: e for e in examples}
    index_path = docs_dir / "index.md"
    page_path = docs_site_path(docs_dir, "index.md")

    lines = generated_header("Examples")
    lines.extend(
        render_page_intro(
            "Browse the generated Datoviz v0.4 example gallery.",
            "Coverage: "
            f"{len(showcase_examples)} showcases, "
            f"{len(visual_examples) + len(composite_examples)} visuals and composites, "
            f"{len(feature_examples)} feature examples, "
            f"{len(runtime_examples)} runtime examples, "
            f"and {len(by_lane['advanced'])} advanced examples.",
        )
    )

    # --- Showcases ---
    lines.extend(["## Showcases", ""])
    lines.extend(
        [
            "Selected composed examples are shown below.",
            "",
        ]
    )
    lines.append('<div class="grid cards" markdown="1">')
    lines.append("")
    for example in showcase_examples:
        lines.append(
            render_card(
                example,
                page_path,
                image_dir,
                image_url_base,
                image_format,
                show_tags=False,
                title_heading=False,
            )
        )
    lines.append("</div>")
    lines.extend(["", f"[Browse all {len(showcase_examples)} showcases](showcases.md).", ""])

    # --- Visuals & Composites ---
    n_vc = len(visual_examples) + len(composite_examples)
    lines.extend(["## Visuals & Composites", ""])
    lines.extend(["Selected visual and composite examples are shown below.", ""])
    for group_label, group_ids in INDEX_VISUAL_GROUPS:
        group_examples = [by_id[id_] for id_ in group_ids if id_ in by_id]
        if not group_examples:
            continue
        lines.extend([f"### {group_label}", ""])
        lines.append('<div class="grid cards" markdown="1">')
        lines.append("")
        for example in group_examples:
            lines.append(
                render_card(
                    example,
                    page_path,
                    image_dir,
                    image_url_base,
                    image_format,
                    show_tags=False,
                    title_heading=False,
                )
            )
        lines.append("</div>")
        lines.append("")

    lines.extend([f"[Browse all {n_vc} visuals and composites](visuals.md).", ""])

    # --- Features ---
    all_flagship_ids = {id_ for _, ids in INDEX_FEATURE_GROUPS for id_ in ids}
    lines.extend(["## Features", ""])
    lines.extend(["Selected isolated feature examples are shown below.", ""])
    for group_label, group_ids in INDEX_FEATURE_GROUPS:
        group_examples = [by_id[id_] for id_ in group_ids if id_ in by_id]
        if not group_examples:
            continue
        lines.extend([f"### {group_label}", ""])
        lines.append('<div class="grid cards" markdown="1">')
        lines.append("")
        for example in group_examples:
            lines.append(
                render_card(
                    example,
                    page_path,
                    image_dir,
                    image_url_base,
                    image_format,
                    show_tags=False,
                    title_heading=False,
                )
            )
        lines.append("</div>")
        lines.append("")
    lines.extend(
        [
            f"[Browse all {len(feature_examples)} feature examples](features.md) — "
            "controllers, adornments, interaction, animation, rendering, and more.",
            "",
        ]
    )

    # --- Runtime & Capture ---
    lines.extend(["## Runtime & Capture", ""])
    lines.extend(
        [
            "Selected windowing, capture, recording, and export examples are shown below.",
            "",
        ]
    )
    for group_label, group_ids in RUNTIME_PAGE_GROUPS:
        group_examples = [by_id[id_] for id_ in group_ids if id_ in by_id]
        if not group_examples:
            continue
        lines.extend([f"### {group_label}", ""])
        lines.append('<div class="grid cards" markdown="1">')
        lines.append("")
        for example in group_examples:
            lines.append(
                render_card(
                    example,
                    page_path,
                    image_dir,
                    image_url_base,
                    image_format,
                    show_tags=False,
                    title_heading=False,
                )
            )
        lines.append("</div>")
        lines.append("")
    lines.extend([f"[Browse all {len(runtime_examples)} runtime examples](runtime.md).", ""])

    write_text(index_path, "\n".join(lines))


def render_visuals_page(
    examples: list[Example],
    docs_dir: Path,
    image_dir: Path,
    image_url_base: str,
    image_format: str = DEFAULT_IMAGE_FORMAT,
) -> None:
    visual_examples = [e for e in examples if e.lane == "visuals"]
    composite_examples = [e for e in examples if e.lane == "composites"]
    page_path = docs_site_path(docs_dir, "visuals.md")
    lines = generated_header("Visuals & Composites")
    lines.extend(
        render_page_intro(
            "Browse one focused example per visual family or composite.",
            f"Coverage: {len(visual_examples)} visual families and {len(composite_examples)} composites.",
        )
    )
    visual_groups = (
        ("0D — point-like", ["visuals_point", "visuals_pixel", "visuals_marker", "visuals_splat"]),
        ("1D — line-like",  ["visuals_segment", "visuals_path", "visuals_vector", "visuals_primitive"]),
        ("2D — planar",     ["visuals_image", "visuals_image_rgba", "visuals_text", "visuals_glyph", "visuals_labels"]),
        ("3D — volumetric", ["visuals_mesh", "visuals_sphere", "visuals_volume"]),
        ("Composites",      ["composites_polygon", "composites_graph"]),
    )
    by_id = {e.id: e for e in examples}
    for group_label, group_ids in visual_groups:
        group_examples = [by_id[i] for i in group_ids if i in by_id]
        if not group_examples:
            continue
        lines.extend([f"## {group_label}", ""])
        lines.append('<div class="grid cards" markdown="1">')
        lines.append("")
        for example in group_examples:
            lines.append(render_card(example, page_path, image_dir, image_url_base, image_format))
        lines.append("</div>")
        lines.append("")
    write_text(docs_dir / "visuals.md", "\n".join(lines))


def render_features_page(
    examples: list[Example],
    docs_dir: Path,
    image_dir: Path,
    image_url_base: str,
    image_format: str = DEFAULT_IMAGE_FORMAT,
) -> None:
    feature_examples = [e for e in examples if e.lane == "features"]
    by_id = {e.id: e for e in examples}
    page_path = docs_site_path(docs_dir, "features.md")
    lines = generated_header("Features")
    lines.extend(
        render_page_intro(
            "Browse isolated examples for layout, navigation, adornments, rendering, interaction, animation, and diagnostics.",
            f"Coverage: {len(feature_examples)} feature examples ({status_counts(feature_examples)}).",
        )
    )
    grouped_ids = {id_ for _, ids in FEATURE_PAGE_GROUPS for id_ in ids}
    for group_label, group_ids in FEATURE_PAGE_GROUPS:
        group_examples = [by_id[i] for i in group_ids if i in by_id]
        if not group_examples:
            continue
        lines.extend([f"## {group_label}", ""])
        lines.append('<div class="grid cards" markdown="1">')
        lines.append("")
        for example in group_examples:
            lines.append(
                render_card(
                    example,
                    page_path,
                    image_dir,
                    image_url_base,
                    image_format,
                )
            )
        lines.append("</div>")
        lines.append("")
    ungrouped = [e for e in feature_examples if e.id not in grouped_ids]
    if ungrouped:
        lines.extend(["## Other", ""])
        lines.append('<div class="grid cards" markdown="1">')
        lines.append("")
        for example in sorted(ungrouped, key=lambda e: e.title.lower()):
            lines.append(
                render_card(
                    example,
                    page_path,
                    image_dir,
                    image_url_base,
                    image_format,
                )
            )
        lines.append("</div>")
        lines.append("")
    write_text(docs_dir / "features.md", "\n".join(lines))


def render_runtime_page(
    examples: list[Example],
    docs_dir: Path,
    image_dir: Path,
    image_url_base: str,
    image_format: str = DEFAULT_IMAGE_FORMAT,
) -> None:
    runtime_examples = [e for e in examples if e.lane == "runtime"]
    by_id = {e.id: e for e in examples}
    page_path = docs_site_path(docs_dir, "runtime.md")
    lines = generated_header("Runtime & Capture")
    lines.extend(
        render_page_intro(
            "Browse examples for opening windows, rendering offscreen, recording, replaying, and exporting media.",
            f"Coverage: {len(runtime_examples)} runtime examples ({status_counts(runtime_examples)}).",
        )
    )
    grouped_ids = {id_ for _, ids in RUNTIME_PAGE_GROUPS for id_ in ids}
    for group_label, group_ids in RUNTIME_PAGE_GROUPS:
        group_examples = [by_id[i] for i in group_ids if i in by_id]
        if not group_examples:
            continue
        lines.extend([f"## {group_label}", ""])
        lines.append('<div class="grid cards" markdown="1">')
        lines.append("")
        for example in group_examples:
            lines.append(render_card(example, page_path, image_dir, image_url_base, image_format))
        lines.append("</div>")
        lines.append("")
    ungrouped = [e for e in runtime_examples if e.id not in grouped_ids]
    if ungrouped:
        lines.extend(["## Other", ""])
        lines.append('<div class="grid cards" markdown="1">')
        lines.append("")
        for example in sorted(ungrouped, key=lambda e: e.title.lower()):
            lines.append(render_card(example, page_path, image_dir, image_url_base, image_format))
        lines.append("</div>")
        lines.append("")
    write_text(docs_dir / "runtime.md", "\n".join(lines))


def render_validation(examples: list[Example], docs_dir: Path) -> None:
    screenshot_examples = [example for example in examples if "screenshot" in example.validation]
    video_examples = [
        example
        for example in examples
        if "video" in example.validation or "animation" in example.id or "gpu_particle" in example.id
    ]
    lines = generated_header("Validation Gallery")
    lines.extend(
        dedent(
            """\
            This page tracks release evidence for gallery publication. It intentionally separates
            executable source coverage from media artifacts, because screenshots and clips are
            generated in Vulkan-capable environments and should not be committed accidentally.
            """
        )
        .strip()
        .splitlines()
    )
    lines.extend(
        [
            "",
            "## Evidence Counts",
            "",
            f"- Reviewed public C examples generated for the website: {len(examples)}",
            f"- Examples declaring screenshot validation: {len(screenshot_examples)}",
            f"- Examples that should have video or motion evidence: {len(video_examples)}",
            "",
            "## Commands",
            "",
            "```sh",
            "just build",
            "just gallery",
            "python3 tools/run_c_examples.py --list",
            "git diff --check",
            "```",
            "",
            "Screenshot and video capture should be run separately from documentation generation.",
            "Gallery screenshots are generated into the `data` submodule under `data/gallery/v0.4`",
            "and should be committed through that submodule, not copied into the main repository.",
            "",
            "## Screenshot Queue",
            "",
            "| Example | Source | Status | Validation |",
            "| --- | --- | --- | --- |",
        ]
    )
    for example in screenshot_examples:
        lines.append(
            f"| [{example.title}]({example.page_path}) | "
            f"[`{example.source}`]({source_url(example)}) | `{example.status}` | "
            f"`{example.validation}` |"
        )
    write_text(docs_dir / "validation-gallery.md", "\n".join(lines))


def render_webgpu_matrix(examples: list[Example], docs_dir: Path) -> None:
    classified = [example for example in examples if example.webgpu]
    status_order = {
        "webgpu-live": 0,
        "webgpu-planned": 1,
        "webgpu-deferred": 2,
        "native-only": 3,
        "browser-only": 4,
    }
    classified.sort(key=lambda example: (status_order.get(example.webgpu_status, 99), example.id))

    lines = generated_header("WebGPU Live Example Matrix")
    lines.extend(
        dedent(
            """\
            This page is generated from `examples/c/MANIFEST.yaml`. "Live in browser" rows are
            public browser routes backed by the same canonical C example or portable C scenario as
            native validation. Other rows explain whether browser support is planned, deferred, or
            native-only.
            """
        )
        .strip()
        .splitlines()
    )
    lines.extend(
        [
            "",
            "| Example | Source | Browser support | Notes | Capability tags | Browser route |",
            "| --- | --- | --- | --- | --- | --- |",
        ]
    )
    page_path = docs_site_path(docs_dir, "webgpu-matrix.md")
    for example in classified:
        requirements = ", ".join(f"`{requirement}`" for requirement in example.webgpu_requirements)
        route = (
            html_link(
                site_html_relative_url(page_path, example.webgpu_site_route),
                example.webgpu_route,
                code=True,
            )
            if example.webgpu_route
            else "Not available"
        )
        note = example.webgpu_reason or ""
        lines.append(
            f"| [{example.title}]({example.page_path}) | "
            f"[`{example.source}`]({source_url(example)}) | "
            f"{webgpu_status_label(example.webgpu_status)} | {note} | {requirements} | {route} |"
        )
    write_text(docs_dir / "webgpu-matrix.md", "\n".join(lines))


def render_example_page(
    example: Example,
    previous: Example | None,
    next_: Example | None,
    docs_dir: Path,
    image_dir: Path,
    image_url_base: str,
    image_format: str = DEFAULT_IMAGE_FORMAT,
) -> None:
    page_path = docs_site_path(docs_dir, example.page_path)
    lines = generated_header(example.title)
    lines.extend(render_example_nav(example, page_path, previous, next_))
    lines.extend([example.summary, ""])
    lines.extend(render_preview(example, page_path, image_dir, image_url_base, image_format))
    lines.extend(render_example_explanation(example))
    lines.extend(render_source_tabs(example))
    lines.extend(render_example_details(example, page_path))
    lines.extend(render_example_nav(example, page_path, previous, next_, location="bottom"))
    write_text(docs_dir / example.page_path, "\n".join(lines))


def clean_generated_pages(docs_dir: Path) -> None:
    generated = docs_dir / "gallery"
    if generated.exists():
        shutil.rmtree(generated)


def main() -> int:
    args = parse_args()
    manifest = load_manifest(args.manifest)
    examples = collect_examples(manifest)
    validate_showcase_groups(examples)
    clean_generated_pages(args.docs_dir)
    render_index(examples, args.docs_dir, args.image_dir, args.image_url_base, args.image_format)
    render_showcases_page(examples, args.docs_dir, args.image_dir, args.image_url_base, args.image_format)
    for filename, config in PAGE_CONFIG.items():
        render_gallery_page(
            filename,
            config,
            examples,
            args.docs_dir,
            args.image_dir,
            args.image_url_base,
            args.image_format,
        )
    render_visuals_page(examples, args.docs_dir, args.image_dir, args.image_url_base, args.image_format)
    render_features_page(examples, args.docs_dir, args.image_dir, args.image_url_base, args.image_format)
    render_runtime_page(examples, args.docs_dir, args.image_dir, args.image_url_base, args.image_format)
    render_validation(examples, args.docs_dir)
    render_webgpu_matrix(examples, args.docs_dir)
    neighbors = example_neighbors(examples)
    for example in examples:
        previous, next_ = neighbors.get(example.id, (None, None))
        render_example_page(
            example,
            previous,
            next_,
            args.docs_dir,
            args.image_dir,
            args.image_url_base,
            args.image_format,
        )
    print(f"Generated {len(examples)} C gallery entries under {args.docs_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
