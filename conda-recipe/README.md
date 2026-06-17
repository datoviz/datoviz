# Conda-Forge Recipe Draft

This directory is a feedstock draft, not an in-repo package build path. Use it as the starting
point for `conda-forge/staged-recipes` after a stable Datoviz release tag and source bundle exist.

Current preflight:

- `import datoviz`, `import datoviz.raw`, `raw.dvz_scene()`, and destroy passed locally without
  creating a Vulkan device.
- The raw ctypes loader can find a native Datoviz library installed under a conda-style prefix
  (`$PREFIX/lib` on Unix, `$PREFIX/Library/bin` on Windows).
- Direct conda-forge repodata check found `cglm` on Linux, macOS, and Windows target subdirs.
- Direct conda-forge repodata check did not find `kvazaar`; keep `DVZ_KVAZAAR_SOURCE=OFF` for the
  first recipe unless a conda-forge package lands.

Before submitting:

1. Replace the source `sha512` placeholder in `meta.yaml`.
2. Verify dependency names against a local `conda-build` or staged-recipes run.
3. Confirm Windows paths and DLL packaging in the feedstock CI logs.
4. Keep package names as proposed: `libdatoviz` for the C library and `datoviz` for Python.

The source bundle must be created with `tools/release_source_bundle.py` or `just
release-source-bundle`; GitHub's auto-generated archives are insufficient because they omit
submodule contents and ignored generated files such as `datoviz/_ctypes.py`.
