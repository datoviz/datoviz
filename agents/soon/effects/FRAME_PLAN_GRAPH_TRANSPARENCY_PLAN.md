# FramePlan Graph And Transparency Follow-Up

> **Execution Status**
> - **Status:** `ACTIVE / FOLLOW-UP NOTE`
> - **Updated on:** `2026-05-19`
> - **Purpose:** track remaining graph-backed transparency pickup work after durable graph,
>   WBOIT/depth-peeling, and MSAA contracts were split into `spec/scene`.


## Current State

Durable contracts live in:

1. [`../../../spec/scene/implementation/GRAPH_TECHNIQUES.md`](../../../spec/scene/implementation/GRAPH_TECHNIQUES.md)
2. [`../../../spec/scene/implementation/TRANSPARENCY_MSAA.md`](../../../spec/scene/implementation/TRANSPARENCY_MSAA.md)
3. [`../../../spec/scene/semantics/TRANSPARENCY.md`](../../../spec/scene/semantics/TRANSPARENCY.md)

Use this file only for pickup sequencing and validation. Do not duplicate resource/pass graph,
WBOIT, depth-peeling, alpha-mode, or MSAA rules here.

The first WBOIT mesh smoke proved that the scene -> FramePlan -> DRP2 -> vklite path can support
multi-pass rendering, intermediate targets, sampled resolve passes, and reused opaque depth. New
transparency work should continue through typed graph resources and passes rather than adding
technique-specific shortcuts in the scene emitter.


## Remaining Transparency Graph Work

Recommended follow-up commits:

1. Keep the generic FramePlan graph vocabulary authoritative for new transparency resources,
   passes, reads, writes, attachments, load/store policy, and diagnostics.
2. Convert any remaining WBOIT-specific lowering assumptions to generic graph resource/pass
   handling without changing emitted streams.
3. Close transparency-specific DRP2 gaps only when required by graph-backed WBOIT, shell, or
   dual-depth-peeling work: named depth resources, attachment access, graph-derived transitions,
   cull mode, and front-face winding.
4. Preserve the simple opaque path for scenes with no transparent or advanced graph-backed
   technique.
5. Keep user-facing transparency APIs declarative through alpha modes and bounded technique
   descriptors, not public low-level graph construction.
6. Use shell two-pass only as a focused validation step for raster-state and pass-order behavior.
7. Keep dual-depth-peeling execution details in
   [`DUAL_DEPTH_PEELING_PLAN.md`](DUAL_DEPTH_PEELING_PLAN.md).


## Non-Goals

1. No public low-level framegraph API.
2. No resource aliasing or transient memory allocator.
3. No async compute scheduling.
4. No automatic pass reordering beyond explicit validated ordering.
5. No replacement of retained scene APIs with graph construction.


## Validation

For docs-only changes, run:

```text
rg for old moved filenames and stale soon/spec links
git diff --check
git status --short
```

For implementation changes in this lane, use:

```text
just build
just test scene
just test drp2
```

Add focused graph, DRP2, and resize/descriptor-refresh coverage when transparency work changes
sampled intermediate resources, attachment formats, or pass ordering.
