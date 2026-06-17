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

import yaml


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_MANIFEST = ROOT / "examples/c/MANIFEST.yaml"
DEFAULT_DOCS_DIR = ROOT / "docs/examples"
DEFAULT_IMAGE_DIR = ROOT / "data/gallery/v0.4"
DEFAULT_IMAGE_URL_BASE = "/assets/gallery/v0.4"
DEFAULT_IMAGE_FORMAT = "webp"
SOURCE_BASE_URL = "https://github.com/datoviz/datoviz/blob/v0.4-dev"
PUBLIC_LANES = ("start", "visuals", "features", "composites", "showcases", "advanced")
STATUS_ORDER = ("supported", "experimental", "prototype", "advanced/unstable", "deferred")
PYTHON_SOURCE_BY_ID = {}

# Semantic ordering for the examples index page.
# Items not listed fall to the end in their natural order.

INDEX_SHOWCASE_ORDER = (
    "brain_volume",               # flagship neuro volume
    "showcase_lipid_brain_atlas", # neuro atlas
    "showcase_embedding_atlas",   # high-dim data
    "showcase_synthetic_mouse",   # neuro anatomy
    "protein_arcball_viewer",     # molecular 3D
    "point_cloud",                # 3D scatter
    "showcase_surface_grid",      # 3D surface
    "textured_terrain_or_planet", # textured mesh
    "showcase_wind_field",        # vector field
    "showcase_gpu_particle_smoke",# GPU compute
    "scientific_plotting_workflow",
    "scalebar_measurement_workflow",
    "linked_panels_axes_panzoom",
    "linked_panels_probe_colorbar",
    "us_state_choropleth",
)

INDEX_VISUAL_ORDER = (
    # 2D primitives
    "point_2d",
    "visual_pixel",
    "visual_primitive",
    "visual_segment",
    "visual_path",
    # markers and text
    "visual_marker",
    "visual_text",
    "visual_glyph",
    "visual_labels",
    # image / field / splat
    "visual_image",
    "visual_vector",
    "visual_splat",
    # 3D
    "visual_mesh",
    "sphere_impostor",
    "volume",
    # composites
    "composite_polygon",
    "composite_graph",
)

INDEX_FEATURE_ORDER = (
    # layout
    "feature_panel_grid",
    "feature_panel_multi",
    # adornments
    "feature_axis_labels",
    "colorbar",
    # navigation
    "feature_controller_arcball",
    # data / scientific
    "feature_sampled_field_2d",
    "feature_marker_symbols",
    "feature_isolines",
    # 3D rendering
    "feature_lighting",
    "technique_ssao",
    # animation / interaction
    "feature_animation_tracks",
    "image_probe",
)

CATEGORY_TO_LANE = {
    "visual": "visuals",
    "feature": "features",
    "composite": "composites",
    "showcase": "showcases",
    "advanced": "advanced",
}

LANE_TO_CATEGORY = {lane: category for category, lane in CATEGORY_TO_LANE.items()}

PAGE_CONFIG = {
    "visual-gallery.md": {
        "title": "Visual Gallery",
        "lanes": ("visuals",),
        "intro": """\
            This page indexes the current C examples for visual families. Visual examples are
            intentionally small: each one demonstrates one retained rendering family and avoids
            unrelated features where possible.
        """,
    },
    "feature-gallery.md": {
        "title": "Feature Gallery",
        "lanes": ("features",),
        "intro": """\
            This page indexes focused C examples for public scene, layout, adornment, interaction,
            update, rendering-technique, and appearance features.
        """,
    },
    "composites.md": {
        "title": "Composites",
        "lanes": ("composites",),
        "intro": """\
            This page indexes semantic scene objects that lower to one or more visual families.
            Composite examples are still atomic building blocks, not polished user-goal showcases.
        """,
    },
    "showcases.md": {
        "title": "Showcases",
        "lanes": ("showcases",),
        "intro": """\
            Showcases are composed examples for user goals, release proof, and public website media.
            They may use workflows, prepared data, animation, postprocess settings, or
            domain-specific context. Each item still needs a deterministic screenshot before final
            publication.
        """,
    },
    "advanced.md": {
        "title": "Advanced Examples",
        "lanes": ("advanced",),
        "intro": """\
            Advanced examples cover low-level runtime, DRP2, and host-integration paths. They are
            public release proof, but are not the preferred starting point for ordinary scene code.
        """,
    },
}

TECHNIQUE_IDS = (
    "technique_depth_test",
    "technique_edl",
    "technique_ssao",
    "technique_msaa",
    "technique_depth_cue",
    "technique_transparency",
    "alpha_blending",
    "feature_lighting",
    "feature_material_mesh",
    "feature_mesh_texture",
    "point_cloud",
    "protein_arcball_viewer",
    "showcase_gpu_particle_smoke",
)


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
    data: dict
    dataset: dict
    encoding: dict
    webgpu: dict
    agent_copy_safe: bool | None
    python_source: str | None

    @property
    def source_path(self) -> Path:
        return ROOT / self.source

    @property
    def rel_executable(self) -> str:
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


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument("--docs-dir", type=Path, default=DEFAULT_DOCS_DIR)
    parser.add_argument("--image-dir", type=Path, default=DEFAULT_IMAGE_DIR)
    parser.add_argument("--image-url-base", default=DEFAULT_IMAGE_URL_BASE)
    parser.add_argument("--image-format", choices=("png", "webp"), default=DEFAULT_IMAGE_FORMAT)
    return parser.parse_args()


def load_manifest(path: Path) -> dict:
    with path.open("r", encoding="utf8") as f:
        manifest = yaml.safe_load(f) or {}
    if not isinstance(manifest.get("examples"), list):
        raise ValueError(f"{path} does not contain an examples list")
    return manifest


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
    return text


def extract_c_summary(path: Path) -> str:
    if not path.exists():
        return ""
    content = path.read_text(encoding="utf8", errors="replace")
    for match in re.finditer(r"/\*(.*?)\*/", content, flags=re.DOTALL):
        block = match.group(1)
        if "Copyright" in block or "SPDX-License" in block:
            continue
        lines = [re.sub(r"^\s*\*\s?", "", line).strip() for line in block.splitlines()]
        lines = [line for line in lines if line]
        if not lines:
            continue
        first = lines[0]
        if " - " in first:
            return normalize_summary(first.split(" - ", 1)[1])
        return normalize_summary(first)
    return ""


def collect_examples(manifest: dict) -> list[Example]:
    examples: list[Example] = []
    for entry in manifest["examples"]:
        raw_category = entry.get("category")
        if raw_category is not None:
            category = str(raw_category)
            lane = CATEGORY_TO_LANE.get(category, category)
        else:
            lane = str(entry.get("lane", ""))
            category = LANE_TO_CATEGORY.get(lane, lane)
        stage = str(entry.get("stage", ""))
        source = str(entry.get("source", ""))
        if lane not in PUBLIC_LANES or stage == "lab" or not source:
            continue
        tags = entry.get("tags")
        if tags is None:
            tags = entry.get("features", [])
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
            summary=extract_c_summary(ROOT / source),
            data=entry.get("data") or {},
            dataset=entry.get("dataset") or {},
            encoding=entry.get("encoding") or {},
            webgpu=entry.get("webgpu") or {},
            agent_copy_safe=entry.get("agent_copy_safe"),
            python_source=PYTHON_SOURCE_BY_ID.get(str(entry["id"])),
        )
        examples.append(example)
    return examples


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
        "workflows": "Workflows",
        "composites": "Composites",
        "showcases": "Showcases",
        "advanced": "Advanced",
        "scientific": "Scientific",
    }.get(lane, lane.capitalize())


def source_url(example: Example) -> str:
    return f"{SOURCE_BASE_URL}/{example.source}"


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


def html_link(href: str, label: str, code: bool = False) -> str:
    content = f"<code>{label}</code>" if code else label
    return f'<a href="{href}">{content}</a>'


def media_block(
    page_path: str | Path,
    example: Example,
    image_dir: Path,
    image_url_base: str,
    image_format: str = DEFAULT_IMAGE_FORMAT,
) -> str:
    image = image_dir / example.lane / f"{example.id}.png"
    if image.exists():
        return f"![{example.title}]({image_url(page_path, example, image_url_base, image_format)})"
    return "_Media pending._"


def render_source_tabs(example: Example) -> list[str]:
    lines = ["## Source", ""]
    lines.extend(['=== "C"', "", "    ```c"])
    lines.append(f'    --8<-- "{example.source}"')
    lines.extend(["    ```", ""])

    if example.python_source is not None and example.python_source_path is not None:
        lines.extend(['=== "Python"', "", "    ```python"])
        lines.append(f'    --8<-- "{example.python_source}"')
        lines.extend(["    ```", ""])

    return lines


def render_webgpu_live(example: Example, page_path: str | Path) -> list[str]:
    if example.webgpu_status != "webgpu-live" or not example.webgpu_site_route:
        return []
    route = site_html_relative_url(page_path, example.webgpu_site_route)
    return [
        "## Live WebGPU",
        "",
        '<div class="dvz-webgpu-live" markdown="1">',
        f'<iframe src="{route}" title="{example.title} WebGPU live example" '
        'loading="lazy" allow="fullscreen; webgpu"></iframe>',
        "</div>",
        "",
        f'{html_link(route, "Open the live WebGPU example")}.',
        "",
    ]


def render_card(
    example: Example,
    page_path: str | Path,
    image_dir: Path,
    image_url_base: str,
    image_format: str = DEFAULT_IMAGE_FORMAT,
) -> str:
    page = Path(example.page_path)
    href = page.as_posix()
    media = media_block(page_path, example, image_dir, image_url_base, image_format)
    tags = ", ".join(f"`{tag}`" for tag in example.tags[:5])
    if len(example.tags) > 5:
        tags += ", ..."
    tag_line = f"<br><span>{tags}</span>" if tags else ""
    return f"""\
<div class="card" markdown="1">

### [{example.title}]({href})

{media}

`{example.status}` `{example.lane}`{tag_line}

{example.summary}

</div>
"""


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
    lines.extend(dedent(config["intro"]).strip().splitlines())
    lines.extend(["", f"Coverage: {len(page_examples)} examples ({status_counts(page_examples)}).", ""])
    page_path = docs_site_path(docs_dir, filename)
    for lane in config["lanes"]:
        lines.extend(
            render_lane_section(page_examples, lane, page_path, image_dir, image_url_base, image_format)
        )
    write_text(docs_dir / filename, "\n".join(lines))


def render_index(
    examples: list[Example],
    docs_dir: Path,
    image_dir: Path = DEFAULT_IMAGE_DIR,
    image_url_base: str = DEFAULT_IMAGE_URL_BASE,
    image_format: str = DEFAULT_IMAGE_FORMAT,
) -> None:
    by_lane = {lane: [e for e in examples if e.lane == lane] for lane in PUBLIC_LANES}
    showcase_examples = semantic_sort(by_lane["showcases"], INDEX_SHOWCASE_ORDER)
    visual_examples = by_lane["visuals"]
    composite_examples = by_lane["composites"]
    feature_examples = by_lane["features"]
    flagship_feature_ids = set(INDEX_FEATURE_ORDER)
    flagship_features = semantic_sort(
        [e for e in feature_examples if e.id in flagship_feature_ids], INDEX_FEATURE_ORDER
    )
    index_path = docs_dir / "index.md"
    page_path = docs_site_path(docs_dir, "index.md")

    lines = generated_header("Examples")
    lines.extend(
        [
            "Datoviz v0.4 ships with "
            f"{len(showcase_examples)} showcases, "
            f"{len(visual_examples) + len(composite_examples)} visual families, "
            f"and {len(feature_examples)} feature examples.",
            "",
        ]
    )

    # --- Showcases ---
    lines.extend(["## Showcases", ""])
    lines.extend(
        [
            "Composed scenes demonstrating scientific workflows, real data, and polished demos.",
            "",
        ]
    )
    lines.append('<div class="grid cards" markdown="1">')
    lines.append("")
    for example in showcase_examples:
        lines.append(render_card(example, page_path, image_dir, image_url_base, image_format))
    lines.append("</div>")
    lines.append("")

    # --- Visuals & Composites ---
    n_vc = len(visual_examples) + len(composite_examples)
    lines.extend(["## Visuals & Composites", ""])
    lines.extend(
        [
            f"{n_vc} rendering families — one example each.",
            "",
        ]
    )
    lines.append('<div class="grid cards" markdown="1">')
    lines.append("")
    for example in semantic_sort(visual_examples + composite_examples, INDEX_VISUAL_ORDER):
        lines.append(render_card(example, page_path, image_dir, image_url_base, image_format))
    lines.append("</div>")
    lines.append("")

    # --- Features & Techniques ---
    lines.extend(["## Features & Techniques", ""])
    lines.extend(
        [
            "A selection of isolated feature and technique examples.",
            "",
        ]
    )
    lines.append('<div class="grid cards" markdown="1">')
    lines.append("")
    for example in flagship_features:
        lines.append(render_card(example, page_path, image_dir, image_url_base, image_format))
    lines.append("</div>")
    lines.append("")
    lines.extend(
        [
            f"[Browse all {len(feature_examples)} feature examples](feature-gallery.md) — "
            "controllers, adornments, interaction, animation, techniques, and more.",
            "",
        ]
    )

    write_text(index_path, "\n".join(lines))


def render_techniques(examples: list[Example], docs_dir: Path) -> None:
    selected = [example for example in examples if example.id in TECHNIQUE_IDS]
    lines = generated_header("Techniques")
    lines.extend(
        dedent(
            """\
            Technique coverage is currently represented by focused feature examples and selected
            showcases. Public rendering techniques should normally be indexed as feature examples,
            with `technique` tags for filtering.
            """
        )
        .strip()
        .splitlines()
    )
    lines.extend(["", "| Technique coverage | Source | Status |", "| --- | --- | --- |"])
    for example in selected:
        lines.append(
            f"| [{example.title}]({example.page_path}) | "
            f"[`{example.source}`]({source_url(example)}) | `{example.status}` |"
        )
    write_text(docs_dir / "techniques.md", "\n".join(lines))


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
            f"- Public C examples in manifest: {len(examples)}",
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
            This page is generated from `examples/c/MANIFEST.yaml`. `webgpu-live` rows are public
            browser routes backed by the same canonical C example or portable C scenario as native
            validation. Browser JavaScript is host glue only.
            """
        )
        .strip()
        .splitlines()
    )
    lines.extend(
        [
            "",
            "| Example | Source | WebGPU status | Requirements | Browser route |",
            "| --- | --- | --- | --- | --- |",
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
            else ""
        )
        lines.append(
            f"| [{example.title}]({example.page_path}) | "
            f"[`{example.source}`]({source_url(example)}) | "
            f"`{example.webgpu_status}` | {requirements} | {route} |"
        )
    write_text(docs_dir / "webgpu-matrix.md", "\n".join(lines))


def render_example_page(
    example: Example,
    docs_dir: Path,
    image_dir: Path,
    image_url_base: str,
    image_format: str = DEFAULT_IMAGE_FORMAT,
) -> None:
    page_path = docs_site_path(docs_dir, example.page_path)
    lines = generated_header(example.title)
    lines.extend([example.summary, ""])
    metadata = [
        f"- ID: `{example.id}`",
        f"- Category: `{example.category}`",
        f"- Lane: `{example.lane}`",
        f"- Status: `{example.status}`",
        f"- Source: [`{example.source}`]({source_url(example)})",
    ]
    if example.agent_copy_safe is not None:
        metadata.append(f"- Agent copy-safe: `{str(example.agent_copy_safe).lower()}`")
    if example.python_source is not None:
        metadata.append(
            f"- Python source: [`{example.python_source}`]({SOURCE_BASE_URL}/{example.python_source})",
        )
    if example.webgpu:
        metadata.append(f"- WebGPU status: `{example.webgpu_status}`")
        if example.webgpu_route:
            metadata.append(
                f"- WebGPU live route: "
                f"{html_link(site_html_relative_url(page_path, example.webgpu_site_route), example.webgpu_route, code=True)}"
            )
        if example.webgpu_requirements:
            requirements = ", ".join(f"`{requirement}`" for requirement in example.webgpu_requirements)
            metadata.append(f"- WebGPU requirements: {requirements}")
    metadata.extend(
        [
            f"- Build: `just example-c {example.rel_executable}`",
            f"- Smoke: `./build/examples/c/{example.rel_executable} --png`",
            f"- Validation: `{example.validation}`",
        ]
    )
    lines.extend(metadata)
    lines.append("")
    if example.tags:
        lines.extend(["## Tags", ""])
        lines.append(", ".join(f"`{tag}`" for tag in example.tags))
        lines.append("")
    if example.data:
        lines.extend(["## Data", "", "| Field | Value |", "| --- | --- |"])
        for key, value in example.data.items():
            lines.append(f"| `{key}` | {value} |")
        lines.append("")
    if example.dataset:
        lines.extend(["## Dataset", "", "| Field | Value |", "| --- | --- |"])
        for key, value in example.dataset.items():
            lines.append(f"| `{key}` | {value} |")
        lines.append("")
    if example.encoding:
        lines.extend(["## Encoding", "", "| Field | Value |", "| --- | --- |"])
        for key, value in example.encoding.items():
            lines.append(f"| `{key}` | {value} |")
        lines.append("")
    lines.extend(
        ["## Media", "", media_block(page_path, example, image_dir, image_url_base, image_format), ""]
    )
    lines.extend(
        [
            "Static screenshots are required before final website publication. Generated media is",
            "prepared in the `data` submodule and linked from this page.",
            "",
        ]
    )
    lines.extend(render_webgpu_live(example, page_path))
    lines.extend(render_source_tabs(example))
    write_text(docs_dir / example.page_path, "\n".join(lines))


def clean_generated_pages(docs_dir: Path) -> None:
    generated = docs_dir / "gallery"
    if generated.exists():
        shutil.rmtree(generated)


def main() -> int:
    args = parse_args()
    manifest = load_manifest(args.manifest)
    examples = collect_examples(manifest)
    clean_generated_pages(args.docs_dir)
    render_index(examples, args.docs_dir, args.image_dir, args.image_url_base, args.image_format)
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
    render_techniques(examples, args.docs_dir)
    render_validation(examples, args.docs_dir)
    render_webgpu_matrix(examples, args.docs_dir)
    for example in examples:
        render_example_page(
            example, args.docs_dir, args.image_dir, args.image_url_base, args.image_format
        )
    print(f"Generated {len(examples)} C gallery entries under {args.docs_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
