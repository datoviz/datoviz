#!/usr/bin/env python3
"""Focused tests for semantic C API reference generation."""

from __future__ import annotations

from dataclasses import replace
from pathlib import Path
import sys
import tempfile
import unittest


TOOLS = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TOOLS))

import build_api_c as api_docs  # noqa: E402
import check_api_c_links as link_check  # noqa: E402


def page(
    key: str,
    header: str,
    *,
    type_symbols: tuple[str, ...] = (),
    type_sources: dict[str, str] | None = None,
) -> api_docs.PagePolicy:
    return api_docs.PagePolicy(
        key=key,
        title=key.title(),
        output=Path(f"docs/reference/c-api/{key}.md"),
        status="stable",
        summary="",
        audience="",
        workflows=(),
        headers=(header,),
        prefixes=(),
        symbols=(),
        type_symbols=type_symbols,
        type_sources=type_sources or {},
        group_labels={},
        group_patterns={},
    )


def item(name: str, header: str, **values) -> dict:
    return {"name": name, "location": {"file": header, "line": 1}, **values}


class TypeCatalogTests(unittest.TestCase):
    def test_symbol_kind_label_is_accessible_but_not_a_markdown_heading(self) -> None:
        rendered = api_docs.symbol_kind_label("Functions")
        self.assertEqual(
            rendered,
            '<p class="dvz-api-kind-label" role="heading" aria-level="3">'
            "<strong>Functions</strong></p>",
        )
        self.assertNotIn("###", rendered)

    def test_rendered_kind_labels_do_not_add_toc_headings(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            policy = replace(page("scene", "scene.h"), output=Path(tmp) / "scene.md")
            function = item(
                "dvz_thing",
                "scene.h",
                result={"qualtype": "void"},
                parameters=[],
            )
            entity = {
                "name": "DvzThing",
                "kind": "record",
                "signature": "struct DvzThing {\n};",
                "source": item("DvzThing", "scene.h"),
                "canonical_header": "",
            }

            api_docs.render_page(policy, [function], [entity], {})
            rendered = policy.output.read_text(encoding="utf8")

            self.assertIn("## Thing { #thing }", rendered)
            self.assertNotIn("\n### Functions", rendered)
            self.assertNotIn("\n### Types", rendered)
            self.assertIn('<p class="dvz-api-kind-label" role="heading"', rendered)
            self.assertIn("#### `dvz_thing()` { #dvz_thing .dvz-api-function }", rendered)
            self.assertIn('<a id="type-dvzthing"></a>', rendered)

    def test_function_heading_has_stable_style_hook(self) -> None:
        function = item(
            "dvz_thing",
            "scene.h",
            result={"qualtype": "void"},
            parameters=[],
        )
        rendered = api_docs.format_function(function, {"dvz_thing"}, {}, "####")
        self.assertEqual(
            rendered[0],
            "#### `dvz_thing()` { #dvz_thing .dvz-api-function }",
        )

    def test_concrete_definition_owns_forward_typedef(self) -> None:
        pages = [page("scene", "scene/**"), page("app", "app.h")]
        record = item("DvzThing", "scene/types.h", kind="struct", opaque=False, fields=[])
        typedef = item("DvzThing", "app.h", type={"qualtype": "struct DvzThing"})
        by_page, targets = api_docs.build_type_catalog(
            pages, {"scene": [record]}, {}, {"app": [typedef]}
        )
        self.assertEqual([entity["name"] for entity in by_page["scene"]], ["DvzThing"])
        self.assertNotIn("app", by_page)
        self.assertEqual(targets["DvzThing"], "scene.md#type-dvzthing")

    def test_explicit_type_owner_overrides_forward_location(self) -> None:
        pages = [
            page("vklite", "vklite/**", type_symbols=("DvzBuffer",)),
            page("drp2", "drp2/**"),
        ]
        typedef = item("DvzBuffer", "drp2/runtime.h", type={"qualtype": "struct DvzBuffer"})
        by_page, _ = api_docs.build_type_catalog(pages, {}, {}, {"drp2": [typedef]})
        self.assertEqual([entity["name"] for entity in by_page["vklite"]], ["DvzBuffer"])

    def test_multiple_explicit_type_owners_fail(self) -> None:
        pages = [
            page("one", "one.h", type_symbols=("DvzThing",)),
            page("two", "two.h", type_symbols=("Dvz*",)),
        ]
        typedef = item("DvzThing", "one.h", type={"qualtype": "struct DvzThing"})
        with self.assertRaisesRegex(ValueError, "multiple explicit owners"):
            api_docs.build_type_catalog(pages, {}, {}, {"one": [typedef]})

    def test_canonical_source_and_hidden_external_type(self) -> None:
        pages = [
            page(
                "vulkan",
                "vk/**",
                type_symbols=("DvzDevice",),
                type_sources={"DvzDevice": "vk/device.h"},
            )
        ]
        device = item("DvzDevice", "app.h", type={"qualtype": "struct DvzDevice"})
        external = item("VkInstance", "vk/instance.h", type={"qualtype": "struct VkInstance_T *"})
        by_page, targets = api_docs.build_type_catalog(
            pages,
            {},
            {},
            {"vulkan": [device, external]},
            hidden_types=("VkInstance",),
        )
        self.assertEqual(api_docs.entity_header(by_page["vulkan"][0]), "vk/device.h")
        self.assertNotIn("VkInstance", targets)

    def test_undocumented_signature_type_fails(self) -> None:
        pages = [page("scene", "scene.h")]
        function = item(
            "dvz_thing",
            "scene.h",
            result={"qualtype": "DvzMissing"},
            parameters=[],
        )
        with self.assertRaisesRegex(ValueError, "DvzMissing"):
            api_docs.build_type_relations(pages, {"scene": [function]}, {})


class LinkValidationTests(unittest.TestCase):
    def test_generated_links_and_explicit_anchors(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            docs = Path(tmp)
            (docs / "one.md").write_text(
                "<!-- generated by tools/build_api_c.py -->\n"
                "[type](two.md#type-dvzthing)\n",
                encoding="utf8",
            )
            (docs / "two.md").write_text(
                "<!-- generated by tools/build_api_c.py -->\n"
                '<a id="type-dvzthing"></a>\n'
                '??? abstract "`DvzThing` · record"\n'
                '#### `dvz_thing()` { #dvz_thing .dvz-api-function }\n',
                encoding="utf8",
            )
            self.assertIn("dvz_thing", link_check.anchors(docs / "two.md"))
            self.assertEqual(link_check.validate(docs), [])
            (docs / "one.md").write_text(
                "<!-- generated by tools/build_api_c.py -->\n"
                "[type](two.md#missing)\n",
                encoding="utf8",
            )
            self.assertEqual(link_check.validate(docs), ["one.md: missing anchor two.md#missing"])


class GeneratedOutputCheckTests(unittest.TestCase):
    def test_detects_committed_reference_drift(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            policy = replace(page("scene", "scene.h"), output=root / "scene.md")
            function = item(
                "dvz_thing",
                "scene.h",
                result={"qualtype": "void"},
                parameters=[],
            )
            functions = {"scene": [function]}
            types_by_page = {}
            types_policy = {"output": str(root / "types.md"), "title": "Types"}

            api_docs.render_page(policy, [function], [], {})
            api_docs.render_types_index(types_policy, [policy], types_by_page)
            self.assertEqual(
                api_docs.check_generated_outputs(
                    [policy], types_policy, functions, types_by_page, {}
                ),
                [],
            )

            policy.output.write_text("stale\n", encoding="utf8")
            errors = api_docs.check_generated_outputs(
                [policy], types_policy, functions, types_by_page, {}
            )
            self.assertEqual(
                errors,
                [f"generated C API reference drift: {policy.output}"],
            )


if __name__ == "__main__":
    unittest.main()
