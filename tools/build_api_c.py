#!/usr/bin/env python3
"""Generate the v0.4 C API reference from extracted public API metadata."""

from __future__ import annotations

import argparse
import ast
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
CTYPES_PATH = ROOT / "datoviz/_ctypes.py"

PARAM_RE = re.compile(r"^@param\s+(?P<name>\w+)\s*(?P<doc>.*)$")
RETURN_RE = re.compile(r"^@returns?\s+(?P<doc>.*)$")


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


@dataclass(frozen=True)
class RawCtypesStatus:
    emitted: frozenset[str]
    skipped: frozenset[str]
    ffi_wrappers: dict[str, str]


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


def generated_list(path: Path, name: str) -> list[str]:
    if not path.exists():
        return []
    module = ast.parse(path.read_text(encoding="utf8"))
    for node in module.body:
        if not isinstance(node, ast.Assign) or len(node.targets) != 1:
            continue
        target = node.targets[0]
        if isinstance(target, ast.Name) and target.id == name:
            value = ast.literal_eval(node.value)
            if isinstance(value, list) and all(isinstance(item, str) for item in value):
                return value
    return []


def load_raw_ctypes_status(path: Path = CTYPES_PATH) -> RawCtypesStatus:
    skipped = frozenset(generated_list(path, "_SKIPPED_FUNCTIONS"))
    emitted: set[str] = set()
    if path.exists():
        module = ast.parse(path.read_text(encoding="utf8"))
        for node in module.body:
            if not isinstance(node, ast.Try) or not node.body:
                continue
            first = node.body[0]
            if not isinstance(first, ast.Assign) or len(first.targets) != 1:
                continue
            target = first.targets[0]
            if isinstance(target, ast.Name) and target.id.startswith("dvz_"):
                emitted.add(target.id)
            continue
        if isinstance(node, ast.FunctionDef) and node.name.startswith("dvz_"):
            emitted.add(node.name)
    ffi_wrappers = {
        "dvz_geometry_arrow_desc": "dvz_ffi_geometry_arrow_desc",
        "dvz_geometry_cone_desc": "dvz_ffi_geometry_cone_desc",
        "dvz_geometry_cube_desc": "dvz_ffi_geometry_cube_desc",
        "dvz_geometry_cylinder_desc": "dvz_ffi_geometry_cylinder_desc",
        "dvz_geometry_disc_desc": "dvz_ffi_geometry_disc_desc",
        "dvz_geometry_plane_desc": "dvz_ffi_geometry_plane_desc",
        "dvz_geometry_regular_polygon_desc": "dvz_ffi_geometry_regular_polygon_desc",
        "dvz_geometry_sector_desc": "dvz_ffi_geometry_sector_desc",
        "dvz_geometry_sphere_desc": "dvz_ffi_geometry_sphere_desc",
        "dvz_geometry_star_desc": "dvz_ffi_geometry_star_desc",
        "dvz_geometry_surface_grid_desc": "dvz_ffi_geometry_surface_grid_desc",
        "dvz_geometry_torus_desc": "dvz_ffi_geometry_torus_desc",
        "dvz_reference_grid_desc": "dvz_ffi_reference_grid_desc",
    }
    return RawCtypesStatus(frozenset(emitted), skipped, ffi_wrappers)


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

    prefix = symbol_prefix(str(item.get("name", "")))
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
    for line in clean_doc(raw):
        if not line:
            if text and text[-1]:
                text.append("")
            continue
        param_match = PARAM_RE.match(line)
        if param_match:
            params[param_match.group("name")] = param_match.group("doc").strip()
            continue
        return_match = RETURN_RE.match(line)
        if return_match:
            ret = return_match.group("doc").strip()
            continue
        if line.startswith("@"):
            continue
        text.append(line)
    while text and not text[-1]:
        text.pop()
    return "\n".join(text), params, ret


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


def raw_ctypes_line(name: str, status: RawCtypesStatus) -> str:
    if name in status.emitted:
        return "Raw ctypes: emitted."
    wrapper = status.ffi_wrappers.get(name)
    if wrapper:
        if wrapper in status.emitted:
            return f"Raw ctypes: available through `{wrapper}()`."
        return f"Raw ctypes: canonical by-value return; FFI wrapper `{wrapper}()` is declared."
    if name in status.skipped:
        return "Raw ctypes: skipped by binding policy."
    return "Raw ctypes: not emitted by the current generated binding."


def format_function(fn: dict, names: set[str], raw_status: RawCtypesStatus) -> list[str]:
    doc, param_docs, ret_doc = doc_parts(fn.get("doc"))
    header = header_of(fn)
    lines = [f"### `{fn['name']}()`", ""]
    lines.extend(
        [
            f"```c title=\"{fn['name']}\"",
            c_signature(fn),
            "```",
            "",
        ]
    )
    if ret_doc or fn.get("parameters"):
        lines.extend(["| Field | Type | Description |", "| --- | --- | --- |"])
        if ret_doc:
            lines.append(f"| return | `{type_name(fn.get('result'))}` | {ret_doc} |")
        for arg in fn.get("parameters") or []:
            name = str(arg.get("name", ""))
            lines.append(
                f"| `{name}` | `{type_name(arg.get('type'))}` | {param_docs.get(name, '')} |"
            )
        lines.append("")
    if doc:
        lines.extend([doc, ""])
    related = related_functions(fn, names)
    if related:
        links = ", ".join(f"[`{name}()`](#{symbol_anchor(name)})" for name in related)
        lines.extend([f"Related: {links}.", ""])
    lines.extend([raw_ctypes_line(str(fn["name"]), raw_status), ""])
    if header:
        line = (fn.get("location") or {}).get("line")
        location = f"`{header}`"
        if line:
            location += f":{line}"
        lines.extend([f"_Declared in {location}._", ""])
    return lines


def object_group(name: str) -> str:
    prefix = symbol_prefix(name)
    return prefix.replace("_", " ").title()


def symbol_anchor(name: str) -> str:
    return name.lower()


def header_summary(functions: list[dict]) -> str:
    headers = sorted({header_of(fn) for fn in functions if header_of(fn)})
    if not headers:
        return ""
    if len(headers) <= 2:
        return ", ".join(f"`{header}`" for header in headers)
    return f"{len(headers)} headers"


def render_page_intro(page: PagePolicy, functions: list[dict]) -> list[str]:
    lines = [
        f"!!! info \"Status: {page.status}\"",
        "",
        "    This generated page lists exported C functions classified by the v0.4 C API",
        "    reference policy. Raw Python `ctypes` call forms are documented separately.",
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
            "",
        ]
    )
    return lines


def render_symbol_groups(grouped: dict[str, list[dict]]) -> list[str]:
    lines = [
        "## Symbol Groups",
        "",
        "| Group | Functions | Headers |",
        "| --- | ---: | --- |",
    ]
    for group in sorted(grouped):
        functions = sorted(grouped[group], key=lambda item: item["name"])
        lines.append(f"| [{group}](#{group.lower().replace(' ', '-')}) | {len(functions)} | {header_summary(functions)} |")
    lines.append("")
    lines.extend(
        [
            '??? info "Grouped symbol index"',
            "",
        ]
    )
    for group in sorted(grouped):
        functions = sorted(grouped[group], key=lambda item: item["name"])
        lines.extend([f"    ### {group}", "", "    | Function | Header |", "    | --- | --- |"])
        for fn in functions:
            lines.append(
                f"    | [`{fn['name']}()`](#{symbol_anchor(fn['name'])}) | `{header_of(fn)}` |"
            )
        lines.append("")
    return lines


def render_page(page: PagePolicy, functions: list[dict], raw_status: RawCtypesStatus) -> None:
    lines = generated_header(page.title, page.summary)
    lines.extend(render_page_intro(page, functions))

    grouped: dict[str, list[dict]] = defaultdict(list)
    for fn in functions:
        grouped[object_group(str(fn["name"]))].append(fn)
    lines.extend(render_symbol_groups(grouped))

    names = {str(fn["name"]) for fn in functions}
    for group in sorted(grouped):
        lines.extend([f"## {group}", ""])
        for fn in sorted(grouped[group], key=lambda item: item["name"]):
            lines.extend(format_function(fn, names, raw_status))

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
    lines = []
    for value in enum.get("values") or []:
        lines.append(f"{value['name']} = {value['value']},")
    return "\n".join(lines)


def typedef_signature(typedef: dict) -> str:
    return f"typedef {type_name(typedef.get('type'))} {typedef['name']};"


def render_types(
    types_policy: dict, pages: list[PagePolicy], records: dict, enums: dict, typedefs: dict
) -> None:
    output = ROOT / str(types_policy["output"])
    title = str(types_policy.get("title", "C Types"))
    summary = str(types_policy.get("summary", ""))
    lines = generated_header(title, summary)

    for page in pages:
        page_records = sorted(records.get(page.key, []), key=lambda item: item.get("name", ""))
        page_enums = sorted(enums.get(page.key, []), key=lambda item: item.get("name", ""))
        page_typedefs = sorted(typedefs.get(page.key, []), key=lambda item: item.get("name", ""))
        if not page_records and not page_enums and not page_typedefs:
            continue
        lines.extend([f"## {page.title}", ""])
        if page_typedefs:
            lines.extend(["### Typedefs", ""])
            for typedef in page_typedefs:
                lines.extend(
                    [
                        f"#### `{typedef['name']}`",
                        "",
                        "```c",
                        typedef_signature(typedef),
                        "```",
                        "",
                    ]
                )
        if page_enums:
            lines.extend(["### Enums", ""])
            for enum in page_enums:
                lines.extend(
                    [
                        f"#### `{enum['name']}`",
                        "",
                        "```c",
                        enum_signature(enum),
                        "```",
                        "",
                    ]
                )
        if page_records:
            lines.extend(["### Records", ""])
            for record in page_records:
                lines.extend(
                    [
                        f"#### `{record['name']}`",
                        "",
                        "```c",
                        type_signature(record),
                        "```",
                        "",
                    ]
                )

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
    raw_status = load_raw_ctypes_status()

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
                page, sorted(functions.get(page.key, []), key=lambda item: item["name"]), raw_status
            )
        render_types(types_policy, pages, records, enums, typedefs)
        write_summary(args.summary, pages, functions, records, enums, typedefs)

    total_functions = sum(len(items) for items in functions.values())
    print(f"C API reference classification OK: {total_functions} functions, {len(pages)} pages")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
