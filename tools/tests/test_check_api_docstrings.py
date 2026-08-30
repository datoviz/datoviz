#!/usr/bin/env python3
"""Tests for public C API docstring validation."""

from __future__ import annotations

from pathlib import Path
import sys
import tempfile
import unittest


TOOLS = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TOOLS))

import check_api_docstrings as docs  # noqa: E402


class DocstringValidationTests(unittest.TestCase):
    def function(self, **values) -> docs.Function:
        defaults = {
            "name": "dvz_thing_set_value",
            "header": "include/datoviz/thing.h",
            "line": 12,
            "result": "DvzResult",
            "parameters": ("thing", "value"),
            "doc": """/**
                * Set a value.
                *
                * @param thing the thing
                * @param value the new value
                * @return `DVZ_OK` on success, otherwise `DVZ_ERROR`
                */""",
        }
        defaults.update(values)
        return docs.Function(**defaults)

    def test_complete_docstring_passes(self) -> None:
        self.assertEqual(docs.validate(self.function()), [])

    def test_missing_stale_and_duplicate_tags_fail(self) -> None:
        function = self.function(
            doc="""/**
                * Set a value.
                * @param thing
                * @param old stale value
                * @param old duplicate stale value
                */"""
        )
        errors = "\n".join(docs.validate(function))
        self.assertIn("missing @param value", errors)
        self.assertIn("stale @param old", errors)
        self.assertIn("duplicate @param old", errors)
        self.assertIn("empty @param thing description", errors)
        self.assertIn("missing @return", errors)

    def test_multiline_and_direction_qualifier_pass(self) -> None:
        function = self.function(
            result="void",
            parameters=("out",),
            doc="""/**
                * Return state through an output argument.
                *
                * @param[out] out caller-owned output storage that remains valid
                *   after this call
                */""",
        )
        self.assertEqual(docs.validate(function), [])

    def test_void_function_return_tag_fails(self) -> None:
        function = self.function(
            result="void",
            parameters=(),
            doc="""/**
                * Perform an action.
                * @return nothing
                */""",
        )
        self.assertIn("unexpected @return", "\n".join(docs.validate(function)))

    def test_growable_borrowed_accessor_requires_invalidation_contract(self) -> None:
        function = self.function(
            name="dvz_drp2_stream_get",
            result="const DvzDrp2Command *",
            parameters=("stream", "index"),
            doc="""/**
                * Return a command.
                * @param stream the stream
                * @param index the command index
                * @return the command
                */""",
        )
        errors = "\n".join(docs.validate(function))
        self.assertIn("must mention 'borrowed'", errors)
        self.assertIn("must mention 'appends a command'", errors)
        self.assertIn("must mention 'stream is destroyed'", errors)

    def test_growable_borrowed_accessor_complete_contract_passes(self) -> None:
        function = self.function(
            name="dvz_drp2_stream_get",
            result="const DvzDrp2Command *",
            parameters=("stream", "index"),
            doc="""/**
                * Return a command.
                *
                * The pointer is borrowed. A call that appends a command invalidates it, as does
                * the point when the stream is destroyed.
                *
                * @param stream the stream
                * @param index the command index
                * @return the command
                */""",
        )
        self.assertEqual(docs.validate(function), [])

    def test_public_inline_function_discovery(self) -> None:
        with tempfile.TemporaryDirectory(dir=docs.ROOT) as tmp:
            root = Path(tmp)
            header = root / "inline.h"
            header.write_text(
                """/** Return the larger value.
 * @param a first value
 * @param b second value
 * @return larger value
 */
static inline int dvz_max(int a, int b) { return a > b ? a : b; }
""",
                encoding="utf8",
            )
            functions = docs.load_inline(root)
        self.assertEqual(len(functions), 1)
        self.assertEqual(functions[0].parameters, ("a", "b"))
        self.assertEqual(docs.validate(functions[0]), [])


if __name__ == "__main__":
    unittest.main()
