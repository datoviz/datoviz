# Scene File Split Refactor Plan

Status: active planning note, created 2026-05-21.

This note records a focused organization refactor for the active v0.4 scene stack. It is motivated
by recurring merge-risk hotspots around path-native stroke work, text/font/atlas/MSDF refactoring,
and depth-peeling upgrades.

The goal is not to redesign the scene API or renderer contract. The goal is to reduce file-level
responsibility overload while preserving the existing scene -> frame-plan -> DRP2 -> runtime path.


## Problem

Several scene implementation files now concentrate unrelated responsibilities. That makes parallel
work risky because independent feature lanes repeatedly collide in the same translation units.

Current high-risk files:

1. `src/scene/frame_plan_runtime.c`
   - Graph resource lookup and texture target resolution.
   - Bind group layout and bind group creation.
   - Technique target preparation for WBOIT, depth peeling, SSAO, EDL, and G-buffer.
   - Render-pass and draw command emission.

2. `src/scene/visual.c`
   - Visual constructors.
   - Attribute storage and validation.
   - Material and style state.
   - Family-specific state for point, marker, segment, path, image, text, glyph, sphere, and volume.

3. `src/scene/scene_emit.c`
   - Derived visual uploads.
   - Visual metadata attachment.
   - Panel render planning.
   - Frame graph pass assembly.

4. `src/scene/visual_pipeline.c`
   - Visual descriptor recognition and lowering.
   - Pass capability decisions.
   - Shader descriptor selection.
   - Pipeline layout and vertex layout descriptions.
   - Bind layout requirements.

5. `src/scene/technique.c`
   - Technique state.
   - Frame graph construction for WBOIT, blended transparency, depth peeling, EDL, SSAO, and
     G-buffer passes.

6. `src/scene/tests/scene_graph.c`
   - Many unrelated regression lanes in one large test file.


## Constraints

1. Keep this as no-behavior-change extraction work unless a feature lane explicitly owns a behavior
   change.
2. Keep comments and update them when needed.
3. Do not move active source-of-truth design material into `docs/`.
4. Preserve the active scene -> frame-plan -> DRP2 -> vklite/canvas path.
5. Avoid broad formatting churn.
6. Do not mix file extraction with path-native stroke, text/MSDF, or depth-peeling behavior changes
   in the same patch unless the extraction is required for that behavior change.


## Recommended Order

### 1. Split `visual_pipeline.c` First

This is the most important collision point for path-native strokes, text/MSDF shader changes, and
depth-peeling upgrades.

Target files:

1. `src/scene/visual_desc.c`
   - Metadata and render-node lowering to `DvzSceneVisualDesc`.
   - Visual family recognition helpers.

2. `src/scene/visual_pass_caps.c`
   - Opaque, blended, WBOIT, depth-peel, depth-test, depth-write, and G-buffer capability decisions.

3. `src/scene/visual_shader_desc.c`
   - Shader key selection.
   - Built-in shader identity and feature variant resolution.

4. `src/scene/visual_pipeline_desc.c`
   - Vertex layout, topology, depth state, alpha state, and render pipeline descriptor assembly.

5. `src/scene/visual_bind_desc.c`
   - Common/material/image/glyph/volume bind layout requirement resolution.

Keep `src/scene/visual_pipeline.c` as a small facade during the transition if that keeps call sites
stable.


### 2. Split `scene_emit.c`

Target files:

1. `src/scene/visual_uploads.c`
   - Segment, path, image, volume, and sampled-field upload helpers.
   - Derived GPU cache upload emission.

2. `src/scene/visual_metadata.c`
   - `_scene_visual_frame_plan_metadata()` and related resource-key metadata helpers.

3. `src/scene/panel_render_emit.c`
   - Panel render-node assembly.
   - Render-pass append logic.
   - Draw-contract placement.

Keep top-level scene emission orchestration in `scene_emit.c`.


### 3. Split `frame_plan_runtime.c`

Target files:

1. `src/scene/runtime_graph_resources.c`
   - Graph resource lookup.
   - Texture target resolution.
   - Attachment and usage conversion helpers.

2. `src/scene/runtime_bind_groups.c`
   - Common, material, glyph, volume, WBOIT, depth-peel, SSAO, EDL, and occlusion bind groups.

3. `src/scene/runtime_technique_targets.c`
   - WBOIT, depth-peel, SSAO, EDL, and G-buffer target preparation.

4. `src/scene/runtime_render_emit.c`
   - Render pass emission.
   - Multi-draw and plain render emission.
   - Graph render pass execution.

Keep `dvz_frame_plan_emitter_emit_drp2()` in `frame_plan_runtime.c` as the public entry point.


### 4. Split `visual.c`

Target files:

1. `src/scene/visual_attrs.c`
   - Attribute validation, storage-name mapping, data mutation, and dirty tracking.

2. `src/scene/visual_material.c`
   - Material descriptors, alpha mode, depth cue, and material uniform synchronization.

3. `src/scene/visual_styles.c`
   - Point, marker, segment, and future path stroke style helpers.

4. `src/scene/visual_families.c`
   - Family constructors and narrow family setters.

If family-specific state keeps growing, split further into files such as `visual_path.c`,
`visual_segment.c`, and `visual_volume.c`.


### 5. Split `technique.c`

Target files:

1. `src/scene/technique_state.c`
   - Technique state initialization and public state setters.

2. `src/scene/technique_graph_wboit.c`
   - WBOIT frame graph construction.

3. `src/scene/technique_graph_depth_peel.c`
   - Depth-peeling frame graph construction.

4. `src/scene/technique_graph_ssao.c`
   - SSAO and blur frame graph construction.

5. `src/scene/technique_graph_gbuffer.c`
   - G-buffer planning and frame graph construction.

Shared graph helpers can stay in `technique.c` initially or move to `technique_graph_common.c` when
the second extraction needs them.


### 6. Split Scene Tests

Target test files:

1. `src/scene/tests/scene_visuals.c`
   - Retained visual construction, emission, and visual-family regressions.

2. `src/scene/tests/scene_techniques.c`
   - WBOIT, depth peeling, EDL, SSAO, MSAA, G-buffer, and transparency tests.

3. `src/scene/tests/scene_runtime.c`
   - DRP2 runtime execution, offscreen smoke paths, and frame graph execution tests.

4. `src/scene/tests/scene_interaction_graph.c`
   - Picking, probing, request propagation, and selection graph tests.

Keep existing test registration stable while moving cases in small batches.


## Parallel Work Guidance

Path-native stroke work can proceed in parallel with text/MSDF and depth-peeling work if the shared
files above are handled carefully.

Preferred ownership:

1. Path work owns path state, path cache/lowering, `path.vert`, `path.frag`, and path tests.
2. Text/MSDF work owns `text_atlas.cpp`, `text_annotation.c`, glyph/text shaders, text/glyph
   layouts, and text tests.
3. Depth-peeling work owns technique graph and runtime target changes for the depth-peel path.

Shared files should be edited additively and late in each feature lane. The highest-risk shared
files are `visual_pipeline.c`, `shader_registry.c`, `scene_emit.c`, `frame_plan_runtime.c`, and
`technique.c`.


## Validation

For each extraction slice:

1. Run `git diff --check`.
2. Run `just build`.
3. Run the narrowest affected scene tests, for example `just test scene` or a focused test filter.
4. For Vulkan/runtime graph extraction, prefer a validation-layer smoke path when practical.

If a tool is unavailable or impractical in the active environment, record that explicitly in the
handoff note.
