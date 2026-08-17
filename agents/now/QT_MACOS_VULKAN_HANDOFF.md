# Qt/PyQt macOS Vulkan Handoff

Status: local implementation and Apple Silicon proof complete; Qt is merged but package publication, compatible PyQt, and exact Datoviz artifacts still block the RC3 provider gate. Updated: 2026-08-18.

This handoff records the verified local Qt, PyQt, and Datoviz provider artifacts; the published feedstock pull requests; the expected dependency-order CI failures; and the remaining maintainer and exact-artifact sequence. Upstream feedstock work remains in sibling repositories, not the Datoviz source tree.

## Goal

Deliver a conda-first `datoviz-qtbridge` provider for RC3 without adding Qt or PyQt to the base Datoviz wheel contract. The gate requires published Vulkan-enabled Qt packages, compatible published PyQt bindings, split Datoviz packages built against that managed runtime, and exact-artifact hosted validation.

## Repository Rules

Read [../../AGENTS.md](../../AGENTS.md), [START.md](START.md), [STATUS.md](STATUS.md), and [RELEASE.md](RELEASE.md) before acting. Keep all upstream clones and build products outside tracked Datoviz paths, preferably as sibling directories. Do not stage or commit the Datoviz `data` submodule or generated/runtime binaries. Do not push, open a pull request, comment, rerun external CI, or otherwise publish without the maintainer's explicit approval of the exact final content and action.

## Current Upstream State

- [qt-main-feedstock PR #406](https://github.com/conda-forge/qt-main-feedstock/pull/406), head `36d761a20ad74615a5189bc69510be92ec4dc5d8`, was merged on 2026-08-17 at merge commit `671db17c3e462e01980e11b7ffd5efceb0b0e366`. It enables Qt Vulkan on macOS with `libvulkan-headers`, `libvulkan-loader`, and `moltenvk`; bumps Qt 6.11.1 from build 1 to build 2; and adds compile and package guards. Post-merge builds are running, and build 2 is not yet published for either macOS architecture.
- [pyqt-feedstock PR #186](https://github.com/conda-forge/pyqt-feedstock/pull/186), head `c8d45d0ff21f6a18824e16a1abfb46dffaa027b4`, builds PyQt6 6.11.0 against Qt 6.11.1, makes `libvulkan-headers` available to native and cross builds, exports the header path for SIP feature probes, removes the cross-build `PyQt_Vulkan` disable, adds a `QVulkanInstance` regression test, and bumps build 2 to build 3. It remains a draft until its macOS matrix can use published Qt build 2.
- PyQt PR #186 passes all five Windows jobs. All five native macOS x86-64 jobs build against currently published non-Vulkan Qt and fail the new package test because `QVulkanInstance` is absent. All five cross-built macOS ARM64 jobs generate the Vulkan binding surface but fail compiling `<qvulkaninstance.h>` because that header is absent from the currently published target Qt package. The compiler already includes the target `include/qt6/QtGui` directory, so adding another include path is not the fix.
- No additional PyQt recipe change is justified from the current failures. The next meaningful PyQt CI run must occur after conda-forge publishes Qt 6.11.1 build 2 for both macOS architectures.
- The historical [qt-main-feedstock PR #170](https://github.com/conda-forge/qt-main-feedstock/pull/170) and [vulkan-headers-feedstock issue #7](https://github.com/conda-forge/vulkan-headers-feedstock/issues/7) explain why macOS Vulkan was previously disabled and why the now-available MoltenVK package removes that packaging limitation.

## Completed Local Proof

- The native Apple M1 Pro Qt build produced `qt6-main-6.11.1-pl5321hd92956d_2.conda`. Qt reports `QT_FEATURE_vulkan 1`; the package contains the public `QVulkan*` headers and Cocoa plugin; the compile/link package test passes; MoltenVK loads through the packaged Vulkan loader; `QVulkanInstance::create()` succeeds; and a Cocoa Vulkan window produces a non-null surface.
- The native PyQt build produced `pyqt6-6.11.0-py310h8bf5152_3.conda`, SHA-256 `932166fcb6bd54a4d4beec9226bd452dc98729cabcdbd2709710dc9377410f0c`. All three PyQt outputs build, the Vulkan import/API assertion passes, and the isolated MoltenVK instance and Cocoa surface probes succeed.
- The Datoviz split recipe builds `libdatoviz`, `datoviz`, and `datoviz-qtbridge`. The packaged bridge loads as `libdatoviz_qtbridge.dylib`, reports ABI 1 and Qt 6.11.1, reads back the adopted Vulkan instance, and completes the hosted PyQt rendering smoke from a fresh prefix.
- Local native proof does not substitute for conda-forge publication, the feedstock cross-build matrix, or supported hosted-platform exact-artifact validation.

## Conda Vulkan Runtime Ownership

Conda environments must use the conda-managed Vulkan loader and MoltenVK installation rather than copying the standalone wheel payload into the split packages. `libdatoviz` dynamically loads `libvulkan-loader`, Qt uses the same environment-owned loader, and Qt adopts the Datoviz-created Vulkan instance through the bridge. A normal solved environment must therefore contain one loader and one MoltenVK implementation.

The base `libdatoviz` output must depend on `moltenvk` on macOS independently of the optional Qt provider. Depending only on `libvulkan-loader` supplies entry points but not a Vulkan driver, while relying on `qt6-main` to bring MoltenVK would make base Datoviz rendering accidentally depend on installing the optional bridge. The current conda-forge MoltenVK package installs its relocatable ICD manifest under `$PREFIX/share/vulkan/icd.d/` and declares a macOS 14 minimum; exact package validation must record that provider floor.

Do not claim support for mixing the standalone macOS Datoviz wheel, which carries a private loader, MoltenVK library, and ICD manifest, with conda-managed Qt/PyQt. The supported provider path uses the mutually pinned conda outputs and verifies the resolved loader and driver from a clean prefix.

## Remaining RC3 Sequence

1. Confirm Qt 6.11.1 build 2 is published for `osx-64` and `osx-arm64` and contains the Vulkan-enabled QtGui package surface.
2. Restart PyQt PR #186 CI through an explicitly approved conda-forge action or comment, then require green native and cross-built macOS jobs rather than weakening the Vulkan test.
3. Mark PyQt PR #186 ready for review only after its compatible matrix is green; then wait for maintainer merge and package publication.
4. Build the Datoviz split packages against the published Qt/PyQt runtime, run a clean-prefix base macOS Vulkan render without Qt, then run exact-artifact bridge, import, loader/driver identity, Vulkan instance, Cocoa surface, hosted rendering, and missing-provider diagnostics.
5. Add mandatory Linux and Windows hosted proof for RC3.
6. Do not cut RC3 while this required provider gate is unavailable unless the maintainer explicitly changes release scope and records the exception.

## Publication Guardrail

Read-only monitoring is allowed. Do not edit either pull request, post a comment, request or rerun CI, push another commit, mark PyQt ready, or perform any other public action without explicit approval of the exact content and action. Use the authenticated `gh` CLI for approved mutations so they appear under the maintainer's identity.
