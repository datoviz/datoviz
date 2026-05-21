# Scene Volume Visual

> **Execution Status**
> - **Status:** `DONE / BASELINE LANDED`
> - **Updated on:** `2026-05-21`
> - **Purpose:** record the retained v0.4 volume visual baseline and point remaining execution
>   work to the active follow-up note.


## Landed Baseline

The durable volume contract lives in
[`../../spec/scene/visuals/VOLUME.md`](../../spec/scene/visuals/VOLUME.md).

The active implementation supports:

1. retained `volume` visuals backed by 3D `DvzSampledField` objects;
2. scene -> DRP2 emission through the existing vklite/canvas runtime path;
3. full-volume composite rendering;
4. MIP rendering;
5. explicit slice rendering;
6. opacity and sampling controls;
7. transfer texture generation;
8. normalized clipping boxes and one arbitrary clipping plane;
9. CPU slice probe/readout.


## Active Follow-Up

Remaining near-term volume and napari-style clipping work is tracked in
[`../soon/scene/SCENE_VOLUME_RENDERING_FOLLOWUP.md`](../soon/scene/SCENE_VOLUME_RENDERING_FOLLOWUP.md).
Keep stable mode, clipping, transfer, and probe semantics in `VOLUME.md`.


## Validation Record

For remaining volume changes, continue to prefer:

```text
just build
just test scene
just test drp2
git diff --check
```
