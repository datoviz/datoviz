> **Execution Status**
> - **Status:** `ACTIVE CAPABILITY / FALLBACK DESIGN NOTE`
> - **Updated on:** `2026-05-09`
> - **Purpose:** define the intended v0.4 scene-facing contract for capability checks, explicit
>   degradation policy, and runtime diagnostics across rendering features.

# Capability and Fallback Design

This note turns capability discussion that is currently spread across DRP2, transparency, picking,
and future ray-tracing material into one active scene-level policy.


## Objective

Keep feature support, degradation, and failure behavior explicit for:

1. transparency modes,
2. picking precision,
3. volume and mesh rendering modes,
4. text rendering paths,
5. host/backend integration and offscreen presentation.


## Existing Grounding In The Repo

Useful existing context:

1. DRP2 capability model:
   [spec/drp2/CAPABILITIES.md](/home/cyrille/GIT/Viz/datoviz/spec/drp2/CAPABILITIES.md)
2. active transparency note:
   [agents/now/TRANSPARENCY_WBOIT_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/agents/now/TRANSPARENCY_WBOIT_DESIGN.md)
3. active picking note:
   [agents/now/PICKING_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/agents/now/PICKING_DESIGN.md)
4. active ray-tracing note:
   [agents/now/RAY_TRACING_FORWARD_COMPAT.md](/home/cyrille/GIT/Viz/datoviz/agents/now/RAY_TRACING_FORWARD_COMPAT.md)
5. completed presentation/offscreen notes:
   [agents/done/PRESENTATION.md](/home/cyrille/GIT/Viz/datoviz/agents/done/PRESENTATION.md)
   and
   [agents/done/OFFSCREEN.md](/home/cyrille/GIT/Viz/datoviz/agents/done/OFFSCREEN.md)

This note records what the active scene and visual API should assume.


## Core Recommendation

Capabilities should be treated as explicit runtime facts, and fallbacks should be opt-in policy,
not silent behavior.

Recommended split:

1. runtime reports structured low-level capability facts,
2. scene or planning derives whether a requested semantic feature can be realized,
3. unsupported requests produce explicit diagnostics by default,
4. any fallback must be an intentional declared policy,
5. capability/support and interaction policy should remain separate API concerns.


## Capability Layers

The active model needs at least three layers:

1. low-level runtime/device facts,
2. derived feature availability,
3. scene-requested feature policy.

Examples:

1. low-level facts:
   - color/depth formats
   - storage image support
   - sample counts
   - readback path availability
2. derived features:
   - WBOIT available
   - image pixel picking available
   - face-level mesh picking available
   - offscreen texture export available
3. scene policy:
   - require feature
   - allow explicit fallback
   - disable feature


## Default Policy

The default should prefer correctness and explicitness over convenience.

Recommended default rules:

1. if a requested feature cannot be realized, scene validation or planning should fail clearly,
2. do not silently downgrade a visual or panel to a weaker mode,
3. skip or disable only when the API request itself marked the weaker behavior acceptable.


## Fallback Classes

Not all fallback situations are equal. Reserve these classes now:

1. `hard_error`
2. `explicit_disable`
3. `explicit_degrade`
4. `skip_with_reason` for tests and examples

Recommended meanings:

1. `hard_error`
   - requested feature is unavailable and no fallback policy was declared
2. `explicit_disable`
   - caller chooses to turn the feature off entirely
3. `explicit_degrade`
   - caller accepts a named weaker semantic mode
4. `skip_with_reason`
   - validation/test tooling records that the environment lacks required capability


## Transparency Example

Transparency is the clearest current case.

Recommended rule:

1. a visual requesting `transparent_wboit` should fail with explicit capability diagnostics when the
   runtime cannot realize that mode,
2. a weaker blend path must not be substituted implicitly,
3. if a future API allows fallback, it should name the allowed degraded mode explicitly.


## Picking Example

Picking also needs explicit capability semantics.

Recommended rule:

1. if a visual declares face-level, item-level, or pixel-level picking, the scene must verify that
   the required identity path and readback path exist,
2. if only object-level picking can be supported, that is a semantic downgrade and must be
   explicitly allowed rather than silently applied,
3. diagnostics should name both the requested precision and the missing requirement.


## Text Example

Text will likely have several realizations over time.

Recommended direction:

1. the scene-level text object asks for semantic behavior such as shaped text, world-space
   placement, or emoji/color glyph support,
2. runtime/backend chooses an implementation path only if it preserves that requested behavior,
3. unsupported behavior becomes an explicit capability result, not a best-effort silent change.


## Volume And Ray Compatibility Example

Volume and future ray-tracing work increase the pressure for explicit capability policy.

Recommended direction:

1. scene keeps semantic requests such as slice mode, sampled-value probe support, and mixed picking
   behavior,
2. runtime decides whether raster or future ray-backed realization is available,
3. capability adaptation remains explicit at the planning boundary.


## External UI And Hosting Example

Host-toolkit integration also has capability edges.

Examples:

1. offscreen render-to-texture available
2. direct host texture sharing unavailable
3. window embedding path available for one backend but not another

Recommended rule:

1. these differences stay in runtime capability reporting,
2. the scene API should expose logical handles and explicit diagnostics rather than leaking
   backend-native assumptions.


## Diagnostics

Diagnostics are part of the contract, not an afterthought.

Recommended minimum diagnostic content:

1. requested feature,
2. failing object or visual identity when relevant,
3. reason or missing capability,
4. whether a fallback was attempted or rejected,
5. recommended next action when practical.


## Public API Direction

The exact names can still move, but the conceptual API should support:

1. runtime capability snapshot query,
2. feature requirement declaration at scene or visual level,
3. optional fallback policy declaration,
4. validation/planning result objects carrying structured diagnostics,
5. interaction-policy objects that consume only capabilities already declared as supported rather
   than doubling as capability declarations themselves.


## Test And Example Policy

Tests and examples should follow the same explicitness rule.

Recommended behavior:

1. required-capability tests fail or skip with an explicit reason,
2. example code does not silently mask a missing feature behind a different visual result,
3. smoke loops record which capability path they exercised.


## Immediate Scope Recommendation

The narrowest useful active implementation target is:

1. one structured runtime capability snapshot,
2. explicit validation for WBOIT, picking precision, and offscreen/host-texture paths,
3. one fallback-policy hook shape in the scene-facing API,
4. structured diagnostics surfaced to tests and examples.


## Explicit Non-Goals For The First Slice

1. designing every future capability bit now,
2. auto-tuning or heuristic mode switching,
3. backend-specific feature flags exposed directly in public scene APIs,
4. silent graceful degradation as the default behavior.
