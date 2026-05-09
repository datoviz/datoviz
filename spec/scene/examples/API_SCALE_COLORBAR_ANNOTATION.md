# API Sketch: Scale, Colorbar, And Annotation Label

This example pressure-tests the public API shape for shared scale objects, colormap objects,
panel-attached colorbars, and retained annotation labels.


## Owning Specs

Read this against:

1. `../api/API_SURFACE.md`
2. `../semantics/SCALES.md`
3. `../semantics/LEGENDS_AND_COLORBARS.md`
4. `../semantics/ANNOTATIONS.md`
5. `../proposals/ANNOTATION_TEXT_SCALE_API.md`


## Desired User Flow

```c
DvzScene* scene = dvz_scene();
DvzFigure* fig = dvz_figure(scene, 1000, 700, 0);
DvzPanel* panel = dvz_panel(fig, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});

DvzScale* depth = dvz_scale(scene, &(DvzScaleDesc){
    .kind = DVZ_SCALE_CONTINUOUS,
    .label = "Depth",
    .unit = "um",
});
dvz_scale_set_domain(depth, -600.0, 0.0);

DvzColormap* depth_map = dvz_colormap_builtin(scene, DVZ_BUILTIN_COLORMAP_MAGMA);
dvz_scale_set_colormap(depth, depth_map);

DvzColorbar* colorbar = dvz_colorbar(panel, depth, &(DvzColorbarDesc){
    .orientation = DVZ_COLORBAR_ORIENTATION_VERTICAL,
    .anchor = DVZ_SCENE_ANCHOR_PANEL_RIGHT,
    .title = "Depth",
});
dvz_colorbar_set_format(colorbar, &(DvzFormatDesc){
    .precision = 0,
    .unit = "um",
});

DvzAnnotation* label = dvz_annotation_label(panel, &(DvzLabelDesc){
    .text = "Layer boundary",
    .placement =
        (DvzTextPlacement){
            .mode = DVZ_TEXT_PLACEMENT_DATA,
            .anchor = DVZ_SCENE_ANCHOR_DATA,
            .position = {125.0, -320.0, 0.0},
        },
});
dvz_annotation_set_format(label, &(DvzFormatDesc){
    .unit = "um",
});
```


## API Pressure

This flow requires:

1. scales and colormaps as scene-owned semantic objects,
2. colorbars as panel-attached explanatory objects,
3. annotations as retained scene-owned handles,
4. one shared formatting descriptor usable by colorbars and labels,
5. annotation placement that can be data-anchored without depending on runtime details.
