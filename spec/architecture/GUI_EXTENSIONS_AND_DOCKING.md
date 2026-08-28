# GUI Extensions And Docking Architecture

Status: normative target architecture; not yet implemented.


## Purpose

Datoviz owns one native Dear ImGui integration inside the optional GUI layer. This specification defines how ImPlot joins that integration, how generated C bindings remain available without moving the implementation away from C++, and how the current side-slot docking helper is replaced by a durable declarative layout model.

The current experimental mixed ImPlot example motivates this work but is transitional evidence only. It does not constrain the final dependency, context, docking, or public API ownership.


## Scope

This specification governs:

1. the Dear ImGui/cimgui and ImPlot/cimplot dependency family;
2. C++ implementation ownership and public C ABI boundaries;
3. ImGui and ImPlot context lifecycle inside `DvzGui`;
4. raw native C access to cimgui and cimplot;
5. declarative docking layout identity, construction, persistence, reset, and lowering;
6. build options, installation, packaging, bindings, and validation.


## Architectural Decisions

### One GUI runtime

Dear ImGui remains the only immediate-mode GUI runtime in Datoviz. ImPlot is an extension of that runtime and emits ordinary ImGui draw data through the existing GLFW/Vulkan GUI path.

ImPlot must not introduce another window backend, event loop, renderer, Vulkan submission path, presentation path, or frame scheduler. A Datoviz GUI viewport embedded beside an ImPlot widget remains an offscreen Datoviz view presented as an ImGui image inside the existing app frame.

The GUI layer remains optional and above the scene/app/runtime foundation described in [MODULE_LAYERS.md](MODULE_LAYERS.md).


### Native C++ implementation and C public boundary

Datoviz GUI implementation files may use native Dear ImGui and ImPlot C++ APIs directly. Rewriting internal `ImGui::` or `ImPlot::` calls through generated C wrappers is not an architectural goal because those wrappers still compile and call the same C++ libraries.

The supported boundaries are:

1. `datoviz/gui.h` provides the curated, implementation-independent `dvz_*` C API;
2. `datoviz/imgui.h` provides an advanced, version-coupled native C escape hatch through cimgui;
3. `datoviz/implot.h` provides the equivalent advanced, version-coupled native C escape hatch through cimplot when ImPlot support is built;
4. C++ Dear ImGui and ImPlot types, namespaces, templates, exceptions, standard-library types, and ownership rules do not cross the canonical `dvz_*` ABI.

The curated API owns Datoviz lifecycle and layout semantics. It must not duplicate the complete ImGui or ImPlot widget API under `dvz_*` names.


### Coherent dependency family

Dear ImGui, cimgui, ImPlot, and cimplot form one reviewed dependency family. Their revisions must be pinned and validated together.

The build must use exactly one Dear ImGui implementation. ImPlot and cimplot must compile against the same ImGui headers, configuration, structs, and symbols used by `datoviz_gui` and `datoviz_cimgui`. A second nested or system ImGui copy is invalid.

The existing `external/cimgui` checkout and its nested ImGui source remain the sole cimgui and Dear ImGui implementation. Admitting cimplot must not add another cimgui or ImGui implementation from cimplot's development checkout or dependency metadata.

cimgui must be generated from the pinned Dear ImGui revision family. cimplot must be generated from the pinned ImPlot revision and compatible cimgui types. Updating any member requires regenerating or selecting the matching generated wrappers and rerunning the complete native C and package-consumer validation.

The initial official family is based on cimgui `0e533fd0b70f6add19825bea83b66743d5b8d95b` with nested Dear ImGui `f5f6ca07be7ce0ea9eed6c04d55833bac3f6b50b`, plus cimplot `75a03832860f7832712cb5ad8d6e3ad6b69dd97c` with nested ImPlot `524f9fcd48d76c13fdf94c5ffbba8787a1ff7e39`. Datoviz fork commits may advance those identities only to carry reviewed patches, regenerated output, or provenance metadata while preserving the recorded upstream bases.

The initial fork delta includes three bounded corrections: port the required Datoviz range-slider extension to Dear ImGui 1.92.7 and regenerate cimgui; correct cimplot v1.0's generated out-of-bounds `FormatSpec[16]` copy using the logic of upstream cimplot fix `125034d782adaa43e840c0f1997aba64fbe2043f`; and give generated raw declarations explicit Datoviz-controlled Windows export, import, and static-use semantics. These patches and their regenerated outputs are part of the pinned family rather than unrecorded build-time modifications.

The default source checkout and release source archive must configure, build, install, and test without network access. Default-on ImPlot support therefore requires complete pinned source ownership under the repository's dependency policy; CMake must not fetch ImPlot or cimplot during ordinary configuration.

System and arbitrary local ImGui, cimgui, ImPlot, or cimplot overrides are outside the initial supported contract. A future override mode requires the exact pinned source/configuration fingerprint and symbol isolation; ordinary version-number checks are insufficient to prove generated-wrapper and ImGui struct ABI compatibility.

Licenses, revision identities, source origins, generated-wrapper provenance, and required third-party notices belong in release and installed-package evidence.


## Build And Component Contract

`DVZ_BUILD_IMPLOT` is the single ImPlot capability option and is a dependent option of `DVZ_BUILD_GUI`. It defaults to the configured GUI value, so the normal GUI build enables ImPlot by default while core-only and GUI-disabled profiles remain valid. Its effective value is `OFF` whenever GUI is disabled.

`DVZ_BUILD_EXAMPLES` controls whether examples are built. The ImPlot example is built exactly when examples, GUI, GLFW presentation, and ImPlot are available; there is no permanent example-specific ImPlot option.

The configured build exports `DVZ_HAS_IMPLOT` as the effective compile-time availability fact for public header guards, installed consumers, tests, and diagnostics. It describes what was actually built, not merely what the user requested.

Package feature state must not depend only on build-tree target definitions. A generated and installed Datoviz build-configuration header records effective availability for direct header consumers, while exported CMake targets, pkg-config metadata, and wheel-local CMake metadata agree with that header.

An explicit ImPlot-disabled build excludes ImPlot and cimplot sources, upstream raw headers, symbols, examples, and tests. The Datoviz wrapper header remains installable and parseable with a clear availability guard, while `cimplot.h` is installed only when the capability is built and exported targets require its include path only in that configuration.

The source target structure should keep responsibilities distinct:

1. the GUI runtime compiles Dear ImGui core and the Datoviz GLFW/Vulkan integration;
2. the raw cimgui wrapper compiles against that exact runtime;
3. the ImPlot extension compiles ImPlot core against that exact runtime;
4. the raw cimplot wrapper compiles against those exact ImGui and ImPlot sources;
5. the aggregate `libdatoviz` and exported package targets carry the required native link language and runtime dependencies for every library form that Datoviz supports.

Native Datoviz packaging is currently shared-library only. Adding general native static packaging, its private dependency closure, C++ runtime propagation, and `pkg-config --static` contract is a separate architectural task and is not a prerequisite for official ImPlot support. If native static packaging becomes supported, the same single-family and raw-C consumer requirements apply to it.

Implementation should split the current large GUI source by responsibility, with focused internal units for runtime/context ownership, docking, embedded viewports, curated widgets, and ImPlot integration. This is a source-maintainability boundary, not a requirement to publish separate shared libraries.


## Context Ownership

Each `DvzGui` owns exactly one `ImGuiContext` and, when ImPlot is available, exactly one associated `ImPlotContext`. A configured ImPlot build does not permit a live `DvzGui` without its paired ImPlot context.

Creation order is ImGui first and ImPlot second. The new ImGui context must be current before ImPlot context creation. ImPlot creation failure fails GUI creation atomically and unwinds the ImGui context. Destruction order is ImPlot first and ImGui second, with both owned contexts made current before ImPlot destruction.

Every Datoviz entry point that establishes a current GUI context must establish both associated contexts before invoking curated widgets, raw user callbacks, docking lowering, or rendering. A scoped pair-activation helper records and restores the previously current pair at the outer Datoviz boundary so nested use and multiple `DvzGui` objects do not leak current-context state.

Destruction uses a dedicated activation path that never restores a pointer to a context destroyed inside the scope. Context-pair restoration may support reentrant helper calls, but nested or cross-GUI frame construction inside callbacks remains invalid.

Datoviz-owned contexts are thread-confined to the GUI/render thread. User callbacks may call raw cimgui or cimplot only during the documented GUI frame callback while Datoviz has made the correct pair current, and must not change either current context.

Advanced raw APIs may expose upstream context functions because they are generated bindings, but users must not destroy, replace, or reparent the contexts owned by `DvzGui`. Independent user-owned contexts remain outside Datoviz lifecycle guarantees.

ImPlot has no separate renderer, backend initialization, or `NewFrame` path in Datoviz. It appends widgets and draw commands to the active ImGui frame and is rendered by the existing ImGui backend.

ImPlot presence alone must not request continuous frames. Input, animation, explicit invalidation, embedded-view work, and other existing scheduler demand remain the only reasons to render another host frame.


## Raw Native C Surface

`datoviz/implot.h` mirrors the role of `datoviz/imgui.h`: it includes the installed Datoviz build-configuration header, establishes the same cimgui type configuration by including `datoviz/imgui.h` first, then includes generated cimplot declarations only under effective `DVZ_HAS_IMPLOT`. It documents that the surface is advanced, version-coupled, and valid inside an active Datoviz GUI frame. It is opt-in directly and through a capability guard in the advanced umbrella; it is not added to the scene/app-first `datoviz.h` umbrella.

The installed package includes every generated header needed to compile that raw C surface when ImPlot is built. Installed C and CMake consumers must not need the repository source tree or private ImGui/ImPlot headers.

The supported installed extension surface is generated C for both C and C++ consumers. Datoviz does not install or promise the upstream ImPlot C++ API or ABI. Mixing packaged raw headers and libraries with another ImGui/ImPlot installation, or co-linking another implementation into the same process image, is unsupported.

On Windows, Datoviz's library build compiles generated cimgui and cimplot declarations with export semantics, installed shared-library consumers see import semantics, and any future static build uses undecorated declarations. Installed headers must never make an ordinary consumer redeclare packaged raw symbols with `dllexport`.

Raw cimgui and cimplot symbols are native integration surfaces, not canonical `DVZ_EXPORT` API. They remain excluded from generated `datoviz._ctypes`, NumPy adaptation, WASM, and stable ABI promises unless a future binding policy explicitly admits selected symbols.

The initial official integration does not add a curated Datoviz plotting API. Applications use cimplot for immediate-mode plots and retained Datoviz scene APIs for Datoviz visuals.


## Docking Layout Model

### Replace slots with a tree

The durable public model is an opaque Datoviz dock layout lowered internally to ImGui docking nodes. Public code never receives or reconstructs `ImGuiID`, calls `DockBuilder*`, includes `imgui_internal.h`, or depends on a private dockspace string.

A layout owns a root node. Splitting a leaf consumes that leaf and returns both resulting child nodes. The caller explicitly retains whichever child represents the continuing main area.

There is no special central slot. In a layout with a right sidebar, the main or central area is simply the remainder returned when the root is split to create the right child.

Direction and size are typed layout policy. Size distinguishes a normalized split ratio from logical-pixel initial intent. Logical pixels are converted against the host logical extent when the authored layout is applied or explicitly reset, ratios and converted sizes use deterministic clamps, and neither authored value is reasserted after later host resize or user splitter movement.


### Host underlay and occupied leaves

The layout distinguishes a pass-through root remainder from an occupied workspace leaf. A pass-through remainder exposes the native host Datoviz view beneath the dockspace, while an occupied leaf contains an ImGui window such as a `DvzGuiViewport` or ImPlot workspace.

Docking arranges ImGui windows only. It does not resize or reserve a `DvzFigure` or `DvzPanel`. Applications that retain a native Datoviz underlay must explicitly reserve scene space for docked overlays when overlap is undesirable; applications that want an isolated Datoviz workspace use an embedded GUI viewport in an occupied leaf.


### Layout and window identity

Each layout has a stable application-defined key and schema version scoped to its owning GUI context. The implementation must not assume a process-global singleton dockspace.

Each dockable window has a stable logical identity independent of its visible title. Renaming visible text, localization, or dynamic status text must not silently orphan persisted layout state.

Layout-managed windows, including GUI viewport windows, use a Datoviz identity-aware window-begin contract that combines visible text with an owned stable key. Callers must not synthesize private ImGui `###` labels to reproduce this identity.

Node handles are opaque Datoviz values valid only for their owning draft or committed layout generation. Stale, cross-layout, and fabricated handles are rejected.


### Declarative construction and atomic application

Callers construct a draft tree and bind stable window identities to leaves without mutating ImGui. Commit validates the complete tree and stores an owned lowering plan on `DvzGui`. Invalid ratios, duplicate window identities, splitting a non-leaf, unknown nodes, cycles, missing required nodes, and unavailable docking capability fail before any DockBuilder mutation.

Lowering occurs on the GUI thread after dockspace submission and before application windows begin, never inside an active ImGui Begin/End scope. No Datoviz-detectable invalid draft may partially mutate live docking state; upstream internal failures receive a bounded diagnostic and recovery path rather than a false absolute atomicity guarantee.

The model must produce deterministic topology independent of unrelated window callback order. Repeated ordinary application of the same committed layout is idempotent; an explicit reset is the deliberate exception.

The first implementation may expose only a full-host main dockspace, but the object model and identity rules must not prevent multiple independent or nested dockspaces later.


### Persistence and reset

The stable root identity derives from the layout key and schema version. A matching live or persisted root is preserved by default after the initial compatible layout is established. Datoviz must not remove child nodes or reassert authored topology every frame.

Application policy distinguishes at least these intents:

1. apply the authored layout only when no compatible persisted layout exists;
2. preserve a compatible persisted layout;
3. explicitly reset to the authored layout;
4. rebuild because the application layout schema version changed.

Any authored topology or default window-assignment change requires a schema-version bump. A bump rebuilds the selected authored root, while missing, corrupt, or incompatible persisted state falls back to the validated authored layout with a clear diagnostic.

When `ini_path` is `NULL`, the authored layout is applied once per `DvzGui` lifetime and still is not rebuilt every frame.

Reset acts on the selected Datoviz dockspace, not unrelated ImGui windows or other GUI contexts.


### Capability and configuration

A dock layout requires compiled docking support and a `DvzGui` configured with both docking and full-host dockspace flags. Requesting a dockspace without docking is invalid configuration and fails GUI creation clearly; ImPlot remains available in GUI configurations that do not enable docking or a dockspace.


### Ownership of ImGui internals

Datoviz may use `imgui_internal.h` and `DockBuilder*` in a focused private docking implementation. That implementation owns generated ImGui IDs, topology lowering, persistence reconciliation, and compatibility changes required by pinned ImGui upgrades.

Examples, public headers, and ordinary application code use only opaque Datoviz layout objects and stable window identities.

The existing side-slot helper must not be expanded with more special slots. It becomes a compatibility wrapper implemented over the new model and remains while `example_tuner.c` or another supported consumer depends on it. Removal requires migration of every supported consumer and a separate API decision; the ImPlot example alone is not sufficient justification.


## Interaction And Rendering Contract

ImPlot widgets participate in ordinary ImGui input capture, focus, clipping, docking, and draw-list submission. They do not own Datoviz scene input routers.

Embedded Datoviz GUI viewports retain their existing explicit input-forwarding contract. Docking a Datoviz viewport beside an ImPlot window must not merge the viewport's arcball/panzoom input with ImPlot interaction or cause both consumers to process the same captured pointer event.

Visible embedded viewports synchronize only when resize, input, scene mutation, animation, explicit frame request, or another existing demand source requires work. An idle embedded viewport must not render merely because its host ImGui frame or an adjacent ImPlot widget rendered.

Large ImPlot draw lists must use a validated backend path that honors vertex offsets or an equivalent safe index strategy. High-density line, scatter, and heatmap cases must not truncate or overflow 16-bit draw indices.


## Validation Contract

Implementation is incomplete until the following pass:

1. default-on vendored configuration, build, test, install, and package-consumer smoke with network access unavailable;
2. explicit `DVZ_BUILD_IMPLOT=OFF` configuration and build with no ImPlot or cimplot source/link leakage;
3. exact dependency-family identity and duplicate-ImGui-symbol checks;
4. C and C++ public-header probes plus installed CMake-package and pkg-config C consumers using `datoviz/implot.h` and raw cimplot calls;
5. every currently supported library form with correct native link-language and C++ runtime behavior, including the native shared-library package;
6. single and multiple `DvzGui` context creation, switching, callback use, failure unwind, repeated destruction, and recreation;
7. docking model unit tests for tree shape, stable identities, deterministic splits, invalid drafts, pre-mutation validation, stale handles, repeated apply, multiple `DvzGui` instances and layout identities without collisions, persistence preservation, schema migration, and explicit reset;
8. native interaction and screenshot smoke for the mixed Datoviz/ImPlot example on supported Linux, macOS, and Windows configurations;
9. idle scheduling proof showing that neither ImPlot availability nor a visible unchanged embedded viewport causes continuous rendering;
10. high-density ImPlot draw-data coverage above the 16-bit index range;
11. generated binding checks proving that raw cimplot symbols remain excluded unless explicitly admitted;
12. Windows export-table and installed-consumer proof for raw cimgui and cimplot declarations, including correct import semantics;
13. raw C runtime plot calls on supported CPU architectures, not compile-only proof, with ARM64 coverage where available;
14. `just spec-check`, public documentation generation, third-party-notice review, `git diff --check`, and exact artifact validation required by the active release gate.


## Migration

Implementation should proceed in independently validated development slices while preserving atomic public admission:

1. on a feature branch, prepare the patched pinned family, offline target graph, paired context lifecycle, generated build-configuration header, raw installed C surface, Windows symbol policy, and minimal example migration away from fetched sources and example-owned context;
2. keep the new capability internal and default-off until that complete slice passes package and platform gates, then publish `DVZ_HAS_IMPLOT=1`, remove the example-only option, and flip the official component default on in the same integration unit;
3. implement and test the declarative docking model behind a focused private lowering unit;
4. migrate the mixed ImPlot example from private docking code to the public layout model;
5. retain the old side-slot helper as a compatibility wrapper until all supported tuner consumers are migrated under a separate API decision.

Dependency-only or symbol-only commits may exist as feature-branch checkpoints, but must not land on the active release line while advertising an ImPlot capability without Datoviz-owned lifecycle, installed headers, and a single library-owned implementation.

The current experimental C++ example may exist before these slices. It is transitional evidence only and does not constrain final context, dependency, docking, or public API ownership.


## Non-Goals

This work does not:

1. replace retained Datoviz plotting or scene semantics with ImPlot;
2. wrap the complete ImPlot API under `dvz_*` names;
3. add ImPlot to DRP2, scene frame plans, WebGPU, WASM, or generated Python bindings;
4. create a general public GUI-extension plugin ABI;
5. promise publication-quality vector export from ImPlot;
6. create a second presentation or scheduling path;
7. make arbitrary system ImGui/ImPlot combinations ABI-compatible.
