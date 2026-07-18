"""Direct wheel writer for Datoviz release wheels."""

from __future__ import annotations

import base64
import csv
import hashlib
import io
import zipfile
from pathlib import Path

from .config import ReleaseWheelConfig, default_platform_tag
from .metadata import (
    distribution_name,
    entry_points_text,
    metadata_text,
    project_metadata,
    wheel_text,
    wheel_version,
)
from .native_payload import stage_payload
from .repair import repair_wheel
from .tags import repair_input_platform_tag


def build_release_wheel(
    wheel_directory: str, config: ReleaseWheelConfig, metadata_directory: str | None = None
) -> str:
    """Build and optionally repair a Datoviz release wheel."""

    del metadata_directory
    requested_tag = config.platform_tag or default_platform_tag()
    input_tag = requested_tag if config.skip_repair else repair_input_platform_tag(requested_tag)
    stage_payload(config, clean=True)
    dist_dir = Path(wheel_directory)
    dist_dir.mkdir(parents=True, exist_ok=True)
    for old in dist_dir.glob("datoviz-*.whl"):
        old.unlink()
    wheel_path = write_wheel_from_stage(config.stage_dir, dist_dir, input_tag, root=config.root)
    repaired = repair_wheel(
        wheel_path, skip=config.skip_repair, platform_tag=requested_tag
    )
    return repaired.name


def write_wheel_from_stage(stage_dir: Path, dist_dir: Path, platform_tag: str, *, root: Path) -> Path:
    """Write a platform wheel directly from a staged Datoviz tree."""

    project = project_metadata(root)
    name = distribution_name(project)
    version = wheel_version(project)
    dist_info = f"{name}-{version}.dist-info"
    wheel_name = f"{name}-{version}-py3-none-{platform_tag}.whl"
    dist_dir.mkdir(parents=True, exist_ok=True)
    wheel_path = dist_dir / wheel_name

    files: dict[str, bytes] = {}
    package_dir = stage_dir / "datoviz"
    if not package_dir.exists():
        raise FileNotFoundError(f"staged package directory not found: {package_dir}")
    for path in sorted(item for item in package_dir.rglob("*") if item.is_file()):
        if _is_forbidden(path):
            raise RuntimeError(f"forbidden file in wheel payload: {path}")
        arcname = path.relative_to(stage_dir).as_posix()
        files[arcname] = path.read_bytes()

    files[f"{dist_info}/METADATA"] = metadata_text(root).encode("utf8")
    files[f"{dist_info}/WHEEL"] = wheel_text(platform_tag).encode("utf8")
    entry_points = entry_points_text(project)
    if entry_points:
        files[f"{dist_info}/entry_points.txt"] = entry_points.encode("utf8")

    records = []
    for arcname, data in sorted(files.items()):
        digest = _hash(data)
        records.append((arcname, f"sha256={digest}", str(len(data))))
    record_name = f"{dist_info}/RECORD"
    records.append((record_name, "", ""))
    files[record_name] = _record_bytes(records)

    with zipfile.ZipFile(wheel_path, "w", compression=zipfile.ZIP_DEFLATED) as zf:
        for arcname, data in sorted(files.items()):
            info = zipfile.ZipInfo(arcname)
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o644 << 16
            zf.writestr(info, data)
    return wheel_path


def build_from_stage(
    stage_dir: Path,
    dist_dir: Path,
    platform_tag: str | None,
    *,
    root: Path,
    skip_repair: bool = False,
) -> Path:
    """Compatibility helper for existing staged local workflows."""

    requested_tag = platform_tag or default_platform_tag()
    input_tag = requested_tag if skip_repair else repair_input_platform_tag(requested_tag)
    dist_dir.mkdir(parents=True, exist_ok=True)
    for old in dist_dir.glob("datoviz-*.whl"):
        old.unlink()
    wheel_path = write_wheel_from_stage(stage_dir, dist_dir, input_tag, root=root)
    return repair_wheel(wheel_path, skip=skip_repair, platform_tag=requested_tag)


def single_wheel(dist_dir: Path) -> Path:
    """Return the single Datoviz wheel in a directory."""

    wheels = sorted(dist_dir.glob("datoviz-*.whl"))
    if len(wheels) != 1:
        raise RuntimeError(f"expected one wheel in {dist_dir}, found: {wheels}")
    return wheels[0]


def _hash(data: bytes) -> str:
    digest = hashlib.sha256(data).digest()
    return base64.urlsafe_b64encode(digest).rstrip(b"=").decode("ascii")


def _record_bytes(records: list[tuple[str, str, str]]) -> bytes:
    stream = io.StringIO(newline="")
    writer = csv.writer(stream)
    writer.writerows(records)
    return stream.getvalue().encode("utf8")


def _is_forbidden(path: Path) -> bool:
    parts = set(path.parts)
    return "__pycache__" in parts or path.name in {".DS_Store"} or path.suffix == ".pyc"
