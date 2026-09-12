#!/usr/bin/env python3
"""
Check instruction links, shared Claude imports, and unwrapped Markdown prose.

The default scope is repository AGENTS.md/CLAUDE.md files, agents/README.md,
and agents/rules/*.md. Explicit paths can check handoffs or other Markdown.
Links are checked for local target existence, not anchors or external availability.
This is a structural check for the repository's Markdown conventions, not a full
CommonMark parser or a check of instruction meaning.
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path
from urllib.parse import unquote, urlsplit

ROOT = Path(__file__).resolve().parents[1]
FENCE = re.compile(r'^ {0,3}(`{3,}|~{3,})(.*)$')
INLINE_CODE = re.compile(r'(`+)(?!`)(.+?)(?<!`)\1(?!`)')
DESTINATION = r'(<[^>\n]+>|(?:\\.|[^\s()]|\([^()\n]*\))+)'
LINK = re.compile(r'!?\[[^\]\n]*\]\(' + DESTINATION + r"(?:\s+[\"'][^\n]*[\"'])?\)")
REFERENCE = re.compile(r'^ {0,3}\[[^\]]+\]:\s*' + DESTINATION)
LIST = re.compile(r'^\s*(?:[-+*]|\d+[.)])\s+')
TABLE_RULE = re.compile(r'^\s*\|?\s*:?-{3,}:?\s*(?:\|\s*:?-{3,}:?\s*)+\|?\s*$')
THEMATIC = re.compile(r'^\s*(?:(?:\*\s*){3,}|(?:-\s*){3,}|(?:_\s*){3,})$')


def content_lines(text: str) -> list[tuple[int, str]]:
    """Mask fenced and indented code while retaining line numbers and separators."""
    result = []
    fence = ''
    indented = False
    previous_blank = True
    for number, line in enumerate(text.splitlines(), 1):
        marker = FENCE.match(line)
        if fence:
            if (
                marker
                and marker[1][0] == fence[0]
                and len(marker[1]) >= len(fence)
                and not marker[2].strip()
            ):
                fence = ''
            result.append((number, ''))
            previous_blank = True
            continue
        if marker:
            fence = marker[1]
            result.append((number, ''))
            previous_blank = True
            continue
        if line.startswith(('    ', '\t')) and (previous_blank or indented):
            indented = True
            result.append((number, ''))
        else:
            indented = False
            result.append((number, line))
        previous_blank = not line.strip()
    return result


def check_markdown(path: Path) -> list[str]:
    """Report missing local link targets and continuation lines in prose/lists."""
    errors = []
    lines = content_lines(path.read_text(encoding='utf-8'))
    previous_prose = False
    in_table = False
    for index, (number, line) in enumerate(lines):
        clean = INLINE_CODE.sub('', line)
        reference = REFERENCE.match(clean)
        targets = [match[1] for match in LINK.finditer(clean)]
        if reference:
            targets.append(reference[1])
        for target in targets:
            target = target.removeprefix('<').removesuffix('>')
            target = re.sub(r'\\([() ])', r'\1', target)
            url = urlsplit(target)
            if url.scheme or url.netloc or not url.path:
                continue
            if not (path.parent / unquote(url.path)).exists():
                errors.append(f'{path}:{number}: missing local link target: {target}')

        stripped = line.strip()
        next_line = lines[index + 1][1] if index + 1 < len(lines) else ''
        in_table = bool(TABLE_RULE.fullmatch(next_line)) or (in_table and '|' in line)
        structural = (
            not stripped
            or in_table
            or stripped.startswith(('#', '|', '>', '<!--', '@'))
            or reference is not None
            or THEMATIC.fullmatch(line) is not None
            or TABLE_RULE.fullmatch(line) is not None
            or TABLE_RULE.fullmatch(next_line) is not None
            or re.fullmatch(r'\s*=+\s*', line) is not None
            or re.fullmatch(r'\s*[=-]+\s*', next_line) is not None
        )
        if not structural and not LIST.match(line) and previous_prose:
            errors.append(f'{path}:{number}: hard-wrapped prose or list item')
        previous_prose = not structural
    return errors


def check_paths(paths: list[Path]) -> list[str]:
    """Check documents and ensure each shared instruction scope has its import."""
    errors = []
    for path in sorted(set(paths)):
        if not path.is_file():
            errors.append(f'{path}: missing instruction file')
            continue
        errors.extend(check_markdown(path))
        if path.name in {'AGENTS.md', 'CLAUDE.md'}:
            shared = path.with_name('AGENTS.md')
            wrapper = path.with_name('CLAUDE.md')
            if not shared.is_file():
                errors.append(f'{wrapper}: missing sibling AGENTS.md')
            if (
                not wrapper.is_file()
                or wrapper.read_text(encoding='utf-8').strip() != '@AGENTS.md'
            ):
                errors.append(f'{wrapper}: expected only @AGENTS.md to share instructions')
    return sorted(set(errors))


def instruction_paths(root: Path) -> list[Path]:
    """Discover tracked and unignored instruction files without visiting submodules."""
    # Fixed read-only Git command, resolved through the developer's normal PATH.
    result = subprocess.run(  # noqa: S603
        ['git', 'ls-files', '--cached', '--others', '--exclude-standard', '-z'],  # noqa: S607
        cwd=root,
        check=True,
        capture_output=True,
    )
    paths = {root / 'AGENTS.md', root / 'CLAUDE.md', root / 'agents/README.md'}
    for name in result.stdout.decode('utf-8').split('\0'):
        path = Path(name)
        if path.name in {'AGENTS.md', 'CLAUDE.md'} or (
            path.parent == Path('agents/rules') and path.suffix == '.md'
        ):
            paths.add(root / path)
    return sorted(paths)


def main() -> int:
    """Check the selected documents and return a failing status for structural errors."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        'paths',
        nargs='*',
        type=Path,
        help='explicit Markdown files (default: repository instructions)',
    )
    args = parser.parse_args()
    paths = args.paths or instruction_paths(ROOT)
    errors = check_paths(paths)
    if errors:
        print('\n'.join(errors), file=sys.stderr)
        return 1
    print(f'Agent instruction checks passed ({len(set(paths))} files).')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
