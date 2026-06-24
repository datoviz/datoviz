# Handoff: Marker Triangle Angle Convention

Status: new GSP visual-QA finding, semantic decision made; needs Datoviz implementation.

Owner: next Datoviz scene/marker agent.


## Summary

GSP visual QA found that Datoviz v0.4 marker positions and image orientation match the Matplotlib
reference, but `DVZ_MARKER_SHAPE_TRIANGLE` appears to use a different local angle convention.

This is not evidence of a global Y flip. It is evidence that the triangle marker's zero-angle
orientation and positive-angle direction differ from the GSP/Matplotlib reference convention.

GSP currently works around this with:

```text
datoviz_triangle_angle = pi - protocol_triangle_angle
```

The desired Datoviz outcome is now decided:

1. `diameter_px` defines the marker bounding-box width and height in pixels;
2. `position` is the center of that bounding box;
3. `DVZ_MARKER_SHAPE_TRIANGLE` at `angle = 0` uses bbox-normalized vertices
   `(-1, -1)`, `(+1, -1)`, `(0, +1)` in the marker-local `[-1, +1]` square;
4. positive `angle` rotates counter-clockwise in the mathematical/rendered y-up convention.

This intentionally matches the Matplotlib `^` marker path up to scale:

```text
Matplotlib: (0, +0.5), (-0.5, -0.5), (+0.5, -0.5)
Datoviz target: (0, +1), (-1, -1), (+1, -1)
```


## Evidence From GSP

GSP repo: `/home/cyrille/GIT/Viz/GSP_API`

Relevant GSP commits on branch `agentic-gsp-vispy2`:

| Commit | Meaning |
| --- | --- |
| `d667ee5` | Enabled Datoviz alpha blending and first triangle orientation workaround. |
| `404af0b` | Ignored local visual-QA render runs. |
| `8a28f37` | Changed Datoviz triangle mapping from `angle + pi` to `pi - angle`. |
| `3aa15fa` | Strengthened S023 visual QA fixtures with asymmetric Y positions and compass triangles. |

The strengthened S023 contact sheet uses:

```text
case_id: marker/angle_size_stroke_ndc
positions:
  (-0.72, -0.36), (-0.42, 0.30), (-0.12, -0.10),
  (0.20, 0.42), (0.50, -0.26), (0.76, 0.14)
angles:
  0, pi/2, pi, 3*pi/2, pi/4, -pi/4
shape:
  MarkerShape.TRIANGLE
```

Observations after this fixture change:

1. Point positions with positive and negative Y match between Matplotlib and Datoviz.
2. The asymmetric image and point-over-image overlay match between Matplotlib and Datoviz.
3. Marker centers with asymmetric Y positions match between Matplotlib and Datoviz.
4. Triangle orientation matches only when GSP uploads `pi - angle` to Datoviz for triangle shapes.
5. The earlier `angle + pi` workaround fixed zero angle but failed nonzero compass angles.

That pattern rules out a global NDC/data Y flip in Datoviz. The mismatch is marker-local.


## Datoviz Files To Inspect

Likely public/API files:

1. `include/datoviz/scene.h`
2. `include/datoviz/scene/enums.h`
3. `include/datoviz/scene/types.h`
4. `spec/scene/visuals/MARKER.md`
5. `docs/reference/visual-families/marker.md`
6. `examples/c/visuals/marker.c`

Likely implementation/test files:

1. `src/scene/visuals/marker/api.c`
2. `src/scene/visuals/marker/shader.c`
3. `src/scene/visuals/point/shader.c`
4. `src/scene/visuals/registry/registry.c`
5. `src/scene/tests/visuals/families_2d.c`
6. `src/scene/tests/query.c`
7. shader sources/registrations for `marker_vert`, `marker_frag`, `marker_bitmap_vert`, and
   `marker_distance_frag`.


## Decided Semantics

Datoviz should implement the following public marker contract before v0.4 API freeze:

1. `position` is the center of the marker's screen-space bounding box.
2. `diameter_px` is the width and height of that bounding box in logical pixels.
3. Built-in marker shapes are normalized to the marker-local `[-1, +1] x [-1, +1]` box unless
   their public spec says otherwise.
4. `DVZ_MARKER_SHAPE_TRIANGLE` at `angle = 0` fills that box with vertices:

   ```text
   (-1, -1), (+1, -1), (0, +1)
   ```

5. `angle` is in radians, rotates the marker glyph around the bbox center, and positive angles
   rotate counter-clockwise in the same y-up rendered coordinate convention users see for retained
   2D/data visuals.

The current Datoviz SDF triangle is not this shape. Sampling the current shader's `sdTriangle`
gives an equilateral-ish shape with approximate vertices:

```text
(-1, -0.577), (+1, -0.577), (0, +1.155)
```

That explains the baseline mismatch: the base edge is around `y = -0.577` instead of `y = -1`.
It also means the apparent marker shape is not bbox-normalized like square, Matplotlib `^`, or the
intended GSP protocol semantics.


## Acceptance Criteria

1. A Datoviz test or example renders compass-style triangle markers with angles:

   ```text
   0, pi/2, pi, 3*pi/2, pi/4, -pi/4
   ```

   at asymmetric Y positions, so both center-coordinate Y flips and local angle direction mistakes
   are visible.

2. The marker docs/spec state the decided contract:

   - `diameter_px` is bbox width/height;
   - `position` is bbox center;
   - `DVZ_MARKER_SHAPE_TRIANGLE` vertices at `angle = 0` are
     `(-1, -1), (+1, -1), (0, +1)`;
   - positive `angle` is counter-clockwise in rendered y-up coordinates.

3. Update `examples/c/visuals/marker.c` and focused marker tests so they exercise the new expected
   behavior.

4. Validate both native GLSL and WebGPU/WGSL marker paths, because both have marker-local rotation
   code and SDF code.

5. Notify GSP after the Datoviz fix:

   - GSP should remove `_datoviz_marker_angles()` triangle remapping.
   - GSP should regenerate S023 visual QA and verify Matplotlib/Datoviz parity.


## Suggested Minimal Probe

Create or extend a retained marker example with:

1. white background;
2. equal-aspect 2D panel with data domain `[-1, 1]` in both axes;
3. six large `DVZ_MARKER_SHAPE_TRIANGLE` items at asymmetric Y positions;
4. visible stroke;
5. colors and sizes matching the GSP S023 compass case.

The key visual result should be obvious without pixel-perfect image comparison: the six arrows must
point in the expected compass directions while their centers remain in the expected positive/negative
Y positions.
