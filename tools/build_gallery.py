#!/usr/bin/env python3
"""Generate the v0.4 public example gallery from the C example manifest."""

from __future__ import annotations

import argparse
import html
import posixpath
import re
import shutil
from dataclasses import dataclass
from pathlib import Path
from textwrap import dedent

import gallery_media
from example_navigation import load_navigation, navigation_anchor, validate_navigation


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

EXAMPLE_NAVIGATION = load_navigation()


def navigation_groups(section_id: str, index: bool = False) -> tuple[tuple[str, list[str]], ...]:
    section = EXAMPLE_NAVIGATION.section(section_id)
    groups = section.index if index else section.groups
    return tuple((group.title, list(group.example_ids)) for group in groups)


SHOWCASE_GROUPS = navigation_groups("showcases")
SHOWCASE_ORDER = EXAMPLE_NAVIGATION.section("showcases").ordered_ids
INDEX_VISUAL_GROUPS = navigation_groups("visuals", index=True)

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
    "visuals_volume": "volume",
}

FEATURE_PAGE_GROUPS = navigation_groups("features")
RUNTIME_PAGE_GROUPS = navigation_groups("runtime")
INDEX_FEATURE_GROUPS = navigation_groups("features", index=True)
INDEX_ADVANCED_GROUPS = navigation_groups("advanced", index=True)

INDEX_HIGHLIGHT_SECTIONS = (
    (
        "showcases",
        "Showcases",
        "Polished, composed demonstrations built around scientific data, simulations, and "
        "complete visualization workflows.",
        "showcase",
    ),
    (
        "visuals",
        "Visuals",
        "Focused examples of the marks Datoviz can draw, from paths and images to meshes.",
        "visual and composite",
    ),
    (
        "features",
        "Features",
        "Focused capabilities for layout, navigation, adornments, geometry, rendering, and "
        "interaction.",
        "feature",
    ),
)

CATEGORY_TO_LANE = gallery_media.CATEGORY_TO_LANE
LANE_TO_CATEGORY = gallery_media.LANE_TO_CATEGORY

PAGE_INTROS = {
    "advanced": (
        "Browse advanced runtime and host-integration examples. "
        "These are useful after you are comfortable with ordinary scene code."
    ),
    "runtime": (
        "Browse examples for opening windows, rendering offscreen, recording, replaying, "
        "and exporting media."
    ),
}
PAGE_CONFIG = {
    EXAMPLE_NAVIGATION.section(id_).overview: {
        "title": EXAMPLE_NAVIGATION.section(id_).page_title,
        "lanes": EXAMPLE_NAVIGATION.section(id_).lanes,
        "intro": intro,
    }
    for id_, intro in PAGE_INTROS.items()
}



@dataclass(frozen=True)
class SourceTab:
    label: str
    language: str
    path: str


@dataclass(frozen=True)
class Example:
    id: str
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
    media: dict
    webgpu: dict
    agent_copy_safe: bool | None
    source_label: str
    source_language: str
    extra_sources: tuple[SourceTab, ...]
    python_source: str | None
    python_status: str | None
    build_command: str | None
    docs_page: str | None

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
        if self.docs_page is not None:
            return self.docs_page
        return f"gallery/{self.lane}/{self.id}.md"

    @property
    def has_detail_page(self) -> bool:
        return self.docs_page is None

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
    def webgpu_local_route(self) -> str:
        return str(self.webgpu.get("local_route", ""))

    @property
    def webgpu_local_site_route(self) -> str:
        if not self.webgpu_local_route:
            return ""
        return f"/{self.webgpu_local_route}"

    @property
    def webgpu_requirements(self) -> tuple[str, ...]:
        requirements = self.webgpu.get("requirements") or ()
        return tuple(str(requirement) for requirement in requirements)

    @property
    def webgpu_rendering_effects(self) -> tuple[dict[str, str], ...]:
        effects = self.webgpu.get("rendering_effects") or ()
        return tuple(
            {
                "effect": str(effect.get("effect", "")),
                "status": str(effect.get("status", "")),
                "warning": str(effect.get("warning", "")),
            }
            for effect in effects
        )

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
        if not summary.startswith("This example "):
            raise ValueError(
                f"public example summary must start with 'This example ': {source}: {summary!r}"
            )
        example = Example(
            id=str(entry["id"]),
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
            media=entry.get("media") or {},
            webgpu=entry.get("webgpu") or {},
            agent_copy_safe=entry.get("agent_copy_safe"),
            source_label=source_label,
            source_language=source_language,
            extra_sources=extra_sources,
            python_source=python_source,
            python_status=python_status,
            build_command=str(entry["build_command"]) if entry.get("build_command") else None,
            docs_page=str(entry["docs_page"]) if entry.get("docs_page") else None,
        )
        examples.append(example)
    return examples


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
    if page.name == "index.md":
        page_dir = page.parent.as_posix()
    else:
        page_dir = page.with_suffix("").as_posix() if page.suffix == ".md" else page.as_posix()
    if page_dir in ("", "."):
        return normalized
    return posixpath.relpath(normalized, page_dir)


def docs_site_path(_docs_dir: Path, page_path: str | Path) -> str:
    return (Path("examples") / page_path).as_posix()


def image_url(
    page_path: str | Path,
    example: Example,
    image_url_base: str,
    image_format: str = DEFAULT_IMAGE_FORMAT,
) -> str:
    target = f"{image_url_base.rstrip('/')}/{example.lane}/{example.id}.{image_format}"
    return site_relative_url(page_path, target)


def asset_url(
    page_path: str | Path,
    example: Example,
    image_url_base: str,
    suffix: str,
) -> str:
    target = f"{image_url_base.rstrip('/')}/{example.lane}/{example.id}{suffix}"
    return site_html_relative_url(page_path, target)


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


def python_status_label(status: str | None) -> str:
    return {
        "manual": "Available; manually maintained direct-engine example",
        "direct-engine": "Available; direct-engine adaptation",
        "generated": "Available; generated adaptation",
        "generated-with-hints": "Available; generated adaptation with authored hints",
        "deferred": "Deferred",
    }.get(status or "", "Available" if status is None else status)


def python_module_name(path: str) -> str:
    source = Path(path)
    assert source.suffix == ".py"
    return ".".join(source.with_suffix("").parts)


def preferred_preview_media(example: Example) -> str:
    preview = example.media.get("preview") if isinstance(example.media, dict) else {}
    if not isinstance(preview, dict):
        return ""
    card = preview.get("card") or {}
    if isinstance(card, dict) and card.get("preferred"):
        return str(card["preferred"])
    return str(preview.get("kind", ""))


def video_media_block(
    page_path: str | Path,
    example: Example,
    image_url_base: str,
    href: str = "",
) -> str:
    title = html.escape(example.title, quote=True)
    poster = html.escape(asset_url(page_path, example, image_url_base, ".poster.webp"), quote=True)
    mp4 = html.escape(asset_url(page_path, example, image_url_base, ".mp4"), quote=True)
    link = ""
    if href:
        link = (
            f'  <a class="dvz-gallery-media-target" href="{html.escape(href, quote=True)}" '
            f'aria-label="{title}"></a>'
        )
    lines = [
        '<div class="dvz-gallery-media dvz-gallery-media--video" data-gallery-lazy="video">',
    ]
    if link:
        lines.append(link)
    lines.extend(
        [
            f'  <img class="dvz-gallery-poster" src="{poster}" alt="{title}" loading="lazy">',
            "  <video class=\"dvz-gallery-video\" muted loop playsinline preload=\"none\"",
            f'         poster="{poster}" aria-label="{title} preview">',
            f'    <source data-src="{mp4}" type="video/mp4">',
            "  </video>",
            "</div>",
        ]
    )
    return "\n".join(lines)


def media_block(
    page_path: str | Path,
    example: Example,
    _image_dir: Path,
    image_url_base: str,
    image_format: str = DEFAULT_IMAGE_FORMAT,
    href: str = "",
) -> str:
    # Documentation generation is hermetic: the manifest declares whether reviewed media is part
    # of the public gallery contract, while capture and site-publication checks validate bytes.
    if example.screenshot_expected:
        if preferred_preview_media(example) == "video-mp4":
            return video_media_block(page_path, example, image_url_base, href=href)
        markdown = f"![{example.title}]({image_url(page_path, example, image_url_base, image_format)})"
        return f"[{markdown}]({href})" if href else markdown
    label = "No screenshot"
    modifier = "not-required"
    return (
        f'<div class="dvz-gallery-placeholder dvz-gallery-placeholder--{modifier}" '
        f'role="img" aria-label="{label} for {example.title}">'
        f"<span>{label}</span>"
        "</div>"
    )


def render_source_tabs(example: Example) -> list[str]:
    lines = ["## Source", ""]
    tabs: list[SourceTab] = []
    if example.python_source is not None and example.python_source_path is not None:
        tabs.append(SourceTab(label="Python", language="python", path=example.python_source))
    tabs.extend([
        SourceTab(
            label=example.source_label,
            language=example.source_language,
            path=example.source,
        ),
        *example.extra_sources,
    ])
    for tab in tabs:
        lines.extend([f'=== "{tab.label}"', "", f"    ```{tab.language}"])
        lines.append(f'    --8<-- "{tab.path}"')
        lines.extend(["    ```", ""])

    return lines


def render_run_and_adapt(example: Example, page_path: str | Path) -> list[str]:
    if example.source.startswith("examples/c/"):
        executable_name = example.rel_executable
        run_command = f"`just example-c {executable_name}` (build and run)"
        executable = f"./build/examples/c/{executable_name}"
        native_action = f"{run_command}, or rerun `{executable}`"
    else:
        build_command = example.build_command or "just build"
        executable = f"./build/{Path(example.source).with_suffix('').as_posix()}"
        native_action = f"`{build_command}`, then `{executable}`"
    lines = [
        "## Run And Adapt",
        "",
        "Commands below assume a Datoviz source checkout and start at the repository root.",
        "Use your configured build environment; Python routes additionally require local bindings.",
        "",
        "| Route | Availability | Command or action |",
        "| --- | --- | --- |",
        f"| {example.source_label} | Canonical native source | {native_action} |",
    ]
    if example.python_source is not None:
        lines.append(
            f"| Python | {python_status_label(example.python_status)} | "
            f"`python3 -m {python_module_name(example.python_source)}` |"
        )
    elif not any(tab.language == "python" for tab in example.extra_sources):
        lines.append("| Python | No verified adaptation on this page | Start from the C source. |")
    for tab in example.extra_sources:
        if tab.language == "python":
            lines.append(
                f"| {tab.label} | Additional integration source; check optional dependencies | "
                f"`python3 -m {python_module_name(tab.path)}` |"
            )
    if example.webgpu_status == "webgpu-live" and example.webgpu_site_route:
        route = site_html_relative_url(page_path, example.webgpu_site_route)
        lines.append(f"| Browser | Live WebGPU route | {html_link(route, 'Open live example')} |")
    else:
        reason = example.webgpu_reason or "Use the native route for this example."
        local_action = ""
        if example.webgpu_local_site_route:
            local_route = site_html_relative_url(page_path, example.webgpu_local_site_route)
            local_action = (
                ' <span class="dvz-local-webgpu-action" hidden>'
                f"Local development: {html_link(local_route, 'Open WebGPU example')}."
                "</span>"
            )
        lines.append(
            f"| Browser | {webgpu_status_label(example.webgpu_status)} | "
            f"{format_markdown_inline(reason)}{local_action} |"
        )
    lines.append("")

    data_kind = str(example.data.get("kind", ""))
    prepared_source = str(
        example.dataset.get("prepared_source")
        or example.dataset.get("promoted_prepared_path")
        or example.dataset.get("cache_prepared_path")
        or example.dataset.get("fallback_prepared_path")
        or ""
    )
    preprocessing = str(example.dataset.get("preprocessing", ""))
    preprocessing_required = bool(preprocessing) and not preprocessing.lower().startswith("none")
    requires_preparation = bool(
        preprocessing_required or prepared_source or data_kind == "prepared"
    )
    if requires_preparation:
        lines.extend(
            [
                '!!! warning "Prepared data required"',
                "",
                "    This example intentionally fails when its prepared input is absent; it does not",
                "    substitute synthetic data.",
            ]
        )
        if prepared_source:
            lines.append(f"    Expected input: `{prepared_source}`.")
        if preprocessing_required:
            lines.append(f"    Prepare it from the repository root with `{preprocessing}`.")
        lines.append("")
    if data_kind == "real":
        lines.extend(
            [
                '!!! info "Real dataset"',
                "",
                "    Check the dataset, license, citation, and preprocessing fields in Example details",
                "    before redistributing data or derived output.",
                "",
            ]
        )

    if example.agent_copy_safe is True:
        lines.extend(
            [
                "This example is approved as a starting point for user code and coding agents. Keep the",
                "object lifetimes and data shapes intact while adapting the data and styling.",
                "",
            ]
        )
    elif example.agent_copy_safe is False:
        lines.extend(
            [
                "Use this example as capability or integration evidence, not as a minimal copy-paste",
                "template. Start from the nearest supported, copy-safe example and add this feature",
                "after verifying the linked API reference.",
                "",
            ]
        )
    return lines


def indent_markdown(lines: list[str], spaces: int = 4) -> list[str]:
    prefix = " " * spaces
    return [f"{prefix}{line}" if line else "" for line in lines]


def render_webgpu_effect_notice(example: Example) -> list[str]:
    limitations = [
        effect for effect in example.webgpu_rendering_effects if effect["status"] != "supported"
    ]
    if not limitations:
        return []

    lines = [
        '<aside class="dvz-webgpu-unavailable" role="note">',
        "<strong>WebGPU rendering difference</strong>",
    ]
    for limitation in limitations:
        label = limitation["effect"].replace("-", " ").upper()
        lines.append(
            f"<span><code>{html.escape(label)}</code>: "
            f"{html.escape(limitation['warning'])}</span>"
        )
    lines.append("</aside>")
    return lines


def render_preview(
    example: Example,
    page_path: str | Path,
    image_dir: Path,
    image_url_base: str,
    image_format: str = DEFAULT_IMAGE_FORMAT,
) -> list[str]:
    screenshot = media_block(page_path, example, image_dir, image_url_base, image_format).splitlines()
    if example.webgpu_status != "webgpu-live" or not example.webgpu_site_route:
        notice_by_status = {
            "webgpu-planned": (
                "Live WebGPU preview not available yet",
                "Browser support for this example is planned. The preview above shows the native version.",
            ),
            "webgpu-deferred": (
                "No live WebGPU preview",
                "Browser support for this example is not currently implemented. The preview above shows the native version.",
            ),
            "native-only": (
                "Native-only example",
                "This example currently runs with the native Vulkan backend only.",
            ),
        }
        notice = notice_by_status.get(example.webgpu_status)
        if notice is None:
            return ["## Preview", "", *screenshot, ""]

        title, message = notice
        support_url = site_html_relative_url(page_path, "/reference/webgpu-subset/")
        support_url = support_url.rstrip("/") + "/"
        status_lines = [
            '<aside class="dvz-webgpu-unavailable" role="note">',
            f'<strong>{title}</strong>',
            f'<span>{message} <a href="{support_url}">Learn about browser support</a>.</span>',
            "</aside>",
        ]
        local_lines = []
        if example.webgpu_local_site_route:
            local_route = site_html_relative_url(page_path, example.webgpu_local_site_route)
            local_live_lines = [
                '<div class="dvz-webgpu-live" markdown="1">',
                f'<iframe data-src="{local_route}" title="{example.title} local WebGPU example" '
                'loading="lazy" allow="fullscreen; webgpu"></iframe>',
                "</div>",
                "",
                f'{html_link(local_route, "Open the local WebGPU example")}.',
            ]
            local_lines = [
                '<div class="dvz-local-webgpu-tabs" hidden markdown="1">',
                "",
                '=== "Screenshot"',
                "",
                *indent_markdown(screenshot),
                "",
                '=== "Live WebGPU"',
                "",
                *indent_markdown(local_live_lines),
                "",
                "</div>",
            ]
        if local_lines:
            fallback_lines = [
                '<div class="dvz-public-webgpu-fallback" markdown="1">',
                "",
                *screenshot,
                "",
                *status_lines,
                "",
                "</div>",
            ]
            return ["## Preview", "", *fallback_lines, "", *local_lines, ""]
        return ["## Preview", "", *screenshot, "", *status_lines, ""]

    route = site_html_relative_url(page_path, example.webgpu_site_route)
    effect_notice = render_webgpu_effect_notice(example)
    embedded_route = f"{route}&embedded=1" if effect_notice else route
    effect_notice_lines = [*effect_notice, ""] if effect_notice else []
    live_lines = [
        '<div class="dvz-webgpu-live" markdown="1">',
        f'<iframe src="{embedded_route}" title="{example.title} WebGPU live example" '
        'loading="lazy" allow="fullscreen; webgpu"></iframe>',
        "</div>",
        "",
        *effect_notice_lines,
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
    lines.extend([f"**{title}**", "", "| Field | Value |", "| --- | --- |"])
    for key, value in values.items():
        lines.append(f"| `{key}` | {format_markdown_inline(value)} |")
    lines.append("")


def render_example_details(example: Example, page_path: str | Path) -> list[str]:
    metadata = [
        f"- ID: `{example.id}`",
        f"- Category: `{example.category}`",
        f"- Lane: `{example.lane}`",
        f"- Status: `{example.status}`",
        f"- Source: [`{example.source}`]({source_url(example)})",
    ]
    if example.agent_copy_safe is not None:
        metadata.append(
            f"- Approved adaptation starter: `{'yes' if example.agent_copy_safe else 'no'}`"
        )
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
        metadata.append(f"- Python adaptation: {python_status_label(example.python_status)}")
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
        if example.webgpu_rendering_effects:
            effects = ", ".join(
                f"`{effect['effect']}` ({effect['status']})"
                for effect in example.webgpu_rendering_effects
            )
            metadata.append(f"- Browser rendering effects: {effects}")
    if example.validation:
        metadata.append(f"- Validation: `{example.validation}`")
    detail_lines = [*metadata, ""]
    if example.tags:
        detail_lines.extend(["**Tags**", "", ", ".join(f"`{tag}`" for tag in example.tags), ""])
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
    media_href = href
    if preferred_preview_media(example) == "video-mp4":
        media_href = site_html_relative_url(page_path, f"examples/{example.page_path[:-3]}/")
        media_href = media_href.rstrip("/") + "/"
    media = media_block(
        page_path,
        example,
        image_dir,
        image_url_base,
        image_format,
        href=media_href,
    )
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
    for section in EXAMPLE_NAVIGATION.sections:
        if lane in section.lanes:
            return section.page_title, section.overview
    return "Examples", "index.md"


def ordered_lane_examples(examples: list[Example], lane: str) -> list[Example]:
    lane_examples = [example for example in examples if example.lane == lane]
    by_id = {example.id: example for example in examples}
    for section in EXAMPLE_NAVIGATION.sections:
        if lane in section.lanes:
            return [
                by_id[id_]
                for id_ in section.ordered_ids
                if id_ in by_id and by_id[id_].lane == lane
            ]
    return sorted(lane_examples, key=lambda e: e.title.lower())


def example_neighbors(examples: list[Example]) -> dict[str, tuple[Example | None, Example | None]]:
    neighbors: dict[str, tuple[Example | None, Example | None]] = {}
    by_id = {example.id: example for example in examples}
    for section in EXAMPLE_NAVIGATION.sections:
        ordered = [by_id[id_] for id_ in section.ordered_ids if id_ in by_id]
        for i, example in enumerate(ordered):
            previous = ordered[i - 1] if i > 0 else None
            next_ = ordered[i + 1] if i + 1 < len(ordered) else None
            neighbors[example.id] = (previous, next_)
    return neighbors


def next_example_section(example: Example) -> tuple[str, str] | None:
    """Return the next Examples overview after the last lane in one navigation section."""
    for index, section in enumerate(EXAMPLE_NAVIGATION.sections):
        if example.lane not in section.lanes or section.lanes[-1] != example.lane:
            continue
        if index + 1 >= len(EXAMPLE_NAVIGATION.sections):
            return None
        next_section = EXAMPLE_NAVIGATION.sections[index + 1]
        path = f"examples/{Path(next_section.overview).with_suffix('').as_posix()}/"
        return next_section.title, path
    return None


def render_example_nav(
    example: Example,
    page_path: str | Path,
    previous: Example | None,
    next_: Example | None,
    location: str = "top",
) -> list[str]:
    previous_href = site_html_relative_url(page_path, f"examples/{previous.page_path[:-3]}/") if previous else ""
    next_href = site_html_relative_url(page_path, f"examples/{next_.page_path[:-3]}/") if next_ else ""
    if previous_href:
        previous_href = previous_href.rstrip("/") + "/"
    if next_href:
        next_href = next_href.rstrip("/") + "/"
    next_title = next_.title if next_ else ""
    if next_ is None:
        section_target = next_example_section(example)
        if section_target is not None:
            next_title, section_path = section_target
            next_href = site_html_relative_url(page_path, section_path)
            next_href = next_href.rstrip("/") + "/"
    previous_link = (
        f'<a href="{previous_href}">← Previous: {previous.title}</a>'
        if previous
        else ""
    )
    next_link = (
        f'<a href="{next_href}">Next: {next_title} →</a>'
        if next_href
        else ""
    )
    return [
        f'<nav class="dvz-example-nav dvz-example-nav--{location}" aria-label="Example navigation">',
        '<div class="dvz-example-nav__siblings">',
        f'<span class="dvz-example-nav__previous">{previous_link}</span>',
        f'<span class="dvz-example-nav__next">{next_link}</span>',
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


def render_page_intro(summary: str) -> list[str]:
    return [summary, ""]


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
    lines.extend(render_page_intro(dedent(config["intro"]).strip()))
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
            "Browse composed scenes demonstrating scientific workflows, real data, and polished demos."
        )
    )
    for group_label, group_ids in SHOWCASE_GROUPS:
        group_examples = [by_id[id_] for id_ in group_ids if id_ in by_id]
        if not group_examples:
            continue
        lines.extend([f"## {group_label} {{ #{navigation_anchor(group_label)} }}", ""])
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
    by_id = {example.id: example for example in examples}
    counts = {
        "visuals": sum(example.lane in ("visuals", "composites") for example in examples),
        "features": sum(example.lane == "features" for example in examples),
        "showcases": sum(example.lane == "showcases" for example in examples),
        "runtime": sum(example.lane == "runtime" for example in examples),
        "advanced": sum(example.lane == "advanced" for example in examples),
    }
    index_path = docs_dir / "index.md"
    page_path = docs_site_path(docs_dir, "index.md")
    lines = generated_header("Examples")
    lines.extend(
        render_page_intro(
            "Explore polished showcases, individual visual families, and focused features. "
            "Each section highlights a curated selection; its category page contains the complete "
            "catalog."
        )
    )
    lines.extend(
        [
            "[Showcases](#showcases) · [Visuals](#visuals) · [Features](#features) · "
            "[Runtime](#runtime-and-advanced) · [Advanced](#runtime-and-advanced)",
            "",
        ]
    )
    for section_id, title, intro, singular_label in INDEX_HIGHLIGHT_SECTIONS:
        section = EXAMPLE_NAVIGATION.section(section_id)
        highlights = [by_id[id_] for id_ in section.highlight_ids if id_ in by_id]
        lines.extend([f"## {title}", "", intro, "", '<div class="grid cards" markdown="1">', ""])
        for example in highlights:
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
        lines.extend(
            [
                "</div>",
                "",
                f"**[Show all {counts[section_id]} {singular_label} examples →]"
                f"({section.overview})**",
                "",
            ]
        )
    lines.extend(
        [
            "## Runtime And Advanced",
            "",
            "Some examples focus on how Datoviz runs rather than on a particular visual result.",
            "",
            "- **[Runtime & Capture](runtime.md)** — open windows, render offscreen, capture, "
            f"record, replay, and export. [Show all {counts['runtime']} runtime examples →]"
            "(runtime.md)",
            "- **[Advanced](advanced.md)** — host integration and lower-level DRP2 or vklite "
            f"workflows. [Show all {counts['advanced']} advanced examples →](advanced.md)",
            "",
            "## Choose By Goal",
            "",
            "| Goal | Start here | Then browse |",
            "| --- | --- | --- |",
            "| Learn the scene → figure → panel → visual workflow | "
            "[Basic Scene](gallery/features/features_basic_scene.md) | "
            f"[{counts['features']} focused features](features.md) |",
            "| Choose marks, lines, images, meshes, text, or volumes | "
            "[Point](gallery/visuals/visuals_point.md) | "
            f"[{counts['visuals']} visuals and composites](visuals.md) |",
            "| Add axes, interaction, layout, animation, or techniques | "
            "[2D Axes](gallery/features/features_axes_2d.md) | "
            f"[{counts['features']} focused features](features.md) |",
            "| Open windows, render offscreen, capture, record, or export | "
            "[Offscreen Capture](gallery/runtime/runtime_offscreen_capture.md) | "
            f"[{counts['runtime']} runtime examples](runtime.md) |",
            "| Study complete scientific visualization compositions | "
            "[Scientific Plotting Workflow]"
            "(gallery/showcases/showcases_scientific_plotting.md) | "
            f"[{counts['showcases']} showcases](showcases.md) |",
            "| Integrate a host or use lower-level rendering APIs | "
            "[Advanced examples](advanced.md) | "
            f"[{counts['advanced']} advanced examples](advanced.md) |",
            "",
            "## Before You Copy An Example",
            "",
            "Every detail page identifies the canonical C source, verified Python availability, "
            "native run command, browser status, data origin, validation level, and whether the "
            "example is approved as a coding-agent adaptation starter.",
            "",
            "- Prefer `supported` examples for application code. Treat `experimental`, "
            "`prototype`, and `advanced/unstable` pages as explicitly scoped evidence.",
            "- A Python tab uses the direct Datoviz engine API (`import datoviz as dvz`); its shape "
            "and lifetime rules still follow the linked C/visual contract.",
            "- `Live in browser` means the same canonical C scenario has a WebGPU route. Planned, "
            "deferred, and native-only examples must be run natively.",
            "- Prepared-data examples do not synthesize a fallback. Run the displayed preparation "
            "command and respect the dataset license/citation notes.",
            "",
            "Coding agents can query [the example inventory](examples.json) and "
            "[capability index](capabilities.json) before selecting a source page.",
            "",
        ]
    )
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
    lines.extend(render_page_intro("Browse one focused example per visual family or composite."))
    by_id = {e.id: e for e in examples}
    for group_label, group_ids in INDEX_VISUAL_GROUPS:
        group_examples = [by_id[i] for i in group_ids if i in by_id]
        if not group_examples:
            continue
        lines.extend([f"## {group_label} {{ #{navigation_anchor(group_label)} }}", ""])
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
            "Browse isolated examples for layout, navigation, adornments, rendering, interaction, animation, and diagnostics."
        )
    )
    grouped_ids = {id_ for _, ids in FEATURE_PAGE_GROUPS for id_ in ids}
    for group_label, group_ids in FEATURE_PAGE_GROUPS:
        group_examples = [by_id[i] for i in group_ids if i in by_id]
        if not group_examples:
            continue
        lines.extend([f"## {group_label} {{ #{navigation_anchor(group_label)} }}", ""])
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
            "Browse examples for opening windows, rendering offscreen, recording, replaying, and exporting media."
        )
    )
    grouped_ids = {id_ for _, ids in RUNTIME_PAGE_GROUPS for id_ in ids}
    for group_label, group_ids in RUNTIME_PAGE_GROUPS:
        group_examples = [by_id[i] for i in group_ids if i in by_id]
        if not group_examples:
            continue
        lines.extend([f"## {group_label} {{ #{navigation_anchor(group_label)} }}", ""])
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
        effect_notes = "; ".join(
            f"{effect['effect'].replace('-', ' ').upper()} {effect['status']}"
            for effect in example.webgpu_rendering_effects
            if effect["status"] != "supported"
        )
        note = "; ".join(part for part in (example.webgpu_reason, effect_notes) if part)
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
    lines.extend(render_run_and_adapt(example, page_path))
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
    validate_navigation(EXAMPLE_NAVIGATION, examples)
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
        if not example.has_detail_page:
            continue
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
    print(f"Generated {len(examples)} public gallery entries under {args.docs_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
