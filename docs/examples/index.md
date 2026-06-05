# Examples

Examples are executable release proof for v0.4. Every public visual family and public feature should
have a minimal C example with a stable identifier.

The current source examples live under `examples/c/`, `examples/webgpu/`, and related runtime or
tool directories.

Current v0.4 example lanes:

| Lane | Source directory | Purpose |
| --- | --- | --- |
| Visuals | `examples/c/visuals/` | One minimal C example per public visual family. |
| Features | `examples/c/features/` | One focused C example per public scene/app feature. |
| Workflows | `examples/c/workflows/` | Small composed examples that connect several focused features. |
| Composites | `examples/c/composites/` | Semantic composites such as polygon sets and graph lowering. |
| Showcases | `examples/c/showcases/` | Curated gallery-facing examples using synthetic or bundled/prepared data. |
| Scientific | `examples/c/scientific/` | Real-data examples with source, license, and preprocessing notes. |
| WebGPU | `examples/webgpu/` | Browser demos and fixture dashboards for the experimental WebGPU subset. |

The public example pages are still being rebuilt from the C manifest. The old `docs/gallery/**`
tree is excluded from the v0.4 MkDocs navigation and should be treated as v0.3-era source material
only.
