"""Wheel metadata generation for Datoviz release wheels."""

from __future__ import annotations

from email.message import Message
from pathlib import Path

try:
    import tomllib
except ModuleNotFoundError:  # pragma: no cover - Python < 3.11 fallback.
    import tomli as tomllib  # type: ignore[no-redef]


def project_metadata(root: Path) -> dict:
    """Read project metadata from pyproject.toml."""

    data = tomllib.loads((root / "pyproject.toml").read_text(encoding="utf8"))
    return data["project"]


def distribution_name(project: dict) -> str:
    """Return the normalized wheel distribution name."""

    return str(project["name"]).replace("-", "_")


def wheel_version(project: dict) -> str:
    """Return the version string used in wheel filenames."""

    return str(project["version"])


def metadata_text(root: Path) -> str:
    """Generate a Core Metadata file from [project]."""

    project = project_metadata(root)
    msg = Message()
    msg["Metadata-Version"] = "2.1"
    msg["Name"] = str(project["name"])
    msg["Version"] = str(project["version"])
    if project.get("description"):
        msg["Summary"] = str(project["description"])
    if project.get("requires-python"):
        msg["Requires-Python"] = str(project["requires-python"])
    if project.get("license", {}).get("text"):
        msg["License"] = str(project["license"]["text"])
    for author in project.get("authors", []):
        if author.get("name"):
            msg["Author"] = str(author["name"])
        if author.get("email"):
            msg["Author-email"] = str(author["email"])
    for classifier in project.get("classifiers", []):
        msg["Classifier"] = str(classifier)
    keywords = project.get("keywords", [])
    if keywords:
        msg["Keywords"] = ", ".join(str(keyword) for keyword in keywords)
    for dep in project.get("dependencies", []):
        msg["Requires-Dist"] = str(dep)
    optional = project.get("optional-dependencies", {})
    for extra, deps in optional.items():
        msg["Provides-Extra"] = str(extra)
        for dep in deps:
            msg["Requires-Dist"] = f'{dep}; extra == "{extra}"'
    for label, url in project.get("urls", {}).items():
        msg["Project-URL"] = f"{label}, {url}"

    body = ""
    readme = project.get("readme")
    if isinstance(readme, str):
        readme_path = root / readme
        if readme_path.exists():
            if readme_path.suffix.lower() == ".md":
                msg["Description-Content-Type"] = "text/markdown"
            body = "\n" + readme_path.read_text(encoding="utf8")
    return msg.as_string() + body


def wheel_text(tag: str) -> str:
    """Generate the WHEEL metadata file."""

    return (
        "Wheel-Version: 1.0\n"
        "Generator: datoviz_build_backend\n"
        "Root-Is-Purelib: true\n"
        f"Tag: py3-none-{tag}\n"
    )


def entry_points_text(project: dict) -> str:
    """Generate entry_points.txt from [project.scripts]."""

    scripts = project.get("scripts", {})
    if not scripts:
        return ""
    lines = ["[console_scripts]"]
    for name, target in sorted(scripts.items()):
        lines.append(f"{name} = {target}")
    return "\n".join(lines) + "\n"
