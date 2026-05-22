# Scene Theming and Styling System

## Status

- **Authority:** proposal (active)
- **Date:** 2026-05-22
- **Scope:** scene-level styling semantics and cross-language boundary contract for C, ctypes, GSP,
  Vispy2, and future WASM bindings.


## Decision addressed

Define where theming/styling semantics live, how postprocessing defaults (MSAA/SSAO/future techniques)
remain extendable, and what successive implementation stages should deliver.


## Summary

1. Canonical theming semantics live in the **C scene layer**.
2. `ctypes` is a thin binding to canonical C APIs.
3. GSP and Vispy2 provide ergonomic OO wrappers and plotting conventions, but must not redefine
   canonical style semantics.
4. Technique defaults (MSAA/SSAO/EDL/future) use an extendable registry keyed by stable identifiers.
5. API shape must be FFI-friendly from day one so WASM and Python wrappers avoid redesign.


## Ownership and layering

| Layer | Owner | Responsibility | Must not do |
|---|---|---|---|
| C scene API | Datoviz core | Canonical tokens, inheritance, style resolve, technique defaults, capability fallback | Depend on Python-side policy for rendering semantics |
| Python ctypes wrapper | Datoviz core/autogen | Near 1:1 function exposure and type mapping | Add divergent default semantics |
| GSP OO package | External package | User ergonomics, config loading, aliases, project defaults | Change canonical precedence rules |
| Vispy2 plotting layer | External package | Plot-type presets, high-level convenience defaults | Introduce a parallel style contract |


## Normative direction

### 1) Canonical scope and precedence

The scene system defines and resolves style with this precedence order:

`builtin < user-theme < figure < panel < visual < item`

Resolution happens in scene before DRP2 emission so runtime behavior is backend-consistent.


### 2) Token and template model

- Provide a compact token vocabulary for colors, typography, spacing, colormap defaults, and DPI scale.
- Provide template presets (`light`, `dark`, `publication`, `presentation`) as token bundles.
- Allow scope-local overrides at figure/panel/visual/item levels.

Initial required token families:

- background/foreground colors (including gradient/image background policy),
- text and axis typography/color,
- default item color and categorical palette defaults,
- default sequential/diverging colormap ids,
- default DPI scaling behavior.


### 3) Technique defaults (extendable)

Technique defaults are first-class style data with at least:

- `technique_id` (stable key),
- `enabled_by_default`,
- `quality_tier` (off/low/medium/high or equivalent),
- optional typed parameter map,
- capability fallback behavior when unavailable.

The registry must support current techniques (MSAA, SSAO, EDL) and future ones without changing
existing theme files or requiring a scene-wide schema break.


### 4) WASM/FFI boundary rules

These rules apply to exported ABI functions and ABI-visible types (not every internal helper):

- Use handle/ID-oriented public calls where practical; avoid exposing internal pointer ownership.
- Prefer fixed-layout POD structs with explicit-sized fields.
- Keep ownership explicit; copy inbound strings/buffers at API boundary when needed.
- Provide batched patch/set APIs to reduce cross-boundary call overhead.
- Return status/error codes and optional diagnostics; avoid returning borrowed internal pointers.

Internal C helper composition remains unconstrained as long as boundary contracts are respected.


## Successive implementation stages

## Stage 0 — Spec and vocabulary lock

**Goal:** lock canonical semantics before implementation spread.

Deliverables:

1. Canonical token list v1 and token type table.
2. Precedence and inheritance semantics.
3. Technique-default registry contract.
4. FFI/WASM boundary contract for public style APIs.

Acceptance criteria:

- One normative scene spec referenced by API and integration docs.
- No duplicate competing semantics in ctypes/GSP/Vispy2 docs.


## Stage 1 — Minimal C theme core

**Goal:** ship a small, usable C theme system powering current scene visuals.

Deliverables:

1. Theme object lifecycle and token set/get APIs.
2. Figure/panel/visual scope application and resolve path.
3. Built-in templates (`light`, `dark`) and token override support.
4. Technique defaults for MSAA/SSAO/EDL with enable + quality fields.

Acceptance criteria:

- Scene resolve path emits deterministic DRP2 choices from theme state.
- Focused tests cover precedence, inheritance, and default-fallback behavior.


## Stage 2 — ctypes and debug introspection

**Goal:** expose canonical behavior cleanly to Python with no policy fork.

Deliverables:

1. Thin ctypes bindings for Stage 1 APIs.
2. Introspection helpers (token metadata, dump/inspect resolved style).
3. Batched token patch API suitable for scripted use.

Acceptance criteria:

- Python tests confirm parity with C precedence and resolve behavior.
- No additional wrapper-only style semantics.


## Stage 3 — GSP ergonomics and config packs

**Goal:** provide user-facing convenience while preserving canonical contract.

Deliverables:

1. OO theme API mapping to C tokens/scopes.
2. Load/save theme packs (JSON/TOML) mapped to canonical tokens.
3. Project and session-level default selection.
4. Validation warnings (unknown token, bad type, unsupported technique).

Acceptance criteria:

- A saved pack round-trips through GSP to canonical C without semantic loss.
- Unknown future technique keys degrade predictably per fallback policy.


## Stage 4 — Vispy2 plotting presets

**Goal:** plot-oriented templates and chart defaults built on canonical theme primitives.

Deliverables:

1. Plot-family presets (scientific/presentation/publication variants).
2. Chart-type defaults that compile into canonical tokens.
3. Accessibility variants (high contrast, colorblind-safe base packs).

Acceptance criteria:

- No plotting preset requires backend-specific style semantics outside canonical tokens.
- Visual output parity across direct scene API and Vispy2 wrappers for equivalent token state.


## Stage 5 — WASM-first ABI lane

**Goal:** ensure web lane can reuse the same semantics with minimal friction.

Deliverables:

1. Handle-based exported ABI subset and batched patch call.
2. Explicit ABI-visible struct/version guarantees.
3. Capability query + fallback diagnostics for postprocess techniques.

Acceptance criteria:

- Browser-hosted smoke path can set theme/template/technique defaults and observe resolved behavior.
- No C-core semantic fork to satisfy WASM.


## What moved to canonical specs (target)

After promotion, split normative rules into:

- `spec/scene/semantics/THEMING.md` (tokens, precedence, resolve semantics),
- `spec/scene/api/THEMING_API.md` (C API contract),
- `spec/scene/integration/WASM_THEME_BINDING.md` (ABI boundary and batching policy).

This proposal remains as decision history once those docs are canonical.


## Non-goals (for initial slice)

- Full CSS-like language.
- Rich text inline styling model.
- Backend-specific custom shader styling semantics exposed as public theme tokens.
- Stabilizing legacy v0.3 public API compatibility.


## Open questions

1. Should technique keys be string-first, enum-first, or dual-path in v1?
2. Should theme serialization be JSON-only initially, or JSON+TOML from first release?
3. Which minimal accessibility checks are enforced versus advisory in v1?
