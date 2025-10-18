# Repository Guidelines

## Project Structure & Module Organization
Datoviz combines a C/Vulkan core with Python bindings. Core rendering code lives in `src/` (C/C++) with public headers in `include/`. The generated Python package sits in `datoviz/`, while example scripts and demos are under `examples/`. Tests are split between GPU integration harnesses in `tests/` and Python API suites in `pytests/`. Assets such as shaders and Vulkan utilities are in `libs/` and `data/`. Build artifacts land in `build/`; avoid committing this directory. Automation recipes live in the root `justfile`.

## Build, Test, and Development Commands
- `just build` — configure via CMake and compile the native library (run twice on fresh checkouts to satisfy msdf dependency).
- `pip install -e .` — install editable Python bindings for local development.
- `just demo` — launch a curated GPU demo to verify a working runtime.
- `just pytest` — execute `pytest` against `tests.py`, covering high-level Python flows.
- `just test test_name=scene` — run a specific C test from `tests/` using the compiled CLI; skip the argument to execute the full suite.

## Coding Style & Naming Conventions
All C/C++ sources are formatted with the repo’s `.clang-format`; configure editors for 4-space indents and banner comments separating sections. Functions and structs use the `dvz_` prefix and uppercase `DVZ_` enums to mirror the public API. Python modules follow Ruff-enforced style (`ruff format` and `ruff check`), keeping lines ≤99 characters, snake_case names, and type-annotated attributes. Generated files (e.g., `datoviz/_ctypes_.py`) must not be edited by hand.

## Testing Guidelines
Prefer `pytest` for Python work; new tests belong in `pytests/` with `test_*.py` filenames. GPU-heavy checks should default to the CLI harness: add cases under `tests/` and trigger them with `just test`. Record expected frame grabs using `DVZ_CAPTURE_PNG` when validating rendering changes. Document any hardware assumptions (driver, GPU model) in the PR if deviations or skips are required.

## Commit & Pull Request Guidelines
Recent history shows imperative, scoped commit subjects (`Set offset field in DvzViewport`, `Avoid resizing non-existing visual...`) and issue references via `#123`; follow that pattern and keep bodies concise. Open PRs against `dev`, summarizing intent, user-visible effects, and test coverage. Include reproduction steps or screenshots for visual tweaks, link related issues, and confirm `just build`/`just pytest` passed before requesting review.
