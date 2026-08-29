#!/usr/bin/env python3
"""Verify that recorded submodule commits are reachable from configured remote branches."""

from __future__ import annotations

import argparse
import subprocess
import tempfile
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


class ReachabilityError(RuntimeError):
    """Raised when a recorded submodule commit is not remotely reachable."""


@dataclass(frozen=True)
class SubmoduleSpec:
    """Remote reachability inputs for one recorded submodule gitlink."""

    name: str
    path: str
    url: str
    branch: str
    commit: str


def _git(*args: str, cwd: Path | None = None, check: bool = True) -> subprocess.CompletedProcess[str]:
    """Run Git with captured text output."""
    return subprocess.run(
        ["git", *args],
        cwd=cwd,
        check=check,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )


def _config(root: Path, key: str) -> str:
    """Read one required value from the worktree's .gitmodules file."""
    result = _git("config", "-f", str(root / ".gitmodules"), "--get", key, check=False)
    value = result.stdout.strip()
    if result.returncode != 0 or not value:
        raise ReachabilityError(f"missing .gitmodules value: {key}")
    return value


def load_spec(root: Path, name: str, revision: str | None = None) -> SubmoduleSpec:
    """Load a submodule path, remote branch, and prospective or recorded commit."""
    prefix = f"submodule.{name}"
    path = _config(root, f"{prefix}.path")
    url = _config(root, f"{prefix}.url")
    branch = _config(root, f"{prefix}.branch")
    if revision is None:
        current = _git("rev-parse", "HEAD", cwd=root / path, check=False)
        commit = current.stdout.strip()
        if current.returncode != 0 or len(commit) != 40:
            raise ReachabilityError(f"{name}: cannot resolve the current {path} commit")
    else:
        tree = _git("ls-tree", revision, "--", path, cwd=root, check=False)
        fields = tree.stdout.strip().split(maxsplit=3)
        if tree.returncode != 0 or len(fields) != 4 or fields[0:2] != ["160000", "commit"]:
            raise ReachabilityError(f"{revision}:{path} is not a submodule gitlink")
        commit = fields[2]
    return SubmoduleSpec(name=name, path=path, url=url, branch=branch, commit=commit)


def advertised_tip(spec: SubmoduleSpec) -> str:
    """Return the exact commit advertised for the configured remote branch."""
    ref = f"refs/heads/{spec.branch}"
    result = _git("ls-remote", "--exit-code", spec.url, ref, check=False)
    rows = [line.split() for line in result.stdout.splitlines() if line.strip()]
    if result.returncode != 0 or len(rows) != 1 or len(rows[0]) != 2 or rows[0][1] != ref:
        detail = result.stderr.strip() or "branch is not advertised"
        raise ReachabilityError(f"{spec.name}: cannot resolve {spec.url} {ref}: {detail}")
    return rows[0][0]


def verify_spec(spec: SubmoduleSpec) -> None:
    """Verify that the gitlink commit is contained in its advertised remote branch."""
    tip = advertised_tip(spec)
    if tip == spec.commit:
        return
    ref = f"refs/heads/{spec.branch}"
    with tempfile.TemporaryDirectory(prefix="datoviz-submodule-reachability-") as temporary:
        object_store = Path(temporary) / "objects.git"
        _git("init", "--bare", "--quiet", str(object_store))
        fetch = _git(
            "--git-dir",
            str(object_store),
            "fetch",
            "--quiet",
            "--no-tags",
            spec.url,
            f"+{ref}:refs/remotes/reachability/{spec.branch}",
            check=False,
        )
        if fetch.returncode != 0:
            detail = fetch.stderr.strip() or "remote branch fetch failed"
            raise ReachabilityError(f"{spec.name}: cannot fetch {ref}: {detail}")
        contained = _git(
            "--git-dir",
            str(object_store),
            "merge-base",
            "--is-ancestor",
            spec.commit,
            tip,
            check=False,
        )
        if contained.returncode != 0:
            raise ReachabilityError(
                f"{spec.name}: gitlink {spec.commit} is not reachable from "
                f"{ref} at {tip}"
            )


def parse_args() -> argparse.Namespace:
    """Parse command-line arguments."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("submodules", nargs="+", help="submodule names from .gitmodules")
    parser.add_argument(
        "--revision",
        help="parent revision to inspect instead of the current submodule checkout",
    )
    return parser.parse_args()


def main() -> int:
    """Verify every requested submodule and report its remote branch containment."""
    args = parse_args()
    try:
        specs = [load_spec(ROOT, name, args.revision) for name in args.submodules]
        for spec in specs:
            verify_spec(spec)
            print(
                f"{spec.name}: {spec.commit} is reachable from "
                f"{spec.url} refs/heads/{spec.branch}"
            )
    except ReachabilityError as exc:
        print(f"ERROR: {exc}")
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
