# Scene Picking

> **Status:** normative scene picking model for v0.4.
> **Authority:** this file defines scene identity, request routing, result freshness, and
> latest-wins semantics for picking. Controller routing is defined in
> [`CONTROLLERS.md`](CONTROLLERS.md); diagnostics use
> [`../validation/DIAGNOSTICS.md`](../validation/DIAGNOSTICS.md).


## Purpose

Picking is a scene-side semantic feature, not a backend readback detail. It lets panel-local
interaction identify scene objects while preserving visual, item, group, and family-defined
identity across batching and the scene -> DRP2 -> runtime boundary.


## Core Rules

1. Picking returns scene identity, not backend identity.
2. Every request is panel-local and anchored to the requesting panel.
3. Results must identify the request they answer and be freshness-checkable before mutation.
4. Hover uses latest-request-wins semantics; stale hover results are discarded silently.
5. Click and explicit query requests may require stronger delivery guarantees than hover.
6. Controllers receive interpreted scene-level results, not encoded GPU payloads.


## Identity Model

| Level | Meaning | Required when |
|---|---|---|
| Scene | owning scene and routing boundary | usually implicit, but part of async routing |
| Panel | requesting panel and view state that produced the hit | always |
| Visual | visual instance that produced the contribution | always for visual hits |
| Family | semantic family that defines result interpretation | always for visual hits |
| Item | one logical item in a flat item table | point, marker, segment, sphere, pixel-like hits |
| Group | one logical group in grouped resources | path, glyph/text, traces, labels, grouped meshes |
| Auxiliary | family-defined local payload | vertex/glyph/primitive id, coordinates, sampled values, hierarchy ids |

Future scientific targets such as graph nodes/edges, cells, voxels, labels, tracks, atoms,
residues, chains, and ensemble members must resolve to scene/domain identity. Auxiliary data should
carry local coordinates needed for readout, such as barycentric cell coordinates, UVW/voxel index,
edge parameter, time/sample id, or molecule hierarchy ids.


## Request And Result Shape

| Concept | Required semantic content |
|---|---|
| Request | stable `request_id`, `panel_id`, kind (`hover`, `click`, `query`), panel-local position or sample coordinate, scene/panel revision for freshness |
| Result | request id, panel id, visual id, family id, item id, group id, optional auxiliary payload, hit-valid flag, freshness disposition |
| Hover policy | one current hover request per panel may supersede older requests; apply only if request id and generation still match |
| Click/query policy | synchronous or stronger completion may be used when the caller needs a stable immediate result |

The final public C structs may change, but these fields are stable semantic requirements.


## Family Expectations

| Family / object | Default identity | Auxiliary payload notes |
|---|---|---|
| `pixel` | visual id + item id | sampled-value or pixel coordinate may be exposed by query-oriented paths |
| `point` | visual id + item id | none required |
| `marker` | visual id + item id | marker-shape detail may be added later |
| `segment` | visual id + item id | none required |
| `path` | visual id + group id | optional vertex/item payload; default hit is the logical path |
| `glyph` / text | visual id + group id | glyph index only for low-level workflows |
| `image` | visual id + optional image item id | local image coordinate or sampled value may be returned |
| `mesh` | visual id + semantic region/group when declared | primitive id is auxiliary and must not replace stable region identity |
| `sphere` | visual id + item id | applies even for impostor-first rendering |
| `volume` | visual id + family-defined payload | slice probe/readout follows [`../visuals/VOLUME.md`](../visuals/VOLUME.md); DVR/MIP ray-cast identity remains deferred |
| axes/annotations | owning axis or annotation id + optional component detail | does not require axes to become primitive visuals |

Grouped resources must preserve group identity by default so batching does not erase logical object
identity.


## Timing, Coalescing, And APIs

Picking may be requested immediately during interaction and delivered after the relevant frame
completes. The scene must be able to discard a result without ambiguity when the request, panel,
visual, or generation no longer matches current state.

| Pick kind | C API style | Delivery rule |
|---|---|---|
| hover | async callback/controller path | latest request wins; stale results discarded |
| click | synchronous blocking helper is allowed | result maps to the issuing request or reports no hit/error |
| explicit query/probe | synchronous or tool-owned async | may expose richer diagnostics, coordinates, and sampled values |

The synchronous click/query surface is a runtime-service wrapper over DRP2 readback completion, not
a DRP2 protocol change.


## FramePlan, Invalidation, And Capabilities

Picking is visible planning work. A picking-enabled frame may add a picking target, render node,
readback node, resource dependencies, and request metadata to route the result.

Always-on and on-demand picking are both valid policies, but the chosen policy must be explicit.
Changes follow [`../pipeline/INVALIDATION_AND_CACHING.md`](../pipeline/INVALIDATION_AND_CACHING.md):

| Change | Likely consequence |
|---|---|
| enabling picking on a visual | visual props, frame plan, and readback routing dirtiness |
| changing payload shape | picking pass and readback routing dirtiness |
| changing only pointer position | new request without full normalization or topology rebuild |

Readback and capability constraints are handled through
[`../validation/ADAPTATION.md`](../validation/ADAPTATION.md) and
[`../../drp2/CAPABILITIES.md`](../../drp2/CAPABILITIES.md). The scene must not claim picking support
when stable identity round-trip is unavailable.


## Selection And Hover State

Picking results update scene-owned hover or selection state through scene transitions:

1. request is issued;
2. picking work is planned and executed;
3. result is interpreted as scene identity;
4. stale results are discarded if superseded or generation-mismatched;
5. hover or selection mutates;
6. redraw is requested if visible state changed.


## Diagnostics

Picking diagnostics should report which visuals participate, which grouped families return group
ids, whether a result was stale, which policy is active for each panel, and which `FramePlan` nodes
exist because of picking. Use the schema in
[`../validation/DIAGNOSTICS.md`](../validation/DIAGNOSTICS.md).


## Non-Goals

This document does not define the exact encoded GPU payload, render-target format, latency policy,
final C struct names, or backend implementation commands.
