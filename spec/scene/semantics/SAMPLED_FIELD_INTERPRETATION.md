# Sampled Field Interpretation

> **Status:** implemented first architecture slice
> **Updated on:** 2026-05-27
> **Scope:** sampled-field format/semantic interpretation for image, labels, volume, colorizer,
> and GPU query behavior.


## Contract

Sampled visual code should not branch independently on raw field formats. The scene resolves:

```text
field format + field semantic + dimension
    -> sample profile
    -> colorizer
    -> visual technique
    -> query schema
    -> backend readback profile
```

The public `volume` family remains one visual family. Scalar, RGBA, unsigned-label, and signed-label
volumes are internal sample-profile and shader-technique variants.


## Implemented Model

The first architecture slice landed in these internal pieces:

1. `src/scene/sample_profile.*` resolves format, semantic, dimension, value kind, sampler kind, and
   colorizer kind.
2. `src/scene/colorizer.*` lowers `DvzScale` to continuous transfer or categorical color lookup.
3. Existing 2D labels and 3D volumes route through sample profiles instead of visual-local format
   switches.
4. Query plans carry a semantic `DvzSceneQuerySchema` before lowering to concrete readback formats.
5. Signed and unsigned 3D label volumes render as `volume` visuals in slice mode.
6. Signed and unsigned 3D label volumes support `COMPOSITE` mode as first nonzero label hit along
   the ray.
7. Label-volume `MIP` is rejected because maximum label id is not categorical semantics.
8. Label-volume sample query uses the existing 4-byte `r32uint` readback, returns raw label bits,
   and decodes the label id on the CPU. Raw zero remains the miss/background sentinel.


## Label Volume Rules

1. Supported label volume storage formats are `R8/R16/R32` signed and unsigned integer single
   channel formats with `DVZ_FIELD_SEMANTIC_LABEL`.
2. Label volumes always use nearest sampling. Linear filtering of integer labels is invalid.
3. Label id `0` is the current background/sentinel in shaders and queries.
4. Positive label ids may use the current dense categorical palette texture.
5. Negative signed labels and sparse/high ids fall back to deterministic hash colors in GPU
   rendering unless the CPU remaps them to dense indices.
6. Query decode returns semantic label ids. CPU-side scale metadata resolves names, colors, and
   domain-specific labels after the GPU result is valid.


## Query Rules

The default query result should be semantic rather than backend-shaped:

```text
label id only -> r32uint
signed label id only -> r32uint with signed decode policy
scalar quantized -> r32uint
scalar quantized + packed UVW -> rg32uint
four exact uint words -> optional future rgba32uint backend profile
```

`rgba32uint` is not the core abstraction. It remains one possible backend lowering for future
multi-word query schemas.


## Sparse Label Lookup

The dense categorical palette was intentionally the first implementation, not the final data model.
Real atlas and segmentation data often use large sparse ids or signed ids. The long-term GPU
colorizer uses the original stored voxel value as the canonical lookup key:

```text
source voxel value -> sparse GPU label table -> rgba / metadata index
```

The scene must not silently regenerate a dense CPU-side volume just to fit a palette texture. CPU
reindexing is acceptable only as an explicit ingestion/import option or when a source dataset already
ships compact indices. Even then, Datoviz must preserve the original semantic label id for query
results, readouts, and external ontology tables.

The first GPU lookup table is a read-only storage buffer bound next to the volume texture. It stores
packed 32-bit label keys plus RGBA colors and metadata indices. A linear scan is acceptable for the
current categorical-scale limit and keeps the first implementation simple; a sorted table and binary
search can replace it once larger category tables are supported.


## Example Pressure

The IBL `iblatlas` package is a good pressure source for label volumes: its Allen atlas object
contains a `uint16` 3D annotation-index label volume, while `BrainRegions` carries Allen IDs,
acronyms, names, parent/order metadata, and RGB/RGBA plot colors. A Datoviz example should upload the
label volume as `DVZ_FIELD_SEMANTIC_LABEL`, build a categorical scale from `BrainRegions`, and
preserve both the stored index and the original atlas ID in query/readout metadata.
