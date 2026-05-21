# Scene API WASM Portability Contract

This document defines the public scene API constraints required for Datoviz scene code to remain
compilable and bindable in a future WASM/WebGPU build.

The scene API target is not merely "valid C". It is WASM-bindable C: public headers must be easy to
parse, generate bindings for, compile with Emscripten, and call from JavaScript or another host
language without depending on native ABI details.


## Scope

This contract applies to installed scene-facing public headers under `include/datoviz/`, including:

1. `datoviz/scene.h`,
2. `datoviz/app.h` where it exposes scene-level app concepts,
3. `datoviz/scene/*.h`,
4. future scene controller, interaction, request, frame-plan, and WebGPU bridge headers.

Internal implementation files may use more C constructs when they remain private to native builds.
However, scene code intended to compile to WASM should follow this document unless a narrower file
documents why it is native-only.


## Public Header Rules

Public scene headers should expose:

1. opaque handles for retained scene-owned objects,
2. plain descriptor, state, request, and result structs,
3. fixed-width integer types for ids, counts, byte sizes, and masks,
4. explicit ownership and lifetime rules in docstrings,
5. output-pointer APIs for returned structured data.

Public scene headers must avoid:

1. nested anonymous struct or union members,
2. layout-dependent C inheritance or public casts between unrelated handle types,
3. public tagged unions whose layout becomes part of the ABI,
4. variadic functions,
5. platform handles or platform types,
6. Vulkan, GLFW, Cocoa, pthread, file descriptor, or native-window types,
7. `sizeof(public-opaque-type)` assumptions,
8. raw function pointers embedded in public structs,
9. compiler-specific attributes outside the existing portability macro layer,
10. C++ standard library types or C++ references in any public signature.

Named structs are preferred for new public descriptors and state snapshots. Existing typedef-only
descriptor structs may remain when generated bindings can parse them. Named unions are acceptable in
private implementation headers when hidden behind opaque handles. They should not appear in
installed scene headers unless a binding plan explicitly approves them.


## Types

Use explicit integer widths for public ABI-bearing values:

1. `uint32_t` for counts, enum masks, dimensions, and indices when the value is naturally bounded,
2. `uint64_t` for byte sizes, stable ids, and file or stream offsets,
3. `int` only for conventional status returns and existing enum-compatible signatures,
4. `bool` only when the value is semantically boolean and not a bitmask.

Avoid exposing `size_t` in new public scene APIs when the value crosses bindings. `size_t` differs
between wasm32 and native 64-bit platforms and complicates generated bindings. Prefer `uint32_t` for
element counts and `uint64_t` for byte sizes. Internal code may still use `size_t` for allocation and
checked arithmetic.

Flags and dimension sets should use explicit masks, not overloaded enums. Prefer a dedicated mask
typedef such as:

```c
typedef uint32_t DvzDimMask;
```

Single-dimension query APIs may continue to use a single-dimension enum when that is clearer.


## Structs

Public structs should be plain data carriers:

1. no anonymous union members,
2. no pointer fields unless ownership and lifetime are explicit,
3. no function pointer fields,
4. no embedded opaque object structs,
5. no platform-specific members behind preprocessor branches,
6. no fields whose interpretation depends on native pointer width.

Prefer versionable descriptors and state snapshots over exposing mutable object internals. For
example, controller state should be read and written through plain state structs:

```c
int dvz_panzoom_get_state(const DvzController* controller, DvzPanzoomState* out);
int dvz_panzoom_set_state(DvzController* controller, const DvzPanzoomState* state);
```

Do not expose direct mutable controller fields in public headers.


## Handles

Public scene objects should be opaque pointer handles:

```c
typedef struct DvzScene DvzScene;
typedef struct DvzPanel DvzPanel;
typedef struct DvzController DvzController;
```

Do not require callers to cast between handle families. If one public concept needs to support
multiple concrete families, expose one generic opaque handle and typed functions that validate the
handle's family internally.

For controllers, the preferred public shape is:

```c
DvzController* dvz_panzoom(DvzScene* scene, const DvzPanzoomDesc* desc);
DvzController* dvz_arcball(DvzScene* scene, const DvzArcballDesc* desc);

int dvz_panel_bind_controller(DvzPanel* panel, DvzController* controller, DvzDimMask dims);
DvzController* dvz_panel_controller(DvzPanel* panel, DvzDim dim);
```

Family-specific state APIs take `DvzController*` and return an error when the handle has the wrong
family. This keeps the ABI simple and avoids public C inheritance.


## Callbacks And Events

Callbacks need an explicit WASM policy before entering public scene headers.

When callbacks are required, prefer a registry-style API over function pointers embedded in structs:

```c
uint64_t dvz_scene_add_callback(
    DvzScene* scene, DvzSceneEventType type, DvzSceneCallback callback, void* user_data);
int dvz_scene_remove_callback(DvzScene* scene, uint64_t callback_id);
```

The callback typedef and dispatch rules must document:

1. whether the callback can cross the WASM/JavaScript boundary,
2. whether it may be called synchronously during API calls,
3. ownership and lifetime of event payload pointers,
4. whether callback ids remain stable after removals,
5. how stale asynchronous request results are discarded.

Do not add public callback structs or direct host event-loop dependencies to the scene layer.


## Backend Boundary

The scene layer must remain backend-neutral:

1. scene emits `FramePlan` and DRP2-compatible work,
2. scene does not call Vulkan, vklite, GLFW, Cocoa, or platform window APIs,
3. native app/window integration stays below or beside scene, not inside scene semantics,
4. WebGPU/WASM transport details stay behind a bridge or runtime adapter,
5. shader contracts intended to be portable use WGSL or a DRP2-declared portable shader format.

Native-only helpers may exist, but their types must not leak into scene public headers.


## Internal Implementation Guidance

Private scene implementation may use named tagged unions for compact storage:

```c
typedef union DvzControllerData
{
    DvzPanzoomStateful panzoom;
    DvzArcballStateful arcball;
    DvzFlyStateful fly;
    DvzTurntableStateful turntable;
} DvzControllerData;
```

Keep such unions in private headers such as `src/scene/_controller.h`. Pair them with an explicit
type tag and validation helpers. Do not expose their layout through installed headers or generated
bindings.


## Validation Requirements

Before a new scene public API is treated as stable, it should satisfy:

1. public headers compile as C and C++ consumers through the normal native build,
2. public scene headers avoid the banned constructs above,
3. generated binding tools can parse the declarations,
4. a minimal Emscripten compile target can build `scene` plus the portable DRP2-facing subset
   without `vk`, `vklite`, `canvas`, GLFW, or native app modules,
5. any unsupported runtime capability fails through explicit validation rather than native-only
   assumptions.

Until the Emscripten target exists, reviewers should apply this document manually during public API
changes and prefer conservative C signatures.


## Controller Acceptance Criteria

Controller API changes must preserve these WASM-facing properties:

1. controllers are scene-owned opaque handles,
2. panels borrow controller handles and bind them by dimension mask,
3. panzoom, arcball, fly, and turntable are controller families, not separate public ABI layouts,
4. linked panels share controller handles rather than copying exposed struct fields,
5. input routing is panel-local and does not require a controller to own one native viewport,
6. state inspection uses typed POD snapshots and output-pointer APIs,
7. destroying a panel never destroys a shared controller.
