# Status

- Task: refresh agent, README, task, architecture, and spec markdown against the current code state
  and recent commits.
- Started: 2026-05-13
- State: completed

## Plan

1. Inspect recent commits and identify stale current-state claims.
2. Update root/agent-facing guidance, public README status, architecture overviews, spec indexes, and
   recent task records.
3. Keep historical `agents/done` records intact except for index pointers.
4. Validate the doc-only edit set with `git diff --check`.

## Current State Captured

The refreshed docs should now describe these current facts:

1. `app` is an active default-build module alongside `drp2` and `scene`.
2. Scene visuals include point, primitive, mesh, path-as-line/strip, and image.
3. Scene now has retained sampled fields, image colormap scale binding, colorbar bookkeeping,
   panzoom/arcball controllers, text/annotation bookkeeping, selection/link/readout bookkeeping, and
   first point-pick/image-probe request execution.
4. Recent commits hardened app trace/status output, request runtime handling, figure-size sync before
   app frame emission, and point-picking panel-coordinate mapping.
5. WebGPU should be treated as a narrow feasibility lane soon, not as a full parallel runtime rewrite.

## Validation

```bash
git diff --check
```
