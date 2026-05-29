# WebGPU Browser Runtime Stress Plan

> **Execution Status**
> - **Status:** `DONE / BROWSER AND DEMO STRESS SLICES LANDED`
> - **Updated on:** `2026-05-29`
> - **Purpose:** record the real-browser retained-runtime and demo-session stress proofs after the
>   `120/120` dashboard proof.


## Current Proof

The browser WebGPU fixture dashboard has passed the committed subset after the retained-runtime
Node smoke landed, after the first real-browser retained-runtime stress slice, and after the
main-demo session stress slice:

1. Browser dashboard: `120/120` rows passed on `2026-05-28` after `183812f27`.
2. Browserless runner smoke: `37` positive DRP2 fixtures, `2` WebGPU attachment streams, and `81`
   semantic negative fixtures.
3. Browserless retained runtime smoke: point, primitive, texture-sampling, and depth-attachment
   streams each render `10` repeated frames through `Drp2WebGpuRuntime` while checking stable live
   resource counts and zero open/recorded refs.
4. Browser dashboard: fixture compatibility `120 pass, 0 unsupported, 0 fail` and retained runtime
   stress `4 pass, 0 fail` passed on `2026-05-29` after `292e82899`.
5. Browser dashboard: fixture compatibility `120 pass, 0 unsupported, 0 fail` and retained runtime
   stress `7 pass, 0 fail` passed on `2026-05-29` after `a1c0d7306`.


## Goal

The landed dashboard stress mode exercises `Drp2WebGpuRuntime` repeated rendering on the real
browser WebGPU device, without mixing the stress rows into the existing fixture compatibility count.

The dashboard should continue reporting the fixture subset separately from runtime stress so a
future result can say, for example:

```text
fixtures: 120 pass, 0 unsupported, 0 fail
runtime stress: 7 pass, 0 fail
```


## Landed Action Items

1. Add a retained-runtime stress section to `examples/webgpu/fixtures.html`.
   - Keep it visually separate from the current fixture table.
   - Use row names such as `runtime repeat: scene_point_wgsl`.
   - Show pass/fail state and compact details for resource-count or WebGPU validation errors.

2. Extend `examples/webgpu/fixture_dashboard.js`.
   - Import and use `Drp2WebGpuRuntime`.
   - Reuse the existing `initWebGPU()` device/context/capability setup.
   - Load each stress stream once, call `runtime.load(stream)`, then call `runtime.render()` for a
     fixed small frame count such as `10`.
   - Compare `runtime.resourceStats()` after each render against the post-load stable stats.
   - Ignore monotonic submitted-work refs if needed, but require `refs.open === 0` and
     `refs.recorded === 0` after every render.

3. Start with a representative stream set.
   - `scene_point_wgsl`
   - `scene_primitive_wgsl`
   - `texture_sampling_wgsl`
   - `attachment_depth_wgsl`

4. Keep strict options enabled where they are already required by the dashboard.
   - `requireExplicitBindGroupLayouts: true`
   - `requireExplicitPipelineMetadata: true`
   - Pass the browser runtime capability snapshot into execution just like the current dashboard
     one-shot rows do.

5. Preserve the existing compatibility dashboard semantics.
   - Do not change the committed `120` fixture row count unless fixtures are added or removed.
   - Do not hide unsupported-feature diagnostics behind the stress mode.
   - Do not fork DRP2 or scene semantics for browser-only convenience.


## Follow-Up After The First Stress Slice

Completed after the first stress slice:

1. Add an interactive update stress for `scene_point_panzoom_wgsl`.
   - Load the stream once.
   - Update the MVP/viewport buffers through the retained runtime path.
   - Render repeated frames and verify resource stats remain stable.

2. Add resize stress.
   - Trigger the dashboard/demo resize path.
   - Confirm the retained stream still renders with stable persistent resource counts.
   - Keep canvas-size and target-format mismatches visible as failures.

Remaining follow-up:

1. Expand stream coverage only after the first stress rows are stable.
   - Candidate next streams: `scene_image_wgsl`, `mesh_dvzr_wgsl`, and
     `triangle_offscreen_readback_wgsl`.
   - Add each stream with an explicit reason and expected browser behavior.

2. Record manual browser proof after each dashboard behavior change.
   - Update `examples/webgpu/COMPAT.md`.
   - Update `agents/now/STATUS.md` or `agents/now/RELEASE.md` only when the proof changes the
     branch-level release state.


## Validation

Before committing implementation changes:

```text
just webgpu-fixture-preflight
just webgpu-runner-smoke
just spec-check
git diff --check
```

After starting the static server from the repository root:

```text
python3 -m http.server 8765
```

Open the browser dashboard:

```text
http://localhost:8765/examples/webgpu/fixtures.html
```

Expected first proof after this plan lands:

```text
fixtures: 120 pass, 0 unsupported, 0 fail
runtime stress: 7 pass, 0 fail
```
