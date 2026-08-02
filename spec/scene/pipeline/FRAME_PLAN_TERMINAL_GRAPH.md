# FramePlan Terminal Graph Debug View

## Status

Approved internal debug contract. This document defines the desired terminal-readable `FramePlan` graph surface. It is implementation-facing and does not define public scene semantics or a stable render-graph API.


## Purpose

The terminal graph view gives developers a fast way to inspect frame-planning topology without
opening JSON, Graphviz, or backend traces.

It should answer:

1. which passes and resources exist in one `FramePlan`,
2. why each pass or resource appears,
3. which resources connect producer passes to consumer passes,
4. where uploads, copies, and readbacks enter the plan,
5. whether graph-backed techniques emit the expected ordering.


## Core Rules

1. The view is a debug rendering of the scene-owned `FramePlan`; it must not expose backend handles,
   command buffers, image views, swapchains, or Vulkan/WebGPU objects.
2. The textual output must be deterministic for a deterministic `FramePlan`.
3. The view must be generated from `FramePlan` nodes, semantic products, graph resources, graph passes, and inferred graph dependencies, not from the lowered DRP2 runtime state.
4. The graph view is allowed to simplify dense graphs, but any hidden edge must still appear in a
   plain dependency list below the drawing.
5. The default view should fit ordinary single-panel plans in a 120-column terminal.
6. The debug API may start internal-only. Public exposure can be decided after the output format has
   survived fixture and live-debug use.


## Vocabulary

| Shape | Meaning |
|---|---|
| `[node]` | Executable work: upload, compute, render, copy, or readback |
| `{product}` | Semantic render product with typed plan-local identity |
| `(resource)` | Physical logical buffer, texture, attachment, external target, or readback resource |
| `──>` | Data dependency or resource flow |
| `│`, `▼` | Continuation of a data dependency |
| `...` | Deliberate elision of repeated or too-wide content |

Use UTF-8 box/arrow characters by default. Provide an ASCII-only mode for CI logs or terminals that
do not render UTF-8 reliably.


## Legacy R0 Characterization Example

The following drawing records the current role- and resource-label-based SSAO graph so R0 diagnostics can recognize and remove it. It is not the approved composition target: the `ssao_composite` black-overlay pass and string-derived resource identity must disappear during R1-R7.

The current legacy view is a vertical, resource-mediated DAG sketch:

```text
FramePlan figure=fig0 frame=42

[upload #0]
id: point.position
bytes: 48 KB
        │
        ▼
(point.position)
kind: buffer
usage: vertex
        │
        ▼
[render #2]
role: opaque
panel: panel0
visuals: points0
        │             │
        ▼             ▼
(rt)              (panel0.depth)
color             depth
        │             │
        │             ▼
        │       [render #3]
        │       role: ssao
        │       panel: panel0
        │             │
        │             ▼
        │       (panel0.ssao)
        │       usage: sampled
        │             │
        └─────────────▼
              [render #4]
              role: ssao_composite
              panel: panel0
                    │
                    ▼
                  (rt)

Edges:
  render #2 -> render #3 via panel0.depth depth_attachment_write -> sampled
  render #3 -> render #4 via panel0.ssao color_attachment -> sampled
```

The drawing is informative and optimized for humans. The `Edges:` section is the deterministic
fallback that preserves exact dependencies.


## Approved Semantic Target

The terminal view must ultimately expose products independently of their physical texture aliases and show AO as a lighting input rather than a color composite:

```text
[surface capture]
  ├──> {surface_depth} ────┐
  ├──> {surface_normal} ───┼──> [ambient visibility]
  └──> {surface_coverage} ─┘              │
                                         ▼
                              {ambient_visibility}
                                         │
                                         ▼
                                [opaque shading]
                                consumes ambient only
```


## Node Metadata

The compact view should keep each box to roughly four lines.

| Node kind | Compact fields |
|---|---|
| Upload | plan index, resource id, byte size, data tag when present |
| Compute | plan or graph index, shader key/work label, dispatch, read/write counts |
| Render pass | index, pass role or graph pass id, panel id, visual count or short visual list |
| Copy | plan index, source, destination, byte size, request id when present |
| Readback | plan index, resource id, request id |
| Product | typed id, semantic kind, validity, samples/resolve, producer and consumers |
| Resource | id, concrete format, usage summary, lifetime, extent when non-obvious |

Verbose mode may add:

1. complete read/write sets,
2. color/depth/stencil attachment load, store, and access modes,
3. viewport and scissor rectangles,
4. sample count and format,
5. pass work labels and graph pass ids,
6. visual id lists with truncation after a small fixed count.

Long labels must be truncated deterministically, preserving enough suffix or prefix to identify
panel-local resources. Example: `panel0.depth_peel.front_accum.2` may become
`panel0.depth_peel...2`.


## Layout Requirements

The first implementation should prefer reliability over clever layout.

1. Order executable nodes by `FramePlan` order and graph passes by graph pass order.
2. Represent graph dependencies through resources whenever this fits cleanly.
3. Group obvious fan-out and fan-in patterns such as color/depth outputs from an opaque pass.
4. Collapse repeated pass families when the expanded graph would exceed the configured width, for
   example `depth_peel_iter x4`.
5. Append every dependency to the plain edge list, including dependencies that were also drawn.
6. If no readable drawing can be produced, emit only compact node blocks plus the edge list.


## API Direction

The preferred initial implementation is internal:

```c
char* dvz_frame_plan_graph_ascii(const DvzFramePlan* plan, uint32_t flags);
```

Candidate flags:

| Flag | Effect |
|---|---|
| `COMPACT` | Minimal metadata, default width target |
| `VERBOSE` | Include attachments, read/write sets, viewport/scissor, and formats |
| `SHOW_UPLOADS` | Include upload nodes and their resource links |
| `SHOW_READBACKS` | Include copy/readback tail nodes |
| `ASCII_ONLY` | Replace UTF-8 drawing glyphs with `-`, `|`, `v`, and `>` |
| `MAX_WIDTH_120` | Keep the main drawing within 120 columns when possible |

Do not commit to public names until at least one implementation slice and fixture set uses the
debug view.


## Relationship To Existing Debug Surfaces

| Existing surface | Role |
|---|---|
| `dvz_frame_plan_json()` | Complete structured serialization for fixtures and diffing |
| `dvz_frame_plan_graph_dump()` | Structured graph pass and dependency dump |
| Terminal graph view | Human-readable topology sketch for logs, failures, and live debugging |
| DRP2 stream JSON | Lowered command-stream validation after FramePlan emission |

The terminal graph view should never replace JSON fixtures. It is an inspection aid.


## Acceptance Criteria

The first useful implementation should include focused tests for:

1. one opaque pass with color and depth resources,
2. SSAO or EDL postprocess fan-out/fan-in,
3. WBOIT or depth-peeling graph simplification,
4. picking or image-probe copy/readback tail,
5. ASCII-only output,
6. deterministic output across repeated generation from the same `FramePlan`.
