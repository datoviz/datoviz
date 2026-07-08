# Scene Visual Boundary Guardrails

Status: retired implementation plan.

The visual-boundary refactor is closed for v0.4. The durable architecture contract now lives in
[`../visuals/BOUNDARY_CONTRACT.md`](../visuals/BOUNDARY_CONTRACT.md).

The completed state is enforced by:

```sh
python3 tools/check_scene_visual_boundaries.py
just spec-check
```

`tools/scene_visual_boundary_allowlist.txt` must remain empty except comments. Do not add new
root-level visual-family switches, family-private includes from generic code, descriptor-kind
matrices outside their owners, or new generic retained-state payload access. Add a registry
callback, neutral descriptor field, explicit shared visual subsystem, or family-owned helper
instead.
