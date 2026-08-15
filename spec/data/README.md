# Data Specs

This directory owns durable binary-asset, manifest, provenance, cache, publication, and showcase-dataset policy.


## Index

1. [ASSET_ARCHITECTURE.md](ASSET_ARCHITECTURE.md): target no-LFS architecture, storage ownership, `datoviz/assets` release bundles, offline guarantees, branch-cutover prerequisite, migration, and legacy retirement.
2. [V0_4_DATA_REPOSITORY.md](V0_4_DATA_REPOSITORY.md): implemented transitional data-submodule layout and provenance policy; its LFS/submodule target is superseded by the asset architecture.
3. [WEBGPU_DATA_BUNDLES.md](WEBGPU_DATA_BUNDLES.md): implemented fetched-data contract for live WebGPU/WASM examples and the starting point for native/WebGPU bundle convergence.


## Boundary

Use this directory for binary ownership, publication, manifest, provenance, cache, and reproducible preparation rules. During migration, follow the current submodule safety rules for implemented paths while using the asset architecture as the target authority.
