"""Explicit public module for the raw Datoviz ctypes API."""

try:
    from ._ctypes import *  # noqa: F403
except ModuleNotFoundError as exc:
    package = __package__ or 'datoviz'
    if exc.name != f'{package}._ctypes':
        raise
    raise RuntimeError(
        'Datoviz raw ctypes bindings have not been generated. Run `just ctypes` from the '
        'repository root, or install a package that includes datoviz/_ctypes.py.'
    ) from exc
