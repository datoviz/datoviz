# Public API Pre-RC Audit Handoff

Status: approved pre-RC API/ABI cleanup campaign.

Purpose: preserve the July 2026 user-side public API consistency audit and give a future agent a
concrete plan to implement, validate, commit, and push the cleanup. The audit focused on function
naming, signatures, argument ordering, constness, ownership, raw `ctypes` shape, and legacy or
accidental public symbols.

Primary recommendation: break API/ABI now where the current surface is accidental, transitional,
hard to bind correctly, or inconsistent with v0.4 architecture. Do not preserve v0.3 compatibility
when it conflicts with a cleaner v0.4 surface.

Maintainer decision, July 2026: use the pre-RC window aggressively. It is critically important that
the v0.4 API is internally consistent before RC1, even if that means broad source/ABI breaks across
examples, docs, tests, raw `ctypes`, and public C headers.


## Maintainer Decisions

Use these defaults unless the code audit finds a stronger local reason:

1. Remove unused, legacy, transitional, or accidental public APIs aggressively.
2. Do not preserve v0.3 compatibility aliases when they weaken v0.4 naming, architecture, binding
   safety, ownership clarity, or consistency.
3. Migrate examples, tests, docs, generated bindings, and generated references in the same
   checkpoint commits as the API breaks.
4. Keep `datoviz.raw` exact to the exported ABI, but do not let raw `ctypes` expose known ownership
   traps such as owned `char*` returns bound as `c_char_p`.
5. Prefer backend-neutral stable app API names. GLFW-specific setup and conversion helpers belong
   in backend or interop surfaces unless deliberately classified as stable ABI.
6. Keep DRP2/vklite protocol and runtime escape hatches public only where useful, and classify them
   as advanced/unstable. Move fixture, JSON/base64 convenience, raw fallback, and development
   diagnostics out of the stable public surface where practical.
7. Use `DvzResult` for ordinary fallible Datoviz APIs, `bool` for predicates, raw integer returns
   for non-error status/count values, and Vulkan-native result codes only in Vulkan escape hatches.
8. Enforce one argument-ordering convention: object first; selector/config/range next; payload
   pointers before counts/sizes; byte-buffer APIs use `bytes, size_bytes`.
9. Make ownership and borrowing explicit with `const` resource arguments, copy-out descriptor/info
   APIs, and explicit free APIs or FFI policy for owned pointer returns.
10. Remove or prefix unprefixed public support macros. Prefer typedefs and inline/static helpers
    over public macros when a real type or helper is intended.


## Non-Negotiable Pickup Rules

1. Read `AGENTS.md`, `agents/now/START.md`, `agents/now/STATUS.md`, and this handoff first.
2. Before public header, exported API, raw-binding policy, or generator changes, read
   `spec/bindings/ARRAY_FACADE.md` and `spec/bindings/CTYPES_POLICY.md`.
3. After changing public headers, exported API, binding policy, or binding generators, refresh and
   validate bindings with:

   ```sh
   just ctypes
   just ctypes-check
   ```

4. Always run `git diff --check` before finalizing.
5. Before committing, run:

   ```sh
   git status --short
   git diff --cached --stat
   ```

   Verify the staged set excludes `data`, generated/runtime binaries, vendored runtime libraries,
   and unrelated user changes.


## Highest Priority Findings

### 1. Remove Internal Object/Container API From Public Surface

`DvzObject`, `DvzContainer`, object status/type enums, and generic container functions are installed,
exported, documented, and emitted in raw `ctypes`, but look like internal lifetime/runtime plumbing.

References:

- `include/datoviz/common/obj.h`
- `datoviz/_ctypes.py`

Preferred fix:

- Move object/container declarations to private/internal headers, or remove `DVZ_EXPORT`.
- If any pieces must remain installed, classify them explicitly as advanced/unstable and exclude
  them from ordinary C docs/raw public examples.


### 2. Remove Legacy And Transitional Scene Symbols

The public scene surface still exposes aliases and wrappers that are already documented as old or
transitional.

References:

- `include/datoviz/scene/animation.h`: `DVZ_CLOCK_REALTIME`, `DVZ_CLOCK_OFFLINE`
- `include/datoviz/scene.h`: old and new 2D view APIs coexist around `DvzPanelView2D`,
  `dvz_panel_view2d()`, `dvz_panel_set_view2d()`, and `dvz_panel_set_view2d_desc()`
- `include/datoviz/scene.h`: `dvz_visual_set_texture_rgba8()` and
  `dvz_visual_set_texture_r32f()` are documented as legacy/transitional wrappers

Preferred fix:

- Remove legacy clock aliases; keep only `DVZ_SCENE_CLOCK_REALTIME`,
  `DVZ_SCENE_CLOCK_FIXED_STEP`, and `DVZ_SCENE_CLOCK_EXTERNAL`.
- Collapse the 2D view API onto `DvzPanelView2DDesc`; rename
  `dvz_panel_set_view2d_desc()` to `dvz_panel_set_view2d(DvzPanel*, const DvzPanelView2DDesc*)`.
- Remove texture wrappers from the stable API, or rename them as explicit sampled-field helpers
  such as `dvz_visual_set_field_rgba8_2d()` with a slot name.


### 3. Fix Raw `ctypes` Ownership Traps

Owned pointer returns and fixed-array pointer types currently generate unsafe or awkward raw ctypes.

References:

- `include/datoviz/fileio/fileio.h`: `dvz_read_gz()` returns owned `char*`
- `include/datoviz/math/mock.h`: `dvz_mock_*()` helpers return owned arrays such as `vec3*`
- `datoviz/_ctypes.py`: owned `char*` is bound as `c_char_p`; `vec3*` returns are awkward

Preferred fix:

- Bind owned byte/string returns as `c_void_p` or explicit pointer types, not `c_char_p`.
- Add or update owned-return binding policy entries.
- Demote mock-data helpers from the public API, or provide explicit FFI helpers/policy for owned
  array returns.
- If mock helpers remain public, rename uppercase dimension suffixes to lowercase:
  `dvz_mock_pos_2d()`, `dvz_mock_pos_3d()`.


### 4. Make Canvas Flags Atomic Bit Flags

`DVZ_CANVAS_FLAGS_FPS = 0x0003` and `DVZ_CANVAS_FLAGS_MONITOR = 0x0005` implicitly include
`DVZ_CANVAS_FLAGS_IMGUI`.

Reference:

- `include/datoviz/canvas/enums.h`

Preferred fix:

- Use one bit per flag, for example `IMGUI = 0x0001`, `FPS = 0x0002`, `MONITOR = 0x0004`.
- Add explicit named combinations only if needed.


### 5. Decide Whether GLFW Is Stable API Or Backend Detail

The default public API exposes GLFW-specific names and compatibility assumptions.

References:

- `include/datoviz/app.h`: `DVZ_VIEW_GLFW`, `dvz_view_glfw()`, GLFW extension config
- `include/datoviz/input/pointer.h`: `dvz_pointer_button_from_glfw()`
- `include/datoviz/input/enums.h` and `include/datoviz/input/keycodes.h`: numeric compatibility
  comments

Preferred fix:

- Rename stable app-facing concepts to backend-neutral names, such as `DVZ_VIEW_WINDOW` and
  `dvz_view_window()`.
- Move GLFW conversion helpers and GLFW-specific setup to backend or interop headers.
- If numeric GLFW compatibility is intentional ABI, document it as such.


## Consistency Fixes Worth Doing Before RC

### 6. Standardize Result Types

Public fallible APIs mix `DvzResult`, `bool`, raw `int`, `int32_t`, and pointer-or-NULL.

References:

- `include/datoviz/common/types.h`: `DvzResult`
- `include/datoviz/app.h`: mostly `DvzResult`
- `include/datoviz/canvas.h`, `include/datoviz/stream.h`, `include/datoviz/window/backend.h`
- `include/datoviz/vklite/commands.h`, `include/datoviz/vklite/buffers.h`,
  `include/datoviz/vklite/sync.h`

Preferred fix:

- Use `DvzResult` for ordinary Datoviz success/error APIs.
- Keep raw `int` only where non-error status values are part of the contract.
- Keep Vulkan-native result codes only in explicitly Vulkan-native escape hatches.


### 7. Pick One Array/Count Argument Convention

Adjacent APIs mix `data, count` and `count, data`.

References:

- `include/datoviz/scene.h`: `dvz_visual_set_data()`, `dvz_visual_set_data_range()`
- `include/datoviz/scene/plot.h`: bars and band setters
- `include/datoviz/scene/text.h`: text batch setters

Preferred convention:

```text
object, selector, range-if-any, data pointer(s), count/size
```

Apply this especially to `dvz_visual_set_data_range()` and text batch setters.


### 8. Tighten Constness And Borrowed Ownership

Bind-only APIs accept mutable resources, and descriptor getters expose borrowed internals.

References:

- `include/datoviz/scene/field.h`: `dvz_visual_set_field()`,
  `dvz_sampled_field_get_desc()`
- `include/datoviz/scene/scale.h`: `dvz_visual_set_scale()`
- `include/datoviz/scene.h`: symbol-set bindings and `dvz_scene_buffer_get_desc()`
- `include/datoviz/scene/text.h`: `dvz_text_atlas_field()`

Preferred fix:

- Use `const DvzSampledField*`, `const DvzScale*`, and `const DvzSymbolSet*` for bind-only
  resource arguments when ownership is not transferred.
- Replace borrowed descriptor getters with copy-out APIs such as
  `bool dvz_sampled_field_info(const DvzSampledField*, DvzSampledFieldDesc* out)` and
  `bool dvz_scene_buffer_info(const DvzSceneBuffer*, DvzSceneBufferDesc* out)`.


### 9. Clean Or Classify Advanced DRP2

The DRP2 public headers mix protocol builders, fixture/JSON plumbing, base64 payloads, byte
payloads, and development replay fallback.

References:

- `include/datoviz/drp2/recording.h`: `DvzDrp2RawFallback`
- `include/datoviz/drp2/stream.h`: base64 and byte write APIs, JSON helpers
- `include/datoviz/drp2/stream.h`: stringly typed shader stage/format, index format, barriers

Preferred fix:

- Remove raw fallback from RC public API, or move it to private/test diagnostics.
- Make byte-oriented APIs primary.
- Rename base64 forms with `_base64` or move them to fixture/JSON headers excluded from stable docs.
- Replace stringly typed command arguments with enums where feasible; otherwise classify these
  commands as advanced/unstable.


### 10. Clean Support-Header Leakage

Public support headers leak unprefixed macros, mutable TU-local buffers, test resources, and
inconsistent byte-buffer signatures.

References:

- `include/datoviz/math/types.h`: `MIN`, `MAX`, `CLIP`, `GB`, `MB`, `KB`, `EPSILON`,
  `_PRETTY_SIZE`, `DvzAlpha` macro
- `include/datoviz/math/box.h`: public declarations that lack `DVZ_EXPORT`
- `include/datoviz/fileio/fileio.h`: `dvz_load_png(bytes, size)` versus
  `dvz_load_jpeg(size, bytes)`, `int*` PPM dimensions, `unsigned long*` resource sizes,
  `dvz_resource_testdata()`

Preferred fix:

- Prefix public macros (`DVZ_MIN`, `DVZ_MAX`, etc.) or remove them from public headers.
- Replace `#define DvzAlpha uint8_t` with `typedef uint8_t DvzAlpha`.
- Keep only reentrant `dvz_pretty_size_r()` or make `dvz_pretty_size()` thread-safe.
- Export and document the full `dvz_box_*` group or make non-exported declarations private.
- Normalize byte-buffer APIs to `bytes, size_bytes`, use unsigned dimensions, use `DvzSize*` or
  `size_t*` consistently, make borrowed resources `const`, and demote `dvz_resource_testdata()`.


## Lower-Priority Polish

- Rename `dvz_scalebar_*` to `dvz_scale_bar_*` for consistency with `DvzScaleBar`.
- Normalize graph and polygon mutators to `*_set_*`, for example `dvz_graph_set_edges()` and
  `dvz_polygon_set_fill_color()`.
- Make `dvz_sampled_field_destroy()` return `void`, matching other destroy APIs.
- Document public text batch setters or remove them in favor of `dvz_text_set_items()` plus
  single-item helpers.
- Decide whether geometry factories should be `dvz_geom_*` or `dvz_geometry_*`, then keep one
  public naming family.
- Split stable `window.h` from backend SPI if window APIs are meant to be user-facing; currently
  `window.h` includes backend registration and wrap-surface internals.
- Review `advanced.h` and `vk.h` umbrella consistency. Either make `vk.h` the full low-level Vulkan
  umbrella or split/rename it so omissions are intentional.


## Suggested Execution Plan

Use small checkpoint commits. Each commit should keep public headers, implementation, tests,
generated ctypes, generated docs if applicable, and examples in sync.

1. **API exposure cleanup**
   - Remove or demote internal object/container API.
   - Remove legacy clock aliases and transitional texture wrappers.
   - Remove or demote mock-data helpers.
   - Validation: `just ctypes`, `just ctypes-check`, narrow compile/build check.

2. **Stable scene naming cleanup**
   - Collapse 2D view APIs.
   - Normalize scale-bar spelling.
   - Normalize graph/polygon mutator names.
   - Change `dvz_sampled_field_destroy()` to `void`.
   - Validation: scene tests, examples that use affected APIs, `just ctypes-check`.

3. **Signature and ownership cleanup**
   - Standardize array/count order.
   - Tighten `const` on bind-only resources and read-only queries.
   - Replace borrowed descriptor getters with copy-out APIs.
   - Validation: affected C tests, raw ctypes regeneration/check, Python smoke if bindings changed.

4. **Runtime/app consistency cleanup**
   - Fix canvas flags.
   - Decide GLFW-neutral app names.
   - Standardize `DvzResult` versus raw `int`.
   - Split stable window API from backend SPI if scope permits.
   - Validation: app/canvas/window/input tests and hosted/offscreen examples where available.

5. **Advanced DRP2/vklite classification cleanup**
   - Remove DRP2 raw fallback from stable surface or clearly move it to diagnostics.
   - Rename or move base64/JSON fixture APIs.
   - Align draw argument ordering or classify one layer as protocol-specific.
   - Add enums for stringly typed command arguments where feasible.
   - Validation: DRP2 stream tests, frame-plan emission tests, WebGPU/WASM fixture tests if command
     schema changes.

6. **Support-header cleanup**
   - Prefix/remove public macros.
   - Normalize file I/O buffer signatures.
   - Fix `dvz_box_*` export/declaration mismatch.
   - Validation: fileio/math/geom tests, raw ctypes regeneration/check.


## Validation Baseline For Future Agent

Minimum after any public API change:

```sh
just ctypes
just ctypes-check
git diff --check
```

Recommended broader checks by touched surface:

```sh
just build
just test scene
just test app
just test drp2
just spec-check
```

Before each commit:

```sh
git status --short
git diff --cached --stat
```

Do not stage or commit `data` submodule updates, generated/runtime binary payloads, vendored
runtime libraries, or unrelated user changes without explicit approval in the current turn.
