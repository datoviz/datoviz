# Windows & Conda Distribution — Agent Handoff

## Purpose

This document captures strategic decisions and current status for Windows packaging and
conda-forge distribution of datoviz v0.4. Read this first for priorities and rationale, then
consult `C_DISTRIBUTION.md` for implementation details on each work item.

---

## Current status

### Active now

| Item | Status | Next action |
|---|---|---|
| pip Linux/macOS wheels | Linux manylinux proof passed; macOS wheel policy now targets macOS 15 for both arm64 and Intel | Run `.github/workflows/wheels.yml`; inspect Linux/macOS release artifacts |
| pip Windows MinGW wheel | local Windows AMD64 build/validation passed after `19e62968`; CI matrix confirmation pending | Inspect Windows AMD64/ARM64 artifacts from `.github/workflows/wheels.yml` |
| Wheel C integration | implemented; Linux/macOS proof passed, Windows AMD64 CMake-consumer smoke passed locally | Re-run clean installed-wheel smokes on Windows CI |
| WSL2 install docs | documented | Keep aligned with source-build docs |
| "Build on Windows in VS" docs | documented in install guide | Expand into a dedicated page if user feedback needs it |
| conda-forge preflight | macOS arm64 render/build proof passed locally; headless import/scene proof passed | Confirm Windows/Linux feedstock logs and dependency review |
| vcpkg overlay | draft added | Replace release-source SHA512 after stable release tag and source bundle |

### Needs release tag

| Item | Status | Next action |
|---|---|---|
| conda-forge feedstock submission | not started | Submit after preflight passes and stable release tag exists |
| MSVC wheel | designed, not started | Build `datoviz.dll` + `datoviz.lib`; use bundled CMake config, not `datoviz-config` |
| vcpkg main catalog submission | not started | After overlay is validated; review takes weeks |
| Spack recipe | not started | `DVZ_VENDORED_DEPS=OFF` confirmed stable — low effort |
| Homebrew formula | partial (justfile machinery exists) | Needs stable release tag |
| `.deb` package | partial (justfile machinery exists) | After pkg-config and install metadata are stable |

### Later or out of scope

| Item | Status | Next action |
|---|---|---|
| conan | post-v0.4 | Not datoviz's primary audience |
| rpm | post-v0.4 | Less critical than deb for this audience |
| Docker headless image | post-v0.4 | GPU passthrough complexity not worth pulling in now |
| nix/nixpkgs | community-driven | No action needed |
| Chocolatey / winget | **out of scope** | App installers, not C library distribution |

---

## Windows

### What Windows users expect, by persona

| Persona | Expected path |
|---|---|
| Scientific Python user | `pip install datoviz` now; `conda install datoviz` once feedstock is live |
| VS C++ developer | `vcpkg install datoviz`, then `find_package(datoviz)` in CMake |
| MSYS2/MinGW developer | `datoviz-config --cflags --libs` or MinGW-compatible build |
| End user (no dev) | Not datoviz's audience |

### Wheels: MinGW is the right call

Python users on Windows don't care whether the DLL was built with MinGW or MSVC. NumPy,
SciPy, and matplotlib all ship MinGW-built DLLs on Windows. The MinGW wheel CI is now the
manual `.github/workflows/wheels.yml` workflow; the highest-leverage Windows support action
is to keep that workflow green and inspect the AMD64/ARM64 artifacts before upload.

The MSVC wheel matters for C developers who `pip install datoviz` and then link against it
from a Visual Studio project. MinGW DLLs require a `.lib` import library generated from a
`.def` file to be usable from MSVC, which is friction. The MSVC wheel ships `datoviz.dll` +
`datoviz.lib` directly, which VS can consume without extra steps.

Current MinGW wheel implementation notes after `19e62968`:

- Windows wheel staging requires Git Bash so `tools/copy_wheel_c_integration.sh` can copy the
  bundled headers and CMake files.
- Native payload staging copies Datoviz DLL/import-library outputs from `build/src`, root `build`,
  and vcpkg runtime DLLs from `build/vcpkg_installed/.../bin`.
- The wheel-local `DatovizConfig.cmake` accepts `libdatoviz.dll.a` as the MinGW import library
  fallback when `datoviz.lib` is absent.
- `datoviz.raw` adds the installed wheel directory to `DVZ_WHEEL_RUNTIME_DIRS` and, on Windows,
  calls `os.add_dll_directory()` so bundled DLLs are discoverable.
- The installed-wheel CMake consumer smoke prepends the wheel prefix to `PATH` before running the
  compiled executable.

### `datoviz-config` is not for MSVC

`datoviz-config` emits `-I`/`-L` flags (GCC syntax). MSVC uses `/I`/`/LIBPATH:`. This is a
deliberate decision — nobody writes MSVC projects as raw `cl.exe` invocations; they use
CMake, MSBuild, or Visual Studio. The documented MSVC integration path is
`find_package(datoviz)` via the bundled `DatovizConfig.cmake`. See C_DISTRIBUTION.md item 4
for the cmake config design.

### DLL placement on Windows

There is no rpath on Windows. Users must either put `datoviz.dll` next to their binary or
add it to PATH. The documented solution (C_DISTRIBUTION.md item 7) is a CMake
`add_custom_command` post-build copy snippet. Document this in the C integration guide.

### Build from source in Visual Studio

This already works. VS 2019+ has first-class CMake support — `File > Open > Folder` picks up
`CMakePresets.json`, and the `msvc` preset is defined. Prerequisites: Visual Studio 2022,
Vulkan SDK, vcpkg. What's missing is a "Building on Windows" documentation page walking
through those three steps. No engineering required.

### vcpkg is high priority

vcpkg is the standard modern dependency management path for VS C++ developers. When vcpkg is
integrated with VS (default in VS 2022), users add `datoviz` to `vcpkg.json` and
`find_package(datoviz)` works automatically — no manual include paths, no `.lib` hunting.

**Two-stage approach:**
1. **Custom vcpkg overlay** (ships immediately, no review wait) — a small repo users point
   their vcpkg at. A `portfile.cmake` is ~30 lines. Unblocked once there is a stable release
   tag.
2. **Official vcpkg registry submission** — after the overlay is validated. Microsoft's review
   takes 1–4 weeks. This is what makes datoviz discoverable via `vcpkg search`.

vcpkg is bumped from post-v0.4 to **active now**. It is the correct answer to "standard way
to add datoviz as a VS dependency."

### Chocolatey and winget are out of scope

These are app installers (think `choco install git`), not C library distribution mechanisms.
They have no role in the datoviz distribution story.

### MSYS2/pacman

Community-driven. Once the MinGW wheel is stable and there is a release tag, someone will
submit a `mingw-w64-datoviz` package to the MSYS2 package repo. No action needed; document
MSYS2 as a supported path.

### WSL2

The recommended Windows path until the native MSVC wheel stabilises. Pure documentation
work — see C_DISTRIBUTION.md item 9 for the step-by-step. Write this page first since it
unblocks Windows users immediately at zero engineering cost.

---

## conda

### Why conda matters for datoviz's audience

The scientific Python community — neuroscience, bioinformatics, computational biology, HPC —
is heavily conda-first. Many users in this space have never used pip directly. If datoviz
is not on conda-forge, a real segment of the target audience hits friction at install time.
IBL itself and most neuroscience labs run conda environments.

### Split package pattern

Preferred conda-forge proposal for libraries with Python bindings:

- `libdatoviz` — the C library (`libdatoviz.so` + headers), no Python dependency
- `datoviz` — Python bindings, depends on `libdatoviz`

This mirrors the important part of OpenCV's conda-forge split (`libopencv` is the native
library, `py-opencv` is the Python binding package). For Datoviz, keeping `datoviz` as the
Python package is better for user expectations because it matches `pip install datoviz` and
`import datoviz`. C developers install `libdatoviz`; Python users install `datoviz` and get both.
Treat this as the initial staged-recipes proposal, not a decision to fight reviewers over.

### Dynamic linking vs. pip wheels

pip wheels statically vendor freetype, zlib, etc. into `libdatoviz`. conda packages
dynamically link against conda's canonical copies. The `DVZ_VENDORED_DEPS=OFF` CMake flag
gates system-first resolution of glfw, cglm, mimalloc, and kvazaar — confirmed working (see
C_DISTRIBUTION.md vendored dependency audit). Set this flag in the conda recipe.

### The Vulkan headless question — validate this first

conda-forge CI builds run without a GPU. Does `import datoviz` (or loading `libdatoviz`)
require a live Vulkan device, or only at first render? If it crashes at import time without
a GPU, the conda-forge CI will fail and the feedstock cannot be accepted.

**Validate before submitting the feedstock:**
```python
# Importing the Python package and loading the raw library should not create a Vulkan device.
import datoviz
import datoviz.raw as raw

# Scene allocation should also stay CPU-side and headless-safe.
scene = raw.dvz_scene()
raw.dvz_scene_destroy(scene)
```

If this crashes, the library needs a lazy Vulkan initialisation path — defer device creation
until first `dvz_app()` call, not at library load or `dvz_scene()`. This is the single
largest unknown for the conda-forge submission.

### Current local conda proof

On 2026-06-18 macOS arm64, a generated release source bundle rendered and built with
`conda mambabuild --override-channels -c conda-forge --no-anaconda-upload conda-recipe`.
The local build produced `libdatoviz` and `datoviz` packages, then the package test imported
`datoviz`, imported `datoviz.raw`, and created/destroyed `raw.dvz_scene()` without a Vulkan device.

The first local build exposed a Python-output issue: pip inherited a user-install preference and
failed inside conda-build. The recipe scripts now force `PIP_USER=false` and avoid
`--ignore-installed`.

### conda-forge submission process

1. Validate Vulkan headless import (above)
2. Verify conda-forge availability for cglm and Kvazaar before forcing those dependencies to
   `SYSTEM`
3. Write `recipe/meta.yaml` with `DVZ_VENDORED_DEPS=OFF`, preferred outputs `libdatoviz` and
   `datoviz`, and correct `host:`/`run:` deps
4. Submit to `conda-forge/staged-recipes` (review takes 1–3 weeks)
5. After merge, the feedstock lives at `conda-forge/datoviz-feedstock`
6. conda-forge bot handles dependency update PRs automatically from then on

Requires a stable release tag before submission. conda-forge will not accept a moving target.

### Spack

~50 lines of Python, `DVZ_VENDORED_DEPS=OFF` already works. Datoviz's neuroscience audience
overlaps heavily with HPC clusters (national labs, universities) where Spack is the dominant
package manager and pip is unavailable. Keep this tag-gated and lower priority than the wheel,
conda, and vcpkg lanes. See C_DISTRIBUTION.md item 15 for the recipe skeleton.

---

## Decisions made — do not re-open

- **MinGW wheel first**, not MSVC. Python users don't care about toolchain.
- **`datoviz-config` is GCC-only**. MSVC users use CMake `find_package`.
- **Dynamic linking only** for `libdatoviz` itself (no static build of datoviz).
- **vcpkg overlay first**, then main catalog submission. Don't wait for registry review.
- **Chocolatey and winget are out of scope** — app installers, not relevant.
- **conda split-package proposal** (`libdatoviz` + `datoviz`), matching native-library/Python
  binding split conventions while preserving `conda install datoviz` for Python users.
- **Vulkan headless validation is a prerequisite** for the conda-forge submission.
- **vcpkg is bumped to high-priority active work** — draft the overlay soon, publish after tag.
- **Spack is low-effort, release-tag-gated work** — do not let it block wheel/conda/vcpkg progress.
- **conan, rpm, Docker, nix** remain post-v0.4 or community-driven.

---

## Key files

| File | Purpose |
|---|---|
| `agents/now/C_DISTRIBUTION.md` | Detailed implementation spec for all distribution work items |
| `.github/workflows/wheels.yml` | Manual wheel CI including Windows MinGW jobs |
| `vcpkg-overlay/ports/datoviz/` | Draft vcpkg overlay port |
| `conda-recipe/` | Draft conda-forge staged-recipes starting point |
| `agents/now/DISTRIBUTION_RELEASE_CHECKLIST.md` | Current local preflight commands and recorded package evidence |
| `CMakePresets.json` | `msvc` and `mingw` presets for Windows builds |
| `justfile` | `just msvc`, `just mingw` recipes |
| `datoviz/cli.py` | `datoviz-config` console script |
| `cmake/DatovizConfig.cmake.wheel` | Wheel-local `find_package` support |
