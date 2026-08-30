# Panel Frame Snapshot Architecture

Status: implemented v0.4 snapshot contract with retained guide layout, hit testing, and rendered-contribution queries.

## Goal

Datoviz panels need one retained, inspectable frame contract that can serve:

- guide layout/query/readback strictness;
- plot/grid clipping evidence;
- View2D and View3D transform readback;
- future retained DATA-space View3D navigation.

The long-term model is:

```text
DvzPanelFrameSnapshot
    revisioned panel/view/guide/visual state
    panel, inner, plot, and grid clip rectangles
    view transforms
    guide boxes and rendered contributions
    diagnostics
```

## Public API

The immutable public snapshot handle exposes copied frame information and retained guide queries:

```c
DvzPanelFrameSnapshot* dvz_panel_resolve_frame(DvzPanel* panel);
DvzId dvz_panel_frame_id(const DvzPanelFrameSnapshot* snapshot);
bool dvz_panel_frame_info(const DvzPanelFrameSnapshot* snapshot, DvzPanelFrameInfo* out);
uint32_t dvz_panel_frame_guide_count(const DvzPanelFrameSnapshot* snapshot);
bool dvz_panel_frame_guide_layout(const DvzPanelFrameSnapshot* snapshot, uint32_t index, DvzGuideLayout* out);
bool dvz_panel_frame_guide_hit(const DvzPanelFrameSnapshot* snapshot, float x_px, float y_px, DvzGuideHit* out);
uint32_t dvz_panel_frame_contribution_count(const DvzPanelFrameSnapshot* snapshot);
bool dvz_panel_frame_contribution(const DvzPanelFrameSnapshot* snapshot, uint32_t index, DvzRenderedContribution* out);
void dvz_panel_frame_ref(DvzPanelFrameSnapshot* snapshot);
void dvz_panel_frame_unref(DvzPanelFrameSnapshot* snapshot);
```

`DvzPanelFrameInfo` currently exposes:

- snapshot, figure, and panel ids;
- coarse panel/layout/view/guide/visual revisions;
- logical figure size;
- framebuffer size derived from device scale;
- device scale and user scale;
- panel, inner, plot, and grid clip rectangles in logical figure pixels;
- plot/view/controller extents;
- source and visible data domains for the current View2D slice;
- DATA-to-VIEW matrix;
- diagnostics for intentionally incomplete strict guide fields.

The revision fields deliberately share a coarse figure frame revision. This gives callers
stable invalidation behavior now while later retained objects add finer panel/layout/view/guide and
visual revisions.

The grid clip rectangle is the plot rectangle. Axis grid visuals are expected to use plot viewport
and plot clipping for real clipping rather than endpoint trimming.

## Invariants

- Returned snapshots are immutable.
- Releasing a snapshot never mutates panel state.
- Later figure/panel/view changes do not alter existing snapshots.
- A new snapshot after a resize or frame-affecting mutation reports a later revision.
- Snapshot rectangles use logical figure pixels.
- Framebuffer dimensions are explicit and derived from logical size times device scale.
- Missing guide strictness fields are reported through diagnostics rather than silently fabricated.

## Deferred To Later Slices

Future work should split the coarse revision into real retained objects:

- revisioned `DvzView2D` and `DvzView3D` descriptors;
- snapshot-based ray/readback APIs;
- retained DATA-space View3D visual attachments;
- retained update counters proving camera navigation does not reupload unchanged visual buffers.

## Rejected Paths

- Treating visual appearance as guide strictness evidence.
- Overlay masking as grid clipping evidence.
- Keeping strict guides as anonymous generated visuals only.
- Reuploading CPU-projected mesh positions for ordinary View3D navigation.
- Exposing native controller, material, shader, or pipeline names as external protocol semantics.
