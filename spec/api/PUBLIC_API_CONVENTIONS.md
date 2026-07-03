# Public API Conventions

Status: normative v0.4 API design guidance.

This document defines cross-module rules for the public C API. It is intentionally short: when a
feature-specific spec needs more detail, it should link here and add only the local rules required by
that feature.


## Goals

1. Keep public names, ownership, and mutation patterns predictable across modules.
2. Keep the C API practical for generated bindings, including future WASM bindings.
3. Preserve implementation freedom by hiding backend, C++, and third-party-library types.
4. Make the v0.4 API consistency pass auditable before release.


## Naming

Public functions use `dvz_<object>_<action>()` for object-level actions.

The v0.4 public ABI uses three deliberate C namespaces:

1. `dvz_*` is the canonical C API.
2. `dvz_ffi_*` is the foreign-function-interface helper ABI for raw `ctypes`, WASM, and other
   runtimes that need pointer-oriented call shapes.
3. `dvz_wasm_*` is the browser/session-specific WASM ABI.

Do not add ad-hoc `_ffi` suffix functions. When a binding needs a pointer-output version of a
canonical by-value initializer, keep the canonical C initializer and add a `dvz_ffi_*` wrapper.

When a public object has subroles, role/property setters use:

```text
dvz_<object>_<role>_<property>()
```

Examples:

```text
dvz_polygon_fill_color()
dvz_polygon_stroke_width_px()
dvz_graph_node_sizes()
dvz_graph_edge_colors()
```

Backend or third-party names should not appear in high-level user APIs unless the user is selecting
an explicit backend policy such as a triangulation backend.


## Public Headers

Application-level C and C++ examples should include the top-level umbrella header:

```c
#include <datoviz.h>
```

The installed forwarding header `include/datoviz.h` delegates to `include/datoviz/datoviz.h`.
Focused module and submodule headers remain valid for advanced or low-level examples that are
documenting a specific subsystem.

Public headers that declare exported functions must wrap those declarations with `EXTERN_C_ON` and
`EXTERN_C_OFF`. The macros live in `include/datoviz/common/macros.h`; under C++ they provide
`extern "C"` linkage, and under C they expand away. The public header probes in `testing/` are the
release guard for umbrella-header parseability from both C and C++.


## Public Struct Naming

Public struct suffixes should describe the role of the record, not just its shape.

Use `Desc` for user-authored descriptors that define one object, resource, attachment, or atomic
operation. These records are commonly passed to constructors or operation functions and may be
size-versioned when they are likely to grow.

Use `Config` for broader runtime, device, application, window, stream, or integration policy. A
config record is usually initialized from a default helper, then optionally modified before creating
or wiring a subsystem.

Use `Request` for one-shot operation input that asks the system to perform work and may produce a
result now or later. Query, readback, capture, and similar command-like inputs should prefer this
suffix when the operation identity matters.

Use `Info` for descriptive metadata and discovery/reporting records. Avoid `Info` for behavior that
the caller expects the callee to enact; use `Desc`, `Config`, or `Request` instead.

Use `State` for current retained state snapshots, `Style` for appearance values, `View` for borrowed
views over caller or internal memory, and `Resources` for bundles of existing borrowed handles or
objects.

Do not rename a public struct just to make suffixes uniform. Prefer renaming only when the current
suffix misleads users about ownership, mutability, or whether the record is an input descriptor,
runtime configuration, request, state snapshot, or borrowed view.

Initializer functions should match the struct suffix:

1. `dvz_<thing>_config()` returns the canonical initialized `Dvz<Thing>Config`;
2. `dvz_<thing>_desc()` returns the canonical initialized `Dvz<Thing>Desc`;
3. `dvz_<thing>_style()` returns the canonical initialized `Dvz<Thing>Style`;
4. `dvz_<thing>_get_config()` returns the current config retained by an existing object.

Do not add new `dvz_<thing>_default_config()` functions. A config initializer already implies
default values.

Canonical C descriptor/config/style initializers may return small public records by value:

```c
DvzGeometryCubeDesc desc = dvz_geometry_cube_desc();
```

Do not replace these canonical C APIs solely for FFI convenience. Add `dvz_ffi_*` out-pointer
wrappers when raw bindings or WASM need pointer-based ABI.


## Visuals, Semantic Objects, And Composites

`DvzVisual` is a low-level render leaf: point, marker, segment, path, mesh, image, volume, and
similar visual families.

Higher-level semantic objects own domain state and expose typed APIs. Examples include polygon
regions, graph topology, axes, colorbars, annotations, and orientation gizmos. They should not expose
their implementation visuals as the primary mutation surface.

`DvzComposite` is the renderable bridge between semantic objects and panel-attached visuals. A
composite is a scene-owned renderable view over one semantic object and may own or derive several
coordinated leaf visuals internally.

Typed semantic object APIs remain the normal user path:

```text
dvz_polygon_geometry()
dvz_polygon_fill_color()
dvz_polygon_set_region_fill_color()
dvz_graph_node_sizes()
dvz_graph_edge_colors()
```

Composites provide the generic panel attachment path:

```text
dvz_panel_add_visual()
dvz_panel_add_composite()
```

Composites may expose generated leaf visuals by stable role names for advanced users, testing, and
integration code. The leaf visual roles are implementation-facing extension points, not a substitute
for the typed semantic API.


## Structs Versus Setters

Use descriptor structs when the data is naturally a coherent record:

1. construction-time configuration,
2. resource descriptors,
3. bulk or atomic operation descriptors,
4. result payloads,
5. records whose fields are normally authored together.

Use flat typed setters when fields are independent, frequently changed, or binding-facing:

1. style and appearance properties,
2. visibility and enable flags,
3. interaction options,
4. role-specific composite state.

Avoid nested public style structs as the primary user path. Optional convenience structs are allowed
when they reduce real C call-site noise, but they should not be the only way to set common style
state.


## Return Values

Use `DvzResult` for stable user-facing operations whose only result is success or failure:

```c
DvzResult dvz_visual_set_data(...);
DvzResult dvz_panel_add_visual(...);
```

`DvzResult` is an `int32_t` status code. `DVZ_OK` is success and `DVZ_ERROR` is the generic failure
code. APIs returning `DvzResult` must return `0` on success and a negative value on failure. Future
specific error codes, if added, must also be negative unless a function explicitly returns a
different status type.

Use `bool` only for predicates or conversion/lookup functions where the return value answers a yes/no
question:

```c
bool dvz_visual_depth_test(const DvzVisual* visual);
bool dvz_panel_transform_point(...);
```

Do not use `bool` for ordinary mutators just to mean success. A mutator that may fail should return
`DvzResult`, unless it appends to a fluent command builder whose established local contract is
"command accepted".

Use pointer returns for constructors and accessors, with `NULL` meaning unavailable or failed. Use
unsigned integer returns for counts. Keep plain `int` when the function returns a richer integer
domain rather than success/failure, such as a signed count, a file descriptor, a backend or sink
result code, a Vulkan-style nonzero result, or a module-specific positive status like
`DVZ_CANVAS_FRAME_WAIT_SURFACE`. Low-level runtime and interop headers may keep `int` when that
better preserves the native backend convention.


## Public Struct Rules

Public structs should be zero-initializable unless explicitly documented otherwise. Zero values must
either mean a documented default or a documented disabled/empty value.

Pointer-passed descriptor structs should document ownership, defaults, and which fields are ignored
or required for each mode.

When a public function documents that a pointer-passed config, descriptor, style, or request may be
`NULL` for defaults, `NULL` must be semantically equivalent to passing the canonical public
initializer result. Context inheritance must not depend on pointer nullness:

```c
dvz_view_gui(view, NULL);

DvzGuiConfig config = dvz_gui_config();
dvz_view_gui(view, &config);
```

If these calls should not be equivalent, the API must use more precise semantics such as clear,
disable, zero, inherit, or reset, and document that directly. The v0.4 cleanup plan for this rule is
in [NULL_DEFAULT_INVARIANT_REFACTOR_PLAN.md](NULL_DEFAULT_INVARIANT_REFACTOR_PLAN.md).

Structs likely to grow after v0.4 should have a compatibility strategy before API freeze. Acceptable
strategies include reserved fields, a documented version/size field, or keeping the struct out of the
stable public surface until it settles.

Growable public descriptor and config structs should put `uint32_t struct_size` first and
`uint32_t flags` second. Canonical initializer functions must set `struct_size = sizeof(struct)` and
`flags = 0`. Public entry points should reject caller-provided size-versioned structs with
`struct_size == 0`, with a size other than the current `sizeof(struct)`, or with unknown nonzero
flags. Future releases may relax the size check for older smaller struct sizes after defining
field-presence rules, but v0.4 should require explicit current-size initialization.

Public structs must not expose Vulkan handles, DRP2 runtime object ids, command buffers, atlas
pages, C++ standard-library types, or third-party-library structs.

The umbrella `<datoviz.h>` should stay scene/app-first. Advanced Vulkan, vklite, DRP2, and other
runtime internals should be included through their explicit module headers by callers that need
them.

For RC1, existing advanced runtime APIs should be documented as `advanced/unstable` rather than
hidden wholesale. Hide or remove only specific accidental symbols whose ownership and release role
are clearly internal.


## Binding And WASM Constraints

Generated bindings and future WASM support are first-class API constraints.

Prefer APIs built from:

1. scalar values,
2. enums,
3. opaque handles,
4. pointer-plus-count arrays,
5. flat data records with simple field types.

Avoid primary APIs that require nested structs, callback-heavy setup, language-specific ownership
conventions, or exposing temporary C pointers whose lifetime is hard to express in bindings.


## Data APIs

Bulk data setters should use explicit pointer-plus-count signatures. Ownership must be documented as
one of:

1. copied before return,
2. borrowed until return,
3. retained by the callee until an explicit replacement or destroy operation.

Large mutable data should support range updates where practical. Style setters should not implicitly
replace bulk data, and bulk data setters should not implicitly change style policy except where a
descriptor explicitly says so.


## Examples

Preferred:

```c
dvz_polygon_stroke_width_px(polygon, 1.0f);
dvz_graph_node_colors(graph, 0, node_count, colors);
dvz_triangulate_polygon(source, &desc);
```

Avoid as the primary API when it only wraps independent style properties:

```c
dvz_polygon_set_style(polygon, &(DvzPolygonStyle){...});
```

The optional convenience form may still exist if the flat setters remain available and documented.
