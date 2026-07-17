"""Regression tests for the release automation CLI."""

import datetime as dt
import importlib.util
import sys
from pathlib import Path

ROOT_DIR = Path(__file__).resolve().parents[1]
TOOL_PATH = ROOT_DIR / "tools" / "release_automation.py"


def _load_release_automation():
    spec = importlib.util.spec_from_file_location("release_automation", TOOL_PATH)
    assert spec is not None
    assert spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def test_utc_now_is_python_310_compatible_utc_timestamp() -> None:
    """Ensure release timestamps use APIs available on Python 3.10."""
    release_automation = _load_release_automation()
    timestamp = release_automation.utc_now()

    assert timestamp.endswith("Z")
    parsed = dt.datetime.fromisoformat(timestamp.removesuffix("Z") + "+00:00")
    assert parsed.tzinfo == dt.timezone.utc
    assert parsed.microsecond == 0
