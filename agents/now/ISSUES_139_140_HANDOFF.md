# Issues #139 And #140 Handoff

Status: approved implementation handoff for the active `v0.4-dev` line. Updated: 2026-08-12.

This handoff covers [issue #139](https://github.com/datoviz/datoviz/issues/139) and [issue #140](https://github.com/datoviz/datoviz/issues/140). The issues are independent and should land as separate focused commits after their respective checks pass.

## Decisions

### Issue #139: physical keys and layout-aware text

Keep `DVZ_KEY_*` as physical key identities named for their standard-US positions. Do not remap them through the current layout and do not add AZERTY-specific aliases.

Add a separate public Unicode text-input stream for layout-aware commands and text. The normative contract and target API are in [../../spec/scene/interaction/KEYBOARD_INPUT.md](../../spec/scene/interaction/KEYBOARD_INPUT.md).

The implementation must cover the input router, union event stream, native GLFW character routing, raw GUI consumption, hosted `dvz_view_emit_text()`, the Qt adapter, generated bindings, focused examples or fixtures, and documentation. Do not fold a general shortcut-remapping system or text editor into this issue.

### Issue #140: coherent mesh geometry replacement

A successful `dvz_mesh_set_geometry()` call replaces the complete retained mesh payload. Vertex/index resources, logical counts, draw metadata, dirty/version state, and the next validated frame plan must describe the same new geometry whether the replacement grows, shrinks, or preserves capacity.

The caller must not recreate the panel, visual, or scene when topology dimensions change. Fix the retained resource/count invalidation path rather than weakening draw validation or clamping stale draw counts during emission.

## Execution Order

1. Reproduce and fix #140 first because it is a narrow current-head correctness failure. Start at `src/scene/domain/mesh_geometry.c`, then trace visual resource replacement, logical-count propagation, render-contract draw metadata, and frame-plan cache invalidation.
2. Add direct regression coverage for large surface grid -> smaller grid -> larger grid on one mesh visual. Assert index logical counts and draw counts before runtime emission, validate repeated frame plans, and include the reported shrink case or its minimal equivalent.
3. Implement #139 from the normative keyboard/text contract. Keep physical and text event cardinality separate throughout the router and backend layers.
4. Refresh generated Python bindings after the public input surface changes and update the chapter-5 keyboard proof to use the correct physical or text path explicitly.

## Validation Gates

For #140, run the focused geometry/mesh tests, affected frame-plan and runtime tests, `just build`, relevant validation-enabled scene tests, and `git diff --check`.

For #139, run focused input/router tests, available GLFW and Qt adapter checks, `just ctypes`, `just ctypes-check`, `just build`, the chapter-5 input smoke when present, and `git diff --check`.

Before each commit, inspect `git status --short` and `git diff --cached --stat`; exclude the `data` submodule, generated/runtime binaries, paper artifacts, and unrelated user changes.

## Completion And GitHub Follow-Up

Each issue is complete only when its regression test fails on the old behavior, passes with the fix, and the relevant contract and generated surfaces agree with the implementation. After the validated commit is pushed, add a concise GitHub comment naming the behavior, tests, and commit, then close only the resolved issue. GitHub publication remains a separate explicitly approved action.
