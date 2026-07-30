from pathlib import Path
import sys
import tempfile
import unittest


TOOLS_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TOOLS_DIR))

import check_python_api_docs as docs  # noqa: E402


class PythonApiDocsTests(unittest.TestCase):
    def test_requires_policy_calls_and_core_concepts(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            policy = root / "ctypes.yml"
            policy.write_text("array_facade:\n  dvz_example: {}\n", encoding="utf8")
            reference = root / "ctypes.md"
            reference.write_text(
                "`dvz_example()` `dvz_view_capture_rgba()` "
                "`dvz_sampled_field_from_array()` `dvz_sampled_field_update_from_array()` "
                "datoviz.raw callback GSP/VisPy2 NumPy\n",
                encoding="utf8",
            )

            self.assertEqual(docs.validate(policy, reference), [])

            reference.write_text("NumPy\n", encoding="utf8")
            errors = docs.validate(policy, reference)
            self.assertTrue(any("dvz_example()" in error for error in errors))
            self.assertTrue(any("exact-call surface" in error for error in errors))


if __name__ == "__main__":
    unittest.main()
