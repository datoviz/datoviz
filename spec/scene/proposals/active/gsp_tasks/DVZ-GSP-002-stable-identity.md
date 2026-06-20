# DVZ-GSP-002: Stable Scene Identity

## Goal

Make GSP able to map protocol objects to Datoviz scene objects and back from query results and
diagnostics using public C APIs only.

Consultation result: use the hybrid RC1 path. Datoviz exposes scene-local Datoviz ids; GSP owns all
protocol ids and adapter maps. Do not add Datoviz `user_id`/protocol-id setters for RC1.

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

Add a public id type:

```c
typedef uint64_t DvzId;

#define DVZ_ID_NONE UINT64_C(0)
```

Minimum useful API:

```c
DVZ_EXPORT DvzId dvz_scene_id(const DvzScene* scene);
DVZ_EXPORT DvzId dvz_figure_id(const DvzFigure* figure);
DVZ_EXPORT DvzId dvz_panel_id(const DvzPanel* panel);
DVZ_EXPORT DvzId dvz_visual_id(const DvzVisual* visual);
DVZ_EXPORT DvzId dvz_scene_buffer_id(const DvzSceneBuffer* buffer);
DVZ_EXPORT DvzId dvz_sampled_field_id(const DvzSampledField* field);
```

Consider adding getters for other public retained objects only when they can appear in query,
selection, diagnostics, or adapter maps:

```c
DVZ_EXPORT DvzId dvz_scale_id(const DvzScale* scale);
DVZ_EXPORT DvzId dvz_colormap_id(const DvzColormap* colormap);
DVZ_EXPORT DvzId dvz_colorbar_id(const DvzColorbar* colorbar);
DVZ_EXPORT DvzId dvz_legend_id(const DvzLegend* legend);
DVZ_EXPORT DvzId dvz_text_id(const DvzText* text);
DVZ_EXPORT DvzId dvz_annotation_id(const DvzAnnotation* annotation);
DVZ_EXPORT DvzId dvz_controller_id(const DvzController* controller);
```

Do not give ids to POD values copied into visuals. For example, do not add a transform id unless
there is a public retained transform handle.

Public id contract:

1. fixed-width `uint64_t`;
2. `DVZ_ID_NONE` is zero;
3. nonzero for live objects;
4. stable for the lifetime of that Datoviz object;
5. scene-local, not globally unique across scenes;
6. suitable for query, selection, diagnostics, and adapter maps;
7. independent from DRP2 ids and backend ids;
8. not a pointer, slot index, Vulkan/vklite/canvas/runtime handle, or GSP protocol id;
9. not persistent across scene destruction, process restart, serialization, or replay unless a
   future API says so.

Implementation must not expose current one-based retained-array slot indices as the public id
contract. Prefer per-scene monotonic ids assigned at object creation. Slot plus generation is
acceptable only if the bit layout remains private. Slot-only ids are rejected because stale async
query results could resolve to a newly-created object after slot reuse.

GSP adapter mapping remains:

```text
GSP protocol id -> Datoviz handle
Datoviz DvzId    -> GSP protocol id
```

## Tests/Validation

1. Public id getters return ids matching query result `visual_id`/`panel_id`.
2. Destroy/recreate behavior proves stale ids cannot resolve to a different live object.
3. Selection/hover paths still resolve query results correctly.
4. Query result ids remain Datoviz ids, not backend or GSP ids.
5. Run `git diff --check`.

## Stop Conditions

1. Implementing ids would require adding Datoviz user/protocol-id setters.
2. Public ids cannot be made stable without changing retained object allocation semantics.
3. Diagnostics need structured subject records before identity can be meaningfully consumed.
