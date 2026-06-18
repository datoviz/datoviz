# C/C++ Distribution and Integration

Status: active RC blocker lane. Updated: 2026-06-18.

Keep this file as the short active handoff. Durable user documentation, build options, and release
validation details live in the linked files below.


## Goal

Make Datoviz installable as a normal C/C++ library that downstream projects can consume without
cloning this repository. The v0.4 RC priority is the Python wheel because it carries the first
installed C/C++ integration path; conda-forge, Homebrew, vcpkg, `.deb`, Spack, and other package
manager channels follow from the same source-bundle and install metadata.


## Source Of Truth

- Release validation and evidence:
  [DISTRIBUTION_RELEASE_CHECKLIST.md](DISTRIBUTION_RELEASE_CHECKLIST.md).
- User-facing C/C++ integration:
  [../../docs/how-to/c-integration.md](../../docs/how-to/c-integration.md).
- CMake options, dependency source modes, package smoke presets, and package CI matrix:
  [../../docs/reference/build-options.md](../../docs/reference/build-options.md).
- Wheel build/check tooling:
  [../../docs/contributors/release-wheels.md](../../docs/contributors/release-wheels.md) and
  `tools/release_wheels/`.
- vcpkg overlay draft:
  [../../vcpkg-overlay/README.md](../../vcpkg-overlay/README.md).
- conda-forge draft recipe:
  [../../conda-recipe/README.md](../../conda-recipe/README.md) and
  [../../conda-recipe/meta.yaml](../../conda-recipe/meta.yaml).


## Current Proof

On 2026-06-18 macOS arm64:

- `just c-integration-smoke` passes for both installed-package and FetchContent consumers after
  `d94f72dd6` exported the Vulkan header dependency from `datoviz::datoviz`.
- Vendored and system-auto package smoke/install presets pass.
- Strict Homebrew-style system proof passes with Homebrew `glfw`, `cglm`, `kvazaar`, and
  `mimalloc`; Vulkan still comes from the local Vulkan SDK, not Homebrew `vulkan-loader`.
- `DATOVIZ_SOURCE_DEPS=system just distribution-validate-local source-install` and
  `just distribution-validate-local audit` pass against the strict source install prefix.
- Host-native repaired macOS wheel proof passes for imports, `datoviz-config`, bundled
  headers/CMake files, native dependency inspection, and the wheel CMake consumer. The optional Qt
  probe fails only because PyQt6 is absent in the clean venv.
- Local Linux x86_64 manylinux proof passes with
  `just wheel-manylinux-docker x86_64` against
  `quay.io/pypa/manylinux_2_34_x86_64@sha256:e05e1c4b281f10dc4c3df2b6f546392a0dd4c6383d620c3f8a6c33e19069d056`
  (image created 2026-06-12). The run built
  `wheelhouse/datoviz-0.4.0.dev0-py3-none-manylinux_2_34_x86_64.whl`; auditwheel kept the
  `manylinux_2_34_x86_64` tag; installed-wheel import, `datoviz.cli`, shaderc GLSL compilation,
  and CMake consumer checks passed; optional Qt probe failed only because PyQt6 was absent.
- Local conda render/build proof passes from a generated release source bundle after the Python
  recipe scripts force `PIP_USER=false`: `libdatoviz` and `datoviz` packages build, and the package
  test imports `datoviz.raw` and creates/destroys a scene without a Vulkan device.
- Local release-target proof for `macosx_11_0_arm64` remains blocked by this host's newer macOS 15
  deployment target in native outputs and Homebrew dependencies; prove it in CI or an older-target
  builder.


## Implemented Surface

- `include/datoviz.h` is the preferred public umbrella include.
- Public C headers parse from C++ through the public header probes.
- Installed CMake consumers use `find_package(datoviz CONFIG REQUIRED)` and link
  `datoviz::datoviz`.
- FetchContent consumers link the same target, while tests, examples, and install/export rules
  default to disabled when Datoviz is embedded as a subproject.
- `datoviz-config` is installed by wheels for GCC-compatible Linux, macOS, and MSYS2/MinGW shells.
  MSVC users use CMake, not `datoviz-config`.
- Wheels bundle public headers and a relocatable wheel-local `DatovizConfig.cmake`.
- System installs generate `datoviz.pc` and installed CMake package metadata.
- `DVZ_VENDORED_DEPS=OFF` plus per-dependency source modes supports package-manager builds for
  GLFW, cglm, Kvazaar, and mimalloc where those packages are available. `msdf-atlas-gen` remains
  source/vendored-only for v0.4.
- Draft vcpkg overlay and conda-forge split recipe exist. The conda draft currently depends on
  `cglm` and disables Kvazaar because direct conda-forge repodata did not find a usable `kvazaar`
  package for the target platforms.
- `.github/workflows/wheels.yml` has Linux, macOS, and Windows wheel jobs plus installed-wheel
  Python/CMake consumer smokes.


## Active Blockers

1. Prove release-target macOS wheels in CI or an older-target builder, especially
   `macosx_11_0_arm64`.
2. Prove Windows AMD64 first, then Windows ARM64: build wheel, inspect native dependencies, install
   in a clean environment, and run the CMake consumer check.
3. Validate the vcpkg overlay on Windows with vcpkg installed; replace placeholder source-bundle
   SHA512 values only after tagging/release-asset publication.
4. Confirm conda recipe dependency names, Unix paths, and Windows DLL layout in staged-recipes or
   feedstock CI logs; local macOS arm64 render/build is green.
5. Keep Homebrew, `.deb`, Spack, rpm, and conan behind the wheel/conda/vcpkg proof unless release
   scope changes.


## Preferred Validation

Local source/package proof:

```sh
just c-integration-smoke
DATOVIZ_SOURCE_DEPS=system just distribution-validate-local source-install
just distribution-validate-local audit
```

Local wheel proof:

```sh
just build
just ctypes
python tools/release_wheels/stage_wheel.py --clean
python tools/release_wheels/build_wheel.py --dist-dir wheelhouse --platform-tag <platform-tag>
just wheel-inspect --wheel wheelhouse/*.whl --native-deps
python tools/release_wheels/check_wheel.py --wheel wheelhouse/*.whl --cmake-consumer --qt-probe optional
```

Windows AMD64 starting point:

```sh
set VCPKG_ROOT=C:/vcpkg
set VCPKG_BINARY_SOURCES=clear;files,C:/vcpkg-binary-cache,readwrite
set DVZ_CMAKE_ARGS=-DDVZ_ENABLE_SHADERC=ON
just build
just ctypes
python tools/release_wheels/stage_wheel.py --clean
python tools/release_wheels/build_wheel.py --dist-dir wheelhouse --platform-tag win_amd64
just wheel-inspect --wheel wheelhouse/*.whl --native-deps
python tools/release_wheels/check_wheel.py --wheel wheelhouse/*.whl --cmake-consumer --qt-probe optional
```

Before finalizing edits in this lane, run:

```sh
git diff --check
git status --short
```


## Guardrails

- Do not stage or commit `data` submodule state unless explicitly approved in the current turn.
- Do not commit generated binaries, vendored runtime libraries, or wheel artifacts unless
  explicitly approved.
- Do not duplicate long implementation recipes here. Link to maintained docs/checklists instead.
- Keep wheel, conda, and vcpkg release paths tied to the same explicit source bundle; do not use
  GitHub auto-generated archives for package-manager release proof because they omit required
  submodule contents.
