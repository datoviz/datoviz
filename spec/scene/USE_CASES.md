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


## UC2: Dynamic Buffer Updates

Requirements:

1. stable visual identity,
2. CPU-side data changes every frame,
3. dirty-range uploads,
4. no unnecessary pipeline rebuild.


## UC3: Picking

Requirements:

1. panel-local picking target,
2. render-to-id path,
3. single-pixel readback or equivalent,
4. mapping back to visual/item identity.


## UC4: Offscreen Rendering

Requirements:

1. render to texture,
2. deterministic readback,
3. no dependence on onscreen window state.


## UC5: Compute-Assisted Visual

Requirements:

1. compute stage before render stage,
2. explicit resource handoff,
3. deterministic validation of unsupported compute paths.


## UC6: Browser Delivery

Requirements:

1. same logical scene state,
2. same DRP2 semantics,
3. no scene dependency on native-only handles or APIs.
