"""Load and validate the public example documentation structure."""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import re
from typing import Any, Iterable

import yaml


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_NAVIGATION = ROOT / 'docs/examples/navigation.yaml'


def navigation_anchor(title: str) -> str:
    """Return the stable URL anchor used for an example navigation group."""
    anchor = re.sub(r'[^a-z0-9]+', '-', title.lower()).strip('-')
    if not anchor:
        raise ValueError(f'example navigation title has no URL-safe characters: {title!r}')
    return anchor


@dataclass(frozen=True)
class ExampleGroup:
    title: str
    example_ids: tuple[str, ...]


@dataclass(frozen=True)
class ExampleSection:
    id: str
    title: str
    page_title: str
    overview: str
    lanes: tuple[str, ...]
    groups: tuple[ExampleGroup, ...]
    example_ids: tuple[str, ...]
    index_groups: tuple[ExampleGroup, ...]

    @property
    def ordered_ids(self) -> tuple[str, ...]:
        if self.groups:
            return tuple(id_ for group in self.groups for id_ in group.example_ids)
        return self.example_ids

    @property
    def index(self) -> tuple[ExampleGroup, ...]:
        return self.index_groups or self.groups


@dataclass(frozen=True)
class ExampleNavigation:
    sections: tuple[ExampleSection, ...]

    def section(self, id_: str) -> ExampleSection:
        for section in self.sections:
            if section.id == id_:
                return section
        raise KeyError(f'unknown example navigation section: {id_}')


def _groups(raw: Any, context: str) -> tuple[ExampleGroup, ...]:
    groups = []
    for item in raw or []:
        title = str(item.get('title') or '').strip()
        ids = tuple(str(id_) for id_ in item.get('examples') or [])
        if not title or not ids:
            raise ValueError(f'{context} groups must have a title and examples')
        groups.append(ExampleGroup(title=title, example_ids=ids))
    return tuple(groups)


def load_navigation(path: Path = DEFAULT_NAVIGATION) -> ExampleNavigation:
    raw = yaml.safe_load(path.read_text(encoding='utf8')) or {}
    sections = []
    for item in raw.get('sections') or []:
        id_ = str(item.get('id') or '').strip()
        title = str(item.get('title') or '').strip()
        page_title = str(item.get('page_title') or title).strip()
        overview = str(item.get('overview') or '').strip()
        lanes = tuple(str(lane) for lane in item.get('lanes') or [])
        groups = _groups(item.get('groups'), id_)
        example_ids = tuple(str(value) for value in item.get('examples') or [])
        if not id_ or not title or not overview or not lanes:
            raise ValueError('example navigation sections require id, title, overview, and lanes')
        if bool(groups) == bool(example_ids):
            raise ValueError(f'section {id_!r} must define exactly one of groups or examples')
        sections.append(
            ExampleSection(
                id=id_,
                title=title,
                page_title=page_title,
                overview=overview,
                lanes=lanes,
                groups=groups,
                example_ids=example_ids,
                index_groups=_groups(item.get('index_groups'), f'{id_}.index'),
            )
        )
    return ExampleNavigation(sections=tuple(sections))


def _duplicates(values: Iterable[str]) -> list[str]:
    items = list(values)
    return sorted({value for value in items if items.count(value) > 1})


def validate_navigation(navigation: ExampleNavigation, examples: Iterable[Any]) -> None:
    sections = navigation.sections
    for field, values in (
        ('section IDs', [section.id for section in sections]),
        ('section titles', [section.title for section in sections]),
        ('overview pages', [section.overview for section in sections]),
        ('lanes', [lane for section in sections for lane in section.lanes]),
    ):
        duplicates = _duplicates(values)
        if duplicates:
            raise ValueError(f'duplicate example navigation {field}: {", ".join(duplicates)}')

    by_id = {example.id: example for example in examples if example.has_detail_page}
    expected = {
        id_
        for id_, example in by_id.items()
        if example.lane in {lane for section in sections for lane in section.lanes}
    }
    listed = [id_ for section in sections for id_ in section.ordered_ids]
    duplicates = _duplicates(listed)
    if duplicates:
        raise ValueError(f'duplicate example navigation entries: {", ".join(duplicates)}')
    missing = sorted(expected - set(listed))
    unknown = sorted(set(listed) - expected)
    if missing:
        raise ValueError(f'ungrouped public examples: {", ".join(missing)}')
    if unknown:
        raise ValueError(f'unknown example navigation entries: {", ".join(unknown)}')

    for section in sections:
        allowed_lanes = set(section.lanes)
        wrong_lane = sorted(
            id_ for id_ in section.ordered_ids if by_id[id_].lane not in allowed_lanes
        )
        if wrong_lane:
            raise ValueError(
                f'examples in the wrong {section.id!r} section: {", ".join(wrong_lane)}'
            )
        index_ids = [id_ for group in section.index_groups for id_ in group.example_ids]
        index_duplicates = _duplicates(index_ids)
        if index_duplicates:
            raise ValueError(
                f'duplicate {section.id!r} index entries: {", ".join(index_duplicates)}'
            )
        invalid_index = sorted(set(index_ids) - set(section.ordered_ids))
        if invalid_index:
            raise ValueError(f'unknown {section.id!r} index entries: {", ".join(invalid_index)}')
