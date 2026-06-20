# ChatGPT Pro Consultation 001: Stable Protocol Identity

## Exact Question To Ask ChatGPT Pro

Datoviz v0.4 is preparing for a future `gsp-backend-datoviz` adapter. GSP has stable protocol
object ids for scenes, figures, panels, visuals, resources, transforms, and query results. Datoviz
already returns scene-visible ids in `DvzQueryResult`, but the current internal visual/panel ids are
derived from retained object slot indices and are not exposed as public getters. The adapter could
also maintain its own protocol-id to Datoviz-handle maps.

Question: for Datoviz v0.4 RC1, should Datoviz expose only lifetime-local public id getters
(`dvz_visual_id()`, `dvz_panel_id()`, resource id getters) and leave GSP protocol ids entirely in
the adapter, or should Datoviz add optional user/protocol id fields and setters on retained scene
objects? Recommend the smallest API that makes GSP query/diagnostic round-trip robust without
polluting Datoviz with GSP-specific semantics.

Evaluate these options:

1. Datoviz exposes lifetime-local id getters only; GSP owns all protocol-id maps.
2. Datoviz exposes user-id setters/getters for panels, visuals, buffers, sampled fields, and maybe
   figures; query results include both Datoviz id and user id.
3. Datoviz exposes generic labels/resource keys but no numeric user ids.
4. Hybrid: id getters before RC1, user-id fields deferred until a concrete GSP adapter proves need.

Constraints:

1. Datoviz v0.4 does not preserve v0.3 API compatibility.
2. Scene API must remain backend-neutral and compatible with raw ctypes and future WASM/WebGPU.
3. Scene query results must return scene-visible identity, not backend identity.
4. Datoviz should not embed GSP or depend on GSP.
5. Public API added before RC1 is hard to reverse.
6. Avoid exposing Vulkan, vklite, canvas, swapchain, or private runtime handles.
7. Public structs should use fixed-width ids, POD descriptors/results, and explicit ownership rules.

## Context Files To Provide

1. `spec/scene/AUTHORITY.md`
2. `spec/scene/core/RUNTIME_BOUNDARY.md`
3. `spec/scene/interaction/PANEL_QUERY.md`
4. `spec/scene/interaction/GPU_QUERY_SYSTEM.md`
5. `spec/scene/proposals/active/GSP_COMPATIBILITY_PREP.md`
6. `spec/scene/proposals/active/GSP_COMPATIBILITY_AUDIT.md`
7. `include/datoviz/scene.h`
8. `include/datoviz/scene/types.h`
9. `include/datoviz/scene/interaction.h`
10. `src/scene/visuals/families.c`
11. `src/scene/interaction/hit_test.c`
12. `src/scene/query/result.c`
13. `src/scene/query/execute.c`

## Expected Output Format

1. Recommended option.
2. Rationale.
3. Exact Datoviz public API shape, if any.
4. Exact GSP adapter responsibility.
5. Query result and diagnostic implications.
6. RC1-safe subset.
7. Deferred v0.5 work.
8. Acceptance tests.
9. Risks and rejected alternatives.

## Work Blocked Until The Answer Exists

1. Adding public user/protocol id setter fields to Datoviz retained scene objects.
2. Adding user-id fields to `DvzQueryResult`.
3. Documenting GSP protocol-id ownership as final.

Not blocked:

1. Public Datoviz lifetime-local id getters if maintainers accept them as non-GSP-specific.
2. The capability query task.
3. Ctypes smoke coverage for existing query/capability structs.
4. Visual-family mapping documentation.
