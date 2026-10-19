# Panel planning and render-product reuse

Status: implementation proposal. Native panel diagnostics are implemented. A composition-only CPU cache was evaluated and not retained; retained GPU panel images are not enabled. The normative invalidation rules remain in [INVALIDATION_AND_CACHING](../pipeline/INVALIDATION_AND_CACHING.md).

## Decision

Separate immutable CPU composition facts from completed GPU panel products. A reused composition snapshot saves CPU resolution work; it does not establish that a panel image exists or remove GPU draws. Reusing a rendered product requires a retained execution session and an explicit successful-submission acknowledgement.

Retain panel-local final color products within the existing typed frame graph, then compose into each acquired target. Do not infer preservation from a borrowed target ID: canvas images rotate, same-size recreation changes their contents, and the current final-target path can clear the whole attachment. Do not add a parallel renderer or infer semantic products from generated resource names.

## CPU boundary

The first experiment cached only the value-owned `DvzPanelCompositionSnapshot`, after current visual classification and before graph assembly. Its exact normalized input signature covered all facts read by the resolver, including capabilities, placement, technique settings, visual pass participation, authored ordering, transparency groups and attachment requirements. Camera matrices, draw counts and data descriptors were refreshed normally. Graph assembly, dynamic uploads, compute, retirement, conversion and validation continued on every required frame.

Eight alternating fresh-process runs, with three 240-frame trials per process, measured median source-frame time of 3.61 ms without that cache and 3.70 ms with it. Separate diagnostic samples put total composition resolution near 57 microseconds across four panels versus approximately 383 microseconds of panel planning; the cache reduced resolution work but did not establish an end-to-end gain. Four focused CPU tests passed, including dynamic descriptor refresh and maximum key bounds. Preserve the experiment as local evidence rather than adding its state and maintenance cost to the renderer.

Before expanding cross-frame caching, measure repeated same-frame composition validation, fixed-capacity initialization/copies, accumulated-node searches during typed binding, and repeated prefix graph validation. Removing deterministic redundant work may be a simpler improvement than adding invalidation machinery.

Do not retain `DvzPanelRenderPlan` wholesale: it contains live visual, attachment and technique pointers. A later topology-fragment cache must retain stable object identities and normalized structural facts, refresh dynamic descriptors, and relocate plan-local product/pass/node references during figure assembly. Adopt that larger cache only when counters show that classification or graph assembly merits the additional lifecycle machinery.

Cache misses, allocation failures and uncertain dependencies take the existing resolution path. Failed resolution never replaces a valid entry. Diagnostic-bearing resolutions require equivalent diagnostic replay or remain uncached. A cache key must compare complete input values; a hash alone is not proof of equality. Cache lifetime is bounded by its owning panel/scene and cannot affect authoritative source lifetimes.

## GPU product contract

| Concern | Required behavior |
| --- | --- |
| Eligibility | Prove the panel subgraph is isolated. Backdrop-dependent blending, cross-panel shared depth, unsupported role ordering, undeclared external writes and unknown dependencies use full rendering. Disjoint rectangles alone do not prove isolation. |
| Product | Declare persistent final panel color and its imports/consumers through existing typed products, lifetimes and graph bindings. Do not make all transient technique intermediates persistent. |
| Ownership | Initially scope cache images to runtime/view/frame slot, reusing existing slot admission and completion. No scene ownership of borrowed canvas handles. |
| Compatibility | Include logical identity, runtime epoch, target generation, dimensions, scales, format, samples, color pipeline and relevant capabilities. |
| Content | Include view transforms, data and derived resources, counts, style/material, lights, clips, annotations, hover/selection, technique parameters and resource generations. |
| Shared state | Track source revisions and per-consumer acknowledgements. Globally cleared dirty flags and figure-wide frame revisions cannot establish another consumer's cache validity. |
| GPU writes | Execute compute normally. Invalidate panels that consume written resources on each dispatch unless an explicit dependency contract proves unchanged content. Unknown dependencies disable reuse. |
| Composition | Compose valid products into the current target while preserving existing clear, color, alpha, depth and role-order semantics. Arbitrary overlapping panels retain the original path until equivalence is established. |
| Placement | Treat placement separately from content only after proving shaders do not depend on target origin. Otherwise placement invalidates content. |
| Queries | Cached presentation color does not satisfy object-ID/depth queries. Query execution and result routing retain their freshness contracts. |
| Capture/export | Read current fully composed output; changed export extent or pipeline requires compatible products or full rendering. |
| Artifacts | Stateless emission cannot assume an earlier artifact executed. Without retained-session acknowledgement, emit complete rendering. Recording/replay must include all creation/content dependencies. |

A prototype exposed an additional prerequisite: outward-rounded panel allocations and fractional logical viewports must share one explicit texel mapping. Preserving the fractional producer viewport and changing presentation fetch alone is insufficient: AO/EDL/transparency resolves and reconstruction still use allocation-normalized UVs. Declare allocation origin/extent, content viewport and coordinate invariance in normalized facts shared by producers, every sampled consumer and presentation. A 2D panel or a source-over-only pass is not proof of coordinate invariance; shaders may depend on absolute fragment coordinates. Derive allocation and consumer mapping from the same actual framebuffer extent and scale. Keep the current paths until that contract and fallback are validated in both shader transports.

## Submission state

A cache entry progresses through invalid, recorded/pending and submitted states. Planning, conversion and command recording are not successful submission. A pending entry records the intended dependency signature and owner generation; only the execution owner can acknowledge that it was sent. Existing queue ordering may make a successfully submitted producer usable by later work without a CPU wait, but producer/consumer dependencies must remain explicit.

Unsent work never advances cache validity. Failed planning, encoding, runtime execution, abandoned recording and unsent submission preserve the previous committed state or invalidate it conservatively. Runtime recovery and resize invalidate pending entries and any images whose storage or content is no longer guaranteed. Destroy/recreate cache storage only through existing completion-aware resource retirement.

With three slots, an unchanged panel initially renders once per slot. Sharing one image across slots is a later optimization requiring explicit write-after-read ordering and retirement; it is not necessary for the first implementation.

## Measurements and acceptance

Diagnostics distinguish planned work, successful emission, actual submission and completed/displayed work. Record panel-local resolution/classification time, graph assembly, cache hits/misses and reasons, node/draw counts, parameter/resource uploads, key/snapshot bytes and retained image memory. Do not label planned render nodes as completed GPU passes. Whole-figure frame revision changes without a known panel-local reason must remain unattributed rather than being reported as a data change.

The representative workload is a four-panel scientific navigator with one rotating 3D panel and three static image/annotation panels. Measure fresh-process matched builds, fixed data, dimensions and input trajectory, warmup, alternating enabled/disabled order, and both CPU and GPU time. Keep profiler overhead separate from uninstrumented timing. Retain an optimization only when representative gains justify key preparation, copying, composition and memory costs.

Required comparisons against forced full rendering:

- Repeated camera motion with unchanged topology; changed MVP with stable composition hits.
- Data/count, style, visibility, clip, ordering, capability and technique changes; resolution failure and retry.
- Shared visuals/resources consumed by two figures/views in either order.
- Every frame slot's first use, same-size recreation, resize with pending work, hide/show, panel removal/movement and overlap fallback.
- Stationary hover, selection, crosshair, slice updates, compute-fed resources, queries, capture and export at another resolution.
- Allocation failure, failed conversion, abandoned recording and unsent submission followed by retry.
- Pixel equality where deterministic, focused native synchronization validation, and valid retained-session resource behavior for other artifact consumers.

The measured unchanged slice work is an opportunity, not a guaranteed saving: extra composition and storage must be charged to the optimized path. Physical desktop pacing and input-to-display latency require separate measurements.
