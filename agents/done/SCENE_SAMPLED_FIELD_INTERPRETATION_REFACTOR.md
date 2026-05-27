# Scene Sampled Field Interpretation Refactor

> **Status:** done
> **Completed on:** 2026-05-27
> **Scope:** sampled-field interpretation, shared colorizers, label volumes, and sampled visual
> query schemas.


## Result

The sampled-field interpretation refactor is implemented for the active image, labels, volume, and
query paths. The durable behavior record is
[`../../spec/scene/semantics/SAMPLED_FIELD_INTERPRETATION.md`](../../spec/scene/semantics/SAMPLED_FIELD_INTERPRETATION.md).


## Commits

1. `9c0e029a0` `scene: add sampled field profile resolver`
2. `a493cd82e` `scene: add sampled visual colorizers`
3. `e8f325cc1` `scene: route labels through sample profiles`
4. `bbf5f9962` `scene: route volumes through sample profiles`
5. `06b9d3d33` `scene: add semantic query schemas`
6. `0412a8faf` `scene: support label volume slices and queries`
7. `e65bd93ef` `scene: add label volume first-hit compositing`
8. `d290bf402` `scene: validate label volume compositing offscreen`


## Decisions

1. Keep one public `volume` visual family. Do not split `label_volume` as a separate public family.
2. Use internal sample profiles to select scalar, RGBA, unsigned-label, and signed-label behavior.
3. Support signed labels as first-class data.
4. Use `COMPOSITE` as first nonzero categorical label hit for label-volume ray marching.
5. Reject label-volume `MIP` because maximum label id is not meaningful categorical rendering.
6. Keep default label sample queries on the existing 4-byte `r32uint` readback path.
7. Treat dense categorical palettes as an implementation detail. Sparse lookup-table support is the
   correct long-term path for atlas labels and large sparse ids.
8. Do not revive CPU retained-data sampling as a rendered visual query fallback.


## Validation

Recorded validation for the final slice:

```text
just build
direnv exec . just test scene
git diff --check
```

The final broad scene run passed `472/472` selected tests.


## Follow-Up

1. Add sparse GPU categorical lookup tables for large/signed atlas ids, while preserving semantic ids
   in query results.
2. Add an IBL `iblatlas` label-volume example using the Allen annotation-index volume and region
   colors.
3. Keep WebGPU/WGSL parity as a separate later lane.
