"""PEP 517 backend for Datoviz release wheels."""

from __future__ import annotations

from typing import Any

from setuptools import build_meta as _setuptools

from .config import parse_config_settings, passthrough_config_settings
from .wheel import build_release_wheel


def build_wheel(
    wheel_directory: str,
    config_settings: dict[str, Any] | None = None,
    metadata_directory: str | None = None,
) -> str:
    """Build either an ordinary setuptools wheel or a Datoviz release wheel."""

    config = parse_config_settings(config_settings)
    if not config.release_wheel:
        return _setuptools.build_wheel(
            wheel_directory,
            passthrough_config_settings(config_settings),
            metadata_directory,
        )
    return build_release_wheel(wheel_directory, config, metadata_directory)


def build_sdist(sdist_directory: str, config_settings: dict[str, Any] | None = None) -> str:
    """Delegate source distributions to setuptools."""

    return _setuptools.build_sdist(sdist_directory, passthrough_config_settings(config_settings))


def build_editable(
    wheel_directory: str,
    config_settings: dict[str, Any] | None = None,
    metadata_directory: str | None = None,
) -> str:
    """Delegate editable wheels to setuptools."""

    return _setuptools.build_editable(
        wheel_directory,
        passthrough_config_settings(config_settings),
        metadata_directory,
    )


def get_requires_for_build_wheel(config_settings: dict[str, Any] | None = None) -> list[str]:
    """Return build requirements for ordinary delegated builds."""

    config = parse_config_settings(config_settings)
    if config.release_wheel:
        return []
    return _setuptools.get_requires_for_build_wheel(passthrough_config_settings(config_settings))


def get_requires_for_build_sdist(config_settings: dict[str, Any] | None = None) -> list[str]:
    """Return setuptools source distribution requirements."""

    return _setuptools.get_requires_for_build_sdist(passthrough_config_settings(config_settings))


def get_requires_for_build_editable(config_settings: dict[str, Any] | None = None) -> list[str]:
    """Return setuptools editable build requirements."""

    return _setuptools.get_requires_for_build_editable(passthrough_config_settings(config_settings))


def prepare_metadata_for_build_wheel(
    metadata_directory: str, config_settings: dict[str, Any] | None = None
) -> str:
    """Delegate metadata preparation to setuptools for frontend compatibility."""

    return _setuptools.prepare_metadata_for_build_wheel(
        metadata_directory,
        passthrough_config_settings(config_settings),
    )

