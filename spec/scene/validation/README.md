# Scene Validation Specs

This directory contains validation, capability adaptation, diagnostics, and deferred-item tracking.

Use these files when changing failure behavior, fallback policy, diagnostics payloads, or feature
promotion/deferment.


## Files

1. [VALIDATION.md](VALIDATION.md): validation layers, timing, scope, and error classes.
2. [ADAPTATION.md](ADAPTATION.md): explicit capability fallback and simplification policy.
3. [DIAGNOSTICS.md](DIAGNOSTICS.md): shared diagnostic record shape.
4. [DEFERRED_TRACKER.md](DEFERRED_TRACKER.md): deferred items by milestone.
5. [IMAGE_PICKING_RECOVERY.md](IMAGE_PICKING_RECOVERY.md): image pick/probe recovery guardrails
   and remaining diagnostics after the core GPU-backed image probe path landed.
6. [MANUAL_SCENE_SMOKE.md](MANUAL_SCENE_SMOKE.md): manual interactive smoke matrix for the active
   scene -> DRP2 -> app path.
7. [RENDER_CONFORMANCE.md](RENDER_CONFORMANCE.md): planned automated DRP2 snapshot and backend image
   reference testing for scene render conformance.


## Active Proposal Inputs

1. [../proposals/active/CAPABILITY_FALLBACK_DESIGN.md](../proposals/active/CAPABILITY_FALLBACK_DESIGN.md)
