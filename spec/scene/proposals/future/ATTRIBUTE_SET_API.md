# Attribute Set API

Status: future proposal distilled from the former shared visual data backlog.

The goal is a small scene-owned object that lets several visuals share one named collection of large
visual attributes without forcing users to manage one low-level scene buffer per attribute.


## Use Cases

1. A 10M-point LiDAR cloud shown as raw pixels in one panel and styled points or splats in another.
2. The same point table rendered through `pixel`, `point`, `marker`, or `splat`.
3. Multi-panel dashboards sharing geometry but changing camera, color scale, postprocess, or
   selection state.
4. Highlight overlays sharing positions while overriding color or selection attributes.
5. External/GPU producer workflows where one registered buffer is reused by several visuals.


## Preferred Concept

Prefer `DvzAttributeSet`: a scene-owned named visual-attribute bundle.

Rationale:

1. it is lower than a visual and higher than raw buffers;
2. it aligns with the existing attribute-source vocabulary;
3. it does not claim to be a general dataset, file, or provenance object;
4. it leaves `DvzSceneData` free for a broader future data abstraction.


## API Sketch

Keep `dvz_visual_set_data()` as the simple one-visual path.

Potential shared path:

```c
DvzAttributeSet* dvz_attribute_set(DvzScene* scene, uint32_t item_count);

bool dvz_attribute_set_data(
    DvzAttributeSet* attrs, const char* name, const DvzAttributeDesc* desc, const void* data);

bool dvz_attribute_set_buffer(
    DvzAttributeSet* attrs, const char* name, const DvzAttributeDesc* desc, DvzSceneBuffer* buffer);

bool dvz_visual_use_attributes(DvzVisual* visual, DvzAttributeSet* attrs);
```

`DvzAttributeDesc` should carry semantic facts, not Vulkan/DRP2 binding slots:

```c
typedef struct DvzAttributeDesc
{
    DvzVisualAttrSource source;
    DvzFormat format;
    DvzVisualAttrMutability mutability;
    uint32_t stride;
} DvzAttributeDesc;
```


## Semantics

1. Each named attribute may be backed by owned CPU data, an owned scene buffer, or an external scene
   buffer.
2. Binding validates that the visual family accepts each required name, source, format, stride, and
   item count.
3. A visual may override individual attributes after binding the set.
4. Dirty tracking stays at the backing buffer or attribute level so one upload can serve all
   consumers.
5. The first slice supports only `PER_ITEM` attributes.


## First Slice

1. Add `DvzAttributeSet` as a scene-owned object.
2. Support `PER_ITEM` `position`, `color`, and size-like attributes.
3. Lower each attribute to one `DvzSceneBuffer`.
4. Let two visuals bind the same set.
5. Test one upload/resource id per shared attribute and multiple draw calls.


## Non-Goals

1. Do not replace `dvz_visual_set_data()`.
2. Do not require attribute sets for ordinary one-off visuals.
3. Do not expose Vulkan, DRP2, or backend binding slots.
4. Do not make this a file, dataset, table, or provenance abstraction.
