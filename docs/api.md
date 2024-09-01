# C API Reference

## Functions

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

### `dvz_app_destroy()`

Destroy the app.

```c
void dvz_app_destroy(
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

### `dvz_app_gui()`

Register a GUI callback.

```c
void dvz_app_gui(
    DvzApp* app,  // the app
    DvzId canvas_id,  // the canvas ID
    DvzAppGuiCallback callback,  // the GUI callback
    void* user_data,  // the user data
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

### `dvz_app_onkeyboard()`

Register a keyboard callback.

```c
void dvz_app_onkeyboard(
    DvzApp* app,  // the app
    DvzAppKeyboardCallback callback,  // the callback
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

### `dvz_app_onresize()`

Register a resize callback.

```c
void dvz_app_onresize(
    DvzApp* app,  // the app
    DvzAppResizeCallback callback,  // the callback
    void* user_data,  // the user data
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

### `dvz_app_run()`

Start the application event loop.

```c
void dvz_app_run(
    DvzApp* app,  // the app
    uint64_t n_frames,  // the maximum number of frames, 0 for infinite loop
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

### `dvz_app_submit()`

Submit the current batch to the application.

```c
void dvz_app_submit(
    DvzApp* app,  // the app
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

### `dvz_arcball_angles()`

Get the current arcball angles.

```c
void dvz_arcball_angles(
    DvzArcball* arcball,  // the arcball
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

### `dvz_arcball_end()`

Finalize arcball position update.

```c
void dvz_arcball_end(
    DvzArcball* arcball,  // the arcball
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

### `dvz_arcball_gui()`

Show a GUI with sliders controlling the three arcball angles.

```c
void dvz_arcball_gui(
    DvzArcball* arcball,  // the arcball
    DvzApp* app,  // the app
    DvzId canvas_id,  // the canvas (or figure) ID
    DvzPanel* panel,  // the panel
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

### `dvz_arcball_model()`

Return the model matrix of an arcball.

```c
void dvz_arcball_model(
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

### `dvz_arcball_rotate()`

Apply a rotation to an arcball.

```c
void dvz_arcball_rotate(
    DvzArcball* arcball,  // the arcball
    vec2 cur_pos,  // the initial position
    vec2 last_pos,  // the final position
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

### `dvz_atlas_destroy()`

Destroy an atlas.

```c
void dvz_atlas_destroy(
    DvzAtlas* atlas,  // the atlas
)
```

### `dvz_atlas_font()`

Load the default atlas and font.

```c
DvzAtlasFont dvz_atlas_font(  // returns: a DvzAtlasFont struct with DvzAtlas and DvzFont objects.
    double font_size,  // the font size
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

### `dvz_basic_alloc()`

Allocate memory for a visual.

```c
void dvz_basic_alloc(
    DvzVisual* visual,  // the visual
    uint32_t item_count,  // the total number of items to allocate for this visual
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

### `dvz_basic_group()`

Set the vertex group index.

```c
void dvz_basic_group(
    DvzVisual* visual,  // the visual
    uint32_t first,  // the index of the first item to update
    uint32_t count,  // the number of items to update
    float* values,  // the group index of each vertex
    int flags,  // the data update flags
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

### `dvz_basic_size()`

Set the point size (for POINT_LIST topology only).

```c
void dvz_basic_size(
    DvzVisual* visual,  // the visual
    float size,  // the point size in pixels
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

### `dvz_camera_lookat()`

Set a camera lookat position.

```c
void dvz_camera_lookat(
    DvzCamera* camera,  // the camera
    vec3 lookat,  // the lookat position
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

### `dvz_camera_perspective()`

Set a camera perspective.

```c
void dvz_camera_perspective(
    DvzCamera* camera,  // the camera
    float fov,  // the field of view angle (in radians)
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

### `dvz_camera_print()`

Display information about a camera.

```c
void dvz_camera_print(
    DvzCamera* camera,  // the camera
)
```

### `dvz_camera_reset()`

Reset a camera.

```c
void dvz_camera_reset(
    DvzCamera* camera,  // the camera
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

### `dvz_camera_up()`

Set a camera up vector.

```c
void dvz_camera_up(
    DvzCamera* camera,  // the camera
    vec3 up,  // the up vector
)
```

### `dvz_camera_viewproj()`

Return the view and proj matrices of the camera.

```c
void dvz_camera_viewproj(
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

### `dvz_colormap()`

Fetch a color from a colormap and a value.

```c
void dvz_colormap(
    DvzColormap cmap,  // the colormap
    uint8_t value,  // the value
)
```

### `dvz_colormap_array()`

Fetch colors from a colormap and an array of values.

```c
void dvz_colormap_array(
    DvzColormap cmap,  // the colormap
    uint32_t count,  // the number of values
    float* values,  // pointer to the array of float numbers
    float vmin,  // the minimum value
    float vmax,  // the maximum value
)
```

### `dvz_colormap_scale()`

Fetch a color from a colormap and an interpolated value.

```c
void dvz_colormap_scale(
    DvzColormap cmap,  // the colormap
    float value,  // the value
    float vmin,  // the minimum value
    float vmax,  // the maximum value
)
```

### `dvz_demo()`

Run a demo.

```c
void dvz_demo(

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

### `dvz_figure_destroy()`

Destroy a figure.

```c
void dvz_figure_destroy(
    DvzFigure* figure,  // the figure
)
```

### `dvz_figure_id()`

Return a figure ID.

```c
DvzId dvz_figure_id(  // returns: the figure ID
    DvzFigure* figure,  // the figure
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

### `dvz_figure_update()`

Update a figure after the composition of the panels and visuals has changed.

```c
void dvz_figure_update(
    DvzFigure* figure,  // the figure
)
```

### `dvz_font()`

Create a font.

```c
DvzFont* dvz_font(  // returns: the font
    long ttf_size,  // size in bytes of a TTF font raw buffer
    char* ttf_bytes,  // TTF font raw buffer
)
```

### `dvz_font_ascii()`

Compute the shift of each glyph in an ASCII string, using the Freetype library.

```c
vec4* dvz_font_ascii(  // returns: an array of (x,y,w,h) shifts
    DvzFont* font,  // the font
    char* string,  // the ASCII string
)
```

### `dvz_font_destroy()`

Destroy a font.

```c
void dvz_font_destroy(
    DvzFont* font,  // the font
)
```

### `dvz_font_draw()`

Render a string using Freetype.

```c
uint8_t* dvz_font_draw(  // returns: an RGBA array allocated by this function and that MUST be freed by the caller
    DvzFont* font,  // the font
    uint32_t length,  // the number of glyphs
    uint32_t* codepoints,  // the Unicode codepoints of the glyphs
    vec4* xywh,  // an array of (x,y,w,h) shifts, returned by dvz_font_layout()
    int flags,  // the font flags
)
```

### `dvz_font_layout()`

Compute the shift of each glyph in a Unicode string, using the Freetype library.

```c
vec4* dvz_font_layout(  // returns: an array of (x,y,w,h) shifts
    DvzFont* font,  // the font
    uint32_t length,  // the number of glyphs
    uint32_t* codepoints,  // the Unicode codepoints of the glyphs
)
```

### `dvz_font_size()`

Set the font size.

```c
void dvz_font_size(
    DvzFont* font,  // the font
    double size,  // the font size
)
```

### `dvz_font_texture()`

Generate a texture with a rendered text.

```c
DvzId dvz_font_texture(  // returns: a tex ID
    DvzFont* font,  // the font
    DvzBatch* batch,  // the batch
    uint32_t length,  // the number of Unicode codepoints
    uint32_t* codepoints,  // the Unicode codepoints
)
```

### `dvz_free()`

Free a pointer.

```c
void dvz_free(
    void* pointer,  // a pointer
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

### `dvz_glyph_ascii()`

Set the glyph ascii characters.

```c
void dvz_glyph_ascii(
    DvzVisual* visual,  // the visual
    char* string,  // the characters
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

### `dvz_glyph_bgcolor()`

Set the glyph background color.

```c
void dvz_glyph_bgcolor(
    DvzVisual* visual,  // the visual
    vec4 bgcolor,  // the background color
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

### `dvz_glyph_texture()`

Assign a texture to a glyph visual.

```c
void dvz_glyph_texture(
    DvzVisual* visual,  // the visual
    DvzId tex,  // the texture ID
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

### `dvz_gui_alpha()`

Set the alpha transparency of the next GUI dialog.

```c
void dvz_gui_alpha(
    float alpha,  // the alpha transparency value
)
```

### `dvz_gui_begin()`

Start a new dialog.

```c
void dvz_gui_begin(
    char* title,  // the dialog title
    int flags,  // the flags
)
```

### `dvz_gui_button()`

Add a button.

```c
bool dvz_gui_button(  // returns: whether the button was pressed
    char* name,  // the button name
    float width,  // the button width
    float height,  // the button height
)
```

### `dvz_gui_checkbox()`

Add a checkbox.

```c
bool dvz_gui_checkbox(  // returns: whether the checkbox's state has changed
    char* name,  // the button name
     width,  // the button width
     height,  // the button height
)
```

### `dvz_gui_colorpicker()`

Add a color picker

```c
bool dvz_gui_colorpicker(
    char* name,  // the widget name
    vec3 color,  // the color
    int flags,  // the widget flags
)
```

### `dvz_gui_corner()`

Set the corner position of the next GUI dialog.

```c
void dvz_gui_corner(
    DvzCorner corner,  // which corner
    vec2 pad,  // the pad
)
```

### `dvz_gui_demo()`

Show the demo GUI.

```c
void dvz_gui_demo(

)
```

### `dvz_gui_end()`

Stop the creation of the dialog.

```c
void dvz_gui_end(

)
```

### `dvz_gui_flags()`

Set the flags of the next GUI dialog.

```c
int dvz_gui_flags(
    int flags,  // the flags
)
```

### `dvz_gui_image()`

Add an image in a GUI dialog.

```c
void dvz_gui_image(
    DvzTex* tex,  // the texture
    float width,  // the image width
    float height,  // the image height
)
```

### `dvz_gui_pos()`

Set the position of the next GUI dialog.

```c
void dvz_gui_pos(
    vec2 pos,  // the dialog position
    vec2 pivot,  // the pivot
)
```

### `dvz_gui_size()`

Set the size of the next GUI dialog.

```c
void dvz_gui_size(
    vec2 size,  // the size
)
```

### `dvz_gui_slider()`

Add a slider.

```c
bool dvz_gui_slider(  // returns: whether the value has changed
    char* name,  // the slider name
    float vmin,  // the minimum value
    float vmax,  // the maximum value
    float* value,  // the pointer to the value
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

### `dvz_image_alloc()`

Allocate memory for a visual.

```c
void dvz_image_alloc(
    DvzVisual* visual,  // the visual
    uint32_t item_count,  // the total number of images to allocate for this visual
)
```

### `dvz_image_anchor()`

Set the image anchors.

```c
void dvz_image_anchor(
    DvzVisual* visual,  // the visual
    uint32_t first,  // the index of the first item to update
    uint32_t count,  // the number of items to update
    vec2* values,  // the relative anchors of each image, (0,0 = position pertains to top left corner)
    int flags,  // the data update flags
)
```

### `dvz_image_color()`

Set the image colors (only when using DVZ_IMAGE_FLAGS_FILL).

```c
void dvz_image_color(
    DvzVisual* visual,  // the visual
    uint32_t first,  // the index of the first item to update
    uint32_t count,  // the number of items to update
    cvec4* values,  // the image colors
    int flags,  // the data update flags
)
```

### `dvz_image_edge_color()`

Set the edge color.

```c
void dvz_image_edge_color(
    DvzVisual* visual,  // the visual
    cvec4 color,  // the edge color
)
```

### `dvz_image_edge_width()`

Set the edge width.

```c
void dvz_image_edge_width(
    DvzVisual* visual,  // the visual
    float width,  // the edge width
)
```

### `dvz_image_position()`

Set the image positions.

```c
void dvz_image_position(
    DvzVisual* visual,  // the visual
    uint32_t first,  // the index of the first item to update
    uint32_t count,  // the number of items to update
    vec3* values,  // the 3D positions of the top left corner
    int flags,  // the data update flags
)
```

### `dvz_image_radius()`

Use a rounded rectangle for images, with a given radius in pixels.

```c
void dvz_image_radius(
    DvzVisual* visual,  // the visual
    float radius,  // the rounded corner radius, in pixel
)
```

### `dvz_image_size()`

Set the image sizes.

```c
void dvz_image_size(
    DvzVisual* visual,  // the visual
    uint32_t first,  // the index of the first item to update
    uint32_t count,  // the number of items to update
    vec2* values,  // the sizes of each image, in pixels
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
    vec4* tl_br,  // the tex coordinates of the top left and bottom right corners (vec4 u0,v0,u1,v1)
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

### `dvz_marker()`

Create a marker visual.

```c
DvzVisual* dvz_marker(  // returns: the visual
    DvzBatch* batch,  // the batch
    int flags,  // the visual creation flags
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

### `dvz_marker_aspect()`

Set the marker aspect.

```c
void dvz_marker_aspect(
    DvzVisual* visual,  // the visual
    DvzMarkerAspect aspect,  // the marker aspect, one of DVZ_MARKER_ASPECT_FILLED, DVZ_MARKER_ASPECT_STROKE,
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
    cvec4 color,  // the edge color
)
```

### `dvz_marker_edge_width()`

Set the marker edge width.

```c
void dvz_marker_edge_width(
    DvzVisual* visual,  // the visual
    float width,  // the edge width
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

### `dvz_marker_shape()`

Set the marker shape.

```c
void dvz_marker_shape(
    DvzVisual* visual,  // the visual
    DvzMarkerShape shape,  // the marker shape
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
    float scale,  // the texture scale
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

### `dvz_mesh_alloc()`

Allocate memory for a visual.

```c
void dvz_mesh_alloc(
    DvzVisual* visual,  // the visual
    uint32_t vertex_count,  // the number of vertices
    uint32_t index_count,  // the number of indices
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

### `dvz_mesh_contour()`

Set the contour information for polygon contours.

```c
void dvz_mesh_contour(
    DvzVisual* visual,  // the visual
    uint32_t first,  // the index of the first item to update
    uint32_t count,  // the number of items to update
    cvec3* values,  // for vertex A, B, C, the least significant bit is 1 if the opposite edge is a
    int flags,  // the data update flags
)
```

### `dvz_mesh_density()`

Set the number of isolines

```c
void dvz_mesh_density(
    DvzVisual* visual,  // the mesh
    uint32_t count,  // the number of isolines
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

### `dvz_mesh_isoline()`

Set the isolines values.

```c
void dvz_mesh_isoline(
    DvzVisual* visual,  // the visual
    uint32_t first,  // the index of the first item to update
    uint32_t count,  // the number of items to update
    float* values,  // the scalar field for which to draw isolines
    int flags,  // the data update flags
)
```

### `dvz_mesh_left()`

Set the distance between the current vertex to the left edge at corner A, B, or C in triangle

```c
void dvz_mesh_left(
    DvzVisual* visual,  // the visual
    uint32_t first,  // the index of the first item to update
    uint32_t count,  // the number of items to update
    vec3* values,  // the distance to the left edge adjacent to each triangle vertex
    int flags,  // the data update flags
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

### `dvz_mesh_light_pos()`

Set the mesh light position.

```c
void dvz_mesh_light_pos(
    DvzVisual* visual,  // the mesh
    vec3 pos,  // the light position
)
```

### `dvz_mesh_linewidth()`

Set the stroke linewidth (wireframe or isoline).

```c
void dvz_mesh_linewidth(
    DvzVisual* visual,  // the mesh
    float linewidth,  // the line width
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

### `dvz_mesh_reshape()`

Update a mesh once a shape has been updated.

```c
void dvz_mesh_reshape(
    DvzVisual* visual,  // the mesh
    DvzShape* shape,  // the shape
)
```

### `dvz_mesh_right()`

Set the distance between the current vertex to the right edge at corner A, B, or C in triangle

```c
void dvz_mesh_right(
    DvzVisual* visual,  // the visual
    uint32_t first,  // the index of the first item to update
    uint32_t count,  // the number of items to update
    vec3* values,  // the distance to the right edge adjacent to each triangle vertex
    int flags,  // the data update flags
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

### `dvz_mesh_stroke()`

Set the stroke color.

```c
void dvz_mesh_stroke(
    DvzVisual* visual,  // the mesh
     stroke,  // the rgba components
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

### `dvz_monoglyph()`

Create a monoglyph visual.

```c
DvzVisual* dvz_monoglyph(  // returns: the visual
    DvzBatch* batch,  // the batch
    int flags,  // the visual creation flags
)
```

### `dvz_monoglyph_alloc()`

Allocate memory for a visual.

```c
void dvz_monoglyph_alloc(
    DvzVisual* visual,  // the visual
    uint32_t item_count,  // the total number of items to allocate for this visual
)
```

### `dvz_monoglyph_anchor()`

Set the glyph anchor (relative to the glyph size).

```c
void dvz_monoglyph_anchor(
    DvzVisual* visual,  // the visual
    vec2 anchor,  // the anchor
)
```

### `dvz_monoglyph_color()`

Set the glyph colors.

```c
void dvz_monoglyph_color(
    DvzVisual* visual,  // the visual
    uint32_t first,  // the index of the first item to update
    uint32_t count,  // the number of items to update
    cvec4* values,  // the colors of the items to update
    int flags,  // the data update flags
)
```

### `dvz_monoglyph_glyph()`

Set the text.

```c
void dvz_monoglyph_glyph(
    DvzVisual* visual,  // the visual
    char* text,  // the ASCII test (string length without the null terminal byte = number of glyphs)
)
```

### `dvz_monoglyph_offset()`

Set the glyph offsets.

```c
void dvz_monoglyph_offset(
    DvzVisual* visual,  // the visual
    uint32_t first,  // the index of the first item to update
    uint32_t count,  // the number of items to update
    ivec2* values,  // the glyph offsets (ivec2 integers: row,column)
    int flags,  // the data update flags
)
```

### `dvz_monoglyph_position()`

Set the glyph positions.

```c
void dvz_monoglyph_position(
    DvzVisual* visual,  // the visual
    uint32_t first,  // the index of the first item to update
    uint32_t count,  // the number of items to update
    vec3* values,  // the 3D positions of the items to update
    int flags,  // the data update flags
)
```

### `dvz_monoglyph_size()`

Set the glyph size (relative to the initial glyph size).

```c
void dvz_monoglyph_size(
    DvzVisual* visual,  // the visual
    float size,  // the glyph size
)
```

### `dvz_monoglyph_textarea()`

All-in-one function for multiline text.

```c
void dvz_monoglyph_textarea(
    DvzVisual* visual,  // the visual
    vec3 pos,  // the text position
    cvec4 color,  // the text color
    float size,  // the glyph size
    char* text,  // the text, can contain `\n` new lines
)
```

### `dvz_mvp()`

Create a MVP structure.

```c
DvzMVP dvz_mvp(  // returns: the MVP structure
    mat4 model,  // the model matrix
    mat4 view,  // the view matrix
    mat4 proj,  // the projection matrix
)
```

### `dvz_ortho_end()`

End an ortho interaction.

```c
void dvz_ortho_end(
    DvzOrtho* ortho,  // the ortho
)
```

### `dvz_ortho_flags()`

Set the ortho flags.

```c
void dvz_ortho_flags(
    DvzOrtho* ortho,  // the ortho
    int flags,  // the flags
)
```

### `dvz_ortho_mvp()`

Apply an MVP matrix to an ortho.

```c
void dvz_ortho_mvp(
    DvzOrtho* ortho,  // the ortho
    DvzMVP* mvp,  // the MVP
)
```

### `dvz_ortho_pan()`

Apply a pan value to an ortho.

```c
void dvz_ortho_pan(
    DvzOrtho* ortho,  // the ortho
    vec2 pan,  // the pan, in NDC
)
```

### `dvz_ortho_pan_shift()`

Apply a pan shift to an ortho.

```c
void dvz_ortho_pan_shift(
    DvzOrtho* ortho,  // the ortho
    vec2 shift_px,  // the shift value, in pixels
    vec2 center_px,  // the center position, in pixels
)
```

### `dvz_ortho_reset()`

Reset an ortho.

```c
void dvz_ortho_reset(
    DvzOrtho* ortho,  // the ortho
)
```

### `dvz_ortho_resize()`

Inform an ortho of a panel resize.

```c
void dvz_ortho_resize(
    DvzOrtho* ortho,  // the ortho
    float width,  // the panel width
    float height,  // the panel height
)
```

### `dvz_ortho_zoom()`

Apply a zoom value to an ortho.

```c
void dvz_ortho_zoom(
    DvzOrtho* ortho,  // the ortho
    float zoom,  // the zoom level
)
```

### `dvz_ortho_zoom_shift()`

Apply a zoom shift to an ortho.

```c
void dvz_ortho_zoom_shift(
    DvzOrtho* ortho,  // the ortho
    vec2 shift_px,  // the shift value, in pixels
    vec2 center_px,  // the center position, in pixels
)
```

### `dvz_ortho_zoom_wheel()`

Apply a wheel zoom to an ortho.

```c
void dvz_ortho_zoom_wheel(
    DvzOrtho* ortho,  // the ortho
    vec2 dir,  // the wheel direction
    vec2 center_px,  // the center position, in pixels
)
```

### `dvz_panel()`

Create a panel in a figure (partial or complete rectangular portion of a figure).

```c
DvzPanel* dvz_panel(
    DvzFigure* fig,  // the figure
    float x,  // the x coordinate of the top left corner, in pixels
    float y,  // the y coordinate of the top left corner, in pixels
    float width,  // the panel width, in pixels
    float height,  // the panel height, in pixels
)
```

### `dvz_panel_arcball()`

Set arcball interactivity for a panel.

```c
DvzArcball* dvz_panel_arcball(  // returns: the arcball
    DvzPanel* panel,  // the panel
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
    int flags,  // the camera flags
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

### `dvz_panel_default()`

Return the default full panel spanning an entire figure.

```c
DvzPanel* dvz_panel_default(  // returns: the panel spanning the entire figure
    DvzFigure* fig,  // the figure
)
```

### `dvz_panel_destroy()`

Destroy a panel.

```c
void dvz_panel_destroy(
    DvzPanel* panel,  // the panel
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

### `dvz_panel_mvp()`

Assign a MVP structure to a panel.

```c
void dvz_panel_mvp(
    DvzPanel* panel,  // the panel
    DvzMVP* mvp,  // a pointer to the MVP structure
)
```

### `dvz_panel_mvpmat()`

Assign the model-view-proj matrices to a panel.

```c
void dvz_panel_mvpmat(
    DvzPanel* panel,  // the panel
    mat4 model,  // the model matrix
    mat4 view,  // the view matrix
    mat4 proj,  // the projection matrix
)
```

### `dvz_panel_ortho()`

Set ortho interactivity for a panel.

```c
DvzOrtho* dvz_panel_ortho(  // returns: the ortho
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

### `dvz_panel_resize()`

Resize a panel.

```c
void dvz_panel_resize(
    DvzPanel* panel,  // the panel
    float x,  // the x coordinate of the top left corner, in pixels
    float y,  // the y coordinate of the top left corner, in pixels
    float width,  // the panel width, in pixels
    float height,  // the panel height, in pixels
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

### `dvz_panzoom_end()`

End a panzoom interaction.

```c
void dvz_panzoom_end(
    DvzPanzoom* pz,  // the panzoom
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

### `dvz_panzoom_mvp()`

Apply an MVP matrix to a panzoom.

```c
void dvz_panzoom_mvp(
    DvzPanzoom* pz,  // the panzoom
    DvzMVP* mvp,  // the MVP
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

### `dvz_panzoom_pan_shift()`

Apply a pan shift to a panzoom.

```c
void dvz_panzoom_pan_shift(
    DvzPanzoom* pz,  // the panzoom
    vec2 shift_px,  // the shift value, in pixels
    vec2 center_px,  // the center position, in pixels
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

### `dvz_panzoom_xlim()`

Set a panzoom x limits.

```c
void dvz_panzoom_xlim(
    DvzPanzoom* pz,  // the panzoom
    vec2 xlim,  // the xlim (FLOAT_MIN/MAX=no lim)
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

### `dvz_panzoom_ylim()`

Set a panzoom y limits.

```c
void dvz_panzoom_ylim(
    DvzPanzoom* pz,  // the panzoom
    vec2 ylim,  // the ylim (FLOAT_MIN/MAX=no lim)
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

### `dvz_panzoom_zlim()`

Set a panzoom z limits.

```c
void dvz_panzoom_zlim(
    DvzPanzoom* pz,  // the panzoom
    vec2 zlim,  // the zlim (FLOAT_MIN/MAX=no lim)
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

### `dvz_panzoom_zoom_shift()`

Apply a zoom shift to a panzoom.

```c
void dvz_panzoom_zoom_shift(
    DvzPanzoom* pz,  // the panzoom
    vec2 shift_px,  // the shift value, in pixels
    vec2 center_px,  // the center position, in pixels
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

### `dvz_path()`

Create a path visual.

```c
DvzVisual* dvz_path(  // returns: the visual
    DvzBatch* batch,  // the batch
    int flags,  // the visual creation flags
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

### `dvz_path_cap()`

Set the path cap.

```c
void dvz_path_cap(
    DvzVisual* visual,  // the visual
    DvzCapType cap,  // the cap
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

### `dvz_path_join()`

Set the path join.

```c
void dvz_path_join(
    DvzVisual* visual,  // the visual
    DvzJoinType join,  // the join
)
```

### `dvz_path_linewidth()`

Set the path line width.

```c
void dvz_path_linewidth(
    DvzVisual* visual,  // the visual
    float width,  // the line width
)
```

### `dvz_path_position()`

Set the path positions. Note: all path point positions must be updated at once for now.

```c
void dvz_path_position(
    DvzVisual* visual,  // the visual
     vertex_count,  // the total number of points across all paths
    vec3* positions,  // the path point positions
    uint32_t path_count,  // the number of different paths
    uint32_t* path_lengths,  // the number of points in each path
    int flags,  // the data update flags
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

### `dvz_pixel_alloc()`

Allocate memory for a visual.

```c
void dvz_pixel_alloc(
    DvzVisual* visual,  // the visual
    uint32_t item_count,  // the total number of items to allocate for this visual
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

### `dvz_point()`

Create a point visual.

```c
DvzVisual* dvz_point(  // returns: the visual
    DvzBatch* batch,  // the batch
    int flags,  // the visual creation flags
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

### `dvz_resample()`

Normalize a value in an interval.

```c
double dvz_resample(  // returns: the normalized value between 0 and 1
    double t0,  // the interval start
    double t1,  // the interval end
    double t,  // the value within the interval
)
```

### `dvz_scene()`

Create a scene.

```c
DvzScene* dvz_scene(  // returns: the scene
    DvzBatch* batch,  // the batch
)
```

### `dvz_scene_destroy()`

Destroy a scene.

```c
void dvz_scene_destroy(
    DvzScene* scene,  // the scene
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

### `dvz_scene_run()`

Start the event loop and render the scene in a window.

```c
void dvz_scene_run(
    DvzScene* scene,  // the scene
    DvzApp* app,  // the app
    uint64_t n_frames,  // the maximum number of frames, 0 for infinite loop
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

### `dvz_segment_alloc()`

Allocate memory for a visual.

```c
void dvz_segment_alloc(
    DvzVisual* visual,  // the visual
    uint32_t item_count,  // the total number of items to allocate for this visual
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

### `dvz_shape_begin()`

Start a transformation sequence.

```c
void dvz_shape_begin(
    DvzShape* shape,  // the shape
    uint32_t first,  // the first vertex to modify
    uint32_t count,  // the number of vertices to modify
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

### `dvz_shape_cube()`

Create a cube shape.

```c
DvzShape dvz_shape_cube(  // returns: the shape
    cvec4* colors,  // the colors of the six faces
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

### `dvz_shape_destroy()`

Destroy a shape.

```c
void dvz_shape_destroy(
    DvzShape* shape,  // the shape
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

### `dvz_shape_end()`

Apply the transformation sequence and reset it.

```c
void dvz_shape_end(
    DvzShape* shape,  // the shape
)
```

### `dvz_shape_merge()`

Merge several shapes.

```c
DvzShape dvz_shape_merge(  // returns: the merged shape
    uint32_t count,  // the number of shapes to merge
    DvzShape* shapes,  // the shapes to merge
)
```

### `dvz_shape_normalize()`

Normalize a shape.

```c
void dvz_shape_normalize(
    DvzShape* shape,  // the shape
)
```

### `dvz_shape_normals()`

Recompute the face normals.

```c
void dvz_shape_normals(
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

### `dvz_shape_polygon()`

Create a polygon shape using the simple earcut polygon triangulation algorithm.

```c
DvzShape dvz_shape_polygon(  // returns: the shape
    uint32_t count,  // the number of points along the polygon border
    dvec2* points,  // the points 2D coordinates
    cvec4 color,  // the polygon color
)
```

### `dvz_shape_print()`

Show information about a shape.

```c
void dvz_shape_print(
    DvzShape* shape,  // the shape
)
```

### `dvz_shape_rescaling()`

Compute the rescaling factor to renormalize a shape.

```c
float dvz_shape_rescaling(
    DvzShape* shape,  // the shape
    int flags,  // the rescaling flags
)
```

### `dvz_shape_rotate()`

Append a rotation to a shape.

```c
void dvz_shape_rotate(
    DvzShape* shape,  // the shape
    float angle,  // the rotation angle
    vec3 axis,  // the rotation axis
)
```

### `dvz_shape_scale()`

Append a scaling transform to a shape.

```c
void dvz_shape_scale(
    DvzShape* shape,  // the shape
    vec3 scale,  // the scaling factors
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

### `dvz_shape_square()`

Create a square shape.

```c
DvzShape dvz_shape_square(  // returns: the shape
    cvec4 color,  // the square color
)
```

### `dvz_shape_surface()`

Create a grid shape.

```c
DvzShape dvz_shape_surface(  // returns: the shape
    uint32_t row_count,  // number of rows
    uint32_t col_count,  // number of cols
    float* heights,  // a pointer to row_count*col_count height values (floats)
    cvec4* colors,  // a pointer to row_count*col_count color values (cvec4)
    vec3 o,  // the origin
    vec3 u,  // the unit vector parallel to each column
    vec3 v,  // the unit vector parallel to each row
    int flags,  // the grid creation flags
)
```

### `dvz_shape_transform()`

Append an arbitrary transformation.

```c
void dvz_shape_transform(
    DvzShape* shape,  // the shape
    mat4 transform,  // the transform mat4 matrix
)
```

### `dvz_shape_translate()`

Append a translation to a shape.

```c
void dvz_shape_translate(
    DvzShape* shape,  // the shape
    vec3 translate,  // the translation vector
)
```

### `dvz_shape_unindex()`

Convert an indexed shape to a non-indexed one by duplicating the vertex values according

```c
void dvz_shape_unindex(
    DvzShape* shape,  // the shape
    int flags,  // the flags
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

### `dvz_slice_alloc()`

Allocate memory for a visual.

```c
void dvz_slice_alloc(
    DvzVisual* visual,  // the visual
    uint32_t item_count,  // the total number of slices to allocate for this visual
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

### `dvz_slice_position()`

Set the slice positions.

```c
void dvz_slice_position(
    DvzVisual* visual,  // the visual
    uint32_t first,  // the index of the first item to update
    uint32_t count,  // the number of items to update
    vec3* p0,  // the 3D positions of the top left corner
    vec3* p1,  // the 3D positions of the top right corner
    vec3* p2,  // the 3D positions of the bottom left corner
    vec3* p3,  // the 3D positions of the bottom right corner
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
    vec3* uvw0,  // the 3D texture coordinates of the top left corner
    vec3* uvw1,  // the 3D texture coordinates of the top right corner
    vec3* uvw2,  // the 3D texture coordinates of the bottom left corner
    vec3* uvw3,  // the 3D texture coordinates of the bottom right corner
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

### `dvz_sphere()`

Create a sphere visual.

```c
DvzVisual* dvz_sphere(  // returns: the visual
    DvzBatch* batch,  // the batch
    int flags,  // the visual creation flags
)
```

### `dvz_sphere_alloc()`

Allocate memory for a visual.

```c
void dvz_sphere_alloc(
    DvzVisual* visual,  // the visual
    uint32_t item_count,  // the total number of spheres to allocate for this visual
)
```

### `dvz_sphere_color()`

Set the sphere colors.

```c
void dvz_sphere_color(
    DvzVisual* visual,  // the visual
    uint32_t first,  // the index of the first item to update
    uint32_t count,  // the number of items to update
    cvec4* color,  // the sphere colors
    int flags,  // the data update flags
)
```

### `dvz_sphere_light_params()`

Set the sphere light parameters.

```c
void dvz_sphere_light_params(
    DvzVisual* visual,  // the visual
    vec4 params,  // the light parameters (vec4 ambient, diffuse, specular, exponent)
)
```

### `dvz_sphere_light_pos()`

Set the sphere light position.

```c
void dvz_sphere_light_pos(
    DvzVisual* visual,  // the visual
    vec3 pos,  // the light position
)
```

### `dvz_sphere_position()`

Set the sphere positions.

```c
void dvz_sphere_position(
    DvzVisual* visual,  // the visual
    uint32_t first,  // the index of the first item to update
    uint32_t count,  // the number of items to update
    vec3* pos,  // the 3D positions of the sphere centers
    int flags,  // the data update flags
)
```

### `dvz_sphere_size()`

Set the sphere sizes.

```c
void dvz_sphere_size(
    DvzVisual* visual,  // the visual
    uint32_t first,  // the index of the first item to update
    uint32_t count,  // the number of items to update
    float* size,  // the radius of the spheres
    int flags,  // the data update flags
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

### `dvz_version()`

Return the current version string.

```c
char* dvz_version(  // returns: the version string

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

### `dvz_visual_depth()`

Set the visual depth.

```c
void dvz_visual_depth(
    DvzVisual* visual,  // the visual
    DvzDepthTest depth_test,  // whether to activate the depth test
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

### `dvz_visual_show()`

Set the visibility of a visual.

```c
void dvz_visual_show(
    DvzVisual* visual,  // the visual
    bool is_visible,  // the visual visibility
)
```

### `dvz_visual_update()`

Update a visual after its data has changed.

```c
void dvz_visual_update(
    DvzVisual* visual,  // the visual
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

### `dvz_volume_texcoords()`

Set the texture coordinates of two corner points.

```c
void dvz_volume_texcoords(
    DvzVisual* visual,  // the visual
    vec3 uvw0,  // coordinates of one of the corner points
    vec3 uvw1,  // coordinates of one of the corner points
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

### `dvz_volume_transfer()`

Set the volume size.

```c
void dvz_volume_transfer(
    DvzVisual* visual,  // the visual
    vec4 transfer,  // transfer function, for now `vec4(x, 0, 0, 0)` where x is a scaling factor
)
```

### `dvz_earcut()`

Compute a polygon triangulation with only indexing on the polygon contour vertices.

```c
DvzIndex* dvz_earcut(  // returns: the computed indices (must be FREED by the caller)
    uint32_t point_count,  // the number of points
    dvec2* polygon,  // the polygon 2D positions
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

### `dvz_mock_band()`

Generate points on a band.

```c
vec3* dvz_mock_band(  // returns: the positions
    uint32_t count,  // the number of positions to generate
    vec2 size,  // the size of the band
)
```

### `dvz_mock_circle()`

Generate points on a circle.

```c
vec3* dvz_mock_circle(  // returns: the positions
    uint32_t count,  // the number of positions to generate
    float radius,  // the radius of the circle
)
```

### `dvz_mock_cmap()`

Generate a set of HSV colors.

```c
cvec4* dvz_mock_cmap(  // returns: colors
    uint32_t count,  // the number of colors to generate
    uint8_t alpha,  // the alpha value
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

### `dvz_mock_fixed()`

Generate identical 3D positions.

```c
vec3* dvz_mock_fixed(  // returns: the repeated positions
    uint32_t count,  // the number of positions to generate
    vec3 fixed,  // the position
)
```

### `dvz_mock_full()`

Generate an array with the same value.

```c
float* dvz_mock_full(  // returns: the values
    uint32_t count,  // the number of scalars to generate
    float value,  // the value
)
```

### `dvz_mock_line()`

Generate 3D positions on a line.

```c
vec3* dvz_mock_line(  // returns: the positions
    uint32_t count,  // the number of positions to generate
    vec3 p0,  // initial position
    vec3 p1,  // terminal position
)
```

### `dvz_mock_linspace()`

Generate an array ranging from an initial value to a final value.

```c
float* dvz_mock_linspace(  // returns: the values
    uint32_t count,  // the number of scalars to generate
    float initial,  // the initial value
    float final,  // the final value
)
```

### `dvz_mock_monochrome()`

Repeat a color in an array.

```c
cvec4* dvz_mock_monochrome(  // returns: colors
    uint32_t count,  // the number of colors to generate
    cvec4 mono,  // the color to repeat
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

### `dvz_mock_range()`

Generate an array of consecutive positive numbers.

```c
uint32_t* dvz_mock_range(  // returns: the values
    uint32_t count,  // the number of consecutive integers to generate
    uint32_t initial,  // the initial value
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

### `dvz_next_pow2()`

Return the smallest power of 2 larger or equal than a positive integer.

```c
uint64_t dvz_next_pow2(  // returns: the power of 2
    uint64_t x,  // the value
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

### `dvz_rand_byte()`

Return a random integer number between 0 and 255.

```c
uint8_t dvz_rand_byte(  // returns: random number

)
```

### `dvz_rand_double()`

Return a random floating-point number between 0 and 1.

```c
double dvz_rand_double(  // returns: random number

)
```

### `dvz_rand_float()`

Return a random floating-point number between 0 and 1.

```c
float dvz_rand_float(  // returns: random number

)
```

### `dvz_rand_int()`

Return a random integer number.

```c
int dvz_rand_int(  // returns: random number

)
```

### `dvz_rand_normal()`

Return a random normal floating-point number.

```c
double dvz_rand_normal(  // returns: random number

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

## Enumerations

### `DvzAppFlags`

```
DVZ_APP_FLAGS_NONE
DVZ_APP_FLAGS_OFFSCREEN
DVZ_APP_FLAGS_WHITE_BACKGROUND
```

### `DvzArcballFlags`

```
DVZ_ARCBALL_FLAGS_NONE
DVZ_ARCBALL_FLAGS_CONSTRAIN
```

### `DvzCameraFlags`

```
DVZ_CAMERA_FLAGS_PERSPECTIVE
DVZ_CAMERA_FLAGS_ORTHO
```

### `DvzCanvasFlags`

```
DVZ_CANVAS_FLAGS_NONE
DVZ_CANVAS_FLAGS_IMGUI
DVZ_CANVAS_FLAGS_FPS
DVZ_CANVAS_FLAGS_MONITOR
DVZ_CANVAS_FLAGS_VSYNC
DVZ_CANVAS_FLAGS_PICK
```

### `DvzCapType`

```
DVZ_CAP_NONE
DVZ_CAP_ROUND
DVZ_CAP_TRIANGLE_IN
DVZ_CAP_TRIANGLE_OUT
DVZ_CAP_SQUARE
DVZ_CAP_BUTT
DVZ_CAP_COUNT
```

### `DvzColormap`

```
DVZ_CMAP_BINARY
DVZ_CMAP_HSV
DVZ_CMAP_CIVIDIS
DVZ_CMAP_INFERNO
DVZ_CMAP_MAGMA
DVZ_CMAP_PLASMA
DVZ_CMAP_VIRIDIS
DVZ_CMAP_BLUES
DVZ_CMAP_BUGN
DVZ_CMAP_BUPU
DVZ_CMAP_GNBU
DVZ_CMAP_GREENS
DVZ_CMAP_GREYS
DVZ_CMAP_ORANGES
DVZ_CMAP_ORRD
DVZ_CMAP_PUBU
DVZ_CMAP_PUBUGN
DVZ_CMAP_PURPLES
DVZ_CMAP_RDPU
DVZ_CMAP_REDS
DVZ_CMAP_YLGN
DVZ_CMAP_YLGNBU
DVZ_CMAP_YLORBR
DVZ_CMAP_YLORRD
DVZ_CMAP_AFMHOT
DVZ_CMAP_AUTUMN
DVZ_CMAP_BONE
DVZ_CMAP_COOL
DVZ_CMAP_COPPER
DVZ_CMAP_GIST_HEAT
DVZ_CMAP_GRAY
DVZ_CMAP_HOT
DVZ_CMAP_PINK
DVZ_CMAP_SPRING
DVZ_CMAP_SUMMER
DVZ_CMAP_WINTER
DVZ_CMAP_WISTIA
DVZ_CMAP_BRBG
DVZ_CMAP_BWR
DVZ_CMAP_COOLWARM
DVZ_CMAP_PIYG
DVZ_CMAP_PRGN
DVZ_CMAP_PUOR
DVZ_CMAP_RDBU
DVZ_CMAP_RDGY
DVZ_CMAP_RDYLBU
DVZ_CMAP_RDYLGN
DVZ_CMAP_SEISMIC
DVZ_CMAP_SPECTRAL
DVZ_CMAP_TWILIGHT_SHIFTED
DVZ_CMAP_TWILIGHT
DVZ_CMAP_BRG
DVZ_CMAP_CMRMAP
DVZ_CMAP_CUBEHELIX
DVZ_CMAP_FLAG
DVZ_CMAP_GIST_EARTH
DVZ_CMAP_GIST_NCAR
DVZ_CMAP_GIST_RAINBOW
DVZ_CMAP_GIST_STERN
DVZ_CMAP_GNUPLOT2
DVZ_CMAP_GNUPLOT
DVZ_CMAP_JET
DVZ_CMAP_NIPY_SPECTRAL
DVZ_CMAP_OCEAN
DVZ_CMAP_PRISM
DVZ_CMAP_RAINBOW
DVZ_CMAP_TERRAIN
DVZ_CMAP_BKR
DVZ_CMAP_BKY
DVZ_CMAP_CET_D10
DVZ_CMAP_CET_D11
DVZ_CMAP_CET_D8
DVZ_CMAP_CET_D13
DVZ_CMAP_CET_D3
DVZ_CMAP_CET_D1A
DVZ_CMAP_BJY
DVZ_CMAP_GWV
DVZ_CMAP_BWY
DVZ_CMAP_CET_D12
DVZ_CMAP_CET_R3
DVZ_CMAP_CET_D9
DVZ_CMAP_CWR
DVZ_CMAP_CET_CBC1
DVZ_CMAP_CET_CBC2
DVZ_CMAP_CET_CBL1
DVZ_CMAP_CET_CBL2
DVZ_CMAP_CET_CBTC1
DVZ_CMAP_CET_CBTC2
DVZ_CMAP_CET_CBTL1
DVZ_CMAP_BGY
DVZ_CMAP_BGYW
DVZ_CMAP_BMW
DVZ_CMAP_CET_C1
DVZ_CMAP_CET_C1S
DVZ_CMAP_CET_C2
DVZ_CMAP_CET_C4
DVZ_CMAP_CET_C4S
DVZ_CMAP_CET_C5
DVZ_CMAP_CET_I1
DVZ_CMAP_CET_I3
DVZ_CMAP_CET_L10
DVZ_CMAP_CET_L11
DVZ_CMAP_CET_L12
DVZ_CMAP_CET_L16
DVZ_CMAP_CET_L17
DVZ_CMAP_CET_L18
DVZ_CMAP_CET_L19
DVZ_CMAP_CET_L4
DVZ_CMAP_CET_L7
DVZ_CMAP_CET_L8
DVZ_CMAP_CET_L9
DVZ_CMAP_CET_R1
DVZ_CMAP_CET_R2
DVZ_CMAP_COLORWHEEL
DVZ_CMAP_FIRE
DVZ_CMAP_ISOLUM
DVZ_CMAP_KB
DVZ_CMAP_KBC
DVZ_CMAP_KG
DVZ_CMAP_KGY
DVZ_CMAP_KR
DVZ_CMAP_BLACK_BODY
DVZ_CMAP_KINDLMANN
DVZ_CMAP_EXTENDED_KINDLMANN
DVZ_CPAL256_GLASBEY
DVZ_CPAL256_GLASBEY_COOL
DVZ_CPAL256_GLASBEY_DARK
DVZ_CPAL256_GLASBEY_HV
DVZ_CPAL256_GLASBEY_LIGHT
DVZ_CPAL256_GLASBEY_WARM
DVZ_CPAL032_ACCENT
DVZ_CPAL032_DARK2
DVZ_CPAL032_PAIRED
DVZ_CPAL032_PASTEL1
DVZ_CPAL032_PASTEL2
DVZ_CPAL032_SET1
DVZ_CPAL032_SET2
DVZ_CPAL032_SET3
DVZ_CPAL032_TAB10
DVZ_CPAL032_TAB20
DVZ_CPAL032_TAB20B
DVZ_CPAL032_TAB20C
DVZ_CPAL032_CATEGORY10_10
DVZ_CPAL032_CATEGORY20_20
DVZ_CPAL032_CATEGORY20B_20
DVZ_CPAL032_CATEGORY20C_20
DVZ_CPAL032_COLORBLIND8
```

### `DvzContourFlags`

```
DVZ_CONTOUR_NONE
DVZ_CONTOUR_EDGES
DVZ_CONTOUR_JOINTS
DVZ_CONTOUR_FULL
```

### `DvzCorner`

```
DVZ_DIALOG_CORNER_TOP_LEFT
DVZ_DIALOG_CORNER_TOP_RIGHT
DVZ_DIALOG_CORNER_BOTTOM_LEFT
DVZ_DIALOG_CORNER_BOTTOM_RIGHT
```

### `DvzDatFlags`

```
DVZ_DAT_FLAGS_NONE
DVZ_DAT_FLAGS_STANDALONE
DVZ_DAT_FLAGS_MAPPABLE
DVZ_DAT_FLAGS_DUP
DVZ_DAT_FLAGS_KEEP_ON_RESIZE
DVZ_DAT_FLAGS_PERSISTENT_STAGING
```

### `DvzDepthTest`

```
DVZ_DEPTH_TEST_DISABLE
DVZ_DEPTH_TEST_ENABLE
```

### `DvzDialogFlags`

```
DVZ_DIALOG_FLAGS_NONE
DVZ_DIALOG_FLAGS_OVERLAY
```

### `DvzEasing`

```
DVZ_EASING_NONE
DVZ_EASING_IN_SINE
DVZ_EASING_OUT_SINE
DVZ_EASING_IN_OUT_SINE
DVZ_EASING_IN_QUAD
DVZ_EASING_OUT_QUAD
DVZ_EASING_IN_OUT_QUAD
DVZ_EASING_IN_CUBIC
DVZ_EASING_OUT_CUBIC
DVZ_EASING_IN_OUT_CUBIC
DVZ_EASING_IN_QUART
DVZ_EASING_OUT_QUART
DVZ_EASING_IN_OUT_QUART
DVZ_EASING_IN_QUINT
DVZ_EASING_OUT_QUINT
DVZ_EASING_IN_OUT_QUINT
DVZ_EASING_IN_EXPO
DVZ_EASING_OUT_EXPO
DVZ_EASING_IN_OUT_EXPO
DVZ_EASING_IN_CIRC
DVZ_EASING_OUT_CIRC
DVZ_EASING_IN_OUT_CIRC
DVZ_EASING_IN_BACK
DVZ_EASING_OUT_BACK
DVZ_EASING_IN_OUT_BACK
DVZ_EASING_IN_ELASTIC
DVZ_EASING_OUT_ELASTIC
DVZ_EASING_IN_OUT_ELASTIC
DVZ_EASING_IN_BOUNCE
DVZ_EASING_OUT_BOUNCE
DVZ_EASING_IN_OUT_BOUNCE
DVZ_EASING_COUNT
```

### `DvzFilter`

```
DVZ_FILTER_NEAREST
DVZ_FILTER_LINEAR
DVZ_FILTER_CUBIC_IMG
```

### `DvzFontFlags`

```
DVZ_FONT_FLAGS_RGB
DVZ_FONT_FLAGS_RGBA
```

### `DvzFormat`

```
DVZ_FORMAT_NONE
DVZ_FORMAT_R8_UNORM
DVZ_FORMAT_R8_SNORM
DVZ_FORMAT_R8_UINT
DVZ_FORMAT_R8_SINT
DVZ_FORMAT_R8G8_UNORM
DVZ_FORMAT_R8G8_SNORM
DVZ_FORMAT_R8G8_UINT
DVZ_FORMAT_R8G8_SINT
DVZ_FORMAT_R8G8B8_UNORM
DVZ_FORMAT_R8G8B8_SNORM
DVZ_FORMAT_R8G8B8_UINT
DVZ_FORMAT_R8G8B8_SINT
DVZ_FORMAT_B8G8R8_UNORM
DVZ_FORMAT_B8G8R8_SNORM
DVZ_FORMAT_R8G8B8A8_UNORM
DVZ_FORMAT_R8G8B8A8_SNORM
DVZ_FORMAT_R8G8B8A8_UINT
DVZ_FORMAT_R8G8B8A8_SINT
DVZ_FORMAT_B8G8R8A8_UNORM
DVZ_FORMAT_R16_UNORM
DVZ_FORMAT_R16_SNORM
DVZ_FORMAT_R32_UINT
DVZ_FORMAT_R32_SINT
DVZ_FORMAT_R32_SFLOAT
DVZ_FORMAT_R32G32_UINT
DVZ_FORMAT_R32G32_SINT
DVZ_FORMAT_R32G32_SFLOAT
DVZ_FORMAT_R32G32B32_UINT
DVZ_FORMAT_R32G32B32_SINT
DVZ_FORMAT_R32G32B32_SFLOAT
DVZ_FORMAT_R32G32B32A32_UINT
DVZ_FORMAT_R32G32B32A32_SINT
DVZ_FORMAT_R32G32B32A32_SFLOAT
```

### `DvzImageFlags`

```
DVZ_IMAGE_FLAGS_SIZE_PIXELS
DVZ_IMAGE_FLAGS_SIZE_NDC
DVZ_IMAGE_FLAGS_RESCALE_KEEP_RATIO
DVZ_IMAGE_FLAGS_RESCALE
DVZ_IMAGE_FLAGS_FILL
```

### `DvzJoinType`

```
DVZ_JOIN_SQUARE
DVZ_JOIN_ROUND
```

### `DvzKeyboardEventType`

```
DVZ_KEYBOARD_EVENT_NONE
DVZ_KEYBOARD_EVENT_PRESS
DVZ_KEYBOARD_EVENT_REPEAT
DVZ_KEYBOARD_EVENT_RELEASE
```

### `DvzKeyboardModifiers`

```
DVZ_KEY_MODIFIER_NONE
DVZ_KEY_MODIFIER_SHIFT
DVZ_KEY_MODIFIER_CONTROL
DVZ_KEY_MODIFIER_ALT
DVZ_KEY_MODIFIER_SUPER
```

### `DvzMarkerAspect`

```
DVZ_MARKER_ASPECT_FILLED
DVZ_MARKER_ASPECT_STROKE
DVZ_MARKER_ASPECT_OUTLINE
```

### `DvzMarkerMode`

```
DVZ_MARKER_MODE_NONE
DVZ_MARKER_MODE_CODE
DVZ_MARKER_MODE_BITMAP
DVZ_MARKER_MODE_SDF
DVZ_MARKER_MODE_MSDF
DVZ_MARKER_MODE_MTSDF
```

### `DvzMarkerShape`

```
DVZ_MARKER_SHAPE_DISC
DVZ_MARKER_SHAPE_ASTERISK
DVZ_MARKER_SHAPE_CHEVRON
DVZ_MARKER_SHAPE_CLOVER
DVZ_MARKER_SHAPE_CLUB
DVZ_MARKER_SHAPE_CROSS
DVZ_MARKER_SHAPE_DIAMOND
DVZ_MARKER_SHAPE_ARROW
DVZ_MARKER_SHAPE_ELLIPSE
DVZ_MARKER_SHAPE_HBAR
DVZ_MARKER_SHAPE_HEART
DVZ_MARKER_SHAPE_INFINITY
DVZ_MARKER_SHAPE_PIN
DVZ_MARKER_SHAPE_RING
DVZ_MARKER_SHAPE_SPADE
DVZ_MARKER_SHAPE_SQUARE
DVZ_MARKER_SHAPE_TAG
DVZ_MARKER_SHAPE_TRIANGLE
DVZ_MARKER_SHAPE_VBAR
DVZ_MARKER_SHAPE_ROUNDED_RECT
DVZ_MARKER_SHAPE_COUNT
```

### `DvzMeshFlags`

```
DVZ_MESH_FLAGS_NONE
DVZ_MESH_FLAGS_TEXTURED
DVZ_MESH_FLAGS_LIGHTING
DVZ_MESH_FLAGS_CONTOUR
DVZ_MESH_FLAGS_ISOLINE
```

### `DvzMockFlags`

```
DVZ_MOCK_FLAGS_NONE
DVZ_MOCK_FLAGS_CLOSED
```

### `DvzMouseButton`

```
DVZ_MOUSE_BUTTON_NONE
DVZ_MOUSE_BUTTON_LEFT
DVZ_MOUSE_BUTTON_MIDDLE
DVZ_MOUSE_BUTTON_RIGHT
```

### `DvzMouseEventType`

```
DVZ_MOUSE_EVENT_RELEASE
DVZ_MOUSE_EVENT_PRESS
DVZ_MOUSE_EVENT_MOVE
DVZ_MOUSE_EVENT_CLICK
DVZ_MOUSE_EVENT_DOUBLE_CLICK
DVZ_MOUSE_EVENT_DRAG_START
DVZ_MOUSE_EVENT_DRAG
DVZ_MOUSE_EVENT_DRAG_STOP
DVZ_MOUSE_EVENT_WHEEL
DVZ_MOUSE_EVENT_ALL
```

### `DvzMouseState`

```
DVZ_MOUSE_STATE_RELEASE
DVZ_MOUSE_STATE_PRESS
DVZ_MOUSE_STATE_CLICK
DVZ_MOUSE_STATE_CLICK_PRESS
DVZ_MOUSE_STATE_DOUBLE_CLICK
DVZ_MOUSE_STATE_DRAGGING
```

### `DvzPanzoomFlags`

```
DVZ_PANZOOM_FLAGS_NONE
DVZ_PANZOOM_FLAGS_KEEP_ASPECT
DVZ_PANZOOM_FLAGS_FIXED_X
DVZ_PANZOOM_FLAGS_FIXED_Y
```

### `DvzPathFlags`

```
DVZ_PATH_FLAGS_OPEN
DVZ_PATH_FLAGS_CLOSED
```

### `DvzPrimitiveTopology`

```
DVZ_PRIMITIVE_TOPOLOGY_POINT_LIST
DVZ_PRIMITIVE_TOPOLOGY_LINE_LIST
DVZ_PRIMITIVE_TOPOLOGY_LINE_STRIP
DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP
DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN
```

### `DvzSamplerAddressMode`

```
DVZ_SAMPLER_ADDRESS_MODE_REPEAT
DVZ_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT
DVZ_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
DVZ_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER
DVZ_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE
```

### `DvzShapeType`

```
DVZ_SHAPE_NONE
DVZ_SHAPE_SQUARE
DVZ_SHAPE_DISC
DVZ_SHAPE_POLYGON
DVZ_SHAPE_CUBE
DVZ_SHAPE_SPHERE
DVZ_SHAPE_CYLINDER
DVZ_SHAPE_CONE
DVZ_SHAPE_SURFACE
DVZ_SHAPE_OBJ
DVZ_SHAPE_OTHER
```

### `DvzTexFlags`

```
DVZ_TEX_FLAGS_NONE
DVZ_TEX_FLAGS_PERSISTENT_STAGING
```

### `DvzUploadFlags`

```
DVZ_UPLOAD_FLAGS_NOCOPY
```

### `DvzViewFlags`

```
DVZ_VIEW_FLAGS_NONE
DVZ_VIEW_FLAGS_STATIC
```

### `DvzViewportClip`

```
DVZ_VIEWPORT_CLIP_INNER
DVZ_VIEWPORT_CLIP_OUTER
DVZ_VIEWPORT_CLIP_BOTTOM
DVZ_VIEWPORT_CLIP_LEFT
```

### `DvzVisualFlags`

```
DVZ_VISUAL_FLAGS_DEFAULT
DVZ_VISUAL_FLAGS_INDEXED
DVZ_VISUAL_FLAGS_INDIRECT
DVZ_VISUAL_FLAGS_VERTEX_MAPPABLE
DVZ_VISUAL_FLAGS_INDEX_MAPPABLE
```

### `DvzVolumeFlags`

```
DVZ_VOLUME_FLAGS_NONE
DVZ_VOLUME_FLAGS_RGBA
DVZ_VOLUME_FLAGS_COLORMAP
DVZ_VOLUME_FLAGS_BACK_FRONT
```

### `DvzKeyCode`

```
DVZ_KEY_UNKNOWN
DVZ_KEY_NONE
DVZ_KEY_SPACE
DVZ_KEY_APOSTROPHE
DVZ_KEY_COMMA
DVZ_KEY_MINUS
DVZ_KEY_PERIOD
DVZ_KEY_SLASH
DVZ_KEY_0
DVZ_KEY_1
DVZ_KEY_2
DVZ_KEY_3
DVZ_KEY_4
DVZ_KEY_5
DVZ_KEY_6
DVZ_KEY_7
DVZ_KEY_8
DVZ_KEY_9
DVZ_KEY_SEMICOLON
DVZ_KEY_EQUAL
DVZ_KEY_A
DVZ_KEY_B
DVZ_KEY_C
DVZ_KEY_D
DVZ_KEY_E
DVZ_KEY_F
DVZ_KEY_G
DVZ_KEY_H
DVZ_KEY_I
DVZ_KEY_J
DVZ_KEY_K
DVZ_KEY_L
DVZ_KEY_M
DVZ_KEY_N
DVZ_KEY_O
DVZ_KEY_P
DVZ_KEY_Q
DVZ_KEY_R
DVZ_KEY_S
DVZ_KEY_T
DVZ_KEY_U
DVZ_KEY_V
DVZ_KEY_W
DVZ_KEY_X
DVZ_KEY_Y
DVZ_KEY_Z
DVZ_KEY_LEFT_BRACKET
DVZ_KEY_BACKSLASH
DVZ_KEY_RIGHT_BRACKET
DVZ_KEY_GRAVE_ACCENT
DVZ_KEY_WORLD_1
DVZ_KEY_WORLD_2
DVZ_KEY_ESCAPE
DVZ_KEY_ENTER
DVZ_KEY_TAB
DVZ_KEY_BACKSPACE
DVZ_KEY_INSERT
DVZ_KEY_DELETE
DVZ_KEY_RIGHT
DVZ_KEY_LEFT
DVZ_KEY_DOWN
DVZ_KEY_UP
DVZ_KEY_PAGE_UP
DVZ_KEY_PAGE_DOWN
DVZ_KEY_HOME
DVZ_KEY_END
DVZ_KEY_CAPS_LOCK
DVZ_KEY_SCROLL_LOCK
DVZ_KEY_NUM_LOCK
DVZ_KEY_PRINT_SCREEN
DVZ_KEY_PAUSE
DVZ_KEY_F1
DVZ_KEY_F2
DVZ_KEY_F3
DVZ_KEY_F4
DVZ_KEY_F5
DVZ_KEY_F6
DVZ_KEY_F7
DVZ_KEY_F8
DVZ_KEY_F9
DVZ_KEY_F10
DVZ_KEY_F11
DVZ_KEY_F12
DVZ_KEY_F13
DVZ_KEY_F14
DVZ_KEY_F15
DVZ_KEY_F16
DVZ_KEY_F17
DVZ_KEY_F18
DVZ_KEY_F19
DVZ_KEY_F20
DVZ_KEY_F21
DVZ_KEY_F22
DVZ_KEY_F23
DVZ_KEY_F24
DVZ_KEY_F25
DVZ_KEY_KP_0
DVZ_KEY_KP_1
DVZ_KEY_KP_2
DVZ_KEY_KP_3
DVZ_KEY_KP_4
DVZ_KEY_KP_5
DVZ_KEY_KP_6
DVZ_KEY_KP_7
DVZ_KEY_KP_8
DVZ_KEY_KP_9
DVZ_KEY_KP_DECIMAL
DVZ_KEY_KP_DIVIDE
DVZ_KEY_KP_MULTIPLY
DVZ_KEY_KP_SUBTRACT
DVZ_KEY_KP_ADD
DVZ_KEY_KP_ENTER
DVZ_KEY_KP_EQUAL
DVZ_KEY_LEFT_SHIFT
DVZ_KEY_LEFT_CONTROL
DVZ_KEY_LEFT_ALT
DVZ_KEY_LEFT_SUPER
DVZ_KEY_RIGHT_SHIFT
DVZ_KEY_RIGHT_CONTROL
DVZ_KEY_RIGHT_ALT
DVZ_KEY_RIGHT_SUPER
DVZ_KEY_MENU
DVZ_KEY_LAST
```

## Structures

### `DvzAtlasFont`

```
struct DvzAtlasFont
    unsigned long ttf_size
    unsigned char* ttf_bytes
    DvzAtlas* atlas
    DvzFont* font
```

### `DvzFrameEvent`

```
struct DvzFrameEvent
    uint64_t frame_idx
    double time
    double interval
    void* user_data
```

### `DvzGuiEvent`

```
struct DvzGuiEvent
    void* user_data
```

### `DvzKeyboardEvent`

```
struct DvzKeyboardEvent
    DvzKeyboardEventType type
    DvzKeyCode key
    int mods
    void* user_data
```

### `DvzMVP`

```
struct DvzMVP
    mat4 model
    mat4 view
    mat4 proj
```

### `DvzMouseButtonEvent`

```
struct DvzMouseButtonEvent
    DvzMouseButton button
```

### `DvzMouseClickEvent`

```
struct DvzMouseClickEvent
    DvzMouseButton button
```

### `DvzMouseDragEvent`

```
struct DvzMouseDragEvent
    DvzMouseButton button
    vec2 press_pos
    vec2 shift
```

### `DvzMouseEvent`

```
struct DvzMouseEvent
    DvzMouseEventType type
    DvzMouseEventUnion content
    vec2 pos
    int mods
    float content_scale
    void* user_data
```

### `DvzMouseEventUnion`

```
union DvzMouseEventUnion
    DvzMouseButtonEvent b
    DvzMouseWheelEvent w
    DvzMouseDragEvent d
    DvzMouseClickEvent c
```

### `DvzMouseWheelEvent`

```
struct DvzMouseWheelEvent
    vec2 dir
```

### `DvzRequestsEvent`

```
struct DvzRequestsEvent
    DvzBatch* batch
    void* user_data
```

### `DvzShape`

```
struct DvzShape
    mat4 transform
    uint32_t first
    uint32_t count
    DvzShapeType type
    uint32_t vertex_count
    uint32_t index_count
    vec3* pos
    vec3* normal
    cvec4* color
    vec4* texcoords
    float* isoline
    vec3* d_left
    vec3* d_right
    cvec3* contour
    DvzIndex* index
    double _
```

### `DvzTimerEvent`

```
struct DvzTimerEvent
    uint32_t timer_idx
    DvzTimerItem* timer_item
    uint64_t step_idx
    double time
    void* user_data
```

### `DvzWindowEvent`

```
struct DvzWindowEvent
    uint32_t framebuffer_width
    uint32_t framebuffer_height
    uint32_t screen_width
    uint32_t screen_height
    int flags
    void* user_data
```

