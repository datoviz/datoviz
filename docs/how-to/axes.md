# Axes

Add labeled tick axes to a panel and control tick placement, grid lines, and axis titles.

## Overview

Axes attach to a panel's data domain. Call `dvz_panel_set_domain()` to define the data-coordinate
range on each dimension, then retrieve the axis with `dvz_panel_axis()`. Tick policy, grid lines,
labels, and optional datetime formatting are all set on the returned `DvzAxis*`.

## Example

=== "C"

    ```c
    #include <stdint.h>
    #include "datoviz/scene.h"

    #define N 256

    int main(void) {
        /* scene */
        DvzScene* scene = dvz_scene();
        DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
        DvzPanel* panel = dvz_panel_full(figure);

        /* set data domain so axes know the tick range */
        dvz_panel_set_domain(panel, DVZ_DIM_X, 0.0, 10.0);
        dvz_panel_set_domain(panel, DVZ_DIM_Y, -2.0, 2.0);

        /* retrieve the auto-created axes */
        DvzAxis* x_axis = dvz_panel_axis(panel, DVZ_DIM_X);
        DvzAxis* y_axis = dvz_panel_axis(panel, DVZ_DIM_Y);

        /* configure tick placement */
        DvzAxisTickPolicy ticks = dvz_axis_tick_policy();
        ticks.target_count = 6;
        ticks.min_pixel_spacing = 110.0f;
        ticks.minor_per_interval = 3;
        dvz_axis_set_tick_policy(x_axis, &ticks);
        dvz_axis_set_tick_policy(y_axis, &ticks);

        /* enable grid lines */
        dvz_axis_set_grid(x_axis, true);
        dvz_axis_set_grid(y_axis, true);

        /* set axis titles */
        dvz_axis_set_label(x_axis, "time (s)");
        dvz_axis_set_label(y_axis, "signal");

        /* bind pan/zoom so axes update on interaction */
        DvzController* controller = dvz_panzoom(scene, NULL);
        dvz_panel_bind_controller(panel, controller, DVZ_DIM_MASK_XY);

        /* run */
        DvzApp* app = dvz_app(scene);
        dvz_view_glfw(app, figure, "Axes");
        dvz_app_run(app, 0);
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 0;
    }
    ```

<!-- TODO: Python -->

## Step by step

Set the data domain before retrieving axes. `dvz_panel_set_domain()` tells the panel the
real-world coordinate range for each dimension. This range drives tick generation and the mapping
from data positions to screen pixels.

Retrieve axes with `dvz_panel_axis()`. Each panel has one axis per dimension; the axis is created
automatically when the panel is created. You configure it in place — no separate allocation is
needed.

Configure tick policy via `dvz_axis_tick_policy()`. `target_count` is a hint for how many major
ticks to aim for; `min_pixel_spacing` prevents crowding at small panel sizes; `minor_per_interval`
controls the number of minor ticks between each pair of major ticks.

Call `dvz_axis_set_grid()` to draw faint grid lines at each major tick. Call
`dvz_axis_set_label()` to add a title string along the axis.

Bind a panzoom controller so axes re-tick automatically as the user pans or zooms. Without a
controller the axes are static.

## Common patterns / Variants

**Axis titles only, no grid:**

```c
dvz_axis_set_label(x_axis, "x");
dvz_axis_set_label(y_axis, "y");
/* grid lines left off */
```

**Datetime axis (UTC):**

```c
DvzDateTimeFormat* fmt = dvz_datetime_format_create(scene);
dvz_datetime_format_timezone(fmt, "UTC");
dvz_datetime_format_rule(fmt, DVZ_TIME_INTERVAL_SECOND, "%H:%M:%S");
dvz_datetime_format_rule(fmt, DVZ_TIME_INTERVAL_MINUTE, "%H:%M");
dvz_datetime_format_rule(fmt, DVZ_TIME_INTERVAL_HOUR,   "%H:%M");
dvz_datetime_format_rule(fmt, DVZ_TIME_INTERVAL_DAY,    "%b %d");

/* t0 and t1 are DvzTimestamp values (microseconds since epoch) */
dvz_axis_set_datetime(x_axis, fmt);
dvz_axis_set_datetime_range(x_axis, 0.0, 8.0, t0, t1);
```

The domain values (`0.0` to `8.0`) are the compact floating-point coordinates used in your data;
`t0`/`t1` map them to real UTC timestamps for formatting.

## See also

- [Coordinate systems](coordinate-systems.md)
- [Transforms and scales](transforms-and-scales.md)
- [Create multiple panels](create-multiple-panels.md)
- [Adornments](adornments.md)
