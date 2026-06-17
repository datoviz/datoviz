"""Command-line helpers for installed Datoviz packages."""

from __future__ import annotations

import sys
from pathlib import Path

_PKG = Path(__file__).resolve().parent


def _usage() -> str:
    return "Usage: datoviz-config [--cflags] [--libs] [--prefix]"


def main() -> int:
    """Emit compiler and linker flags for the installed Datoviz package."""
    args = set(sys.argv[1:])
    if not args or "--help" in args:
        print(_usage())
        return 0

    unknown = args - {"--cflags", "--libs", "--prefix"}
    if unknown:
        print(f"datoviz-config: unknown option: {sorted(unknown)[0]}", file=sys.stderr)
        print(_usage(), file=sys.stderr)
        return 2

    if "--prefix" in args:
        print(_PKG)
    if "--cflags" in args:
        print(f"-I{_PKG / 'include'}")
    if "--libs" in args:
        rpath = ""
        if sys.platform.startswith("linux"):
            rpath = f" -Wl,-rpath,{_PKG}"
        elif sys.platform == "darwin":
            rpath = f" -Wl,-rpath,{_PKG}"
        print(f"-L{_PKG} -ldatoviz{rpath}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
