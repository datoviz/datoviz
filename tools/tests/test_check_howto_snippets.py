#!/usr/bin/env python3
"""Focused tests for authored documentation snippet checks."""

from __future__ import annotations

import ast
from pathlib import Path
import sys
import unittest


TOOLS = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TOOLS))

import check_howto_snippets as snippets  # noqa: E402


class PythonPublicSymbolTests(unittest.TestCase):
    def test_inventory_contains_generated_and_managed_top_level_symbols(self) -> None:
        public = snippets._python_public_symbols()
        expected = {
            "dvz_scene",
            "dvz_visual_set_data_many",
            "DVZ_DIM_X",
            "DvzColor",
            "run",
            "capture",
            "Host",
        }
        self.assertTrue(expected <= public, sorted(expected - public))

    def test_dvz_attributes_report_names_and_lines(self) -> None:
        tree = ast.parse(
            "import datoviz as dvz\n"
            "scene = dvz.dvz_scene()\n"
            "dvz.run(scene, figure)\n"
            "value = context.dvz_not_a_public_symbol\n"
        )
        self.assertEqual(
            snippets._python_dvz_attributes(tree),
            [("dvz_scene", 2), ("run", 3)],
        )

    def test_unknown_dvz_attribute_is_outside_inventory(self) -> None:
        tree = ast.parse("dvz.dvz_typo(scene)\n")
        [(name, line)] = snippets._python_dvz_attributes(tree)
        self.assertEqual((name, line), ("dvz_typo", 1))
        self.assertNotIn(name, snippets._python_public_symbols())


if __name__ == "__main__":
    unittest.main()
