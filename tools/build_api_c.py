#!/usr/bin/env python3
"""Generate the v0.4 C API reference from extracted public API metadata."""

from __future__ import annotations

import argparse
import fnmatch
import json
import re
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path
from textwrap import dedent

import yaml


ROOT = Path(__file__).resolve().parents[1]
API_PATH = ROOT / "build/bindings/datoviz_api.json"
POLICY_PATH = ROOT / "spec/api/C_API_REFERENCE_POLICY.yaml"
STATUS_PATH = ROOT / "spec/api/status.yml"
DOCS_ROOT = ROOT / "docs"
SUMMARY_PATH = ROOT / "build/docs/c-api-reference-summary.json"
PARAM_RE = re.compile(r"^@param\s+(?P<name>\w+)\s*(?P<doc>.*)$")
RETURN_RE = re.compile(r"^@returns?\s+(?P<doc>.*)$")
PUBLIC_TYPE_RE = re.compile(r"\bDvz[A-Za-z0-9_]+\b")


@dataclass(frozen=True)
class PagePolicy:
    key: str
    title: str
    output: Path
    status: str
    summary: str
    audience: str
    workflows: tuple[tuple[str, str], ...]
    headers: tuple[str, ...]
    prefixes: tuple[str, ...]
    symbols: tuple[str, ...]
    group_labels: dict[str, str]
    group_patterns: dict[str, tuple[str, ...]]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--api", type=Path, default=API_PATH)
    parser.add_argument("--policy", type=Path, default=POLICY_PATH)
    parser.add_argument("--status", type=Path, default=STATUS_PATH)
    parser.add_argument("--summary", type=Path, default=SUMMARY_PATH)
    parser.add_argument(
        "--check",
        action="store_true",
        help="validate classification without writing generated Markdown",
    )
    return parser.parse_args()


def load_json(path: Path) -> dict:
    if not path.exists():
        raise FileNotFoundError(f"{path} not found; run the binding extraction pipeline first")
    return json.loads(path.read_text(encoding="utf8"))


def _as_list(value) -> list[str]:
    if value is None:
        return []
    if isinstance(value, str):
        return [value]
    if isinstance(value, list):
        return [str(item) for item in value]
    return []


def load_status_entries(path: Path) -> list[dict]:
    raw = yaml.safe_load(path.read_text(encoding="utf8")) or {}
    entries = raw.get("modules") or []
    if not isinstance(entries, list):
        raise ValueError(f"{path} must contain a modules list")
    return [entry for entry in entries if isinstance(entry, dict)]


def page_status(entries: list[dict], page_key: str) -> str:
    tiers = {
        str(entry.get("tier"))
        for entry in entries
        if page_key in set(_as_list(entry.get("docs_group")))
    }
    if not tiers:
        return "unclassified; see spec/api/status.yml"
    if tiers == {"stable"}:
        return "stable"
    if tiers == {"experimental"}:
        return "experimental"
    if tiers == {"advanced"}:
        return "advanced/unstable"
    if tiers == {"internal"}:
        return "internal"
    return "mixed tiers; see spec/api/status.yml"


def load_policy(path: Path, status_entries: list[dict]) -> tuple[list[PagePolicy], dict, tuple[str, ...]]:
    raw = yaml.safe_load(path.read_text(encoding="utf8")) or {}
    pages = []
    for key, entry in (raw.get("pages") or {}).items():
        pages.append(
            PagePolicy(
                key=str(key),
                title=str(entry["title"]),
                output=ROOT / str(entry["output"]),
                status=page_status(status_entries, str(key)),
                summary=str(entry["summary"]),
                audience=str(entry.get("audience", "")),
                workflows=tuple(
                    (str(item["label"]), str(item["href"])) for item in entry.get("workflows", ())
                ),
                headers=tuple(str(item) for item in entry.get("headers", ())),
                prefixes=tuple(str(item) for item in entry.get("prefixes", ())),
                symbols=tuple(str(item) for item in entry.get("symbols", ())),
                group_labels={
                    str(prefix): str(label)
                    for prefix, label in (entry.get("group_labels") or {}).items()
                },
                group_patterns={
                    str(label): tuple(str(pattern) for pattern in patterns)
                    for label, patterns in (entry.get("group_patterns") or {}).items()
                },
            )
        )
    return pages, raw.get("types") or {}, tuple(str(item) for item in raw.get("hidden_headers", ()))


def header_of(item: dict) -> str:
    return str((item.get("location") or {}).get("file") or "")


def symbol_prefix(name: str) -> str:
    if name.startswith("dvz_"):
        name = name[4:]
        return name.split("_", 1)[0]
    if name.startswith("Dvz"):
        name = name[3:]
        if name.startswith("DRP2"):
            return "drp2"
        match = re.match(r"[A-Z]+(?=[A-Z][a-z]|$)|[A-Z]?[a-z]+|[0-9]+", name)
        if match:
            return match.group(0).lower()
    return name.split("_", 1)[0]


def header_matches(header: str, patterns: tuple[str, ...]) -> bool:
    return any(fnmatch.fnmatch(header, pattern) for pattern in patterns)


def classify_symbol(item: dict, pages: list[PagePolicy], hidden_headers: tuple[str, ...]) -> str | None:
    header = header_of(item)
    if header_matches(header, hidden_headers):
        return None

    name = str(item.get("name", ""))
    explicit = [
        page.key
        for page in pages
        if header_matches(header, page.headers)
        and any(fnmatch.fnmatchcase(name, pattern) for pattern in page.symbols)
    ]
    if len(explicit) > 1:
        raise ValueError(f"{name} matches explicit symbol rules for multiple pages: {explicit}")
    if explicit:
        return explicit[0]

    prefix = symbol_prefix(name)
    for page in pages:
        if header_matches(header, page.headers) and prefix in page.prefixes:
            return page.key
    for page in pages:
        if header_matches(header, page.headers) and not page.prefixes:
            return page.key
    return None


def clean_doc(raw: str | None) -> list[str]:
    raw = (raw or "").strip()
    if raw.startswith("/**"):
        raw = raw[3:]
    if raw.endswith("*/"):
        raw = raw[:-2]
    lines = []
    for line in raw.splitlines():
        line = line.strip()
        if line.startswith("*"):
            line = line[1:].strip()
        lines.append(line)
    return lines


def doc_parts(raw: str | None) -> tuple[str, dict[str, str], str]:
    text: list[str] = []
    params: dict[str, str] = {}
    ret = ""
    current_field: tuple[str, str] | None = None
    for line in clean_doc(raw):
        if not line:
            if text and text[-1]:
                text.append("")
            current_field = None
            continue
        param_match = PARAM_RE.match(line)
        if param_match:
            name = param_match.group("name")
            params[name] = param_match.group("doc").strip()
            current_field = ("param", name)
            continue
        return_match = RETURN_RE.match(line)
        if return_match:
            ret = return_match.group("doc").strip()
            current_field = ("return", "")
            continue
        if line.startswith("@"):
            current_field = None
            continue
        if current_field is not None:
            kind, name = current_field
            if kind == "param":
                params[name] = " ".join(part for part in (params.get(name, ""), line) if part)
            else:
                ret = " ".join(part for part in (ret, line) if part)
            continue
        text.append(line)
    while text and not text[-1]:
        text.pop()
    return "\n".join(text), params, ret


def table_cell(text: str) -> str:
    return str(text).replace("|", "\\|").replace("\n", "<br>")


def type_name(type_info: dict | None) -> str:
    if not type_info:
        return "void"
    return str(type_info.get("qualtype") or type_info.get("canonical") or "void")


def c_signature(fn: dict) -> str:
    result = type_name(fn.get("result"))
    args = fn.get("parameters") or []
    if not args:
        return f"{result} {fn['name']}(void);"

    lines = [f"{result} {fn['name']}("]
    for index, arg in enumerate(args):
        suffix = "," if index + 1 < len(args) else ""
        lines.append(f"    {type_name(arg.get('type'))} {arg.get('name', '')}{suffix}")
    lines.append(");")
    return "\n".join(lines)


def generated_header(title: str, summary: str | None = None) -> list[str]:
    lines = [
        f"# {title}",
        "",
        "<!-- WARNING: generated by tools/build_api_c.py from build/bindings/datoviz_api.json -->",
        "",
    ]
    if summary:
        lines.extend([summary, ""])
    return lines


def related_functions(fn: dict, names: set[str]) -> list[str]:
    name = str(fn.get("name", ""))
    related = []
    if name.endswith("_create"):
        candidate = f"{name[:-7]}_destroy"
        if candidate in names:
            related.append(candidate)
    elif name.endswith("_destroy"):
        candidate = f"{name[:-8]}_create"
        if candidate in names:
            related.append(candidate)
    if name.endswith("_config"):
        candidate = f"{name[:-7]}_create"
        if candidate in names:
            related.append(candidate)
    if name.endswith("_desc"):
        candidate = name[:-5]
        if candidate in names:
            related.append(candidate)
    if name.endswith("_set_data"):
        for suffix in ("_set_data_range", "_set_data_many"):
            candidate = f"{name[:-9]}{suffix}"
            if candidate in names:
                related.append(candidate)
    if name.endswith("_set_data_range"):
        candidate = name.removesuffix("_range")
        if candidate in names:
            related.append(candidate)
    if name.endswith("_set_data_many"):
        candidate = name.removesuffix("_many")
        if candidate in names:
            related.append(candidate)
    return sorted(set(related))


def type_anchor(name: str) -> str:
    return f"type-{name.lower()}"


def linked_type_name(type_info: dict | None, type_targets: dict[str, str]) -> str:
    value = table_cell(type_name(type_info))
    return PUBLIC_TYPE_RE.sub(
        lambda match: (
            f"[`{match.group(0)}`]({type_targets[match.group(0)]})"
            if match.group(0) in type_targets
            else f"`{match.group(0)}`"
        ),
        value,
    )


def format_function(
    fn: dict, names: set[str], type_targets: dict[str, str], heading: str = "###"
) -> list[str]:
    doc, param_docs, ret_doc = doc_parts(fn.get("doc"))
    header = header_of(fn)
    lines = [f"{heading} `{fn['name']}()`", ""]
    if doc:
        lines.extend([doc, ""])
    lines.extend(
        [
            "```c",
            c_signature(fn),
            "```",
            "",
        ]
    )
    if ret_doc or fn.get("parameters"):
        lines.extend(["| Field | Type | Description |", "| --- | --- | --- |"])
        if ret_doc:
            lines.append(
                f"| return | {linked_type_name(fn.get('result'), type_targets)} | {table_cell(ret_doc)} |"
            )
        for arg in fn.get("parameters") or []:
            name = str(arg.get("name", ""))
            lines.append(
                f"| `{name}` | {linked_type_name(arg.get('type'), type_targets)} | {table_cell(param_docs.get(name, ''))} |"
            )
        lines.append("")
    related = related_functions(fn, names)
    if related:
        links = ", ".join(f"[`{name}()`](#{symbol_anchor(name)})" for name in related)
        lines.extend([f"Related: {links}.", ""])
    if header:
        line = (fn.get("location") or {}).get("line")
        location = f"`{header}`"
        if line:
            location += f":{line}"
        lines.extend([f"_Declared in {location}._", ""])
    return lines


def object_group(name: str, group_labels: dict[str, str] | None = None) -> str:
    symbol = name.removeprefix("dvz_")
    if group_labels:
        for prefix in sorted(group_labels, key=len, reverse=True):
            if symbol == prefix or symbol.startswith(f"{prefix}_"):
                return group_labels[prefix]
    prefix = symbol_prefix(name)
    return prefix.replace("_", " ").title()


def symbol_group(name: str, page: PagePolicy) -> str:
    matches = [
        label
        for label, patterns in page.group_patterns.items()
        if any(fnmatch.fnmatchcase(name, pattern) for pattern in patterns)
    ]
    if len(matches) > 1:
        raise ValueError(f"{name} matches multiple groups on {page.key}: {matches}")
    if matches:
        return matches[0]
    return object_group(name, page.group_labels)


def symbol_anchor(name: str) -> str:
    return name.lower()


def header_summary(functions: list[dict]) -> str:
    headers = sorted({header_of(fn) for fn in functions if header_of(fn)})
    if not headers:
        return ""
    if len(headers) <= 2:
        return ", ".join(f"`{header}`" for header in headers)
    return f"{len(headers)} headers"


def render_page_intro(page: PagePolicy, functions: list[dict], type_count: int) -> list[str]:
    lines = [
        f"!!! info \"Status: {page.status}\"",
        "",
        "    This generated page lists exported C functions and their canonical public types",
        "    classified by the v0.4 C API reference policy. Raw Python `ctypes` call forms are",
        "    documented separately.",
        "",
    ]
    if page.audience:
        lines.extend([page.audience, ""])
    if page.workflows:
        lines.extend(["Common workflows:", ""])
        for label, href in page.workflows:
            lines.append(f"- [{label}]({href})")
        lines.append("")
    lines.extend(
        [
            f"Functions: {len(functions)}",
            f"Types: {type_count}",
            "",
        ]
    )
    return lines


def render_symbol_groups(
    grouped: dict[str, list[dict]], types_grouped: dict[str, list[dict]]
) -> list[str]:
    lines = [
        "## Symbol Groups",
        "",
        "| Group | Functions | Types | Headers |",
        "| --- | ---: | ---: | --- |",
    ]
    for group in sorted(set(grouped) | set(types_grouped)):
        functions = sorted(grouped[group], key=lambda item: item["name"])
        page_types = types_grouped[group]
        headers = functions + [entity["source"] for entity in page_types]
        lines.append(
            f"| [{group}](#{group.lower().replace(' ', '-')}) | {len(functions)} | "
            f"{len(page_types)} | {header_summary(headers)} |"
        )
    lines.append("")
    lines.extend(
        [
            '??? info "Grouped symbol index"',
            "",
        ]
    )
    for group in sorted(set(grouped) | set(types_grouped)):
        functions = sorted(grouped[group], key=lambda item: item["name"])
        page_types = sorted(types_grouped[group], key=lambda item: item["name"])
        lines.extend(
            [
                f"    ### {group}",
                "",
                "    | Symbol | Kind | Header |",
                "    | --- | --- | --- |",
            ]
        )
        for entity in page_types:
            lines.append(
                f"    | [`{entity['name']}`](#{type_anchor(entity['name'])}) | "
                f"{entity['kind']} | `{header_of(entity['source'])}` |"
            )
        for fn in functions:
            lines.append(
                f"    | [`{fn['name']}()`](#{symbol_anchor(fn['name'])}) | function | "
                f"`{header_of(fn)}` |"
            )
        lines.append("")
    return lines


def render_type_entity(entity: dict) -> list[str]:
    source = entity["source"]
    doc, _, _ = doc_parts(source.get("doc"))
    lines = [f'<a id="{type_anchor(entity["name"])}"></a>', "", f"#### `{entity['name']}`", ""]
    if doc:
        lines.extend([doc, ""])
    lines.extend(["```c", entity["signature"], "```", ""])
    header = header_of(source)
    if header:
        line = (source.get("location") or {}).get("line")
        location = f"`{header}`" + (f":{line}" if line else "")
        lines.extend([f"_Declared in {location}._", ""])
    return lines


def render_page(
    page: PagePolicy,
    functions: list[dict],
    page_types: list[dict],
    type_targets: dict[str, str],
) -> None:
    lines = generated_header(page.title, page.summary)
    lines.extend(render_page_intro(page, functions, len(page_types)))

    grouped: dict[str, list[dict]] = defaultdict(list)
    for fn in functions:
        grouped[symbol_group(str(fn["name"]), page)].append(fn)
    types_grouped: dict[str, list[dict]] = defaultdict(list)
    for entity in page_types:
        types_grouped[symbol_group(str(entity["name"]), page)].append(entity)
    lines.extend(render_symbol_groups(grouped, types_grouped))

    names = {str(fn["name"]) for fn in functions}
    for group in sorted(set(grouped) | set(types_grouped)):
        lines.extend([f"## {group}", ""])
        if types_grouped[group]:
            lines.extend(["### Types", ""])
            for entity in sorted(types_grouped[group], key=lambda item: item["name"]):
                lines.extend(render_type_entity(entity))
        if grouped[group]:
            lines.extend(["### Functions", ""])
        for fn in sorted(grouped[group], key=lambda item: item["name"]):
            lines.extend(format_function(fn, names, type_targets, "####"))

    page.output.parent.mkdir(parents=True, exist_ok=True)
    page.output.write_text("\n".join(lines).rstrip() + "\n", encoding="utf8")


def type_signature(record: dict) -> str:
    name = str(record.get("name", ""))
    if record.get("opaque"):
        return f"typedef struct {name} {name};"
    kind = str(record.get("kind") or "struct")
    fields = []
    for field in record.get("fields") or []:
        fields.append(f"    {type_name(field.get('type'))} {field.get('name', '')};")
    return f"{kind} {name} {{\n" + "\n".join(fields) + "\n};"


def enum_signature(enum: dict) -> str:
    name = str(enum.get("name", ""))
    lines = [f"enum {name} {{"]
    for value in enum.get("values") or []:
        lines.append(f"    {value['name']} = {value['value']},")
    lines.append("};")
    return "\n".join(lines)


def typedef_signature(typedef: dict) -> str:
    return f"typedef {type_name(typedef.get('type'))} {typedef['name']};"


def build_type_catalog(
    pages: list[PagePolicy], records: dict, enums: dict, typedefs: dict
) -> tuple[dict[str, list[dict]], dict[str, str]]:
    page_by_key = {page.key: page for page in pages}
    components: dict[str, list[tuple[str, str, dict]]] = defaultdict(list)
    for kind, classified in (("record", records), ("enum", enums), ("typedef", typedefs)):
        for page_key, items in classified.items():
            for item in items:
                components[str(item.get("name", ""))].append((page_key, kind, item))

    by_page: dict[str, list[dict]] = defaultdict(list)
    targets: dict[str, str] = {}
    for name, entries in sorted(components.items()):
        records_for_name = [item for _, kind, item in entries if kind == "record"]
        enums_for_name = [item for _, kind, item in entries if kind == "enum"]
        typedefs_for_name = [item for _, kind, item in entries if kind == "typedef"]
        concrete_records = [item for item in records_for_name if not item.get("opaque")]
        if len(concrete_records) > 1 or len(enums_for_name) > 1:
            raise ValueError(f"public type {name} has multiple concrete definitions")
        definition_items = concrete_records + enums_for_name
        owner_entries = (
            [entry for entry in entries if entry[2] in definition_items]
            if definition_items
            else (
                [entry for entry in entries if entry[1] == "record"]
                if records_for_name
                else entries
            )
        )
        page_keys = {page_key for page_key, _, _ in owner_entries}
        if len(page_keys) != 1:
            raise ValueError(f"public type {name} has conflicting canonical owners: {sorted(page_keys)}")
        page_key = next(iter(page_keys))
        if concrete_records:
            source = concrete_records[0]
            kind = "record"
            signature = type_signature(source)
        elif enums_for_name:
            source = enums_for_name[0]
            kind = "enum"
            signature = enum_signature(source)
        elif typedefs_for_name:
            source = typedefs_for_name[0]
            kind = "typedef"
            signature = typedef_signature(source)
        else:
            source = records_for_name[0]
            kind = "opaque handle"
            signature = type_signature(source)
        entity = {"name": name, "kind": kind, "signature": signature, "source": source}
        by_page[page_key].append(entity)
        targets[name] = f"{page_by_key[page_key].output.name}#{type_anchor(name)}"
    return by_page, targets


def render_types_index(
    types_policy: dict, pages: list[PagePolicy], types_by_page: dict[str, list[dict]]
) -> None:
    output = ROOT / str(types_policy["output"])
    title = str(types_policy.get("title", "C Types"))
    summary = (
        "Alphabetical index of public C types. Canonical definitions live beside the functions "
        "that use them on the generated module pages."
    )
    lines = generated_header(title, summary)
    lines.extend(
        [
            "| Type | Kind | Canonical module | Header |",
            "| --- | --- | --- | --- |",
        ]
    )
    entries = []
    for page in pages:
        for entity in types_by_page.get(page.key, []):
            entries.append((entity["name"], page, entity))
    for name, page, entity in sorted(entries):
        target = f"{page.output.name}#{type_anchor(name)}"
        lines.append(
            f"| [`{name}`]({target}) | {entity['kind']} | {page.title} | "
            f"`{header_of(entity['source'])}` |"
        )
    lines.append("")

    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text("\n".join(lines).rstrip() + "\n", encoding="utf8")


def validate_classification(kind: str, items: list[dict], pages: list[PagePolicy], hidden_headers: tuple[str, ...]) -> tuple[dict, list[str]]:
    by_page: dict[str, list[dict]] = defaultdict(list)
    missing = []
    for item in items:
        name = str(item.get("name", ""))
        if kind == "functions" and not name.startswith("dvz_"):
            continue
        page_key = classify_symbol(item, pages, hidden_headers)
        if page_key is None and kind != "functions":
            header = header_of(item)
            for page in pages:
                if header_matches(header, page.headers):
                    page_key = page.key
                    break
        if page_key is None:
            if not header_matches(header_of(item), hidden_headers):
                missing.append(f"{name} ({header_of(item)})")
            continue
        by_page[page_key].append(item)
    return by_page, missing


def validate_policy_patterns(
    api: dict, pages: list[PagePolicy], functions: dict, types_by_page: dict
) -> None:
    all_items = [
        item
        for kind in ("functions", "records", "enums", "typedefs")
        for item in api.get(kind, [])
    ]
    errors = []
    for page in pages:
        for pattern in page.symbols:
            if not any(
                header_matches(header_of(item), page.headers)
                and fnmatch.fnmatchcase(str(item.get("name", "")), pattern)
                for item in all_items
            ):
                errors.append(f"{page.key}: symbol pattern {pattern!r} matches nothing")
        page_entities = functions.get(page.key, []) + [
            entity["source"] for entity in types_by_page.get(page.key, [])
        ]
        for label, patterns in page.group_patterns.items():
            for pattern in patterns:
                if not any(
                    fnmatch.fnmatchcase(str(item.get("name", "")), pattern)
                    for item in page_entities
                ):
                    errors.append(
                        f"{page.key}/{label}: group pattern {pattern!r} matches no entity"
                    )
    if errors:
        raise ValueError("invalid C API reference patterns:\n  " + "\n  ".join(errors))


def write_summary(
    path: Path, pages: list[PagePolicy], functions: dict, records: dict, enums: dict, typedefs: dict
) -> None:
    summary = {
        "pages": {
            page.key: {
                "title": page.title,
                "status": page.status,
                "functions": len(functions.get(page.key, [])),
                "records": len(records.get(page.key, [])),
                "enums": len(enums.get(page.key, [])),
                "typedefs": len(typedefs.get(page.key, [])),
            }
            for page in pages
        }
    }
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf8")


def main() -> int:
    args = parse_args()
    api = load_json(args.api)
    status_entries = load_status_entries(args.status)
    pages, types_policy, hidden_headers = load_policy(args.policy, status_entries)

    functions, missing_functions = validate_classification(
        "functions", api.get("functions", []), pages, hidden_headers
    )
    records, missing_records = validate_classification(
        "records", api.get("records", []), pages, hidden_headers
    )
    enums, missing_enums = validate_classification("enums", api.get("enums", []), pages, hidden_headers)
    typedefs, missing_typedefs = validate_classification(
        "typedefs", api.get("typedefs", []), pages, hidden_headers
    )
    types_by_page, type_targets = build_type_catalog(pages, records, enums, typedefs)
    validate_policy_patterns(api, pages, functions, types_by_page)

    missing = {
        "functions": missing_functions,
        "records": missing_records,
        "enums": missing_enums,
        "typedefs": missing_typedefs,
    }
    missing = {key: value for key, value in missing.items() if value}
    if missing:
        print("Unclassified public API symbols:")
        for key, values in missing.items():
            print(f"  {key}: {len(values)}")
            for value in values[:20]:
                print(f"    {value}")
            if len(values) > 20:
                print("    ...")
        return 1

    if not args.check:
        for page in pages:
            render_page(
                page,
                sorted(functions.get(page.key, []), key=lambda item: item["name"]),
                sorted(types_by_page.get(page.key, []), key=lambda item: item["name"]),
                type_targets,
            )
        render_types_index(types_policy, pages, types_by_page)
        write_summary(args.summary, pages, functions, records, enums, typedefs)

    total_functions = sum(len(items) for items in functions.values())
    print(f"C API reference classification OK: {total_functions} functions, {len(pages)} pages")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
