#!/usr/bin/env python3
"""Check the Vulkan course chapters against their per-chapter step programs.

Each chapter page teaches by showing excerpts of a single growing program. This tool enforces that
every C excerpt in a chapter appears verbatim in that chapter's step source, so prose and code can
never drift. Whitespace at line ends is normalized; nothing else is.
"""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import re
import sys


ROOT = Path(__file__).resolve().parents[1]
DOCS = ROOT / "docs" / "gpu-graphics"
SOURCES = ROOT / "examples" / "c" / "vulkan"

# Chapter page -> step program whose content its C excerpts must match.
CHAPTERS = {
    "01-setup.md": "step01.c",
    "02-window.md": "step02.c",
    "03-frame.md": "step03.c",
    "04-triangle.md": "step04.c",
    "05-shader-files.md": "step05.c",
    "06-vertex-buffers.md": "step06.c",
    "07-index-buffers.md": "step07.c",
    "08-push-constants.md": "step08.c",
    "09-matrices.md": "step09.c",
    "10-depth-culling.md": "step10.c",
    "11-mouse-control.md": "step11.c",
    "12-texture-upload.md": "step12.c",
    "13-texture-sampling.md": "step13.c",
    "14-lighting.md": "step14.c",
    "15-mesh.md": "step15.c",
}

FENCE = re.compile(r"^(?P<indent> *)```(?P<lang>[a-z]*)\s*$", re.MULTILINE)

# Excerpts that deliberately differ from the step program: experiments the reader is invited to try,
# which by definition are not in the finished chapter. Matched as a substring of the block.
ALLOWED_DIVERGENCE = {
    "03-frame.md": ("VK_ATTACHMENT_LOAD_OP_LOAD",),
}


@dataclass
class Block:
    lang: str
    line: int
    code: str


def _normalize(text: str) -> str:
    return "\n".join(line.rstrip() for line in text.splitlines())


def _dedent(text: str, indent: int) -> str:
    if indent == 0:
        return text
    out = []
    for line in text.splitlines():
        out.append(line[indent:] if line[:indent].isspace() or not line.strip() else line.lstrip())
    return "\n".join(out)


def _blocks(markdown: str) -> list[Block]:
    """Extract fenced code blocks, keeping their language and starting line."""
    blocks: list[Block] = []
    lines = markdown.splitlines()
    index = 0
    while index < len(lines):
        opening = FENCE.match(lines[index] + "\n")
        if opening is None:
            index += 1
            continue
        indent = len(opening.group("indent"))
        lang = opening.group("lang")
        closing = f"{' ' * indent}```"
        end = index + 1
        while end < len(lines) and lines[end].rstrip() != closing:
            end += 1
        body = "\n".join(lines[index + 1 : end])
        blocks.append(Block(lang=lang, line=index + 1, code=_dedent(body, indent)))
        index = end + 1
    return blocks


def _check_chapter(page: Path, source: Path, errors: list[str]) -> int:
    markdown = page.read_text()
    haystack = _normalize(source.read_text())
    shader_dir = source.with_suffix("")
    shader_haystacks = [
        _normalize(path.read_text())
        for pattern in ("*.vert", "*.frag", "*.glsl")
        for path in sorted(shader_dir.glob(pattern))
    ]
    allowed = ALLOWED_DIVERGENCE.get(page.name, ())
    checked = 0
    included_shaders: list[Path] = []

    for block in _blocks(markdown):
        if block.lang not in {"c", "glsl"}:
            continue
        if block.code.lstrip().startswith("--8<--"):
            match = re.search(r'--8<--\s+["\'](?P<path>[^"\']+)["\']', block.code)
            if match is None:
                errors.append(f"{page.relative_to(ROOT)}:{block.line}: malformed snippet include")
                continue
            included = Path(match.group("path"))
            expected_c = Path("examples/c/vulkan") / source.name
            expected_shader_dir = Path("examples/c/vulkan") / source.stem
            if block.lang == "c" and included != expected_c:
                errors.append(
                    f"{page.relative_to(ROOT)}:{block.line}: full C listing must include {expected_c}"
                )
            elif block.lang == "glsl" and included.parent != expected_shader_dir:
                errors.append(
                    f"{page.relative_to(ROOT)}:{block.line}: shader listing must come from "
                    f"{expected_shader_dir}"
                )
            elif block.lang == "glsl":
                included_shaders.append(included)
            checked += 1
            continue
        code = _normalize(block.code)
        if not code.strip():
            continue
        if any(marker in code for marker in allowed):
            continue
        checked += 1
        candidates = [haystack, *shader_haystacks] if block.lang == "glsl" else [haystack]
        if not any(code in candidate for candidate in candidates):
            errors.append(
                f"{page.relative_to(ROOT)}:{block.line}: {block.lang} excerpt is not present "
                f"verbatim in {source.relative_to(ROOT)}"
            )
    if shader_haystacks:
        expected_shaders = {
            Path("examples/c/vulkan") / source.stem / "shader.vert",
            Path("examples/c/vulkan") / source.stem / "shader.frag",
        }
        if set(included_shaders) != expected_shaders or len(included_shaders) != 2:
            errors.append(
                f"{page.relative_to(ROOT)}: full shader listings must include shader.vert and "
                "shader.frag exactly once"
            )
    return checked


def main() -> int:
    errors: list[str] = []
    checked = 0

    for page_name, source_name in CHAPTERS.items():
        page = DOCS / page_name
        source = SOURCES / source_name
        if not page.is_file():
            errors.append(f"missing chapter page: {page.relative_to(ROOT)}")
            continue
        if not source.is_file():
            errors.append(f"missing step program: {source.relative_to(ROOT)}")
            continue
        checked += _check_chapter(page, source, errors)

    index = DOCS / "index.md"
    if not index.is_file():
        errors.append("missing course overview: docs/gpu-graphics/index.md")

    for source in sorted(SOURCES.glob("step*.c")):
        if source.name not in CHAPTERS.values():
            errors.append(f"step program has no chapter: {source.relative_to(ROOT)}")

    if errors:
        print("\n".join(errors), file=sys.stderr)
        return 1
    print(f"vulkan course: {len(CHAPTERS)} chapters, {checked} code excerpts verified")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
