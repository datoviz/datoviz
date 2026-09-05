# AGENTS.md - Scene Spec Writing Rules

These rules apply to documents created or edited under `spec/scene/`.

The goal is to keep the scene spec useful as an implementation guide. Prefer short, authoritative documents over exhaustive notes. Remove duplication when editing nearby text, and link to the canonical source instead of restating it.

## General Rules

- Keep documents short, direct, and scoped to their category.
- Do not add boilerplate sections just because another file has them.
- Prefer tables, compact bullet lists, and links over repeated explanatory prose.
- Avoid long pseudocode, speculative API sketches, or implementation recipes unless the document is explicitly an implementation note or API example.
- State the document's authority clearly: normative, proposal, informative, historical, or roadmap.
- Preserve important design decisions, invariants, unresolved choices, and acceptance criteria.
- Delete or compress duplicated rationale when the rule already lives in a canonical spec.
- Use `README.md` files as indexes and routing guides, not as duplicate summaries of every child document.
- Keep examples informative. They pressure-test the design; they do not override normative specs.

## Canonical Design Specs

Use for `core/`, `api/`, `semantics/`, `pipeline/`, `interaction/`, `validation/`, `export/`, and most `integration/` documents.

Preferred structure:

1. Status / authority
2. Purpose
3. Core rules
4. Vocabulary or concepts
5. Required behavior
6. Boundaries and non-goals
7. Relationship to nearby specs
8. Open questions, only when still actionable

Keep these documents normative and compact. Move repeated lifecycle, invalidation, resource, diagnostic, and runtime-boundary prose to the canonical document for that topic.

## Proposals

Use for `proposals/active/`, `proposals/promoted/`, `proposals/future/`, and `proposals/history/`.

Preferred structure:

1. Status
2. Decision or question addressed
3. Short summary
4. Chosen direction or options
5. What moved into canonical specs
6. Remaining unresolved points

Promoted proposals should be short records. Once rules move into canonical specs, do not keep a second full copy in the proposal.

## Examples

Use for `examples/`.

Preferred structure:

1. Summary
2. User-visible result
3. Feature pressure points
4. Required data and resources
5. Minimal implementation target
6. Validation / acceptance criteria
7. Links to shared policies or canonical specs

Do not repeat generic cache/download policy, "API not final" caveats, FramePlan/DRP2 boilerplate, or agent pickup instructions in every example. Put shared guidance in a category README, template, or canonical policy document, then link to it.

## Implementation Notes

Use for `implementation/` and implementation-ready `slices/`.

Preferred structure:

1. Status
2. Current implementation boundary
3. Design constraints
4. Internal data flow
5. Failure, lifetime, or performance cases
6. Tests and diagnostics
7. Known gaps

Implementation notes may be more concrete than semantic specs, but they should not redefine public semantics.

## Integration Docs

Use for host, UI, platform, and external-ecosystem integration notes.

Preferred structure:

1. Status / authority
2. External owner versus Datoviz owner boundary
3. Required host capabilities
4. Data and control flow
5. Runtime constraints
6. Minimal milestone
7. Open integration risks

Keep integration docs focused on what is unique to that integration. Link to shared high-DPI, threading, hosted-backend, picking, resource, and runtime-boundary specs instead of duplicating them.

## Compression Checklist

Before finishing a `spec/scene` edit, ask:

1. Can this section be replaced by a link to the canonical document?
2. Does this repeat generic scene/backend boundary text?
3. Does this repeat invalidation, resource, diagnostic, cache, or capability policy?
4. Is a code block or API sketch necessary for the decision being recorded?
5. Would a table preserve the same information in fewer lines?
6. Is the document still clear if every paragraph that only says "why this matters" is removed?

If the answer points to duplication, compress the text during the edit.
