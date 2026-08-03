# Scene Render Contract Resolver

Status: promoted historical rationale; superseded by the typed render-product and technique-composition architecture. Updated: 2026-08-03.

This proposal established that retained visual requirements must resolve into passive draw and pass contracts before FramePlan graph construction and DRP2 lowering. Lowering may assert resolved facts and fill mechanical protocol details, but it must not independently choose alpha, depth, blend, attachment, bind-group, capability-fallback, or technique-order policy.

The R1-R9 render-product campaign completed that direction with typed panel-local products, immutable composition snapshots, declarative work contracts, generic lowering, coherent surface products, explicit resolve semantics, and deterministic capability diagnostics. The former remaining-work checklist is obsolete and remains available in Git history.

Current authority lives in:

1. [Frame Plan](../../pipeline/FRAME_PLAN.md) for graph, product, pass, and validation behavior.
2. [Visual Contract](../../semantics/VISUAL_CONTRACT.md) for visual draw requirements.
3. [Effects](../../semantics/EFFECTS.md), [Graph Techniques](../../implementation/GRAPH_TECHNIQUES.md), and [Occlusion Effects](../../implementation/OCCLUSION_EFFECTS.md) for product-driven effect behavior.
4. [Transparency](../../semantics/TRANSPARENCY.md) and [Transparency And MSAA](../../implementation/TRANSPARENCY_MSAA.md) for alpha, WBOIT, depth-peeling, and multisample behavior.
5. [Render Products And Technique Composition](RENDER_PRODUCTS_AND_TECHNIQUE_COMPOSITION.md) for the approved historical decision record and rationale.
