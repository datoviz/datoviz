# DVZ-GSP-005: Visual Family Mapping

## Goal

Publish a Datoviz-owned visual-family table that lets a GSP adapter map semantic visual families to
public Datoviz constructors, attributes, resources, styles, coordinate spaces, query capabilities,
and known unsupported behavior without reading private source.

## Files To Inspect/Change

| File | Reason |
|---|---|
| `include/datoviz/scene.h` | constructors, attribute docstrings, style/material functions |
| `include/datoviz/scene/field.h` | image/labels/volume sampled-field resource mapping |
| `include/datoviz/scene/scale.h` | scale/colormap mapping |
| `include/datoviz/scene/types.h` | descriptors and query result payload fields |
| `src/scene/visuals/*/api.c` | actual family API behavior |
| `src/scene/visuals/*/query.c` | query capability and payload behavior |
| `src/scene/tests/visuals/` | visual-family render/emission tests |
| `src/scene/tests/query.c` | per-family query coverage |
| `examples/c/visuals/` | public usage examples |
| `spec/scene/visuals/STATUS.md` | existing family status summary |

## Non-Goals

1. Do not expose private visual internals as public API.
2. Do not mark a visual/query payload supported just because a field exists in `DvzQueryResult`.
3. Do not preserve v0.3 visual names when they conflict with v0.4 architecture.
4. Do not add GSP-specific constructors.

## Implementation Notes

Start with this table shape:

| Family | Constructor | Required data | Optional data/resources | Query support | GSP first-slice status |
|---|---|---|---|---|---|
| point | `dvz_point()` | `position`, `color`, `diameter` | `item_state`, link keys | item id, visual id, displayed color where available | ready |
| image | `dvz_image()` | sampled field plus placement attributes | scalar/color fields, scale/colormap | pixel/sample/image payloads, exact fields documented per profile | ready with limits |
| primitive | `dvz_primitive()` | `position`, `color` | `normal`, index buffer | item/primitive currently limited | usable |
| mesh | `dvz_mesh()` | `position` | color, normal, texcoords, instance transform, index, texture | item/mesh identity; face/region payload pending | usable with limits |
| volume | `dvz_volume()` | 3D sampled field | render mode, slice, value range, transfer | slice sample first; MIP/DVR unsupported | limited |
| text/glyph | `dvz_text`/`dvz_glyph()` | text/glyph attributes and atlas | font/text placement | query deferred | render-only first |

Place the durable table under `spec/scene/visuals/` unless maintainers prefer keeping it in the GSP
proposal until promoted.

## Tests/Validation

1. Cross-check table entries against public headers and examples.
2. Cross-check query support against `src/scene/tests/query.c`.
3. Add or update tests only when the table claims a capability not already covered.
4. Run `git diff --check`.

## Stop Conditions

1. A family's public API and implementation disagree materially.
2. A required GSP mapping depends on private visual fields.
3. Query payload support cannot be stated without additional implementation work.
