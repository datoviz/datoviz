# FramePlan JSON Serialization

This document defines the JSON shape used for FramePlan debug serialization, converter testing,
and scene-to-DRP2 fixture generation.

It is normative for the test and debug surface only.
The in-memory C representation may differ.


## Purpose

A serializable FramePlan allows:

1. scene planning to be tested without a GPU,
2. converter tests to be deterministic and inspectable,
3. scene examples to be expressed as converter fixtures that can be validated by the DRP2 fixture
   runner,
4. debugging of scene planning without tracing through Vulkan output.


## Top-Level Shape

```json
{
  "frame_plan_schema": "0.2",
  "frame_plan": {
    "figure_id": "<string>",
    "frame_index": "<uint64>",
    "nodes": [ <node>, ... ],
    "products": [ <product>, ... ]
  }
}
```

The JSON above is the current schema `0.2` shape. Schema `0.1` is the pre-product characterization shape.

`figure_id` is the scene-assigned figure identity.
`frame_index` is a monotonically increasing counter per figure.
`nodes` is an ordered list of plan nodes, in execution order.

The `products` array is serialized independently from physical graph resources. Each product record exposes its typed plan-local ID and version, kind, domain, panel/view and camera identity where applicable, extent policy and resolved extent, format class and concrete format, sample domain and resolve policy, coordinate/encoding/alpha/coverage/validity metadata, required accesses, physical resource realization, source product, coherent surface-record identity, producer, consumers, and capability adaptations. Diagnostic names remain non-authoritative.

### Product Record

```json
{
  "id": 10,
  "label": "surface_depth@1",
  "version": 1,
  "kind": "surface_depth",
  "domain": "panel",
  "panel_id": "panel.0",
  "view_id": "view.0",
  "camera_id": "camera.0",
  "projection_id": "projection.0",
  "extent": {
    "policy": "panel_relative",
    "rounding": "outward",
    "origin": { "x": 16, "y": 12 },
    "width": 640,
    "height": 480,
    "render_scale": 1.0,
    "local_to_target": [1.0, 1.0, 16.0, 12.0]
  },
  "format_class": "depth_float",
  "concrete_format": 100,
  "sample_domain": "single",
  "sample_count": 1,
  "resolve_policy": "none",
  "coordinate_space": "panel_local",
  "encoding": "linear_view_depth",
  "alpha": "none",
  "coverage": "none",
  "validity": "explicit_coverage",
  "validity_product_id": 12,
  "has_background_value": false,
  "background_value": [0.0, 0.0, 0.0, 0.0],
  "has_integer_sentinel": false,
  "integer_sentinel": 0,
  "required_accesses": ["color_attachment", "sampled"],
  "lifetime": "per_frame",
  "resource_index": 3,
  "resource_label": "panel.0.surface.depth",
  "source_product_id": 0,
  "surface_record_id": 1,
  "producer_pass_index": 2,
  "producer_pass_label": "panel.0.surface_capture",
  "consumers": [
    { "pass_index": 3, "validity_requirement": "explicit_coverage", "pass_label": "panel.0.gtao" }
  ],
  "capability_adaptations": 0
}
```

The numeric `id`, `source_product_id`, and `surface_record_id` fields are typed plan-local identities; zero is the absent identity. `label` is optional diagnostic text and never authoritative. `resource_index`, `producer_pass_index`, and consumer `pass_index` values are indices into the immutable graph arrays. Resource and pass labels are duplicated only for diagnostics. The numeric `concrete_format` uses the same internal format enum as graph resources. A source product identifies an explicit predecessor or resolve source; it is never inferred from labels or shared physical storage. Every consumer declares the validity form it accepts. `capability_adaptations` is a deterministic bitset whose named meanings are defined with the composer capability contract before nonzero values are emitted.


## Node Types

Each node has a `"type"` field and type-specific fields.

### `UploadNode`

```json
{
  "type": "upload",
  "resource_id": "<string>",
  "byte_offset": "<uint64>",
  "byte_size": "<uint64>",
  "data_tag": "<string>"
}
```

`data_tag` is a human-readable label for the uploaded content (e.g. `"point.position"`,
`"lasso_polygon"`). Not required for correctness; useful for diffing test output.


### `ComputeNode`

```json
{
  "type": "compute",
  "shader_key": "<string>",
  "dispatch": { "x": "<uint32>", "y": "<uint32>", "z": "<uint32>" },
  "reads": [ "<resource_id>", ... ],
  "writes": [ "<resource_id>", ... ]
}
```

`shader_key` is a scene-registry key identifying the compute shader (e.g.
`"lasso_point_in_polygon"`, `"nonlinear_transform_mercator"`).


### `RenderNode`

```json
{
  "type": "render",
  "panel_id": "<string>",
  "render_target_id": "<string>",
  "visuals": [ "<visual_id>", ... ],
  "picking": false
}
```

`picking` is `true` when this render pass writes to the picking target instead of (or in
addition to) the color target.


### `CopyNode`

```json
{
  "type": "copy",
  "src_resource_id": "<string>",
  "dst_resource_id": "<string>",
  "byte_size": "<uint64>"
}
```

`src_resource_id` and `dst_resource_id` identify the logical resources participating in the copy.
`byte_size` is the planned copy size for buffer-oriented debug fixtures.


### `ReadbackNode`

```json
{
  "type": "readback",
  "resource_id": "<string>",
  "request_id": "<string>"
}
```

`request_id` links the readback to a pending pick or offscreen export request.


## Relationship To DRP2 Fixtures

The scene-to-DRP2 converter consumes a `FramePlan` and emits a DRP2 command stream. The DRP2
runtime consumes that command stream; direct `FramePlan` submission is only a convenience path.
Both are serializable to JSON.

Converter test fixtures take the form:

```
input:  frame_plan.json   (this schema)
output: drp2_stream.json  (DRP2 fixture schema from spec/drp2/)
```

The fixture runner validates that the converter output matches the expected DRP2 stream.
This is the primary acceptance test for the converter before GPU execution.


## Versioning

The current serialization shape is versioned as `"frame_plan_schema": "0.2"` (pre-stable). Schema `0.2` adds typed semantic products without changing the DRP2 protocol.
Fixtures pinned to a schema version are valid only for that version.
Schema bumps are permitted while the scene spec is pre-release.


## Relationship To Other Documents

| Document | Relationship |
|---|---|
| `FRAME_PLAN.md` | normative FramePlan IR definition |
| `spec/drp2/COMMANDS.md` | DRP2 command stream schema produced by the converter |
| `../API_DESIGN.md` | FramePlan inspection is a test/debug surface only, not public API |
