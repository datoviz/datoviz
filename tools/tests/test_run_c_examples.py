from __future__ import annotations

import argparse
import subprocess
import sys
import unittest
from pathlib import Path
from tempfile import TemporaryDirectory
from unittest import mock


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))

import run_c_examples as runner


class RunCExamplesTests(unittest.TestCase):
    def test_strict_review_selection_rejects_all_non_runnable_members(self):
        manifest = {
            "examples": [
                {
                    "id": "features_ok",
                    "source": "examples/c/features/ok.c",
                },
                {
                    "id": "advanced_gui_implot",
                    "source": "examples/c/advanced/gui_implot.cpp",
                    "source_language": "cpp",
                },
                {
                    "id": "advanced_qt_hosting",
                    "source": "examples/qt/qt_hosting.cpp",
                    "source_language": "cpp",
                },
            ]
        }
        args = argparse.Namespace(start_at="", code=True, strict=True)

        with self.assertRaises(ValueError) as raised:
            runner.manifest_id_examples(
                ROOT,
                ROOT / "build/examples/c",
                manifest,
                ["features_ok", "advanced_gui_implot", "advanced_qt_hosting"],
                args,
                [],
            )

        message = str(raised.exception)
        self.assertIn("advanced_gui_implot", message)
        self.assertIn("advanced_qt_hosting", message)
        self.assertIn("non-runnable entries", message)

    def test_main_runs_entire_selection_and_fails_if_any_child_fails(self):
        args = argparse.Namespace(
            all_built=False,
            batch="",
            build_dir="build",
            code=False,
            code_command="code",
            example_arg=[],
            frames="",
            ignore=[],
            list=False,
            review_order=False,
            strict=False,
        )
        with TemporaryDirectory() as tmp:
            root = Path(tmp)
            examples = [
                ("features/fails", root / "fails"),
                ("features/passes", root / "passes"),
            ]
            results = [subprocess.CompletedProcess([], 7), subprocess.CompletedProcess([], 0)]
            with (
                mock.patch.object(runner, "parse_args", return_value=args),
                mock.patch.object(runner, "repo_root", return_value=root),
                mock.patch.object(runner, "manifest_examples", return_value=(examples, [], [])),
                mock.patch.object(runner, "apply_runtime_env"),
                mock.patch.object(runner.subprocess, "run", side_effect=results) as run,
            ):
                self.assertEqual(runner.main(), 1)

        self.assertEqual(run.call_count, 2)


if __name__ == "__main__":
    unittest.main()
