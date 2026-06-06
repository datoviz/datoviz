#!/usr/bin/env python3
"""Generate the v0.4 public example gallery from the C example manifest."""

from __future__ import annotations

import argparse
import re
import shutil
from dataclasses import dataclass
from pathlib import Path
from textwrap import dedent

import yaml


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_MANIFEST = ROOT / "examples/c/MANIFEST.yaml"
DEFAULT_DOCS_DIR = ROOT / "docs/examples"
DEFAULT_IMAGE_DIR = ROOT / "docs/images/gallery"
SOURCE_BASE_URL = "https://github.com/datoviz/datoviz/blob/v0.4-dev"
PUBLIC_LANES = ("visuals", "features", "composites", "showcases")
STATUS_ORDER = ("supported", "experimental", "prototype", "advanced/unstable", "deferred")
PYTHON_SOURCE_BY_ID = {}

CATEGORY_TO_LANE = {
    "visual": "visuals",
    "feature": "features",
    "composite": "composites",
    "showcase": "showcases",
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
}

TECHNIQUE_IDS = (
    "depth_test",
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
        return f"../images/gallery/{self.lane}/{self.id}.png"

    @property
    def python_source_path(self) -> Path | None:
        if self.python_source is None:
            return None
        return ROOT / self.python_source


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument("--docs-dir", type=Path, default=DEFAULT_DOCS_DIR)
    parser.add_argument("--image-dir", type=Path, default=DEFAULT_IMAGE_DIR)
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
        "scientific": "Scientific",
    }.get(lane, lane.capitalize())


def source_url(example: Example) -> str:
    return f"{SOURCE_BASE_URL}/{example.source}"


def media_block(example: Example, image_dir: Path, depth: int = 0) -> str:
    rel_prefix = "../" * depth
    image = image_dir / example.lane / f"{example.id}.png"
    if image.exists():
        return f"![{example.title}]({rel_prefix}../images/gallery/{example.lane}/{example.id}.png)"
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


def render_card(example: Example, docs_dir: Path, image_dir: Path) -> str:
    page = Path(example.page_path)
    href = page.as_posix()
    media = media_block(example, image_dir)
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
    examples: list[Example], lane: str, docs_dir: Path, image_dir: Path
) -> list[str]:
    lane_examples = [example for example in examples if example.lane == lane]
    if not lane_examples:
        return []
    lines = [f"## {lane_title(lane)}", ""]
    lines.append('<div class="grid cards" markdown="1">')
    lines.append("")
    for example in sorted(lane_examples, key=lambda item: item.title.lower()):
        lines.append(render_card(example, docs_dir, image_dir))
    lines.append("</div>")
    lines.append("")
    return lines


def render_gallery_page(
    filename: str, config: dict, examples: list[Example], docs_dir: Path, image_dir: Path
) -> None:
    page_examples = [example for example in examples if example.lane in config["lanes"]]
    lines = generated_header(config["title"])
    lines.extend(dedent(config["intro"]).strip().splitlines())
    lines.extend(["", f"Coverage: {len(page_examples)} examples ({status_counts(page_examples)}).", ""])
    for lane in config["lanes"]:
        lines.extend(render_lane_section(page_examples, lane, docs_dir, image_dir))
    write_text(docs_dir / filename, "\n".join(lines))


def render_index(examples: list[Example], docs_dir: Path) -> None:
    by_lane = {lane: [example for example in examples if example.lane == lane] for lane in PUBLIC_LANES}
    visual_examples = by_lane["visuals"]
    feature_examples = by_lane["features"]
    composite_examples = by_lane["composites"]
    showcase_examples = by_lane["showcases"]
    lines = generated_header("Examples")
    lines.extend(
        dedent(
            """\
            Examples are executable release proof for v0.4. The public gallery is generated from
            `examples/c/MANIFEST.yaml` and points at the C sources that exercise the current
            scene -> DRP2 -> runtime path.

            Static screenshots are required before final website publication. This generated index
            keeps media status explicit while capture artifacts are prepared separately.
            """
        )
        .strip()
        .splitlines()
    )
    lines.extend(["", "## Public Taxonomy", ""])
    lines.extend(
        [
            "| Category | Examples | Use |",
            "| --- | ---: | --- |",
            f"| [Visual gallery](visual-gallery.md) | {len(visual_examples)} | One public visual family per example. |",
            f"| [Feature gallery](feature-gallery.md) | {len(feature_examples)} | One isolated feature or technique per example. |",
            f"| [Composites](composites.md) | {len(composite_examples)} | One semantic object lowering to one or more visuals per example. |",
            f"| [Showcases](showcases.md) | {len(showcase_examples)} | Composed workflows, scientific stories, real-data examples, and polished demos. |",
        ]
    )
    lines.extend(["", "## Gallery Sections", ""])
    lines.extend(
        [
            "| Section | Examples | Status |",
            "| --- | ---: | --- |",
            f"| [Visual gallery](visual-gallery.md) | {len(visual_examples)} | {status_counts(visual_examples)} |",
            f"| [Feature gallery](feature-gallery.md) | {len(feature_examples)} | {status_counts(feature_examples)} |",
            f"| [Composites](composites.md) | {len(composite_examples)} | {status_counts(composite_examples)} |",
            f"| [Showcases](showcases.md) | {len(showcase_examples)} | {status_counts(showcase_examples)} |",
            f"| [Techniques](techniques.md) | {len([e for e in examples if e.id in TECHNIQUE_IDS])} | Rendering and compute behavior coverage |",
            f"| [Validation gallery](validation-gallery.md) | {len(examples)} | Release evidence checklist |",
        ]
    )
    lines.extend(
        [
            "",
            "## Current Source Lanes",
            "",
            "Public source lanes use `visuals`, `features`, `composites`, or `showcases`.",
            "Concepts such as `workflow`, `scientific`, and `real-data` are manifest tags.",
            "",
            "Coding agents should use [`docs/examples/examples.json`](examples.json),",
            "[`docs/examples/capabilities.json`](capabilities.json), and the",
            "[agent quickstart](../contributors/agent-quickstart.md) when selecting copy-safe",
            "starting points by example or capability.",
            "",
        ]
    )
    lines.extend(["| Lane | Source directory | Examples |", "| --- | --- | ---: |"])
    for lane in PUBLIC_LANES:
        lines.append(f"| {lane_title(lane)} | `examples/c/{lane}/` | {len(by_lane[lane])} |")
    write_text(docs_dir / "index.md", "\n".join(lines))


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
            "Screenshot and video capture should be run separately from documentation generation. Do not stage",
            "`data/gallery`, `docs/images/gallery`, or other generated media without explicit approval.",
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


def render_example_page(example: Example, docs_dir: Path, image_dir: Path) -> None:
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
    lines.extend(["## Media", "", media_block(example, image_dir, depth=2), ""])
    lines.extend(
        [
            "Static screenshots are required before final website publication. Generated media is",
            "prepared separately from this page and should not be staged without explicit approval.",
            "",
        ]
    )
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
    render_index(examples, args.docs_dir)
    for filename, config in PAGE_CONFIG.items():
        render_gallery_page(filename, config, examples, args.docs_dir, args.image_dir)
    render_techniques(examples, args.docs_dir)
    render_validation(examples, args.docs_dir)
    for example in examples:
        render_example_page(example, args.docs_dir, args.image_dir)
    print(f"Generated {len(examples)} C gallery entries under {args.docs_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
