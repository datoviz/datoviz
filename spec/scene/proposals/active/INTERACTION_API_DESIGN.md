> **Execution Status**
> - **Status:** `SCENE SPEC PROPOSAL`
> - **Updated on:** `2026-05-09`
> - **Purpose:** define the intended first scene-facing API shape for picking, hover, selection,
>   linked selection, probes, and pinned readouts.

# Interaction API Design

This note turns the active interaction behavior decisions into one focused scene-facing API shape.


## Objective

Define a coherent first API for:

1. pick requests,
2. hover state,
3. persistent selection,
4. linked selection through link channels,
5. probe/readout requests,
6. pinned readouts.


## Existing Grounding In Scene Proposals

This API note is a consolidation of:

1. [PICKING_DESIGN.md](../promoted/PICKING_DESIGN.md)
2. [SELECTION_HIGHLIGHT_DESIGN.md](SELECTION_HIGHLIGHT_DESIGN.md)
3. [PROBE_READOUT_DESIGN.md](../promoted/PROBE_READOUT_DESIGN.md)
4. [TRANSFORM_CONTROLLER_DESIGN.md](TRANSFORM_CONTROLLER_DESIGN.md)


## Core Recommendation

Keep the interaction API split into three layers:

1. capability declaration on visuals,
2. interaction policy objects,
3. retained interaction state and result objects.

Do not collapse all three into one ad hoc callback surface.


## Main Scene Objects

Recommended first-class scene-facing concepts:

1. interaction policy object
2. selection object
3. link channel
4. pinned readout object
5. pick result
6. probe result

Recommended ownership:

1. scene owns policies, channels, and retained interaction objects,
2. panels use policies to interpret input,
3. visuals declare what identities they can expose and optionally attach to selection/link state.


## Visual Capability Surface

Visuals should declare what interaction identities they support.

Conceptual API shape:

```text
dvz_visual_set_pick_capabilities(visual, caps)
dvz_visual_set_link_keys(visual, channel, mapping)
dvz_visual_set_selection(visual, selection)
```

Where `caps` conceptually covers:

1. object-level picking support
2. face-level picking support
3. item-level picking support
4. pixel/sample picking support
5. grouped parent/sub-primitive picking support


## Interaction Policy Surface

Interaction policy should be explicit and reusable.

Conceptual API shape:

```text
DvzInteractionPolicy* policy = dvz_interaction(scene);
dvz_interaction_bind_panel(policy, panel);
dvz_interaction_set_pick_policy(policy, ...);
dvz_interaction_set_selection_policy(policy, ...);
dvz_interaction_set_probe_policy(policy, ...);
dvz_interaction_set_input(policy, gesture, semantics);
```

What policy should control:

1. object versus face mesh selection
2. object/group/item grouped-primitive selection
3. probe-only versus persistent sample selection
4. linked hover enable/disable
5. linked selection enable/disable
6. active link channel
7. clear-on-miss versus retain probe behavior
8. whether persistent sample selection auto-creates pinned readouts


## Pick Request Surface

Pick requests should stay explicit and panel-oriented.

Conceptual API shape:

```text
dvz_panel_pick(panel, x, y, request_flags)
dvz_scene_poll_pick_result(scene, &result)
```

Coordinates stay in logical panel/window units.

Multi-hit should fit the same API family:

```text
dvz_panel_pick_hits(panel, x, y, request_flags)
dvz_scene_poll_pick_hits(scene, &hits)
```


## Pick Result Shape

Recommended conceptual `DvzPickResult` fields:

1. panel id
2. visual id
3. optional instance id
4. optional parent payload kind/id
5. raw payload kind/id
6. resolved payload kind/id
7. optional semantic/world coordinate later

This keeps raw and resolved identities both visible.


## Hover State Surface

Hover should be retained scene state with both raw and resolved identity.

Conceptual API shape:

```text
const DvzHoverState* hover = dvz_scene_hover(scene, panel);
```

Recommended fields:

1. raw hover hit
2. resolved hover target
3. optional active link channel
4. optional current probe/readout payload derived from hover


## Selection Surface

Selection should be a retained scene-owned object.

Conceptual API shape:

```text
DvzSelection* sel = dvz_selection(scene, desc);
dvz_selection_set_mode(sel, mode);
dvz_selection_clear(sel);
dvz_selection_get(sel, &items, &count);
dvz_selection_set_input(sel, gesture, semantics);
```

Selection modes:

1. replace
2. additive
3. subtractive
4. toggle

Selection contents should store resolved targets, not raw hits.


## Link Channel Surface

Linked selection should be channel-based and scene-owned.

Conceptual API shape:

```text
DvzLinkChannel* channel = dvz_link_channel(scene, "region");
dvz_interaction_set_active_link_channel(policy, channel);
```

Recommended first-slice rules:

1. one active link channel at a time
2. each local identity has zero or one key per channel
3. many local identities may share one key
4. changing active channel does not reinterpret existing selection retroactively


## Probe Surface

Probe/readout should have one coherent API family, not a separate bespoke path per visual type.

Conceptual API shape:

```text
dvz_panel_probe(panel, x, y, probe_flags)
dvz_scene_poll_probe_result(scene, &probe)
```

Recommended conceptual `DvzProbeResult` fields:

1. panel id
2. visual id
3. payload kind/id
4. semantic coordinate
5. structured values
6. optional formatted display string
7. optional units


## Pinned Readout Surface

Pinned readouts should be retained scene-owned objects.

Conceptual API shape:

```text
DvzPinnedReadout* rd = dvz_pinned_readout(scene, &probe_result);
dvz_pinned_readout_destroy(rd);
dvz_pinned_readout_set_format(rd, ...);
```

Recommended first-slice behavior:

1. multiple pinned readouts allowed
2. pinned readout stores raw originating hit when available
3. pinned readout stores resolved semantic payload
4. pinned readout is passive by default
5. optional policy may allow clicking it to reselect/reprobe


## Grouped Primitive API Implication

Grouped primitive selection policy must be overridable per visual instance.

Conceptual surface:

```text
dvz_visual_set_interaction_granularity(visual, mode)
```

Where `mode` may conceptually choose:

1. object
2. group
3. item/sub-primitive


## Instanced Mesh API Implication

Object-level interaction on instanced meshes should resolve per instance.

Conceptual rule:

1. object target = `visual id + optional instance id`
2. face target = `visual id + instance id + face id`

Do not let object-level selection on one instance silently mean “the whole visual”.


## Example Conceptual Flow

```text
policy = dvz_interaction(scene)
channel = dvz_link_channel(scene, "region")
sel = dvz_selection(scene, &desc)

dvz_interaction_bind_panel(policy, panel)
dvz_interaction_set_active_link_channel(policy, channel)
dvz_interaction_set_selection_policy(policy, DVZ_SELECT_FACE_IF_ENABLED)
dvz_interaction_set_probe_policy(policy, DVZ_PROBE_CLEAR_ON_MISS)

dvz_visual_set_pick_capabilities(mesh, caps)
dvz_visual_set_selection(mesh, sel)
dvz_visual_set_link_keys(mesh, channel, face_region_keys)

dvz_panel_pick(panel, x, y, flags)
dvz_scene_poll_pick_result(scene, &pick)
dvz_scene_poll_probe_result(scene, &probe)
```


## Immediate Scope Recommendation

The narrowest useful first API slice is:

1. per-visual pick capability declaration
2. one interaction policy object bound per panel
3. retained selection object
4. retained link channels
5. one pick result object with raw/resolved identity
6. one probe result object
7. multiple retained pinned readouts


## Explicit Non-Goals For The First Slice

1. callback-only interaction as the primary model
2. backend-native ids or handles in public interaction types
3. multi-channel linked selection in one action
4. custom reducer callbacks in the first implementation
