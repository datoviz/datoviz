showcases/brain_volume
i want to see it from the other side by default

showcases/choropleth
for aspect-fixed right-drag zoom, the boundary line between zoom in and zoom out seems currently to be x=0 vertical line. I would like it to be y=-x line which may be more natural, ie zoom in should work when right draft right or up

showcases/linked_probe_colorbar
the text on the left panel at the top is hard to read because of its color on the background, also not enough padding above it, too close to the clip top border

showcases/surface_grid
i'm not sure if the lighting at the bottom of the grid is correct

showcases/synthetic_mouse
there is a fake mesh apparently, again, ensure the example fails if the data is not available, NOT simulated data

showcases/textured_planet
initially the camera rotates slowly, but if i rotate the camera with the mouse and then stop, the automatic rotation of the camera stops too. I would like it to continue as before

composites/polygon
i want fixed aspect ratio and also all polygons should be visible initially, i shouldn't have to zoom out to see all polygons

composites/graph
i want another more compelling and realistic example of a graph, let's brainstorm about this one

features/animation_tracks
i think it would be cleaner and clearer with a reference grid on the XZ plane

features/annotation_readout
the text is too close from the point, there is some overlap and the first character of the text is hard to read

features/axis_labels
why is this in a small panel with a border? looks weird

features/builtin_shapes_2d
i want fixed/equal aspect ratio

features/builtin_shapes_3d
good but not well balanced in terms of symmetry, the objects on the scene

features/compute_buffer_animation
works but stops unless the mouse moves in the window, a problem with app request frame or something

features/controller_arcball
we need a XZ reference plane grid

features/controller_fly
left drag camera movement is too sensitive

features/controller_orbit_camera
we need a XZ reference plane grid

features/controller_turntable
we need a XZ reference plane grid

features/gui_controls
i think we need to show more capabilities of the datoviz gui wrappers, with more controls in the gui

features/guide_lines
problem with the placement of the line labels, they seem to be in world space, they get smaller or bigger as i zoom in or out, and their position is totally wrong

features/guide_spans
problem with the placement of the spans labels, they seem to be in world space, they get smaller or bigger as i zoom in or out, and their position is totally wrong

features/lighting
we need panel legends
the camera should be much less zoomed in initially
the spheres are way too close together
perhaps we could link the camera across the panels?

features/material_mesh
i would like different panels with the same mesh but different materials, and linked orbit camera across the panels. also with panel legends

features/orientation_gizmo
there was a much nicer 3D gizmo in the legacy examples no? also, when rotating the arcball and releasing the mouse, the gizmo slightly shifts whereas the model does not so it is not accurate.

features/overlay_card
the visual shown is very weird, just show a simple curve or something

features/picking
click to toggle selection doesn't appear to do anything

features/reference_grid
ok but could it be made infinite? perhaps with some fog at a distance?
also i would like the cube to be immediately on the plane, not with some space between them

features/selection_mesh_instances
instance hover is badly broken

features/selection_pixel
y flip problem (same as showcases/embedding_atlas) and click to select doesn't appear to do anything

features/technique_depth_cue
features/technique_edl
need arcball to compare both panels (and with linked panels)

features/technique_msaa
need linked panels

features/technique_ssao
still need to tweak the ssao params, check the protein example and use the same params? more blur etc. also we need the arcball

features/technique_transparency
the first two panels are empty

features/video_export
doesn't record a video by default, doesn't stop automatically after a few seconds

features/visibility
could we toggle visibility on and off like 2x per second?







visuals/glyph
explain this visual, it's unclear to me its relationship with marker and text
why am i not seeing text characters in this example
why do we need it, when is it expected to be used

showcases/lipid_brain_atlas
that's not what i had in mind, i would have imagined millions of color points in 3D (voxels basically) with a way to slice through it using a gui. wdyt?

showcases/embedding_atlas
a persisting bug that we really need to fix properly, we need to think about the right long term fix here. as soon as i hover a point, the image immediately y flips. then the hover is not working and seems y flipped too.
also i am not sure what this dataset is supposed to show, is it fake/simulated? If so, it needs to be replaced by actual data. NO FALLBACK to simulated, it should fail if the data is not available, with clear instructions as to how to prepare it.

features/bounds_overlay
something is wrong because the dots or spheres go a bit beyond the bounding box

features/gui_viewport
still the same instability problem when resizing the viewport

features/panel_domain_fit
i still don't understand that one, explain

features/triangulation_polygon
this is ugly, think of a nicer example. and showing triangulation wireframe in fact

features/volume_occlusion
this one is pretty bad, let's brainstorm on it carefully




NEW EXAMPLES/GENERAL COMMENTS
- i would like a new feature example showing linking of arcball in 2 controllers with two meshes but linked arcballs
- when resizing the window to a size that is too small, error and visual bugs and "_app_draw emit failed: scene grid layout resolution failed"
