#!/usr/bin/env python3
"""Compose a Datoviz documentation deployment with a preserved legacy site."""

from __future__ import annotations

import argparse
import re
import shutil
from pathlib import Path


TEXT_SUFFIXES = {".css", ".html", ".js", ".json", ".txt", ".xml"}
MEDIA_ASSET_PREFIXES = ("assets/gallery/v0.4/", "assets/tutorials/vulkan/")
MEDIA_REFERENCE_RE = re.compile(r"(?:data-src|poster|src)=[\"']([^\"']+)[\"']")


def _copy_tree(source: Path, destination: Path) -> None:
    shutil.copytree(source, destination, ignore=shutil.ignore_patterns(".git", ".DS_Store"))


def _rewrite_legacy_urls(root: Path, prefix: str) -> None:
    absolute_prefix = f"/{prefix}/"
    public_prefix = f"https://datoviz.org/{prefix}/"
    for path in sorted(root.rglob("*")):
        if not path.is_file() or path.suffix.lower() not in TEXT_SUFFIXES:
            continue
        try:
            text = path.read_text(encoding="utf8")
        except UnicodeDecodeError:
            continue
        rewritten = text.replace("https://datoviz.org/", public_prefix)
        for attribute in ("action", "href", "src"):
            rewritten = rewritten.replace(f'{attribute}="/', f'{attribute}="{absolute_prefix}')
            rewritten = rewritten.replace(f"{attribute}='/", f"{attribute}='{absolute_prefix}")
        rewritten = rewritten.replace("url(/", f"url({absolute_prefix}")
        if rewritten != text:
            path.write_text(rewritten, encoding="utf8")


def _validate_media_asset_references(root: Path) -> None:
    missing: set[str] = set()
    for path in sorted(root.rglob("*.html")):
        text = path.read_text(encoding="utf8", errors="ignore")
        for reference in MEDIA_REFERENCE_RE.findall(text):
            for prefix in MEDIA_ASSET_PREFIXES:
                if prefix not in reference:
                    continue
                relative = reference.split(prefix, 1)[1]
                relative = relative.split("?", 1)[0].split("#", 1)[0]
                asset = root / prefix / relative
                if not asset.is_file():
                    missing.add(asset.relative_to(root).as_posix())
    if missing:
        details = "\n  ".join(sorted(missing))
        raise ValueError(f"generated site references missing media:\n  {details}")


def stage_deployment(
    built_site: Path,
    site_repo: Path,
    output: Path,
    *,
    legacy_prefix: str,
    seed_legacy: bool,
) -> None:
    built_site = built_site.resolve()
    site_repo = site_repo.resolve()
    output = output.resolve()
    if not built_site.is_dir():
        raise ValueError(f"built site does not exist: {built_site}")
    if not (site_repo / ".git").exists():
        raise ValueError(f"website checkout is not a Git repository: {site_repo}")
    if output.exists():
        raise ValueError(f"output path must not exist: {output}")
    if not legacy_prefix or "/" in legacy_prefix or legacy_prefix in {".", ".."}:
        raise ValueError(f"invalid legacy prefix: {legacy_prefix!r}")

    _copy_tree(built_site, output)
    (output / ".nojekyll").touch()
    _validate_media_asset_references(output)

    existing_legacy = site_repo / legacy_prefix
    staged_legacy = output / legacy_prefix
    if existing_legacy.is_dir():
        _copy_tree(existing_legacy, staged_legacy)
    elif seed_legacy:
        _copy_tree(site_repo, staged_legacy)
        for control_file in (".nojekyll", "CNAME"):
            path = staged_legacy / control_file
            if path.exists():
                path.unlink()
        _rewrite_legacy_urls(staged_legacy, legacy_prefix)
    else:
        raise ValueError(
            f"legacy archive is missing at {existing_legacy}; use --seed-legacy for the first cutover"
        )


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--built-site", type=Path, required=True)
    parser.add_argument("--site-repo", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--legacy-prefix", default="v0.3")
    parser.add_argument("--seed-legacy", action="store_true")
    args = parser.parse_args()
    stage_deployment(
        args.built_site,
        args.site_repo,
        args.output,
        legacy_prefix=args.legacy_prefix,
        seed_legacy=args.seed_legacy,
    )
    print(args.output.resolve())
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
