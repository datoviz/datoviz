# Distribution Release Checklist

Status: active release gate. Updated: 2026-06-18.

Use this checklist before dispatching live GitHub Actions, publishing source assets, submitting
package-manager recipes, or uploading release wheels. It keeps the wheel, conda, and vcpkg paths
tied to the same explicit source bundle.

Detailed references:

- Active handoff: [C_DISTRIBUTION.md](C_DISTRIBUTION.md).
- Wheel policy and commands: [../../docs/contributors/release-wheels.md](../../docs/contributors/release-wheels.md).
- C/C++ integration docs: [../../docs/how-to/c-integration.md](../../docs/how-to/c-integration.md).
- Build options and package presets: [../../docs/reference/build-options.md](../../docs/reference/build-options.md).
- Conda recipe draft: [../../conda-recipe/README.md](../../conda-recipe/README.md).
- vcpkg overlay draft: [../../vcpkg-overlay/README.md](../../vcpkg-overlay/README.md).


## Local Preflight

Run the repeatable local distribution preflight:

```sh
just distribution-validate-local all
```

Individual lanes are available as `source-install`, `vcpkg`, and `conda`. The source-install lane
defaults to `DATOVIZ_SOURCE_DEPS=vendored`; use `DATOVIZ_SOURCE_DEPS=system` only on machines with
the target distro/package-manager development packages installed.

Audit existing install prefixes without rebuilding:

```sh
just distribution-validate-local audit
```

The audit checks source-install, vcpkg, and conda prefixes when present. It verifies installed
headers, Datoviz shared libraries, Linux runtime paths, `datoviz.pc`, CMake package files, and
small CMake/pkg-config consumers. Override prefixes when needed:

```sh
DATOVIZ_SOURCE_AUDIT_PREFIX=/tmp/datoviz-dist-validate/source-prefix \
DATOVIZ_VCPKG_AUDIT_PREFIX=/tmp/datoviz-vcpkg/installed/x64-linux-dynamic \
DATOVIZ_CONDA_AUDIT_PREFIX=/tmp/datoviz-local \
  just distribution-validate-local audit
```


## Latest Local Proof

Keep this section compact; detailed history belongs in commits and release notes.

- On 2026-06-17 macOS arm64, shaderc/Vulkan source-build proof passed:
  `just shaderc-smoke`, `just test-runtime-vklite`, and `just test-drp2-contract`.
- On 2026-06-18 macOS arm64, installed C consumers passed after `d94f72dd6` exported Vulkan
  headers from `datoviz::datoviz`: `just c-integration-smoke` covers installed-package and
  FetchContent consumers.
- On 2026-06-18 macOS arm64, vendored, system-auto, and strict Homebrew-style source/package lanes
  passed. The strict lane used Homebrew `glfw`, `cglm`, `kvazaar`, and `mimalloc`; Vulkan came
  from the local Vulkan SDK, not Homebrew `vulkan-loader`.
- On 2026-06-18 macOS arm64,
  `DATOVIZ_SOURCE_DEPS=system just distribution-validate-local source-install` and
  `just distribution-validate-local audit` passed against the strict source install prefix.
- On 2026-06-18 macOS arm64, host-native repaired wheel proof passed for imports,
  `datoviz-config`, bundled headers/CMake files, native dependency inspection, and the wheel CMake
  consumer. The local repaired wheel was host-tagged `macosx_15_0_arm64`, so
  `macosx_11_0_arm64` still needs CI or an older-target builder.
- On 2026-06-18 macOS arm64, conda preflight passed after bootstrapping micromamba in `/tmp` and
  building from a generated release source bundle. `conda render` and `conda mambabuild
  --override-channels -c conda-forge --no-anaconda-upload conda-recipe` produced local
  `libdatoviz-0.4.0-h70deae4_0.tar.bz2` and `datoviz-0.4.0-py312_0.tar.bz2` packages; the package
  test imported `datoviz`, imported `datoviz.raw`, and created/destroyed `raw.dvz_scene()` without
  a Vulkan device.
- On Ubuntu 24.04 noble, the distro-style system source-install lane passed with system
  `libmimalloc`, `libglfw`, `zlib`, `freetype`, and `tinyxml2`, with no unresolved `ldd` entries.


## Source Bundle

Generate bindings and the release source bundle before package-manager validation:

```sh
just ctypes
just release-source-bundle 0.4.0
```

Record the printed SHA512 digest. GitHub auto-generated source archives are not valid for v0.4
packaging because they omit submodules and ignored generated files.

The bundle must include:

- `datoviz/_ctypes.py`
- external submodules, including nested `external/cimgui/imgui` and
  `external/msdf-atlas-gen/msdfgen`
- required `data/assets/fonts/*.ttf` files used by scene text defaults


## Wheel Matrix

Use local CI parity before dispatching `.github/workflows/wheels.yml`:

```sh
just wheel-ci-local <host-platform-tag>
```

When native code must be rebuilt first:

```sh
just wheel-ci-local <host-platform-tag> 1
```

Windows AMD64 remains the first local Windows proof target:

```sh
set VCPKG_ROOT=C:/vcpkg
set VCPKG_BINARY_SOURCES=clear;files,C:/vcpkg-binary-cache,readwrite
set DVZ_CMAKE_ARGS=-DDVZ_ENABLE_SHADERC=ON
just wheel-ci-local win_amd64 1
```

Equivalent expanded Windows AMD64 runbook, useful when debugging without GitHub Actions:

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

Record the resulting wheel filename, bundled DLL list, native dependency inspection, and CMake
consumer output before treating Windows AMD64 as proven. If AMD64 is green locally, use GitHub
Actions only for the remaining matrix confirmation, especially Windows ARM64 and older macOS
release tags.

Before accepting wheel evidence:

1. Linux wheels build and inspect on `x86_64` and `aarch64`.
2. macOS wheels build and inspect on `x86_64` and `arm64`; `macosx_11_0_arm64` must be proven in
   CI or a clean older-target builder, not retagged from this macOS 15 host.
3. Windows wheels build and inspect on AMD64 and ARM64.
4. Installed-wheel smokes pass for Python 3.10 through 3.14.
5. The CMake consumer check passes from the installed wheel.
6. Qt probing stays optional unless the runner installs a known-good Qt/PyQt stack.


## Conda Preflight

Render and build from the generated bundle:

```sh
DATOVIZ_CONDA_SOURCE_URL=file:///tmp/datoviz-source-bundle-smoke/datoviz-0.4.0-source.tar.gz \
DATOVIZ_CONDA_SOURCE_SHA512=<sha512> \
conda render --override-channels -c conda-forge conda-recipe

DATOVIZ_CONDA_SOURCE_URL=file:///tmp/datoviz-source-bundle-smoke/datoviz-0.4.0-source.tar.gz \
DATOVIZ_CONDA_SOURCE_SHA512=<sha512> \
conda mambabuild --override-channels -c conda-forge --no-anaconda-upload conda-recipe
```

Expected proof:

- `libdatoviz` builds and installs headers plus `libdatoviz`.
- `datoviz` imports `datoviz` and `datoviz.raw`.
- `raw.dvz_scene()` and `raw.dvz_scene_destroy()` pass without creating a Vulkan device.
- `just distribution-validate-local audit` reports the expected conda prefix metadata and no
  unresolved runtime dependencies.

macOS arm64 notes from the local 2026-06-18 run:

- `libdatoviz` linked dynamically to conda-forge `mimalloc`, `libzlib`, `libfreetype6`,
  `tinyxml2`, `glfw`, and `libcxx`.
- The recipe intentionally disables Kvazaar for now because direct conda-forge repodata did not
  expose `kvazaar` on the target subdirs checked.
- Conda-build warned that manually listed run dependencies such as `cglm`, `libpng`, `zlib`,
  `freetype`, and `libvulkan-loader` were over-declared on macOS. Revisit `host` versus explicit
  `run` requirements during staged-recipes review; do not drop platform-critical runtime
  dependencies solely from this macOS warning.


## vcpkg Overlay Preflight

Validate fast checkout mode first:

```sh
DATOVIZ_VCPKG_SOURCE_PATH=$PWD \
  vcpkg install datoviz --classic --overlay-ports=vcpkg-overlay/ports --triplet x64-linux-dynamic
```

Then validate release-bundle mode:

```sh
DATOVIZ_VCPKG_SOURCE_URL=file:///tmp/datoviz-source-bundle-smoke/datoviz-0.4.0-source.tar.gz \
DATOVIZ_VCPKG_SOURCE_SHA512=<sha512> \
  vcpkg install datoviz --classic --overlay-ports=vcpkg-overlay/ports --triplet x64-linux-dynamic
```

For Windows release confidence, repeat with `x64-windows` or the selected Windows dynamic triplet
before publishing the overlay.

Expected proof:

- Datoviz release and debug shared libraries exist in the vcpkg install prefix.
- `DatovizConfig.cmake`, version files, and targets are fixed up under `share/datoviz`.
- `datoviz.pc` exists under `lib/pkgconfig`.
- Runtime dependency inspection resolves vcpkg-managed dependencies and reports no unresolved
  entries.


## Distro-Style System Preflight

Run this after installing the target distro development packages:

```sh
DATOVIZ_SOURCE_DEPS=system just distribution-validate-local source-install
just distribution-validate-local audit
```

Ubuntu 24.04 noble package names validated so far:

```sh
sudo apt-get install -y \
  cmake ninja-build pkg-config \
  libcglm-dev libfreetype-dev libglfw3-dev libmimalloc-dev libtinyxml2-dev libvulkan-dev zlib1g-dev
```

Package-manager recipes should use `DVZ_VENDORED_DEPS=OFF` and only force `SYSTEM` for dependency
packages proven in that ecosystem.


## Live Release Gates

Do not run or publish live release actions until the user explicitly approves them. When approved:

1. Confirm `git diff --check` is clean and `git status --short` has no unapproved `data` gitlink,
   generated binary, runtime library, wheel, or source-bundle changes staged.
2. Upload `datoviz-<version>-source.tar.gz` as a release asset.
3. Replace placeholder SHA512 values in `conda-recipe/meta.yaml` and
   `vcpkg-overlay/ports/datoviz/portfile.cmake`.
4. Run the manual wheel workflow `.github/workflows/wheels.yml`.
5. Inspect Linux, macOS, and Windows artifacts before upload.
6. Submit conda-forge staged-recipes and vcpkg catalog PRs after local and CI proof are clean.
