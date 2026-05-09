# API Sketch: Mesh Selection With Link Channel

This example pressure-tests the public API shape for mesh object/face selection and linked
highlighting.


## Owning Specs

Read this against:

1. `../api/API_SURFACE.md`
2. `../interaction/PICKING.md`
3. `../interaction/SELECTION.md`
4. `../proposals/MESH_API_DESIGN.md`
5. `../proposals/INTERACTION_API_DESIGN.md`


## Desired User Flow

```c
DvzScene* scene = dvz_scene();
DvzFigure* fig = dvz_figure(scene, 1200, 800, 0);
DvzPanel* left = dvz_panel(fig, (DvzPanelDesc){0.0f, 0.0f, 0.5f, 1.0f});
DvzPanel* right = dvz_panel(fig, (DvzPanelDesc){0.5f, 0.0f, 0.5f, 1.0f});

DvzMeshResource* mesh = dvz_mesh_resource(scene, &mesh_desc);
DvzVisual* cortex_left = dvz_mesh(left, mesh, 0);
DvzVisual* cortex_right = dvz_mesh(right, mesh, 0);

DvzLinkChannel* region = dvz_link_channel(scene, "region");
dvz_visual_set_link_keys(cortex_left, region, face_region_keys, face_count);
dvz_visual_set_link_keys(cortex_right, region, face_region_keys, face_count);

DvzSelection* selection = dvz_selection(scene, &(DvzSelectionDesc){
    .mode = DVZ_SELECTION_REPLACE,
    .target = DVZ_PICK_TARGET_FACE,
});

DvzInteractionPolicy* policy = dvz_interaction(scene);
dvz_interaction_bind_panel(policy, left);
dvz_interaction_bind_panel(policy, right);
dvz_interaction_set_selection(policy, selection);
dvz_interaction_set_link_channel(policy, region);

dvz_panel_pick(left, mouse_x, mouse_y, &(DvzPickRequest){
    .target = DVZ_PICK_TARGET_FACE,
});

DvzPickResult pick = {0};
if (dvz_scene_poll_pick(scene, &pick) && pick.hit)
{
    dvz_selection_apply_pick(selection, &pick);
}
```


## API Pressure

This flow requires:

1. opaque handles for retained scene objects,
2. public descriptors for mesh resource creation, selection, and pick request,
3. a fixed `DvzPickResult` carrying raw face identity and resolved link target,
4. a public link-key representation,
5. selection state independent from visual ownership,
6. linked highlight behavior across panels without global mutable state.
