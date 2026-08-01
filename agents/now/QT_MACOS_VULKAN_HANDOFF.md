# Qt/PyQt macOS Vulkan Handoff

Status: ready for local Apple Silicon investigation and implementation. Prepared: 2026-08-01.

This handoff owns a local, unpublished experiment to enable Qt and subsequently PyQt Vulkan bindings on conda-forge macOS packages. Run the work natively on an Apple Silicon Mac from a logged-in desktop session. Do not modify the Datoviz source tree for the upstream feedstock changes.

## Goal

Prove that conda-forge can build `qt6-main` on macOS with `QT_FEATURE_vulkan`, load MoltenVK through the conda-forge Vulkan loader, create a `QVulkanInstance`, and create a Cocoa Vulkan window surface. Preserve a focused patch and evidence suitable for maintainer review. PyQt changes are a second checkpoint after the Qt package works.

## Repository Rules

Read [../../AGENTS.md](../../AGENTS.md), [START.md](START.md), [STATUS.md](STATUS.md), and [RELEASE.md](RELEASE.md) before acting. Keep all upstream clones and build products outside tracked Datoviz paths, preferably as sibling directories. Do not stage or commit the Datoviz `data` submodule or generated/runtime binaries. Do not push, open a pull request, comment, rerun external CI, or otherwise publish without the maintainer's explicit approval of the exact final content and action.

## Confirmed Evidence

- [pyqt-feedstock PR #186](https://github.com/conda-forge/pyqt-feedstock/pull/186), head `ef557951fbcbafd4ee302a6809e6afa0b9749e06`, adds Vulkan headers and a `QVulkanInstance` regression test. Native macOS builds complete but report `Disabled QtGui bindings features: PyQt_OpenGL_ES2, PyQt_XCB, PyQt_Wayland, PyQt_Vulkan`, then fail the new import. Adding headers only to the PyQt host environment cannot restore a feature absent from the Qt package.
- [qt-main-feedstock PR #170](https://github.com/conda-forge/qt-main-feedstock/pull/170) enabled Qt Vulkan in 2023 only on non-macOS platforms. Its macOS attempt failed because Qt included `MoltenVK/mvk_vulkan.h` and no conda-forge MoltenVK package existed. This was a packaging limitation, not a Qt architectural limitation.
- [vulkan-headers-feedstock issue #7](https://github.com/conda-forge/vulkan-headers-feedstock/issues/7) tracked MoltenVK packaging and is now closed as completed.
- [moltenvk-feedstock](https://github.com/conda-forge/moltenvk-feedstock), observed main `854856123c7d5efa4a251875ff79301bc7282c09`, publishes `moltenvk 1.4.1` for `osx-64` and `osx-arm64`. It installs `lib/libMoltenVK.dylib`, `share/vulkan/icd.d/MoltenVK_icd.json`, and `include/MoltenVK/*.h`; its run export is bounded to the MoltenVK minor series.
- [qt-main-feedstock](https://github.com/conda-forge/qt-main-feedstock), observed main `6d4c63e6d236b14d33da54d296d7d999b7560476`, currently places `libvulkan-headers` and `libvulkan-loader` under `not osx` and enables `FEATURE_vulkan=ON` only on Linux.
- Qt 6.11.1 still conditionally includes `MoltenVK/mvk_vulkan.h` from `src/plugins/platforms/cocoa/qcocoawindow.h`. Its Cocoa plugin conditionally compiles `qcocoavulkaninstance.mm`, requests `VK_EXT_metal_surface` and `VK_MVK_macos_surface`, and creates a Vulkan surface from a `CAMetalLayer`.
- `vulkan-headers` is an obsolete conda-forge package at version `1.3.231.1`. Use current `libvulkan-headers`, aligned with the Qt recipe, for new work.

## Local Workspace

Start in the Datoviz repository, verify `uname -m` reports `arm64`, then clone the upstream feedstock as a sibling rather than inside Datoviz:

```sh
cd ..
git clone https://github.com/conda-forge/qt-main-feedstock.git
cd qt-main-feedstock
git switch -c agent/macos-vulkan
```

Record the actual upstream commit before editing because the observed hashes above may have advanced.

## Proposed Qt Patch

First inspect the current recipe and rendered configurations. Keep the change minimal:

1. Bump the `qt6-main` build number because the version is unchanged.
2. Make `libvulkan-headers` and `libvulkan-loader` host requirements available on macOS as well as the currently supported platforms. Preserve the existing platform treatment of unrelated requirements such as `freetype`.
3. Add `moltenvk` as a macOS host requirement and verify its run export produces an appropriate runtime dependency in the rendered `qt6-main` package.
4. Add `-DFEATURE_vulkan=ON` to the Darwin build configuration, including the `osx-arm64` target configuration.
5. Add focused package/build/runtime tests. Avoid broad recipe refactoring or unrelated dependency updates.

Do not assume the exact YAML shape from this handoff; follow the current v1 recipe conventions in the cloned feedstock.

## Native Build

Use an ARM64 Miniforge installation and a normal macOS Terminal session, not a Rosetta shell. Ensure Xcode Command Line Tools are installed and reserve substantial free disk space. The generated local build helper requires an absolute writable SDK directory:

```sh
export OSX_SDK_DIR="$PWD/.conda-sdks"
mkdir -p "$OSX_SDK_DIR"
python build-locally.py osx_arm64_ 2>&1 | tee qt-osx-arm64.log
```

The feedstock currently uses `rattler-build`, and `.ci_support/osx_arm64_.yaml` is the relevant configuration. Preserve the complete log and note wall time, peak disk pressure if visible, package filenames, and the `build_artifacts` location. A native M1 build is valuable proof but does not replace the feedstock's generated cross-build CI configuration; report any difference in build and target platform behavior.

## Qt Acceptance Tests

The Qt checkpoint is complete only when all applicable claims have direct evidence:

1. The configure summary reports Vulkan enabled for QtGui; a silent or auto-disabled feature is a failure.
2. A C++ compile guard using `#if !QT_CONFIG(vulkan)` succeeds against the produced package.
3. A C++ program includes, links, constructs, and calls `QVulkanInstance` using the installed public Qt package.
4. The installed Cocoa platform plugin contains the Vulkan implementation expected from `qcocoavulkaninstance.mm`.
5. In an isolated environment sourced from the local `build_artifacts` channel plus conda-forge, the Vulkan loader discovers the packaged MoltenVK ICD and `QVulkanInstance::create()` succeeds.
6. From a logged-in desktop session, a `QWindow` configured as `QSurface::VulkanSurface` accepts the instance and `QVulkanInstance::surfaceForWindow()` returns a non-null surface.
7. Package metadata contains an appropriate MoltenVK and Vulkan-loader runtime dependency; the test must not succeed only because those packages leak in from the build environment.
8. Existing focused feedstock tests still pass, and `git diff --check` is clean.

For runtime diagnosis, preserve the relevant `qt.vulkan` logging, loader diagnostics, environment package list, architecture output, and exact probe source. Do not weaken a failing runtime assertion merely to obtain a green build.

## PyQt Follow-up

Do not begin this checkpoint until the locally built Qt package passes the Qt acceptance tests. Then prepare a separate local clone and branch for `pyqt-feedstock`:

1. Build PyQt against the locally produced Vulkan-enabled `qt6-main` package.
2. Replace the PR's `vulkan-headers` dependency with current `libvulkan-headers`.
3. Reassess and preferably remove the cross-build `--disabled-feature=PyQt_Vulkan` override now that target Qt headers expose the feature. Preserve it only where a demonstrated target-platform limitation remains.
4. Confirm the SIP configuration no longer lists `PyQt_Vulkan` as disabled and verify the generated binding sources contain `QVulkanInstance` and `QWindow.setVulkanInstance` before compiling.
5. Run native Apple Silicon imports, `QVulkanInstance.create()`, Cocoa surface creation, and the Datoviz Qt bridge rendering smoke in an isolated environment containing the local Qt and PyQt artifacts.
6. Keep the existing Linux and Windows regression intent. The Linux CI failure in PR #186 is independent: GCC/G++ 14.4 activation now yields command names instead of absolute paths, causing dangling symlinks in `build-pyqt-sip.sh`; resolve commands with `command -v` or an equivalently robust mechanism.

## Deliverable

Return a concise report containing the upstream base commit, exact diff, rendered dependencies, build commands, artifact filenames and hashes, probe sources, complete pass/fail results, relevant log excerpts, and remaining risk. Keep local commits focused if useful, but do not push them. If the Qt checkpoint fails, identify the earliest concrete cause and preserve the smallest reproducer rather than proceeding to PyQt.
