# Shared visual data API note

This note records a future scene/visual API addition exposed by the LIDAR split-screen discussion:
several visuals should be able to consume the same large attribute payload without forcing users to
manually create and bind one `DvzSceneBuffer` per attribute.

The current API already has two useful lower-level pieces:

1. the same `DvzVisual*` can be attached to several panels, which is ideal when only panel state
   differs, for example raw versus EDL postprocessing;
2. `dvz_visual_set_attr_buffer()` lets different visuals bind the same scene-owned buffer, but it is
   intentionally low-level and requires manual buffer descriptors, strides, usage flags, and one
   binding call per attribute.

The missing layer is a small public object that owns a named collection of compatible visual
attributes and can be bound to one or more visuals.


## Use Cases

1. A 10M-point LIDAR cloud rendered as raw pixels in one panel and as styled/filtered points in
   another without duplicating position/color/size buffers.
2. The same point table rendered through different visual families, for example `pixel`, `point`,
   `marker`, or a future splat visual.
3. Multi-panel dashboards where each panel uses the same geometry but different camera, scale,
   color, postprocessing, or selection state.
4. Raw versus highlighted overlays where the highlighted visual uses a subset or alternate color
   source but shares position.
5. External/GPU producer workflows, such as CUDA/CuPy interop, where a large buffer should be
   registered once and reused by several visuals.


## Terminology Options

### `DvzSceneData`

Pros:

1. simple and user-friendly;
2. broad enough for positions, colors, sizes, group ids, and future table-like sources;
3. reads well in examples: "create scene data, bind it to visuals".

Cons:

1. "data" is very broad and may overlap conceptually with files, datasets, and general app data;
2. does not immediately communicate that the object is mainly a bundle of visual attributes.


### `DvzAttributeStore`

Pros:

1. precise: the object stores named attributes;
2. aligns with the existing attribute-source design (`PER_ITEM`, `PER_GROUP`, `CONSTANT`, etc.);
3. makes it clear that this is lower than a visual but higher than raw buffers.

Cons:

1. slightly internal-sounding;
2. "store" may imply database-like ownership or mutability semantics beyond what is needed.


### `DvzAttributeSet`

Pros:

1. concise;
2. emphasizes a coherent set of attributes with shared item count/topology;
3. fits the intended binding operation: a visual uses an attribute set.

Cons:

1. "set" can imply mathematical uniqueness rather than named slots;
2. less explicit about ownership/lifetime than "store" or "data".


### `DvzVisualData`

Pros:

1. clearly scoped to visual consumption;
2. easy to explain as "data shared by visuals";
3. avoids confusion with scene-level resources unrelated to rendering.

Cons:

1. may sound visual-owned even though the object should be scene-owned and reusable;
2. less suitable if the object later supports selection/link keys or derived metadata that are not
   strictly visual attributes.


### `DvzGeometry`

Pros:

1. familiar term in graphics APIs;
2. good for positions, normals, indices, and mesh-like resources.

Cons:

1. too narrow for colors, sizes, markers, group ids, and non-geometric attribute sources;
2. misleading for image/volume or table-like point-cloud data.


## Recommended Term

Prefer **`DvzAttributeSet`** for the public C API, with documentation describing it as a
scene-owned named visual-attribute bundle.

Rationale:

1. the object is not a general dataset; it exists to bind visual attributes;
2. it should compose with the existing attribute-source vocabulary;
3. it leaves `DvzSceneData` available for a higher-level future object that might represent loaded
   files, tables, provenance, or non-rendering data;
4. it is short enough for public C functions without becoming unreadable.

Use "attribute set" in docs when discussing user-level concepts. Use "scene buffer" only for the
lower-level resource objects that actually back uploaded bytes.


## Proposed API Shape

Keep `dvz_visual_set_data()` as the simple one-visual convenience path.

Add a reusable attribute bundle path:

```c
DvzAttributeSet* dvz_attribute_set(DvzScene* scene, uint32_t item_count);

bool dvz_attribute_set_data(
    DvzAttributeSet* attrs, const char* attr_name,
    const DvzAttributeDesc* desc, const void* data);

bool dvz_attribute_set_buffer(
    DvzAttributeSet* attrs, const char* attr_name,
    const DvzAttributeDesc* desc, DvzSceneBuffer* buffer);

bool dvz_visual_use_attributes(DvzVisual* visual, DvzAttributeSet* attrs);
```

The descriptor should carry semantic information, not GPU binding details:

```c
typedef struct DvzAttributeDesc
{
    DvzVisualAttrSource source;
    DvzFormat format;
    DvzVisualAttrMutability mutability;
    uint32_t stride;
} DvzAttributeDesc;
```

Open naming questions:

1. `dvz_visual_use_attributes()` versus `dvz_visual_bind_attributes()`.
   Prefer `use` for public ergonomics unless the rest of the scene API standardizes on `bind`.
2. `DvzAttributeSet` versus `DvzVisualAttributes`.
   Prefer `DvzAttributeSet` because the object is scene-owned and can be used by several visuals.
3. `DvzAttributeDesc` versus `DvzVisualAttributeDesc`.
   Prefer `DvzAttributeDesc` if it remains in the scene visual namespace; use the longer name if
   public headers need disambiguation.


## Semantics

The attribute set should be a thin owner/registry over `DvzSceneBuffer` resources, not a parallel
resource system.

1. Each named attribute may be backed by owned CPU data, an owned scene buffer, or an external scene
   buffer.
2. Binding an attribute set to a visual validates that the visual family accepts every required
   attribute name, source, format, stride, and item count.
3. One visual may override specific attributes after binding an attribute set, for example sharing
   `position` but using a panel-specific or visual-specific `color`.
4. Dirty tracking stays at the backing buffer/attribute level so one upload services all visuals
   using that data.
5. The first implementation should support `PER_ITEM` attributes only; `CONSTANT`, `PER_GROUP`, and
   `PER_SPAN` can follow the existing attribute-source plan.


## Non-Goals For The First Slice

1. Do not replace `dvz_visual_set_data()`; keep it as the simple API.
2. Do not require this object for ordinary one-off visuals.
3. Do not make users specify Vulkan/DRP2 binding slots.
4. Do not make it a file/dataset/provenance abstraction; prepared example data remains outside the
   scene runtime API.


## Suggested First Slice

Implement the minimal path needed for shared large point clouds:

1. add `DvzAttributeSet` as a scene-owned object;
2. support `PER_ITEM` `position`, `color`, and `size`;
3. lower each attribute to a `DvzSceneBuffer`;
4. let two `DvzVisual*` objects bind the same attribute set;
5. add a scene test that verifies one upload/resource id per shared attribute and two draw calls;
6. update the future LIDAR split-screen example to use the shared attribute set only if the two
   panels need distinct visuals. If one visual attached to two panels is enough, keep the example on
   the simpler path.

