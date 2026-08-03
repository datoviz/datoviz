# GSP Compatibility Preparation

Status: historical record of the completed Datoviz-side compatibility campaign. Updated: 2026-08-03.

The `gsp-compat-prep` campaign established a public live view capability snapshot, scene-local object identity getters, parent scene and figure identities in query results, raw ctypes coverage for capability and query structs, an adapter-facing visual-family mapping, and a public C offscreen render/capture/query smoke. The six implementation missions and their consultation packet are complete; their detailed prompts and checklists remain available in Git history.

The accepted ownership boundary is:

1. Datoviz owns its C renderer/runtime, retained scene/app path, generated low-level Python binding, scene-local identities, capability reporting, queries, and raster capture.
2. GSP owns backend-independent visual semantics and protocol identities; an adapter maintains protocol-id to Datoviz-handle/id maps.
3. VisPy2 owns the Pythonic plotting, scene, interaction, and notebook experience.
4. Datoviz does not add general `user_id` or GSP protocol-id fields to retained objects or query results for the v0.4 contract.

Current authority lives in:

1. [`../../../api/PYTHON_GSP_SCOPE.md`](../../../api/PYTHON_GSP_SCOPE.md) for product ownership and public positioning.
2. [`../../../api/GSP_BACKEND_STRATEGY.md`](../../../api/GSP_BACKEND_STRATEGY.md) and [`../../../api/GSP_BACKEND_READINESS.md`](../../../api/GSP_BACKEND_READINESS.md) for adapter strategy and current readiness.
3. [`../../visuals/GSP_MAPPING.md`](../../visuals/GSP_MAPPING.md) for visual/resource/query mapping.
4. [`../../../bindings/CTYPES_POLICY.md`](../../../bindings/CTYPES_POLICY.md) for generated binding policy.
