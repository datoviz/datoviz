# Wheel Backend Architecture Plan

Status: proposed aggressive refactor, implementation-ready after local owner approval.

This plan describes the direct path to the preferred long-term Datoviz wheel architecture. It is
not the minimal RC workaround. The goal is to remove the current split between wheel staging,
`pip wheel`, post-build retagging, platform repair, and installed-smoke scripts, and replace it
with a first-class Datoviz wheel backend that produces the intended platform wheel directly.

The implementation should promote the existing `tools/release_wheels/` package into the backend
rather than start from a blank design. The current scripts already encode useful payload, matrix,
inspection, and installed-smoke knowledge; the refactor should make that knowledge the release
wheel authority.


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
  config.py
  metadata.py
  wheel.py
  native_payload.py
  manifest.py
  repair.py
  tags.py
  validate.py
```

The backend should be usable through PEP 517 with `backend-path`, for example:

```toml
[build-system]
requires = ["packaging", "setuptools", "tomli; python_version < '3.11'", "wheel"]
build-backend = "datoviz_build_backend.backend"
backend-path = ["tools"]
```

Normal Python source installs, editable installs, and source distributions must remain ordinary
Python builds. The backend should delegate those paths to `setuptools.build_meta` unless release
wheel mode is explicitly requested. Release wheel mode is entered only when the frontend passes a
namespaced config setting such as:

```sh
python -m build --wheel \
  --config-setting=datoviz.release-wheel=true \
  --config-setting=datoviz.platform-tag=macosx_15_0_arm64
```

In release wheel mode, `build_wheel()` should directly emit the final artifact:

```text
datoviz-<version>-py3-none-macosx_15_0_arm64.whl
datoviz-<version>-py3-none-manylinux_2_34_x86_64.whl
datoviz-<version>-py3-none-win_amd64.whl
```

It should not depend on first producing `py3-none-any` and retagging as an external post-process.
Any platform repair that rewrites wheel contents or filenames is part of `build_wheel()` and the
returned artifact path must point to the repaired, final wheel.

The backend may call `setuptools.build_meta` internally to generate metadata, but the external
release-wheel contract is owned by Datoviz.


## Backend Responsibilities

The backend owns the complete release-wheel contract:

1. discover or accept the target platform tag;
2. verify that the native build tree exists and was configured with required release options such
   as `DVZ_ENABLE_SHADERC=ON`;
3. collect Python package files;
4. collect public C headers and CMake package files;
5. collect `libdatoviz` and any split runtime libraries required by the platform build;
6. collect `dlopen()` runtime dependencies from explicit runtime directories, especially shaderc;
7. write or delegate wheel metadata with `Root-Is-Purelib: true` and a
   `py3-none-<platform>` tag;
8. write a payload manifest that records every included native/header/CMake/runtime file;
9. build the wheel archive with the final platform tag from the start;
10. run platform repair as part of wheel construction when required;
11. validate wheel contents and run installed smoke checks.

The backend must preserve the important distinction that source installs are ordinary Python source
installs, while release-wheel builds require an existing native Datoviz build tree and platform
runtime payload policy.


## Source Builds And Metadata

The backend must not make ordinary `pip install .`, editable installs, or source distributions
depend on a configured native build tree. Required behavior:

1. `build_sdist()` delegates to setuptools and produces the normal source distribution.
2. `build_editable()` delegates to setuptools when available.
3. `build_wheel()` without `datoviz.release-wheel=true` delegates to setuptools and keeps the
   existing source-install behavior.
4. `build_wheel()` with `datoviz.release-wheel=true` uses the Datoviz release-wheel path and
   requires a native build tree.

Project metadata remains sourced from `[project]` in `pyproject.toml`: name, version,
dependencies, optional dependencies, entry points, classifiers, URLs, license, and readme. The
backend must either delegate metadata generation to setuptools or have tests that compare generated
`METADATA`, entry points, and dependency metadata against the setuptools output.

The wheel writer must also produce valid `WHEEL` and `RECORD` files. Validation must check
`RECORD` hashes, `Root-Is-Purelib`, and the exact `py3-none-<platform>` tag.


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
payload-manifest = "datoviz/_wheel_payload.json"

[tool.datoviz.wheel.macos]
platform-tags = ["macosx_15_0_x86_64", "macosx_15_0_arm64"]
repair-tool = "delocate"
runtime-patterns = [
  "libshaderc*.dylib",
  "libvulkan*.dylib",
  "libMoltenVK.dylib",
  "MoltenVK_icd.json",
]

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

Release wheel config settings should be namespaced:

```text
datoviz.release-wheel=true
datoviz.platform-tag=<platform-tag>
datoviz.native-build-dir=<path>
datoviz.dist-dir=<path>
datoviz.include-qtbridge=true|false
datoviz.skip-repair=true|false
```

Unknown `datoviz.*` config settings should fail with a clear error. Non-Datoviz settings should be
passed through to the delegated setuptools backend on non-release builds.


## Payload Manifest

The release path should create a manifest before archiving the wheel. Each entry should record:

1. source path;
2. wheel archive path;
3. kind: `python`, `header`, `cmake`, `libdatoviz`, `runtime`, `qtbridge`, or `metadata`;
4. required or optional status;
5. policy reason, such as `core-runtime`, `shaderc-runtime`, `vulkan-loader`, `moltenvk`, or
   `cmake-consumer`;
6. platform repair status when repair changes the file.

The manifest gives reviewers and future automation a stable way to detect missing payloads,
unexpected runtime libraries, accidental generated files, or unrelated assets. Validation should
reject `__pycache__`, `.DS_Store`, source-tree build artifacts outside the declared policy, and
unapproved generated/runtime binary payloads.


## Platform Validation

The backend should fail early when a wheel tag would be dishonest.

macOS:

1. require `MACOSX_DEPLOYMENT_TARGET` for release-tag builds;
2. inspect bundled dylibs for minimum macOS version;
3. reject a macOS wheel tag if any bundled dylib requires a newer target;
4. run `delocate-wheel` or equivalent repair inside `build_wheel()`;
5. revalidate the repaired wheel filename, contents, and native dependencies.

Linux:

1. build inside the intended manylinux image or an equivalent controlled environment;
2. run `auditwheel show`;
3. run `auditwheel repair` inside `build_wheel()` when required;
4. reject unresolved native dependencies that are not allowed by policy;
5. revalidate the repaired wheel filename, contents, and native dependencies.

Windows:

1. collect Datoviz and required runtime DLLs explicitly;
2. run `delvewheel show`;
3. run `delvewheel repair` inside `build_wheel()` when required;
4. reject missing runtime dependencies before upload;
5. revalidate the repaired wheel filename, contents, and native dependencies.


## CI Shape

GitHub Actions should stop using `cibuildwheel` as the wheel builder for Datoviz. CI should build
one wheel per platform/architecture and then smoke-test the resulting `py3-none-<platform>` wheel
across the supported Python versions.

The build job should look conceptually like:

```sh
just build
python -m build --wheel \
  --config-setting=datoviz.release-wheel=true \
  --config-setting=datoviz.platform-tag=<platform-tag>
python -m tools.datoviz_build_backend.validate \
  --wheel dist/datoviz-*.whl \
  --shaderc \
  --cmake-consumer \
  --qt-probe optional
```

The install-smoke jobs should download the artifact and run the same installed checks on Python
3.10, 3.11, 3.12, 3.13, and 3.14. Python 3.15 remains a prerelease, non-blocking lane until it is
appropriate for ordinary users.

The migration must also remove or quarantine the existing `[tool.cibuildwheel]` configuration from
`pyproject.toml` once `.github/workflows/wheels.yml` no longer uses `cibuildwheel`. Keeping stale
cibuildwheel policy beside the new backend would create two apparent sources of truth.


## Migration Plan

This is an aggressive refactor, but it should still land in controlled commits:

1. Add `tools/datoviz_build_backend/` and move reusable code from `tools/release_wheels/` into
   backend modules without changing command behavior.
2. Add config parsing, namespaced config settings, and explicit release-wheel mode.
3. Add setuptools delegation for normal wheel, editable, and sdist builds.
4. Add direct wheel writing with final `py3-none-<platform>` tags, valid metadata, valid
   `RECORD`, and a payload manifest.
5. Move platform-tag selection, payload collection, repair, and validation from the current
   scripts into backend modules.
6. Make `just wheel-stage`, `just wheel-build`, `just wheel-validate`, and `just wheel-check`
   delegate to the backend so local workflows stay stable while internals change.
7. Add focused tests for delegated source builds, metadata parity, payload manifests, tag
   validation, missing-runtime diagnostics, repair invocation, and macOS deployment-target
   rejection.
8. Switch `.github/workflows/wheels.yml` from `cibuildwheel` to the backend.
9. Remove or quarantine `[tool.cibuildwheel]` from `pyproject.toml`.
10. Remove duplicated legacy wheel code after local and CI parity is proven.
11. Update maintainer docs to describe the backend as the sole wheel authority.


## Non-Goals

1. Do not add a CPython extension module only to satisfy generic wheel tooling.
2. Do not change the wheel tag to `cp<version>-...`; the Python layer remains ABI-independent.
3. Do not claim an older macOS, manylinux, or Windows support tag than the bundled native payloads
   actually support.
4. Do not vendor SDK/runtime payloads in git.
5. Do not make source installs depend on a local native build tree unless the user is explicitly
   building a release wheel.
6. Do not silently synthesize missing runtime dependencies; fail with the searched paths and the
   exact policy that required the dependency.


## Acceptance Criteria

The refactor is complete when:

1. the namespaced release-wheel build command shown above produces
   `datoviz-<version>-py3-none-<tag>.whl` directly;
2. local `just wheel-*` commands are thin wrappers around the backend;
3. GitHub Actions no longer invokes `cibuildwheel` for Datoviz wheel construction;
4. Linux, macOS, and Windows wheels build, inspect, and install-smoke on the declared release
   matrix;
5. shaderc runtime GLSL compilation passes from the installed wheel;
6. CMake consumer smoke passes from the installed wheel;
7. optional Qt probing remains optional and reports clear diagnostics when PyQt/Qt support is
   absent;
8. deployment-target mismatches fail before upload with a clear, platform-specific error;
9. ordinary source, editable, and non-release wheel builds do not require a native build tree;
10. wheel `METADATA`, entry points, dependencies, `WHEEL`, and `RECORD` are valid and pass
    `twine check`;
11. wheel validation can run from outside the source checkout and does not depend on repo-local
    files;
12. the payload manifest is present and contains no undeclared runtime libraries, `__pycache__`,
    `.DS_Store`, or unrelated generated artifacts;
13. `[tool.cibuildwheel]` is removed or explicitly marked obsolete after the CI workflow switches
    to the backend.


## Implementation Handoff

A future agent can implement this end to end from this document plus the current
`tools/release_wheels/` code. The intended pickup order is:

1. preserve the current `just wheel-*` user interface;
2. move code into `tools/datoviz_build_backend/` behind compatibility wrappers;
3. add release-wheel mode and setuptools delegation before changing CI;
4. prove one host-native wheel locally;
5. switch GitHub Actions only after local parity passes;
6. delete or quarantine obsolete cibuildwheel configuration last.

The highest-risk details are metadata parity, valid `RECORD` generation, repair tools that rewrite
wheel contents, and keeping normal Python installs independent of native build artifacts.
