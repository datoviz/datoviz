some notes about the examples


## Closure ledger - 2026-06-09

Status values: `resolved` means source/metadata was changed or audited with evidence; `partial`
means a contained improvement landed but more visual/runtime work remains; `deferred` means the item
is still visible with the blocker or preferred next action.

| Item | Status | Evidence / next action |
| --- | --- | --- |
| `composites/graph` | resolved | Rewritten as a cleaner two-community graph with bridge edges in `examples/c/composites/graph.c`; compiled and docs regenerated. |
| `composites/polygon` | resolved | Rewritten as a regular holed polygon plus non-overlapping star/tiles in `examples/c/composites/polygon.c`; triangulation remains covered by `features/triangulation_polygon.c`. |
| `features/animation_tracks` | resolved | Cube rotation remains track-backed and the camera eye now follows looping `dvz_track_keyframes()` while the target stays fixed at the cube center in `examples/c/features/animation_tracks.c`; compiled. |
| `features/annotation_readout` | resolved | Larger closer label in `examples/c/features/annotation_readout.c`; compiled in focused example batch. |
| `features/app_glfw` | resolved | Standard 1600x1200 size in `examples/c/features/app_glfw.c`. |
| `features/axis_labels` | resolved | Removed confusing tilted segment clutter; retained only simple plot-frame guides in `examples/c/features/axis_labels.c`. |
| `features/colormap_scale` | resolved | Scale/colormap setup is explicit in `examples/c/features/colormap_scale.c`. |
| `features/controller_fly` | deferred | Runtime audit found basis/drag ambiguity in `src/controller/fly.c`; do not flip blindly. Needs interactive validation and sign regression for the exact z-up plane pose. |
| `features/technique_depth_cue` | resolved | Two-panel comparison now uses a regular 3D sphere lattice in `examples/c/features/technique_depth_cue.c`; compiled and docs regenerated. |
| `features/technique_depth_test` | deferred | Shader antialiasing diagnosis still required. |
| `features/technique_edl` | resolved | Two-panel comparison now uses the shared regular 3D sphere-lattice pattern in `examples/c/features/technique_edl.c`; compiled and docs regenerated. |
| `features/external_surface` | resolved | Public advanced example is `examples/c/advanced/external_surface_glfw.c`; gallery generator now includes advanced pages. |
| `features/gui_cimgui` | deferred | App/window path remains justified until scenario GUI hosting exists. |
| `features/gui_controls` | deferred | Needs current builtin-controls decision; app/window path remains justified for GUI host integration. |
| `features/gui_viewport` | deferred | Runtime audit points to Retina logical/framebuffer resize in `src/gui/gui.cpp`; needs runtime fix, not example-only patch. |
| `features/guide_lines` | resolved | Cursor event callback updates guides in real time in `examples/c/features/guide_lines.c`; compiled. |
| `features/guide_spans` | resolved | Cursor event callback centers spans in real time in `examples/c/features/guide_spans.c`; compiled. |
| `features/input_events` | partial | Current example prints emitted input events through app/view router; a live terminal window variant remains optional. |
| `features/legend_categorical` | resolved | Removed highlighted entry that made one swatch larger in `examples/c/features/legend_categorical.c`; compiled. |
| `features/lighting` | resolved | Three-panel material/light comparison in `examples/c/features/lighting.c`; compiled. |
| `features/marker_symbols` | resolved | Added row labels and a distinct colored bitmap-pin row in `examples/c/features/marker_symbols.c`; compiled and docs regenerated. |
| `features/technique_msaa` | partial | Prefix is correct; stronger 3D mesh/cube comparison still needed. |
| `features/offscreen_capture` | deferred | Keep exact-pixel offscreen semantics; add high-DPI/supersampled variant separately if desired. |
| `features/overlay_card` | resolved | Adds a simple signal and highlighted sample behind the fixed readout card in `examples/c/features/overlay_card.c`; compiled and docs regenerated. |
| `features/panel_background` | resolved | Uses explicit inset gradient panel background in `examples/c/features/panel_background.c`; compiled. |
| `features/panel_domain_fit` | resolved | Side-by-side free-domain vs equal-aspect fit in `examples/c/features/panel_domain_fit.c`; compiled. |
| `features/panzoom` | resolved | Manifest/source/docs now use `feature_panzoom` with source/executable `panzoom`; generated stale `controller_panzoom` page removed. |
| `features/pick*` | resolved | `features/picking.c` is the unified example; query y-orientation regression exists in `src/scene/tests/query.c`. |
| `features/record_replay` | deferred | Runtime audit points to setup-only replay frame consumption in `_app_draw_replay()`; avoid render-twice workaround. |
| `features/basic_scene` | resolved | No `scene_` source/executable prefix; manifest/docs use `feature_basic_scene` as internal gallery ID. |
| `features/compute_buffer_animation` | resolved | Compute shader now drives small independent circular point motion in `examples/c/features/compute_buffer_animation.c`; compiled. |
| `features/json_export` | resolved | Writes `scene_export.json` and prints `./scene_export.json`; title/feature metadata updated to JSON Export. |
| `features/technique_ssao` | partial | Prefix is correct; blur/grain visual tuning remains. |
| `features/technique_transparency` | partial | Prefix is correct; stronger 3D overlapping-cubes comparison remains. |
| `features/video_export` | resolved | Runner record modes print video output paths; `examples/c/features/video_export.c` runs frame animation. |
| `features/visibility` | resolved | Batching/visual-level visibility comment exists in `examples/c/features/visibility.c`. |
| `features/visual_transform` | resolved | Two-panel without/with transform comparison in `examples/c/features/visual_transform.c`; compiled. |
| `features/volume_occlusion` | deferred | Needs two-panel volume occluder comparison rewrite. |
| `showcases/brain_volume` | partial | Name is already `brain_volume`; initial tilted view still needs visual smoke in Vulkan-capable environment. |
| `showcases/choropleth` | deferred | Retina title size and fixed-aspect right-drag zoom remain. |
| `showcases/scientific_plotting` | deferred | X-axis clipping remains to inspect/fix. |
| `showcases/textured_planet` | deferred | Dark-side lighting and zoom limits/sensitivity remain. |
| `showcases/wind_field` | deferred | Ellipsoid pointer needs removal/replacement/explanation. |
| `visuals/glyph` | deferred | Clarify low-level atlas glyph vs marker/text, or switch to actual font glyph semantics. |
| `visuals/segment` | resolved | Shows butt, square, and round caps in `examples/c/visuals/segment.c`; compiled. |
| General comments/descriptions | partial | Changed summaries regenerate docs; broader top-of-file copy audit remains. |
| Scenario-helper use | partial | New/changed examples use scenario runner where appropriate; app/window/advanced exceptions remain explicit. |
| Builtin-shapes parity | deferred | v0.4 has 2D/3D shape examples; Platonic solids/histogram/gizmo-shape parity is deferred or needs geom API additions. |
| Further: `showcases/surface_grid` | deferred | Needs new showcase port. |
| Further: `features/instancing` | resolved | Current feature exists as `examples/c/features/selection_mesh_instances.c`; plain non-selection instancing example still optional. |
| Further: `features/isolines` | resolved | Current implementation exists in `examples/c/features/isolines.c` and is in generated docs. |
| Further: raw triangles | resolved | `advanced_raw_triangle_vklite` and `advanced_raw_triangle_drp2` are in CMake/manifest/generated advanced docs. |
| Further: `bounds_overlay.c` | deferred | Needs one/two feature-example ports. |
| Further: `arcball_gizmo.c` | resolved | Superseded by synchronized inset `examples/c/features/orientation_gizmo.c`; generated docs now include it. |


composites/graph
not very pretty, can't we have a more classic/standard graph? something more compelling

composites/polygon
this is ugly can we have something cleaner and more regular without overlapping polygons
like a regular polygon, possibly with a hole, and something like a star or something
also could we show the underlying triangulation? how would that work?

features/animation_tracks
this one is a bit ugly, can't we find something more compelling? let's say a rotation animation on an object, and at the same time, a camera using dvz_track_keyframes to hover over the visual while keeping the same target point to the center of the visual? something both simple (to mostly show the animation API and not much more) and compelling. wdyt?

features/annotation_readout
on macOS the text is way too small and far from the point (too much on the bottom right compared to the point)

features/app_glfw
shouldn't we use the standard window size?

features/axis_labels
wtf are all these segments? some axis aligned other tilted. seems very wrong and I don't understand what that is supposed to represent

features/colormap_scale
i'm a bit confused as to what this is supposed to illustrate mostly because the key feature seems to be hidden behind a helper no?

features/controller_fly
the left-right drag is very weird and possibly broken

features/technique_depth_cue
to show this technique we should use a 3D example no? for ex 3D spheres, perhaps organized in a 3D regular lattice, with this technique disabled vs enabled in two different panels? wdyt?

features/technique_depth_test
the disc edges don't seem to be perfectly antialiased (without msaa, just talking about the shaders here), can you double check?

features/technique_edl
i wonder if the techniques examples should not be prefixed with technique_?
also, this one i think should use a 3D example, perhaps the same as features/technique_depth_cue, but showing edl instead of depth_cue? wdyt?

features/external_surface
that one is not functional atm. wdyt we should do to show a good example of using an external vulkan surface?

features/gui_cimgui
why not use the scenario path for app/window creation?

features/gui_controls
there are no more builtin controls to show here? also, why not use the scenario path for app/window creation?

features/gui_viewport
three little problems here, not with the example but with the underlying implementation
first, the viewport seems antialiased on macOS retina so there may be a framebuffer/view pixel resolution mismatch.
second, initially, in the first frames after starting the example, the viewport shows a very zoomed in part of the viewport before immediately switching back to the correct viewport.
third, there is some lag when resizing the viewport with dear imgui, the viewport correctly resizes itself but after a few frames so it all seems a bit unstable/wobbly
perhaps we could have another example, more complicated, showing docking with multiple viewports, and there could be a button to add a new viewport with random positions each time and viewport dialog title "viewport 1", "viewport 2" etc? wdyt?

features/guide_lines
instead of switching the positions after 1 second, could the horizontal and vertical lines match the cursor position in real time?

features/guide_spans
instead of switching the spans after 1 second, could the horizontal and vertical spans be centered around cursor position in real time?

features/input_events
better to show a live window where we display keyboard and mouse events in the terminal in real time no?

features/legend_categorical
in the legend the gray square is bigger than the other squares

features/lighting
not sure if this is a good illustration of the lighting system, shouldn't we show various types of lights/materials/settings etc? wdyt?

features/marker_symbols
shouldn't the different types of symbols show clearly which is using which technique? for ex for bitmap using a clearly bitmap symbol with, perhaps, colors, that could not be easily made with other techniques (look at v0.3 marker visual example where we had used an actual red pin bitmap)

features/technique_msaa
should have technique_ prefix
also, better to show a 3D scene i think with some meshes like cubes because we would better see the aliasing without msaa

features/offscreen_capture
so this one looks a bit aliased on macOS retina compared to desktop version because the framebuffer size is the requested size, whereas on macOS the underlying framebuffer is twice larger in each dimension. what would be a proper to fix this? something like scale, dpi, resolution or whatever in the offscreen path? or would that be confusing because the requested pixels in offscreen should match the png size? or perhaps we could have an option where we specify the offscreen size not in pixels, but in dots or points or something with dpi and scale? wdyt? what would be a standard, natural way of tackling this, thinking about the right?

features/overlay_card
for that one i think i would prefer a slightly more realistic example that shows an actual use case of the overlay rather than an isolated overlay with nothing else, while keeping the spirit "1 feature = 1 example", what would you propose?

features/panel_background
i don't think this is working, i do not see a background

features/panel_domain_fit
i'm not sure what this one is supposed to show? is it supposed to force some equal aspect or something? I don't think it's working

features/panzoom
why not just "panzoom"

features/pick*
resolved by `features/picking.c`: unified hover scaling and click selection/tinting, with a
query y-orientation regression in `src/scene/tests/query.c`

features/record_replay
that one is broken, record_replay_replay.png is all purple

features/basic_scene
do we need the scene_ prefix?

features/compute_buffer_animation
do we need the scene_ prefix?
this example should instead show some small circular independent movement of each dot with different phases and angular speeds

features/json_export
should find a name without scene_ prefix i think
this example should save the json to a file and print the path of the saved json

features/technique_ssao
should have technique_ as prefix
should have more blur because looks a lot grainy

features/technique_transparency
there are no other transparency techniques to show? are you sure a 3D example is not clearer to show the differences between the techniques? like 2 cubes of different sizes that partially overlap or something? wdyt?

features/video_export
this one should run for a couple of seconds and save a video file and print on the terminal the path to the video file no?

features/visibility
we must add a comment that we use different visuals just for demo purposes only but in reality it is better to group visuals of the same family together to benefit from batching, however visibility applies to the level of an entire visual

features/visual_transform
this one is not very clear? perhaps we could have two panels with the same visual, without or with transform?

features/volume_occlusion
this one is very weird and needs to be redone. perhaps two panels with a volume occluded by a plane or a mesh, with volume occlusion off and on



showcases/brain_volume
this one should be called just "brain_volume"
also the initial view should be tilted in such a way that we see the slice and some of the brain volume behind

showcases/choropleth
title and subtitle are way too small on macOS retina
zoom with right drag should force fixed aspect ratio

showcases/scientific_plotting
xaxis is clipped by panel bottom end

showcases/textured_planet
the dark side of the planet is way too dark we don't see anything
also i think we should put some limits to zoom in/out with this view, and perhaps some kind of log scale because the more we zoom the slower we want to go or something? right now on macOS the double finger zoom is way to sensitive

showcases/wind_field
the ellipsoid pointer is weird, what is it?



visuals/glyph
i don't understand this one, compared to marker? shouldn't these be font glyphs? what's the difference with marker?

visuals/segment
shouldn't we show different types of segment ends?



GENERAL COMMENTS
- shouldn't the comments/description at the top of each example be a bit more detailed, so that we can reuse the text on each example's webpage?
- we should ensure all examples use the scenario helper, although there may be a few justified exceptions when what we want to demonstrate lies precisely in what the scenario wraps for, in that case it is reasonable not to use the scenario api
- we miss some builtin shapes in 2D or 3D like we had in v0.3, ex pythagorean solids etc, can you check what we had in v0.3, what we have in v0.4 so far, what's missing for feature parity, if you see any other standard shape we could include, and report back? no need to match the api, terminology etc, it's just to give an idea. then we could make 1 or 2 examples with all the shapes that we have builtin



FURTHER EXAMPLES
- i would like showcases/surface_grid, like the one in legacy, and showing wireframes
- i would like features/instancing showing a single mesh instanced multiple times with different transforms, like in legacy
- i would like features/isolines, what would you recommend? could be on mesh, in image, or both?
- i would like raw_triangle_vklite and raw_triangle_drp2 like in legacy, to show how to use datoviz without the scene api, where should we put them? not in features, not in visuals, not in showcases i guess?
- bounds_overlay.c in legacy, it would be worth porting it into 1 or 2 feature examples no?
- arcball_gizmo.c in legacy, it would be worth porting it into a feature example no? it should appear in a small inset at the bottom right, together with a mesh in the main panel, synchronized
