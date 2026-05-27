# Scene Sparse Label Volume Lookup

> **Status:** done
> **Completed on:** 2026-05-27
> **Scope:** sparse categorical color lookup for signed and unsigned 3D label volumes, plus raw
> label-volume query readback edge cases.


## Result

Signed and unsigned integer label volumes now keep the source voxel value as the semantic label id
and use a GPU sparse lookup table derived from the bound categorical `DvzScale` for slice and
composite rendering. The implementation avoids CPU-side volume reindexing for large sparse label ids.

The durable behavior contract lives in
[`../../spec/scene/semantics/SAMPLED_FIELD_INTERPRETATION.md`](../../spec/scene/semantics/SAMPLED_FIELD_INTERPRETATION.md).


## Commits

1. `d2dfa7876` `spec: define sparse label lookup contract`
2. `33ab572da` `scene: add sparse lookup for label volumes`
3. `3160395a0` `spec: update label volume support status`


## Implementation Notes

1. Categorical scales now retain category entries in dynamic storage, with
   `DVZ_SCENE_MAX_SCALE_CATEGORIES` as a guard rather than an inline scene-size multiplier.
2. Volume label rendering binds a read-only storage buffer at the volume descriptor-set layout's
   label-lookup binding. Non-label volume pipelines bind a 16-byte dummy lookup buffer.
3. Dense transfer textures remain available for compact small ids; sparse lookup handles signed ids
   and large unsigned ids before falling back to deterministic hash colors.
4. Label-volume sample queries use the existing 4-byte `r32uint` payload as raw label bits. Raw zero
   remains the miss/background sentinel; `UINT32_MAX` and signed `-1` are preserved.


## Validation

Recorded validation:

```text
just build
direnv exec . just test scene
git diff --check
```

The broad scene run passed `478/478` selected tests.


## Remaining Follow-Up

1. Add an IBL `iblatlas` example using the Allen annotation-index volume and `BrainRegions`
   color/name metadata.
2. Add displayed RGBA to query results only after the multi-output/richer query payload architecture
   is chosen.
3. Keep DVR/MIP ray-hit query semantics, WebGPU/WGSL parity, bricking/out-of-core fields, and full
   MPR as separate follow-up lanes.
