# Windows Native Validation Handoff

Status: Windows stack and build fixes are implemented on the local `fix/windows-stack-build` branch. The batch is not merged or pushed. The earlier visible DRP2 asymmetry, provider, and exact-candidate gates retain their original scope.

## Current Availability

The Windows machine was available for the 2026-11-24 source checks below. Availability for future remote or attended sessions must be confirmed when needed. The earlier conda provider run was blocked before Python import; see [the published-runtime intake](QT_MACOS_VULKAN_HANDOFF.md#published-runtime-intake-2026-10-10). Historical source and provider results do not validate an RC3 artifact. RC3 remains unfrozen during course proofreading.

## Linux-Led Windows Fixes, 2026-11-25

The code batch at `a2be5baf6` moves large scene planning, composition, graph-lowering, emitter, draw, legend, axis-label, text-layout, and recording-loader workspaces off the stack. Composition dependency validation uses a bitset. Failure paths retain transactional output and graph counts; legend allocation failures remain eligible for retry. Large composition test fixtures and the recording JSON buffer use heap storage. Repeatability compares serialized composition fields rather than unspecified C structure padding.

The default Windows build now uses MSVC x64 and Ninja in `build`. The helper selects its own Python interpreter for CMake, checks PyYAML, initializes the developer environment, rejects incompatible caches, and stages runtime DLLs beside libraries and executables. Visual Studio mixed-language targets receive C and C++ options through source properties, with a configure-time C11 atomics probe. CI uses the documented Windows route and checks Linux stack reports; hosted CI has not run for this local branch.

### Completed Linux evidence

- GCC 13.3 Debug: 373/373 configured Datoviz-owned production compilation units accounted for, including eight empty reports; 7,844 frame records and no frames above 16 KiB. Seven browser-only C sources are outside this native configuration. CUDA/NVENC and the optional Qt bridge were enabled.
- GCC 13.3 Release, CUDA disabled: 372/372 configured owned units accounted for, including seven empty reports; 3,863 frame records and no frames above 16 KiB. The browser sources and NVENC backend were excluded. The largest owned frame was 14,960 bytes in recording loading; Debug's largest was 12,352 bytes in axis text preparation.
- `direnv exec . just test scene`: 782/782 pass, no failures or skips. `just test recording`: 9/9 pass. The selected 58 CPU-only planning, composition, annotation, text, and recording cases pass under ASan with leak detection enabled.
- A broader sanitizer run reached external D-Bus/graphics-runtime leaks; it is not a clean sanitizer pass. The CPU-only run above has no reported sanitizer errors or leaks.
- `just ctypes` and `just ctypes-check` pass, including ABI validation for 207 records. The 30 Python tooling tests pass, and `just --dump` parses the recipes.
- The changed prerequisite row, Windows instructions, scatter note, and stack-analysis section in the source-build guide, plus the new build-option row, received the humanizer pass. `just docs-build-check` and `just docs-status-check` pass, including the rendered language-tab check for 408 Markdown pages and 414 HTML pages. This handoff's new section also received the humanizer pass; older sections were not re-reviewed.

### Windows evidence and limits

Native runs use a verified source hash manifest for `a2be5baf6`. Archive extraction can preserve timestamps older than existing objects, so transferred source hashes were verified and timestamps refreshed before compilation. Earlier incremental runs are diagnostic evidence only. The native compile-command audit accounts for 371 configured owned production units, with no missing or source-stale objects. The scene executable's PE header confirms a 1,048,576-byte stack reserve.

| Check | Configuration and result |
| --- | --- |
| Toolchain | Windows 11 x64, Visual Studio 2022 / MSVC 14.44.35207, Vulkan SDK 1.4.350.0, Python 3.12. CUDA and Qt bridge disabled. |
| NVIDIA Release | 781/781 scene tests pass, no failures or skips; requested and resolved GPU index 0, GeForce RTX 5060 Laptop GPU, vendor/device `0x10de/0x2d59`, API version raw `4211017`, driver version raw `2497167360`. |
| AMD Release | 781/781 scene tests pass, no failures or skips; requested and resolved GPU index 1, Radeon 780M Graphics, vendor/device `0x1002/0x1900`, API version raw `4211003`, driver version raw `8388961`. |
| NVIDIA Debug | 781/781 scene tests pass, no failures or skips; requested and resolved index 0, same device identity as Release. |
| AMD Debug | 781/781 scene tests pass, no failures or skips; requested and resolved index 1, same device identity as Release. |
| MSVC Debug analysis | Full configured build and the excluded atlas-generator target succeed. No C6262 warnings in owned production sources; five warnings remain in unchanged vendored `fpng`, with a maximum reported frame of 17,176 bytes. Other compiler/analyzer warnings remain outside the changed production files; this is not a clean general static-analysis claim. |
| Source consumers | Clean `just build Release`, default `just test planning`, Python source import, raw ctypes and array-facade smokes, and Windows manifest example discovery pass. `scatter --live 120` completes all 120 frames and exits zero; rendering and interaction were not observed. |

Both full Debug runs reported that the validation layer was unavailable. Those results establish execution and assertions; they are not validation-layer passes. Raw reports, compiler logs, source manifests, and stack inventories remain in ignored build evidence. No binaries, submodule changes, machine-access details, or reports were committed. The branch remains local.

### Resize follow-up and validation layers

A later four-case Debug run exposed an order-dependent failure in `test_app_offscreen_resize_reuses_runtime_with_mesh_and_image` on AMD. Repeating that subset reproduced the failure once in three rounds. Each offscreen resize replaced the borrowed command-buffer handle used to scope runtime resources; fresh handles could add depth targets beyond the test's expected count. The earlier full-suite passes did not expose this allocation-dependent behavior.

Commit `b106bba7b` retains the canvas-owned offscreen command buffer across image resizes and waits for the prior submission before retiring images. The canvas regression now requires the same command buffer after resize, and the scene regression requires unchanged semantic and backend object counts from the first frame onward. Image generations still advance, and retired images remain available to live-image consumers.

The elevated Windows SSH session causes the Vulkan loader to ignore layer-path environment variables. The SDK validation layer was installed but absent from the machine's explicit-layer registry. A temporary registration enabled the focused runs below; its value was removed afterward. Loader logs confirm Khronos validation-layer insertion in all four runs, with no VUID messages, validation errors, or validation warnings. Loader registry-discovery diagnostics remain; these are not warning-free loader logs.

- Linux: `just build`, the full scene suite (782/782), the canvas filter (50/50), and the isolated resize case pass. The refreshed GCC Debug inventory accounts for 373/373 configured production units, with no frame violations above 16 KiB.
- Windows Debug: `just build` passes using the existing virtual environment. On each of NVIDIA and AMD, three rounds of the four-case dynamic-emission, point-emission, retained-frame, and resize subset pass (12/12); three rounds of the canvas depth/resize case also pass (3/3). Transferred source hashes match the local manifest for the tested source.

This follow-up has focused Windows Debug evidence. The full Windows Debug/Release matrix above applies to `a2be5baf6`, before the resize correction. The revised public build instructions, option row, dispatch paragraph, and this new handoff section received the humanizer pass and a final check for direct wording; older prose was not re-reviewed.

Compiler frame reports cover the recorded compiler, configuration, and enabled sources. They do not bound total call-stack depth or validate disabled platform code. The 256 KiB worker regression exercises empty and populated planning, composition, graph lowering, and stream emission. Allocation regressions cover unchanged outputs, graph rollback after product-map allocation failure, and legend retry; dependency chains and cycles cross indices 63/64 and 127/128.

The read-only audit also identified extent-reference and load-source recursion for a separate cycle-rejection investigation; those paths were not proven defective or changed in this batch. Attended rendering and interaction, the earlier visible DRP2 asymmetry, provider trust, and exact-candidate artifact proof remain separate gates. MinGW remains an unvalidated optional route.

## Linux-Led Windows Investigation, 2026-11-24

Linux owns the investigation and fixes. Use the Windows validation host for native MSVC builds and tests when available; remote execution does not establish visible rendering or interaction. Keep access details in ignored local notes. Use high-effort Sol 6.1 subagents for independent read-only audits of stack use and Windows build paths, then integrate and validate their findings on Linux.

### Evidence

- At source head `82ef4b0a3`, a fresh Visual Studio 2022 x64 Release configuration with CUDA disabled needed PyYAML for the selected Python interpreter. The build failed because `scene.c` uses C11 atomics without `/experimental:c11atomics`: Visual Studio evaluates target-level `COMPILE_LANGUAGE:C` options as C++ in the mixed C/C++ scene target. A local source-specific option allowed the full build to complete; this partial change was not committed.
- Windows application control initially blocked vcpkg `freetype.dll` and `zlib1.dll`. The maintainer disabled Smart App Control on this host; afterward the Release scatter scenario completed 120 live frames and exited normally. No visual or interaction observation was recorded. This does not resolve the separate conda GLFW provider gate.
- The normal Release scene runner has a 1 MiB stack and crashed with stack overflow. A diagnostic copy with an 8 MiB stack passed 778/778 scene tests on AMD, with no failures or skips. Moving one 128 KiB test JSON buffer to heap made that individual case pass on the normal stack, but other scene cases still overflowed. This partial test change was not committed.
- MSVC Release object disassembly showed stack frames of 361,320 bytes in `_scene_emit_panel_render_caps`, 360,248 bytes in its callee `_scene_panel_render_plan_build`, and 200,056 bytes in nested `_scene_panel_composition_resolve`. Their combined frame sizes are 921,624 bytes before other frames. `DvzPanelRenderPlan` is 360,192 bytes and `DvzPanelCompositionSnapshot` is 199,912 bytes in that build. These measurements establish production stack pressure, not the exact crash call stack.
- On Windows, `just build` selects the MinGW recipe and requires GCC; this MSVC-only environment lacks GCC. `just docs-build-check` depends on that recipe and failed before its documentation checks. The separate `msvc` preset uses a multi-configuration `build-msvc` tree, while tests, Python loading, examples, and documentation recipes also assume other build layouts. A partial public-guide update was not committed.

### Work plan

1. Audit large stack objects and nested call paths across production and tests on Linux using compiler stack-usage reports and targeted call-chain analysis. Reproduce measurements with MSVC where possible. Prioritize render-plan and composition drafts; also check worker-thread and embedding paths. Capture exact crash stacks where practical.
2. Design and implement owned heap or suitably scoped workspace storage for large working objects. Preserve transactional output behavior, concurrency, cleanup on every failure path, and allocation-failure handling. Do not use shared static scratch space. Reduce large test fixtures separately. A larger test executable stack alone is not a library fix.
3. Add focused regression coverage for the affected planning and failure paths, including a deliberately bounded worker thread. Run Linux builds, focused tests, and suitable allocation or sanitizer checks. On Windows, run Debug and Release focused tests and the full scene suite with the normal 1 MiB stack; distinguish platform skips and validation messages. Repeat relevant GPU checks on both devices when available.
4. Fix compiler-option handling for C and C++ across mixed Visual Studio targets, with a configure-time capability diagnostic if needed. Evaluate per-source options or separate object targets without losing generated shaders, exports, or dependency propagation. Verify generated commands for both Visual Studio and Ninja generators.
5. Design a consistent Windows build workflow. Evaluate MSVC plus Ninja in `build` as the default, with an initialized developer environment and explicit diagnostics; retain MinGW as an explicit separate route only if validated. Audit all consumers of build directories, configuration-specific paths, Python native-library discovery, runtime DLL staging, tests, examples, and documentation. Do not silently reconfigure an incompatible existing build tree. Make Windows CI exercise the documented path.
6. Update the public source-build guide after the chosen workflow works from a clean environment. Include PyYAML and interpreter selection, toolchain prerequisites, runtime paths, and appropriate test commands. Apply humanizer to revised prose and run the required documentation checks with their declared dependencies on a supported host.
7. Record source revision, configurations, commands, GPU identity, test totals, skips, warnings, and any remaining limitation. Perform attended rendering, resize, interaction, close, and reopen checks separately. Source-checkout evidence does not replace exact-candidate artifact validation.

The plan and new evidence above received the humanizer pass. Earlier prose in this handoff was not reviewed in this pass.

## Scope

This is the operational handoff for the physical Windows 11 AMD/NVIDIA laptop. Use [HANDOFF_GPU_SELECTION.md](HANDOFF_GPU_SELECTION.md) for the GPU-selection contract, [DISTRIBUTION_RELEASE_CHECKLIST.md](DISTRIBUTION_RELEASE_CHECKLIST.md) for package gates, [../../spec/testing/INTERACTION_LATENCY.md](../../spec/testing/INTERACTION_LATENCY.md) for the accepted frame-slot and pacing contract, and [../../spec/release/PHYSICAL_VALIDATION.md](../../spec/release/PHYSICAL_VALIDATION.md) plus [../../docs/contributors/release-physical-validation.md](../../docs/contributors/release-physical-validation.md) for exact-candidate physical proof.

Source-checkout results are development evidence only. They must not be reported as installed-wheel or release-candidate evidence.

## Completed Development Baseline

- The host is Windows 11 x64 with Visual Studio 2022, MSVC 14.44.35207, Windows SDK 10.0.26100.0, Vulkan SDK 1.4.350.0, CUDA Toolkit 13.2, Qt 6.11.1 MSVC 2022 64-bit, and Node.js LTS 24.18.1. Vulkan GPU 0 is AMD Radeon 780M Graphics and GPU 1 is NVIDIA GeForce RTX 5060 Laptop GPU.
- The affected Debug and Release `canvas`, `gui`, `vk`, `vklite`, and `video` matrix passed on both GPUs with no failures or Vulkan validation messages. Expected capability skips remained distinct from failures.
- Runtime commit `e179adeda` passed the full modular Debug and Release suites on the NVIDIA Vulkan ICD: 1,074 selected, 1,070 passed, 0 failed, and four expected Windows/platform skips in each configuration.
- The full specification suite passes after the Windows file-URI and portable JSON-path fixes and installation of the required Python `jsonschema` dependency.
- Debug and Release NVIDIA NVENC spot checks passed; AMD produced the expected unsupported-device skip.
- The native Qt bridge builds at `build-msvc/qtbridge/<Configuration>/datoviz_qtbridge.dll`.
- The checkout-backed `x64-windows` vcpkg overlay and standalone Debug/Release CMake consumers passed with vcpkg commit `39344dff01c5a5a0134caf2624cdd492f05d30ea` and MSVC 14.44.35207.
- The completed physical FIFO comparison used candidate `2c32d3e92fac41785ea063e6677bd4ae688f57cf` on the production-default AMD path at 1920×1080 and 144 Hz. One slot measured -1.94% paired p95 input-to-submit, two slots -0.25%, and automatic three slots +0.16%; all verdicts were no material change. Required Canvas cases passed on AMD and NVIDIA with no validation messages, scripted real-input smokes rendered, resized, returned to idle, and closed normally, and the maintainer reported all three configurations smooth. The accepted cross-platform one-slot fallback and its limitations are recorded in `spec/testing/INTERACTION_LATENCY.md`.
- Interactive Release `start/scatter` with `DVZ_PRESENT_MODE=fifo-latest` and `DVZ_FPS_CAP=60` had smooth physical pan/zoom behavior.
- Retained local reports and logs remain outside the repository under `Documents/Codex/2026-08-01/je-configure-ce-pc-windows-11/outputs/`; do not commit them.

## 1. Resolve And Repeat The Visible AMD/NVIDIA Checks

The automated symmetric source campaign is complete, but the earlier scripted Release GLFW screenshot check rendered one orange point on NVIDIA and a blank black frame on AMD; `--scene-points 100000` was parsed and reported but the NVIDIA screenshot still contained one point. Both processes closed normally and reported no Vulkan validation error. This visible gate remains open.

1. Determine why the DRP2 live canvas is blank on AMD and why the requested 100,000-point grid appears as a single point on NVIDIA.
2. Repeat the Release GLFW live canvas on GPU 0 and GPU 1 after affected runtime changes land.
3. Confirm visible rendering, resize, normal close, and reopen on each window; record pass, fail, or skip with a reason.
4. Keep expected capability skips separate from the unexplained asymmetry: AMD has no NVENC and lacks the tested external-memory path, while the NVIDIA GLFW lifecycle case has an expected platform skip.

```powershell
& .\build-msvc\testing\Release\dvztest.exe --list-gpus
& .\build-msvc\testing\Release\dvztest.exe --module <module> --gpu 0 --json <amd-report.json>
& .\build-msvc\testing\Release\dvztest.exe --module <module> --gpu 1 --json <nvidia-report.json>
& .\build-msvc\testing\Release\dvz_live_canvas.exe --backend glfw --gpu 0 --frames 300
& .\build-msvc\testing\Release\dvz_live_canvas.exe --backend glfw --gpu 1 --frames 300
```

Do not treat `VK_LOADER_DRIVERS_SELECT` as the selector contract; it is only an optional cross-check.

## 2. Resolve The Qt/PyQt Provider Blocker

Qt 6.11.1, the native Release bridge, and PyQt6 6.11.0 are installed. The bridge loads and reports ABI 1 with Qt 6.11.1, but the PyPI Windows PyQt6 package does not expose `QVulkanInstance`; `python -m datoviz.qt` therefore stops with the intended Vulkan-support diagnostic. This is a provider limitation, not a Datoviz rendering failure, and it does not replace the conda-provider campaign in [QT_MACOS_VULKAN_HANDOFF.md](QT_MACOS_VULKAN_HANDOFF.md).

1. Obtain a Windows PyQt6 build that exposes `QVulkanInstance` and matches the managed Qt runtime, or validate an explicitly supported alternative provider.
2. Run `python -m datoviz.qt` and `python examples/python/qt/hosted_pyqt.py --smoke-ms 1000` with `DATOVIZ_QTBRIDGE_LIBRARY` pointing at the matching bridge.
3. Run one visible hosted window and confirm rendering, resize, interaction, normal close, and reopen.

```powershell
$env:DATOVIZ_QTBRIDGE_LIBRARY = (Resolve-Path .\build-msvc\qtbridge\Release\datoviz_qtbridge.dll).Path
python -m datoviz.qt
python .\examples\python\qt\hosted_pyqt.py --smoke-ms 1000
```

## 3. Validate The Exact Candidate

Start only after the accepted RC source bundle and Windows wheel are fixed and identified.

1. Create a clean environment that cannot import or load Datoviz from the source checkout.
2. Record the version, artifact and release commits, successful wheel workflow, wheel filename, and SHA256 checksum.
3. Run the installed-artifact validator, import/ABI checks, native dependency inventory, CMake consumer, and selected automated rendering profile.
4. Perform the canonical live set on the same machine: 2D pan/zoom, 3D controls, text/layout resize, image or color-scale interaction, textured mesh, picking/query/readback, close, and reopen.
5. Record every human observation as pass, fail, or skip with a reason; process exit status alone is not a visual pass.
6. Repeat affected evidence whenever the wheel checksum or a runtime-affecting commit changes.
7. Replace the vcpkg overlay's placeholder source URL and SHA512 only after the exact release source asset exists, then repeat the clean overlay and installed consumer proof. Never substitute GitHub's automatically generated source archive.

Hosted Windows exact-artifact validation remains mandatory for RC3. Final v0.4.0 also requires physical Windows proof or an explicit maintainer-approved exception.

## Evidence Discipline

- Keep generated binaries, recordings, JSON reports, logs, source bundles, wheels, and `data` state out of ordinary code commits.
- Record exact commit, toolchain, OS, architecture, GPU/driver, commands, totals, skips, checksums, and artifact identities where applicable.
- A crash, Vulkan validation message, wrong selected-device identity, or unexplained asymmetric result remains open until fixed or explicitly dispositioned.
