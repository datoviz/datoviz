# GUI Controls

Add an ImGui overlay panel with sliders, checkboxes, and buttons that read and write visual attributes at runtime.

## Overview

Datoviz exposes a subset of Dear ImGui through a `DvzGui` object attached to a view. A GUI callback function is called every frame; inside it you call `dvz_gui_*` widget functions. Return values indicate whether the widget value changed, so you can re-upload data only when necessary.

## Example

=== "C"

    ```c
    #include <stdbool.h>
    #include <stdint.h>
    #include "datoviz/app.h"
    #include "datoviz/gui.h"
    #include "datoviz/scene.h"

    #define N 5

    typedef struct {
        DvzVisual* point;
        float diameter;
        float opacity;
        bool visible;
    } State;

    /* Re-upload point size and alpha whenever a widget changes. */
    static void upload(State* s) {
        float diameters[N];
        uint8_t colors[N * 4];
        for (int i = 0; i < N; i++) {
            diameters[i] = s->diameter;
            colors[4*i+0] = 80;
            colors[4*i+1] = 180;
            colors[4*i+2] = 255;
            colors[4*i+3] = (uint8_t)(255.0f * s->opacity);
        }
        DvzVisualDataUpdate updates[] = {
            {.attr_name = "diameter", .data = diameters, .item_count = N},
            {.attr_name = "color",    .data = colors,    .item_count = N},
        };
        dvz_visual_set_data_many(s->point, updates, 2);
    }

    /* GUI callback — called every frame. */
    static void on_gui(DvzGui* gui, DvzView* view, void* user_data) {
        (void)view;
        State* s = (State*)user_data;
        bool changed = false;

        if (dvz_gui_begin(gui, "Controls", NULL, 0)) {
            changed |= dvz_gui_slider_float(gui, "Diameter", &s->diameter, 8.0f, 64.0f);
            changed |= dvz_gui_slider_float(gui, "Opacity",  &s->opacity,  0.1f,  1.0f);
            if (dvz_gui_checkbox(gui, "Visible", &s->visible))
                dvz_visual_set_visible(s->point, s->visible);
        }
        dvz_gui_end(gui);

        if (changed)
            upload(s);
    }

    int main(void) {
        float positions[N * 3] = {
            -0.6f, -0.3f, 0,
            -0.3f,  0.2f, 0,
             0.0f, -0.1f, 0,
             0.3f,  0.4f, 0,
             0.6f, -0.2f, 0,
        };

        /* scene */
        DvzScene*  scene  = dvz_scene();
        DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
        DvzPanel*  panel  = dvz_panel_full(figure);

        /* visual */
        DvzVisual* point = dvz_point(scene, 0);
        dvz_visual_set_data(point, "position", positions, N);
        dvz_panel_add_visual(panel, point, NULL);

        /* app */
        DvzApp*  app  = dvz_app(scene);
        DvzView* view = dvz_view_glfw(app, figure, 800, 600, "GUI controls");

        /* GUI overlay */
        State state = {.point = point, .diameter = 32.0f, .opacity = 1.0f, .visible = true};
        upload(&state);
        DvzGuiConfig cfg = dvz_gui_config();
        DvzGui* gui = dvz_view_gui(view, &cfg);
        dvz_view_set_gui_callback(view, on_gui, &state);

        dvz_app_run(app, 0);
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 0;
    }
    ```

<!-- TODO: Python -->

## Step by step

Create a scene, figure, panel, and visual as usual, then call `dvz_app()` and `dvz_view_glfw()` to get a `DvzView`. Call `dvz_gui_config()` to get a default configuration, then `dvz_view_gui()` to attach a `DvzGui` overlay to the view. Register your callback with `dvz_view_set_gui_callback()` — it receives the `DvzGui*`, the `DvzView*`, and your `user_data` pointer every frame.

Inside the callback, open a window with `dvz_gui_begin()` and close it with `dvz_gui_end()`. Between those calls, add widgets: `dvz_gui_slider_float()`, `dvz_gui_checkbox()`, `dvz_gui_button()`, etc. Most widget functions return `true` when their value changed in that frame — use this to decide whether to re-upload data to the visual.

Call `dvz_visual_set_data_many()` (or `dvz_visual_set_data()`) inside `upload()` to push the new values to the GPU. For visibility toggling, call `dvz_visual_set_visible()` directly.

## Common patterns / Variants

**Button that resets state:**

```c
if (dvz_gui_button(gui, "Reset"))
    state.diameter = 32.0f;
```

**Collapsing section:**

```c
if (dvz_gui_collapsing_header(gui, "Advanced", 0)) {
    dvz_gui_slider_float(gui, "Bloom radius", &s->bloom_radius, 0.5f, 12.0f);
}
```

**Color picker:**

```c
dvz_gui_color_edit4(gui, "Color", s->color_rgba, 0);
```

**Same-line layout:**

```c
dvz_gui_checkbox(gui, "Show A", &a);
dvz_gui_same_line(gui, 0.0f, -1.0f);
dvz_gui_checkbox(gui, "Show B", &b);
```

## See also

- [Update visual data](update-visual-data.md)
- [Input events](input-events.md)
- [Animation](animation.md)
