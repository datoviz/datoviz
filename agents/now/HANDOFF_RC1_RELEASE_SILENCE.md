# RC1 Release Validation And Logging Handoff

Status: closed by `e5bb34631`; replacement wheel and seven-machine conformance evidence passed.
Created: 2026-07-18. Updated: 2026-07-18.

The maintainer wants final C and Python release packages to be silent by default and not request
Vulkan validation layers implicitly. Resolve this before uploading RC1 wheels to TestPyPI.


## Confirmed Diagnosis

The intended Release behavior and actual implementation disagree.

1. `src/CMakeLists.txt` defines `ENABLE_VALIDATION_LAYERS=0` for Release configurations, but the
   Vulkan implementation does not reference that macro.
2. `dvz_gpu_ctx_config()` in `src/vk/gpu_ctx.c` initializes `.enable_validation = true` in every
   build. The default context therefore requests `VK_LAYER_KHRONOS_validation` whenever that layer
   is installed on an end-user machine. When it is absent, `src/vk/instance.c` can log
   `validation layer is not supported`.
3. CMake defines `DEBUG=0` for Release, but `src/common/_log.h` uses `#ifdef DEBUG` to select the
   default logger level. Because `DEBUG` is defined even when its value is zero, Release currently
   selects `LOG_INFO` instead of the intended silent threshold.
4. `src/common/log.c` initializes the environment-selected level through a GCC/Clang constructor.
   That is not a portable initialization route for the MSVC-built Windows DLL. Its zero-initialized
   logger state can expose trace-level logging before any explicit initialization.
5. Datoviz logs go to `stderr`, not `stdout`, but this still pollutes terminals and notebooks for
   both C and Python users. Python currently adds no wrapper-level suppression.

Relevant files:

- `src/CMakeLists.txt`
- `src/common/_log.h`
- `src/common/log.c`
- `src/vk/gpu_ctx.c`
- `src/vk/instance.c`
- `src/vk/validation.h`
- `tools/datoviz_build_backend/validate.py`


## Required Release Contract

1. A default Release-wheel import and basic scene lifecycle must produce no stdout or stderr.
2. Default Release GPU/app creation must not request the Khronos validation layer or install a
   Vulkan debug messenger.
3. Developers must retain an explicit validation opt-in through the existing GPU-context
   configuration where validation support is available.
4. `DVZ_LOG_LEVEL` must remain the explicit logging opt-in and work consistently on macOS, Linux,
   and Windows.
5. Debug and validation-oriented source builds must retain useful diagnostics.
6. Errors must continue to propagate through return values and the public error callback; terminal
   silence must not hide API failure state.

Preferred narrow implementation:

- make `dvz_gpu_ctx_config()` derive its default `enable_validation` value from
  `ENABLE_VALIDATION_LAYERS`, while leaving `dvz_gpu_ctx_config_validation(..., true)` as an
  explicit opt-in;
- replace value-insensitive `#ifdef DEBUG` logic with `#if DEBUG`;
- give the logger state a correct compile-time default instead of relying on zero initialization;
- add a portable, one-time environment initialization path rather than depending solely on a
  GCC/Clang constructor;
- do not introduce a Python-only workaround or globally redirect process stderr.

Check whether the current CMake wording is accurate while implementing this. It says Release
disables validation "unless explicitly ON", but the generator expression currently evaluates to
zero for every Release configuration regardless of `DVZ_USE_VALIDATION`.


## Regression Proof

Add focused tests that cover at least:

1. Release compile definitions select a silent default logger threshold.
2. The default Release `DvzGpuCtxConfig` has validation disabled.
3. Explicit GPU-context validation opt-in remains represented in instance flags.
4. A Release installed-wheel smoke creates and destroys a basic scene while capturing both streams
   and requires empty stdout/stderr under the default environment.
5. The same installed package honors an explicit `DVZ_LOG_LEVEL=info` opt-in.
6. The installed native version does not contain `(DEBUG)`; retain the existing check in
   `tools/datoviz_build_backend/validate.py`.
7. Exercise the environment initialization on Windows, not only Unix constructor builds.

Avoid requiring a validation layer in ordinary end-user wheel tests. A dedicated developer/CI lane
may install `VK_LAYER_KHRONOS_validation` to prove explicit opt-in separately.


## Validation Sequence

Use the narrow loop while iterating, then run:

```sh
just build
just test
just spec-check
just ctypes-check
git diff --check
```

Build a local Release wheel and run the installed-package smoke with stdout and stderr captured.
On this MacBook M3, also test once with any locally installed validation layer discoverable, because
clean hosted environments may not expose the implicit-validation failure mode.

Before committing, follow repository hygiene: inspect `git status --short` and
`git diff --cached --stat`; do not stage `paper/paper.pdf`, `data`, generated libraries, wheels, or
runtime payloads.


## Release Consequence

This changes native wheel bytes. Do not reuse Wheels run `29641789685` as final RC1 payload evidence
after the fix.

After committing and pushing:

1. dispatch a new six-platform Wheels run;
2. run the hosted six-lane conformance campaign on that exact run;
3. download the new macOS arm64 wheel and repeat unattended conformance plus the terminal-silence
   checks on the MacBook M3;
4. present the seven attended checks in one uninterrupted sequence and record the maintainer's
   consolidated result;
5. submit and sync the new physical evidence;
6. regenerate the consolidated report, RC source bundle, release state, and checksums;
7. only then resume the TestPyPI upload and package-index verification workflow.

The package-index workflow work may proceed independently, but it must be dispatched against the
new canonical wheel run rather than `29641789685`.


## Local Resolution Evidence

Commit `e5bb34631` implements the narrow native fix and regression coverage:

- Release builds select a silent logger threshold with `#if DEBUG` and a compile-time state
  initializer;
- macOS/Linux `pthread_once` and Windows `InitOnceExecuteOnce` apply `DVZ_LOG_LEVEL` portably;
- default GPU-context validation follows `ENABLE_VALIDATION_LAYERS`, while the existing explicit
  configuration opt-in remains active;
- installed Release-wheel validation captures both streams around a basic scene lifecycle, proves
  default silence, and proves `DVZ_LOG_LEVEL=info` output;
- focused Python validation passed 12 tests;
- the full M3 native suite passed 1035/1045 with 10 capability skips and no failures;
- a locally built macOS arm64 Release wheel installed and passed the new Release smoke;
- `just spec-check`, `just ctypes-check`, and `git diff --check` passed.

This closes the source-level blocker. Cross-platform MSVC behavior and replacement artifact bytes
are covered by the final evidence below.


## Final Replacement Evidence

- `Wheels` run `29644925786` built exact commit
  `ea06c5cdf0e7a267341b5834419d7854959399dd`; all 29 build and installed-wheel jobs passed.
- Hosted conformance run `29645577693` passed Linux x86_64/aarch64, Windows AMD64/ARM64, macOS
  Intel without a GPU, macOS arm64 with MoltenVK, and the aggregate report gate.
- The physical MacBook M3 tested the exact arm64 wheel
  `datoviz-0.4.0rc1-py3-none-macosx_15_0_arm64.whl`, SHA-256
  `21c1f68e852d92c7a8134867c5f5455442a37f73dfb57b8b40752fce871a26e2`.
- Both physical unattended profiles passed, including deterministic captures, cross-frontend
  decoded-pixel parity, installed C/Python examples, shaderc, and the CMake consumer.
- The maintainer approved all seven attended scenarios: pan/zoom, 3D arcball, text resize, image
  probe, textured mesh, picking, and close/reopen.
- Physical evidence intake run `29645582130` accepted the immutable bundle. The synced report at
  `build/physical-evidence/report/index.html` contains six hosted rows plus the physical M3 row,
  has no missing machines, and passes every release gate.

The Release-silence issue no longer blocks TestPyPI. Preserve the exact wheel checksums through the
package-index verification workflow.
