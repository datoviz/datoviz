# C/C++ Distribution and Integration — Agent Handoff

## Goal

Make datoviz a first-class C/C++ library that users can install (via pip, conda, brew, or system
package) and immediately use from their own C/C++ projects without cloning the datoviz repo. The
Python wheel is the primary distribution vehicle for v0.4; other channels follow.

---

## Decisions already made

- **`datoviz-config` script** is the pkg-config equivalent for Linux/macOS/MinGW. It emits
  `-I`/`-L` flags. MSVC users use CMake `find_package` instead — no MSVC flag syntax needed.
- **Dynamic linking only** for `libdatoviz` itself. No static build of datoviz.
- **Vendored deps (freetype, zlib, etc.)**: statically linked into `libdatoviz` for pip wheels
  (self-contained, no conflicts). Dynamically linked against the host package manager's copies
  for conda-forge, Homebrew, deb, rpm, spack (those package managers own the ABI).
- **DLL placement on Windows**: CMake post-build copy command (documented snippet). Not PATH.
- **FetchContent support**: add `PROJECT_IS_TOP_LEVEL` guards in CMakeLists so datoviz can be
  pulled as a subdirectory dependency without installing.
- **Public C/C++ headers**: `include/datoviz.h` exists as the preferred umbrella include and
  forwards to `include/datoviz/datoviz.h`, which now includes `app.h`. The previously missing
  `EXTERN_C_ON` / `EXTERN_C_OFF` guards on the five public subheaders are fixed, and
  `testing/dvz_public_header_probe.cpp` verifies that the installed umbrella header parses as C++.
- **MSVC support**: after RC, before final v0.4 release. pip wheel with `.dll` + `.lib` +
  headers. CMake `find_package` is the documented integration path.
- **Windows paths in scope**: WSL2 (document only, no engineering), MinGW64 (works today),
  MSVC (wheel + docs after RC).
- **vcpkg port / conan**: post-v0.4.

---

## Distribution channel tiers

| Channel | Audience | Dep linking | Engineering | Timeline |
|---|---|---|---|---|
| pip / PyPI (Linux, macOS) | Python users | static vendored | already planned | v0.4 RC |
| pip / PyPI (MinGW wheel) | Windows Python users | static vendored | already planned | v0.4 RC |
| pip / PyPI (MSVC wheel) | Windows C/Python native | static vendored | CI job + `.lib` | v0.4 final |
| conda-forge | Scientific Python ecosystem | dynamic (conda's libs) | `meta.yaml` feedstock | v0.4 final |
| Homebrew | macOS C/C++ developers | dynamic (brew's libs) | `Formula/datoviz.rb` | v0.4 final |
| apt / .deb | Ubuntu/Debian, headless CI | dynamic (system libs) | `debian/` packaging | v0.4 final |
| rpm / .rpm | Fedora, RHEL, openSUSE | dynamic (system libs) | `.spec` file | post-v0.4 |
| vcpkg port | Windows/cross-platform C++ | vcpkg manages | `portfile.cmake` | post-v0.4 |
| conan | C++ build system users | conan manages | `conanfile.py` | post-v0.4 |
| Spack | HPC clusters, national labs | dynamic (spack's libs) | `package.py` | post-v0.4 |
| nix / nixpkgs | Reproducibility-focused devs | nix manages | `default.nix` | community |
| winget / chocolatey | Windows end users | bundled installer | installer + manifest | post-v0.4 |
| Docker / container image | Headless CI, cloud rendering | bundled | `Dockerfile` (partial) | post-v0.4 |

**Why `.deb` in v0.4 final:** Ubuntu is datoviz's primary Linux target, it's what CI runs, and
it's the cleanest story for "install datoviz system-wide and use from C without Python". The
justfile already has partial `.deb`/pkg build machinery. Pull it up.

**Why Spack matters:** datoviz's scientific visualization audience overlaps heavily with HPC
users (national labs, universities). Spack is the dominant package manager on clusters where pip
is not available. A `package.py` recipe is ~50 lines and unlocks that entire community.

---

## Work items

### 1a. Public umbrella header — completed

**Current state:** completed in the source tree.

- `include/datoviz/datoviz.h` includes `app.h`.
- `include/datoviz.h` exists as the top-level forwarding header.
- C and C++ public header probes are registered in `testing/CMakeLists.txt`.

Users can write:

```c
#include <datoviz.h>
```

The CMakeLists install rule `install(DIRECTORY include/ DESTINATION include)` already covers
both files.

Keep new minimal C/C++ examples on the top-level include unless a subheader is being documented
explicitly.

---

### 1b. C++ public header guards — completed

These five public subheaders now wrap exported declarations in `EXTERN_C_ON` /
`EXTERN_C_OFF`:

- `include/datoviz/math/vec.h` — 15 exported functions
- `include/datoviz/math/parallel.h` — 4 exported functions
- `include/datoviz/vk/device.h` — 19 exported functions
- `include/datoviz/vk/gpu.h` — 3 exported functions
- `include/datoviz/common/version.h` — 1 exported function

`EXTERN_C_ON` / `EXTERN_C_OFF` are defined in `include/datoviz/common/macros.h` and expand to
`extern "C" {` / `}` under C++, nothing under C.

---

### 2. `datoviz-config` console script

**File to create:** `datoviz/cli.py`

```python
"""datoviz-config — emit compiler/linker flags for building against the installed datoviz."""
import sys
from pathlib import Path

_PKG = Path(__file__).parent


def main():
    args = set(sys.argv[1:])
    if not args or "--help" in args:
        print("Usage: datoviz-config [--cflags] [--libs] [--cmake-dir] [--prefix]")
        return
    if "--prefix" in args:
        print(_PKG)
    if "--cflags" in args:
        print(f"-I{_PKG / 'include'}")
    if "--libs" in args:
        print(f"-L{_PKG} -ldatoviz")
    if "--cmake-dir" in args:
        print(_PKG / "lib" / "cmake" / "datoviz")
```

**Register in `pyproject.toml`:**
```toml
[project.scripts]
datoviz-config = "datoviz.cli:main"
```

**Usage the user sees (Linux / macOS / MSYS2):**
```bash
gcc scatter.c $(datoviz-config --cflags --libs) -o scatter
./scatter
```

**Note for MSVC users:** `datoviz-config` does not emit MSVC-compatible flags (`/I`, `/LIBPATH:`).
MSVC users should use the CMake `find_package` path (see work item 4). Document this explicitly.

---

### 3. Bundle headers into the wheel

During the wheel build, copy `include/` into `datoviz/include/`. The existing `package-data`
glob does not cover subdirectories.

**Change in `pyproject.toml`:**
```toml
[tool.setuptools.package-data]
datoviz = ["lib*", "*.so", "*.dll", "*.dylib", "*.json", "include/**/*.h"]
```

**Wheel build hook:** the CMake install step (or a `setup.py` hook) must copy
`include/` → `datoviz/include/` before the wheel is assembled. Check `wheels.yml` — if it does
`cmake --install`, the install rule already handles this. If not, add an explicit copy step.

---

### 4. Bundle CMake files into the wheel

**Goal:** `find_package(datoviz)` works after `pip install datoviz`:
```bash
cmake -B build -Ddatoviz_DIR=$(datoviz-config --cmake-dir)
```

**What to bundle:** a hand-written relocatable `DatovizConfig.cmake` into
`datoviz/lib/cmake/datoviz/`. Do NOT use the auto-generated `DatovizTargets.cmake` — it has
absolute build-tree paths baked into `IMPORTED_LOCATION` that break after pip installs to an
arbitrary site-packages location.

**Hand-written `cmake/DatovizConfig.cmake.wheel` (new file in the repo):**
```cmake
cmake_minimum_required(VERSION 3.21)

# Resolve the wheel root (three levels up from this file's location:
# site-packages/datoviz/lib/cmake/datoviz/ -> site-packages/datoviz/)
get_filename_component(_dvz_root "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)

if(NOT TARGET datoviz::datoviz)
    add_library(datoviz::datoviz SHARED IMPORTED)
    set_target_properties(datoviz::datoviz PROPERTIES
        IMPORTED_LOCATION             "${_dvz_root}/libdatoviz${CMAKE_SHARED_LIBRARY_SUFFIX}"
        IMPORTED_IMPLIB               "${_dvz_root}/datoviz.lib"   # MSVC only, ignored elsewhere
        INTERFACE_INCLUDE_DIRECTORIES "${_dvz_root}/include"
        INTERFACE_LINK_OPTIONS        "$<$<PLATFORM_ID:Linux>:-Wl,-rpath,${_dvz_root}>"
    )
endif()
```

This file is installed into the wheel at `datoviz/lib/cmake/datoviz/DatovizConfig.cmake` by the
wheel build step (not by `cmake --install`, which writes the build-tree version).

**Add to `package-data`:**
```toml
datoviz = [..., "lib/cmake/datoviz/*.cmake"]
```

---

### 5. Rpath — runtime library finding

The compiled binary must find `libdatoviz` at runtime without `LD_LIBRARY_PATH` / `DYLD_LIBRARY_PATH`.

**Linux:** handled by `INTERFACE_LINK_OPTIONS` in the wheel's `DatovizConfig.cmake` (see item 4).
For the `datoviz-config --libs` path, append the rpath flag:
```python
if "--libs" in args:
    rpath = f"-Wl,-rpath,{_PKG}" if sys.platform.startswith("linux") else ""
    print(f"-L{_PKG} -ldatoviz {rpath}".strip())
```

**macOS:** verify `libdatoviz.dylib` install name is `@rpath/libdatoviz.dylib` (not an absolute
path). Check with `otool -D build/src/libdatoviz.dylib`. The wheel build runs `install_name_tool`
— confirm it sets the install name correctly, not just the rpath. Add `-Wl,-rpath,<_PKG>` to the
`datoviz-config --libs` output on macOS too.

**Windows:** no rpath. See sections 7 and 8 for DLL placement.

---

### 6. FetchContent support

Wrap install rules, test targets, and example targets in `PROJECT_IS_TOP_LEVEL` guards so
downstream CMake projects can use `FetchContent_MakeAvailable(datoviz)` without triggering
tests and installs.

Requires bumping minimum CMake from 3.20 → 3.21 (when `PROJECT_IS_TOP_LEVEL` was introduced).

```cmake
# In top-level CMakeLists.txt, guard these blocks:
if(PROJECT_IS_TOP_LEVEL)
    add_subdirectory(examples/c)
    include(CTest)
    add_subdirectory(tests)
    install(...)
endif()
```

FetchContent consumer pattern:
```cmake
include(FetchContent)
FetchContent_Declare(datoviz
    GIT_REPOSITORY https://github.com/datoviz/datoviz.git
    GIT_TAG v0.4.0
)
FetchContent_MakeAvailable(datoviz)
target_link_libraries(myapp PRIVATE datoviz::datoviz)
```

**Note in docs:** FetchContent compiles datoviz from source — slow, requires the full toolchain
(Vulkan SDK, cmake, ninja, etc.). Prefer the pip wheel for most users.

---

### 7. Windows — MinGW64

**Current state:** CI builds with MinGW64 and produces `datoviz.dll`. This path works today.

**DLL placement for CMake users** — document this snippet in `docs/guide/c-cmake-integration.md`:
```cmake
add_custom_command(TARGET myapp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:datoviz::datoviz>"
        "$<TARGET_FILE_DIR:myapp>"
)
```

**MinGW runtime DLLs:** `libgcc_s_seh-1.dll`, `libstdc++-6.dll`, `libwinpthread-1.dll` must be
present alongside the user's binary. Options: bundle them in the wheel (already done in
justfile), or document that the user needs MinGW on PATH. Confirm which approach the wheel takes
and document it.

**datoviz-config on MSYS2:** works as-is (emits GCC-compatible `-I`/`-L` flags). Users run it
in an MSYS2 terminal.

---

### 8. Windows — MSVC (after RC, before final v0.4)

**What's needed for the pip wheel:**
- Build `datoviz.dll` + `datoviz.lib` (import library) with MSVC + vcpkg static triplet
  (`x64-windows-static-md`) so the wheel is self-contained with no vcpkg runtime DLL deps.
- Bundle `datoviz.dll`, `datoviz.lib`, headers, and cmake files in the wheel under `datoviz/`.
- The wheel `DatovizConfig.cmake` already handles `IMPORTED_IMPLIB` (see item 4).

**vcpkg static triplet:** using `x64-windows-static-md` statically links freetype, zlib etc.
into `datoviz.dll` itself. This avoids bundling a dozen vcpkg DLLs alongside every user binary.
Verify no LGPL libs are being statically linked (freetype is MIT/FTL, zlib is zlib license —
both fine for static linking).

**CI:** add an MSVC wheel build job to `wheels.yml` (`windows-latest` runner, Visual Studio).
The `just msvc` recipe already exists; wire it into wheel assembly.

**DLL placement:** same `add_custom_command` snippet as MinGW (section 7).

**`datoviz-config` on MSVC:** explicitly not supported for MSVC flag syntax. Document:
"On Windows with MSVC, use `find_package(datoviz)` with CMake."

---

### 9. Windows — WSL2

**Engineering required:** none.

**Documentation only** — add a "Windows via WSL2" section to `docs/start/install.md`:

1. Run `wsl --install` in PowerShell (Windows 11 or Windows 10 21H2+, build 19044+)
2. Install Ubuntu 24.04 from the Microsoft Store
3. GPU passthrough: works automatically with recent Intel/AMD/NVIDIA drivers (WSL2 Vulkan via
   WDDM 2.x / WSLg). No extra configuration needed on Windows 11.
4. Then follow the Linux install path exactly.

WSL2 is the recommended Windows path until the native MSVC wheel stabilises.

---

### 10. conda-forge

**What:** a conda-forge feedstock — a separate GitHub repo (`datoviz-feedstock`) with a
`recipe/meta.yaml`. conda-forge maintainers or datoviz contributors submit it to the
`conda-forge/staged-recipes` repo.

**Key points:**
- Link dynamically against conda's `freetype`, `zlib`, `libvulkan`, etc. — do not vendor.
- Declare them as `host:` and `run:` dependencies in `meta.yaml`.
- The CMakeLists must support `-DDVZ_VENDORED_DEPS=OFF` (or equivalent) to skip building
  internal copies and use `find_package(Freetype)`, `find_package(ZLIB)` etc. from the
  conda environment. Add this flag if it doesn't exist.
- The conda package installs `libdatoviz.so` + headers to the conda prefix, making
  `find_package(datoviz)` work automatically for C/C++ packages that depend on it.
- The Python package in conda should be a thin wrapper that loads the conda-prefix lib,
  not a bundled wheel.

**Timeline:** v0.4 final. Requires a stable release tag first.

---

### 11. Homebrew

**What:** a `Formula/datoviz.rb` submitted to homebrew-core (or a tap for initial release).

**Key points:**
- Link dynamically against `brew`'s `freetype`, `libpng`, etc.
- Same `-DDVZ_VENDORED_DEPS=OFF` CMake flag requirement as conda.
- Homebrew formula builds from source tarball (`url` pointing to the GitHub release).
- Installs to `$(brew --prefix)/lib/libdatoviz.dylib` and `$(brew --prefix)/include/datoviz/`.
- `datoviz-config` is not installed by Homebrew — instead, the standard `pkg-config` path works
  if datoviz ships a `datoviz.pc` file (see below).
- Consider generating `datoviz.pc` from a `datoviz.pc.in` template in CMakeLists:
  ```cmake
  configure_file(cmake/datoviz.pc.in datoviz.pc @ONLY)
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/datoviz.pc"
      DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
  ```

**Timeline:** v0.4 final.

---

### 12. apt / .deb (Ubuntu/Debian)

**What:** a `.deb` package installable via `apt`. Primary target: Ubuntu 24.04 LTS.

**Why v0.4 final, not post:** Ubuntu is datoviz's primary CI platform. The justfile already has
partial pkg/deb build machinery. A `.deb` enables `sudo apt install datoviz` — the cleanest
path for C-only users who don't want Python involved at all.

**Key points:**
- `debian/` directory with `control`, `rules`, `changelog`, `copyright`.
- Dynamic linking against Ubuntu's `libfreetype-dev`, `zlib1g-dev`, etc.
- Install `libdatoviz.so` to `/usr/lib/x86_64-linux-gnu/`, headers to `/usr/include/datoviz/`,
  `datoviz.pc` to `/usr/lib/x86_64-linux-gnu/pkgconfig/`, cmake files to
  `/usr/lib/cmake/datoviz/`.
- The existing GitHub Actions Ubuntu runner makes CI validation straightforward.
- Consider publishing to a PPA (Personal Package Archive) on Launchpad for Ubuntu users before
  getting into the official Ubuntu/Debian archive.

**Timeline:** v0.4 final.

---

### 13. rpm / .rpm (Fedora, RHEL, openSUSE)

Similar to `.deb` but for the RPM ecosystem. A `.spec` file drives the build.

Install paths follow RPM conventions: `%{_libdir}`, `%{_includedir}`, `%{_libdir}/pkgconfig/`,
`%{_libdir}/cmake/datoviz/`.

**Timeline:** post-v0.4. Lower priority than `.deb` for datoviz's current audience.

---

### 14. pkg-config file (`datoviz.pc`)

Needed by Homebrew and `.deb`/`.rpm` users (who won't have `datoviz-config` from pip).

**New file: `cmake/datoviz.pc.in`:**
```
prefix=@CMAKE_INSTALL_PREFIX@
exec_prefix=${prefix}
libdir=${exec_prefix}/@CMAKE_INSTALL_LIBDIR@
includedir=${prefix}/@CMAKE_INSTALL_INCLUDEDIR@

Name: datoviz
Description: GPU rendering engine for scientific visualization
Version: @PROJECT_VERSION@
Libs: -L${libdir} -ldatoviz
Cflags: -I${includedir}
```

**In CMakeLists.txt:**
```cmake
configure_file(cmake/datoviz.pc.in datoviz.pc @ONLY)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/datoviz.pc"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
```

This is independent of the wheel. System-installed datoviz (via brew, apt, etc.) exposes
`pkg-config --cflags --libs datoviz` automatically.

---

### 15. Spack

A `package.py` recipe submitted to the Spack package repository.

**Why:** datoviz's scientific visualization audience overlaps heavily with HPC users at national
labs and universities. Spack is the dominant package manager on clusters where pip is unavailable
or unreliable. A `package.py` is ~50 lines and unlocks that entire community.

```python
class Datoviz(CMakePackage):
    """GPU rendering engine for scientific visualization."""
    homepage = "https://datoviz.org"
    url = "https://github.com/datoviz/datoviz/archive/v0.4.0.tar.gz"
    version("0.4.0", sha256="...")
    depends_on("freetype")
    depends_on("zlib")
    depends_on("vulkan-loader")
    def cmake_args(self):
        return [self.define("DVZ_VENDORED_DEPS", False)]
```

**Timeline:** post-v0.4. Requires the same `-DDVZ_VENDORED_DEPS=OFF` flag as conda/brew.

---

### 16. Documentation pages to write/update

**`docs/start/install.md`** — restructure to cover all paths:
- pip (Linux/macOS/Windows-MinGW) — primary
- conda (`conda install -c conda-forge datoviz`) — once feedstock is live
- Homebrew (`brew install datoviz`) — once formula is live
- apt (`sudo apt install datoviz`) — once PPA/package is live
- Windows via WSL2 (step-by-step, see item 9)
- Windows native MinGW64 (note MSYS2 requirement)
- Windows native MSVC (note "coming in v0.4 final")
- Build from source (existing content, keep)

**`docs/start/index.md`** (minimal code patterns) — under the C tab add:
```bash
# After: pip install datoviz  (or brew/apt/conda)
gcc scatter.c $(datoviz-config --cflags --libs) -o scatter
./scatter
# On Windows (MSVC): use CMake find_package — see guide
```

**New page: `docs/guide/c-integration.md`**
- The `datoviz-config` one-liner (Linux/macOS/MSYS2)
- Full `CMakeLists.txt` example for `find_package` path (all platforms)
- Full `CMakeLists.txt` example for `FetchContent` path
- The DLL post-build copy snippet for Windows
- `pkg-config` path for system installs

Add to `mkdocs.yml` nav under the Guide section.

---

## CMake flag needed across non-pip channels

conda-forge, Homebrew, apt, rpm, and Spack all need datoviz to link dynamically against
their system copies of supported external dependencies. The package-manager entry point is:

```cmake
option(DVZ_VENDORED_DEPS "Use bundled third-party dependency source trees when available" ON)
```

When `OFF`, dependencies whose source mode is `AUTO` are resolved system-first, with vendored
fallback only when the dependency supports it. This flag is `ON` by default, preserving the
normal source checkout behavior, and should be `OFF` in conda/brew/deb/rpm/spack recipes.

For stricter package CI, force individual dependency modes:

```sh
cmake --preset package-smoke-system-required
cmake --build --preset package-smoke-system-required
cmake --build --preset package-install-system-required
```

That preset requires system `glfw3`, cglm, Kvazaar, and mimalloc. Use
`package-smoke-system-auto` for a local system-preferred smoke that still allows vendored fallback.

### Vendored dependency audit

An agent audited `CMakeLists.txt` and all `external/` subdirectories to determine which
dependencies are replaceable. Results:

| Library | Version | Type | Replaceable? | `find_package()` call | Notes |
|---|---|---|---|---|---|
| **glfw** | 3.4.0 | shared/static lib | **Yes** | `find_package(glfw3 REQUIRED)` | Universally packaged. Target alias `glfw::glfw`. Wayland disabled in vendored build — system package fine on X11/macOS/Windows. **Highest priority.** |
| **kvazaar** | 2.3.2 | static lib | **Yes** | `find_package(kvazaar REQUIRED)` or `pkg_check_modules` | Available as `libkvazaar-dev` on Debian/Ubuntu; on conda-forge. Target `kvazaar::kvazaar`. |
| **mimalloc** | 3.0.10 | static lib | **Yes** | `find_package(mimalloc REQUIRED)` | Ships a CMake config; conda-forge and vcpkg have it. Used only in Release builds. Target `mimalloc-static`. |
| **cglm** | 0.9.6 | header-only/static | **Yes** | `find_package(cglm REQUIRED)` or `pkg_check_modules` | Packaged by Homebrew and some distros. Required by the active math stack. Target normalized as `dvz_cglm`. |
| **msdf-atlas-gen** | 1.4.0 | static lib | **No** | — | Too niche for a reliable distribution dependency. Keep source/vendored-only for v0.4 packaging. |
| **shaderc** headers | v2024.4 | header-only | **Yes** | via Vulkan SDK | Only headers vendored; `.so`/`.lib` comes from the Vulkan SDK already. |
| **Vulkan headers** | — | header-only | **Yes** | `find_package(Vulkan REQUIRED)` | Standard Vulkan SDK. |
| **volk** | 330 | static (meta-loader) | **No** | — | Tightly integrated Vulkan meta-loader. System packages are rare; API coupling is exact. **Must stay vendored.** |
| **cimgui** | docking branch | static lib | **No** | — | Requires the *docking branch* specifically — checked at configure time. No system package ships this variant. **Must stay vendored.** |
| **vk_mem_alloc.h** | 3.4.0-dev | header+cpp stub | **No** | — | Compiled as a `.cpp` stub inside datoviz. No stable system target. **Must stay vendored.** |
| **stb_image.h** | — | header-only | **No** | — | Single-header; no system CMake target. Stay vendored. |
| **tiny_obj_loader.h** | 2.0.0 | header-only | **No** | — | Single-header; no system package. Stay vendored. |
| **fpng** | — | header+impl | **No** | — | Niche fast-path PNG encoder; no system package. Stay vendored. |
| **minimp4.h** | — | header-only | **No** | — | Single-header MP4 muxer; no system package. Stay vendored. |
| **earcut.hpp** | — | header-only | **No** | — | Single-header polygon triangulator; no system package. Stay vendored. |
| **tinycthread** | — | header+impl | **No** | — | Tiny pthreads wrapper; no system package. Stay vendored. |
| **b64** | — | static (small) | **No** | — | No standard CMake package. Stay vendored. |
| **memorymeasure** | — | header+impl | **No** | — | Internal utility. Stay vendored. |

**Summary:** `DVZ_VENDORED_DEPS=OFF` gates system-first replacement of **glfw**, **kvazaar**,
**mimalloc**, and **cglm**. **msdf-atlas-gen** stays source/vendored-only because it is too niche
for reliable package-manager availability. Everything else must stay vendored (niche,
single-header, or architecturally coupled like volk/cimgui). Freetype and ZLIB are already
system-first (no vendored copy in `external/`).

**Source modes for package recipes:**

```sh
-DDVZ_VENDORED_DEPS=OFF
-DDVZ_CGLM_SOURCE=SYSTEM
-DDVZ_KVAZAAR_SOURCE=SYSTEM
-DDVZ_MIMALLOC_SOURCE=SYSTEM
```

Use explicit `SYSTEM` in CI/package builders to fail early when a declared system dependency is
missing. Keep `AUTO` for developer builds where vendored fallback is desirable.

**Package CI matrix:**

| Platform | System dependencies to install | Expected preset lane | Dependency policy |
|---|---|---|---|
| Ubuntu 24.04 | `libglfw3-dev libcglm-dev libmimalloc-dev` | `package-smoke-system-auto`, then `package-install-system-auto` | Prefer system GLFW, cglm, and mimalloc. Leave Kvazaar as `AUTO` or set `DVZ_KVAZAAR_SOURCE=VENDORED` unless the builder provides `libkvazaar-dev`. |
| Fedora | `glfw-devel mimalloc-devel` plus any available cglm/Kvazaar development packages | `package-smoke-system-auto`, then `package-install-system-auto` | Prefer system packages that exist in the target Fedora/EPEL release. Keep cglm and Kvazaar as `AUTO` unless the packaging environment explicitly provides them. |
| macOS / Homebrew | `glfw cglm kvazaar mimalloc` | `package-smoke-system-required`, then `package-install-system-required` | Homebrew has all four package-manager candidates, so this is the strict system-dependency lane. |

`msdf-atlas-gen` is intentionally absent from CI/package dependency lists. It stays
source/vendored-only for v0.4 because distro availability is too narrow for a reliable package
contract.

**Package-manager smoke commands:**

Homebrew has all four package-manager candidates:

```sh
brew install glfw cglm kvazaar mimalloc
cmake --preset package-smoke-system-required
cmake --build --preset package-smoke-system-required
cmake --build --preset package-install-system-required
```

Ubuntu 24.04 package metadata covers GLFW, cglm, and mimalloc, but Kvazaar availability is not
reliable across distributions:

```sh
sudo apt-get update
sudo apt-get install -y libglfw3-dev libcglm-dev libmimalloc-dev
cmake --preset package-smoke-system-auto
cmake --build --preset package-smoke-system-auto
cmake --build --preset package-install-system-auto
```

Use `DVZ_KVAZAAR_SOURCE=VENDORED` or leave `AUTO` when a distribution does not provide a Kvazaar
development package. Do not make msdf-atlas-gen a package-manager requirement.

Fedora/EPEL package metadata clearly covers GLFW and mimalloc. Use the system-auto lane until a
specific target release has confirmed cglm and Kvazaar development packages:

```sh
sudo dnf install -y glfw-devel mimalloc-devel
cmake --preset package-smoke-system-auto
cmake --build --preset package-smoke-system-auto
cmake --build --preset package-install-system-auto
```

Conda-forge has GLFW and mimalloc packages. cglm and Kvazaar availability should be verified in
the feedstock environment before forcing `SYSTEM`; otherwise keep those two as `AUTO` or
`VENDORED`.

---

## Implementation order

Completed:

1. Public umbrella include: `include/datoviz.h` forwards to `include/datoviz/datoviz.h`, and the
   umbrella includes `app.h`.
2. C++ public header guards: the five previously missing `EXTERN_C_ON` / `EXTERN_C_OFF` wrappers
   are present.
3. System dependency source modes for GLFW, cglm, Kvazaar, and mimalloc are implemented and
   documented.
4. `datoviz.pc.in`, CMake package install metadata, and package smoke/install presets are in place.

Active / next:

1. Wheel C integration lane: `datoviz-config`, bundled headers, wheel CMake config, and wheel smoke
   CI. Coordinate with the active wheel-build agent before editing `pyproject.toml`,
   `datoviz/cli.py`, wheel CMake config files, or `tools/release_wheels/*`.
2. FetchContent `PROJECT_IS_TOP_LEVEL` guards and subdirectory consumer smoke (6). This is
   independent of the wheel lane and is the next good non-overlapping CMake task.
3. Rpath verification in `datoviz-config` output once the console script lands (5).
4. `.deb` packaging (12) after pkg-config, dependency modes, and install metadata are stable.
5. Homebrew formula (11) after release tagging; uses the strict Homebrew system-required lane.
6. conda-forge feedstock (10) after release tagging; verify cglm/Kvazaar availability in the
   feedstock environment before forcing `SYSTEM`.
7. MSVC wheel CI job (8) after RC.
8. rpm spec file (13), Spack recipe (15), vcpkg, and conan remain post-v0.4 unless release scope
   changes.
9. Documentation pages (16) can continue in parallel; mark unfinished package-manager sections as
   "coming soon".

## Testing the full path

After each platform is wired up, validate with a minimal out-of-tree smoke test:

```bash
# pip wheel smoke test
python -m venv /tmp/dvz-test
/tmp/dvz-test/bin/pip install dist/datoviz-*.whl
export PATH="/tmp/dvz-test/bin:$PATH"
gcc scatter.c $(datoviz-config --cflags --libs) -o /tmp/scatter
/tmp/scatter
```

Add this as a CI job in `wheels.yml` (post-wheel-build, per platform). Equivalent tests for
the brew and deb paths once those are live.
