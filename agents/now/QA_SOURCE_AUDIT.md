# RC3 Exploratory Source Audit Record

Status: completed development-evidence record at validated implementation head `545c99379`, integrated into `v0.4-dev` by `2d83d0b63`. Updated: 2026-08-03.

This file retains the actionable results, exact-head evidence, and limitations of the completed exploratory C source audit. The former autonomous module queue and two-lane orchestration instructions are obsolete and remain available in Git history. This record is development evidence, not exact release-artifact or platform-matrix proof.

## Completed Checkpoints

| Area | Result | Checkpoints |
| --- | --- | --- |
| Scene/window teardown | Fixed owned scene/runtime cleanup and a host-window leak found by sanitizer-guided inspection. | `1b96056e4` |
| Test runner | Fixed isolated-child failure propagation so crashes and sanitizer failures cannot be reported as successful tests; added scheduler regression coverage. | `faaa1dedf` |
| Vendored video helper | Fixed HEVC VPS accounting, lookup, and cleanup in `external/minimp4.h`; the isolated header records the Datoviz patches that must survive a vendor refresh. | `e090875ac` |
| Module inventory | Confirmed the retired `ds` module is not linked into the active library and removed it from active-module guidance. | `ecb39a0b4` |
| Common logging | Replaced unsafe concurrent local-time conversion and hardened logger synchronization and object-container access. | `0a756e200`, `1071ca745` |
| File I/O | Hardened file/NPY size and shape validation, PNG/PPM cleanup and error handling, gzip failure paths, and byte-write handling with malformed-input and failure-path coverage. | `27ad849f6`, `9a1a16838`, `ec0c0e9ae` |
| Geometry | Added checked primitive count arithmetic, rejected malformed OBJ numeric records, and exercised hashed polygon triangulation. | `50c0b09e5`, `e112798da`, `dbe5736f7` |
| Thread wrappers | Fixed wrapper and atomic ownership/lifetime edges with focused cleanup and Datoviz-owned TSan coverage. | `189f1b955`, `1071ca745` |
| Common allocation | Added overflow-safe allocation and copy-size handling with boundary tests. | `c86450dc3` |
| Runtime shader compiler | Hardened runtime-library path construction and cleanup; added first-use concurrency, invalid-input, provider, diagnostic-ownership, and double-destroy coverage. | `badf143a4` |
| Sanitizer configuration | Made validation-layer exclusion durable in Linux ASan, MSan, and TSan recipes and verified the resolved caches. | `a5a3073ec` |
| Input | Hardened subscription growth, removal during dispatch, callback-ID wrap, allocation failure, duplicate presses, and backwards click timestamps. | `e710ea15f` |
| Math statistics | Removed duplicate exported statistics implementations while preserving the documented zero-count range contract and adding focused coverage. | `c157b2cfb` |
| Math boxes | Added the missing output assertion and regression coverage for ordinary, empty, nonpositive, merge, normalization, and inverse cases. | `c0005f0f6` |
| Math easing | Corrected `DVZ_EASING_IN_SINE`, which had duplicated out-sine, and added distinguishing midpoint coverage. | `4b7250561` |
| DRP2 and vklite bounds | Corrected render-pass resolve JSON to the active flat schema, made target zero disable resolve, and made graphics/compute specialization bounds overflow-safe. | `a9d3e29b0` |
| Stream and video ownership | Rolled back partially created sinks, preserved imported semaphore-FD ownership, propagated sink-stop failures, and latched encoder write/mux/flush failures. | `1ff4bc01f`, `26ac18493` |
| Window registry lifetime | Replaced realloc-sensitive backend-slot pointers in live windows with stable indices and covered registry growth. | `5e9b5d8b6` |
| Canvas acquired-frame recovery | Added safe abandonment and swapchain recreation after post-acquire preparation failures, including stream start/update recovery. | `37cb1e216` |
| App draw recovery | Made attachment, emission, and artifact failures observable to `dvz_view_render_once()` while preserving retry demand and successful-frame accounting. | `9a3b0af79` |

## Final Integrated Evidence

At exact validated implementation head `545c99379` on Linux with NVIDIA GeForce RTX 5090, Vulkan 1.4.328, clang-tidy 18.1.3, cppcheck 2.13.0, CMake 3.28.3, and Ninja 1.11.1:

- `just build` and validation-enabled `just test` passed 1,128/1,128 tests with no failures or skips: 727 scene, 127 DRP2, 47 vklite, 42 canvas, 41 app, 30 vk, 24 window, seven stream, and five video cases.
- `just test-drp2-contract` passed 95/95, `just drp2-fixtures` passed 125/125, `just test-runtime-vklite` passed 100/100, `just test-slow` passed 34/34, and `just spec-check` passed all fixture, scheduler, query, architecture, shader ABI, and visual-boundary guards.
- `just present-check --frames 120` passed all seven bounded presentation scenarios with zero reported stutters and zero steady swapchain recreations.
- `just webgpu-check` passed build, data, fixture, preflight, scenario, and runner gates with the approved choropleth data; live browser cases were skipped because the headless host loses its WebGPU instance.
- `just ctypes-check` and `just ctypes-smoke` passed generation freshness, 16 generator tests, policy, facade, ABI validation for 201 records, and raw smoke. Example manifests, generated docs, docs build/status/snippets, How-To snippets, and the three-step Vulkan course check/smoke passed.
- Full-tree `just analyze` completed with 1,709 existing advisory diagnostics dominated by padding and insecure-API portability checks. Full-tree cppcheck completed with its known incomplete-configuration parser error and false constant-condition results; no new actionable touched-path defect was identified.
- The ASan/UBSan/LSan build resolved `DVZ_USE_VALIDATION:BOOL=OFF`. The complete Window CPU module passed 24/24 and focused registry-growth, DRP2 resolve, Stream stop, and Video output-error tests passed without a sanitizer report.
- `git diff --check` passed, the evidence worktree was clean before its final documentation update, and no protected `data` or generated binary payload was added by the QA commits.

## Known Limitations

- Vulkan-backed Canvas and specialization-constant sanitizer children stalled during non-instrumented provider/driver teardown without a sanitizer report. They are inconclusive; their validation-enabled normal counterparts pass.
- TSan shader smoke and downstream ASan DRP2 shader execution stall inside the dynamically loaded, non-instrumented shaderc provider. Bounded runs were terminated without a sanitizer or race report and remain inconclusive.
- Full-tree clang-tidy and cppcheck results are dispositioned advisory/configuration output, not a zero-diagnostic claim.
- Input TSan was not applicable because the public router contract is synchronous and the production caller audit found no supported Datoviz-owned concurrent access path.
- MSan, Valgrind, exact installed packages, immutable source archives, wheels, hosted platforms, and new physical-platform proof remain outside this exploratory campaign.
- Physical Windows, macOS, AMD, and Intel coverage remains governed by the RC3 platform matrix; this campaign's exact-head hardware evidence is Linux/NVIDIA only.

## Release Carry-Forward

Do not restart the exploratory source audit for RC3. Freeze one exact candidate and carry this evidence forward only where the candidate ancestry and affected-path review permit it. Final release evidence must identify the exact commit and artifacts, commands, toolchain and provider versions, configurations, totals, timeouts, checksums, GPU/driver facts, and explicit exclusions.
