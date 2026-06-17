# Retained Resources

Retained resources are the state Datoviz keeps between frames so user code does not have to rebuild
the renderer every time something changes.

A visual keeps retained attribute data after a successful set-data call. A sampled field keeps the
texture or buffer-shaped data needed by image, color-mapped, or probe workflows. A controller keeps
view state. A figure and panel keep layout, viewport, and transform state. These objects are scene
objects first; GPU resources are derived from them.

Retained state has two important consequences:

- updating data should usually update an existing object instead of destroying and recreating it;
- the runtime can reuse buffers, textures, pipelines, and descriptor-shaped state when the resource
  shape and render contract remain compatible.

Data content changes are cheaper than shape changes. Replacing point positions with the same item
count can usually become an update stream. Growing the item count, changing texture dimensions,
changing visual family, or changing material/technique state may require setup work because buffers,
textures, or pipelines need different shapes.

Ownership stays at the scene level. User arrays only need to remain valid until the set-data call
returns, unless a specific API documents borrowed lifetime. After that, Datoviz owns its retained
copy or retained resource description. Runtime GPU handles are implementation details unless the API
explicitly exposes borrowed interop.

Retained resources should be invalidated narrowly. A color array update should not rebuild a panel
layout. A controller pan should not recreate a texture. A texture size change should refresh the
texture resource and dependent bindings, not the whole scene.

See also:

- [Invalidation and caching](invalidation-and-caching.md)
- [Frame lifecycle](frame-lifecycle.md)
- [Objects and lifetimes](../reference/objects-and-lifetimes.md)
