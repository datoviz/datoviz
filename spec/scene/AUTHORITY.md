# Scene Spec Authority

This file records how to read `spec/scene/`, which files win when documents overlap, and which
rules are implementation-facing.


## DRP2 Boundary

The scene layer is a consumer of DRP2. It emits DRP2 commands; it never calls Vulkan, vklite, or
any backend API directly.

The scene spec and the DRP2 spec (`spec/drp2/`) are designed in parallel and feed requirements into
each other:

1. when scene needs a DRP2 capability that does not yet exist, that is a DRP2 spec input, not a
   reason to add a scene-side workaround or backend escape hatch;
2. when DRP2 adds or changes commands, the scene spec should be updated to reflect what scene can
   now express;
3. open scene questions that depend on DRP2 details should stay explicitly open until the relevant
   DRP2 decision is made.

This constraint applies especially to `core/RUNTIME_BOUNDARY.md`, `pipeline/FRAME_PLAN.md`, and
`validation/ADAPTATION.md`.


## Normative Invariants

The current scene spec should be read with these invariants:

1. scene owns high-level semantics and authored state, while runtime owns execution details;
2. exactly one scene-level `FramePlan` is the canonical producer-side execution artifact for a frame
   build, even when it contains panel-local subplans or per-panel nodes;
3. uploads and lazy materialization work should appear in `FramePlan`, not in a separate execution
   side path;
4. identity queries must return scene identity rather than backend identity;
5. hover queries follow latest-request-wins semantics and stale results must be discardable;
6. request processing may coalesce unresolved panel-local query requests, but accepted results
   must still obey public freshness rules;
7. data normalization and panel-local navigation are separate stages;
8. panel navigation should usually not force normalization rebuild or bulk data reupload;
9. compute-derived resources are frame-local by default unless persistence is declared explicitly;
10. legends and colorbars may aggregate only semantically identical mappings unless sharing is
    configured explicitly;
11. capability adaptation must be explicit and deterministic rather than implicit or backend-shaped;
12. validation runs after dirty-scope resolution and before planning;
13. capability adaptation runs after validation and before planning.


## Reading Conventions

Unless a document says otherwise:

1. sections titled `Core Rule`, `Rules`, `Requirements`, `Hard Requirements`, `Normative
   Invariants`, `Current Preferred Direction`, or `Contract` are normative for the current planning
   baseline;
2. sections titled `Purpose`, `Position`, `Why`, `Rationale`, `Examples`, `Deferred Questions`,
   `Open Choices`, `Deferred API Choices`, `What This Document Intentionally Leaves Open`,
   `Immediate Follow-Up`, `Immediate Follow-On Specs`, or `Recommended Next Step` are informative
   unless they explicitly say otherwise;
3. worked examples under `examples/` are informative pressure tests, not independent sources of
   normative behavior;
4. when two documents overlap, the more specialized contract document wins over a broader
   orientation document;
5. `proposals/active/` records are active design addenda used to clarify or extend normative
   documents until those rules are promoted into specialized spec files;
6. `decisions/` records are historical ADR-style records and should explain rationale, not hold
   current implementation-facing rules on their own;
7. `api/API_SURFACE.md` is the normative bridge from scene semantics to public C API shape policy;
8. installed headers under `include/datoviz/scene*.h` are authoritative for names already drafted.


## Status Vocabulary

Scene spec files should use one of these statuses, either explicitly in a `Normative Status` section
or by inheriting the directory role from the nearest `README.md`:

1. `Normative`: implementation-facing rules for the current v0.4 planning baseline.
2. `Proposal`: active design addendum that may contain planning rules not yet absorbed into a
   specialized normative spec.
3. `Informative`: examples, rationale, implementation notes, or trackers that do not override
   normative specs.
4. `Historical Decision`: ADR-style rationale for older choices; not a current standalone source
   of implementation rules.


## Source-Of-Truth Order

For cross-tree overlap, use this source-of-truth order:

1. DRP2 command, lifetime, and error prose for protocol semantics;
2. DRP2 active JSON schemas for machine-checkable command shape;
3. installed scene headers for public names and signatures that already exist;
4. scene normative documents for scene semantics;
5. `spec/scene/api/API_SURFACE.md` for public API shape policy and not-yet-implemented groups;
6. scene proposals for rules not yet promoted into specialized spec files;
7. scene implementation slices for concrete work boundaries that apply those rules;
8. historical scene decision records for rationale behind older choices;
9. examples, deferred trackers, and historical header sketches as informative material.
