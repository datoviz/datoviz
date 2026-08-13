# Issue #138 Performance Handoff

Status: local baseline and bounded near-term corrections complete; optional RC4 and post-v0.4 scope remains deferred. Updated: 2026-08-13.

This handoff covers [issue #138](https://github.com/datoviz/datoviz/issues/138), motivated by Philippe Strauss's rolling sampled-field and structured-surface measurements. It is a performance investigation and architecture pressure test, not an RC3 correctness blocker. Follow the durable release boundary in [../../spec/scene/proposals/future/SAMPLED_FIELD_VIEWS_AND_INCREMENTAL_UPDATES.md](../../spec/scene/proposals/future/SAMPLED_FIELD_VIEWS_AND_INCREMENTAL_UPDATES.md).

## Diagnosis

The current update path performs substantially more work than the changing data requires: full F64 surface mutation, generic triangle-normal accumulation, F64-to-F32 conversion and copies, full mutable-attribute upload, replacement of unchanged index topology, and full sampled-field upload. Only a new acquisition row is conceptually changing. SIMD or threading the generic normal accumulator cannot remove the redundant conversion, topology churn, or transfer volume.

The reported traces are valuable architectural evidence but are not a release baseline because they include debug builds and fanless-laptop conditions. Establish reproducible release and debug measurements before retaining an optimization or setting a performance claim.

## Checkpoint 0: Baseline

1. Reproduce representative 1D, 2D, and 3D update modes with fixed field dimensions and update cadence.
2. Time acquisition/history mutation, height update, normal generation, F64-to-F32 conversion/copies, FramePlan construction, DRP2 command construction, submission, and GPU execution separately.
3. Record bytes and commands uploaded per resource per frame, including whether unchanged index topology is retransmitted.
4. Measure release and debug builds separately and record hardware, backend, present mode, validation state, visible panels, and active versus parked updates.
5. Keep benchmark assertions out of ordinary CI; add deterministic counters/invariants for upload shape and resource churn.

Commit boundary: diagnostic counters or benchmark harness only, with a short reproducible result table.

## Checkpoint 1: Bounded Near-Term Corrections

1. Consume the retained-buffer and atomic geometry foundation from [ISSUES_139_140_HANDOFF.md](ISSUES_139_140_HANDOFF.md) so unchanged topology is allocated and uploaded once.
2. Stop updating parked or inactive panels, or reduce their update cadence explicitly.
3. Use existing sampled-field regional updates where the current physical layout permits small row writes.
4. Update only mutable mesh attributes; do not call complete geometry replacement when topology is unchanged.
5. Prototype an O(vertices) regular-grid normal kernel based on row/column finite-difference tangents with explicit basis, orientation, boundary, and degenerate-cell behavior.
6. Retain the specialized kernel only if representative end-to-end release measurements improve materially and numerical tests agree with the generic reference within a documented tolerance.

Do not add a public `surface_grid_fast_update()` helper, parallel presentation/runtime path, or new SIMD/threading dependency for this checkpoint.

Commit boundary: each retained optimization has before/after evidence, deterministic correctness coverage, and no per-frame index upload.

### Local retained-index evidence

On 2026-08-13, the `surface` lab mode compared committed baseline `c3f041919` with exact unchanged-index deduplication on an Intel Core i9-14900K Linux host. Both builds used CMake `Release`, disabled GUI, GLFW, validation, and mimalloc, and ran five samples of 20 warm-up plus 200 measured frames on the fixed 128x128 surface. The lab stops after semantic DRP2 emission, so these results contain no submission or GPU time.

| Metric, five-run median or deterministic count | Baseline | Candidate | Change |
| --- | ---: | ---: | ---: |
| Mesh conversion/commit time | 35.2912 ms | 32.4428 ms | -8.1% |
| Semantic emission time | 49.8358 ms | 40.1163 ms | -19.5% |
| Buffer write commands | 1,400 | 1,200 | -14.3% |
| Buffer write bytes | 195,428,800 | 118,009,600 | -39.6% |
| Index write commands | 200 | 0 | -100% |
| Index write bytes | 77,419,200 | 0 | -100% |

This evidence supports retaining exact byte-identical index deduplication in `dvz_mesh_set_geometry()`. Equal-length changed index data remains a required upload and is covered separately by deterministic tests.

### Local regular-grid normal evidence

The same host and release configuration compared the retained-index baseline using generic triangle accumulation with the finite-difference structured-grid kernel. Each side used five samples of 20 warm-up plus 200 measured frames. The baseline median spent 5.0956 ms updating heights and 121.4057 ms generating generic normals, or 126.5013 ms combined; the structured kernel's combined median was 54.4380 ms, a 57.0% reduction. Mesh conversion, command shape, and upload bytes remain unchanged by this kernel.

Deterministic coverage compares orientation with the generic reference at dot product greater than 0.995 on a smooth non-orthogonal grid, exercises centered and one-sided boundary differences, and verifies the established positive-Z fallback for a degenerate basis. This evidence supports retaining the internal kernel only for geometry carrying valid surface-grid provenance; generic geometry continues to use triangle accumulation.

## Checkpoint 2: Optional RC4 Foundations

Sampler addressing and bounded sparse-region tracking may land only as the non-blocking generic sampled-field work already authorized in the release plan. These foundations do not by themselves make the 3D surface GPU-driven and must not delay required RC4 course work.

## Checkpoint 3: Post-v0.4 Streaming Architecture

The durable path is one sparse sampled-row upload feeding binding-local circular/logical views, repeat or clamp addressing, GPU structured-grid displacement, and GPU-derived normals. Static grid topology remains resident, one sampled field may feed both 2D and 3D consumers, and the existing scene -> FramePlan -> DRP2 -> vklite path remains authoritative.

Promote the retained internal geometry boundary to a public scene-owned resource only after sharing, partial attribute updates, optional facets, logical extents, indexed/nonindexed transitions, and ownership/lifetime semantics are pressure-tested together. Preserve the CPU grid path as the fallback and numerical reference.

## Validation Matrix

- flat, sloped, non-orthogonal-basis, boundary, and degenerate regular grids;
- generic versus specialized normal orientation and tolerance;
- no index creation/write command after initialization;
- sparse row upload byte counts and wrap crossing;
- active versus parked panel work;
- release/debug separation and complete benchmark metadata;
- circular logical view consistency across rendering, probing, picking, query, and export;
- runtime reset/recreation forcing the correct full upload;
- native/WebGPU capability adaptation or explicit unsupported diagnostics.

## Completion And GitHub Follow-Up

The local baseline and bounded near-term corrections are complete. #138 remains open pending an approved GitHub follow-up; the retained evidence reports eliminated work without promising that optional RC4 foundations deliver GPU-displaced 3D streaming. GitHub comments, closure, or roadmap commitments remain separate publication actions requiring explicit approval.
