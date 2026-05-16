# DRP2 Normal Trace Normalization Plan

## Problem

`DVZ_DRP2_TRACE=normal` currently reports the LIDAR EDL stream as changed on every live frame even
after warmup. A 20-frame smoke showed that object creation stops after the initial frame-resource
ring is populated, but consecutive frames still use different scoped EDL color/depth targets and
resolve bind groups:

```text
frame 4  uses EDL targets 28/27 and bind group 5017
frame 5  uses EDL targets 30/29 and bind group 5021
frame 6  uses EDL targets 32/31 and bind group 5022
frame 7  uses EDL targets 34/33 and bind group 5023
frame 8  reuses EDL targets 28/27 and bind group 5017
```

This is expected for live GLFW presentation because `app` sets
`cfg.runtime_resource_scope_id = _app_frame_runtime_scope(frame)`, and frame-graph resources such
as EDL targets are scoped to the borrowed canvas/swapchain frame. The trace problem is that normal
mode compares adjacent frames literally enough that a stable frame-resource ring looks like a
semantic stream change.

`full` trace should keep raw ids. `normal` trace should compare stable rendering intent.

## Goal

Make `DVZ_DRP2_TRACE=normal` report `unchanged` for stable live streams that only rotate through
frame-scoped resources, while still reporting `changed` for real semantic changes such as:

- EDL on/off or pass count changes.
- Depth cue shader vs non-cue shader changes.
- Draw count, topology, or pipeline state changes.
- Render target extent, format, usage, attachment load/store, or depth/write state changes.
- Different visual/resource roles.

## Strategy

1. Keep raw `full` trace unchanged.

2. Add a trace-side semantic id resolver in `src/app/trace.c`.

   The resolver should be used only by the normalized snapshot builder and normal-mode equality.
   It should not modify DRP2 streams, runtime ids, or labels.

3. Prefer labels over numeric ids when labels exist.

   For every command field that currently contributes an id to the normal snapshot, resolve the id
   with `dvz_drp2_stream_label()`. Use the label when available, falling back to the raw id only for
   genuinely unlabeled resources.

4. Strip frame-scope suffixes from labels.

   Normalize labels like:

   ```text
   fig0_p0.edl.color_scope_0000644c743ff4b0
   fig0_p0.edl.depth_scope_0000644c743ff4b0
   fig0_p0.wboit.accum_scope_000000000000007c
   fig0_p0.peel.front_ping_scope_000000000000007b
   ```

   to stable semantic labels:

   ```text
   fig0_p0.edl.color
   fig0_p0.edl.depth
   fig0_p0.wboit.accum
   fig0_p0.peel.front_ping
   ```

   The first implementation can strip any suffix matching `_scope_<16 hex digits>`.

5. Normalize derived per-frame bind groups.

   EDL resolve bind groups are currently keyed by per-frame texture ids, for example:

   ```text
   _bg_edl_28_27_26
   _bg_edl_30_29_26
   ```

   Normal trace should display these by semantic role, not by raw dependency ids. A pragmatic first
   pass can normalize all `_bg_edl_*` labels to `_bg_edl_resolve`. A stronger follow-up can derive
   a dependency signature such as:

   ```text
   _bg_edl(fig0_p0.edl.color, fig0_p0.edl.depth, fig0_p0.edl.params)
   ```

   Apply the same principle to WBOIT and depth-peel resolve/sample bind groups if their labels or
   entries include scoped resource ids.

6. Keep structural data in the normalized lines.

   Normalization should not erase fields that indicate real work:

   - render/compute pass ordinal,
   - target role and depth role,
   - viewport/scissor,
   - pipeline semantic label or id,
   - bind-group slot and semantic role,
   - vertex/index buffer semantic role,
   - draw payload,
   - copy/readback dimensions and byte ranges.

## Suggested Implementation Shape

Add small helpers to `src/app/trace.c`:

```c
static bool _trace_semantic_id(
    const DvzDrp2CommandStream* stream, uint64_t id, char* out, uint32_t out_size);

static bool _trace_normalize_label(
    const char* label, char* out, uint32_t out_size);
```

Then thread `stream` through `_trace_snapshot_append_command()` so snapshot lines can call the
resolver instead of formatting raw ids directly. Keep the raw command printer in `src/app/app.c`
unchanged.

## Tests

Add focused tests in `src/app/tests/test_app.c`:

1. Two streams with different raw pass/encoder ids still compare equal.

   Existing coverage already checks this; keep it.

2. Two streams with different frame-scoped EDL target labels compare equal.

   Build stream A with labels:

   ```text
   fig0_p0.edl.color_scope_aaaaaaaaaaaaaaaa
   fig0_p0.edl.depth_scope_aaaaaaaaaaaaaaaa
   _bg_edl_28_27_26
   ```

   Build stream B with labels:

   ```text
   fig0_p0.edl.color_scope_bbbbbbbbbbbbbbbb
   fig0_p0.edl.depth_scope_bbbbbbbbbbbbbbbb
   _bg_edl_30_29_26
   ```

   The normalized snapshots should compare equal.

3. A real semantic change still compares different.

   Examples:

   - EDL two-pass stream vs one-pass opaque stream.
   - Pixel cue pipeline label vs non-cue pixel pipeline label.
   - `draw vertices=10397299` vs another count.
   - target extent or depth attachment state change.

4. Snapshot line count checks should assert normalized output remains readable, for example:

   ```text
   render#0 target=fig0_p0.edl.color clear=yes depth=yes depth_target=fig0_p0.edl.depth
   pass#1 bind[0]=_bg_edl_resolve
   ```

## Validation

After implementation:

```bash
cmake --build build --target dvztest_app showcase_lidar_glfw -j2
./build/testing/dvztest_app trace
DVZ_DRP2_TRACE=normal DVZ_DRP2_TRACE_COLOR=0 NO_COLOR=1 \
  ./build/examples/c/showcase_lidar_glfw 20
git diff --check
```

Expected live trace behavior after warmup:

- Frame 0 remains `changed`.
- Frames that introduce a new frame-resource scope may still be `changed` until the ring is
  populated.
- Once the ring is populated, stable frames should report `unchanged` unless the user toggles EDL,
  depth cueing, point size, resize, or navigation state.

## Implementation Status

Implemented on `2026-05-16`:

- Normalized snapshot ids now prefer DRP2 stream labels over numeric ids.
- Labels ending in `_scope_<16 hex digits>` are compared and printed without the frame-scope
  suffix.
- EDL and WBOIT bind-group labels derived from rotating texture ids are collapsed to stable resolve
  roles in normal trace snapshots.
- Normal-mode changed-frame dumps keep the existing detailed raw display, including command
  prefixes, ids, labels, and color handling. The normalized snapshot is used only for
  `changed`/`unchanged` detection and semantic line counts.
- App trace tests cover scoped EDL target/bind-group normalization and still catch real draw-count
  changes.

Validation run:

```bash
cmake --build build --target showcase_lidar_glfw dvztest -j2
./build/testing/dvztest app
DVZ_DRP2_TRACE=normal DVZ_DRP2_TRACE_COLOR=0 NO_COLOR=1 \
  ./build/examples/c/showcase_lidar_glfw 20
```

Observed LIDAR normal trace:

```text
frame 00000000 | changed   | 69 cmds | 58 semantic
frame 00000001 | changed   | 34 cmds | 25 semantic
frame 00000002 | unchanged | 34 cmds | 25 semantic
frame 00000003 | unchanged | 34 cmds | 25 semantic
frame 00000004 | changed   | 31 cmds | 22 semantic
frame 00000005 | unchanged | 31 cmds | 22 semantic
...
frame 00000019 | unchanged | 31 cmds | 22 semantic
```

Frame 4 is a real normalized-shape change because the frame-resource creation commands disappear
after the ring is populated; frames after that stay unchanged for the stable view.
