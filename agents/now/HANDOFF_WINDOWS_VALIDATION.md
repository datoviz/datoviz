# Windows Native Validation Handoff

Status: native development, automated AMD/NVIDIA source validation, and the Windows vcpkg consumer are green; the FIFO frame-slot comparison remains, the visible DRP2 live-canvas gate exposed an AMD/NVIDIA rendering asymmetry, and a Vulkan-enabled PyQt provider plus exact-candidate physical proof also remain. Updated: 2026-08-02.

## Scope

This is the operational handoff for the physical Windows 11 AMD/NVIDIA laptop. Use [HANDOFF_GPU_SELECTION.md](HANDOFF_GPU_SELECTION.md) for the GPU-selection contract, [DISTRIBUTION_RELEASE_CHECKLIST.md](DISTRIBUTION_RELEASE_CHECKLIST.md) for package gates, and [../../spec/release/PHYSICAL_VALIDATION.md](../../spec/release/PHYSICAL_VALIDATION.md) plus [../../docs/contributors/release-physical-validation.md](../../docs/contributors/release-physical-validation.md) for exact-candidate physical proof.

Source-checkout results are development evidence only. They must not be reported as installed-wheel or release-candidate evidence.

## Machine And Completed Baseline

- Windows 11 x64 with Visual Studio 2022, MSVC 14.44.35207, Windows SDK 10.0.26100.0, Vulkan SDK 1.4.350.0, CUDA Toolkit 13.2, Qt 6.11.1 MSVC 2022 64-bit, and Node.js LTS 24.18.1.
- Vulkan GPU 0 is AMD Radeon 780M Graphics; GPU 1 is NVIDIA GeForce RTX 5060 Laptop GPU.
- The isolated validation branch starts from `5393b56a8`; its retained campaign reports are source-checkout evidence and must be rerun when the runtime candidate changes.
- The final affected Debug and Release matrix passed `canvas`, `gui`, `vk`, `vklite`, and `video` on both GPUs with 0 failures and no Vulkan validation messages. Expected capability skips were four canvas, one vklite, and one NVENC case on AMD, and one GLFW lifecycle canvas case on NVIDIA.
- The broad NVIDIA Debug run passed 1,066 of 1,067 selected cases with one expected skip. The broad Release run initially exposed leaked logger state from the thread module; after making the test adapter restore its threshold per case, Release also passed 1,066 of 1,067 with one expected skip and no Vulkan validation messages.
- Debug and Release `dvz_live_canvas` offscreen scene smoke passed on both GPUs. In the scripted Release GLFW screenshot check, NVIDIA rendered one orange point but AMD rendered a blank black frame; `--scene-points 100000` was parsed and reported by the benchmark but the NVIDIA screenshot still contained only one point. Both processes closed normally and reported no Vulkan validation error. This visible gate is open and must not be reported as passed.
- The full spec-check suite passes on Windows after installing its `jsonschema` dependency and fixing fixture-tool handling of Windows file URIs and portable JSON path separators.
- Runtime commit `e179adeda` passed the full modular Debug and Release suites on the NVIDIA Vulkan ICD: 1,074 selected, 1,070 passed, 0 failed, and 4 expected Windows/platform skips in each configuration.
- Debug and Release NVIDIA NVENC spot checks passed; the AMD NVENC check produced the expected unsupported-device skip.
- The native Qt bridge builds at `build-msvc/qtbridge/<Configuration>/datoviz_qtbridge.dll`.
- Interactive Release `start/scatter` with `DVZ_PRESENT_MODE=fifo-latest` and `DVZ_FPS_CAP=60` had smooth physical pan/zoom behavior.
- The `dvztest_modules` aggregate target reduced the measured combined Debug and Release rebuild from 478.7 seconds to 260.5 seconds; an aggregate Debug no-op build took 14.3 seconds.
- Retained local logs, JSON reports, recordings, and the detailed summary are outside the repository under `Documents/Codex/2026-08-01/je-configure-ce-pc-windows-11/outputs/`. Do not commit those payloads.

## 1. Resolve And Repeat The Visible AMD/NVIDIA Checks

The automated symmetric source campaign is complete at the branch point recorded above, but the GLFW scene result is asymmetric. Follow [HANDOFF_GPU_SELECTION.md](HANDOFF_GPU_SELECTION.md#remaining-physical-windows-campaign) and preserve separate output roots for GPU 0 and GPU 1 when repeating it for another candidate.

1. Determine why the DRP2 live canvas is blank on AMD and why the requested 100,000-point grid appears as a single point on NVIDIA.
2. Repeat the Release GLFW live canvas on GPU 0 and GPU 1 after the affected runtime changes land.
3. On each window, confirm visible rendering, resize behavior, normal close, and reopen; record pass, fail, or skip with a reason.
4. Keep the observed AMD/NVIDIA capability skips separate from unexplained asymmetry: AMD has no NVENC and lacks the tested external-memory interop path, while the NVIDIA GLFW lifecycle case has an expected platform skip.

The runner interface is:

```powershell
& .\build-msvc\testing\Debug\dvztest.exe --list-gpus
& .\build-msvc\testing\Debug\dvztest.exe --module <module> --gpu 0 --json <amd-report.json>
& .\build-msvc\testing\Debug\dvztest.exe --module <module> --gpu 1 --json <nvidia-report.json>
& .\build-msvc\testing\Debug\dvz_live_canvas.exe --backend offscreen --gpu <index> --frames 300
& .\build-msvc\testing\Debug\dvz_live_canvas.exe --backend glfw --gpu <index> --frames 300
```

Use the corresponding `Release` executables for the Release pass. Do not treat `VK_LOADER_DRIVERS_SELECT` as the selector contract; it is only an optional cross-check.

## 2. Run The FIFO Frame-Slot Comparison Now

This campaign closes the Windows evidence gap in [HANDOFF_FRAME_DEMAND.md](HANDOFF_FRAME_DEMAND.md#next-decision). It measures ordinary FIFO with one, two, and automatic/current frame-slot counts; it does not test or select `fifo-latest`, `mailbox`, or `immediate`, and it does not authorize a default-policy change.

### Preconditions

1. Use the physical Windows 11 laptop on AC power with a fixed power profile, display, resolution, refresh rate, and variable-refresh setting for the entire campaign. Close unrelated GPU-heavy and CPU-heavy applications and do not interact with the benchmark windows.
2. Use a Visual Studio 2022 Developer PowerShell with Git, Python 3, CMake, Ninja, the Vulkan SDK, and the normal Datoviz source-build dependencies on `PATH`.
3. Enable Windows Developer Mode or run the shell with permission to create directory symbolic links. The comparison runner creates temporary Git worktrees and links their `data` directories to the clean primary `data` checkout.
4. Pull the latest `v0.4-dev`, initialize submodules, and require a clean primary worktree and clean `data` submodule. Do not stage or commit `data` or generated outputs.
5. Use reference `c49e98e1a` and a candidate containing `44307644a`; record the candidate's full commit ID. Do not silently substitute another reference. If the intended candidate has runtime changes after `44307644a`, use that exact candidate and identify it in the report.
6. Record Windows build, GPU driver, Vulkan loader/SDK, display resolution, refresh rate, variable-refresh setting, power mode, and `vulkaninfo --summary`. The interaction workload currently uses the application's production-default Vulkan device, so record its identity and keep it unchanged for all three comparisons; do not claim symmetric AMD/NVIDIA benchmark evidence from this campaign.

Preparation and focused validation:

```powershell
git status --short
git -C .\data status --short
git pull --rebase
git submodule update --init --recursive
git rev-parse HEAD
& .\build-msvc\testing\Release\dvztest.exe --list-gpus
cmake --build .\build-msvc --config Release --target dvztest_modules
& .\build-msvc\testing\Release\dvztest.exe --module canvas --gpu 0 --json "$HOME\Documents\Codex\frame-slots-canvas-amd.json"
& .\build-msvc\testing\Release\dvztest.exe --module canvas --gpu 1 --json "$HOME\Documents\Codex\frame-slots-canvas-nvidia.json"
```

Both Canvas runs must finish without failures, crashes, Vulkan validation errors, or unexpected skips. At minimum, verify that `test_canvas_frame_slot_count_resolution`, `test_canvas_glfw_one_frame_slot`, and `test_canvas_glfw_two_frame_slots` ran and passed on each usable GPU. If a GPU cannot present to GLFW, record the exact skip or failure instead of weakening the test scope.

Perform a visible Release smoke for each configuration before measuring. For each window, pan and zoom continuously for at least 30 seconds, resize repeatedly, release input and confirm return to idle, then close normally. Record responsiveness, visible stutter, corruption, hangs, validation messages, and close behavior. Do not set an FPS cap.

```powershell
$scatter = (Resolve-Path .\build-msvc\examples\c\start\Release\scatter.exe).Path
$env:DVZ_PRESENT_MODE = "fifo"
Remove-Item Env:DVZ_FPS_CAP -ErrorAction SilentlyContinue

$env:DVZ_MAX_FRAMES_IN_FLIGHT = "1"
& $scatter

$env:DVZ_MAX_FRAMES_IN_FLIGHT = "2"
& $scatter

Remove-Item Env:DVZ_MAX_FRAMES_IN_FLIGHT -ErrorAction SilentlyContinue
& $scatter
Remove-Item Env:DVZ_PRESENT_MODE -ErrorAction SilentlyContinue
```

### Paired measurements

Run all three commands from the repository root in the same shell and session. The Python command is intentional because the `just compare-interaction` recipe is Linux-gated. Five paired runs and 300 frames are the minimum development comparison; the runner randomizes base/candidate order within each pair and forces ordinary FIFO.

```powershell
$reference = "c49e98e1a"
$candidate = (git rev-parse HEAD).Trim()
$stamp = Get-Date -Format "yyyyMMdd-HHmmss"
$outputRoot = Join-Path $HOME "Documents\Codex\frame-slot-comparison-$stamp"
New-Item -ItemType Directory -Force -Path $outputRoot | Out-Null

$env:DVZ_MAX_FRAMES_IN_FLIGHT = "1"
python .\tools\compare_present_benchmarks.py $reference $candidate --workload scatter-interaction --runs 5 --frames 300 --output "$outputRoot\slots-1"

$env:DVZ_MAX_FRAMES_IN_FLIGHT = "2"
python .\tools\compare_present_benchmarks.py $reference $candidate --workload scatter-interaction --runs 5 --frames 300 --output "$outputRoot\slots-2"

Remove-Item Env:DVZ_MAX_FRAMES_IN_FLIGHT -ErrorAction SilentlyContinue
python .\tools\compare_present_benchmarks.py $reference $candidate --workload scatter-interaction --runs 5 --frames 300 --output "$outputRoot\slots-auto"
```

If configuration needs machine-specific CMake cache values, pass each one as `--cmake-arg "-DNAME=VALUE"` identically to all three commands and record it. Do not modify the benchmark, change presentation mode, add a frame cap, filter the Vulkan loader differently between runs, or rerun only unfavorable pairs. A failed comparison leaves `report.json` with `status: failed`; diagnose and rerun the entire affected slot configuration after the cause is fixed.

The baseline predates frame-slot configuration telemetry and ignores `DVZ_MAX_FRAMES_IN_FLIGHT`, so `present_configurations.scatter-interaction.verified` may be `false` for the historical baseline. That is expected only when the base configuration is absent and the candidate configuration reports `resolved_present_mode: fifo` with the requested `slot_count`; any known mismatch or wrong candidate configuration is a failed campaign.

### Required report

Return one concise Markdown summary plus the three unmodified `report.md` and `report.json` pairs. Keep raw logs and generated builds outside Git. The summary must contain:

1. exact reference and candidate commit IDs;
2. machine, Windows build, CPU, production-default Vulkan device and driver, Vulkan loader/SDK, display resolution and refresh rate, variable-refresh state, and power mode;
3. exact commands, environment variables, extra CMake arguments, start/end time, and any deviations;
4. focused Canvas test totals, skips, failures, and validation-message count for GPU 0 and GPU 1;
5. visible smoke observations for one, two, and auto slots, including interaction, resize, idle return, and close;
6. for one, two, and auto slots: requested and resolved present mode, image count, candidate slot count, configuration-verification state, base and candidate median p95 input-to-submit latency, paired median delta, 95% confidence interval, threshold, and verdict;
7. median p95 input-to-render-start, slot-wait, and acquire-wait values from the raw run metrics for each configuration;
8. every crash, warning, validation message, presentation anomaly, thermal/power disturbance, retry, and discarded run, with its disposition;
9. a recommendation of `one`, `two`, `auto`, or `inconclusive`, explicitly compared with the Linux result of -43.17% for one slot, -28.59% for two slots, and +0.06% for auto.

### Disposition

Do not change code or defaults solely because a benchmark verdict says `improvement`. First update the Windows results table in [HANDOFF_FRAME_DEMAND.md](HANDOFF_FRAME_DEMAND.md), update the lane state in [STATUS.md](STATUS.md), and commit only those concise documentation changes; do not commit raw reports, logs, recordings, generated builds, or `data` state. Then report the evidence to the maintainer and request the separate policy decision.

Recommend one slot only if the one-slot candidate passes validation and physical behavior, materially improves or does not regress Windows p95 input-to-submit latency, and is consistent with the Linux direction. Recommend two slots if one slot fails correctness or materially regresses Windows while two slots passes and improves. Recommend auto if both bounded settings fail correctness or materially regress. Report `inconclusive` if configuration identity, display stability, sample integrity, or confidence intervals do not support a decision. My preferred policy, if Windows confirms Linux, is one FIFO frame slot by default with `DVZ_MAX_FRAMES_IN_FLIGHT` retained as an override; present-mode policy remains a separate decision.

## 3. Resolve The Local Qt/PyQt Provider Blocker

Qt 6.11.1, the native Release bridge, and PyQt6 6.11.0 are installed. The bridge loads and reports ABI 1 with Qt 6.11.1, but the PyPI Windows PyQt6 package does not expose `QVulkanInstance`; `python -m datoviz.qt` therefore stops with the intended Vulkan-support diagnostic. This is a provider limitation, not a Datoviz rendering failure, and it does not replace the externally blocked exact conda-provider campaign in [STATUS.md](STATUS.md).

1. Obtain a Windows PyQt6 build that exposes `QVulkanInstance` and matches Qt 6.11.1, or validate an explicitly supported alternative provider.
2. Run `python -m datoviz.qt` and `python examples/python/qt/hosted_pyqt.py --smoke-ms 1000` with `DATOVIZ_QTBRIDGE_LIBRARY` pointing at the matching bridge.
3. Run one visible hosted window and have the maintainer confirm rendering, resize, interaction, normal close, and reopen.

PowerShell template:

```powershell
$env:DATOVIZ_QTBRIDGE_LIBRARY = (Resolve-Path .\build-msvc\qtbridge\Release\datoviz_qtbridge.dll).Path
python -m datoviz.qt
python .\examples\python\qt\hosted_pyqt.py --smoke-ms 1000
```

## 4. Windows vcpkg Overlay Result

The checkout-backed `x64-windows` overlay was built from source with vcpkg commit `39344dff01c5a5a0134caf2624cdd492f05d30ea` and MSVC 14.44.35207. Debug and Release standalone CMake consumers both built and ran, reporting Datoviz 0.4.0rc2. The shared-package fixes keep the internal image helper within each DLL layer and install MSDF runtime DLLs under `bin` and `debug/bin`.

For checkout-backed validation, disable binary-cache restoration because the ABI key does not include file contents behind `DATOVIZ_VCPKG_SOURCE_PATH`. Follow [../../vcpkg-overlay/README.md](../../vcpkg-overlay/README.md) and retain the exact vcpkg commit, triplet, compiler, command lines, and consumer result.

PowerShell template:

```powershell
$env:DATOVIZ_VCPKG_SOURCE_PATH = (Get-Location).Path
$env:VCPKG_KEEP_ENV_VARS = "DATOVIZ_VCPKG_SOURCE_PATH"
& <vcpkg.exe> install datoviz:x64-windows --classic --overlay-ports="$PWD\vcpkg-overlay\ports" --triplet x64-windows --binarysource=clear
cmake -S .\examples\c\integration\cmake_package -B .\build\vcpkg-consumer -DCMAKE_TOOLCHAIN_FILE=<vcpkg-root>\scripts\buildsystems\vcpkg.cmake
cmake --build .\build\vcpkg-consumer --config Release
```

The exact release-source URL and SHA512 validation must wait until the accepted source bundle exists. Never substitute GitHub's automatically generated source archive.

## 5. Run Exact-Candidate Windows Proof Later

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
