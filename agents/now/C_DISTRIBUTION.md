# C/C++ Distribution And Integration

Status: implemented distribution surface with RC3 exact base-artifact, conda, and vcpkg gates remaining. The Vulkan-enabled Qt/PyQt runtime is now published; Datoviz split-package Linux and macOS provider proof is recorded, while Windows runtime proof is unavailable and blocked by dependency trust. RC3 promotion of the provider remains undecided. Provider intake updated: 2026-10-10; earlier source, wheel, and vendor diagnostics retain their recorded identities.

Use [DISTRIBUTION_RELEASE_CHECKLIST.md](DISTRIBUTION_RELEASE_CHECKLIST.md) for commands, [../../docs/how-to/c-integration.md](../../docs/how-to/c-integration.md) for users, [../../docs/reference/build-options.md](../../docs/reference/build-options.md) for build modes, and [STATUS.md](STATUS.md) for current release blockers.

## Implemented Surface

- `include/datoviz.h` is the public umbrella include and public headers parse from C++.
- Installed CMake consumers use `find_package(datoviz CONFIG REQUIRED)` and `datoviz::datoviz`; FetchContent consumers use the same target.
- System installs provide CMake and pkg-config metadata; wheels provide headers, generated bindings, `datoviz-config`, and relocatable wheel-local CMake metadata.
- Windows wheels provide `datoviz.dll`, the MSVC import library, required runtime DLLs, and explicit DLL discovery. MSVC consumers use CMake rather than `datoviz-config`.
- Vendored, system-auto, and strict system dependency modes have local proof.
- Linux, macOS 15, and Windows wheels have hosted build, inspection, installed Python 3.10-3.14, runtime shaderc, and CMake-consumer proof from the closed RC2 campaign.
- The conda recipe uses split `libdatoviz`, `datoviz`, and optional `datoviz-qtbridge` outputs. The published Vulkan-enabled Qt/PyQt runtime has Linux x86-64 and native macOS Apple Silicon package/provider proof; see [the current Qt/PyQt handoff](QT_MACOS_VULKAN_HANDOFF.md). These local artifacts retain RC2 metadata and do not establish frozen RC3 artifact proof.
- A draft vcpkg overlay and repeatable local distribution validation tooling exist.
- The current pre-freeze tree passes local source-bundle creation, source build/install, CMake and pkg-config consumers, and the current 17-file license inventory. Installed-wheel validation now clears source-tree Python/native/provider/loader overrides, clears reused virtual environments, and asserts that Python, bindings, the native library, and CMake metadata resolve inside the installed environment.

The earlier Ubuntu-host diagnostic wheel required `manylinux_2_38` and used Debug. The 2026-09-08 [fresh Release source proof](FRACTAL_CODE_HARDENING.md#fresh-release-source-proof) and [fresh local manylinux wheel proof](FRACTAL_CODE_HARDENING.md#fresh-local-manylinux-wheel-proof) instead cover a source install and a `manylinux_2_34_x86_64` wheel from runtime checkpoint `b67d0a95d`. The source audit passes the 18-package license inventory, 111-header install inventory, CMake and pkg-config consumers, and installed-tree audit, with unavailable vcpkg and conda explicitly skipped. The wheel passes clean Python 3.13 builder checks and Python 3.12 host checks for Release configuration, precompiled shaders, shaderc, offscreen rendering, a native window under Xvfb, installed CMake and Python/C examples, all three current course programs, and the `dvz_buffer_resize` integer return binding. Both artifacts retain the development version `0.4.0rc2`; neither is a published RC2 artifact or frozen RC3 candidate. Later checkpoint `1953c2370` scopes the GCC msdf-atlas-gen warning flag to the single unused vendored `json-export.cpp` compilation, while bounded differential evidence leaves the Kvazaar warning classified as vendored. Standalone Vulkan ASan comparison isolates the NVIDIA leak signature to the proprietary driver rather than Datoviz. Exact RC3 rebuild, six-platform hosted proof, conda, and vcpkg validation remain required.

## Linux Development Package Validation, 2026-10-10

A mutable source snapshot with runtime head `62101912dfde633b4510dfbdbccf7d4a7f0b8ba2` passes a fresh Release source build/install, independent installed CMake and pkg-config consumers, the current 17-file license inventory, and the 111-header installed-prefix audit. The archive is `datoviz-0.4.0rc2-source.tar.gz` (13,928,728 bytes), SHA512 `790408eb7827020ad38aefba18deef1400bf7b4bba3d32acf201a952b64b69c03961b7ec80099a5628feccf3348b5fac80a79d21956e55c45ec626bb150ccd9e`. This development archive includes the mutable handoff state and fresh generated bindings; it is not the published RC2 archive or a frozen RC3 candidate.

An isolated manylinux builder rebuilt that snapshot as Release and produced `datoviz-0.4.0rc2-py3-none-manylinux_2_34_x86_64.whl` (21,435,330 bytes), SHA256 `9b257e54b9491dffc87cae974394282105a0786c58fac52e29b37ad841e1bc33`. Payload/dependency inspection, precompiled shader and runtime shaderc checks, and installed CMake consumers pass in the builder. A clean Python 3.13 host environment additionally passes offscreen rendering, a native window under Xvfb/Mesa llvmpipe, and installed Python/C rendering examples. Qt is absent from the base wheel and its optional probe gives the expected diagnostic. The unchanged vendored Kvazaar AVX2 compiler warning remains recorded in the earlier vendor disposition.

All three split conda outputs rebuilt from the same archive and passed their package tests. Clean indexed-channel solves preserve runtime dependency constraints; base-only rendering and shaderc pass without Qt, and [the provider retest](QT_MACOS_VULKAN_HANDOFF.md#linux-head-development-retest-2026-10-10) passes with published Qt/PyQt. Artifacts and logs remain under `/tmp/datoviz-development-*20261010*`; no artifact was published or staged. Source, wheel, and provider checks are Linux development evidence only. NVIDIA proof waits for the driver-mismatch reboot; macOS and Windows machines are unavailable.

## Remaining RC3 Gates

1. Build the final source bundle and six-wheel matrix from the exact RC3 commit; inspect native dependencies and pass Python, shaderc, CMake, rendering, and clean-install smokes.
2. Validate the base conda outputs without making the optional Qt provider an RC3 gate; confirm dependency names, install paths, Windows DLL layout, and headless import/scene behavior.
3. Validate the vcpkg overlay on Windows with vcpkg installed and replace release-source SHA512 placeholders only after exact asset publication.
4. Decide checksum/signing policy and verify third-party notices and licenses.

## Qt/PyQt Provider Status

Upstream Vulkan-enabled Qt and compatible PyQt packages are published. Linux x86-64 and native macOS Apple Silicon split-package builds, fresh-prefix checks, and hosted rendering proof are recorded in [the current Qt/PyQt handoff](QT_MACOS_VULKAN_HANDOFF.md); those results apply to their recorded development package identities, not a frozen RC3 candidate. Windows package compilation completed, but Windows application control rejected conda's GLFW dependency before Python/provider runtime checks. The physical macOS and Windows machines are currently unavailable, so new native runtime checks remain pending. The maintainer is evaluating whether to promote the provider into RC3; keep that allocation undecided until explicitly resolved. Any promoted provider still needs exact-candidate proof on the claimed platforms. Do not present the earlier RC4 allocation as an active upstream-publication blocker.

Homebrew, `.deb`, Spack, rpm, conan, Chocolatey, winget, MSYS2, nix, Docker distribution, and additional channels remain community-driven, post-v0.4, or optional unless release scope changes.

## Package Decisions

- Wheels remain the primary installed C/C++ path because they carry the library, headers, CMake files, and Python binding together.
- CMake does not accept PEP 440 prerelease text such as `0.4.0rc2` in `find_package`. RC wheels expose their numeric release segment for compatible requests such as `find_package(datoviz 0.4 CONFIG REQUIRED)`, while a stable numeric `EXACT` request does not resolve an RC wheel.
- Windows pip wheels remain MSVC/vcpkg-built; downstream executables copy the Datoviz DLL beside the executable or add the runtime directory to `PATH`.
- Conda uses dynamic conda-forge dependencies and split native, Python, and optional Qt provider outputs. Qt never becomes a base-wheel dependency.
- Headless package tests must import `datoviz` and `datoviz.raw` and create/destroy a raw scene without requiring a Vulkan device.
- One explicit release source bundle feeds wheels, conda, and vcpkg. GitHub auto-generated archives are invalid because they omit required submodule and generated content.

## Local Validation

```sh
just c-integration-smoke
just distribution-validate-local all
just distribution-validate-local audit
just wheel-ci-local <host-platform-tag>
git diff --check
git status --short
```

Do not commit generated packages, wheels, native libraries, source bundles, `data` state, or unrelated changes without exact approval.
