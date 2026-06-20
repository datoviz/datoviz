# DVZ-GSP-002: Stable Scene Identity

## Goal

Make GSP able to map protocol objects to Datoviz scene objects and back from query results and
diagnostics using public C APIs only.

## Files To Inspect/Change

| File | Reason |
|---|---|
| `include/datoviz/scene.h` | potential public id getter/setter declarations |
| `include/datoviz/scene/types.h` | id-bearing structs and query result fields |
| `src/scene/visuals/families.c` | `_scene_visual_public_id()` current behavior |
| `src/scene/interaction/hit_test.c` | `_scene_panel_public_id()` current behavior |
| `src/scene/core/frame_trace.c` | `_scene_figure_id()` current emitted figure ids |
| `src/scene/query/result.c` and `src/scene/query/execute.c` | query id population |
| `src/scene/interaction/core.c` | selection/hover lookup by public ids |
| `src/scene/tests/query.c` and `src/scene/tests/interaction.c` | existing identity assertions |
| `spec/scene/proposals/active/gsp_tasks/CHATGPT_PRO_CONSULTATION_001.md` | protocol-id ownership decision |

## Non-Goals

1. Do not expose backend resource ids as scene ids.
2. Do not make raw pointer addresses part of the public identity contract.
3. Do not add broad object metadata storage unless the GSP/user-id decision requires it.
4. Do not change existing query result field names without a separate migration decision.

## Implementation Notes

Minimum useful API:

```c
DVZ_EXPORT uint64_t dvz_panel_id(const DvzPanel* panel);
DVZ_EXPORT uint64_t dvz_visual_id(const DvzVisual* visual);
DVZ_EXPORT uint64_t dvz_scene_buffer_id(const DvzSceneBuffer* buffer);
DVZ_EXPORT uint64_t dvz_sampled_field_id(const DvzSampledField* field);
```

Potential API after consultation:

```c
DVZ_EXPORT bool dvz_visual_set_user_id(DvzVisual* visual, uint64_t id);
DVZ_EXPORT uint64_t dvz_visual_user_id(const DvzVisual* visual);
```

If Datoviz does not accept user/protocol ids, document that the GSP adapter owns protocol-id mapping
and Datoviz exposes only stable lifetime-local object ids.

Current internal ids are one-based retained-array indices. Before exposing them, document lifetime,
destroy/reuse, and cross-scene uniqueness rules.

## Tests/Validation

1. Public id getters return ids matching query result `visual_id`/`panel_id`.
2. Destroy/recreate behavior is tested or explicitly documented.
3. Selection/hover paths still resolve query results correctly.
4. If user ids are added, duplicate/zero/lifetime behavior is tested.
5. Run `git diff --check`.

## Stop Conditions

1. Choosing between Datoviz-owned user ids and adapter-owned protocol maps remains unresolved.
2. Public ids cannot be made stable without changing retained object allocation semantics.
3. Diagnostics need structured subject records before identity can be meaningfully consumed.
