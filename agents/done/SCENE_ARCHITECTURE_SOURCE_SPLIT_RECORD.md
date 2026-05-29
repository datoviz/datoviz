# Scene Architecture Source Split Record

This record archives the broad scene architecture/source split that preceded the visual-boundary
guardrails phase. It is historical context, not an active execution plan.


## Completed Outcome

The broad split moved the scene implementation away from large mixed-owner files and toward the
current layered layout:

1. `frame_plan/` owns backend-neutral pass, resource, dependency, readback, fixture, and debug
   structures;
2. `scene_emit/` owns retained scene to FramePlan orchestration;
3. `render_contract/` owns validation between planned visual/resource/draw state and runtime
   requirements;
4. `runtime/` emits DRP2 from resolved FramePlan state;
5. `visuals/registry/` owns the private `DvzVisualFamilyOps` registry;
6. `visuals/<family>/` folders own substantial family-specific lowering, bind, pipeline, shader,
   draw, bounds, and query pieces;
7. `query/` owns generic request orchestration, readback routing, policy, and shared item decode;
8. `domain/`, `annotation/`, `text/`, and `core/` have first-pass owner files for helpers that
   previously lived in broad mixed files.


## Important Completed Slices

Completed source-ownership moves include:

1. deletion of the old normal-path untyped visual descriptor compatibility flow;
2. mandatory typed visual metadata for normal scene render output;
3. visual-family registry coverage for active visual types;
4. family-owned retained lowering for active visual families;
5. family-owned bind descriptors, normal pipeline descriptors, shader descriptor bodies, and draw
   descriptor hooks;
6. family hooks for image, labels, and volume metadata fill;
7. generic dense-attribute, retained-index-buffer, and material-trigger upload support helpers;
8. derived upload orchestration split from panel-visible upload orchestration;
9. generated image quad and stroke geometry payload builders moved to owner subsystems;
10. image/glyph texture upload payload decisions and volume source/transfer/label payload decisions
    moved out of broad scene emission;
11. query scratch helpers, standard item-id decode, item-target eligibility, native/sample target
    policy, and render-metadata completeness moved to generic query/frame-plan ownership;
12. vector/segment/path item-result family decode ownership, image/labels/volume unsupported-query
    policy hooks, and shared point-like/stroke/indexed query planning;
13. field dirty propagation, scalar sampled-field interpretation, sampled-field data setters, and
    sampled-field binding release split into owner files;
14. axis text realization, tick/domain planning, generated primitive-visual synchronization, and
    layout reserve split into annotation owner files;
15. narrow helper declarations moved out of `_scene.h` into owner-private headers.


## Remaining Active Work

Do not resume work from the old long roadmaps. The active remaining architecture work is now:

[`../../spec/scene/implementation/SCENE_VISUAL_BOUNDARY_GUARDRAILS.md`](../../spec/scene/implementation/SCENE_VISUAL_BOUNDARY_GUARDRAILS.md)

That spec owns the next phase: making generic scene code registry-driven, confining
visual-specific behavior to family folders or explicit shared visual subsystems, and adding
mechanical guardrails so the old coupling does not return.
