> **Execution Status**
> - **Status:** `ACTIVE ASSET BOUNDARY DESIGN NOTE`
> - **Updated on:** `2026-05-09`
> - **Purpose:** define the intended v0.4 boundary between imported/authored assets, scene-owned
>   runtime resources, and exported semantic outputs.

# Asset Boundary Design

This note records the active v0.4 direction for import/export and asset ownership so mesh, image,
text, volume, and annotation work do not grow incompatible assumptions.


## Objective

Clarify the boundary between:

1. authored or imported source assets,
2. CPU-side reusable Datoviz objects,
3. scene-owned uploaded resources,
4. exported outputs such as screenshots, data readouts, and future scene serialization.


## Existing Grounding In The Repo

Useful current context:

1. active geometry note:
   [agents/now/GEOM_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/agents/now/GEOM_DESIGN.md)
2. active resource-update note:
   [agents/now/RESOURCE_UPDATE_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/agents/now/RESOURCE_UPDATE_DESIGN.md)
3. active volume note:
   [agents/now/VOLUME_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/agents/now/VOLUME_DESIGN.md)
4. completed offscreen/export context:
   [agents/done/OFFSCREEN.md](/home/cyrille/GIT/Viz/datoviz/agents/done/OFFSCREEN.md)

This note narrows what should be kept distinct in the active implementation path.


## Core Recommendation

Datoviz should keep a strict split between source assets, reusable semantic objects, and runtime
realization resources.

Recommended layers:

1. import/authored asset layer,
2. CPU-side semantic object layer,
3. scene-owned runtime resource layer,
4. export/readback layer.


## Import Layer

Imported files or externally authored data should remain one step removed from scene runtime state.

Examples:

1. OBJ mesh import,
2. image file load,
3. volume dataset metadata and sampled field load,
4. font file load,
5. future annotation layout or region tables from external tooling.

Recommended rule:

1. import produces CPU-side Datoviz-owned objects or value tables,
2. import does not directly create backend-native resources,
3. import format details should not leak into scene-facing render APIs.


## CPU-Side Semantic Objects

The CPU layer is where Datoviz should keep reusable semantic meaning.

Examples:

1. `DvzGeometry` or equivalent mesh geometry object,
2. image/value arrays plus semantic extent metadata,
3. volume sampled field plus scientific coordinate metadata,
4. text runs and font references,
5. annotation or selection identity tables.

Recommended rule:

1. these objects are the bridge between file formats and scene/runtime resources,
2. they should be inspectable, reusable, and updatable without immediate runtime upload,
3. they should own the stable ordering and metadata needed for picking, selection, and export.


## Scene-Owned Runtime Resources

Scene resources are uploaded realizations of semantic objects, not the source of truth.

Recommended rule:

1. scene-owned resources may be rebuilt, partially updated, or duplicated across panels,
2. runtime layout choices remain internal,
3. public APIs refer to logical resources rather than imported file handles or Vulkan-native
   objects.


## Stable Identity Across The Boundary

Import/export pressure often breaks identity unless the boundary is explicit.

Recommended requirement:

1. stable item, face, pixel, or sample ordering should be defined at the semantic-object layer,
2. scene resource updates preserve that ordering unless explicitly replaced,
3. export/readback paths refer back to semantic identities rather than runtime encodings,
4. picking, selection, probe/readout, and semantic export should all route back through this same
   semantic-identity layer rather than inventing separate runtime-facing id systems.

This matters directly for picking, selection, probe readout, and reproducible annotations.


## Export Layer

Export should cover more than screenshots.

Useful export/result classes to reserve now:

1. image or video capture,
2. picked identity and semantic coordinate readout,
3. sampled-value probe payloads,
4. selection sets,
5. future scene or panel state serialization.

Recommended rule:

1. export objects should be described in semantic terms,
2. runtime-specific staging, readback, and encoding mechanics stay below that boundary,
3. screenshot or video capture should remain distinct from semantic export/readout payloads even
   when both originate from one interaction workflow.


## Relationship To Text

Text especially benefits from a clean boundary.

Recommended split:

1. imported font files and shaping inputs stay in the import/semantic layer,
2. glyph runs and layout remain semantic objects,
3. atlas textures are runtime resources,
4. text export or annotation serialization should refer to semantic text content and style, not
   atlas placement.


## Relationship To Volume And Image

Image and volume work need explicit metadata ownership.

Recommended rule:

1. image semantic extent, units, and axis meaning stay above the runtime resource layer,
2. volume scientific bounds, orientation, and sampled-value meaning stay above the runtime resource
   layer,
3. export/readback should recover those semantics even when rendering used normalized or downcast
   coordinates internally.


## Relationship To External UI

External UI should mutate semantic or scene-owned objects, not imported assets directly.

Examples:

1. a property panel changes scale range,
2. a slice slider changes retained slice state,
3. a mesh visibility tree changes scene object state,
4. an import browser loads a new semantic asset and then binds it into the scene.


## What Not To Freeze

Avoid freezing the wrong boundaries now.

Do not:

1. make file formats part of the visual API,
2. make runtime buffer/texture layout part of exported semantics,
3. assume every visual is backed by one imported file,
4. conflate screenshot export with semantic data export.


## Public API Direction

The exact names can still move, but the conceptual API should support:

1. import helpers that produce Datoviz-owned semantic objects,
2. explicit scene resource creation from those semantic objects,
3. retained updates at the semantic or resource layer where appropriate,
4. export/readback objects that reference semantic identities and metadata.


## Immediate Scope Recommendation

The narrowest useful active implementation target is:

1. keep `geom`, image, and volume semantic objects distinct from uploaded scene resources,
2. preserve stable semantic identity for picking and selection across resource updates,
3. keep screenshot/capture export separate from semantic readout/export payloads,
4. leave room for future scene-state serialization without turning runtime resources into the source
   of truth.


## Explicit Non-Goals For The First Slice

1. specifying every import format,
2. designing a full scene file format now,
3. solving long-term asset packaging and distribution,
4. exposing backend-native export handles in the public scene API.
