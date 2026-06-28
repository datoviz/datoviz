# Picking Y-Flip Handoff

Status: active bug report from 2026-06-28.

User-visible symptom:

`./build/examples/c/features/picking` appears vertically inverted for hover picking. Hovering near
the bottom highlights markers near the top.

Initial diagnosis:

The example marker positions do not look like the likely source. Native GLFW input enters as
top-left logical window coordinates. The scenario helper converts figure pixels to panel-local
pixels with the same top-left convention, and `dvz_panel_query()` stores those panel-local
coordinates. The likely mismatch is later in shared query coordinate handling, where request
recentering or framebuffer/query target mapping appears to apply a bottom-left/NDC sign convention
inconsistently.

Preferred fix plan:

1. Fix the shared scene query transform/sign path, not `examples/c/features/picking.c`.
2. Keep `dvz_panel_query()` and scenario pointer helpers in top-left panel-pixel coordinates.
3. Add or adjust a focused query coordinate test that would fail for a top/bottom inversion.
4. Rebuild the picking example and run the narrow relevant scene/query tests.
5. Run `git diff --check` before finalizing.

Rationale:

An example-level Y flip would hide the bug only for `features/picking` and leave selection,
image/label/probe query paths, and WASM query setup vulnerable to the same mismatch. The fix should
land where panel-local request coordinates are converted into query NDC/render-state coordinates.

