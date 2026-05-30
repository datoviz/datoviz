# Scene Interaction Specs

This directory contains interaction, picking, selection, callbacks, and animation behavior.

Use these files when changing panel controllers, pointer/event routing, panel query readback, retained
selection state, or animation timing.


## Files

1. [GPU_QUERY_SYSTEM.md](GPU_QUERY_SYSTEM.md): GPU-only query architecture replacing public
   pick/probe.
2. [PANEL_QUERY.md](PANEL_QUERY.md): unified under-cursor query model replacing public pick/probe.
3. [PICKING.md](PICKING.md): scene-side picking, identity round-trip, grouped hits, and readback.
4. [SELECTION.md](SELECTION.md): selection state, highlight rendering, linking, and lasso behavior.
5. [CONTROLLERS.md](CONTROLLERS.md): panel controllers, transform ownership, and event routing.
6. [CAMERA_CONTROLLERS.md](CAMERA_CONTROLLERS.md): fly, pivot-orbit, and turntable camera
   controller semantics.
7. [EVENT_CALLBACKS.md](EVENT_CALLBACKS.md): callback delivery and observer semantics.
8. [ANIMATION.md](ANIMATION.md): scene clocks, timelines, easing, and video export coordination.


## Active Proposal Inputs

1. [../proposals/promoted/INTERACTION_API_DESIGN.md](../proposals/promoted/INTERACTION_API_DESIGN.md)
2. [../proposals/promoted/PICKING_DESIGN.md](../proposals/promoted/PICKING_DESIGN.md)
3. [../proposals/promoted/PROBE_READOUT_DESIGN.md](../proposals/promoted/PROBE_READOUT_DESIGN.md)
4. [../proposals/promoted/SELECTION_HIGHLIGHT_DESIGN.md](../proposals/promoted/SELECTION_HIGHLIGHT_DESIGN.md)
5. [../proposals/promoted/TRANSFORM_CONTROLLER_DESIGN.md](../proposals/promoted/TRANSFORM_CONTROLLER_DESIGN.md)
6. [../proposals/active/ASYNC_CALLBACKS.md](../proposals/active/ASYNC_CALLBACKS.md)
