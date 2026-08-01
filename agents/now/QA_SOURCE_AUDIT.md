# Incremental C QA Handoff

Status: active incremental source audit completed through `shader`; persistent validation-off sanitizer configuration and the `input` module are next. Updated: 2026-08-02.

This handoff records the current static-analysis, sanitizer, lifetime, bounds, and corruption-prevention pass. It is development evidence for the RC3 release-quality lane, not final exact-artifact or platform-matrix proof.

## Operating Contract

- Work one active module at a time, confirm that it is built and used before investing in it, keep each slice small, and checkpoint only after focused tests pass.
- Keep Vulkan validation layers disabled during this pass because their known defects and leaks contaminate memory-tool results. Pass `-DDVZ_USE_VALIDATION=OFF` explicitly when configuring every normal or sanitizer build and verify the resulting `CMakeCache.txt`; do not infer the setting from an old build directory.
- Use normal tests first, then ASan/UBSan/LSan, TSan for Datoviz-owned concurrency where practical, and focused `clang-tidy`/`cppcheck`. Treat tool stalls, provider incompatibilities, suppressions, and unavailable coverage as limitations rather than passes.
- Do not claim a repository-wide clean result from per-module analysis. This campaign has deliberately favored narrow, actionable findings over a noisy whole-tree report.
- Keep changes to `external/` isolated in their own commits and document local patches in the vendored file header. Do not modify other vendored sources unless a finding cannot be fixed at a Datoviz-owned boundary.
- Preserve the repository staging rules, especially for `data`, generated bindings, runtime libraries, and unrelated concurrent work.

The local Linux environment currently has `cppcheck` and `clang-tidy`. Separate `build`, `build-asan`, and `build-tsan` trees were configured with validation disabled; future agents must recheck rather than rely on those local caches. CUDA was disabled in sanitizer builds. No additional tool installation is required for the immediate next slice.

## Completed Checkpoints

| Area | Result | Checkpoints |
| --- | --- | --- |
| Scene/window teardown | Fixed owned scene/runtime cleanup and a host-window leak found by sanitizer-guided inspection. This was a focused teardown slice, not a complete `scene` or `window` audit. | `1b96056e4` |
| Test runner | Fixed isolated-child failure propagation so crashes and sanitizer failures cannot be silently reported as successful tests; added scheduler regression coverage. | `faaa1dedf` |
| Vendored video helper | Fixed HEVC VPS accounting, lookup, and cleanup in `external/minimp4.h`. The external change is isolated and its header records the Datoviz patches that must survive a vendor refresh. | `e090875ac` |
| Module inventory | Confirmed the retired `ds` module is not linked into the active library and removed it from the active-module guidance. It was not audited as production code. | `ecb39a0b4` |
| Common logging | Replaced unsafe concurrent local-time conversion, then hardened logger synchronization and object-container access. Added concurrency and object lifetime tests. | `0a756e200`, `1071ca745` |
| File I/O | Hardened file/NPY size and shape validation, PNG/PPM cleanup and error handling, gzip failure paths, and byte-write handling. Added malformed-input and failure-path coverage. Public file-I/O headers and generated ctypes changed together and binding checks were run at those checkpoints. | `27ad849f6`, `9a1a16838`, `ec0c0e9ae` |
| Geometry | Confirmed `geom` is active and consumed by production paths. Added checked primitive count arithmetic, rejected malformed OBJ numeric records, and exercised the hashed polygon triangulation path. | `50c0b09e5`, `e112798da`, `dbe5736f7` |
| Thread wrappers | Fixed wrapper and atomic ownership/lifetime edges and added focused cleanup coverage. Datoviz-owned thread/common concurrency paths were exercised under TSan during the slice. | `189f1b955`, `1071ca745` |
| Common allocation | Added overflow-safe allocation and copy size handling with boundary tests. | `c86450dc3` |
| Runtime shader compiler | Confirmed `shader` is active through DRP2 pipeline creation and downstream vklite/scene runtime use. Hardened runtime-library path construction and result cleanup; corrected first-use concurrency coverage; added null, empty, malformed, default-entry-point, provider-missing, provider-incompatible, diagnostic ownership, and double-destroy cases. | `badf143a4` |

Focused normal tests, relevant sanitizer runs, per-file static analysis, `just build`, `just spec-check`, and `git diff --check` were used throughout the checkpoints where applicable. There is no retained machine-readable campaign report, so final RC3 evidence must rerun the frozen matrix from exact release artifacts.

## Latest Confirmed State

At `badf143a4` on the Linux NVIDIA RTX 5090 host:

- `just build` passed with Vulkan validation disabled.
- `just spec-check` passed, including 125/125 DRP2 fixtures and the source guards.
- Normal shader compilation, runtime-directory lookup, missing-provider, and incompatible-provider smoke cases passed.
- The same four shader adapter cases passed under ASan/UBSan/LSan without a report.
- The downstream normal DRP2 shader selection passed 6/6 tests.
- Focused `clang-tidy` and `cppcheck` on `src/shader/shader.c` produced no actionable project-source finding.
- `git diff --check` passed and the worktree was clean immediately after the shader checkpoint.

The normal shader build cache initially named stale `libshaderc.so.1`; the installed Vulkan SDK provides `libshaderc_shared.so.1`. The local cache was refreshed to the available basename and both direct and runtime-directory loading passed. This was a local configuration correction, not a committed source change.

## Known Limitations

- TSan shader smoke and downstream ASan DRP2 shader execution stall inside the dynamically loaded, non-instrumented shaderc provider. Bounded runs were terminated without a sanitizer or race report. These runs are inconclusive, not passes, and the first-use concurrency test must not be weakened to avoid the provider issue.
- Vulkan validation has intentionally not been exercised in this campaign. It remains a separate RC3 gate after known validation-layer defects and leaks are dispositioned or suppressed with evidence.
- No full-tree `just analyze`, complete `just test`, MSan, Valgrind, long-loop, installed-package, source-archive, wheel, hosted-platform, or physical-platform campaign has been claimed here.
- The existing `_sanitizer-build` recipe in `justfiles/build.just` does not explicitly pass `-DDVZ_USE_VALIDATION=OFF`. Current local caches are safe, but a fresh sanitizer tree can regress to the configured default.
- `scene` and `window` received a teardown fix only; they still need full module audits later. The Vulkan-facing foundation (`window`, `canvas`, `stream`, `video`, `vk`, `vklite`, `drp2`, `scene`, and `app`) should be approached only after the smaller CPU-oriented modules because ownership and provider noise require more careful matrices.

## Immediate Restart Sequence

1. Make validation exclusion durable by adding `-DDVZ_USE_VALIDATION=OFF` to the Linux `_sanitizer-build` configuration in `justfiles/build.just`; configure fresh or explicitly cleared ASan and TSan trees and verify both caches contain `DVZ_USE_VALIDATION:BOOL=OFF`. Keep this as a focused build-tool checkpoint.
2. Audit the active `input` module next. First map its production callers and test entry points, then inspect callback ownership, event queue/state bounds, key/button indexing, null payload handling, and teardown/re-registration behavior.
3. Run the narrow normal input tests, focused `clang-tidy` and `cppcheck`, then ASan/UBSan/LSan. Add TSan only if the inspected ownership contract permits concurrent access or the tests expose a meaningful concurrency path.
4. Commit only actionable fixes and focused regression tests. If `input` is clean, record the evidence here without manufacturing a code change.
5. Continue with `math`, then revisit the incompletely audited `window` boundary before moving through `canvas`, `stream`, `video`, `vk`, `vklite`, `drp2`, `scene`, and `app`. Reorder only when a finding or release blocker establishes a better dependency path.

For final RC3 release evidence, convert this exploratory sequence into a frozen matrix with exact commit/artifact identity, commands, tool versions, configurations, pass/fail/skip totals, timeouts, suppressions, provider versions, GPU/driver identity, and explicit exclusions.
