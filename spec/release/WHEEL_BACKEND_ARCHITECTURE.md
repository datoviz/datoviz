# Wheel Backend Architecture Plan

Status: proposed aggressive refactor.

This plan describes the direct path to the preferred long-term Datoviz wheel architecture. It is
not the minimal RC workaround. The goal is to remove the current split between wheel staging,
`pip wheel`, post-build retagging, platform repair, and installed-smoke scripts, and replace it
with a first-class Datoviz wheel backend that produces the intended platform wheel directly.


## Motivation

Datoviz v0.4 wheels are Python-ABI independent but platform dependent:

```text
py3-none-<platform>
```

The Python package uses generated `ctypes` bindings and does not contain a CPython extension module.
The wheel still carries native runtime payloads: `libdatoviz`, public C headers, CMake package
files, and platform runtime libraries such as shaderc, Vulkan loader, or MoltenVK when policy
requires them.

Generic `cibuildwheel` expects the Python build backend to emit a platform wheel by itself. The
current Datoviz path instead creates a `py3-none-any` wheel, then retags and repairs it. That is a
valid transitional implementation, but it is not the best long-term architecture and causes CI to
fight the packaging model.


## Target Architecture

Create a dedicated Datoviz wheel backend under `tools/`:

```text
tools/datoviz_build_backend/
  __init__.py
  backend.py
  wheel.py
  native_payload.py
  repair.py
  tags.py
  validate.py
```

The backend should be usable through PEP 517 with `backend-path`, for example:

```toml
[build-system]
requires = ["packaging", "wheel"]
build-backend = "datoviz_build_backend.backend"
backend-path = ["tools"]
```

Its `build_wheel()` implementation should directly emit the final artifact:

```text
datoviz-<version>-py3-none-macosx_11_0_arm64.whl
datoviz-<version>-py3-none-manylinux_2_34_x86_64.whl
datoviz-<version>-py3-none-win_amd64.whl
```

It should not depend on first producing `py3-none-any` and retagging as an external post-process.


## Backend Responsibilities

The backend owns the complete release-wheel contract:

1. discover or accept the target platform tag;
2. verify that the native build tree exists and was configured with required release options such
   as `DVZ_ENABLE_SHADERC=ON`;
3. collect Python package files;
4. collect public C headers and CMake package files;
5. collect `libdatoviz` and any split runtime libraries required by the platform build;
6. collect `dlopen()` runtime dependencies from explicit runtime directories, especially shaderc;
7. write wheel metadata with `Root-Is-Purelib: true` and a `py3-none-<platform>` tag;
8. build the wheel archive with the final platform tag from the start;
9. run platform repair or dependency inspection;
10. validate wheel contents and run installed smoke checks.

The backend must preserve the important distinction that source installs are ordinary Python source
installs, while release-wheel builds require an existing native Datoviz build tree and platform
runtime payload policy.


## Configuration Model

Move wheel policy into explicit project configuration, not shell fragments spread across CI.

Example shape:

```toml
[tool.datoviz.wheel]
platform-specific = true
native-build-dir = "build"
include-headers = true
include-cmake-package = true
runtime-dirs-env = "DVZ_WHEEL_RUNTIME_DIRS"
require-shaderc = true

[tool.datoviz.wheel.macos]
platform-tags = ["macosx_10_13_x86_64", "macosx_11_0_arm64"]
repair-tool = "delocate"
runtime-patterns = ["libshaderc*.dylib", "libvulkan*.dylib", "libMoltenVK.dylib"]

[tool.datoviz.wheel.linux]
platform-tags = ["manylinux_2_34_x86_64", "manylinux_2_34_aarch64"]
repair-tool = "auditwheel"
runtime-patterns = ["libshaderc*.so*"]

[tool.datoviz.wheel.windows]
platform-tags = ["win_amd64", "win_arm64"]
repair-tool = "delvewheel"
runtime-patterns = ["*.dll"]
```

The exact TOML format can evolve, but the policy should be declarative enough that local release
commands and GitHub Actions consume the same source of truth.


## Platform Validation

The backend should fail early when a wheel tag would be dishonest.

macOS:

1. require `MACOSX_DEPLOYMENT_TARGET` for release-tag builds;
2. inspect bundled dylibs for minimum macOS version;
3. reject `macosx_11_0_arm64` if any bundled dylib requires a newer target;
4. run `delocate-wheel` or equivalent repair after the early check.

Linux:

1. build inside the intended manylinux image or an equivalent controlled environment;
2. run `auditwheel show` and repair as required;
3. reject unresolved native dependencies that are not allowed by policy.

Windows:

1. collect Datoviz and required runtime DLLs explicitly;
2. run `delvewheel show` or repair;
3. reject missing runtime dependencies before upload.


## CI Shape

GitHub Actions should stop using `cibuildwheel` as the wheel builder for Datoviz. CI should build
one wheel per platform/architecture and then smoke-test the resulting `py3-none-<platform>` wheel
across the supported Python versions.

The build job should look conceptually like:

```sh
just build
python -m build --wheel --config-setting=platform-tag=<platform-tag>
python -m tools.datoviz_build_backend.validate \
  --wheel dist/datoviz-*.whl \
  --shaderc \
  --cmake-consumer \
  --qt-probe optional
```

The install-smoke jobs should download the artifact and run the same installed checks on Python
3.10, 3.11, 3.12, 3.13, and 3.14. Python 3.15 remains a prerelease, non-blocking lane until it is
appropriate for ordinary users.


## Migration Plan

This is an aggressive refactor, but it should still land in controlled commits:

1. Add `tools/datoviz_build_backend/` with a backend that reproduces the current wheel output.
2. Move platform-tag selection, payload collection, repair, and validation from the current
   scripts into backend modules.
3. Make `just wheel-stage`, `just wheel-build`, `just wheel-validate`, and `just wheel-check`
   delegate to the backend so local workflows stay stable while internals change.
4. Switch `.github/workflows/wheels.yml` from `cibuildwheel` to the backend.
5. Add focused tests for payload manifests, tag validation, missing-runtime diagnostics, and macOS
   deployment-target rejection.
6. Remove duplicated legacy wheel code after local and CI parity is proven.
7. Update maintainer docs to describe the backend as the sole wheel authority.


## Non-Goals

1. Do not add a CPython extension module only to satisfy generic wheel tooling.
2. Do not change the wheel tag to `cp<version>-...`; the Python layer remains ABI-independent.
3. Do not claim an older macOS, manylinux, or Windows support tag than the bundled native payloads
   actually support.
4. Do not vendor SDK/runtime payloads in git.
5. Do not make source installs depend on a local native build tree unless the user is explicitly
   building a release wheel.


## Acceptance Criteria

The refactor is complete when:

1. `python -m build --wheel --config-setting=platform-tag=<tag>` produces
   `datoviz-<version>-py3-none-<tag>.whl` directly;
2. local `just wheel-*` commands are thin wrappers around the backend;
3. GitHub Actions no longer invokes `cibuildwheel` for Datoviz wheel construction;
4. Linux, macOS, and Windows wheels build, inspect, and install-smoke on the declared release
   matrix;
5. shaderc runtime GLSL compilation passes from the installed wheel;
6. CMake consumer smoke passes from the installed wheel;
7. optional Qt probing remains optional and reports clear diagnostics when PyQt/Qt support is
   absent;
8. deployment-target mismatches fail before upload with a clear, platform-specific error.
