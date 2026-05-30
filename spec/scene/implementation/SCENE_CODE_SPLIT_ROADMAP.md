# Scene Code Split Roadmap

This file is retired as an active source-split roadmap.

The earlier broad split across frame planning, scene emission, render contracts, runtime emission,
core scene helpers, query helpers, annotation/domain helpers, and visual helper files has already
landed in many small slices. Future agents should not treat the old roadmap as pending work.

The single active document for remaining visual-architecture cleanup is
[`SCENE_VISUAL_BOUNDARY_GUARDRAILS.md`](SCENE_VISUAL_BOUNDARY_GUARDRAILS.md).

Use that file before:

1. adding or removing visual-family switches in generic code;
2. moving visual-specific behavior between root visual helpers and family folders;
3. adding visual-family callbacks or registry capabilities;
4. writing architecture checks for scene visual boundaries.

Historical context for the completed source split is kept in git history.

Do not add new active work to this file. Update `SCENE_VISUAL_BOUNDARY_GUARDRAILS.md` instead.
