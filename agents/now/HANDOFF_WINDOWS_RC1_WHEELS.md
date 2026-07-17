# Windows RC1 Wheel Completion Handoff

Status: active RC1 blocker. Updated: 2026-07-17.

This is the end-to-end pickup plan for completing Windows AMD64/ARM64 RC1 wheels and making the
Windows GitHub Actions lane fast enough to iterate reliably. Read [`../../AGENTS.md`](../../AGENTS.md),
[`START.md`](START.md), [`STATUS.md`](STATUS.md), and [`RELEASE.md`](RELEASE.md) first. Execute the
steps in order and make the checkpoint commits described below.


## Safety And Publication Boundaries

1. Work on `v0.4-dev`. Preserve unrelated changes and inspect `git status --short --branch` before
   syncing.
2. Do not stage or commit `data`, submodule state, generated DLLs/libraries/wheels, build trees, or
   vcpkg payloads.
3. Run `git diff --check`, `git status --short`, and `git diff --cached --stat` before every commit.
4. Local commits are expected. Do not push or dispatch GitHub Actions without explicit approval in
   the current session.
5. A GitHub Packages NuGet binary cache writes external packages. Prepare and validate its workflow
   configuration locally, but stop for explicit approval of the exact first package publication or
   write-enabled workflow dispatch.
6. Do not execute ARM64 binaries or install the ARM64 wheel on this x64 host. Validate those
   artifacts statically and use an ARM64 runner for execution proof.
7. Avoid GPU/render tests that raise interactive Windows assertion dialogs until the underlying
   test is known safe. CPU/mock tests and static artifact inspection are safe local loops.


## Integrated Repository Position

The completed macOS/upstream work was integrated before this handoff was rewritten. At the time of
writing, the four rebased local Windows commits are:

```text
455966985 build: unify MSVC pthread implementation
5c3cb34d3 build: export C11 requirement to consumers
102fd5cae build: honor configured Windows wheel paths
013163a0f build: prevent Windows min max macro collisions
```

They sit on `e6733a4b4` (`Fix SPIR-V compute target warning`) from `origin/v0.4-dev`. Recheck the
actual hashes after any later rebase. The completed upstream/macOS sequence also includes the
retained-scene reopen fix and hardened RC release validation. Do not restart the macOS
investigation; keep its completed fixes in the final matrix.


## Proven Windows Results

### AMD64

- Full native MSVC build passes.
- Aggregate thread tests pass 4/4.
- Five focused stream CPU/mock tests pass individually.
- A fresh AMD64 wheel installs and imports from `site-packages` on CPython 3.10, 3.11, 3.12, 3.13,
  and 3.14.
- The installed-wheel CMake consumer passes.
- The original Win32 `min`/`max`, configured wheel-path, C11 consumer, and duplicate pthread
  implementation failures are covered by the four commits above.
- A local shaderc smoke exposed a missing runtime only when the local staging command omitted
  `DVZ_WHEEL_RUNTIME_DIRS`; the hosted workflow already sets that variable. Revalidate AMD64 after
  changing shared/static shaderc policy.

### ARM64 cross-build on this x64 host

- The Visual Studio 2022 ARM64 compiler is installed at the `Hostx64/arm64` toolchain location.
- The complete 609-step native build passes, including `text_atlas.cpp` and final Datoviz DLLs.
- Every produced and staged DLL has PE machine `0xAA64`.
- All builtin shaders compile to SPIR-V.
- Wheel staging and repair complete and produce
  `datoviz-0.4.0rc1-py3-none-win_arm64.whl` with 22 ARM64 DLLs.
- The generated wheel is diagnostic only and must not be published: it does not contain
  `libshaderc_shared.dll`.

The reusable local dependency state is:

```text
ARM64 installed tree: build-windows-arm64/vcpkg_installed
ARM64 native build:   build-windows-arm64-ci
vcpkg binary cache:   C:/vcpkg-binary-cache
x64 host glslc:       build/vcpkg_installed/x64-windows/tools/shaderc/glslc.exe
x64 host validator:   build/vcpkg_installed/x64-windows/tools/glslang/glslangValidator.exe
```

Do not delete or reinstall these dependencies without evidence that they are corrupt. The first
cold ARM64 vcpkg build took about 40 minutes; with the installed tree and binary archive cache
present, the Datoviz ARM64 build itself took about 151 seconds.


## Active Correctness Blocker: ARM64 Shaderc Runtime

The ARM64 build config currently records:

```text
DVZ_HAS_SHADERC=1
DVZ_SHADERC_RUNTIME_LIBRARY=libshaderc_shared.dll
```

However, the `arm64-windows` vcpkg shaderc port supplies static linkage and no ARM64
`libshaderc_shared.dll`. CMake may also discover the x64 Vulkan SDK import library while
cross-building, but Datoviz calls shaderc through lazy-loaded function pointers, so no architecture
mismatch is exposed at link time. The Windows wheel staging branch copies generic DLLs but does not
enforce `require-shaderc = true`; `delvewheel` cannot find a dependency that is opened dynamically.
The result is a wheel that builds and inspects successfully while runtime GLSL compilation fails.

Preferred resolution: use vcpkg's static shaderc target on Windows and retain the existing
lazy-loaded shared runtime on Linux/macOS. This avoids inventing and maintaining a custom ARM64
shaderc DLL build. Keep the policy explicit in CMake rather than hiding an architecture-specific
exception in the wheel script.

Before implementing, inspect the vcpkg-exported shaderc targets and transitive link set for both
`x64-windows` and `arm64-windows`. If static linkage would violate a license, CRT, symbol, or binary
size constraint, document the evidence and propose an overlay port that builds the shared runtime;
do not silently disable runtime GLSL compilation for ARM64.


## End-To-End Execution Plan

### Checkpoint 1: Make Windows shaderc architecture-correct

1. Add an explicit Windows static-shaderc mode in the Datoviz CMake configuration.
2. Link the vcpkg shaderc target and its declared transitive dependencies into the appropriate
   Datoviz target on Windows.
3. In `src/drp2/pipeline.c`, select direct shaderc symbols for the static Windows mode while keeping
   the existing lazy-loader behavior for shared-runtime platforms.
4. Ensure `DVZ_ENABLE_SHADERC=ON` fails configuration when neither a usable static target nor a
   valid shared runtime is available.
5. Add or extend focused shaderc tests so both direct/static and lazy/shared policies remain
   intentional.
6. Validate AMD64 execution locally with the non-rendering shaderc smoke.
7. Rebuild ARM64 and validate its DLL architecture statically. Do not execute it locally.

Checkpoint commit preference:

```text
build: support static shaderc in Windows wheels
```

### Checkpoint 2: Make broken Windows wheels impossible to stage

1. Update `tools/datoviz_build_backend/native_payload.py` so Windows honors
   `require-shaderc = true` according to the selected shaderc policy:
   - shared mode requires and records the correct shaderc DLL;
   - static mode records that no shaderc runtime DLL is expected.
2. Add unit tests covering missing shared runtime, successful shared runtime, and successful static
   mode. Do not infer success merely from any DLL matching `*.dll`.
3. Extend wheel inspection metadata or validation so the expected shaderc policy is visible in
   `_wheel_payload.json`.
4. Rebuild AMD64 and ARM64 wheels. Confirm every packaged DLL matches the wheel architecture and
   that debug/release duplicates are not included accidentally.
5. Run the AMD64 installed-wheel import, CLI, shaderc, and CMake-consumer checks. Inspect ARM64
   statically.

Checkpoint commit preference:

```text
test: enforce Windows wheel shaderc policy
```

### Checkpoint 3: Make the ARM64 CI build deterministic

Port the proven local cross-build requirements to `.github/workflows/wheels.yml`:

1. Set the target system/processor explicitly for ARM64 (`Windows`/`ARM64`) so CMake does not
   classify the target as x64 merely because Ninja runs on an x64 host.
2. Resolve `glslc.exe` and `glslangValidator.exe` as host tools. Never execute an ARM64 tool while
   cross-building on an x64 host.
3. Pass those paths as quoted CMake arguments; PowerShell must expand the variables rather than
   forwarding literal `$glslc` text.
4. Keep the existing PE-machine assertion and expand final wheel inspection to all DLLs.
5. Keep AMD64 and ARM64 build directories/install roots distinct.
6. Add focused workflow/configuration tests where feasible instead of relying only on a hosted run.

Checkpoint commit preference:

```text
ci: make Windows ARM64 wheel builds deterministic
```

### Checkpoint 4: Fix the existing GitHub Actions caches

The current installed-tree cache key omits the vcpkg triplet even though the cached tree is
architecture-specific. Fix this before adding another cache layer:

1. Include `${{ matrix.vcpkg_triplet }}` and relevant toolchain inputs in every installed-tree key.
2. Separate the vcpkg checkout/download state, installed tree, and binary archives.
3. Restore caches before dependency installation.
4. Save the dependency cache immediately after a successful install so a later Datoviz source
   failure does not discard the expensive dependency work.
5. Prevent AMD64 and ARM64 jobs from racing on or restoring one another's installed trees.
6. Retain useful fallback prefixes without permitting cross-triplet restores.
7. Add cache-hit/miss diagnostics and vcpkg progress output so long jobs have observable progress.

Checkpoint commit preference:

```text
ci: isolate Windows vcpkg caches by triplet
```

### Checkpoint 5: Add the optional GitHub Packages NuGet binary cache

This cache is useful to both GitHub Actions and this Windows workstation. It is an optimization,
not a prerequisite for correctness.

1. Configure a GitHub Packages NuGet source for vcpkg binary caching.
2. Give CI `contents: read` and `packages: write`; authenticate CI with `GITHUB_TOKEN`.
3. Document local read-only use with a token carrying `read:packages`. Do not commit credentials.
4. Keep triplet/toolchain ABI separation; NuGet package identity must not mix incompatible builds.
5. Prepare a one-small-package publication test and inspect its repository association,
   visibility, download access, retention, and storage impact.
6. Stop and obtain explicit approval before the first package write or write-enabled workflow
   dispatch. The current maintainer appears to have repository and organization administration, so
   no additional human approval is expected, but the external publication boundary still applies.
7. After approval, prove a cold CI restore and a read-only restore on this workstation.

Checkpoint commit preference:

```text
ci: share Windows vcpkg binaries through GitHub Packages
```

### Checkpoint 6: Run and retain the final RC evidence

1. Rebase onto the latest completed `origin/v0.4-dev` before publication validation; resolve source
   conflicts deliberately and rerun affected checks.
2. Run local AMD64 native and wheel validation, including supported Python versions and the CMake
   consumer.
3. Run the local ARM64 cross-build and static wheel inspection.
4. With explicit approval, push the checkpoint commits and dispatch the complete wheel workflow.
5. Require both Windows architectures, both macOS architectures, both Linux architectures, and all
   installed-wheel smoke jobs to pass at the exact candidate SHA.
6. Download the Windows artifacts and inspect wheel tags, payload manifests, import libraries,
   CMake package files, DLL architectures, and shaderc policy.
7. Execute the ARM64 installed-wheel shaderc smoke on an actual ARM64 runner.
8. Update `STATUS.md`, `RELEASE.md`, and distribution evidence with the exact workflow run, SHA,
   artifact checksums, and results. Remove this handoff from active dispatch once its facts are
   captured in release evidence.

Checkpoint commit preference:

```text
docs: record final Windows RC1 wheel evidence
```


## Local ARM64 Reproduction Notes

Use a fresh or known-good ARM64 build directory with the existing installed dependency tree. The
important configuration properties are:

```text
Generator:                    Ninja
CMAKE_BUILD_TYPE:             Debug (matches current CI)
CMAKE_SYSTEM_NAME:            Windows
CMAKE_SYSTEM_PROCESSOR:       ARM64
VCPKG_TARGET_TRIPLET:          arm64-windows
VCPKG_INSTALLED_DIR:           build-windows-arm64/vcpkg_installed
DVZ_ENABLE_SHADERC:            ON
DVZ_BUILD_TESTING:             OFF
DVZ_BUILD_EXAMPLES:            OFF
glslc/glslangValidator:        x64 host tools from build/vcpkg_installed/x64-windows/tools
```

After building, parse each DLL's PE header and require machine `0xAA64`. Stage with the ARM64
release/debug vcpkg `bin` directories in `DVZ_WHEEL_RUNTIME_DIRS`, explicitly pass
`--build-dir build-windows-arm64-ci`, build with `--platform-tag win_arm64`, and inspect the wheel
as a ZIP. Do not import it on this host.

The earlier build emitted an `embed_resources.cmake` warning treating a long SPIR-V list as a
single missing path. Individual SPIR-V files existed and the final link passed. Recheck embedded
shader payload metadata during the next clean build; fix it only if inspection shows a real missing
payload rather than a cosmetic list-format warning.


## Required Validation At Each Code Checkpoint

Use the narrowest relevant test while iterating, then before committing run:

```text
git diff --check
git status --short
git diff --cached --stat
```

For public header, API, binding-policy, or generator changes, also run `just ctypes` and
`just ctypes-check`. For workflow-only changes, run the repository's workflow/release automation
tests and inspect the rendered PowerShell argument paths. Final release acceptance requires
artifact validation, not merely a green compile.
