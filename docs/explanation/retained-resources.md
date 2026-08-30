# Retained resources

Datoviz keeps scene data and derived resource descriptions between frames. An application can therefore update colors, move a camera, or redraw a panel without recreating the visualization and all of its GPU objects. This is the **retained resource model**.

**Audience:** developers optimizing updates or changing resource preparation. **Prerequisite:** the
[scene building blocks](figure-panel-visual-model.md). For public copy/borrow rules, use
[Objects and lifetimes](../reference/objects-and-lifetimes.md).


## Three kinds of state

| State | Examples | Authority and lifetime |
| --- | --- | --- |
| Authored scene state | Visual attributes, sampled fields, styles, controller state, domains. | Scene-owned after a successful copying call; changed only by later scene mutations. |
| Persistent derived state | Normalized coordinates, prepared family geometry, atlas/texture data, parameter blocks. | Reusable cache derived from authored state; invalidated by its dependencies. |
| Frame-local state | Transient render targets, compute results without declared persistence, command encoders, readback staging. | Exists for one planned/executed frame unless a contract promotes it to persistent state. |

Concrete GPU buffers, textures, pipelines, and descriptors belong to the runtime. They may persist
across frames, but they are caches of logical scene/runtime identity rather than the authoritative
scientific data.


## Copy, borrow, and external storage

Ordinary visual set-data calls copy the supplied array before returning; user code may then reuse or
free its input. A borrowed pointer, mapped readback, external GPU buffer, packet span, or host handle
has a narrower explicit lifetime. Never generalize a borrowing rule from one API to another.

An external resource also changes synchronization responsibilities. Its contract must state who may
write, transition, submit, or destroy it and when Datoviz may consume it.


## Content, shape, and topology

These changes are not equivalent:

| Change | Typical consequence |
| --- | --- |
| Same-size subrange content update | Update the retained bytes and emit a ranged write. |
| Same logical resource, full content replacement | Update packet/write; persistent runtime identity may remain. |
| Item count or byte size changes | Recreate or resize storage; dependencies may need rebinding. |
| Texture dimensions, format, or usage change | Recreate the logical/runtime texture and dependent bindings. |
| Visual family, pass participation, or technique changes | Rebuild relevant plan topology and possibly pipelines/targets. |
| Camera or panzoom changes | Update panel-derived transforms and redraw; keep authored data and normalization. |

Stable logical ids let the planner and runtime tell “update this object” from “create a different
object.” They are not public backend handles.


## Sharing

Several panels or visuals may reference the same retained source or derived resource. Share only
when the semantic mapping, format, and lifetime agree. A panel-local transform should remain local;
changing one panel must not invalidate a shared normalized point array used by another.


## Sources of truth

- [Scene resource model](https://github.com/datoviz/datoviz/blob/main/spec/scene/pipeline/RESOURCE_MODEL.md)
- [Attribute sources and mutability](https://github.com/datoviz/datoviz/blob/main/spec/scene/pipeline/ATTRIBUTE_SOURCES.md)
- [Invalidation and caching](invalidation-and-caching.md)
- [GPU resource ownership](gpu-resource-ownership.md)
