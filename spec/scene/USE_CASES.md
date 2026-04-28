# Scene Use Cases

These use cases should pressure-test the future scene and DRP2 boundaries.


## UC1: Static Plot In One Panel

Requirements:

1. one panel,
2. one camera,
3. one visual,
4. static vertex data,
5. one color target.

This is the minimum path that DRP2 and scene must express cleanly.

Likely family examples:

1. `primitive`
2. `pixel`
3. `point`

Likely resource classes:

1. `ItemTable`
2. optional `ParameterBlockResource`

Likely variant axes:

1. primitive topology for `primitive`
2. point versus pixel rendering path


## UC2: Dynamic Buffer Updates

Requirements:

1. stable visual identity,
2. CPU-side data changes every frame,
3. dirty-range uploads,
4. no unnecessary pipeline rebuild.

Likely family examples:

1. `point`
2. `marker`
3. `path`
4. `glyph`

Likely resource classes:

1. `ItemTable`
2. `GroupedItemTable`
3. `ParameterBlockResource`

Pressure on the spec:

1. family identity should remain stable while only resource contents change,
2. variant identity should not churn when only item data changes,
3. dirty tracking should isolate item updates from style or family changes.


## UC3: Picking

Requirements:

1. panel-local picking target,
2. render-to-id path,
3. single-pixel readback or equivalent,
4. mapping back to visual/item identity.

Likely family examples:

1. `point`
2. `marker`
3. `mesh`
4. `glyph`

Likely resource classes:

1. primary family data resource
2. picking-oriented `DerivedField`
3. `ReadbackTarget`

Pressure on the spec:

1. picking should be described semantically, not as a backend attachment trick,
2. families should declare whether picking is native, optional, or unsupported,
3. the mapping back to visual and item identity should be family-aware.


## UC4: Offscreen Rendering

Requirements:

1. render to texture,
2. deterministic readback,
3. no dependence on onscreen window state.

Likely family examples:

1. `image`
2. `mesh`
3. `sphere`
4. `volume`

Likely resource classes:

1. source family resources
2. offscreen `DerivedField`
3. `ReadbackTarget`

Pressure on the spec:

1. family semantics should not depend on onscreen presentation state,
2. export and inspection paths should remain scene-visible,
3. readback should compose with the same family and variant model used onscreen.


## UC5: Compute-Assisted Visual

Requirements:

1. compute stage before render stage,
2. explicit resource handoff,
3. deterministic validation of unsupported compute paths.

Likely family examples:

1. `mesh`
2. `sphere`
3. `volume`
4. future specialized `path` or `image` variants

Likely resource classes:

1. source resource such as `IndexedGeometry` or `SampledField`
2. compute-written `DerivedField`
3. render-consumed resource view of that derived output

Pressure on the spec:

1. compute participation should usually be a variant axis inside a family, not a separate family by
   default,
2. unsupported compute should map to a deterministic fallback or a clear planning diagnostic,
3. scene planning should make the handoff explicit without exposing backend synchronization details.


## UC6: Browser Delivery

Requirements:

1. same logical scene state,
2. same DRP2 semantics,
3. no scene dependency on native-only handles or APIs.

Pressure on the spec:

1. family names should remain semantic rather than backend-shaped,
2. variant axes should be capability-driven, not Vulkan-driven,
3. scene resource classes should not assume native-only object models,
4. volume slice semantics should not depend on explicit texture-handle vocabulary.


## UC7: Neuroanatomy Atlas Explorer

Requirements:

1. one 3D panel mixing transparent enclosing surfaces, interior region meshes, and one volume slice,
2. stable region identity that survives batching and is suitable for click selection,
3. scene-owned region visibility and opacity state driven by an external hierarchical UI,
4. panel-local 3D camera navigation with orbit and wheel zoom,
5. picking on both region meshes and the volume slice,
6. probe readout returning 3D coordinates plus sampled value,
7. one linked 2D panel that updates from slice state or probe state,
8. explicit capability-sensitive transparency behavior.

Likely family examples:

1. `mesh`
2. `volume`
3. `path`
4. `point`

Likely resource classes:

1. `IndexedGeometry`
2. `SampledField`
3. `ParameterBlockResource`
4. panel-local picking `DerivedField`
5. `ReadbackTarget`

Pressure on the spec:

1. mesh-family picking may need a stronger grouped-identity story for selectable region collections,
2. volume slice picking should be able to report semantic coordinates and sampled values,
3. scene state must remain separate from external UI widget structure while still being easy to
   mutate from it,
4. linked 2D subplot updates should compose with 3D interaction without unnecessary shared-resource
   churn,
5. transparency policy should remain capability-aware and explicit rather than implicit or backend-
   shaped.
