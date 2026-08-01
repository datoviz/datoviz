# Build, Test, And Module Rules

These rules cover the build system, include boundaries, module activation, and validation loops.


## Active Modules

Active modules currently linked into `libdatoviz` by default are:

`common`, `ds`, `fileio`, `geom`, `math`, `thread`, `shader`, `input`, `window`, `canvas`, `stream`, `video`, `vk`, `vklite`, `drp2`, `scene`, and `app`.

Modules such as `color`, `wasm`, richer text/gui layers, and broader renderer/client layers remain
scaffolding. Keep them untouched unless a task explicitly activates them.


## Build Commands

Run from the repository root:

```sh
just clean
just build
just test [filter]
```

Use `just build` for normal compilation and `just test [filter]` for the repo test workflow.

For tests depending on Vulkan, GLFW, Metal, swapchains, or graphics presentation on macOS, use the
repo environment:

```sh
direnv exec . just test [filter]
```


## Python Binding Freshness

Datoviz's generated Python binding files are local artifacts. Before running Python-facing
validation, GSP integration checks, packaging smoke tests, or release checks after public API work,
make the local binding match the headers:

```sh
just ctypes
just ctypes-check
```

Run this whenever a task touches:

- `include/datoviz/**`
- exported `dvz_*` function signatures or public structs/enums
- `spec/bindings/**`
- `tools/bindings/**`
- generated facade behavior or Python packaging paths that import `datoviz._ctypes`

`just ctypes-check` includes the binding freshness check, NumPy-adaptation check, binding policy
validation, and C ABI layout validation. If it fails, regenerate with `just ctypes`, fix the
header/policy/generator mismatch, and rerun the check before reporting success.


## Validation Strategy

Prefer the narrowest relevant validation loop while iterating:

1. Use `just test <module-or-filter>` for focused module work.
2. Use focused binaries such as `dvztest_drp2`, `dvztest_scene`, `dvztest_vk`,
   `dvztest_canvas`, or `dvztest_integration` when available.
3. Use `just ctypes-check` after public API/header/binding changes, especially before Python or
   GSP integration validation.
4. Use `just test` or `dvztest` for broad validation.
5. Run `git diff --check` before finalizing code changes.

For nontrivial C changes touching allocation, byte sizes, pointer lifetimes, object tables, Vulkan
resources, command buffers, frame lifetimes, or synchronization, consider static or dynamic
analysis when practical.

Useful tools include `clang-tidy`, `scan-build`, `cppcheck`, ASan/UBSan builds, Valgrind for
CPU-only paths, Vulkan validation layers, and representative live smoke loops such as
`dvz_live_canvas --frames 300`. If a tool is unavailable, too noisy, or impractical, report that
and fall back to focused tests.


## Gallery Media

Static gallery screenshots are captured as PNGs under `data/gallery/v0.4/` and converted to
build-local WebP derivatives under `build/gallery-webp/v0.4/`. Do not stage or commit `data`
submodule media or generated WebP files unless the user explicitly approves those exact payloads in
the current turn.

Use `just gallery-refresh` for a full local media/docs refresh. It must capture PNG screenshots,
convert static WebPs, regenerate animated WebPs, rebuild generated gallery docs/manifests, run the
gallery media checks, and run `git diff --check`. Use `just check-gallery-media-pipeline` after
changing gallery media tooling or manifest preview metadata.

When canonical PNGs exist but the local screenshot cache is absent, use `python3 tools/capture_gallery.py --all-screenshot --cache --verify-existing --jobs auto`. Verification captures into an isolated temporary tree, leaves `data` untouched, and writes a local cache record only when the normalized recapture is byte-identical or differs by no more than eight channel levels across at most 0.1% of RGBA components. Larger differences remain failures requiring explicit review and approval before any `data` change.

Canonical v0.4 native gallery PNGs use the designated physical Linux reference host defined in `spec/release/GALLERY_REFERENCE_SCREENSHOTS.md`. Use `just gallery-reference-candidates` to produce two isolated build-local capture sets, provenance, enhanced differences, and a review index. The two Linux runs must be byte-identical before promotion. Cross-platform or cross-driver pixel equivalence is comparison evidence, not canonical-origin proof.

Animated gallery captures, animated WebP previews, MP4 cards, and posters use canonical `1280x720` frames end to end. The media comparison pipeline consumes the canonical frame cache directly and must not add a per-frame resize stage.

Animated gallery previews use manifest metadata:

```yaml
media:
  preview:
    kind: animated-webp
    frames: 60
    fps: 30
```

MP4 gallery cards preserve their explicit native capture rate and are never upsampled. A capture at 60 fps first encodes at 60 fps with the configured base CRF; if it exceeds the 1 MB budget, the pipeline samples consistently to 30 fps while preserving duration. At 30 fps or a lower native rate, an oversized result advances through a bounded CRF ladder in increments of four through CRF 40. The command fails if the CRF 40 result remains oversized; it never silently reduces spatial resolution. Keep `preview.fps` equal to `card.fps * card.sample_step` so encoding never changes the preview duration.

`tools/compare_gallery_media.py` accepts `--jobs N` for independent encoding work and `--capture-jobs N` for independent captures. Automatic worker counts are CPU-bounded and capped at four; capture defaults to one worker on macOS. Use `--jobs 1 --capture-jobs 1` for explicit serial execution. Attempts for one MP4 remain sequential, each example uses an isolated temporary workspace, and reports retain manifest order regardless of worker completion order.

Gallery card and poster encodes use content-addressed cache records under `build/gallery-cache/cards/`. Cache keys cover canonical frame content, the encoding profile, generated variants, implementation inputs, and encoder identities; cache hits also verify every output hash. Use `--force` only when intentionally rebuilding verified current outputs.

Generate selected previews with:

```sh
python3 tools/build_gallery_animations.py --id <example_id> --force
```

The static WebP converter must not overwrite examples with `kind: animated-webp`; those WebP paths
are owned by `tools/build_gallery_animations.py`. The animation tool captures a deterministic PNG
sequence by launching the example with `--preview`, `--preview-sequence`, `--preview-frames M`, and
`--png`, then encodes the PNG sequence with `img2webp`. For animated examples whose motion depends
on elapsed simulation state rather than a controller preview descriptor, add an explicit
preview-mode path that derives deterministic state from `ctx->preview_frame_index` and
`ctx->preview_frame_count`; otherwise every captured frame may start from the same initial state and
produce a static animated WebP.


## CMake Pattern

Modules build as object libraries. New modules should follow the existing pattern:

```cmake
file(GLOB MODULE_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.c*")
add_library(datoviz_<module> OBJECT ${MODULE_SRC})

target_include_directories(datoviz_<module>
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/src/common
)

target_compile_definitions(datoviz_<module> PUBLIC ${DVZ_COMPILE_DEFINITIONS})
```

Add the module with `add_subdirectory(<module>)` and link the object library from
`src/CMakeLists.txt` only when the module is active.

Use `${DVZ_COMPILE_DEFINITIONS}` for module target definitions. Do not copy legacy patterns that
refer to `${COMPILE_DEFINITIONS}` in new code.


## Include Boundaries

Public headers live under `include/datoviz/` and are installed. Internal implementation lives under
`src/`. Shared internal helpers live under `src/common/`.

Use public includes for public API:

```c
#include "datoviz/math.h"
#include "datoviz/math/vec.h"
```

Use shared internal includes in implementation:

```c
#include "_alloc.h"
#include "_assertions.h"
```

Keep `${PROJECT_SOURCE_DIR}/src/common` on module and test include paths so `_macros.h` and other
shared internals remain reachable until the public/private split is revisited.

Only install headers from `include/datoviz/`.


## Testing Pattern

All module test sources live under `src/<module>/tests/*.c*`.

Each module exposes an entry point such as:

```c
int test_<module>(TstSuite* suite);
```

That function appends test cases to the shared suite using the `testing.h` suite/item API
(`TEST_SIMPLE`, `TEST`, `AT`, `AC`, `ACn`, and related helpers). Invoke module entry points from
the unified runner; do not use constructors for registration.

For intentionally failing/error-path checks, wrap the assertion in an expected-error scope so known
`log_error()` output is suppressed:

```c
AT_EXPECTED_ERROR_STRICT(suite, (expr_that_should_fail));
```

The unified `dvztest` runner remains the broad aggregation point.
