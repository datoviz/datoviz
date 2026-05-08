# Render Pass Batching — Plan

> **Status:** `ACTIVE` — Phase 1 complete (commits ae3c77b4, 3328f4fc, 0bf62fc6, 27a3a310);
> next implementation target is phase 2.
> **Updated:** `2026-05-08`
> **Owner-of-record:** scene + drp2 emitter.
> **Predecessor:** commit `70f057d1` ("scene: emit one render node per visual; FIXED
> controller_mode binds identity MVP") introduced the regression this plan addresses.


## 1. Problem

After the controller_mode work, the scene emits **one DRP2 render node per visual**, each
of which becomes a full Vulkan render pass:

```text
panel with N visuals  →  N × { BeginRenderPass, SetPipeline, SetBindGroup, SetVertexBuffer*, Draw, EndRenderPass }
```

Subsequent passes use `LOAD_OP_LOAD` so the panel composites correctly.


### Cost on different GPUs

| Hardware                         | Per-pass overhead | Practical limit at 60 Hz |
|----------------------------------|-------------------|---------------------------|
| Desktop NVIDIA / AMD discrete    | ~1 µs setup       | hundreds of visuals fine  |
| Apple Silicon (tile-based)       | LOAD reads ~8 MB at 1080p, STORE writes 8 MB | tens of visuals before bandwidth saturates |
| Mobile GPUs (Adreno / Mali / PVR)| same as Apple, often worse | same |
| WebGPU on tile-based             | same as native    | same |

For Datoviz's target use cases — scientific dashboards with many overlays per panel
(axes + grid + scatter + line + selection + legend + crosshair = 7 visuals already) and
plans to ship on Apple Silicon and the web — **the per-visual-pass design caps us well
below the panels-with-many-visuals scenarios users will routinely hit**.


### What we want

```text
panel with N visuals  →  1 × { BeginRenderPass,
                               (SetPipeline, SetBindGroup, SetVertexBuffer*, Draw)×N,
                               EndRenderPass }
```

State changes between draws are cheap. Pass changes are not.


## 2. Plan

Four phases, in increasing order of impact and complexity. Phase 1 is the architectural
fix; the rest are layered optimizations.


### Phase 1 — one pass per panel, multiple draws within

**This is the real fix.** Revert the per-visual node split from `70f057d1` and instead
do per-visual *commands* within a single pass.

#### Frame plan render node — revised shape

```c
struct {
    char panel_id[…];
    char render_target_id[…];
    DvzPanelDesc desc;
    bool picking;

    /* Z-sorted visual list (already in place). */
    uint32_t visual_count;
    char visuals[N][LABEL_SIZE];

    /* NEW: parallel array, one entry per visual. Lets the converter pick the
     * right MVP bind group for each visual within a single pass. */
    DvzControllerMode controller_modes[N];

    /* Panel's APPLY MVP, pre-computed by scene.c from the panzoom/arcball.
     * Identity MVP for FIXED visuals is computed by the converter; not stored here. */
    bool   has_mvp;
    DvzMVP apply_mvp;
} render;
```

#### scene.c

Revert the per-visual `dvz_frame_plan_render_panel` calls. Go back to **one render
node per panel**:

```c
/* (1) Stable z-layer-sorted index list — keep. */
sort visuals by (z_layer, insertion_index);

/* (2) Compute panel APPLY MVP. */
glm_mat4_identity(apply_mvp.{model,view,proj});
if (panel->panzoom) dvz_panzoom_mvp(panel->panzoom, &apply_mvp);
if (panel->arcball) dvz_arcball_mvp(panel->arcball, &apply_mvp);

/* (3) One render node, populate visuals[] + controller_modes[] in z order. */
dvz_frame_plan_render_panel(plan, panel_id, "rt", false, panel->desc);
DvzFramePlanNode* node = dvz_frame_plan_last_render_node(plan);
node->u.render.has_mvp = true;
node->u.render.apply_mvp = apply_mvp;
for k in z_sorted_indices:
    if (visual missing position) continue;
    dvz_frame_plan_render_visual(plan, "v<vidx>");
    node->u.render.controller_modes[node->u.render.visual_count - 1]
        = panel->visuals[k].controller_mode;
```

#### converter.c — `_emitter_emit_render`

Today the function:
1. Takes the concatenated `vertex_buffer_ids[]` from all visuals in the node.
2. Picks a single pipeline based on the data tags found in that array.
3. Emits one `BeginRenderPass + SetPipeline + SetBindGroup + SetVertexBuffer×K + Draw + EndRenderPass`.

Refactor into two stages:

**a) Per-visual resolve.** Replace `_emitter_resolve_render_vertex_buffers` (which
collects all visuals' buffers into one flat list) with a loop:

```c
struct VisualDraw {
    uint64_t pipeline_id;
    uint64_t bind_group_id;          // either apply_mvp_bg or fixed_mvp_bg
    uint64_t vertex_buffer_ids[K];   // the visual's own buffers
    uint32_t vertex_buffer_count;
    uint32_t vertex_count;
    /* + image-visual-specific bg, if any */
};

VisualDraw draws[N];
for each visual_id in node.visuals:
    look up "<visual_id>_position", "_color", "_size", "_texcoords", …
    draws[i].vertex_count = first_position_buffer_byte_size / item_size;
    draws[i].pipeline_id = pipeline_for(family, topology, shader_format);  // cached
    draws[i].bind_group_id = (controller_modes[i] == FIXED) ? fixed_bg : apply_bg;
```

The pipeline cache key becomes `(family, topology, shader_format)`. The infrastructure
is already there — `_obj_id` and the resource map. Today's "pick one pipeline based
on the concatenated buffers" heuristic gets replaced by an explicit per-visual lookup.

**b) Single pass, N draws.**

```c
write apply_mvp_buf  ← apply_mvp                    /* once per panel per frame */
write fixed_mvp_buf  ← identity                     /* once total; cache forever */
BeginRenderPass(panel.desc, clear = first_render_node)
for d in draws:
    if (d.pipeline_id != last_pipeline)  SetPipeline(d.pipeline_id)
    if (d.bind_group_id != last_bg)      SetBindGroup(0, d.bind_group_id)
    /* image visuals also need set 1 (sampler+texture) — that varies per visual */
    if (d.image_bg && d.image_bg != last_image_bg) SetBindGroup(1, d.image_bg)
    SetVertexBuffer(0..K-1, d.vertex_buffer_ids[…])
    Draw(d.vertex_count)
EndRenderPass
```

The state-change tracking (`last_pipeline`, `last_bg`) is the natural way the loop
should be written and folds in part of phase 2 essentially for free.

#### DRP2 / runtime — no changes required

Multiple `SetPipeline` / `SetBindGroup` / `SetVertexBuffer` / `Draw` commands inside
one render pass already work (they're part of the DRP2 stream surface). vklite already
translates each to its `vkCmd*` counterpart. No new opcodes needed.

#### Constraint adjustments

`DVZ_SCENE_MAX_NODE_RESOURCES = 8` is currently a *total* across all visuals on the
node and bites at the third visual on a panel (3 visuals × 3 attrs = 9 > 8). With the
refactor it becomes a *per-visual* bound, and 8 is plenty (no visual family today has
more than 4 attrs).

#### Tests

- Update `test_scene_z_layer_orders_emit`: it was written against the per-visual-pass
  output where N visuals → N `Draw` commands in N passes. After phase 1 it's still
  N `Draw` commands but inside one pass. The order assertion (3-vertex draw before
  5-vertex draw) still holds; just the surrounding `BeginRenderPass / EndRenderPass`
  structure changes.

- Update `test_scene_controller_mode_fixed_emits_separate_mvp`: still expects two
  MVP UBOs (one apply, one fixed) — that doesn't change. What does change is that
  there's now one `BeginRenderPass` with both `Draw` commands inside it, not two
  separate passes.

- New test `test_scene_panel_one_pass_per_panel`: emit a panel with 3 visuals,
  verify exactly one `BeginRenderPass` per panel in the DRP2 JSON.

- New offscreen pixel test `test_app_offscreen_panel_three_visuals_all_drawn`:
  put three non-overlapping points (red / green / blue) on one panel, capture,
  assert all three colors are present in the framebuffer. Today's converter (even
  pre-phase-1) only renders the first sorted visual's `vertex_count`-worth of
  geometry, so this would catch any regression in "all visuals actually draw."


### Phase 2 — pipeline / bind-group state caching across panels

Same pattern as the within-panel tracker, but across passes within a frame: many
panels share the same point pipeline, the same identity MVP bind group, etc. The
`last_*` trackers can survive across panels (a `BeginRenderPass` doesn't invalidate
them at the *DRP2 stream* level even if Vulkan considers state pass-scoped). For
Vulkan, state IS pass-scoped, so each pass still re-binds — but the DRP2 emit can
skip redundant emit if the same panel emits the same state on its own subsequent
draws.

Tiny code change once phase 1 is in. ~30 lines.


### Phase 3 — one render pass per figure, panels as scissored sub-regions

For figures with many panels (grid dashboards: 10×10 = 100 panels), per-panel passes
multiply the cost again. The standard fix:

```text
figure (N panels)  →  1 × {
    BeginRenderPass(figure_extent, LOAD_OP_CLEAR cfg.clear_color),
    for each panel:
        SetViewport(panel.rect)
        SetScissor(panel.rect)
        emit panel's draws (phase 1 layout)
    EndRenderPass
}
```

What this requires:

- The emit_render plumbing currently takes one panel desc per BeginRenderPass and
  uses it both as render area and as viewport. Split: the render area becomes the
  whole figure framebuffer; the viewport / scissor is per-panel via DRP2
  `SetViewport` / `SetScissor` commands inside the pass.
- DRP2 already has set_viewport / set_scissor. Confirmed.
- Per-panel "clear color" (when distinct from figure's clear color) becomes either
  a quad inside the panel's scissor (already what `dvz_panel_set_background_color`
  does — perfect), or `vkCmdClearAttachments` with `pRects` if we ever want
  fragment-skip-clear. Quads remain the recommended path.
- Multi-pass things that *do* need separate passes — e.g. transparent-OIT accumulate
  + resolve from `spec/scene/TRANSPARENCY.md` — must be done before this phase 3
  collapse, because OIT's resolve pass naturally requires its own pass anyway.
  This is fine; OIT comes after.

Bigger change than phase 1, deserves its own design pass before implementation.
Defer.


### Phase 4 — sort by pipeline within a z-layer slab

Once we have lots of visuals per panel, the per-draw cost shifts from passes (gone
after phase 1+3) to *pipeline switches*. Within consecutive visuals on the same
z-layer, group by pipeline. Don't reorder *across* z layers (that breaks user-visible
ordering); only within ties.

Small win, no architectural change. Optional.


## 3. Phase 1 task breakdown

Concrete sequencing for the next session:

1. **Revert per-visual render-node emit in `scene.c`.** Restore one
   `dvz_frame_plan_render_panel` per panel; populate `visuals[]` and the new
   `controller_modes[]` parallel array. Compute and stash `apply_mvp` on the node
   from the panzoom/arcball.

2. **Add `controller_modes[N]` to the render arm of `DvzFramePlanNode`** in
   `src/scene/_frame_plan.h`. Keep the existing `mvp` / `has_mvp` fields (now
   semantically the panel's APPLY MVP).

3. **Refactor `_emitter_emit_render`** in `src/scene/converter.c` to:
   - Build a per-visual `VisualDraw[]` list.
   - Cache pipelines by `(family, topology, shader_format)` via `_obj_id`.
   - Emit `BeginRenderPass` once, then loop with state-change tracking, then
     `EndRenderPass`.
   - Write apply MVP and identity MVP to their respective UBOs once per node.

4. **Drop the per-node-total cap on `MAX_NODE_RESOURCES`**, reinterpret as
   per-visual.

5. **Test updates** (described above): adjust the two existing z_layer / controller
   tests; add the one-pass-per-panel structural test and the three-visuals pixel
   test.

6. **Hello-points pixel sanity:** after phase 1, three points on one panel + a
   background quad must all render. Today the third visual is silently dropped by
   the converter's composite-draw heuristic — phase 1 fixes that incidentally.

7. **Smoke run** of `hello_point_glfw` to confirm panzoom + background still work.


## 4. Estimated effort

| Phase | Scope | LOC | Effort |
|-------|-------|-----|--------|
| 1     | Within-panel pass batching            | ~200 + tests | 1 session |
| 2     | State-change tracker                  | ~30          | 30 min after phase 1 |
| 3     | Figure-wide pass + per-panel viewport | ~400 + tests | own session, design doc first |
| 4     | Pipeline-sort within z-slab           | ~50          | optional |


## 5. Non-goals for this plan

- **No changes to the public C API.** `dvz_panel_add_visual`, `DvzVisualAttachDesc`,
  background API, controller API all stay as they are. The refactor is internal to
  the scene → frame plan → converter path.
- **Not introducing `vkCmdClearAttachments`.** Per-panel backgrounds remain quads
  (see §6 below).
- **Not re-batching at the DRP2 layer.** DRP2 is intentionally one-command-per-call;
  batching is a scene/converter concern.
- **Not addressing transparency / OIT.** That's `spec/scene/TRANSPARENCY.md` and is
  orthogonal — it adds *more* passes (accum + resolve). Phase 3's "one pass per
  figure" needs to be revisited once OIT lands.


## 6. Per-panel clears: confirmed quad path

Reaffirming the design choice locked in by commit `100d24ae`:

| Use case                              | Mechanism                          |
|---------------------------------------|------------------------------------|
| Initial figure-wide framebuffer clear | `LOAD_OP_CLEAR` at pass start (`cfg->clear_color`) |
| Per-panel background (color/grad/img) | `dvz_panel_set_background_color` + future `_gradient` / `_image` — fullscreen quad attached at z_layer=-1, controller_mode=FIXED |
| Mid-pass region clear                 | **not needed** — quads at z_layer=-1 already do this; `vkCmdClearAttachments` rejected (solid-color only, no perf win on tile-based GPUs, breaks rendering uniformity) |


## 7. Why phase 1 first

Phase 1 fixes a *correctness* gap as well as a perf gap. Today (post-`70f057d1`),
multi-visual-per-panel rendering quietly drops all but the first sorted visual's
draw because the converter still emits one composite Draw per pass with the
*total* `vertex_count` driven by a single visual's position buffer. After phase 1,
each visual gets its own `Draw` with its own vertex count — the rendering matches
the data the user supplied.

The `dvz_panel_set_background_color` example we shipped already exercises this:
two visuals on a panel (background quad + points), and only the background actually
draws fully; the points are clipped to the quad's vertex_count (4) and silently
truncated. **The example produces wrong pixels right now** for any panel with
≥2 visuals, even though the JSON looks plausible. That's a bug ceiling on every
multi-visual demo we'd want to ship.

So phase 1 isn't "just" a perf cleanup — it's the prerequisite for correct
multi-visual scenes. Land it before the next visual family (text, mesh, line)
because each new family will be exercised in multi-visual contexts and the
truncation bug will mask their behavior.
