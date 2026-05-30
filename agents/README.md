# Agents Directory

This directory is for current agent dispatch and repo-wide agent rules. It is not an archive.


## Read Order

1. [../AGENTS.md](../AGENTS.md)
2. [now/START.md](now/START.md)
3. [now/STATUS.md](now/STATUS.md)
4. [now/RELEASE.md](now/RELEASE.md)
5. the nearest file in [rules/](rules/) or `spec/`


## Layout

`now/` contains short active release and status notes:

1. [now/START.md](now/START.md): branch dispatch.
2. [now/STATUS.md](now/STATUS.md): current blockers and active lanes.
3. [now/RELEASE.md](now/RELEASE.md): release sequence.
4. [now/DOCUMENTATION.md](now/DOCUMENTATION.md): public documentation release gates.

`rules/` contains detailed agent rules split out of the root entry point:

1. [rules/REPO_HYGIENE.md](rules/REPO_HYGIENE.md)
2. [rules/BUILD_TEST.md](rules/BUILD_TEST.md)
3. [rules/C_CODING.md](rules/C_CODING.md)
4. [rules/GRAPHICS_SAFETY.md](rules/GRAPHICS_SAFETY.md)
5. [rules/SCENE_DRP2.md](rules/SCENE_DRP2.md)


## Maintenance

Keep this directory small. Prefer these destinations:

1. durable design and semantics: `spec/`
2. public user documentation: `docs/`
3. current execution status: `agents/now/`
4. repo-wide automation rules: `agents/rules/`

Delete stale plans once their useful facts are in code, tests, specs, docs, or git history. Do not
add new `done/`, `soon/`, or `later/` queues.
