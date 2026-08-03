# Public Documentation Visual Pilot Review

Status: four-page pilot implemented and awaiting maintainer review; broad rollout has not started. Updated: 2026-08-03.

The completed pilot establishes a reusable visual language for public conceptual documentation without changing scene/runtime semantics or adding a diagram dependency. Review the rendered site at desktop and mobile widths before applying the system broadly.

## Pilot Surface

| Page | Implemented visual |
| --- | --- |
| `docs/start/index.md` | Two-column real-output hero, primary actions, and four-step first path. |
| `docs/start/core-concepts.md` | Responsive nested scene/figure/panel/visual/data schematic with controller ownership. |
| `docs/start/choose-your-layer.md` | Supported/experimental/advanced layer and decision map including the external GSP/VisPy2 boundary. |
| `docs/advanced/index.md` | Canonical scene-to-output architecture map and audience routing cards. |

Reusable components live in `docs/stylesheets/content.css`: `dvz-doc-hero`, `dvz-output-example`, `dvz-step-flow`, `dvz-layer-diagram`, `dvz-object-diagram`, `dvz-sequence`, and `dvz-audience-grid`.

## Maintainer Review Needed

Please review:

1. visual density and whether each page has at most one dominant teaching visual;
2. cyan/mint/amber/rose status vocabulary and accessibility without color;
3. architecture and ownership labels against current scene, FramePlan, DRP2, native Vulkan, and browser WebGPU boundaries;
4. desktop and mobile flow, overflow, and scanning behavior;
5. use of real Datoviz output, captions, alt text, and source links;
6. whether the same component language should roll out to the remaining Get Started, Advanced, and Explanation pages.

## Approved Design Constraints

1. Use HTML/CSS for responsive flows, stacks, cards, status blocks, and simple containment diagrams; use editable hand-authored SVG only when spatial structure demands it.
2. Use real existing gallery output rather than mockups or AI-generated technical imagery.
3. Keep code and tables when they are clearest; pair code with output rather than replacing exact reference material.
4. Always include text status labels and nearby prose equivalents; color and diagrams are not sole carriers of meaning.
5. Reuse the current MkDocs asset path and Material icons; do not add Mermaid, Graphviz, PlantUML, another icon package, or a page-specific styling framework.
6. Keep generated API pages primarily textual and change them only through their generator or metadata source.
7. Do not edit or commit the `data` submodule, generated site output, build-local media, videos, or binaries without exact approval.

## Rollout After Approval

Apply the reviewed vocabulary first to the remaining Get Started pages, then to the Advanced/Explanation architecture, runtime, DRP2, frame-lifecycle, retained-resource, invalidation, ownership, coordinate, interaction, query, and GSP/VisPy2 boundary pages. Add a visual only for a visible result, spatial/ownership relationship, ordered state transition, or meaningful audience/API/platform choice.

Run `just docs-build-check`, `just docs-status-check`, `git diff --check`, and desktop/mobile rendered inspection for each coherent checkpoint. Publication remains a separate approval action.
