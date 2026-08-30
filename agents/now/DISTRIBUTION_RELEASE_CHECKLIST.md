# Distribution Release Checklist

Status: active RC3/final release gate. Updated: 2026-08-30.

Use this checklist against the exact candidate commit before live workflows or publication. Current blocker state belongs in [STATUS.md](STATUS.md); distribution decisions belong in [C_DISTRIBUTION.md](C_DISTRIBUTION.md).

## Artifact Policy

- Use one explicit release source bundle for wheels, conda, and vcpkg; never use GitHub auto-generated archives.
- Include generated ctypes, required external submodules, nested submodules, and required font assets.
- Keep Qt/PyQt outside base wheels. The optional conda provider must use one compatible managed Qt runtime.
- Do not commit generated packages, wheels, source bundles, runtime libraries, or unapproved `data` state.

## Local Source Preflight

```sh
just build
just ctypes
just ctypes-check
just c-integration-smoke
just distribution-validate-local all
just distribution-validate-local audit
just release-source-bundle <version>
```

Record the source bundle filename and SHA512. Confirm installed headers, Datoviz libraries, runtime paths, pkg-config metadata, CMake package metadata, and small CMake/pkg-config consumers.

## Wheel Matrix

Run local CI parity on an available host:

```sh
just wheel-ci-local <host-platform-tag>
```

The exact candidate campaign must prove:

1. Linux `x86_64` and `aarch64` wheels build, repair, inspect, and install.
2. macOS 15 `x86_64` and `arm64` wheels build, repair, inspect, and install without dishonest lower deployment tags.
3. Windows AMD64 and ARM64 wheels build, inspect, and install with `datoviz.dll`, `datoviz.lib`, CMake metadata, and required runtime DLLs.
4. Python 3.10-3.14 clean-install smokes pass on supported hosted platforms. The native Windows ARM64 setup-python manifest currently supplies 3.11-3.14, while 3.10 remains AMD64-only; keep the ARM64 matrix explicit and do not claim a native 3.10 smoke until the provider publishes it.
5. Runtime shaderc compilation and the installed CMake consumer pass from each eligible artifact.
6. The rewritten Vulkan course passes against the exact package version newer than RC2.
7. Native dependency inspection shows no missing or unintended libraries.

## RC3 conda matrix

The exact RC3 candidate must prove:

1. The base `libdatoviz` and `datoviz` outputs use correct dependency names, install paths, run exports, and Windows DLL layout.
2. Headless base-package tests import `datoviz` and `datoviz.raw` and create/destroy a raw scene without a Vulkan device.
3. A clean macOS base-package prefix without Qt or a system Vulkan SDK resolves the conda-managed loader and MoltenVK ICD, creates a Vulkan instance, and completes an offscreen render.
4. Supported hosted base-package tests are green before publication. Official `datoviz-qtbridge` artifacts are deferred to RC4 and are not claimed by RC3.

## RC4 Qt provider matrix

After compatible PyQt publication, the exact provider artifacts must prove:

1. Published Qt exposes the Vulkan-enabled QtGui surface on every claimed target and published PyQt exposes `QVulkanInstance` against that same Qt ABI/runtime.
2. The macOS Qt-provider prefix resolves one conda-managed Vulkan loader and MoltenVK implementation rather than a standalone-wheel runtime payload.
3. The bridge imports, diagnoses a missing provider cleanly, creates a Vulkan instance and host surface where supported, and completes hosted rendering from exact packages.
4. Supported hosted Linux, macOS, and Windows provider tests are green before publication.

## vcpkg Matrix

1. Replace the placeholder source SHA512 only after the exact source asset exists.
2. Validate the overlay on Windows with a clean vcpkg installation and supported triplet.
3. Build and run a consumer using the installed package target and runtime DLL layout.
4. Submit an official registry change only after overlay and hosted proof are clean and exact publication is approved.

## Release Quality And Evidence

Before accepting artifacts:

1. Run relevant unit, integration, rendering, Vulkan-validation, documentation, gallery, example, sanitizer where practical, and long-loop gates.
2. Record exact commit, toolchain, platform, architecture, GPU/driver where relevant, commands, test totals, skips, checksums, and artifact identities.
3. Treat unavailable physical hardware as an exclusion, never a pass.
4. Verify licenses, third-party notices, bundled provider licenses, and checksum/signing policy.
5. Run `git diff --check`, inspect `git status --short`, and inspect `git diff --cached --stat` before any commit or publication action.

## Live Publication Sequence

Live GitHub workflows, source-asset uploads, package-manager submissions, TestPyPI/PyPI uploads, tags, releases, and announcements require the approvals defined in `AGENTS.md`.

After approval:

1. build and checksum the exact source bundle;
2. run and inspect the exact wheel/provider matrices;
3. validate clean installs and external consumers from downloaded artifacts;
4. attach immutable evidence and checksums;
5. publish only the reviewed artifact set;
6. verify package indexes and release assets independently after publication.
