# Future Scene Proposal Notes

These files are exploratory, v0.5+, or pressure-test roadmaps. They preserve direction without
making v0.4 implementation depend on speculative public APIs.

## Overview

1. [LONG_TERM_AMBITION.md](LONG_TERM_AMBITION.md): broad long-term renderer, runtime, scale,
   ecosystem, and GSP/VisPy2 ambition note.
2. [SCIENTIFIC_VISUALIZATION_ROADMAP.md](SCIENTIFIC_VISUALIZATION_ROADMAP.md): orientation index
   for future scientific data models and pressure tests.
3. [FIGURE_GUIDES.md](FIGURE_GUIDES.md): coordinate-reference, span, and guide-mark model for
   future cross-panel annotations linked to panel domains and controllers.
4. [INTEROPERABILITY_ARCHITECTURE.md](INTEROPERABILITY_ARCHITECTURE.md): post-v0.4 architecture, ecosystem and platform pressure map, prioritized milestones, and promotion gates for external frames, data, targets, hosts, queries, remote sources, scene interchange, and optional providers.
5. [VISUAL_MODIFIERS.md](VISUAL_MODIFIERS.md): v0.5+ visual-wide and item-state modifier semantics, capability rules, and reactive-endpoint pressure.


## Semantic Resources

1. [GRAPH_NETWORK_DESIGN.md](GRAPH_NETWORK_DESIGN.md): graph topology, layout, node/edge identity,
   and graph-specific picking.
2. [UNSTRUCTURED_GRID_DESIGN.md](UNSTRUCTURED_GRID_DESIGN.md): volumetric cell topology, attached
   fields, cut surfaces, and derived mesh views.
3. [ATTRIBUTE_SET_API.md](ATTRIBUTE_SET_API.md): future shared visual-attribute bundle API.


## Field And Domain Roadmaps

1. [SAMPLED_FIELD_VIEWS_AND_INCREMENTAL_UPDATES.md](SAMPLED_FIELD_VIEWS_AND_INCREMENTAL_UPDATES.md): staged optional RC4 sampler and sparse-update foundations followed by v0.5+ binding-local views, streaming handoff, and GPU field-displaced structured meshes.
2. [FIELD_VISUALIZATION_ROADMAP.md](FIELD_VISUALIZATION_ROADMAP.md): vector, tensor, categorical-label, sparse, and bricked field directions.
3. [DOMAIN_RESOURCE_ROADMAP.md](DOMAIN_RESOURCE_ROADMAP.md): tracks, trajectories, ensembles, uncertainty, and molecular or structural-biology resources.


## Resource Management And Frame-Plan Pressure

1. [OUT_OF_CORE_PROGRESSIVE_DESIGN.md](OUT_OF_CORE_PROGRESSIVE_DESIGN.md): page/chunk residency and
   progressive upload behavior.
2. [SCENE_COMPUTE_TASK_ROADMAP.md](SCENE_COMPUTE_TASK_ROADMAP.md): staged path from the v0.4
   compute-to-graphics slice to retained scene compute tasks that lower into `DvzFramePlan`.
3. [SPLATTING_FRAME_PLAN_REQUIREMENTS.md](SPLATTING_FRAME_PLAN_REQUIREMENTS.md): non-normative
   frame-plan pressure from scalable Gaussian splatting.
4. [UNIFIED_RAY_RENDERING.md](UNIFIED_RAY_RENDERING.md): future panel-level ray integration path.
5. [RAY_TRACING_FORWARD_COMPAT.md](RAY_TRACING_FORWARD_COMPAT.md): non-v0.4-blocking ray-tracing
   compatibility rationale; current preparation lives in lighting, frame-plan, and unified ray
   rendering specs.
6. [COMPOSITION_LAYER_RENDERING.md](COMPOSITION_LAYER_RENDERING.md): post-v0.4 plan for preserving
   underlay/data/overlay ordering across opaque and transparent render passes.
