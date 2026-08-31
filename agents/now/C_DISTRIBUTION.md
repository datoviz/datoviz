# C/C++ Distribution And Integration

Status: implemented distribution surface with RC3 exact base-artifact, conda, and vcpkg gates remaining; official Qt/PyQt provider artifacts are RC4. Updated: 2026-08-31.

Use [DISTRIBUTION_RELEASE_CHECKLIST.md](DISTRIBUTION_RELEASE_CHECKLIST.md) for commands, [../../docs/how-to/c-integration.md](../../docs/how-to/c-integration.md) for users, [../../docs/reference/build-options.md](../../docs/reference/build-options.md) for build modes, and [STATUS.md](STATUS.md) for current release blockers.

## Implemented Surface

- `include/datoviz.h` is the public umbrella include and public headers parse from C++.
- Installed CMake consumers use `find_package(datoviz CONFIG REQUIRED)` and `datoviz::datoviz`; FetchContent consumers use the same target.
- System installs provide CMake and pkg-config metadata; wheels provide headers, generated bindings, `datoviz-config`, and relocatable wheel-local CMake metadata.
- Windows wheels provide `datoviz.dll`, the MSVC import library, required runtime DLLs, and explicit DLL discovery. MSVC consumers use CMake rather than `datoviz-config`.
- Vendored, system-auto, and strict system dependency modes have local proof.
- Linux, macOS 15, and Windows wheels have hosted build, inspection, installed Python 3.10-3.14, runtime shaderc, and CMake-consumer proof from the closed RC2 campaign.
- The conda recipe uses split `libdatoviz`, `datoviz`, and optional `datoviz-qtbridge` outputs. Local Apple Silicon package and hosted Qt bridge proof is green.
- A draft vcpkg overlay and repeatable local distribution validation tooling exist.
- The current pre-freeze tree passes local source-bundle creation, source build/install, CMake and pkg-config consumers, and the 18-package license inventory. Installed-wheel validation now clears source-tree Python/native/provider/loader overrides, clears reused virtual environments, and asserts that Python, bindings, the native library, and CMake metadata resolve inside the installed environment.

Local wheel probing is intentionally preliminary: an Ubuntu 24 host cannot repair to `manylinux_2_34` because its build references GLIBC 2.38, while a repaired `manylinux_2_38` diagnostic wheel passes isolated Python, shaderc, rendering, native-window, CMake, and installed Python/C examples but correctly fails the Release gate because the current checkout build is Debug. The exact RC3 wheel must be built as Release in the supported manylinux image after the version freeze.

## Remaining RC3 Gates

1. Build the final source bundle and six-wheel matrix from the exact RC3 commit; inspect native dependencies and pass Python, shaderc, CMake, rendering, and clean-install smokes.
2. Validate the base conda outputs without making the optional Qt provider an RC3 gate; confirm dependency names, install paths, Windows DLL layout, and headless import/scene behavior.
3. Validate the vcpkg overlay on Windows with vcpkg installed and replace release-source SHA512 placeholders only after exact asset publication.
4. Decide checksum/signing policy and verify third-party notices and licenses.

## Remaining RC4 Provider Gate

Publish Vulkan-enabled Qt and compatible PyQt packages, then build the exact conda split outputs against that managed runtime and validate the optional provider on hosted Linux, macOS, and Windows. Confirm provider diagnostics and feedstock CI results without changing the base-wheel contract.

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
