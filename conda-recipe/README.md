# Conda-Forge Recipe Draft

This directory is a feedstock draft, not an in-repo package build path. Use it as the starting
point for `conda-forge/staged-recipes` after a stable Datoviz release tag and source bundle exist.

Current preflight:

- `import datoviz`, `import datoviz.raw`, `raw.dvz_scene()`, and destroy passed locally without
  creating a Vulkan device.
- On 2026-06-18 macOS arm64, `conda render` and `conda mambabuild --override-channels -c
  conda-forge --no-anaconda-upload conda-recipe` passed from a generated local release source
  bundle. The resulting package test imported `datoviz`, imported `datoviz.raw`, and created and
  destroyed a scene.
- The raw ctypes loader can find a native Datoviz library installed under a conda-style prefix
  (`$PREFIX/lib` on Unix, `$PREFIX/Library/bin` on Windows).
- Direct conda-forge repodata check found `cglm` on Linux, macOS, and Windows target subdirs.
- Direct conda-forge repodata check did not find `kvazaar`; keep `DVZ_KVAZAAR_SOURCE=OFF` for the
  first recipe unless a conda-forge package lands.
- The Python build scripts set `PIP_USER=false`; this is required in environments where pip config
  would otherwise force a user install, which conda-build forbids.
- On 2026-07-31 Linux x86_64, the stock conda-forge `pyqt6` 6.11.0 package did not export `QVulkanInstance` or `QWindow.setVulkanInstance`, but a local rebuild of the same feedstock with `vulkan-headers` added to its host requirements exported both and passed the full PyQt package tests.
- Using that locally rebuilt PyQt6 package, all three Datoviz split outputs built and passed their package tests; a fresh-prefix `python -m datoviz.qt` loaded the packaged bridge with ABI 1 and Qt 6.11.1, and `examples/python/qt/hosted_pyqt.py --smoke-ms 1500` rendered and exited successfully on Linux x86_64. A separate base-only environment imported `datoviz` and `datoviz.raw` without installing PyQt6 or the bridge.

Before submitting:

1. Replace the source `sha512` placeholder in `meta.yaml`.
2. Confirm Linux and Windows paths, dependency names, and DLL/shared-library packaging in
   staged-recipes or feedstock CI logs.
3. Revisit explicit `run:` dependencies after staged-recipes review; the local macOS build warned
   that several manually listed run requirements were over-declared even though they are valid
   host/build requirements.
4. Keep the split package names as proposed: `libdatoviz` for the C library, `datoviz` for Python, and `datoviz-qtbridge` for the optional PyQt6 provider.
5. Validate `datoviz-qtbridge` against the same conda-managed Qt major/minor runtime used by PyQt6; the provider package must not add Qt, PyQt6, or the bridge to either base package.

The source bundle must be created with `tools/release_source_bundle.py` or `just
release-source-bundle`; GitHub's auto-generated archives are insufficient because they omit
submodule contents and ignored generated files such as `datoviz/_ctypes.py`.

For local builds before the release asset exists, override the source fields:

```sh
DATOVIZ_CONDA_SOURCE_URL=file:///tmp/datoviz-source-bundle-smoke/datoviz-0.4.0-source.tar.gz \
DATOVIZ_CONDA_SOURCE_SHA512=<printed sha512> \
conda build --override-channels -c conda-forge conda-recipe
```
