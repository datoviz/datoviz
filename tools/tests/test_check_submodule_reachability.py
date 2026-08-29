import subprocess
import sys
from pathlib import Path

import pytest


sys.path.insert(0, str(Path(__file__).parents[1]))

import check_submodule_reachability as reachability


def git(path: Path, *args: str) -> str:
    result = subprocess.run(
        ["git", *args],
        cwd=path,
        check=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    return result.stdout.strip()


def data_remote(tmp_path: Path) -> tuple[Path, Path, str]:
    worktree = tmp_path / "data"
    remote = tmp_path / "data.git"
    worktree.mkdir()
    git(worktree, "init", "--quiet", "--initial-branch=v0.4-dev")
    git(worktree, "config", "user.name", "Reachability Test")
    git(worktree, "config", "user.email", "reachability@example.invalid")
    (worktree / "payload.txt").write_text("first\n", encoding="utf8")
    git(worktree, "add", "payload.txt")
    git(worktree, "commit", "--quiet", "-m", "first")
    first = git(worktree, "rev-parse", "HEAD")
    subprocess.run(["git", "init", "--bare", "--quiet", str(remote)], check=True)
    git(worktree, "remote", "add", "origin", str(remote))
    git(worktree, "push", "--quiet", "-u", "origin", "v0.4-dev")
    return worktree, remote, first


def spec(remote: Path, commit: str) -> reachability.SubmoduleSpec:
    return reachability.SubmoduleSpec(
        name="data",
        path="data",
        url=str(remote),
        branch="v0.4-dev",
        commit=commit,
    )


def test_accepts_advertised_branch_tip(tmp_path):
    _, remote, first = data_remote(tmp_path)

    reachability.verify_spec(spec(remote, first))


def test_load_spec_uses_prospective_submodule_checkout(tmp_path):
    _, remote, first = data_remote(tmp_path)
    (tmp_path / ".gitmodules").write_text(
        '[submodule "data"]\n'
        "\tpath = data\n"
        f"\turl = {remote}\n"
        "\tbranch = v0.4-dev\n",
        encoding="utf8",
    )

    loaded = reachability.load_spec(tmp_path, "data")

    assert loaded.commit == first


def test_accepts_commit_contained_in_advertised_branch(tmp_path):
    worktree, remote, first = data_remote(tmp_path)
    (worktree / "payload.txt").write_text("second\n", encoding="utf8")
    git(worktree, "commit", "--quiet", "-am", "second")
    git(worktree, "push", "--quiet", "origin", "v0.4-dev")

    reachability.verify_spec(spec(remote, first))


def test_rejects_commit_missing_from_advertised_branch(tmp_path):
    worktree, remote, _ = data_remote(tmp_path)
    (worktree / "payload.txt").write_text("unpublished\n", encoding="utf8")
    git(worktree, "commit", "--quiet", "-am", "unpublished")
    unpublished = git(worktree, "rev-parse", "HEAD")

    with pytest.raises(reachability.ReachabilityError, match="not reachable"):
        reachability.verify_spec(spec(remote, unpublished))
