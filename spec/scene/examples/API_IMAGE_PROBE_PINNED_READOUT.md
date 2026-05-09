# API Sketch: Image Probe With Pinned Readout

This example pressure-tests the public API shape for image sampling, semantic probe payloads, and
pinned readout annotations.


## Owning Specs

Read this against:

1. `../api/API_SURFACE.md`
2. `../interaction/PICKING.md`
3. `../semantics/ANNOTATIONS.md`
4. `../semantics/SCALES.md`
5. `../proposals/PROBE_READOUT_DESIGN.md`


## Desired User Flow

```c
DvzScene* scene = dvz_scene();
DvzFigure* fig = dvz_figure(scene, 800, 800, 0);
DvzPanel* panel = dvz_panel(fig, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f});

DvzScale* intensity = dvz_scale(scene, &(DvzScaleDesc){
    .kind = DVZ_SCALE_CONTINUOUS,
    .label = "Intensity",
    .unit = "a.u.",
});
dvz_scale_set_domain(intensity, 0.0, 1.0);

DvzColormap* cmap = dvz_colormap_builtin(scene, DVZ_BUILTIN_COLORMAP_VIRIDIS);
dvz_scale_set_colormap(intensity, cmap);

DvzVisual* image = dvz_image(panel, &(DvzImageDesc){
    .width = width,
    .height = height,
    .format = DVZ_IMAGE_FORMAT_F32,
    .scale = intensity,
});
dvz_image_set_data(image, pixels, width, height, sizeof(float));

dvz_panel_probe(panel, mouse_x, mouse_y, &(DvzProbeRequest){
    .target = DVZ_SCENE_TARGET_PIXEL,
});

DvzProbeResult probe = {0};
if (dvz_scene_poll_probe(scene, &probe) && probe.hit)
{
    DvzPinnedReadout* readout = dvz_pinned_readout(panel, &probe);
    dvz_pinned_readout_set_format(readout, &(DvzFormatDesc){
        .precision = 3,
        .unit = "a.u.",
    });
}
```


## API Pressure

This flow requires:

1. a probe request separate from persistent selection,
2. a public `DvzProbeResult` that carries coordinate, scalar value, and scale reference,
3. pinned readouts as retained scene-owned objects,
4. shared formatting between probe labels and scale/colorbar labels,
5. image probes that do not expose texture handles or backend readback details.
