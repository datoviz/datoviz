# Architecture Specs

This directory owns durable cross-cutting architecture rules that are broader than one subsystem
but still normative enough to guide implementation and tests.


## Index

1. [MODULE_LAYERS.md](MODULE_LAYERS.md): source-module dependency layers, promotion rules, and
   reusable subsystem boundaries.
2. [SCENE_SPLIT_REFACTOR_PLAN.md](SCENE_SPLIT_REFACTOR_PLAN.md): implemented controller/internal-scene boundary plus durable criteria for optional future primitive promotion.
3. [EXTERNAL_DEPENDENCIES.md](EXTERNAL_DEPENDENCIES.md): policy for optional third-party
   dependencies such as UI integration layers.
4. [SHADER_TOOLCHAIN.md](SHADER_TOOLCHAIN.md): build-time `glslc`, runtime shaderc, SPIR-V validation, public compilation API, packaging, and RC3/RC4 proof contract.
5. [REACTIVE_APPLICATION_STATE.md](REACTIVE_APPLICATION_STATE.md): exploratory v0.5+ application-state, transaction, endpoint, binding, GUI-projection, and ownership architecture.
6. [GUI_EXTENSIONS_AND_DOCKING.md](GUI_EXTENSIONS_AND_DOCKING.md): target ImGui/cimgui/ImPlot/cimplot ownership, native C boundary, offline dependency family, and declarative docking layout contract.


## Boundary

Put a document here when it constrains multiple source modules, build targets, public headers, or
release artifacts. Keep subsystem-specific contracts in their owning directory, such as `drp2/`,
`scene/`, `api/`, `bindings/`, `build/`, or `testing/`.
