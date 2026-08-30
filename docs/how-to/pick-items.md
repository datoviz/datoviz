# Pick items

Map a pointer location to a rendered item or scene value.

Use picking when the user points at rendered geometry and the application needs the item, instance,
face, or primitive that was drawn there. Use probing when the user needs a sampled field value at a
data coordinate.

!!! info "At a glance"

    - **Status:** Supported GPU-backed queries; promoted picking examples run natively and in WebGPU.
    - **Languages:** C-first asynchronous workflow; Python exposes exact query descriptors and polling calls.
    - **Prerequisites:** A query-capable visual, rendered frames, and outer-panel-local pointer coordinates.
    - **Result:** A later poll returns the frontmost rendered target and its visual-local or linked id.

## Task workflow

Use picking when the target is a rendered item, instance, or primitive. Use probing when the target
is a sampled field value at a data coordinate.

![Marker items with the item under the pointer identified by picking](../assets/gallery/v0.4/features/features_picking.webp)

## Minimal workflow

1. Enable query support on the visual with `dvz_visual_set_query_capabilities()`.
2. Convert pointer input to outer-panel-local logical pixel coordinates.
3. Queue a panel query with `dvz_panel_query_px()`.
4. Poll resolved results with `dvz_scene_poll_query()` after subsequent frames.
5. Map `resolved_id` or `link_key` back to application data.
6. Update hover, selection, or readout state from the query result.

Keep the visual's item order stable if the pick result is used as an index into application data.

The code below is an asynchronous C excerpt. Check every mutator/queue result, render subsequent
frames, and keep polling rather than waiting synchronously in the input callback.

```c
dvz_visual_set_query_capabilities(visual, DVZ_QUERY_CAPABILITY_ITEM);

DvzQueryRequest request = dvz_query_request();
request.request_id = 1;
request.target = DVZ_SCENE_TARGET_ITEM;
request.hit_policy = DVZ_QUERY_HIT_FRONTMOST;

dvz_panel_query_px(panel, panel_x, panel_y, &request);

DvzQueryResult result = {0};
while (dvz_scene_poll_query(scene, &result))
{
    if (result.request_id != 1)
        continue;
    if (result.status == DVZ_QUERY_STATUS_HIT && result.hit &&
        result.resolved_target == DVZ_SCENE_TARGET_ITEM)
    {
        uint64_t item_id = result.resolved_id;
        /* Map item_id to application data, selection state, or a readout. */
    }
}
```


## Pointer coordinates

Queries use `DVZ_PANEL_COORD_PANEL_PX` coordinates: logical pixels local to the outer panel
rectangle, not raw window coordinates. When the pointer event comes from the scenario runner, use
the scenario helper shown in `examples/c/features/picking.c`. In a direct app or hosted
integration, first translate host-window coordinates to figure coordinates with
`dvz_figure_window_to_layout()` when needed, then convert figure to panel pixels with
`dvz_panel_transform_point()` before calling `dvz_panel_query_px()`.

When the application already has a data-coordinate point, use `dvz_panel_query_data()` or convert it
with `dvz_panel_data_to_position()`.


## Result handling

| Field | Use |
| --- | --- |
| `status` and `hit` | Check whether the query resolved to a rendered target. |
| `visual_family` and `visual_id` | Confirm the hit came from the expected visual family or visual. |
| `resolved_target` | Confirm whether the answer is an item, primitive, face, texel, or other target. |
| `resolved_id` | Use as the visual-local item or primitive id when item order is stable. |
| `link_key` | Use for application ids when the visual has explicit link keys. |
| `has_data_position` and `data_position` | Use for readouts when the visual can report data-space position. |

Use `request_id` to distinguish hover queries from click queries when both are active. The examples
use one request id for hover and another for click selection.


## Hover and selection

For retained hover or selection styling, apply successful query results to the retained interaction
objects instead of rewriting visual attributes manually:

```c
DvzHover* hover = dvz_hover(scene, NULL);
DvzSelection* selection = dvz_selection(scene, NULL);

dvz_hover_apply_query(hover, &result);
dvz_selection_apply_query(selection, &result);
```

Use [Select and highlight data](select-items.md) for selection modes, styles, and clearing rules.


## Important details

Picking is tied to what is rendered. Hidden, clipped, transparent, or depth-tested items may not
behave like a CPU-side nearest-neighbor search.

Panel queries are GPU-backed and normally resolve after rendering work has advanced. Queue the
request from input or frame code, then consume results from the scene polling path.

Query result storage is caller-owned. Hover and selection objects retain their own state after
`dvz_hover_apply_query()` or `dvz_selection_apply_query()`; destroy those objects before destroying
their scene if you remove them early.

## Common mistakes

- Using picking to read image scalar values; use field probing.
- Reordering visual data without updating the application-side id mapping.
- Expecting identical results from native and WebGPU paths without checking feature status.
- Forgetting to enable query capabilities on the visual before issuing item queries.
- Treating query results as immediate return values instead of polling resolved results.
- Using raw window coordinates instead of outer-panel-local logical pixels.

## See also

- [Probe image or field values](probe-fields.md)
- [Select and highlight data](select-items.md)
- [Handle input events](input-events.md)

??? example "Complete and related examples"

    - Canonical complete example: [Picking](../examples/gallery/features/features_picking.md) - Source: `examples/c/features/picking.c`
    - [Pixel Selection](../examples/gallery/features/features_selection_pixel.md) - Source: `examples/c/features/selection_pixel.c`
    - [Label Probe](../examples/gallery/features/features_probe_labels.md) - Source: `examples/c/features/probe_labels.c`
