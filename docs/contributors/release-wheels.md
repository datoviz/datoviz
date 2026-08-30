# Release Wheels

This page is for maintainers preparing v0.4 release-candidate wheels. User-facing installation
instructions belong in the install docs. Low-level implementation notes live next to the scripts in
`tools/release_wheels/`.


## Current Policy

Datoviz v0.4 wheels include the Python package, the generated `ctypes` binding, public C
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

Shader and Vulkan dependency policy:

- Source builds default to `DVZ_ENABLE_SHADERC=AUTO`, so runtime GLSL compilation is enabled when shaderc headers are present and unavailable otherwise.
- Official release wheels must configure with `DVZ_ENABLE_SHADERC=ON`.
- Official release wheels must configure with `DVZ_REQUIRE_PRECOMPILED_SHADERS=ON` and `DVZ_VALIDATE_SPIRV=ON`; their build environments require `glslc` and `spirv-val`.
- Official release wheels must use a Release native build and pass the installed
  `--release-build` check; a native version containing `(DEBUG)` is not publishable.
- Official release wheels disable mimalloc. A wheel loads Datoviz into an existing interpreter,
  where overriding the process allocator can cross unsafe allocation boundaries with dynamically
  loaded dependencies such as shaderc.
- Do not commit vendored SDK or runtime payloads such as Vulkan SDK libraries or shaderc dylibs.
- On macOS wheels, `delocate` repairs linked dylib dependencies. `DVZ_WHEEL_RUNTIME_DIRS` is only
  for libraries Datoviz loads with `dlopen()` at runtime.
- `glslc` and `spirv-val` are build/test tools and are not part of the Datoviz runtime GLSL compilation path.

Required Python install-smoke versions are 3.10, 3.11, 3.12, 3.13, and 3.14. Python 3.15 is a
prerelease smoke lane and should stay non-blocking until it is appropriate for normal PyPI users.

Target platform artifacts are:

| OS | Architecture | Wheel platform tag |
|---|---:|---|
| Linux | `x86_64` | `manylinux_2_34_x86_64` |
| Linux | `aarch64` | `manylinux_2_34_aarch64` |
| macOS | `x86_64` | `macosx_15_0_x86_64` |
| macOS | `arm64` | `macosx_15_0_arm64` |
| Windows | `AMD64` | `win_amd64` |
| Windows | `ARM64` | `win_arm64` |


## Local Commands

Use the primary `just` recipes from the repository root.

| Task | Command |
|---|---|
| Print the intended matrix | `just wheel-matrix` |
| Stage the wheel tree from `build/` | `just wheel-stage --clean` |
| Build the platform wheel | `just wheel-build --platform-tag manylinux_2_34_x86_64` |
| Validate wheel version and tag | `just wheel-validate --platform-tag manylinux_2_34_x86_64` |
| Inspect wheel contents | `just wheel-inspect` |
| Inspect native dependencies | `just wheel-inspect --native-deps` |
| Install-smoke the wheel | `just wheel-check --cmake-consumer --qt-probe optional` |
| Run the local CI-parity path | `just wheel-ci-local manylinux_2_34_x86_64` |
| Run local CI parity after rebuilding native code | `just wheel-ci-local manylinux_2_34_x86_64 1` |
| Run the local manylinux Docker proof | `just wheel-manylinux-docker x86_64` |
| Check a local TestPyPI candidate | `just testpypi-check manylinux_2_34_x86_64` |
| Upload one local wheel to TestPyPI | `just testpypi-upload manylinux_2_34_x86_64 dist yes` |

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

For macOS release-tag proof, set the deployment target before building native code and every
packaged dependency. The wheel platform tag must not claim an older macOS version than any bundled
dylib supports; `delocate-wheel` enforces this and rejects mismatches. Local host-native proofs may
use the host tag, but release evidence must use the matrix tag.

For the arm64 release target:

```sh
MACOSX_DEPLOYMENT_TARGET=15.0 just build
DVZ_WHEEL_RUNTIME_DIRS="<shaderc-lib>:<vulkan-loader-lib>:<moltenvk-lib>" \
  just wheel-stage --clean
MACOSX_DEPLOYMENT_TARGET=15.0 \
  just wheel-build --platform-tag macosx_15_0_arm64
just wheel-validate --platform-tag macosx_15_0_arm64
just wheel-inspect --native-deps
just wheel-check --shaderc --cmake-consumer --qt-probe optional
```

If `delocate-wheel` reports that `libdatoviz.dylib` or copied dependencies have a newer minimum
macOS target, rebuild those inputs with the intended `MACOSX_DEPLOYMENT_TARGET` or move the proof to
a clean CI/builder environment. Do not retag a newer-target wheel as an older macOS wheel.


## Pipeline Stages

`wheel-stage` copies the release payload into `build/wheel-stage/`:

1. `datoviz/*.py` and `datoviz/experimental/`;
2. `pyproject.toml`;
3. native Datoviz runtime library from `build/`;
4. public headers and CMake package files used by C consumers;
5. platform runtime dependencies that the current wheel policy bundles;
6. `datoviz/_wheel_payload.json`, so the backend and validation tools can audit included files.

`wheel-build` builds from the staged tree, clears stale `dist/datoviz-*.whl` files, and writes the
final `py3-none-<platform>` wheel directly through the Datoviz build backend. It no longer creates
`py3-none-any` first and retags it afterwards. `--skip-repair` is available only for local backend
diagnostics; release evidence must run the platform repair path.

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

The main wheel must still install and import without PyQt. It must not bundle Qt, PyQt, or
`datoviz_qtbridge`. The `datoviz[qt]` extra installs PyQt6 only and is insufficient for hosted
rendering without a compatible native bridge. `datoviz.qt` should fail with a clear diagnostic when
PyQt6, Qt Vulkan support, the optional Qt bridge provider, or platform Vulkan WSI support is
missing.

Maintainer checks should use:

```sh
just wheel-check --cmake-consumer --qt-probe optional
```

Use `--qt-probe required` only on a machine or CI runner where PyQt6, Qt Vulkan support, and the
Datoviz Qt bridge provider are expected to be present.

The Qt/PyQt path remains source-build-only for RC3. Official provider artifacts are deferred to RC4 and remain separate from the base-wheel contract. Source builds may build `datoviz_qtbridge` with `DVZ_ENABLE_QT_BRIDGE=ON` or `AUTO`; local and split provider validation should point `datoviz.qt` at that bridge with `DATOVIZ_QTBRIDGE_LIBRARY`. Conda-forge is the preferred first binary provider channel because it can keep Qt, PyQt, and the bridge on one managed runtime. PyPI provider wheels should wait until the bridge ABI, runtime-version policy, and per-platform Qt library layout are proven.

On a Qt-capable source host, validate the split provider route with:

```sh
DVZ_CMAKE_ARGS="-DDVZ_ENABLE_QT_BRIDGE=ON" just build
DATOVIZ_QTBRIDGE_LIBRARY=build/qtbridge/libdatoviz_qtbridge.so python -m datoviz.qt
DATOVIZ_QTBRIDGE_LIBRARY=build/qtbridge/libdatoviz_qtbridge.so \
  python examples/python/qt/hosted_pyqt.py --smoke-ms 1000
```

Record the PyQt and bridge Qt runtime versions from that lane. If Qt development files, PyQt6
Vulkan bindings, the bridge library, or platform WSI support are missing, record the specific
diagnostic as an environment block rather than treating the base wheel optional probe as provider
proof.


## GitHub Actions Workflow

The v0.4 wheel workflow is manual-only:

```text
.github/workflows/wheels.yml
```

It has no scheduled trigger. Dispatch it only after the local backend scripts and at least one
targeted branch run have proven the path. Keep `.github/workflows-draft/wheels.yml` as a staging
reference for major workflow rewrites.

The workflow uses `python -m pip wheel` with Datoviz release-wheel config settings, not
`cibuildwheel`. The wheel policy source of truth is `[tool.datoviz.wheel]` in `pyproject.toml` and
the backend under `tools/datoviz_build_backend/`.

Before relying on a run for release evidence:

1. `just wheel-ci-local <host-platform-tag>` passes on each maintained host OS;
2. Linux wheels build and inspect on `x86_64` and `aarch64`;
3. macOS wheels build and inspect on `x86_64` and `arm64`;
4. Windows wheels build and inspect on `AMD64` and `ARM64`;
5. host-native Python install smokes pass for Python 3.10 through 3.14;
6. the Python 3.15 prerelease lane remains non-blocking;
7. Qt probing is optional unless the runner explicitly installs a known-good Qt/PyQt stack;
8. artifact names match `wheel-<os>-<arch>`;
9. upload to TestPyPI or PyPI is handled by a separate release workflow.


## Local TestPyPI Rehearsal

Install `twine` and configure a TestPyPI token before uploading:

```sh
python -m pip install --upgrade twine
```

For a single locally built wheel:

```sh
just testpypi-check manylinux_2_34_x86_64
just testpypi-upload manylinux_2_34_x86_64 dist yes
```

For a complete wheelhouse:

```sh
just testpypi-check-all wheelhouse
just testpypi-upload-all wheelhouse yes
```

The upload recipes refuse to run unless the final argument is `yes`. TestPyPI does not allow
overwriting an existing filename/version, so bump the package version before retrying a previously
uploaded candidate.

After the upload, verify the package-index path against the exact successful Wheels run:

```sh
gh workflow run package-index-verification.yml --ref main \
  -f index=testpypi \
  -f version=0.4.0rc2 \
  -f wheel_run_id=<wheels-run-id>
```

`workflow_dispatch` requires the workflow file to exist on the repository default branch. Before
that is true for a release branch, commit
`.github/package-index-verification-request.json` on `main` with the equivalent request:

```json
{
  "index": "testpypi",
  "version": "0.4.0rc2",
  "wheel_run_id": "<wheels-run-id>"
}
```

Changing that request file triggers the same workflow on the release branch. Do not create or
update it until the named version has been uploaded to the selected index.

The six native jobs download Datoviz through the selected package index, require the downloaded
filename and SHA-256 to match the corresponding GitHub Actions wheel artifact exactly, and run a
lightweight clean installed-package import/version/CLI smoke. They intentionally do not repeat the
full rendering and example conformance campaign: exact byte identity transfers that evidence to
the indexed files. The report job publishes one consolidated HTML artifact. Retrieve and open it
with the verification workflow run ID:

```sh
just package-index-report <verification-workflow-run-id>
```

Use the same workflow with `-f index=pypi` after public publication and before switching public
installation documentation to the released package.


## Known Constraints

Cross-arch build and cross-arch execution are different guarantees. Linux `aarch64` and Windows
`ARM64` should be treated as build and inventory coverage unless a native runner is available.

Local native rebuilds inherit local CMake cache options. If `just wheel-ci-local <tag> 1` fails in
`just build`, fix the native build configuration or run the packaging validation from an existing
known-good `build/` tree with:

```sh
just wheel-ci-local <tag>
```

Do not publish artifacts from a dirty or manually patched staging tree. Recreate `build/wheel-stage`
with `just wheel-stage --clean` before release validation.


## Local Manylinux Docker Proof

A normal recent Linux workstation can build and smoke-test a Datoviz wheel, but it may not be valid
`manylinux_2_34` release evidence. For example, an Ubuntu 24.04 native build can emit GLIBC 2.38
symbols, which auditwheel correctly constrains to a newer platform tag.

Use the local manylinux Docker route for Linux x86_64 RC evidence:

```sh
just wheel-manylinux-docker x86_64
```

This is the local route to reuse before dispatching or interpreting Linux wheel CI. A native Ubuntu
wheel may be useful for iteration, but it is not enough for `manylinux_2_34` RC evidence because it
can pick up newer host glibc symbols.

By default this uses the current upstream PyPA base image:

```text
quay.io/pypa/manylinux_2_34_x86_64:latest
```

Override the image only for a known equivalent builder:

```sh
DATOVIZ_MANYLINUX_IMAGE=rossant/datoviz_manylinux:latest just wheel-manylinux-docker x86_64
```

The container script installs the missing RPM packages, builds Datoviz with
`DVZ_ENABLE_SHADERC=ON`, regenerates the Python binding and NumPy-adaptation module, builds the release wheel
through the v0.4 backend, runs auditwheel inspection, and executes the installed-wheel
shaderc/CMake consumer smoke. Output wheels are written to `wheelhouse/` and must not be committed.
Set `DATOVIZ_MANYLINUX_GENERATE_CTYPES=0` only when intentionally checking an already generated
tree.

Keep the upstream PyPA image as the release source of truth unless a refreshed custom image is
needed for build time. If a custom `rossant/datoviz_manylinux` image is used for RC evidence, record
its digest, base image, creation date, and package deltas from the upstream PyPA image.

Keep the local Docker route scoped to `x86_64` until repeated runs are stable. Do not extend the
recipe to `aarch64` only because a platform tag exists; add that path only after a native or
explicitly validated emulated builder can run the installed-wheel smoke.
