#!/usr/bin/env python3
"""Check page-wide Python/C alternatives in authored Markdown and rendered documentation."""

from __future__ import annotations

import argparse
import json
import re
from dataclasses import dataclass, field
from fnmatch import fnmatch
from html.parser import HTMLParser
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
LABELS = {'Python': 'python', 'C': 'c'}
ALIASES = {
    'py': 'python',
    'python': 'python',
    'python3': 'python',
    'pycon': 'python',
    'c': 'c',
    'cpp': 'c',
    'c++': 'c',
    'h': 'c',
    'hpp': 'c',
}
TAB = re.compile(r'^(\s*)===\+?\s+["\']([^"\']+)["\']\s*$')
FENCE = re.compile(r'^( *)(`{3,}|~{3,})(.*)$')
INCLUDE = re.compile(r'^\s*--8<--\s+["\']([^"\']+)["\']\s*$')


@dataclass
class Problem:
    """A source location and the language-selection rule it violates."""

    path: Path
    line: int
    message: str

    def __str__(self):
        """Format a diagnostic suitable for a command-line check."""
        return f'{self.path}:{self.line}: {self.message}'


@dataclass
class _Line:
    text: str
    path: Path
    number: int
    include_family: str | None = None
    include_nonempty: bool | None = None


@dataclass
class _Group:
    path: Path
    line: int
    indent: int
    labels: list[str] = field(default_factory=list)


@dataclass
class _Block:
    path: Path
    line: int
    family: str | None
    contexts: list[tuple[_Group, str]]
    nonempty: bool = False


def _family(info):
    return ALIASES.get(info.strip().split()[0].lower()) if info.strip() else None


def _source_lines(path, root, stack=()):
    """Expand Markdown snippets while retaining source locations and code-include types."""
    if path in stack:
        raise ValueError(f'{path}: cyclic Markdown include')
    if not path.is_relative_to(root):
        raise ValueError(f'{path}: include escapes the repository root')
    try:
        lines = path.read_text().splitlines()
    except OSError as error:
        raise ValueError(f'{path}: cannot read included source: {error.strerror}') from error
    fence = None
    for number, text in enumerate(lines, 1):
        match = INCLUDE.match(text)
        if match:
            target = (root / match[1]).resolve()
            if not target.is_file():
                raise ValueError(f'{path}:{number}: missing include {match[1]}')
            if not target.is_relative_to(root):
                raise ValueError(f'{path}:{number}: include escapes the repository root')
            prefix = text[: len(text) - len(text.lstrip())]
            if target.suffix.lower() == '.md' and fence is None:
                for included in _source_lines(target, root, (*stack, path)):
                    yield _Line(
                        prefix + included.text,
                        included.path,
                        included.number,
                        included.include_family,
                        included.include_nonempty,
                    )
            else:
                family = ALIASES.get(target.suffix.lower().lstrip('.'))
                yield _Line(prefix, path, number, family, bool(target.read_text().strip()))
            continue
        yield _Line(text, path, number)
        marker = FENCE.match(text)
        if marker:
            if fence is None:
                fence = marker[2]
            elif (
                marker[2][0] == fence[0] and len(marker[2]) >= len(fence) and not marker[3].strip()
            ):
                fence = None


def _mixed(groups):
    return set(LABELS) <= {label for group in groups for label in group.labels}


def _paired(group):
    return group.labels.count('Python') == group.labels.count('C') == 1 and group.labels.index(
        'Python'
    ) < group.labels.index('C')


def _rules(groups, blocks):
    """Require matching host-language alternatives while allowing auxiliary source panes."""
    mixed = _mixed(groups)
    if not mixed:
        return []
    problems = []
    for group in groups:
        if any(label in LABELS for label in group.labels) and not _paired(group):
            problems.append(
                Problem(
                    group.path,
                    group.line,
                    'language tabs need one Python label before one C label',
                )
            )
    for group in groups:
        if not _paired(group):
            continue
        primary = {
            label: [
                block
                for block in blocks
                if any(owner is group and name == label for owner, name in block.contexts)
            ]
            for label in LABELS
        }
        host_code = any(
            block.family in LABELS.values() for content in primary.values() for block in content
        )
        for label, family in LABELS.items():
            if not any(
                block.nonempty and (block.family == family if host_code else True)
                for block in primary[label]
            ):
                kind = label if host_code else 'common-language'
                problems.append(
                    Problem(
                        group.path, group.line, f'{label} pane needs a nonempty {kind} code block'
                    )
                )
    auxiliary = {}
    for block in blocks:
        if block.family not in LABELS.values():
            continue
        for group, label in block.contexts:
            if _paired(group) and label not in LABELS:
                auxiliary.setdefault((id(group), label), set()).add(block.family)
    for block in blocks:
        if block.family not in LABELS.values():
            continue
        controls = [
            (group, label)
            for group, label in block.contexts
            if any(name in LABELS for name in group.labels)
        ]
        if not controls:
            problems.append(
                Problem(block.path, block.line, f'bare {block.family} snippet on a Python/C page')
            )
        elif any(
            not _paired(group)
            or LABELS.get(label, block.family) != block.family
            or (label not in LABELS and len(auxiliary[(id(group), label)]) != 1)
            for group, label in controls
        ):
            problems.append(
                Problem(
                    block.path,
                    block.line,
                    f'{block.family} snippet is outside its matching paired tab',
                )
            )
    return problems


def _report_page(path, groups, blocks, problems, report):
    if report is not None:
        report.update(
            {
                'path': str(path),
                'mixed': _mixed(groups),
                'code_blocks': {
                    family: sum(block.family == family for block in blocks)
                    for family in ('python', 'c')
                },
                'language_tabsets': sum(_paired(group) for group in groups),
                'errors': [str(problem) for problem in problems],
            }
        )


def check_markdown(path: Path, root: Path = ROOT, *, report: dict | None = None) -> list[Problem]:
    """Check fences, nested tabs and repository-relative snippet includes on one page."""
    groups, blocks, contexts, problems = [], [], [], []
    fence = None
    comment = False
    try:
        lines = _source_lines(path.resolve(), root.resolve())
        for source in lines:
            text = source.text.expandtabs(4)
            if source.include_nonempty is not None:
                if fence is not None:
                    fence[2].nonempty |= source.include_nonempty
                    if fence[1] and source.include_family and source.include_family != fence[1]:
                        problems.append(
                            Problem(
                                source.path,
                                source.number,
                                'source include language disagrees with its fence',
                            )
                        )
                elif source.include_family:
                    blocks.append(
                        _Block(
                            source.path,
                            source.number,
                            source.include_family,
                            contexts.copy(),
                            source.include_nonempty,
                        )
                    )
                continue
            marker = FENCE.match(text)
            if fence is not None:
                if (
                    marker
                    and marker[2][0] == fence[0][0]
                    and len(marker[2]) >= len(fence[0])
                    and not marker[3].strip()
                ):
                    fence = None
                else:
                    fence[2].nonempty |= bool(text.strip())
                continue
            if comment:
                comment = '-->' not in text
                continue
            if text.lstrip().startswith('<!--'):
                comment = '-->' not in text
                continue
            tab = TAB.match(text)
            indent = len(text) - len(text.lstrip())
            if tab:
                while contexts and contexts[-1][0].indent > indent:
                    contexts.pop()
                if contexts and contexts[-1][0].indent == indent:
                    group = contexts.pop()[0]
                else:
                    group = _Group(source.path, source.number, indent)
                    groups.append(group)
                group.labels.append(tab[2])
                contexts.append((group, tab[2]))
                continue
            if text.strip():
                while contexts and contexts[-1][0].indent >= indent:
                    contexts.pop()
            if marker:
                family = _family(marker[3])
                block = _Block(source.path, source.number, family, contexts.copy())
                blocks.append(block)
                fence = (marker[2], family, block)
    except ValueError as error:
        problems.append(Problem(path, 1, str(error)))
    problems.extend(_rules(groups, blocks))
    _report_page(path, groups, blocks, problems, report)
    return problems


@dataclass
class _Node:
    tag: str
    attrs: dict[str, str | None]
    line: int
    parent: _Node | None = None
    children: list = field(default_factory=list)

    def has_class(self, name):
        return name in (self.attrs.get('class') or '').split()

    def text(self):
        return ''.join(
            child if isinstance(child, str) else child.text() for child in self.children
        )

    def walk(self):
        yield self
        for child in self.children:
            if isinstance(child, _Node):
                yield from child.walk()


class _HTML(HTMLParser):
    def __init__(self):
        super().__init__()
        self.root = _Node('', {}, 1)
        self.stack = [self.root]

    def handle_starttag(self, tag, attrs):
        node = _Node(tag, dict(attrs), self.getpos()[0], self.stack[-1])
        self.stack[-1].children.append(node)
        if tag not in {
            'area',
            'base',
            'br',
            'col',
            'embed',
            'hr',
            'img',
            'input',
            'link',
            'meta',
            'param',
            'source',
            'track',
            'wbr',
        }:
            self.stack.append(node)

    def handle_startendtag(self, tag, attrs):
        self.handle_starttag(tag, attrs)
        if self.stack[-1].tag == tag:
            self.stack.pop()

    def handle_endtag(self, tag):
        for index in range(len(self.stack) - 1, 0, -1):
            if self.stack[index].tag == tag:
                del self.stack[index:]
                return

    def handle_data(self, data):
        self.stack[-1].children.append(data)


def check_html(path: Path, *, report: dict | None = None) -> list[Problem]:
    """Check rendered code ancestry using the highlighter's language classes."""
    parser = _HTML()
    parser.feed(path.read_text())
    article = next((node for node in parser.root.walk() if node.tag == 'article'), parser.root)
    groups, blocks, owners, problems = [], [], {}, []
    for node in article.walk():
        if not node.has_class('tabbed-set'):
            continue
        children = [child for child in node.children if isinstance(child, _Node)]
        labels = next((child for child in children if child.has_class('tabbed-labels')), None)
        content = next((child for child in children if child.has_class('tabbed-content')), None)
        if labels is None or content is None:
            continue
        names = [
            child.text().strip()
            for child in labels.children
            if isinstance(child, _Node) and child.tag == 'label'
        ]
        branches = [
            child
            for child in content.children
            if isinstance(child, _Node) and child.has_class('tabbed-block')
        ]
        group = _Group(path, node.line, 0, names)
        groups.append(group)
        if len(branches) != len(names):
            problems.append(Problem(path, node.line, 'tab labels and content branches disagree'))
            continue
        for branch, label in zip(branches, names, strict=True):
            owners[id(branch)] = group, label
    for node in article.walk():
        if node.tag != 'code' or node.parent is None or node.parent.tag != 'pre':
            continue
        family, contexts = None, []
        ancestor = node
        while ancestor is not None:
            if family is None:
                for name in (ancestor.attrs.get('class') or '').split():
                    if name.startswith('language-'):
                        family = _family(name.removeprefix('language-'))
            if id(ancestor) in owners:
                contexts.append(owners[id(ancestor)])
            ancestor = ancestor.parent
        blocks.append(_Block(path, node.line, family, contexts, bool(node.text().strip())))
    problems.extend(_rules(groups, blocks))
    _report_page(path, groups, blocks, problems, report)
    return problems


def _exclusions(root):
    """Read the configured literal exclude_docs block without adding a YAML dependency."""
    config = root / 'mkdocs.yml'
    if not config.exists():
        return []
    match = re.search(
        r'^exclude_docs:\s*\|[^\n]*\n((?:[ \t]+[^\n]*\n|\n)*)', config.read_text(), re.MULTILINE
    )
    return [line.strip() for line in match[1].splitlines() if line.strip()] if match else []


def main() -> int:
    """Check published Markdown sources and, optionally, an already built site."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo-root', type=Path, default=ROOT)
    parser.add_argument('--docs-dir', type=Path)
    parser.add_argument('--site-dir', type=Path)
    parser.add_argument('--report', type=Path, help='write a JSON inventory for review')
    args = parser.parse_args()
    root = args.repo_root.resolve()
    docs = args.docs_dir or root / 'docs'
    if not docs.is_dir() or (args.site_dir and not args.site_dir.is_dir()):
        parser.error('documentation and site directories must exist')
    exclusions = _exclusions(root)
    pages = [
        path
        for path in sorted(docs.rglob('*.md'))
        if not any(fnmatch(path.relative_to(docs).as_posix(), pattern) for pattern in exclusions)
    ]
    problems, source_reports, html_reports = [], [], []
    for path in pages:
        report = {}
        problems.extend(check_markdown(path, root, report=report))
        report['path'] = path.resolve().relative_to(root).as_posix()
        if report['mixed']:
            source_reports.append(report)
    html_pages = sorted(args.site_dir.rglob('*.html')) if args.site_dir else []
    for path in html_pages:
        report = {}
        problems.extend(check_html(path, report=report))
        report['path'] = path.relative_to(args.site_dir).as_posix()
        if report['mixed']:
            html_reports.append(report)
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(
            json.dumps(
                {
                    'markdown': source_reports,
                    'html': html_reports,
                    'markdown_pages_checked': len(pages),
                    'html_pages_checked': len(html_pages),
                    'problems': [str(problem) for problem in problems],
                },
                indent=2,
            )
            + '\n'
        )

    for problem in problems:
        print(problem)
    print(
        f'Documentation language tabs: {len(pages)} Markdown pages, '
        f'{len(html_pages)} HTML pages, {len(problems)} problems'
    )
    return bool(problems)


if __name__ == '__main__':
    raise SystemExit(main())
