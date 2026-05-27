# API Pressure Sketches

> **Example status:** API sketch bundle
> **Target:** C API design pressure and future examples
> **Data:** synthetic or inline
> **Validation:** design review, then focused examples once APIs exist

These sketches may use provisional names. Treat them as pressure tests unless installed headers
already define the symbols.


## `image_probe_pinned_readout`

Pressure: image probe request payloads, stable panel/visual/item identity, pinned readout text,
formatting, stale-result handling, and multi-panel routing.

Minimal target: probe an image-backed sampled field, pin one result, and render or update a compact
readout without requiring an application-level callback API.


## `scale_colorbar_annotation`

Pressure: scales, colormaps, continuous colorbars, categorical legends, annotations, labels, and
data/screen placement.

Minimal target: one image or mesh visual with a scale and colorbar plus one retained annotation.
Richer legend layout and publication typography belong later or in GSP/Matplotlib.


## `sampled_field`

Pressure: shared 2D/3D resource ownership across image, labels, and volume consumers; full and
region updates; geometry metadata; and probe payloads.

Minimal target: one sampled field bound to multiple consumers without duplicating source data.


## `mesh_selection_link`

Pressure: mesh face/region identity, selection styling, link channels, and propagation into another
panel or visual. This is v0.5 because marker/image selection should stabilize first.
