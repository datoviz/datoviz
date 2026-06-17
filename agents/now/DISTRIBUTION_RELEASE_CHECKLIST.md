# Distribution Release Checklist

Use this checklist before dispatching live GitHub Actions or publishing release assets. It keeps
the wheel, conda, and vcpkg paths tied to the same source bundle.

## Local Source Bundle

The repeatable local preflight entry point is:

```sh
just distribution-validate-local all
```

Individual lanes are available as `source-install`, `vcpkg`, and `conda`.
The source-install lane defaults to `DATOVIZ_SOURCE_DEPS=vendored`; use
`DATOVIZ_SOURCE_DEPS=system` when validating distro-style system dependencies on a machine with
the required development packages installed.

Audit existing install prefixes without rebuilding:

```sh
just distribution-validate-local audit
```

The audit lane checks the source install prefix, vcpkg prefix, and conda prefix when present. It
verifies installed Datoviz headers, `libdatoviz`, `ldd`/runtime paths on Linux, `datoviz.pc`,
CMake package files, and small CMake/pkg-config consumers. Override default prefixes with:

```sh
DATOVIZ_SOURCE_AUDIT_PREFIX=/tmp/datoviz-dist-validate/source-prefix \
DATOVIZ_VCPKG_AUDIT_PREFIX=/tmp/datoviz-vcpkg/installed/x64-linux-dynamic \
DATOVIZ_CONDA_AUDIT_PREFIX=/tmp/datoviz-local \
  just distribution-validate-local audit
```

Local shaderc/Vulkan source-build proof:

- `just shaderc-smoke` runs a native build-tree `dvz_compile_glsl()` smoke.
- On 2026-06-17 macOS arm64, `just shaderc-smoke` passed and produced 1168 SPIR-V bytes.
- On 2026-06-17 macOS arm64, `just test-runtime-vklite` passed 88/89 selected tests with one
  environment skip, `vk/memory_interop_buffer_export` (`external semaphore FD unsupported`).
  The DRP2 GLSL shader module runtime cases passed.
- On 2026-06-17 macOS arm64, `just test-drp2-contract` passed 91/91 selected tests.

1. Generate current ctypes bindings:
   ```sh
   just ctypes
   ```
2. Create the source bundle:
   ```sh
   just release-source-bundle 0.4.0
   ```
3. Record the printed SHA512 digest. GitHub-generated archives are not valid for v0.4 packaging
   because they omit submodules and ignored generated files.

The bundle must include:

- `datoviz/_ctypes.py`
- external submodules, including nested `external/cimgui/imgui` and
  `external/msdf-atlas-gen/msdfgen`
- required `data/assets/fonts/*.ttf` files used by scene text defaults

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
- `raw.dvz_scene()` and `raw.dvz_scene_destroy()` pass in the test environment.
- `just distribution-validate-local audit` reports the expected Datoviz headers, shared libraries,
  CMake package files, pkg-config metadata, and no unresolved `ldd` entries for the conda prefix.

## vcpkg Overlay Preflight

First validate fast checkout mode:

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

Expected Linux dynamic audit proof:

- `libdatoviz.so`, `libdatoviz_core.so`, `libdatoviz_vk.so`, and `libdatoviz_canvas.so` exist in
  both release and debug library directories for `x64-linux-dynamic`.
- `DatovizConfig.cmake`, version files, and targets are fixed up under `share/datoviz`.
- `datoviz.pc` exists under `lib/pkgconfig`.
- `ldd` resolves vcpkg-managed runtime dependencies from the vcpkg prefix and reports no
  unresolved entries.

## Distro-Style System Dependency Preflight

Run this after installing the target distro development packages:

```sh
DATOVIZ_SOURCE_DEPS=system just distribution-validate-local source-install
```

On Ubuntu 24.04 noble, the required package names validated so far are:

```sh
sudo apt-get install -y \
  cmake ninja-build pkg-config \
  libcglm-dev libfreetype-dev libglfw3-dev libmimalloc-dev libtinyxml2-dev libvulkan-dev zlib1g-dev
```

Current local result on Ubuntu 24.04 noble: after installing those packages,
`DATOVIZ_SOURCE_DEPS=system just distribution-validate-local source-install` passes, and
`just distribution-validate-local audit` reports the source prefix linked to system `libmimalloc`,
`libglfw`, `zlib`, `freetype`, and `tinyxml2` with no unresolved `ldd` entries.

## Live Release Gates

Do not run or publish from this checklist until the user explicitly approves live release actions.
When approved:

1. Upload `datoviz-<version>-source.tar.gz` as a release asset.
2. Replace placeholder SHA512 values in `conda-recipe/meta.yaml` and
   `vcpkg-overlay/ports/datoviz/portfile.cmake`.
3. Run the manual wheel workflow `.github/workflows/wheels.yml`.
4. Inspect Linux, macOS, and Windows artifacts before upload.
5. Submit conda-forge staged-recipes and vcpkg catalog PRs after local and CI proof are clean.
