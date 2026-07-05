# Handoff: Microbubble showcase example using retained visual item ranges

**Target branch:** `v0.4-dev`, after the retained visual item-range slice is merged.

**Primary goal:** implement a Datoviz C showcase that renders animated 3D microbubble tracks efficiently with retained GPU data and per-frame visual item-range updates, without custom shaders and without adding domain-specific concepts to Datoviz core.

---

## 1. Scope

Implement a focused example and data-preparation path:

```text
microbubbles outputs/viewer/data/tracks.bin
    -> Python preparation script
    -> data/examples/microbubbles/prepared/microbubbles_dvz.bin
    -> examples/c/showcases/microbubbles.c
    -> two retained Datoviz point layers updated via visual item ranges
```

The example should prove that the new v0.4 RC visual item-range API supports a real time-sorted scientific event-cloud use case:

```c
dvz_visual_set_item_range(visual, first_item, item_count)
```

The example must not require per-frame upload of positions, colors, or sizes.

---

## 2. Hard boundaries

### In scope

- A C-first showcase under `examples/c/showcases/`.
- A small Python data-preparation script that converts Aleph microbubble viewer `tracks.bin` into a simple Datoviz-ready binary.
- Runtime animation by changing visual item ranges only.
- Two visual layers:
  - accumulated faint vessel map;
  - bright moving front / recent tail.
- Default dark scientific visual style.
- Arcball or orbit-camera interaction.
- Clean failure when prepared data is absent, with an exact preparation command.
- Manifest/docs/example metadata updates following existing example conventions.

### Out of scope

- No Datoviz core concepts named `microbubble`, `track`, `frame`, `speed`, `intensity`, or similar domain fields.
- No new visual family.
- No custom shader framework.
- No visual modifier framework.
- No GPU compaction, indirect draw, or compute filtering.
- No shader-side temporal predicate.
- No runtime synthetic fallback if the prepared dataset is missing.
- No bundled 98 GB raw data, no pipeline execution inside Datoviz, no dependency on the Aleph repository at runtime.

Datoviz receives only generic visual payloads:

```text
position        vec3 f32
color           rgba u8
diameter_px     f32
item ranges     first/count
```

The Python preparation layer owns all domain interpretation.

---

## 3. Assumptions

The retained visual item-range slice exists and is tested for `point`:

```c
int  dvz_visual_set_item_range(DvzVisual* visual, uint32_t first_item, uint32_t item_count);
DvzResult dvz_visual_clear_item_range(DvzVisual* visual);
bool dvz_visual_get_item_range(const DvzVisual* visual, DvzItemRange* out);
```

Required semantics for the example:

- Range changes affect draw contribution, not attribute buffers.
- `item_count == 0` renders nothing.
- Picking/query item ids, if enabled, remain global logical item indices, not range-local indices.
- Multi-panel behavior is not needed for this showcase.
- `point` is the required first proof. `splat` is optional only if it is already range-safe.

If `splat` item ranges are not implemented or not stable, use `point` only.

---

## 4. Input data

The external input is the Aleph microbubble viewer binary:

```text
outputs/viewer/data/tracks.bin
```

Expected source format, produced by `ultratrace-ulm track-viewer`:

```text
magic:   ULMT
version: 3 preferred
header:  n_tracks, total_points, max_speed, bounds_min, bounds_max
tracks:  uint32 point_offset, uint32 length
points:  float32 x, y, z, frame, speed, intensity
```

Version 2 may be accepted only if easy, but version 3 should be the primary path because it includes intensity.

Do not make the C example read the Aleph format directly. The C example should read a simple Datoviz-prepared binary.

---

## 5. Datoviz-prepared binary format

Create a simple binary format for the example. Suggested filename:

```text
data/examples/microbubbles/prepared/microbubbles_dvz.bin
```

Suggested magic/version:

```text
magic:   MBVZ
version: 1
```

Suggested layout:

```c
// All integers little-endian.
// Keep header fixed-size and offset-based so C parsing stays simple.
typedef struct MicrobubbleDvzHeader
{
    uint32_t magic;              // 'MBVZ'
    uint32_t version;            // 1
    uint32_t header_size;        // sizeof(MicrobubbleDvzHeader)
    uint32_t flags;              // reserved, 0

    uint32_t point_count;
    uint32_t frame_count;
    uint32_t frame_offsets_count; // frame_count + 1
    uint32_t reserved0;

    float bounds_min[3];         // normalized visual coordinates
    float bounds_max[3];
    float source_bounds_min[3];  // source mm coordinates
    float source_bounds_max[3];

    uint64_t frame_offsets_offset; // uint32[frame_count + 1]
    uint64_t positions_offset;     // float32[point_count][3]
    uint64_t accum_colors_offset;  // rgba_u8[point_count]
    uint64_t front_colors_offset;  // rgba_u8[point_count]
    uint64_t accum_sizes_offset;   // float32[point_count]
    uint64_t front_sizes_offset;   // float32[point_count]

    uint64_t file_size;
} MicrobubbleDvzHeader;
```

Arrays should be tightly packed and 8-byte aligned. The loader should validate:

- magic;
- version;
- file size;
- offsets are increasing and inside file;
- array byte ranges do not overflow;
- `frame_offsets_count == frame_count + 1`;
- `frame_offsets[0] == 0`;
- `frame_offsets[frame_count] == point_count`;
- frame offsets are monotonic.

Optional sidecar metadata:

```text
data/examples/microbubbles/prepared/microbubbles_dvz.json
```

The C example should not require a JSON parser. The JSON is for human inspection only.

---

## 6. Python preparation script

Add a script, suggested path:

```text
tools/prepare_microbubbles_demo.py
```

Suggested command:

```bash
python3 tools/prepare_microbubbles_demo.py \
  --input /path/to/microbubbles/outputs/viewer/data/tracks.bin \
  --output data/examples/microbubbles/prepared/microbubbles_dvz.bin \
  --min-length 35 \
  --max-points 2000000
```

The script should:

1. Read and validate the Aleph `tracks.bin` header/table/points.
2. Prefer version 3. If accepting version 2, set intensity to `1.0`.
3. Optionally filter tracks by minimum length if the source table is available.
4. Build one flat point list.
5. Sort points by integer frame, stable within frame.
6. Build `frame_offsets[frame_count + 1]` so that all points for frame `f` are in:

   ```text
   [frame_offsets[f], frame_offsets[f + 1])
   ```

7. Normalize positions to Datoviz visual coordinates, centered and aspect-preserving:

   ```python
   center = 0.5 * (bounds_min + bounds_max)
   scale = 1.8 / np.max(bounds_max - bounds_min)
   pos = (pos_mm - center) * scale
   ```

8. Precompute all visual encodings on the CPU:

   ```text
   accum_colors: low-alpha cyan/blue vessel map
   front_colors: brighter speed/intensity-colored head/tail points
   accum_sizes:  small screen-space diameters
   front_sizes:  larger screen-space diameters
   ```

9. Write the Datoviz-prepared binary.
10. Write optional JSON metadata:

    ```json
    {
      "source": ".../tracks.bin",
      "point_count": ...,
      "frame_count": ...,
      "source_bounds_mm": [[...], [...]],
      "normalized_bounds": [[...], [...]],
      "max_speed": ...,
      "created_by": "tools/prepare_microbubbles_demo.py"
    }
    ```

Do not make this script depend on the Aleph Python package. Use only standard Python plus NumPy.

A tiny `--dry-run` or `--inspect` mode is useful but optional.

A `--synthetic` mode may be useful for developing the loader, but the runtime C showcase must not silently synthesize fallback data. If `--synthetic` is added, document it clearly as a development-only preparation mode and do not make it the release/example default.

---

## 7. C showcase implementation

Add:

```text
examples/c/showcases/microbubbles.c
```

Optionally split the loader if the file becomes large:

```text
examples/c/showcases/microbubbles_loader.h
examples/c/showcases/microbubbles_loader.c
```

Use a single-file example if that is more consistent with nearby showcases and keeps CMake simpler.

### Default data path

Use:

```c
#define DEFAULT_DATA_PATH "data/examples/microbubbles/prepared/microbubbles_dvz.bin"
```

Allow override through an environment variable if scenario-runner custom CLI arguments are inconvenient:

```text
DVZ_MICROBUBBLES_DATA=/path/to/microbubbles_dvz.bin
```

If the file is missing, fail cleanly and print the exact preparation command:

```text
Missing prepared microbubble data: data/examples/microbubbles/prepared/microbubbles_dvz.bin
Prepare it with:
  python3 tools/prepare_microbubbles_demo.py --input /path/to/outputs/viewer/data/tracks.bin --output data/examples/microbubbles/prepared/microbubbles_dvz.bin
```

Do not generate fake data in the C example.

### Visuals

Use two retained visuals over the same logical point count.

Minimum path using existing dense API:

```c
DvzVisual* accum = dvz_point(ctx->scene, 0);
DvzVisual* front = dvz_point(ctx->scene, 0);

// Accumulated faint layer.
dvz_visual_set_data(accum, "position",    data->positions,    data->point_count);
dvz_visual_set_data(accum, "color",       data->accum_colors, data->point_count);
dvz_visual_set_data(accum, "diameter_px", data->accum_sizes,  data->point_count);

// Moving bright layer.
dvz_visual_set_data(front, "position",    data->positions,    data->point_count);
dvz_visual_set_data(front, "color",       data->front_colors, data->point_count);
dvz_visual_set_data(front, "diameter_px", data->front_sizes,  data->point_count);

// Initially render nothing until the first frame callback.
dvz_visual_set_item_range(accum, 0, 0);
dvz_visual_set_item_range(front, 0, 0);
```

If shared attribute buffers are already available, use them for the `position` buffer. If not, duplicate the retained upload for the two visuals; the important RC proof is that animation does not re-upload per frame.

Set styles:

```c
DvzPointStyleDesc style = dvz_point_style_desc();
style.aspect = DVZ_SHAPE_ASPECT_FILLED;
style.stroke_width_px = 0.0f;
dvz_point_set_style(accum, &style);
dvz_point_set_style(front, &style);
```

Recommended alpha/depth settings:

```c
dvz_visual_set_depth_test(accum, true or false after visual test);
dvz_visual_set_depth_test(front, true or false after visual test);
dvz_visual_set_alpha_mode(accum, DVZ_ALPHA_BLENDED or DVZ_ALPHA_WBOIT if stable);
dvz_visual_set_alpha_mode(front, DVZ_ALPHA_BLENDED or DVZ_ALPHA_WBOIT if stable);
```

Choose the simplest mode that produces stable native and offscreen output. Do not block the example on perfect transparency.

### Animation

Use the scenario frame callback.

State:

```c
typedef struct MicrobubbleState
{
    MicrobubbleData data;
    DvzVisual* accum;
    DvzVisual* front;
    uint32_t tail_frames;
    float playback_frames_per_second;
    float hold_seconds;
} MicrobubbleState;
```

Frame logic:

```c
static void _scenario_frame(DvzScenarioContext* ctx, void* user)
{
    MicrobubbleState* state = (MicrobubbleState*)user;
    const uint32_t frame_count = state->data.frame_count;
    if (frame_count == 0)
        return;

    double t = ctx->time * state->playback_frames_per_second;
    uint32_t frame = (uint32_t)fmod(t, (double)frame_count);

    uint32_t hi = state->data.frame_offsets[frame + 1];
    uint32_t tail0 = frame > state->tail_frames ? frame - state->tail_frames : 0;
    uint32_t lo = state->data.frame_offsets[tail0];

    dvz_visual_set_item_range(state->accum, 0, hi);
    dvz_visual_set_item_range(state->front, lo, hi - lo);
}
```

Optional: add a hold/reset cycle later. First keep the animation simple.

### Camera and panel

Use a dark panel background and a 3D camera/arcball or orbit camera, following existing showcase style.

Recommended setup:

```c
ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
DvzPanel* panel = dvz_panel_full(ctx->figure);
example_graphite_cyan_set_panel_background(panel);
example_set_default_3d_camera(panel, 1.0f);

DvzController* controller = dvz_arcball(ctx->scene, NULL);
dvz_scenario_bind_controller(ctx, panel, controller, DVZ_DIM_MASK_XYZ);
```

If the point cloud orientation looks wrong, fix orientation in the Python preparation script, not in Datoviz core.

---

## 8. Optional enhancements after the minimal proof

Only after the minimal point version works:

1. Try `splat` for the accumulated layer if item ranges are implemented for splat and output remains stable.
2. Add a faint bounding box or reference grid.
3. Add a small text/readout overlay with point count and frame index if existing text APIs are stable.
4. Add GUI controls only if existing GUI example patterns are stable and this does not expand scope:
   - playback speed;
   - tail length;
   - point size multiplier.
5. Add offscreen recording proof if the scenario runner supports it cleanly.

Do not add custom shaders for glow/jitter in this pass. The visual effect should come from prepared RGBA/diameter arrays plus the two item-range layers.

---

## 9. Repo files likely to update

Expected additions:

```text
tools/prepare_microbubbles_demo.py
examples/c/showcases/microbubbles.c
```

Expected edits:

```text
examples/c/CMakeLists.txt
examples/c/MANIFEST.yaml
spec/scene/examples/PLANNING.md                 # only if current process requires example planning entry
docs / generated gallery metadata               # only through existing generation/check process
agents/now/STATUS.md                            # one short active-lane/status note if implementation becomes RC work
```

Do not touch generated files by hand. Use the repo’s existing example-manifest/gallery generation workflow if needed.

---

## 10. Manifest and data policy

Follow the current example data guardrail:

- If the example declares prepared data, it must not silently synthesize runtime fallback data when the expected bundle is absent.
- It should fail with the exact preparation command.
- Synthetic/simulated data is acceptable only when explicit in the manifest and example contract.

For this showcase, the data source should be declared as external/prepared. Do not commit large real microbubble data.

If a tiny prepared fixture is later needed for CI, add it as a separate explicit fixture decision and keep it very small.

---

## 11. Validation checklist

After implementation, run the narrowest relevant checks first:

```bash
git diff --check
python3 tools/prepare_microbubbles_demo.py --help
python3 tools/check_example_manifests.py
just build
just example-c showcases/microbubbles
```

With real prepared data available:

```bash
python3 tools/prepare_microbubbles_demo.py \
  --input /path/to/outputs/viewer/data/tracks.bin \
  --output data/examples/microbubbles/prepared/microbubbles_dvz.bin

./build/examples/c/showcases/microbubbles --png
./build/examples/c/showcases/microbubbles --live
```

If supported and cheap:

```bash
./build/examples/c/showcases/microbubbles --offscreen-record 120
```

If WebGPU/WASM promotion is not in scope, explicitly leave it native-only/prepared-data for now. Do not reimplement the visualization in JavaScript.

---

## 12. Acceptance criteria

The implementation is acceptable when:

1. The example compiles with the native C examples.
2. Missing data produces a clear error and exact preparation command.
3. Prepared real Aleph `tracks.bin` converts successfully.
4. The example renders an animated 3D microbubble cloud using two retained visual layers.
5. Per-frame animation calls only item-range setters, not `dvz_visual_set_data()` or data-range uploads.
6. The animation loops smoothly and remains interactive with the 3D controller.
7. At least one PNG capture artifact is produced from prepared data.
8. The example metadata/manifest is consistent with the repo’s gallery/example checks.
9. The implementation does not add domain-specific API to Datoviz core.
10. Broader GPU features remain explicitly deferred.

---

## 13. Suggested commit structure

Prefer small commits:

```text
1. examples: add microbubble preparation tool
2. examples: add microbubble item-range showcase
3. docs: register microbubble showcase metadata
```

If this is done as one commit, use:

```text
examples: add microbubble item-range showcase
```

---

## 14. Implementation prompt for an agent

Copy-pasteable prompt:

```text
You are working in my local clone of datoviz/datoviz on v0.4-dev, after the retained visual item-range API has landed.

Implement the native C microbubble showcase described in agents/now/HANDOFF_MICROBUBBLE_EXAMPLE.md.

Important constraints:
- Do not add Datoviz core semantics for speed, frame, track, intensity, or microbubbles.
- Do not add a new visual family.
- Do not add custom shaders, visual modifiers, GPU compaction, indirect draw, or compute filtering.
- Use prepared generic visual arrays and visual item ranges.
- Runtime animation must update only visual item ranges, not re-upload point attributes per frame.
- If prepared data is absent, fail with the exact preparation command; do not synthesize fallback data in the C example.

Implementation target:
- Add tools/prepare_microbubbles_demo.py to convert Aleph outputs/viewer/data/tracks.bin into data/examples/microbubbles/prepared/microbubbles_dvz.bin.
- Add examples/c/showcases/microbubbles.c using two retained point visual layers: accumulated and front/tail.
- Register the example in examples/c/CMakeLists.txt and examples/c/MANIFEST.yaml following nearby showcase conventions.
- Update only the minimal docs/agent/status files required by repo convention.

Validation:
- git diff --check
- python3 tools/prepare_microbubbles_demo.py --help
- python3 tools/check_example_manifests.py
- just build
- just example-c showcases/microbubbles
- With real data: prepare the binary and run ./build/examples/c/showcases/microbubbles --png and --live.

Show the diff summary and be explicit about any validation you could not run.
```
