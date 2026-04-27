# Next Steps — Scene Spec Work

This file tracks outstanding spec tasks in priority order.
Pick up from here in the next working session.

The spec is otherwise complete: all visual family specs are done, all deferred questions
across `spec/scene/` are resolved, and `spec/scene/headers/scene_api.h` covers the full
v0.4 C API surface.


## 1. Fill gaps in `scene_api.h`

Several decisions made during the deferred-question resolution pass are not yet reflected
in the header. Add the following to `spec/scene/headers/scene_api.h`:

### `DvzMutability` enum + setter

```c
typedef enum {
    DVZ_MUTABILITY_DYNAMIC   = 0,  /* default: scene copies data on write */
    DVZ_MUTABILITY_STATIC    = 1,  /* upload once; no further writes expected */
    DVZ_MUTABILITY_STREAMING = 2,  /* updated every frame; scene uses mapped memory */
} DvzMutability;

DVZ_EXPORT void dvz_visual_set_mutability(DvzVisual* visual, const char* attr_name,
                                           DvzMutability hint);
```

### Scale typed constructors + update functions

The existing `dvz_scale(scene, kind_flags)` generic call stays, but add these shorthands:

```c
/* Typed constructors */
DVZ_EXPORT DvzScale* dvz_scale_color(DvzScene* scene, const char* colormap_name,
                                      double domain_min, double domain_max);
DVZ_EXPORT DvzScale* dvz_scale_size(DvzScene* scene, float px_min, float px_max,
                                     double domain_min, double domain_max);
DVZ_EXPORT DvzScale* dvz_scale_opacity(DvzScene* scene, double domain_min, double domain_max);

/* Update functions (operate on any DvzScale*) */
DVZ_EXPORT void dvz_scale_set_domain(DvzScale* scale, double min, double max);
DVZ_EXPORT void dvz_scale_set_colormap(DvzScale* scale, const char* colormap_name);
DVZ_EXPORT void dvz_scale_set_stops(DvzScale* scale, const uint8_t* rgba_stops, uint32_t count);
DVZ_EXPORT void dvz_scale_destroy(DvzScale* scale);
```

### Panel colorbar convenience wrapper

Add a `DvzPanelSide` enum and the wrapper:

```c
typedef enum {
    DVZ_PANEL_SIDE_TOP    = 0,
    DVZ_PANEL_SIDE_RIGHT  = 1,
    DVZ_PANEL_SIDE_BOTTOM = 2,
    DVZ_PANEL_SIDE_LEFT   = 3,
} DvzPanelSide;

/* Creates a fixed-width adjacent panel for a colorbar and returns its handle. */
DVZ_EXPORT DvzPanel* dvz_panel_attach_colorbar(DvzPanel* panel, DvzPanelSide side,
                                                float width_px);
```

### Picture-in-picture flag for offscreen panels

Add a flags parameter to `dvz_panel_set_offscreen` (or a separate flag constant used at
call time):

```c
#define DVZ_PANEL_OFFSCREEN_DEFAULT 0x00  /* exclusive: renders to texture only */
#define DVZ_PANEL_OFFSCREEN_PIP     0x01  /* also composite into main framebuffer */

/* Updated signature */
DVZ_EXPORT DvzTexture* dvz_panel_set_offscreen(DvzPanel* panel, uint32_t flags);
```

### Runtime boundary object

```c
typedef struct DvzRuntime DvzRuntime;

DVZ_EXPORT DvzRuntime* dvz_runtime_create(/* backend-specific init params */);
DVZ_EXPORT void        dvz_runtime_destroy(DvzRuntime* rt);
DVZ_EXPORT int         dvz_runtime_submit(DvzRuntime* rt, DvzFramePlan* frame_plan);
DVZ_EXPORT int         dvz_runtime_get_capabilities(DvzRuntime* rt,
                                                     DvzCapabilitySnapshot* out_caps);
```


## 2. Retire `SCENE_API_SKETCH.md`

`spec/scene/SCENE_API_SKETCH.md` is a 1137-line older sketch that is superseded by
`spec/scene/headers/scene_api.h`. It should be replaced with a short stub that redirects
to the header:

```markdown
# Scene API Sketch (superseded)

This document is superseded by `spec/scene/headers/scene_api.h`, which is the
authoritative informative header for the v0.4 scene C API.
```

Or delete the file entirely if the redirect is not needed.


## 3. Consistency pass on `PREFERRED_API_PROFILE.md`

`spec/scene/PREFERRED_API_PROFILE.md` defines the Python binding architecture and the
preferred C API profile. It was written before `scene_api.h` was finalised. Check it for:

- Constructor naming: should match `dvz_point`, `dvz_marker`, `dvz_path`, etc. (not
  any older `dvz_scene_visual_*` or `dvz_visual_create_*` patterns).
- `dvz_visual_set_data(visual, attr_name, data, n)` — confirm this is the canonical
  attribute write call (not `dvz_visual_write` or `dvz_visual_set_attr`).
- `dvz_visual_alloc(visual, n)` — confirm this is the pre-allocation hint spelling.
- Python ctypes binding patterns — should use opaque handle types matching
  `DvzVisual*`, `DvzPanel*`, `DvzScale*`, `DvzTexture*`, `DvzFont*`, `DvzSelection*`.
- Any references to removed or renamed functions (e.g. `dvz_scene_visual`,
  `dvz_scene_panel` from the old generic API — these should now be the typed constructors
  and `dvz_panel(fig, desc)`).
- FFI target requirements — still valid? Check against scene_api.h forward declarations.

Update PREFERRED_API_PROFILE to match scene_api.h where inconsistencies are found.


## 4. Audit example specs against v0.4 API

Check all files in `spec/scene/examples/` for consistency with the v0.4 API surface:

- `POINT_2D.md`
- `PATH_AXES_2D.md`
- `MARKER_PICKING.md`
- `IMAGE_SLICE.md`
- `SPHERE_IMPOSTOR.md`
- `VOLUME_OFFSCREEN.md`
- `LINKED_PANELS_AXES_PANZOOM.md`
- `LINKED_PANELS_PROBE_COLORBAR.md`
- `ANIMATION_VIDEO_EXPORT.md`
- `MOUSE_BRAIN_ATLAS_EXPLORER.md`

For each file, check:
- Uses typed constructors (`dvz_point`, `dvz_marker`, …) not `dvz_scene_visual`.
- Uses `dvz_visual_set_data(visual, "attr_name", data, n)` not per-family setters.
- Uses `dvz_panel(fig, desc)` not `dvz_scene_panel(scene, desc)`.
- References correct enum names from scene_api.h (`DVZ_MARKER_SHAPE_DISC` not
  `DVZ_SHAPE_DISC`, `DVZ_CAP_ROUND` not `DVZ_CAP_TYPE_ROUND`, etc.).
- Uses `dvz_scale_color` / `dvz_scale_size` for mappings.
- Any v0.3 function names (`dvz_point_position`, `dvz_marker_color`, etc.) must be
  replaced with `dvz_visual_set_data` calls.

Update examples to use v0.4 patterns and ensure they are self-consistent end-to-end.


## 5. DRP2 spec open-items check

Quickly scan `spec/drp2/` for any remaining open questions that could be resolved
independently of active implementation work. The COMMANDS.md, CAPABILITIES.md,
ERRORS.md, and CONFORMANCE.md files are the most likely to have open items.
