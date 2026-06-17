"""Configuration parsing for Datoviz release wheel builds."""

from __future__ import annotations

import os
import sysconfig
from dataclasses import dataclass
from pathlib import Path
from typing import Any

try:
    import tomllib
except ModuleNotFoundError:  # pragma: no cover - Python < 3.11 fallback.
    import tomli as tomllib  # type: ignore[no-redef]


ROOT = Path(__file__).resolve().parents[2]
DEFAULT_STAGE = ROOT / "build" / "wheel-stage"
DEFAULT_DIST = ROOT / "dist"
NAMESPACE = "datoviz."


@dataclass(frozen=True)
class ReleaseWheelConfig:
    """Resolved Datoviz release wheel configuration."""

    root: Path
    release_wheel: bool = False
    platform_tag: str | None = None
    native_build_dir: Path = ROOT / "build"
    stage_dir: Path = DEFAULT_STAGE
    dist_dir: Path = DEFAULT_DIST
    include_qtbridge: bool = False
    skip_repair: bool = False
    runtime_dirs_env: str = "DVZ_WHEEL_RUNTIME_DIRS"
    payload_manifest: str = "datoviz/_wheel_payload.json"
    require_shaderc: bool = True


def default_platform_tag() -> str:
    """Return the host platform tag shape used by wheel filenames."""

    return sysconfig.get_platform().replace("-", "_").replace(".", "_")


def _as_list(value: Any) -> list[Any]:
    if value is None:
        return []
    if isinstance(value, list):
        return value
    return [value]


def _bool_setting(value: Any, *, name: str) -> bool:
    values = _as_list(value)
    if not values:
        return False
    if len(values) > 1:
        raise ValueError(f"{name} may be specified only once")
    item = values[0]
    if isinstance(item, bool):
        return item
    text = str(item).strip().lower()
    if text in {"1", "true", "yes", "on"}:
        return True
    if text in {"0", "false", "no", "off"}:
        return False
    raise ValueError(f"{name} expects a boolean value, got {item!r}")


def _single_setting(settings: dict[str, Any], name: str) -> str | None:
    value = settings.get(name)
    values = _as_list(value)
    if not values:
        return None
    if len(values) > 1:
        raise ValueError(f"{name} may be specified only once")
    return str(values[0])


def _project_wheel_table(root: Path) -> dict[str, Any]:
    pyproject = root / "pyproject.toml"
    if not pyproject.exists():
        return {}
    data = tomllib.loads(pyproject.read_text(encoding="utf8"))
    tool = data.get("tool", {})
    datoviz = tool.get("datoviz", {})
    wheel = datoviz.get("wheel", {})
    if not isinstance(wheel, dict):
        raise ValueError("[tool.datoviz.wheel] must be a TOML table")
    return wheel


def parse_config_settings(
    config_settings: dict[str, Any] | None = None, *, root: Path = ROOT
) -> ReleaseWheelConfig:
    """Parse PEP 517 config settings and project wheel defaults."""

    settings = dict(config_settings or {})
    unknown = sorted(
        key for key in settings if key.startswith(NAMESPACE) and key not in _KNOWN_SETTINGS
    )
    if unknown:
        raise ValueError(f"unknown Datoviz build config setting: {unknown[0]}")

    wheel_table = _project_wheel_table(root)
    release = _bool_setting(settings.get("datoviz.release-wheel"), name="datoviz.release-wheel")
    platform_tag = _single_setting(settings, "datoviz.platform-tag")

    native_build_dir = Path(
        _single_setting(settings, "datoviz.native-build-dir")
        or wheel_table.get("native-build-dir", "build")
    )
    stage_dir = Path(_single_setting(settings, "datoviz.stage-dir") or DEFAULT_STAGE)
    dist_dir = Path(_single_setting(settings, "datoviz.dist-dir") or DEFAULT_DIST)
    include_qtbridge = _bool_setting(
        settings.get("datoviz.include-qtbridge"), name="datoviz.include-qtbridge"
    )
    skip_repair = _bool_setting(settings.get("datoviz.skip-repair"), name="datoviz.skip-repair")

    runtime_dirs_env = str(wheel_table.get("runtime-dirs-env", "DVZ_WHEEL_RUNTIME_DIRS"))
    payload_manifest = str(wheel_table.get("payload-manifest", "datoviz/_wheel_payload.json"))
    require_shaderc = bool(wheel_table.get("require-shaderc", True))

    return ReleaseWheelConfig(
        root=root.resolve(),
        release_wheel=release,
        platform_tag=platform_tag,
        native_build_dir=_resolve(root, native_build_dir),
        stage_dir=_resolve(root, stage_dir),
        dist_dir=_resolve(root, dist_dir),
        include_qtbridge=include_qtbridge,
        skip_repair=skip_repair,
        runtime_dirs_env=runtime_dirs_env,
        payload_manifest=payload_manifest,
        require_shaderc=require_shaderc,
    )


def passthrough_config_settings(config_settings: dict[str, Any] | None) -> dict[str, Any] | None:
    """Return config settings without Datoviz-only keys for delegated builds."""

    if not config_settings:
        return config_settings
    return {key: value for key, value in config_settings.items() if not key.startswith(NAMESPACE)}


def runtime_roots(build_dir: Path, env_name: str) -> list[Path]:
    """Return native build and runtime dependency search roots."""

    roots = [build_dir]
    env = os.environ.get(env_name, "")
    for item in env.split(os.pathsep):
        if item:
            roots.append(Path(item).expanduser().resolve())
    return roots


def _resolve(root: Path, path: Path) -> Path:
    return path if path.is_absolute() else root / path


_KNOWN_SETTINGS = {
    "datoviz.release-wheel",
    "datoviz.platform-tag",
    "datoviz.native-build-dir",
    "datoviz.stage-dir",
    "datoviz.dist-dir",
    "datoviz.include-qtbridge",
    "datoviz.skip-repair",
}

