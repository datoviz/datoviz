# Datoviz, Raw ctypes, GSP, and VisPy2 Scope

Status: normative v0.4 API and documentation positioning.

This document records the v0.4 ownership split between Datoviz, raw generated Python bindings, GSP,
and VisPy2. It exists so release notes, public documentation, examples, and future binding work do
not accidentally recreate the v0.3 Python API inside the Datoviz repository.


## Decision

Datoviz v0.4 owns the native renderer/runtime foundation:

1. the C API;
2. retained scene/app objects;
3. DRP2 emission and runtime execution;
4. native Vulkan and experimental WebGPU/WASM backend work;
5. native interaction, cameras/controllers, animation, picking/probing, selection, capture, and
   performance-oriented examples;
6. raw generated `ctypes` bindings that honestly mirror the intended v0.4 C surface.

GSP owns the backend-independent scientific-visualization rendering representation:

1. visuals;
2. data buffers;
3. transforms;
4. style/state needed to describe rendered visual output;
5. a renderer-neutral contract that can target Datoviz, Matplotlib, and future renderers.

VisPy2 owns the Pythonic user experience above GSP:

1. object-oriented scene and plotting APIs;
2. Python-level convenience constructors and data workflows;
3. interaction tools, camera abstractions, selections, animations, and application/notebook
   integration when those concepts are user-facing Python APIs;
4. backend selection and fallback behavior across Datoviz, Matplotlib, and other renderers.


## Rationale

The v0.3 Datoviz repository included a Pythonic object-oriented API. In v0.4, that API should not be
rebuilt inside Datoviz if its shape is intended to match GSP. Keeping the Pythonic API in Datoviz
would make one renderer repository the de facto owner of a backend-independent visual model, and
would make Matplotlib or other renderers look like adapters to Datoviz rather than peer GSP
renderers.

The cleaner split is:

```text
Datoviz
  C renderer/runtime, scene/app, DRP2, native interaction, raw ctypes

GSP
  backend-independent visual rendering representation

VisPy2
  Pythonic plotting, scene objects, interaction tools, and backend selection
```

This lets Datoviz v0.4 release independently as an engine/API/architecture release. GSP and VisPy2
can then provide the Pythonic scientific visualization layer in a separate release cycle.


## Public Documentation Consequences

Datoviz documentation should be C-first and renderer-focused:

1. full C getting-started guide, tutorials, examples, visual reference, and advanced runtime topics;
2. raw `ctypes` documentation as a low-level integration and backend-author surface;
3. a clear "which API should I use?" page;
4. a Datoviz-as-GSP-backend page once the adapter is public;
5. links to GSP/VisPy2 for Pythonic plotting and high-level scientific workflows.

Datoviz documentation should not host the complete high-level VisPy2/GSP user guide. It may include
short positioning notes and bridge documentation, but Pythonic tutorials, plotting examples,
notebook workflows, backend-comparison examples, and high-level interaction docs should live with
VisPy2/GSP.

The raw `ctypes` documentation should set expectations explicitly:

1. it mirrors the C API;
2. object ownership follows the C API;
3. it is useful for tests, backend integration, and low-level automation;
4. it is not the recommended Pythonic plotting interface.


## Examples Consequences

Datoviz should keep C examples as first-class release proof. They validate the actual v0.4 Datoviz
surface: retained scene objects, app/window/offscreen paths, visual families, controllers,
picking/probing, capture, DRP2/runtime behavior, and performance.

Datoviz should include only a small raw-`ctypes` Python example set, for example:

1. load the shared library and query the version;
2. create/destroy a minimal scene path;
3. render or capture a simple offscreen point/image scene when available;
4. run a tiny pick/probe smoke when runtime support is available.

Rich Python scientific workflows should live in VisPy2/GSP. Advanced example ownership should be
decided by purpose:

1. renderer capability proof belongs in Datoviz C;
2. protocol/runtime validation belongs in Datoviz C or DRP2/DVZR fixtures;
3. Pythonic plotting, dashboard, notebook, and data-workflow examples belong in VisPy2/GSP;
4. backend comparison examples belong in VisPy2/GSP.


## Interaction Components

GSP may remain render-only. User-facing Python interaction does not need to be part of the GSP core
contract. VisPy2 can own the Python interaction layer above GSP: events, cameras, controllers,
selection tools, animations, and application integration.

Datoviz can still provide reusable C interaction components for VisPy2 to wrap, provided those
components are renderer-neutral. The desired boundary is:

```text
VisPy2 events/tools
  -> reusable Datoviz controller/camera/math components
  -> GSP visual state or backend requests
  -> Datoviz, Matplotlib, or another renderer
```

Avoid this boundary:

```text
VisPy2 interaction
  -> Datoviz scene/app internals
  -> Datoviz renderer only
```

Current Datoviz code is close but not fully exposed this way. The `input` module is already fairly
standalone. Panzoom, arcball, fly, and turntable logic are mostly renderer-neutral internally, but
their public constructors are scene-owned. Camera and animation are also currently panel/scene-owned
from the public API. A future extraction pass should expose explicit standalone public constructors
and state-export functions for reusable controller/camera components while keeping scene-owned
Datoviz wrappers on top.


## Release Positioning

Datoviz v0.4 may be released before GSP/VisPy2 is ready, as long as the public messaging is honest:

1. v0.4 is the native C renderer/runtime foundation;
2. the v0.3-style high-level Python object-oriented API is not part of Datoviz v0.4;
3. raw `ctypes` is included for low-level integration and backend work;
4. the Pythonic scientific visualization API is expected to live in GSP/VisPy2;
5. GSP/VisPy2 follows on its own release timeline.

Public announcements should not imply that Datoviz v0.4 is the Pythonic successor to v0.3.
