# Architecture Map

Use this page to find the current source of truth before changing a subsystem. It is intentionally
an index, not another architecture spec.


## Public Documentation

| Need | Location |
| --- | --- |
| user orientation | `docs/start/` |
| runnable examples and gallery | `docs/examples/` and `examples/c/MANIFEST.yaml` |
| task workflows | `docs/how-to/` |
| exact feature/API status | `docs/reference/` |
| contributor procedures | `docs/contributors/` |

`mkdocs.yml` is the source of truth for public navigation.


## Durable Specs

| Surface | Location |
| --- | --- |
| documentation architecture | `spec/docs/` |
| scene semantics and visuals | `spec/scene/` |
| DRP2 schemas and command authority | `spec/drp2/` |
| raw `ctypes` and Python facade boundaries | `spec/bindings/` and `spec/api/` |
| release policy | `spec/release/` |
| test-suite direction | `spec/testing/` |

Specs own durable behavior. Do not put active work queues or release handoff notes there.


## Implementation

| Surface | Location |
| --- | --- |
| public C headers | `include/datoviz/` |
| shared internal helpers | `src/common/` |
| active modules | `src/common`, `src/ds`, `src/fileio`, `src/geom`, `src/math`, `src/thread`, `src/input`, `src/window`, `src/canvas`, `src/stream`, `src/video`, `src/vk`, `src/vklite`, `src/drp2`, `src/scene`, `src/app` |
| generated Python raw binding | `datoviz/raw.py`, `datoviz/_ctypes.py` |
| array-aware Python facade | `datoviz/_array_facade.py` |
| example manifest tooling | `tools/build_examples_manifest.py` |

The active runtime path is:

```text
scene frame plans -> drp2 command streams -> vklite runtime ->
canvas/stream frame execution -> optional app presentation
```


## Active Handoff Notes

Use `agents/` for current execution status:

| Need | Location |
| --- | --- |
| mandatory automation entry point | `AGENTS.md` |
| current branch dispatch | `agents/now/START.md` |
| release blockers | `agents/now/STATUS.md` |
| release sequencing | `agents/now/RELEASE.md` |
| repository rules | `agents/rules/` |

Do not copy active blocker lists into public docs. Link to them from contributor docs only when the
reader needs the current development queue.
