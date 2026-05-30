# Architecture Specs

This directory owns durable cross-cutting architecture rules that are broader than one subsystem
but still normative enough to guide implementation and tests.


## Index

1. [MODULE_LAYERS.md](MODULE_LAYERS.md): source-module dependency layers, promotion rules, and
   reusable subsystem boundaries.
2. [SCENE_SPLIT_REFACTOR_PLAN.md](SCENE_SPLIT_REFACTOR_PLAN.md): concrete plan for promoting
   scene-independent primitives out of the retained scene layer.
3. [EXTERNAL_DEPENDENCIES.md](EXTERNAL_DEPENDENCIES.md): policy for optional third-party
   dependencies such as UI integration layers.


## Boundary

Put a document here when it constrains multiple source modules, build targets, public headers, or
release artifacts. Keep subsystem-specific contracts in their owning directory, such as `drp2/`,
`scene/`, `api/`, `bindings/`, `build/`, or `testing/`.
