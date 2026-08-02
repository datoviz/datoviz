# Windows Native Validation Handoff

Status: native development environment and source-build validation are green; symmetric AMD/NVIDIA, local PyQt hosting, Windows vcpkg, and exact-candidate physical proof remain. Updated: 2026-08-02.

## Scope

This is the operational handoff for the physical Windows 11 AMD/NVIDIA laptop. Use [HANDOFF_GPU_SELECTION.md](HANDOFF_GPU_SELECTION.md) for the GPU-selection contract, [DISTRIBUTION_RELEASE_CHECKLIST.md](DISTRIBUTION_RELEASE_CHECKLIST.md) for package gates, and [../../spec/release/PHYSICAL_VALIDATION.md](../../spec/release/PHYSICAL_VALIDATION.md) plus [../../docs/contributors/release-physical-validation.md](../../docs/contributors/release-physical-validation.md) for exact-candidate physical proof.

Source-checkout results are development evidence only. They must not be reported as installed-wheel or release-candidate evidence.

## Machine And Completed Baseline

- Windows 11 x64 with Visual Studio 2022, MSVC 14.44.35207, Windows SDK 10.0.26100.0, Vulkan SDK 1.4.350.0, CUDA Toolkit 13.2, and Qt 6.11.1 MSVC 2022 64-bit.
- Vulkan GPU 0 is AMD Radeon 780M Graphics; GPU 1 is NVIDIA GeForce RTX 5060 Laptop GPU.
- Runtime commit `e179adeda` passed the full modular Debug and Release suites on the NVIDIA Vulkan ICD: 1,074 selected, 1,070 passed, 0 failed, and 4 expected Windows/platform skips in each configuration.
- Debug and Release NVIDIA NVENC spot checks passed; the AMD NVENC check produced the expected unsupported-device skip.
- The native Qt bridge builds at `build-msvc/qtbridge/<Configuration>/datoviz_qtbridge.dll`.
- Interactive Release `start/scatter` with `DVZ_PRESENT_MODE=fifo-latest` and `DVZ_FPS_CAP=60` had smooth physical pan/zoom behavior.
- The `dvztest_modules` aggregate target reduced the measured combined Debug and Release rebuild from 478.7 seconds to 260.5 seconds; an aggregate Debug no-op build took 14.3 seconds.
- Retained local logs, JSON reports, recordings, and the detailed summary are outside the repository under `Documents/Codex/2026-08-01/je-configure-ce-pc-windows-11/outputs/`. Do not commit those payloads.

## 1. Complete The Symmetric AMD/NVIDIA Campaign Now

This is the highest-value remaining physical-machine task. Follow [HANDOFF_GPU_SELECTION.md](HANDOFF_GPU_SELECTION.md#remaining-physical-windows-campaign) and preserve separate output roots for GPU 0 and GPU 1.

1. Record `--list-gpus` output and Windows, Vulkan loader, GPU, and driver versions.
2. Run identical Debug scopes on both indices for `vk`, `vklite`, `drp2`, `canvas`, `scene`, `app`, `gui`, `capture`, and applicable `video` cases.
3. Repeat the same scopes in Release without using `DVZ_LOG_LEVEL` to influence behavior.
4. Run `dvz_live_canvas` with both offscreen and GLFW backends on each index; a human must assess visible presentation and interaction.
5. Keep enumeration, production-default, invalid-index, CPU-only, CUDA/UUID, and encoder tests in a separate exemption set.
6. Compare exact totals, skips, exits, validation messages, presentation behavior, and report metadata between AMD and NVIDIA.

The runner interface is:

```powershell
& .\build-msvc\testing\Debug\dvztest.exe --list-gpus
& .\build-msvc\testing\Debug\dvztest.exe --module <module> --gpu 0 --json <amd-report.json>
& .\build-msvc\testing\Debug\dvztest.exe --module <module> --gpu 1 --json <nvidia-report.json>
& .\build-msvc\testing\Debug\dvz_live_canvas.exe --backend offscreen --gpu <index> --frames 300
& .\build-msvc\testing\Debug\dvz_live_canvas.exe --backend glfw --gpu <index> --frames 300
```

Use the corresponding `Release` executables for the Release pass. Do not treat `VK_LOADER_DRIVERS_SELECT` as the selector contract; it is only an optional cross-check.

## 2. Complete Local Qt/PyQt Source-Hosting Proof Now

Qt and the native bridge are installed, but this Python 3.12 environment still needs PyQt6 and a source-hosted smoke. This local proof validates the Windows development environment; it does not replace the externally blocked exact conda-provider campaign in [STATUS.md](STATUS.md).

1. Install PyQt6 into the intended Python 3.12 environment and record its version and Qt runtime version.
2. Point `DATOVIZ_QTBRIDGE_LIBRARY` at the bridge for the matching Debug or Release configuration when automatic discovery does not find it.
3. Run `python -m datoviz.qt` and `python examples/python/qt/hosted_pyqt.py --smoke-ms 1000`.
4. Run one visible hosted window and have the maintainer confirm rendering, resize, interaction, normal close, and reopen.
5. Preserve explicit negative diagnostics for a missing bridge, unavailable `QVulkanInstance`, and provider/runtime mismatch when those paths can be reproduced safely.

PowerShell template:

```powershell
$env:DATOVIZ_QTBRIDGE_LIBRARY = (Resolve-Path .\build-msvc\qtbridge\Release\datoviz_qtbridge.dll).Path
python -m datoviz.qt
python .\examples\python\qt\hosted_pyqt.py --smoke-ms 1000
```

## 3. Validate The Windows vcpkg Overlay Now

Use a clean vcpkg installation and the supported `x64-windows` triplet. First validate the checkout-backed pre-tag path, then build and run the CMake package consumer. Follow [../../vcpkg-overlay/README.md](../../vcpkg-overlay/README.md) and retain the exact vcpkg commit, triplet, compiler, command lines, and consumer result.

PowerShell template:

```powershell
$env:DATOVIZ_VCPKG_SOURCE_PATH = (Get-Location).Path
& <vcpkg.exe> install datoviz --overlay-ports="$PWD\vcpkg-overlay\ports" --triplet x64-windows
cmake -S .\examples\c\integration\cmake_package -B .\build\vcpkg-consumer -DCMAKE_TOOLCHAIN_FILE=<vcpkg-root>\scripts\buildsystems\vcpkg.cmake
cmake --build .\build\vcpkg-consumer --config Release
```

The exact release-source URL and SHA512 validation must wait until the accepted source bundle exists. Never substitute GitHub's automatically generated source archive.

## 4. Run Exact-Candidate Windows Proof Later

Start this only after the accepted RC source bundle and Windows wheel are fixed and identified. Follow the runnable procedure in [../../docs/contributors/release-physical-validation.md](../../docs/contributors/release-physical-validation.md).

1. Create a clean environment that does not import or load Datoviz from the source checkout.
2. Record release version, artifact commit, release commit, successful wheel workflow, wheel filename, and SHA256 checksum.
3. Run the installed-artifact validator, import/ABI checks, native dependency inventory, CMake consumer, and selected automated rendering profile.
4. On the same machine, perform the canonical live set: 2D pan/zoom, 3D arcball or fly controls, text/layout resize, image or color-scale interaction, textured mesh, picking/query/readback, close, and reopen.
5. Record each human observation as pass, fail, or skip with a reason; process exit status alone is not a visual pass.
6. Repeat affected evidence whenever the wheel checksum changes or a runtime-affecting commit invalidates the candidate.

Hosted Windows exact-artifact validation remains mandatory for RC3. Final v0.4.0 also requires physical Windows proof or an explicit maintainer-approved exception.

## Completion And Evidence Discipline

- Keep generated binaries, recordings, JSON reports, logs, source bundles, wheels, and `data` state out of ordinary code commits.
- Record exact commit, toolchain, OS, architecture, GPU/driver, commands, totals, skips, checksums, and artifact identities where applicable.
- Update this handoff and [STATUS.md](STATUS.md) when a lane completes or its blocker changes; move durable results into release evidence rather than growing an agent diary.
- A failure, crash, Vulkan validation message, wrong selected-device identity, or unexplained asymmetric result remains open until fixed or explicitly dispositioned.
