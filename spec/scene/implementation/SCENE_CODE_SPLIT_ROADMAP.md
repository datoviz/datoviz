# Scene Code Split Roadmap

This file is retired as an active source-split roadmap.

The earlier broad split across frame planning, scene emission, render contracts, runtime emission,
core scene helpers, query helpers, annotation/domain helpers, and visual helper files has already
landed in many small slices. Future agents should not treat the old roadmap as pending work.

The main active document for remaining visual-architecture cleanup is
[`SCENE_VISUAL_BOUNDARY_GUARDRAILS.md`](SCENE_VISUAL_BOUNDARY_GUARDRAILS.md).

Use that file before:

1. adding or removing visual-family switches in generic code;
2. moving visual-specific behavior between root visual helpers and family folders;
3. adding visual-family callbacks or registry capabilities;
4. writing architecture checks for scene visual boundaries.

Historical context for the completed source split is kept in git history.

Small remaining cleanup work belongs in owning specs or in the boundary guardrails. Preserve these
specific post-v0.4 refactor candidates when they become relevant:

1. split the internal frame artifact emission helpers only along real ownership boundaries such as
   panel ordering, MVP setup, and pass emission;
2. centralize texture dirty-state helpers instead of repeating family-specific checks;
3. share retained slot allocators and destruction-reset helpers across visual resources;
4. isolate JSON append helpers from core scene mutation;
5. keep request cleanup separate from adding new query features.

Do not add new active work to this file. Update `SCENE_VISUAL_BOUNDARY_GUARDRAILS.md` instead.
