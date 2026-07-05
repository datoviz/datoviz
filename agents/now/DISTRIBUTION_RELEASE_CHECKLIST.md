# Distribution Release Checklist

Status: active release gate. Updated: 2026-06-22.

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


## Package Policy Snapshot

- Use one explicit release source bundle for wheels, conda, and vcpkg; do not use GitHub
  auto-generated archives for package-manager proof.
- Windows wheels must continue to carry the MSVC DLL/import-library pair, bundled CMake package
  files, required vcpkg runtime DLLs, and installed-wheel CMake consumer proof.
- The conda-forge proposal is split output packages: `libdatoviz` for the C library and headers,
  `datoviz` for Python bindings. Conda validation must remain headless-safe through raw scene
  create/destroy without creating a Vulkan device.
- vcpkg overlay publication waits for the stable source bundle and SHA512; official vcpkg registry
  submission follows overlay validation.
- Chocolatey and winget are out of scope for v0.4. Spack, Homebrew, `.deb`, rpm, conan, Docker,
  and nix remain lower-priority or post-v0.4 unless release scope changes.


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
  consumer. On 2026-06-22, hosted CI showed the current macOS Vulkan/Homebrew runtime stack requires
  macOS 15, so the v0.4 wheel policy now targets `macosx_15_0_arm64` and
  `macosx_15_0_x86_64`.
- On 2026-06-18 Linux x86_64, local manylinux Docker proof passed with
  `just wheel-manylinux-docker x86_64` against
  `quay.io/pypa/manylinux_2_34_x86_64@sha256:e05e1c4b281f10dc4c3df2b6f546392a0dd4c6383d620c3f8a6c33e19069d056`.
  The run built `datoviz-0.4.0.dev0-py3-none-manylinux_2_34_x86_64.whl`; auditwheel kept the
  `manylinux_2_34_x86_64` tag, and installed-wheel import, `datoviz.cli`, shaderc GLSL
  compilation, and CMake consumer checks passed. The optional Qt probe failed only because PyQt6
  was absent in the clean venv.
- On 2026-06-18 Windows AMD64, local Windows wheel proof passed after `19e62968`: wheel build,
  native dependency inspection, installed-wheel import, `datoviz.cli`, shaderc GLSL compilation,
  and the installed-wheel CMake consumer check completed successfully.
- On 2026-06-18 macOS arm64, conda preflight passed after bootstrapping micromamba in `/tmp` and
  building from a generated release source bundle. `conda render` and `conda mambabuild
  --override-channels -c conda-forge --no-anaconda-upload conda-recipe` produced local
  `libdatoviz-0.4.0-h70deae4_0.tar.bz2` and `datoviz-0.4.0-py312_0.tar.bz2` packages; the package
  test imported `datoviz`, imported `datoviz.raw`, and created/destroyed `raw.dvz_scene()` without
  a Vulkan device.
- On Ubuntu 24.04 noble, the distro-style system source-install lane passed with system
  `libmimalloc`, `libglfw`, `zlib`, `freetype`, and `tinyxml2`, with no unresolved `ldd` entries.
- On 2026-06-22, hosted wheel CI run `27966579584` proved Linux x86_64/aarch64 wheel build,
  native inspection, upload, and Linux installed-wheel smokes for Python 3.10 through 3.14 plus a
  prerelease smoke. The downloaded Linux artifacts were
  `datoviz-0.4.0.dev0-py3-none-manylinux_2_34_x86_64.whl` and
  `datoviz-0.4.0.dev0-py3-none-manylinux_2_34_aarch64.whl`, each with required generated Python
  bindings, CMake package files, `libdatoviz.so`, and `libshaderc_shared.so`.
- On 2026-06-22, hosted wheel CI run `27966579584` proved Windows AMD64 and ARM64 wheel build,
  native inspection, upload, and Windows Python 3.10 through 3.14 installed-wheel smokes. The
  downloaded Windows artifacts were `datoviz-0.4.0.dev0-py3-none-win_amd64.whl` and
  `datoviz-0.4.0.dev0-py3-none-win_arm64.whl`, each with generated Python bindings, CMake package
  files, `datoviz.dll`, and the required `datoviz.lib` import library.
- On 2026-06-22, hosted wheel CI run `27971470056` proved that the old
  `macosx_11_0_arm64` target was dishonest with current bundled macOS runtime dependencies:
  `delocate` rejected the wheel because `libvulkan`, `libshaderc_shared`, `freetype`, `libpng`,
  and `tinyxml2` required macOS 15. The next macOS proof must use the macOS 15 wheel tags.
- On 2026-06-22, hosted wheel CI run `27975460115` passed the full wheel matrix on commit
  `92c41fd6e`: Linux x86_64/aarch64, macOS 15 arm64/Intel, Windows AMD64/ARM64, installed-wheel
  smokes for Python 3.10 through 3.14 on Linux/macOS/Windows, and the non-blocking Linux
  prerelease smoke. Downloaded artifacts had the expected tags:
  `manylinux_2_34_x86_64`, `manylinux_2_34_aarch64`, `macosx_15_0_arm64`,
  `macosx_15_0_x86_64`, `win_amd64`, and `win_arm64`. Local artifact inspection confirmed
  required generated Python bindings, CMake package files, Linux `libdatoviz.so` plus
  `libshaderc_shared.so`, Windows `datoviz.dll` plus `datoviz.lib`, and architecture-correct
  macOS dylibs for arm64 and x86_64.


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

Windows AMD64 has local proof. Use this only to reproduce or debug the Windows AMD64 lane:

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
consumer output for release notes. Windows `wheel-stage` requires Git Bash for the C integration
copy script.

Before accepting wheel evidence:

1. Linux wheels build and inspect on `x86_64` and `aarch64`.
2. macOS 15 wheels build and inspect on `x86_64` and `arm64`; do not retag a wheel as supporting
   an older macOS version than any bundled dylib supports.
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
