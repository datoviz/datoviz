# Release Wheels

This page is for maintainers preparing v0.4 release-candidate wheels. User-facing installation
instructions belong in the install docs. Low-level implementation notes live next to the scripts in
`tools/release_wheels/`.


## Current Policy

Datoviz v0.4 wheels include the Python package, the generated raw `ctypes` binding, public C
headers, CMake package files, and the platform native Datoviz runtime library.

The Python layer loads Datoviz through `ctypes`, so the wheel is independent of the Python C ABI.
Release artifacts are therefore platform wheels tagged:

```text
py3-none-<platform>
```

For example:

```text
datoviz-0.4.0.dev0-py3-none-manylinux_2_34_x86_64.whl
```

Required Python install-smoke versions are 3.10, 3.11, 3.12, 3.13, and 3.14. Python 3.15 is a
prerelease smoke lane and should stay non-blocking until it is appropriate for normal PyPI users.

Target platform artifacts are:

| OS | Architecture | Wheel platform tag |
|---|---:|---|
| Linux | `x86_64` | `manylinux_2_34_x86_64` |
| Linux | `aarch64` | `manylinux_2_34_aarch64` |
| macOS | `x86_64` | `macosx_10_13_x86_64` |
| macOS | `arm64` | `macosx_11_0_arm64` |
| Windows | `AMD64` | `win_amd64` |
| Windows | `ARM64` | `win_arm64` |


## Local Commands

Use the primary `just` recipes from the repository root.

| Task | Command |
|---|---|
| Print the intended matrix | `just wheel-matrix` |
| Stage the wheel tree from `build/` | `just wheel-stage --clean` |
| Build and retag the wheel | `just wheel-build --platform-tag manylinux_2_34_x86_64` |
| Validate wheel version and tag | `just wheel-validate --platform-tag manylinux_2_34_x86_64` |
| Inspect wheel contents | `just wheel-inspect` |
| Inspect native dependencies | `just wheel-inspect --native-deps` |
| Install-smoke the wheel | `just wheel-check --cmake-consumer --qt-probe optional` |
| Run the local CI-parity path | `just wheel-ci-local manylinux_2_34_x86_64` |
| Run local CI parity after rebuilding native code | `just wheel-ci-local manylinux_2_34_x86_64 1` |

The normal local loop is:

```sh
just build
just wheel-stage --clean
just wheel-build --platform-tag manylinux_2_34_x86_64
just wheel-validate --platform-tag manylinux_2_34_x86_64
just wheel-inspect --native-deps
just wheel-check --cmake-consumer --qt-probe optional
```

`just wheel-ci-local` defaults to using the existing `build/` tree. Pass `1` as the second argument
when the native build should be refreshed first. This is useful because packaging changes should be
testable without reconfiguring unrelated local native build options.


## Pipeline Stages

`wheel-stage` copies the release payload into `build/wheel-stage/`:

1. `datoviz/*.py` and `datoviz/experimental/`;
2. `pyproject.toml`;
3. native Datoviz runtime library from `build/`;
4. public headers and CMake package files used by C consumers;
5. platform runtime dependencies that the current wheel policy bundles;
6. `tools/release_wheels/check_wheel.py`, so cibuildwheel can run the installed smoke from the
   staged project.

`wheel-build` builds from the staged tree, clears stale `dist/datoviz-*.whl` files, and retags the
pure Python wheel as `py3-none-<platform>`.

`wheel-validate` checks wheel filenames in `dist/` against the expected project version and platform
tags. With no `--platform-tag`, it expects the full release matrix. Use `--platform-tag` for local
single-platform validation.

`wheel-inspect` lists packaged files and, with `--native-deps`, delegates to the platform dependency
tool.

`wheel-check` creates a clean virtual environment, installs the wheel, and verifies:

1. `import datoviz`;
2. `import datoviz.raw`;
3. `python -m datoviz.cli --prefix`;
4. compiler and linker flags from `datoviz.cli`;
5. CMake package discovery and a tiny C consumer executable;
6. optional render smoke when `--render` is passed;
7. optional or required Qt probe depending on `--qt-probe`.


## Native Dependency Inspection

CI should fail early if the expected inspection or repair tool is unavailable:

| OS | Tool |
|---|---|
| Linux | `auditwheel` |
| macOS | `delocate-listdeps` from `delocate` |
| Windows | `python -m delvewheel show` from `delvewheel` |

The current local `wheel-inspect --native-deps` command reports a missing tool but does not fail.
The draft GitHub Actions workflow installs and checks the expected tool before building wheels.


## Qt And PyQt

The main wheel includes `datoviz.qt`, but PyQt is optional:

```sh
python -m pip install "datoviz[qt]"
```

The main wheel must still install and import without PyQt. `datoviz.qt` should fail with a clear
diagnostic when PyQt6, Qt Vulkan support, the optional Qt bridge provider, or platform Vulkan WSI
support is missing.

Maintainer checks should use:

```sh
just wheel-check --cmake-consumer --qt-probe optional
```

Use `--qt-probe required` only on a machine or CI runner where PyQt6, Qt Vulkan support, and the
Datoviz Qt bridge provider are expected to be present.


## Draft GitHub Actions Workflow

The v0.4 wheel workflow is intentionally not live yet:

```text
.github/workflows-draft/wheels-v04.yml
```

It should stay outside `.github/workflows/` until the local scripts and at least one manual branch
run have proven the path.

Before enabling it:

1. `just wheel-ci-local <host-platform-tag>` passes on each maintained host OS;
2. Linux wheels build and inspect on `x86_64` and `aarch64`;
3. macOS wheels build and inspect on `x86_64` and `arm64`;
4. Windows wheels build and inspect on `AMD64` and `ARM64`;
5. host-native Python install smokes pass for Python 3.10 through 3.14;
6. the Python 3.15 prerelease lane remains non-blocking;
7. Qt probing is optional unless the runner explicitly installs a known-good Qt/PyQt stack;
8. artifact names match `wheel-<os>-<arch>`;
9. upload to TestPyPI or PyPI is handled by a separate release workflow.


## Known Constraints

Cross-arch build and cross-arch execution are different guarantees. Linux `aarch64` can usually be
covered through cibuildwheel with emulation, but Windows `ARM64` should be treated as build and
inventory coverage unless a native ARM64 runner is available.

Local native rebuilds inherit local CMake cache options. If `just wheel-ci-local <tag> 1` fails in
`just build`, fix the native build configuration or run the packaging validation from an existing
known-good `build/` tree with:

```sh
just wheel-ci-local <tag>
```

Do not publish artifacts from a dirty or manually patched staging tree. Recreate `build/wheel-stage`
with `just wheel-stage --clean` before release validation.
