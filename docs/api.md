# API Reference

### `dvz_app()`

Create an app.

```c
DvzApp* dvz_app(  // returns: the app
    int flags,  // the app creation flags
)
```

### `dvz_app_batch()`

Return the app batch.

```c
DvzBatch* dvz_app_batch(  // returns: the batch
    DvzApp* app,  // the app
)
```

### `dvz_app_frame()`

Run one frame.

```c
void dvz_app_frame(
    DvzApp* app,  // the app
)
```

### `dvz_app_onframe()`

Register a frame callback.

```c
void dvz_app_onframe(
    DvzApp* app,  // the app
    DvzAppFrameCallback callback,  // the callback
    void* user_data,  // the user data
)
```

### `dvz_app_onmouse()`

Register a mouse callback.

```c
void dvz_app_onmouse(
    DvzApp* app,  // the app
    DvzAppMouseCallback callback,  // the callback
    void* user_data,  // the user data
)
```

### `dvz_app_onkeyboard()`

Register a keyboard callback.

```c
void dvz_app_onkeyboard(
    DvzApp* app,  // the app
    DvzAppKeyboardCallback callback,  // the callback
    void* user_data,  // the user data
)
```

### `dvz_app_onresize()`

Register a resize callback.

```c
void dvz_app_onresize(
    DvzApp* app,  // the app
    DvzAppResizeCallback callback,  // the callback
    void* user_data,  // the user data
)
```

### `dvz_app_timer()`

Create a timer.

```c
DvzTimerItem* dvz_app_timer(  // returns: the timer
    DvzApp* app,  // the app
    double delay,  // the delay, in seconds, until the first event
    double period,  // the period, in seconds, between two events
    uint64_t max_count,  // the maximum number of events
)
```

### `dvz_app_ontimer()`

Register a timer callback.

```c
void dvz_app_ontimer(
    DvzApp* app,  // the app
    DvzAppTimerCallback callback,  // the timer callback
    void* user_data,  // the user data
)
```

### `dvz_app_gui()`

Register a GUI callback.

```c
void dvz_app_gui(
    DvzApp* app,  // the app
    DvzId canvas_id,  // the canvas ID
    DvzAppGui callback,  // the GUI callback
    void* user_data,  // the user data
)
```

### `dvz_app_run()`

Start the application event loop.

```c
void dvz_app_run(
    DvzApp* app,  // the app
    uint64_t n_frames,  // the maximum number of frames, 0 for infinite loop
)
```

### `dvz_app_submit()`

Submit the current batch to the application.

```c
void dvz_app_submit(
    DvzApp* app,  // the app
)
```

### `dvz_app_screenshot()`

Make a screenshot of a canvas.

```c
void dvz_app_screenshot(
    DvzApp* app,  // the app
    DvzId canvas_id,  // the ID of the canvas
    char* filename,  // the path to the PNG file with the screenshot
)
```

### `dvz_app_destroy()`

Destroy the app.

```c
void dvz_app_destroy(
    DvzApp* app,  // the app
)
```

### `dvz_free()`

Free a pointer.

```c
void dvz_free(
    void* pointer,  // a pointer
)
```

### `dvz_scene()`

Create a scene.

```c
DvzScene* dvz_scene(  // returns: the scene
    DvzBatch* batch,  // the batch
)
```

### `dvz_scene_run()`

Start the event loop and render the scene in a window.

```c
void dvz_scene_run(
    DvzScene* scene,  // the scene
    DvzApp* app,  // the app
    uint64_t n_frames,  // the maximum number of frames, 0 for infinite loop
)
```

### `dvz_scene_destroy()`

Destroy a scene.

```c
void dvz_scene_destroy(
    DvzScene* scene,  // the scene
)
```

### `dvz_figure()`

Create a figure, a desktop window with panels and visuals.

```c
DvzFigure* dvz_figure(  // returns: the figure
    DvzScene* scene,  // the scene
    uint32_t width,  // the window width
    uint32_t height,  // the window height
    int flags,  // the figure creation flags (not yet stabilized)
)
```

### `dvz_figure_resize()`

Resize a figure.

```c
void dvz_figure_resize(
    DvzFigure* fig,  // the figure
    uint32_t width,  // the window width
    uint32_t height,  // the window height
)
```

### `dvz_scene_figure()`

Get a figure from its id.

```c
DvzFigure* dvz_scene_figure(  // returns: the figure
    DvzScene* scene,  // the scene
    DvzId id,  // the figure id
)
```

### `dvz_figure_destroy()`

Destroy a figure.

```c
void dvz_figure_destroy(
    DvzFigure* figure,  // the figure
)
```

### `dvz_panel()`

Create a panel in a figure (partial or complete rectangular portion of a figure).

```c
DvzPanel* dvz_panel(
    DvzFigure* fig,  // the figure
    float x,  // the x coordinate of the top-left corner, in pixels
    float y,  // the y coordinate of the top-left corner, in pixels
    float width,  // the panel width, in pixels
    float height,  // the panel height, in pixels
)
```

### `dvz_panel_default()`

Return the default full panel spanning an entire figure.

```c
DvzPanel* dvz_panel_default(  // returns: the panel spanning the entire figure
    DvzFigure* fig,  // the figure
)
```

### `dvz_panel_transform()`

Assign a transform to a panel.

```c
void dvz_panel_transform(
    DvzPanel* panel,  // the panel
    DvzTransform* tr,  // the transform
)
```

### `dvz_panel_resize()`

Resize a panel.

```c
void dvz_panel_resize(
    DvzPanel* panel,  // the panel
    float x,  // the x coordinate of the top-left corner, in pixels
    float y,  // the y coordinate of the top-left corner, in pixels
    float width,  // the panel width, in pixels
    float height,  // the panel height, in pixels
)
```

### `dvz_panel_margins()`

Set the margins of a panel.

```c
void dvz_panel_margins(
    DvzPanel* panel,  // the panel
    float top,  // the top margin, in pixels
    float right,  // the right margin, in pixels
    float bottom,  // the bottom margin, in pixels
    float left,  // the left margin, in pixels
)
```

### `dvz_panel_contains()`

Return whether a point is inside a panel.

```c
bool dvz_panel_contains(  // returns: true if the position lies within the panel
    DvzPanel* panel,  // the panel
    vec2 pos,  // the position
)
```

### `dvz_panel_at()`

Return the panel containing a given point.

```c
DvzPanel* dvz_panel_at(  // returns: the panel containing the point, or NULL if there is none
    DvzFigure* figure,  // the figure
    vec2 pos,  // the position
)
```

### `dvz_panel_camera()`

Set a camera for a panel.

```c
DvzCamera* dvz_panel_camera(  // returns: the camera
    DvzPanel* panel,  // the panel
)
```

### `dvz_panel_panzoom()`

Set panzoom interactivity for a panel.

```c
DvzPanzoom* dvz_panel_panzoom(  // returns: the panzoom
    DvzPanel* panel,  // the panel
)
```

### `dvz_panel_arcball()`

Set arcball interactivity for a panel.

```c
DvzArcball* dvz_panel_arcball(  // returns: the arcball
    DvzPanel* panel,  // the panel
)
```

### `dvz_panel_update()`

Trigger a panel update.

```c
void dvz_panel_update(
    DvzPanel* panel,  // the panel
)
```

### `dvz_panel_visual()`

Add a visual to a panel.

```c
void dvz_panel_visual(
    DvzPanel* panel,  // the panel
    DvzVisual* visual,  // the visual
)
```

### `dvz_panel_destroy()`

Destroy a panel.

```c
void dvz_panel_destroy(
    DvzPanel* panel,  // the panel
)
```

### `dvz_visual_fixed()`

Fix some axes in a visual.

```c
void dvz_visual_fixed(
    DvzVisual* visual,  // the visual
    bool fixed_x,  // whether the x axis should be fixed
    bool fixed_y,  // whether the y axis should be fixed
    bool fixed_z,  // whether the z axis should be fixed
)
```

### `dvz_visual_clip()`

Set the visual clipping.

```c
void dvz_visual_clip(
    DvzVisual* visual,  // the visual
    DvzViewportClip clip,  // the viewport clipping
)
```

### `dvz_visual_show()`

Set the visibility of a visual.

```c
void dvz_visual_show(
    DvzVisual* visual,  // the visual
    bool is_visible,  // the visual visibility
)
```

### `dvz_colormap()`

Fetch a color from a colormap and a value.

```c
void dvz_colormap(
    DvzColormap cmap,  // the colormap
    uint8_t value,  // the value
)
```

### `dvz_colormap_scale()`

Fetch a color from a colormap and an interpolated value.

```c
void dvz_colormap_scale(
    DvzColormap cmap,  // the colormap
    double value,  // the value
    double vmin,  // the minimum value
    double vmax,  // the maximum value
)
```

### `dvz_shape_print()`

Show information about a shape.

```c
void dvz_shape_print(
    DvzShape* shape,  // the shape
)
```

### `dvz_shape_destroy()`

Destroy a shape.

```c
void dvz_shape_destroy(
    DvzShape* shape,  // the shape
)
```

### `dvz_shape_square()`

Create a square shape.

```c
DvzShape dvz_shape_square(  // returns: the shape
    cvec4 color,  // the square color
)
```

### `dvz_shape_disc()`

Create a disc shape.

```c
DvzShape dvz_shape_disc(  // returns: the shape
    uint32_t count,  // the number of points along the disc border
    cvec4 color,  // the disc color
)
```

### `dvz_shape_cube()`

Create a cube shape.

```c
DvzShape dvz_shape_cube(  // returns: the shape
    cvec4* colors,  // the colors of the six faces
)
```

### `dvz_shape_sphere()`

Create a sphere shape.

```c
DvzShape dvz_shape_sphere(  // returns: the shape
    uint32_t rows,  // the number of rows
    uint32_t cols,  // the number of columns
    cvec4 color,  // the sphere color
)
```

### `dvz_shape_cone()`

Create a cone shape.

```c
DvzShape dvz_shape_cone(  // returns: the shape
    uint32_t count,  // the number of points along the disc border
    cvec4 color,  // the cone color
)
```

### `dvz_shape_cylinder()`

Create a cylinder shape.

```c
DvzShape dvz_shape_cylinder(  // returns: the shape
    uint32_t count,  // the number of points along the cylinder border
    cvec4 color,  // the cylinder color
)
```

### `dvz_shape_normalize()`

Normalize a shape.

```c
void dvz_shape_normalize(
    DvzShape* shape,  // the shape
)
```

### `dvz_shape_obj()`

Load a .obj shape.

```c
DvzShape dvz_shape_obj(  // returns: the shape
    char* file_path,  // the path to the .obj file
)
```

### `dvz_basic()`

Create a basic visual using the few GPU visual primitives (point, line, triangles).

```c
DvzVisual* dvz_basic(  // returns: the visual
    DvzBatch* batch,  // the batch
    DvzPrimitiveTopology topology,  // the primitive topology
    int flags,  // the visual creation flags
)
```

### `dvz_basic_position()`

Set the vertex positions.

```c
void dvz_basic_position(
    DvzVisual* visual,  // the visual
    uint32_t first,  // the index of the first item to update
    uint32_t count,  // the number of items to update
    vec3* values,  // the 3D positions of the items to update
    int flags,  // the data update flags
)
```

### `dvz_basic_color()`

Set the vertex colors.

```c
void dvz_basic_color(
    DvzVisual* visual,  // the visual
    uint32_t first,  // the index of the first item to update
    uint32_t count,  // the number of items to update
    cvec4* values,  // the colors of the items to update
    int flags,  // the data update flags
)
```

### `dvz_basic_alloc()`

Allocate memory for a visual.

```c
void dvz_basic_alloc(
    DvzVisual* visual,  // the visual
    uint32_t item_count,  // the total number of items to allocate for this visual
)
```

### `dvz_pixel()`

Create a pixel visual.

```c
DvzVisual* dvz_pixel(  // returns: the visual
    DvzBatch* batch,  // the batch
    int flags,  // the visual creation flags
)
```

### `dvz_pixel_position()`

Set the pixel positions.

```c
void dvz_pixel_position(
    DvzVisual* visual,  // the visual
    uint32_t first,  // the index of the first item to update
    uint32_t count,  // the number of items to update
    vec3* values,  // the 3D positions of the items to update
    int flags,  // the data update flags
)
```

### `dvz_pixel_color()`

Set the pixel colors.

```c
void dvz_pixel_color(
    DvzVisual* visual,  // the visual
    uint32_t first,  // the index of the first item to update
    uint32_t count,  // the number of items to update
    cvec4* values,  // the colors of the items to update
    int flags,  // the data update flags
)
```

### `dvz_pixel_alloc()`

Allocate memory for a visual.

```c
void dvz_pixel_alloc(
    DvzVisual* visual,  // the visual
    uint32_t item_count,  // the total number of items to allocate for this visual
)
```

### `dvz_point()`

Create a point visual.

```c
DvzVisual* dvz_point(  // returns: the visual
    DvzBatch* batch,  // the batch
    int flags,  // the visual creation flags
)
```

### `dvz_point_position()`

Set the point positions.

```c
void dvz_point_position(
    DvzVisual* visual,  // the visual
    uint32_t first,  // the index of the first item to update
    uint32_t count,  // the number of items to update
    vec3* values,  // the 3D positions of the items to update
    int flags,  // the data update flags
)
```

### `dvz_point_color()`

Set the point colors.

```c
void dvz_point_color(
    DvzVisual* visual,  // the visual
    uint32_t first,  // the index of the first item to update
    uint32_t count,  // the number of items to update
    cvec4* values,  // the colors of the items to update
    int flags,  // the data update flags
)
```

### `dvz_point_size()`

Set the point sizes.

```c
void dvz_point_size(
    DvzVisual* visual,  // the visual
    uint32_t first,  // the index of the first item to update
    uint32_t count,  // the number of items to update
    float* values,  // the sizes of the items to update
    int flags,  // the data update flags
)
```

### `dvz_point_alloc()`

Allocate memory for a visual.

```c
void dvz_point_alloc(
    DvzVisual* visual,  // the visual
    uint32_t item_count,  // the total number of items to allocate for this visual
)
```

### `dvz_marker()`

Create a marker visual.

```c
DvzVisual* dvz_marker(  // returns: the visual
    DvzBatch* batch,  // the batch
    int flags,  // the visual creation flags
)
```

### `dvz_marker_mode()`

Set the marker mode.

```c
void dvz_marker_mode(
    DvzVisual* visual,  // the visual
    DvzMarkerMode mode,  // the marker mode, one of DVZ_MARKER_MODE_CODE, DVZ_MARKER_MODE_BITMAP,
)
```

### `dvz_marker_aspect()`

Set the marker aspect.

```c
void dvz_marker_aspect(
    DvzVisual* visual,  // the visual
    DvzMarkerAspect aspect,  // the marker aspect, one of DVZ_MARKER_ASPECT_FILLED, DVZ_MARKER_ASPECT_STROKE,
)
```

### `dvz_marker_shape()`

Set the marker shape.

```c
void dvz_marker_shape(
    DvzVisual* visual,  // the visual
    DvzMarkerShape shape,  // the marker shape
)
```

### `dvz_marker_position()`

Set the marker positions.

```c
void dvz_marker_position(
    DvzVisual* visual,  // the visual
    uint32_t first,  // the index of the first item to update
    uint32_t count,  // the number of items to update
    vec3* values,  // the 3D positions of the items to update
    int flags,  // the data update flags
)
```

### `dvz_marker_size()`

Set the marker sizes.

```c
void dvz_marker_size(
    DvzVisual* visual,  // the visual
    uint32_t first,  // the index of the first item to update
    uint32_t count,  // the number of items to update
    float* values,  // the colors of the items to update
    int flags,  // the data update flags
)
```

### `dvz_marker_angle()`

Set the marker angles.

```c
void dvz_marker_angle(
    DvzVisual* visual,  // the visual
    uint32_t first,  // the index of the first item to update
    uint32_t count,  // the number of items to update
    float* values,  // the angles of the items to update
    int flags,  // the data update flags
)
```

### `dvz_marker_color()`

Set the marker colors.

```c
void dvz_marker_color(
    DvzVisual* visual,  // the visual
    uint32_t first,  // the index of the first item to update
    uint32_t count,  // the number of items to update
    cvec4* values,  // the colors of the items to update
    int flags,  // the data update flags
)
```

### `dvz_marker_edge_color()`

Set the marker edge color.

```c
void dvz_marker_edge_color(
    DvzVisual* visual,  // the visual
    cvec4 value,  // the edge color
)
```

### `dvz_marker_edge_width()`

Set the marker edge width.

```c
void dvz_marker_edge_width(
    DvzVisual* visual,  // the visual
    float value,  // the edge width
)
```

### `dvz_marker_tex()`

Set the marker texture.

```c
void dvz_marker_tex(
    DvzVisual* visual,  // the visual
    DvzId tex,  // the texture ID
    DvzId sampler,  // the sampler ID
)
```

### `dvz_marker_tex_scale()`

Set the texture scale.

```c
void dvz_marker_tex_scale(
    DvzVisual* visual,  // the visual
    float value,  // the texture scale
)
```

### `dvz_marker_alloc()`

Allocate memory for a visual.

```c
void dvz_marker_alloc(
    DvzVisual* visual,  // the visual
    uint32_t item_count,  // the total number of items to allocate for this visual
)
```

### `dvz_segment()`

Create a segment visual.

```c
DvzVisual* dvz_segment(  // returns: the visual
    DvzBatch* batch,  // the batch
    int flags,  // the visual creation flags
)
```

### `dvz_segment_position()`

Set the segment positions.

```c
void dvz_segment_position(
    DvzVisual* visual,  // the visual
    uint32_t first,  // the index of the first item to update
    uint32_t count,  // the number of items to update
    vec3* initial,  // the initial 3D positions of the segments
    vec3* terminal,  // the terminal 3D positions of the segments
    int flags,  // the data update flags
)
```

### `dvz_segment_shift()`

Set the segment shift.

```c
void dvz_segment_shift(
    DvzVisual* visual,  // the visual
    uint32_t first,  // the index of the first item to update
    uint32_t count,  // the number of items to update
    vec4* values,  // the dx0,dy0,dx1,dy1 shift quadriplets of the segments to update
    int flags,  // the data update flags
)
```

### `dvz_segment_color()`

Set the segment colors.

```c
void dvz_segment_color(
    DvzVisual* visual,  // the visual
    uint32_t first,  // the index of the first item to update
    uint32_t count,  // the number of items to update
    cvec4* values,  // the colors of the items to update
    int flags,  // the data update flags
)
```

### `dvz_segment_linewidth()`

Set the segment line widths.

```c
void dvz_segment_linewidth(
    DvzVisual* visual,  // the visual
    uint32_t first,  // the index of the first item to update
    uint32_t count,  // the number of items to update
    float* values,  // the segment line widths
    int flags,  // the data update flags
)
```

### `dvz_segment_cap()`

Set the segment cap types.

```c
void dvz_segment_cap(
    DvzVisual* visual,  // the visual
    uint32_t first,  // the index of the first item to update
    uint32_t count,  // the number of items to update
    DvzCapType* initial,  // the initial segment cap types
    DvzCapType* terminal,  // the terminal segment cap types
    int flags,  // the data update flags
)
```

### `dvz_segment_alloc()`

Allocate memory for a visual.

```c
void dvz_segment_alloc(
    DvzVisual* visual,  // the visual
    uint32_t item_count,  // the total number of items to allocate for this visual
)
```

### `dvz_path()`

Create a path visual.

```c
DvzVisual* dvz_path(  // returns: the visual
    DvzBatch* batch,  // the batch
    int flags,  // the visual creation flags
)
```

### `dvz_path_position()`

Set the path positions. Note: all path point positions must be updated at once for now.

```c
void dvz_path_position(
    DvzVisual* visual,  // the visual
    uint32_t vertex_count,  // the total number of points across all paths
    vec3* positions,  // the path point positions
    uint32_t path_count,  // the number of different paths
    uint32_t* path_lengths,  // the number of points in each path
    int flags,  // the data update flags
)
```

### `dvz_path_color()`

Set the path colors.

```c
void dvz_path_color(
    DvzVisual* visual,  // the visual
    uint32_t first,  // the index of the first item to update
    uint32_t count,  // the number of items to update
    cvec4* values,  // the colors of the items to update
    int flags,  // the data update flags
)
```

### `dvz_path_linewidth()`

Set the path line width.

```c
void dvz_path_linewidth(
    DvzVisual* visual,  // the visual
    float value,  // the line width
)
```

### `dvz_path_alloc()`

Allocate memory for a visual.

```c
void dvz_path_alloc(
    DvzVisual* visual,  // the visual
    uint32_t total_point_count,  // the total number of points to allocate for this visual
)
```

### `dvz_glyph()`

Create a glyph visual.

```c
DvzVisual* dvz_glyph(  // returns: the visual
    DvzBatch* batch,  // the batch
    int flags,  // the visual creation flags
)
```

### `dvz_glyph_alloc()`

Allocate memory for a visual.

```c
void dvz_glyph_alloc(
    DvzVisual* visual,  // the visual
    uint32_t item_count,  // the total number of items to allocate for this visual
)
```

### `dvz_glyph_position()`

Set the glyph positions.

```c
void dvz_glyph_position(
    DvzVisual* visual,  // the visual
    uint32_t first,  // the index of the first item to update
    uint32_t count,  // the number of items to update
    vec3* values,  // the 3D positions of the items to update
    int flags,  // the data update flags
)
```

### `dvz_glyph_axis()`

Set the glyph axes.

```c
void dvz_glyph_axis(
    DvzVisual* visual,  // the visual
    uint32_t first,  // the index of the first item to update
    uint32_t count,  // the number of items to update
    vec3* values,  // the 3D axis vectors of the items to update
    int flags,  // the data update flags
)
```

### `dvz_glyph_size()`

Set the glyph sizes.

```c
void dvz_glyph_size(
    DvzVisual* visual,  // the visual
    uint32_t first,  // the index of the first item to update
    uint32_t count,  // the number of items to update
    vec2* values,  // the sizes (width and height) of the items to update
    int flags,  // the data update flags
)
```

### `dvz_glyph_anchor()`

Set the glyph anchors.

```c
void dvz_glyph_anchor(
    DvzVisual* visual,  // the visual
    uint32_t first,  // the index of the first item to update
    uint32_t count,  // the number of items to update
    vec2* values,  // the anchors (x and y) of the items to update
    int flags,  // the data update flags
)
```

### `dvz_glyph_shift()`

Set the glyph shifts.

```c
void dvz_glyph_shift(
    DvzVisual* visual,  // the visual
    uint32_t first,  // the index of the first item to update
    uint32_t count,  // the number of items to update
    vec2* values,  // the shifts (x and y) of the items to update
    int flags,  // the data update flags
)
```

### `dvz_glyph_texcoords()`

Set the glyph texture coordinates.

```c
void dvz_glyph_texcoords(
    DvzVisual* visual,  // the visual
    uint32_t first,  // the index of the first item to update
    uint32_t count,  // the number of items to update
    vec4* coords,  // the x,y,w,h texture coordinates
    int flags,  // the data update flags
)
```

### `dvz_glyph_angle()`

Set the glyph angles.

```c
void dvz_glyph_angle(
    DvzVisual* visual,  // the visual
    uint32_t first,  // the index of the first item to update
    uint32_t count,  // the number of items to update
    float* values,  // the angles of the items to update
    int flags,  // the data update flags
)
```

### `dvz_glyph_color()`

Set the glyph colors.

```c
void dvz_glyph_color(
    DvzVisual* visual,  // the visual
    uint32_t first,  // the index of the first item to update
    uint32_t count,  // the number of items to update
    cvec4* values,  // the colors of the items to update
    int flags,  // the data update flags
)
```

### `dvz_glyph_groupsize()`

Set the glyph group size.

```c
void dvz_glyph_groupsize(
    DvzVisual* visual,  // the visual
    uint32_t first,  // the index of the first item to update
    uint32_t count,  // the number of items to update
    float* values,  // the glyph group sizes
    int flags,  // the data update flags
)
```

### `dvz_glyph_bgcolor()`

Set the glyph background color.

```c
void dvz_glyph_bgcolor(
    DvzVisual* visual,  // the visual
    vec4 bgcolor,  // the background color
)
```

### `dvz_glyph_texture()`

Assign a texture to a glyph visual.

```c
void dvz_glyph_texture(
    DvzVisual* visual,  // the visual
    DvzId tex,  // the texture ID
)
```

### `dvz_glyph_atlas()`

Associate an atlas with a glyph visual.

```c
void dvz_glyph_atlas(
    DvzVisual* visual,  // the visual
    DvzAtlas* atlas,  // the atlas
)
```

### `dvz_glyph_unicode()`

Set the glyph unicode code points.

```c
void dvz_glyph_unicode(
    DvzVisual* visual,  // the visual
    uint32_t count,  // the number of glyphs
    uint32_t* codepoints,  // the unicode codepoints
)
```

### `dvz_glyph_ascii()`

Set the glyph ascii characters.

```c
void dvz_glyph_ascii(
    DvzVisual* visual,  // the visual
    char* string,  // the characters
)
```

### `dvz_glyph_xywh()`

Set the xywh parameters of each glyph.

```c
void dvz_glyph_xywh(
    DvzVisual* visual,  // the visual
    uint32_t first,  // the index of the first item to update
    uint32_t count,  // the number of items to update
    vec4* values,  // the xywh values of each glyph
    vec2 offset,  // the xy offsets of each glyph
    int flags,  // the data update flags
)
```

### `dvz_image()`

Create an image visual.

```c
DvzVisual* dvz_image(  // returns: the visual
    DvzBatch* batch,  // the batch
    int flags,  // the visual creation flags
)
```

### `dvz_image_position()`

Set the image positions.

```c
void dvz_image_position(
    DvzVisual* visual,  // the visual
    uint32_t first,  // the index of the first item to update
    uint32_t count,  // the number of items to update
    vec4* ul_lr,  // the 2D positions of the upper-left and lower-right corners (vec4 x0,y0,x1,y1)
    int flags,  // the data update flags
)
```

### `dvz_image_texcoords()`

Set the image texture coordinates.

```c
void dvz_image_texcoords(
    DvzVisual* visual,  // the visual
    uint32_t first,  // the index of the first item to update
    uint32_t count,  // the number of items to update
    vec4* ul_lr,  // the tex coordinates of the upper-left and lower-right corners (vec4 u0,v0,u1,v1)
    int flags,  // the data update flags
)
```

### `dvz_image_texture()`

Assign a texture to an image visual.

```c
void dvz_image_texture(
    DvzVisual* visual,  // the visual
    DvzId tex,  // the texture ID
    DvzFilter filter,  // the texture filtering mode
    DvzSamplerAddressMode address_mode,  // the texture address mode
)
```

### `dvz_image_alloc()`

Allocate memory for a visual.

```c
void dvz_image_alloc(
    DvzVisual* visual,  // the visual
    uint32_t item_count,  // the total number of images to allocate for this visual
)
```

### `dvz_tex_image()`

Create a 2D texture to be used in an image visual.

```c
DvzId dvz_tex_image(  // returns: the texture ID
    DvzBatch* batch,  // the batch
    DvzFormat format,  // the texture format
    uint32_t width,  // the texture width
    uint32_t height,  // the texture height
    void* data,  // the texture data to upload
)
```

### `dvz_mesh()`

Create a mesh visual.

```c
DvzVisual* dvz_mesh(  // returns: the visual
    DvzBatch* batch,  // the batch
    int flags,  // the visual creation flags
)
```

### `dvz_mesh_position()`

Set the mesh vertex positions.

```c
void dvz_mesh_position(
    DvzVisual* visual,  // the visual
    uint32_t first,  // the index of the first item to update
    uint32_t count,  // the number of items to update
    vec3* values,  // the 3D vertex positions
    int flags,  // the data update flags
)
```

### `dvz_mesh_color()`

Set the mesh colors.

```c
void dvz_mesh_color(
    DvzVisual* visual,  // the visual
    uint32_t first,  // the index of the first item to update
    uint32_t count,  // the number of items to update
    cvec4* values,  // the vertex colors
    int flags,  // the data update flags
)
```

### `dvz_mesh_texcoords()`

Set the mesh texture coordinates.

```c
void dvz_mesh_texcoords(
    DvzVisual* visual,  // the visual
    uint32_t first,  // the index of the first item to update
    uint32_t count,  // the number of items to update
    vec4* values,  // the vertex texture coordinates (vec4 u,v,*,alpha)
    int flags,  // the data update flags
)
```

### `dvz_mesh_normal()`

Set the mesh normals.

```c
void dvz_mesh_normal(
    DvzVisual* visual,  // the visual
    uint32_t first,  // the index of the first item to update
    uint32_t count,  // the number of items to update
    vec3* values,  // the vertex normal vectors
    int flags,  // the data update flags
)
```

### `dvz_mesh_texture()`

Assign a 2D texture to a mesh visual.

```c
void dvz_mesh_texture(
    DvzVisual* visual,  // the visual
    DvzId tex,  // the texture ID
    DvzFilter filter,  // the texture filtering mode
    DvzSamplerAddressMode address_mode,  // the texture address mode
)
```

### `dvz_mesh_index()`

Set the mesh indices.

```c
void dvz_mesh_index(
    DvzVisual* visual,  // the visual
    uint32_t first,  // the index of the first item to update
    uint32_t count,  // the number of items to update
    DvzIndex* values,  // the face indices (three vertex indices per triangle)
    int flags,  // the data update flags
)
```

### `dvz_mesh_alloc()`

Allocate memory for a visual.

```c
void dvz_mesh_alloc(
    DvzVisual* visual,  // the visual
    uint32_t vertex_count,  // the number of vertices
    uint32_t index_count,  // the number of indices
)
```

### `dvz_mesh_light_pos()`

Set the mesh light position.

```c
void dvz_mesh_light_pos(
    DvzVisual* visual,  // the mesh
    vec4 pos,  // the light position
)
```

### `dvz_mesh_light_params()`

Set the mesh light parameters.

```c
void dvz_mesh_light_params(
    DvzVisual* visual,  // the mesh
    vec4 params,  // the light parameters (vec4 ambient, diffuse, specular, exponent)
)
```

### `dvz_mesh_shape()`

Create a mesh out of a shape.

```c
DvzVisual* dvz_mesh_shape(  // returns: the mesh
    DvzBatch* batch,  // the batch
    DvzShape* shape,  // the shape
    int flags,  // the visual creation flags
)
```

### `dvz_fake_sphere()`

Create a fake sphere visual.

```c
DvzVisual* dvz_fake_sphere(  // returns: the visual
    DvzBatch* batch,  // the batch
    int flags,  // the visual creation flags
)
```

### `dvz_fake_sphere_position()`

Set the fake sphere positions.

```c
void dvz_fake_sphere_position(
    DvzVisual* visual,  // the visual
    uint32_t first,  // the index of the first item to update
    uint32_t count,  // the number of items to update
    vec3* pos,  // the 3D positions of the sphere centers
    int flags,  // the data update flags
)
```

### `dvz_fake_sphere_color()`

Set the fake sphere colors.

```c
void dvz_fake_sphere_color(
    DvzVisual* visual,  // the visual
    uint32_t first,  // the index of the first item to update
    uint32_t count,  // the number of items to update
    cvec4* color,  // the sphere colors
    int flags,  // the data update flags
)
```

### `dvz_fake_sphere_size()`

Set the fake sphere sizes.

```c
void dvz_fake_sphere_size(
    DvzVisual* visual,  // the visual
    uint32_t first,  // the index of the first item to update
    uint32_t count,  // the number of items to update
    float* size,  // the radius of the spheres
    int flags,  // the data update flags
)
```

### `dvz_fake_sphere_alloc()`

Allocate memory for a visual.

```c
void dvz_fake_sphere_alloc(
    DvzVisual* visual,  // the visual
    uint32_t item_count,  // the total number of spheres to allocate for this visual
)
```

### `dvz_fake_sphere_light_pos()`

Set the sphere light position.

```c
void dvz_fake_sphere_light_pos(
    DvzVisual* visual,  // the visual
    vec3 pos,  // the light position
)
```

### `dvz_volume()`

Create a volume visual.

```c
DvzVisual* dvz_volume(  // returns: the visual
    DvzBatch* batch,  // the batch
    int flags,  // the visual creation flags
)
```

### `dvz_volume_alloc()`

Allocate memory for a visual.

```c
void dvz_volume_alloc(
    DvzVisual* visual,  // the visual
    uint32_t item_count,  // the total number of volumes to allocate for this visual
)
```

### `dvz_volume_texture()`

Assign a 3D texture to a volume visual.

```c
void dvz_volume_texture(
    DvzVisual* visual,  // the visual
    DvzId tex,  // the texture ID
    DvzFilter filter,  // the texture filtering mode
    DvzSamplerAddressMode address_mode,  // the texture address mode
)
```

### `dvz_volume_size()`

Set the volume size.

```c
void dvz_volume_size(
    DvzVisual* visual,  // the visual
    float w,  // the texture width
    float h,  // the texture height
    float d,  // the texture depth
)
```

### `dvz_tex_volume()`

Create a 3D texture to be used in a volume visual.

```c
DvzId dvz_tex_volume(  // returns: the texture ID
    DvzBatch* batch,  // the batch
    DvzFormat format,  // the texture format
    uint32_t width,  // the texture width
    uint32_t height,  // the texture height
    uint32_t depth,  // the texture depth
    void* data,  // the texture data to upload
)
```

### `dvz_slice()`

Create a slice visual (multiple 2D images with slices of a 3D texture).

```c
DvzVisual* dvz_slice(  // returns: the visual
    DvzBatch* batch,  // the batch
    int flags,  // the visual creation flags
)
```

### `dvz_slice_position()`

Set the slice positions.

```c
void dvz_slice_position(
    DvzVisual* visual,  // the visual
    uint32_t first,  // the index of the first item to update
    uint32_t count,  // the number of items to update
    vec3* p0,  // the 3D positions of the upper-left corner
    vec3* p1,  // the 3D positions of the upper-right corner
    vec3* p2,  // the 3D positions of the lower-left corner
    vec3* p3,  // the 3D positions of the lower-right corner
    int flags,  // the data update flags
)
```

### `dvz_slice_texcoords()`

Set the slice texture coordinates.

```c
void dvz_slice_texcoords(
    DvzVisual* visual,  // the visual
    uint32_t first,  // the index of the first item to update
    uint32_t count,  // the number of items to update
    vec3* uvw0,  // the 3D texture coordinates of the upper-left corner
    vec3* uvw1,  // the 3D texture coordinates of the upper-right corner
    vec3* uvw2,  // the 3D texture coordinates of the lower-left corner
    vec3* uvw3,  // the 3D texture coordinates of the lower-right corner
    int flags,  // the data update flags
)
```

### `dvz_slice_texture()`

Assign a texture to a slice visual.

```c
void dvz_slice_texture(
    DvzVisual* visual,  // the visual
    DvzId tex,  // the texture ID
    DvzFilter filter,  // the texture filtering mode
    DvzSamplerAddressMode address_mode,  // the texture address mode
)
```

### `dvz_slice_alloc()`

Allocate memory for a visual.

```c
void dvz_slice_alloc(
    DvzVisual* visual,  // the visual
    uint32_t item_count,  // the total number of slices to allocate for this visual
)
```

### `dvz_tex_slice()`

Create a 3D texture to be used in a slice visual.

```c
DvzId dvz_tex_slice(  // returns: the texture ID
    DvzBatch* batch,  // the batch
    DvzFormat format,  // the texture format
    uint32_t width,  // the texture width
    uint32_t height,  // the texture height
    uint32_t depth,  // the texture depth
    void* data,  // the texture data to upload
)
```

### `dvz_slice_alpha()`

Set the slice transparency alpha value.

```c
void dvz_slice_alpha(
    DvzVisual* visual,  // the visual
    float alpha,  // the alpha value
)
```

### `dvz_resample()`

Normalize a value in an interval.

```c
double dvz_resample(  // returns: the normalized value between 0 and 1
    double t0,  // the interval start
    double t1,  // the interval end
    double t,  // the value within the interval
)
```

### `dvz_easing()`

Apply an easing function to a normalized value.

```c
double dvz_easing(  // returns: the eased value
    DvzEasing easing,  // the easing mode
    double t,  // the normalized value
)
```

### `dvz_circular_2D()`

Generate a 2D circular motion.

```c
void dvz_circular_2D(
    vec2 center,  // the circle center
    float radius,  // the circle radius
    float angle,  // the initial angle
    float t,  // the normalized value
)
```

### `dvz_circular_3D()`

Generate a 3D circular motion.

```c
void dvz_circular_3D(
    vec3 center,  // the circle center
    vec3 u,  // the first 3D vector defining the plane containing the circle
    vec3 v,  // the second 3D vector defining the plane containing the circle
    float radius,  // the circle radius
    float angle,  // the initial angle
    float t,  // the normalized value
)
```

### `dvz_interpolate()`

Make a linear interpolation between two scalar value.

```c
float dvz_interpolate(  // returns: the interpolated value
    float p0,  // the first value
    float p1,  // the second value
    float t,  // the normalized value
)
```

### `dvz_interpolate_2D()`

Make a linear interpolation between two 2D points.

```c
void dvz_interpolate_2D(  // returns: the interpolated point
    vec2 p0,  // the first point
    vec2 p1,  // the second point
    float t,  // the normalized value
)
```

### `dvz_interpolate_3D()`

Make a linear interpolation between two 3D points.

```c
void dvz_interpolate_3D(  // returns: the interpolated point
    vec3 p0,  // the first point
    vec3 p1,  // the second point
    float t,  // the normalized value
)
```

### `dvz_arcball_initial()`

Set the initial arcball angles.

```c
void dvz_arcball_initial(
    DvzArcball* arcball,  // the arcball
    vec3 angles,  // the initial angles
)
```

### `dvz_arcball_reset()`

Reset an arcball to its initial position.

```c
void dvz_arcball_reset(
    DvzArcball* arcball,  // the arcball
)
```

### `dvz_arcball_resize()`

Inform an arcball of a panel resize.

```c
void dvz_arcball_resize(
    DvzArcball* arcball,  // the arcball
    float width,  // the panel width
    float height,  // the panel height
)
```

### `dvz_arcball_flags()`

Set the arcball flags.

```c
void dvz_arcball_flags(
    DvzArcball* arcball,  // the arcball
    int flags,  // the flags
)
```

### `dvz_arcball_constrain()`

Add arcball constraints.

```c
void dvz_arcball_constrain(
    DvzArcball* arcball,  // the arcball
    vec3 constrain,  // the constrain values
)
```

### `dvz_arcball_set()`

Set the arcball angles.

```c
void dvz_arcball_set(
    DvzArcball* arcball,  // the arcball
    vec3 angles,  // the angles
)
```

### `dvz_arcball_angles()`

Get the current arcball angles.

```c
void dvz_arcball_angles(
    DvzArcball* arcball,  // the arcball
)
```

### `dvz_arcball_rotate()`

Apply a rotation to an arcball.

```c
void dvz_arcball_rotate(
    DvzArcball* arcball,  // the arcball
    vec2 cur_pos,  // the initial position
    vec2 last_pos,  // the final position
)
```

### `dvz_arcball_model()`

Return the model matrix of an arcball.

```c
void dvz_arcball_model(
    DvzArcball* arcball,  // the arcball
)
```

### `dvz_arcball_end()`

Finalize arcball position update.

```c
void dvz_arcball_end(
    DvzArcball* arcball,  // the arcball
)
```

### `dvz_arcball_mvp()`

Apply an MVP matrix to an arcball (only the model matrix).

```c
void dvz_arcball_mvp(
    DvzArcball* arcball,  // the arcball
    DvzMVP* mvp,  // the MVP
)
```

### `dvz_arcball_print()`

Display information about an arcball.

```c
void dvz_arcball_print(
    DvzArcball* arcball,  // the arcball
)
```

### `dvz_camera_initial()`

Set the initial camera parameters.

```c
void dvz_camera_initial(
    DvzCamera* camera,  // the camera
    vec3 pos,  // the initial position
    vec3 lookat,  // the lookat position
    vec3 up,  // the up vector
)
```

### `dvz_camera_reset()`

Reset a camera.

```c
void dvz_camera_reset(
    DvzCamera* camera,  // the camera
)
```

### `dvz_camera_zrange()`

Set the camera zrange.

```c
void dvz_camera_zrange(
    DvzCamera* camera,  // the camera
    float near,  // the near value
    float far,  // the far value
)
```

### `dvz_camera_ortho()`

Make an orthographic camera.

```c
void dvz_camera_ortho(
    DvzCamera* camera,  // the camera
    float left,  // the left value
    float right,  // the right value
    float bottom,  // the bottom value
    float top,  // the top value
)
```

### `dvz_camera_resize()`

Inform a camera of a panel resize.

```c
void dvz_camera_resize(
    DvzCamera* camera,  // the camera
    float width,  // the panel width
    float height,  // the panel height
)
```

### `dvz_camera_position()`

Set a camera position.

```c
void dvz_camera_position(
    DvzCamera* camera,  // the camera
    vec3 pos,  // the pos
)
```

### `dvz_camera_lookat()`

Set a camera lookat position.

```c
void dvz_camera_lookat(
    DvzCamera* camera,  // the camera
    vec3 lookat,  // the lookat position
)
```

### `dvz_camera_up()`

Set a camera up vector.

```c
void dvz_camera_up(
    DvzCamera* camera,  // the camera
    vec3 up,  // the up vector
)
```

### `dvz_camera_perspective()`

Set a camera perspective.

```c
void dvz_camera_perspective(
    DvzCamera* camera,  // the camera
    float fov,  // the field of view angle (in radians)
)
```

### `dvz_camera_viewproj()`

Return the view and proj matrices of the camera.

```c
void dvz_camera_viewproj(
    DvzCamera* camera,  // the camera
)
```

### `dvz_camera_mvp()`

Apply an MVP to a camera.

```c
void dvz_camera_mvp(
    DvzCamera* camera,  // the camera
    DvzMVP* mvp,  // the MVP
)
```

### `dvz_camera_print()`

Display information about a camera.

```c
void dvz_camera_print(
    DvzCamera* camera,  // the camera
)
```

### `dvz_panzoom_reset()`

Reset a panzoom.

```c
void dvz_panzoom_reset(
    DvzPanzoom* pz,  // the panzoom
)
```

### `dvz_panzoom_resize()`

Inform a panzoom of a panel resize.

```c
void dvz_panzoom_resize(
    DvzPanzoom* pz,  // the panzoom
    float width,  // the panel width
    float height,  // the panel height
)
```

### `dvz_panzoom_flags()`

Set the panzoom flags.

```c
void dvz_panzoom_flags(
    DvzPanzoom* pz,  // the panzoom
    int flags,  // the flags
)
```

### `dvz_panzoom_xlim()`

Set a panzoom x limits.

```c
void dvz_panzoom_xlim(
    DvzPanzoom* pz,  // the panzoom
    vec2 xlim,  // the xlim (FLOAT_MIN/MAX=no lim)
)
```

### `dvz_panzoom_ylim()`

Set a panzoom y limits.

```c
void dvz_panzoom_ylim(
    DvzPanzoom* pz,  // the panzoom
    vec2 ylim,  // the ylim (FLOAT_MIN/MAX=no lim)
)
```

### `dvz_panzoom_zlim()`

Set a panzoom z limits.

```c
void dvz_panzoom_zlim(
    DvzPanzoom* pz,  // the panzoom
    vec2 zlim,  // the zlim (FLOAT_MIN/MAX=no lim)
)
```

### `dvz_panzoom_pan()`

Apply a pan value to a panzoom.

```c
void dvz_panzoom_pan(
    DvzPanzoom* pz,  // the panzoom
    vec2 pan,  // the pan, in NDC
)
```

### `dvz_panzoom_zoom()`

Apply a zoom value to a panzoom.

```c
void dvz_panzoom_zoom(
    DvzPanzoom* pz,  // the panzoom
    vec2 zoom,  // the zoom, in NDC
)
```

### `dvz_panzoom_pan_shift()`

Apply a pan shift to a panzoom.

```c
void dvz_panzoom_pan_shift(
    DvzPanzoom* pz,  // the panzoom
    vec2 shift_px,  // the shift value, in pixels
    vec2 center_px,  // the center position, in pixels
)
```

### `dvz_panzoom_zoom_shift()`

Apply a zoom shift to a panzoom.

```c
void dvz_panzoom_zoom_shift(
    DvzPanzoom* pz,  // the panzoom
    vec2 shift_px,  // the shift value, in pixels
    vec2 center_px,  // the center position, in pixels
)
```

### `dvz_panzoom_end()`

End a panzoom interaction.

```c
void dvz_panzoom_end(
    DvzPanzoom* pz,  // the panzoom
)
```

### `dvz_panzoom_zoom_wheel()`

Apply a wheel zoom to a panzoom.

```c
void dvz_panzoom_zoom_wheel(
    DvzPanzoom* pz,  // the panzoom
    vec2 dir,  // the wheel direction
    vec2 center_px,  // the center position, in pixels
)
```

### `dvz_panzoom_xrange()`

Get or set the xrange.

```c
void dvz_panzoom_xrange(
    DvzPanzoom* pz,  // the panzoom
    vec2 xrange,  // the xrange (get if (0,0), set otherwise)
)
```

### `dvz_panzoom_yrange()`

Get or set the yrange.

```c
void dvz_panzoom_yrange(
    DvzPanzoom* pz,  // the panzoom
    vec2 yrange,  // the yrange (get if (0,0), set otherwise)
)
```

### `dvz_panzoom_mvp()`

Apply an MVP matrix to a panzoom.

```c
void dvz_panzoom_mvp(
    DvzPanzoom* pz,  // the panzoom
    DvzMVP* mvp,  // the MVP
)
```

### `dvz_demo()`

Demo.

```c
void dvz_demo(

)
```

### `dvz_next_pow2()`

Return the smallest power of 2 larger or equal than a positive integer.

```c
uint64_t dvz_next_pow2(  // returns: the power of 2
    uint64_t x,  // the value
)
```

### `dvz_mean()`

Compute the mean of an array of double values.

```c
double dvz_mean(  // returns: the mean
    uint32_t n,  // the number of values
    double* values,  // an array of double numbers
)
```

### `dvz_min_max()`

Compute the min and max of an array of float values.

```c
void dvz_min_max(  // returns: the mean
    uint32_t n,  // the number of values
    float* values,  // an array of float numbers
    vec2 out_min_max,  // the min and max
)
```

### `dvz_normalize_bytes()`

Normalize the array.

```c
uint8_t* dvz_normalize_bytes(  // returns: the normalized array
    uint32_t count,  // the number of values
    float* values,  // an array of float numbers
)
```

### `dvz_range()`

Compute the range of an array of double values.

```c
void dvz_range(
    uint32_t n,  // the number of values
    double* values,  // an array of double numbers
)
```

### `dvz_rand_byte()`

Return a random integer number between 0 and 255.

```c
uint8_t dvz_rand_byte(  // returns: random number

)
```

### `dvz_rand_int()`

Return a random integer number.

```c
int dvz_rand_int(  // returns: random number

)
```

### `dvz_rand_float()`

Return a random floating-point number between 0 and 1.

```c
float dvz_rand_float(  // returns: random number

)
```

### `dvz_rand_double()`

Return a random floating-point number between 0 and 1.

```c
double dvz_rand_double(  // returns: random number

)
```

### `dvz_rand_normal()`

Return a random normal floating-point number.

```c
double dvz_rand_normal(  // returns: random number

)
```

### `dvz_mock_pos2D()`

Generate a set of random 2D positions.

```c
vec3* dvz_mock_pos2D(  // returns: the positions
    uint32_t count,  // the number of positions to generate
    float std,  // the standard deviation
)
```

### `dvz_mock_pos3D()`

Generate a set of random 3D positions.

```c
vec3* dvz_mock_pos3D(  // returns: the positions
    uint32_t count,  // the number of positions to generate
    float std,  // the standard deviation
)
```

### `dvz_mock_uniform()`

Generate a set of uniformly random scalar values.

```c
float* dvz_mock_uniform(  // returns: the values
    uint32_t count,  // the number of values to generate
    float vmin,  // the minimum value of the interval
    float vmax,  // the maximum value of the interval
)
```

### `dvz_mock_color()`

Generate a set of random colors.

```c
cvec4* dvz_mock_color(  // returns: random colors
    uint32_t count,  // the number of colors to generate
    uint8_t alpha,  // the alpha value
)
```

