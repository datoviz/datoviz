# Optional Providers And Plugins

Status: v0.4 architecture policy for optional external integrations. Only provider plugins are
v0.4 scope; recipes, custom visual packages, and technique plugins are future extension layers.

Datoviz is a library, not an application. Its extension model should therefore be based on explicit
capabilities that feed the existing scene -> DRP2 -> vklite/canvas/app runtime path. Extensions must
not create parallel renderers, presentation layers, frame streams, Vulkan wrappers, or scene
semantics.


## Extension Classes

Use these names consistently when planning or documenting extension work:

| Class | Purpose | v0.4 status |
| --- | --- | --- |
| Provider | Optional bridge to an external dependency or host SDK. | In scope for Qt bridge only. |
| Recipe | Higher-level reusable composition of public Datoviz objects. | Public API usage, not core ABI. |
| Custom visual | Descriptor-backed visual with user-provided shaders and resources. | Design only. |
| Technique/pass | Frame-plan extension for postprocess, prepass, or render effect work. | Future design only. |

Providers are dependency boundaries. They may link to Qt, shaderc, CUDA, codec libraries, or similar
external SDKs, but `libdatoviz` must not gain those dependencies just because a provider exists.

Recipes are ordinary user or community packages built on public Datoviz APIs. A recipe may create
multiple built-in visuals, controllers, annotations, and update helpers, but Datoviz core does not
need to know that the composition has a special type.

Custom visuals are future descriptor-backed shader packages. They should declare attributes,
bindings, shaders, topology, and draw state while Datoviz owns resource lifetime, pass assignment,
and command emission.

Technique/pass plugins are future render-graph contributors. They should declare pass inputs,
outputs, ordering constraints, required capabilities, and shader resources. They should not submit
Vulkan commands directly in the first public design.


## v0.4 Provider Rules

1. `libdatoviz` remains the core runtime and must not link Qt, PyQt, shaderc, CUDA, NVENC,
   Kvazaar, Naga, Tint, SPIRV-Cross, or other optional heavy dependencies by default.
2. A provider is built as a separate shared object, executable helper, or host-language package.
3. A provider is loaded explicitly by the integration layer that needs it.
4. Provider failure must be diagnosed as an unavailable optional capability, not as a core Datoviz
   import or startup failure.
5. Providers must check ABI, runtime version, and host-library compatibility before doing work.
6. Providers exchange primitive handles, opaque pointers, or declared data descriptors with
   Datoviz. They should not expose internal Datoviz structs unless those structs are public,
   versioned, and already part of the supported ABI.
7. Providers do not own the Vulkan instance, device, queues, swapchain, frame streams, or command
   submission path unless a future spec explicitly grants that ownership.
8. Providers must degrade cleanly in pip, conda, source-build, and system-package environments.


## Candidate Provider Boundaries

Qt host bridge:

1. Needed for v0.4 PyQt hosting because current PyQt wheels expose `QVulkanInstance` but not the
   binding for `QVulkanInstance::setVkInstance()` or `QVulkanInstance::vkInstance()`.
2. Must be separate from core `libdatoviz`.
3. Should be loaded by `datoviz.qt` only when PyQt hosting is requested.
4. Is implemented but source-build-only in RC1: Qt/PyQt hosting works when a source-built provider,
   Qt runtime, PyQt Vulkan binding surface, and platform WSI extensions are available. A packaged
   provider is an RC2 deliverable.
5. Must diagnose missing bridge libraries, ABI mismatches, Qt runtime mismatches, unsupported
   PyQt/PySide bindings, and missing Vulkan platform support before creating hosted Datoviz views.

Shader compiler provider:

1. Core DRP2 and scene code should be able to consume precompiled shader modules.
2. Runtime GLSL, WGSL, SPIR-V transformation, Naga, Tint, shaderc, and SPIRV-Cross support should
   be optional provider work unless a dependency is small and release-critical.
3. Shader compiler provider absence should produce a clear diagnostic that precompiled shader
   modules are required.

Video encoder providers:

1. Core may keep the frame/video stream contracts.
2. Heavy encoders such as NVENC or Kvazaar are good backend-provider candidates.
3. Encoder provider selection should be capability-based, not compiled into scene semantics.

CUDA/CuPy provider:

1. Core may expose Vulkan external-memory and external-semaphore contracts where they are part of
   the runtime foundation.
2. CUDA SDK, CuPy, and Python interop glue should remain advanced/unstable and optional.
3. CUDA-specific provider failure must not affect ordinary Vulkan rendering.


## Recipes And Community Packages

Many advanced user extensions should be recipes, not native plugins. A recipe package may define a
domain object that creates and coordinates existing Datoviz visuals:

1. neuron populations from point, segment/path, glyph, and text visuals;
2. molecule renderers from sphere, segment, mesh, and labels;
3. geospatial overlays from image tiles, vector fields, paths, and annotations;
4. flow visualizations from particles, paths, sampled fields, and colorbars.

Recipes are the preferred distribution route when custom shader code, pass-graph access, or native
dependency loading is unnecessary. They can ship as Python packages, C helper libraries, or examples
that use public APIs.


## Custom Visual Direction

Custom visuals should start descriptor-first:

1. declare attributes and formats;
2. declare textures, samplers, uniforms, and push-style parameters;
3. declare topology, depth/blend/cull state, and draw/instance counts;
4. provide SPIR-V shader modules by default;
5. optionally provide GLSL/WGSL source when a shader compiler provider is present.

This layer should not expose the current internal visual registry ABI directly. The internal
registry is useful implementation evidence, but a public custom-visual ABI should be narrower,
versioned, and stable.

Native visual-family plugins should be reserved for cases that cannot be represented by descriptors:

1. custom CPU preprocessing;
2. custom GPU staging;
3. unusual draw or dispatch generation;
4. custom picking/query semantics;
5. custom pass participation.

Native visual-family plugins are not v0.4 scope.


## Technique/Pass Direction

Technique/pass plugins are future frame-plan contributors. They should declare:

1. pass type: prepass, main pass, postprocess, compute pass, resolve pass;
2. inputs: color, depth, normal, object-id, sampled fields, storage buffers, or plugin outputs;
3. outputs: modified color, auxiliary textures, buffers, or diagnostics;
4. required capabilities;
5. ordering constraints relative to built-in passes;
6. shader modules and binding declarations.

Examples include eye-dome lighting, object outlines, screen-space compositing, custom volume
resolve passes, and specialized splat sorting or resolve work.

The first public technique design should emit frame-plan or DRP2-compatible descriptors and let
Datoviz schedule resources and command emission. It should not let plugins record arbitrary Vulkan
commands into Datoviz-owned command buffers.


## Packaging Policy

Main Datoviz wheels should stay lean and should not bundle Qt, CUDA, heavy shader toolchains, or
large codec stacks by default.

Preferred packaging order:

1. source/local provider builds for early v0.4 experiments;
2. conda-forge split packages for providers that depend on conda-managed Qt/CUDA/codec stacks;
3. optional PyPI provider wheels only after ABI and runtime-version checks are proven;
4. no bundled host SDKs in the main `datoviz` wheel unless explicitly approved by release policy.

For the Qt host bridge, source builds should keep `DVZ_ENABLE_QT_BRIDGE=AUTO` as the default:
build `datoviz_qtbridge` when Qt6 Gui development files are available and skip it without failing
the core build when they are absent. Installed providers should be discoverable by `datoviz.qt`
next to the Python package or through the platform loader; `DATOVIZ_QTBRIDGE_LIBRARY` remains the
explicit override for local builds, split packages, and debugging.


## Validation

Documentation-only changes need:

```sh
git diff --check
git status --short
```

Provider code changes should add provider-specific smoke tests and prove that importing or using
core Datoviz still works when the provider is absent.
