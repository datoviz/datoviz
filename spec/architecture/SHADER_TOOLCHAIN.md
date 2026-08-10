# Shader Toolchain And Runtime Compilation

Status: implemented v0.4 architecture and release contract. Official-package and supported-platform proof remain RC3 gates; RC4 owns exact-artifact course proof and freeze. Updated: 2026-08-08.

## Decision

Datoviz v0.4 treats runtime GLSL compilation as a first-class supported capability for external user shaders, examples, and the modern GPU graphics tutorial. It is not merely an emergency fallback for Datoviz's built-in shaders.

SPIR-V remains the native runtime shader-module contract. Datoviz does not make GLSL source, shaderc, glslang, or a command-line compiler mandatory for applications that provide precompiled SPIR-V.

Use one coherent Shaderc-based path:

| Role | Required tool | Contract |
| --- | --- | --- |
| Build Datoviz-owned native shaders | `glslc` | Compile scene, test, and example GLSL to deterministic SPIR-V before embedding or packaging. Canvas has no built-in shader compiler dependency. |
| Compile external user and tutorial shaders at runtime | `libshaderc` | Lazily compile GLSL to owned SPIR-V through a public Datoviz API with availability and diagnostics. |
| Validate generated SPIR-V | `spirv-val` | Validate build products in CI and release lanes without making ordinary consumer startup depend on the validator. |
| Optional compiler cross-checks and specialized workflows | `glslangValidator` | Keep optional; do not require it for the normal native Datoviz build after the `glslc` consolidation. |

`glslc` and `libshaderc` are two interfaces to the same compiler family and are intentionally used for different phases. Datoviz must not invoke command-line `glslc` or `glslangValidator` from the runtime compilation API.

GLSL remains the v0.4 tutorial shader language. Do not add Slang, a second shader-language track, or a general compiler-plugin framework before final v0.4.0. Reassess other source languages after the release without changing the SPIR-V runtime boundary.

## Shipped And External Shader Policy

Release builds, wheels, and installed packages must carry precompiled SPIR-V for Datoviz-owned native shaders. Release source-archive validation must build those products with `glslc` and fail if required precompiled shaders cannot be produced. A release artifact must not depend accidentally on a developer machine's runtime shaderc installation to create built-in pipelines.

Developer source builds may retain embedded-GLSL fallback when `glslc` is unavailable, provided CMake reports the reduced configuration clearly and runtime shaderc availability is proven before the fallback is used. `DVZ_ENABLE_SHADERC=AUTO` must not claim usable runtime compilation merely because a header was found.

External user shaders and tutorial shaders remain separate files compiled at application startup. Editing one of these shaders and restarting the application must not require recompiling C. The v0.4 tutorial files remain self-contained; runtime include resolution, hot reload, shader caching, reflection, and multi-language compilation are deferred unless a concrete RC blocker requires them.

## Build-System Contract

The implemented native shader pipeline uses one reusable CMake helper based on `glslc`. Scene shaders, native shader fixtures, and applicable examples share discovery, target-profile flags, dependency tracking, output naming, diagnostics, and optional validation. Canvas uses Vulkan copy/blit operations and has no built-in shader compilation path.

The helper must:

1. discover `glslc` from `DVZ_GLSLC_EXECUTABLE`, `DVZ_GLSLC`, the Vulkan SDK, or `PATH` without reading or creating generic parent-project cache variables, with a namespaced switch to disable automatic SDK and `PATH` discovery for hermetic configurations;
2. accept an explicit shader stage, source, include directories, target profile, output, and dependencies;
3. use deterministic flags and rebuild when source or declared includes change;
4. expose generated SPIR-V as target dependencies rather than relying on build order;
5. support `spirv-val` validation in CI and release configurations;
6. fail release builds when required precompiled shaders cannot be produced;
7. keep an explicitly diagnosed developer fallback where policy allows it.

Canvas imposes no command-line shader compiler requirement on normal native source builds. Specialized WebGPU, validation, or contributor workflows may continue to discover their own tools separately.

Build-time and runtime compilation use named target profiles from one documented policy:

| Profile | Target environment | SPIR-V | Reason |
| --- | --- | --- | --- |
| v0.4 graphics | Vulkan 1.0 | SPIR-V 1.0 | Preserve the proven graphics and discard behavior across supported drivers while the consolidation lands. |
| v0.4 compute | Vulkan 1.3 | SPIR-V 1.6 | Preserve the proven compute policy that avoids deprecated `WorkgroupSize` emission. |

Agents must not independently change these versions in CMake, runtime compilation, or tests. A profile change requires focused shader-module, render, compute, validation, packaging, and supported-platform evidence.

## Runtime Module Boundary

Runtime compiler discovery, dynamic loading, target selection, compilation, diagnostics, and result ownership live in the focused `src/shader` module rather than the DRP2 pipeline implementation. DRP2 and vklite may consume the service but do not own its process-global loader policy.

The module may live in `libdatoviz`, but it must not create a hard link or startup dependency on shared shaderc for ordinary rendering. On platforms using a shared provider, initialization must lazy-load the approved runtime library and degrade to an unavailable capability without breaking import, startup, or precompiled-SPIR-V rendering. A platform may use a reviewed static provider when packaging constraints require it.

Initialization must be thread-safe and idempotent. Do not retain unsynchronized mutable `loaded` and `available` flags. The provider library may remain resident for the process lifetime when required to keep resolved symbols valid.

Discovery and diagnostics must distinguish:

1. Datoviz built without the shader compiler adapter;
2. adapter built but runtime provider absent;
3. provider found but incompatible or missing required symbols;
4. invalid compile request;
5. GLSL compilation failure;
6. successful compilation.

Official v0.4 wheels and release packages must bundle or otherwise guarantee the exact runtime shaderc provider used by installed tutorial consumers. This approval covers the runtime library, not the `glslc`, `glslangValidator`, or full Vulkan SDK toolchain. Custom source builds may disable runtime compilation and continue using precompiled SPIR-V.

## Public API Outcome

The typed `dvz_shader_compile()` API is the primary tutorial contract. The string-only `dvz_compile_glsl()` surface remains a narrow compatibility convenience over it.

The general API must provide:

1. a shader-stage enum rather than accepting arbitrary stage strings as the primary contract;
2. source bytes with an explicit size;
3. the real source filename for diagnostics;
4. an explicit entry point with `main` as the ordinary default;
5. a named Datoviz target profile rather than raw duplicated shaderc constants;
6. a result status that distinguishes availability, request, provider, and compilation failures;
7. actionable owned or caller-provided diagnostics;
8. owned aligned SPIR-V bytes with an unambiguous `dvz_memory_free()` contract;
9. an availability or preflight query that is safe before GPU initialization;
10. a file-oriented convenience or null-terminated text reader suitable for external tutorial shaders.

The accepted public surface is declared in `include/datoviz/shader.h` and uses `DvzShaderCompileRequest`, `DvzShaderCompileResult`, `DvzShaderCompileStatus`, `DvzShaderStage`, `DvzShaderProfile`, `dvz_shader_compiler_status()`, `dvz_shader_compiler_available()`, `dvz_shader_compile()`, and `dvz_shader_compile_result_destroy()`. It does not expose shaderc types or require a GPU context.

Audit every existing caller when the API changes. Correct allocator mismatches, especially callers using C `free()` for Datoviz-owned SPIR-V, and add focused lifetime tests. Public header or binding changes require `just ctypes` and `just ctypes-check`.

## Diagnostics

Compilation diagnostics must identify the requested source filename and stage, preserve useful shaderc line and column information, and explain provider discovery failures with the relevant configuration or environment override. The tutorial must include one deliberate shader failure so a reader sees the diagnostic contract before debugging a larger pipeline.

Do not log a generic hardcoded source name such as a fixture path for unrelated user shaders. Do not require users to inspect internal logs to distinguish an unavailable compiler from invalid GLSL; the public result must carry that distinction.

## Validation

Implemented local proof covers items 1-7 below. RC3 still requires official-package and supported-platform proof for items 8-10:

1. focused tests for valid vertex, fragment, and compute compilation;
2. tests for empty source, invalid stage, malformed GLSL, real source filenames, absent provider, incompatible provider where practical, and correct result cleanup;
3. a concurrency test or equivalent proof for first-use initialization;
4. equivalent profile selection in build-time and runtime compilation;
5. scene, fixture, and example shader builds through the shared `glslc` helper, with a separate Canvas build proving no command-line shader compiler dependency;
6. `spirv-val` proof for generated shader products in hosted CI and release lanes;
7. source-build proof with runtime compilation enabled and disabled;
8. installed CMake-consumer proof against official packages;
9. packaged runtime shaderc proof on Linux, macOS, and Windows;
10. deterministic offscreen and bounded live GLFW tutorial proof with Vulkan validation.

RC4 repeats installed exact-artifact compilation and every tutorial chapter against the frozen API and packaged provider. Final v0.4.0 accepts only blocker or feedback-driven shader-toolchain changes.

## Deferred

- Runtime shader hot reload.
- Shader compilation caches.
- Runtime `#include` callbacks or a general virtual shader filesystem.
- Reflection and automatic descriptor or vertex-layout generation.
- Slang, HLSL, WGSL, and multiple compiler backends in the native tutorial path.
- A separately distributed general shader-compiler plugin ABI.
- Automatic shader optimization beyond deterministic compiler defaults and explicitly reviewed release flags.
