# Cached Scientific Video With Native Overlays

Status: future proposal, unimplemented. This is a separate performance feature beyond the [native recording workflow](NATIVE_VIDEO_WORKFLOW.md), not a claim about current overlay-only export capability.

## Question

Can a user revise fonts, labels, branding, and annotation placement without reloading scientific data or redrawing an expensive animation, while keeping composition and final video recording inside Datoviz?

Retained data and command replay still execute scientific drawing. A cached raster image avoids one redraw; a cached clean animation avoids redrawing an entire timeline. [Record/replay](../../../../docs/how-to/record-replay.md) is command replay, not a pixel cache or a durable edited-video interchange format.

## Direction And Boundaries

Use a base-frame source plus retained native presentation annotations, composited through the existing scene renderer and exported through the existing video sink. A source may be a live scientific scene, cached image, or decoded cached video. Reuse [semantic text](../../semantics/TEXT.md), installed overlay and placement primitives, and [composition-layer ordering](COMPOSITION_LAYER_RENDERING.md). Coordinate source/decode work with [external-frame interoperability](INTEROPERABILITY_ARCHITECTURE.md) and the [video-source plan](../../integration/NVDEC_VIDEO_SOURCE_PLAN.md), rather than creating another decoder API here.

The base is an immutable clean scientific recording, with explicit timestamps/frame identities, pixel dimensions, resolved panel rectangles, and named presentation groups. Store synchronized metadata tracks for changing feature names, units, slice coordinates, or other overlay values. A flattened MP4 alone cannot recover those semantics or remove previously burned-in labels.

| Edit | Expected invalidation |
| --- | --- |
| Overlay font, color, text, or position within a fixed presentation region | Overlay layout and drawing only |
| Camera, scientific data, slice, mapping/LUT, or scientific viewport dimensions | Relevant scientific base frames |
| Move a cached panel without changing its pixel dimensions | May reuse panel textures if independently cached |
| Resize a scientific panel or change its reserved space | Rerender base, unless the user explicitly accepts resampling |

Cheap iteration therefore freezes scientific panel geometry. A whole-frame cache cannot independently rearrange panels; that requires per-panel caches and is an optional extension. Include ticks, ramps, and legends in the base or overlay according to an explicit dependency contract: moving text may be presentation-only, but changing the mapped scalar domain or LUT is scientific invalidation.

The first milestone is one cached image plus editable native text and image annotations. Then support bounded streaming of a clean animation, exact deterministic frame selection, and the existing native recording sink. Keep ordinary scene layout and rendering authoritative; no custom external overlay compositor, second encoding subsystem, or general video editor.

## User Workflow

1. Prepare the retained scene, timeline, scientific layout, and shared typography; preview representative states at the intended display size.
2. Record a reusable clean base once, alongside presentation metadata and source identity.
3. Open that base in a lightweight native composition scene, without the scientific arrays. Edit presentation styles, scrub the synchronized timeline, and preview immediately without encoding.
4. Export the final composition directly through native video recording. Always compose from the clean base; do not repeatedly encode a previously annotated result.

Burned-in video revisions still require decoding, composition, and encoding. The promised saving is scientific preparation and rendering, not free export. A lossless or quality-controlled base and explicit alpha, color-space, transfer, range, and scaling contracts are needed to assess fidelity.

## Acceptance Criteria

- An overlay-only edit causes zero scientific draw calls and zero scientific-data uploads; a lightweight composition session can operate without those data resources loaded.
- Fixed frame identities and timestamps keep captions aligned during exact seeks, export, feature changes, and loop seams. Offline export must not silently drop frames to meet wall-clock playback speed.
- Cache provenance records source and layout identity, dimensions, timeline, and format. Stale scientific bases and unsupported resampling are diagnosed rather than reused silently.
- Decode queues and texture rings have bounded memory, explicit ownership, completion synchronization, cancellation, and safe teardown. Follow shared resource contracts rather than keeping all RGBA frames resident.
- Compare cached composition with direct rendering for alpha, color, clipping, text placement, DPI, and frame ordering. Measure time to first preview, edit-to-preview latency, export throughput, peak memory, readback traffic, and scientific preparation avoided.

## Open Decisions

Choose the minimal cache/metadata representation and ownership boundary, lossless image versus encoded-video tradeoffs, exact seek support, and how to identify presentation dependencies. Establish a measured expensive-scene workload before expanding to per-panel caches or GPU decoder acceleration.
