# Windows Native Validation Handoff

Status: source-checkout AMD/NVIDIA matrices, FIFO frame-slot comparison, and checkout-backed vcpkg consumer proof are complete; visible DRP2 live-canvas asymmetry, a Vulkan-enabled PyQt provider, and exact-candidate proof remain. Updated: 2026-08-03.

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
