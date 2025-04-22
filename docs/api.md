# API Reference

## Main functions

### `dvz_arcball_angles()`

Get the current arcball angles.

=== "Python"

    ``` python
    dvz.arcball_angles(
        arcball,  # the arcball (LP_DvzArcball)
    )
    ```

=== "C"

    ``` c
    void dvz_arcball_angles(
        DvzArcball* arcball,  // the arcball
    );
    ```

### `dvz_arcball_constrain()`

Add arcball constraints.

=== "Python"

    ``` python
    dvz.arcball_constrain(
        arcball,  # the arcball (LP_DvzArcball)
        constrain,  # the constrain values (vec3)
    )
    ```

=== "C"

    ``` c
    void dvz_arcball_constrain(
        DvzArcball* arcball,  // the arcball
        vec3 constrain,  // the constrain values
    );
    ```

### `dvz_arcball_end()`

Finalize arcball position update.

=== "Python"

    ``` python
    dvz.arcball_end(
        arcball,  # the arcball (LP_DvzArcball)
    )
    ```

=== "C"

    ``` c
    void dvz_arcball_end(
        DvzArcball* arcball,  // the arcball
    );
    ```

### `dvz_arcball_flags()`

Set the arcball flags.

=== "Python"

    ``` python
    dvz.arcball_flags(
        arcball,  # the arcball (LP_DvzArcball)
        flags,  # the flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    void dvz_arcball_flags(
        DvzArcball* arcball,  // the arcball
        int flags,  // the flags
    );
    ```

### `dvz_arcball_gui()`

Show a GUI with sliders controlling the three arcball angles.

=== "Python"

    ``` python
    dvz.arcball_gui(
        arcball,  # the arcball (LP_DvzArcball)
        app,  # the app (LP_DvzApp)
        canvas_id,  # the canvas (or figure) ID (int, 64-bit unsigned)
        panel,  # the panel (LP_DvzPanel)
    )
    ```

=== "C"

    ``` c
    void dvz_arcball_gui(
        DvzArcball* arcball,  // the arcball
        DvzApp* app,  // the app
        DvzId canvas_id,  // the canvas (or figure) ID
        DvzPanel* panel,  // the panel
    );
    ```

### `dvz_arcball_initial()`

Set the initial arcball angles.

=== "Python"

    ``` python
    dvz.arcball_initial(
        arcball,  # the arcball (LP_DvzArcball)
        angles,  # the initial angles (vec3)
    )
    ```

=== "C"

    ``` c
    void dvz_arcball_initial(
        DvzArcball* arcball,  // the arcball
        vec3 angles,  // the initial angles
    );
    ```

### `dvz_arcball_model()`

Return the model matrix of an arcball.

=== "Python"

    ``` python
    dvz.arcball_model(
        arcball,  # the arcball (LP_DvzArcball)
    )
    ```

=== "C"

    ``` c
    void dvz_arcball_model(
        DvzArcball* arcball,  // the arcball
    );
    ```

### `dvz_arcball_mvp()`

Apply an MVP matrix to an arcball (only the model matrix).

=== "Python"

    ``` python
    dvz.arcball_mvp(
        arcball,  # the arcball (LP_DvzArcball)
        mvp,  # the MVP (LP_DvzMVP)
    )
    ```

=== "C"

    ``` c
    void dvz_arcball_mvp(
        DvzArcball* arcball,  // the arcball
        DvzMVP* mvp,  // the MVP
    );
    ```

### `dvz_arcball_print()`

Display information about an arcball.

=== "Python"

    ``` python
    dvz.arcball_print(
        arcball,  # the arcball (LP_DvzArcball)
    )
    ```

=== "C"

    ``` c
    void dvz_arcball_print(
        DvzArcball* arcball,  // the arcball
    );
    ```

### `dvz_arcball_reset()`

Reset an arcball to its initial position.

=== "Python"

    ``` python
    dvz.arcball_reset(
        arcball,  # the arcball (LP_DvzArcball)
    )
    ```

=== "C"

    ``` c
    void dvz_arcball_reset(
        DvzArcball* arcball,  // the arcball
    );
    ```

### `dvz_arcball_resize()`

Inform an arcball of a panel resize.

=== "Python"

    ``` python
    dvz.arcball_resize(
        arcball,  # the arcball (LP_DvzArcball)
        width,  # the panel width (float, 64-bit)
        height,  # the panel height (float, 64-bit)
    )
    ```

=== "C"

    ``` c
    void dvz_arcball_resize(
        DvzArcball* arcball,  // the arcball
        float width,  // the panel width
        float height,  // the panel height
    );
    ```

### `dvz_arcball_rotate()`

Apply a rotation to an arcball.

=== "Python"

    ``` python
    dvz.arcball_rotate(
        arcball,  # the arcball (LP_DvzArcball)
        cur_pos,  # the initial position (vec2)
        last_pos,  # the final position (vec2)
    )
    ```

=== "C"

    ``` c
    void dvz_arcball_rotate(
        DvzArcball* arcball,  // the arcball
        vec2 cur_pos,  // the initial position
        vec2 last_pos,  // the final position
    );
    ```

### `dvz_arcball_set()`

Set the arcball angles.

=== "Python"

    ``` python
    dvz.arcball_set(
        arcball,  # the arcball (LP_DvzArcball)
        angles,  # the angles (vec3)
    )
    ```

=== "C"

    ``` c
    void dvz_arcball_set(
        DvzArcball* arcball,  // the arcball
        vec3 angles,  // the angles
    );
    ```

### `dvz_atlas_destroy()`

Destroy an atlas.

=== "Python"

    ``` python
    dvz.atlas_destroy(
        atlas,  # the atlas (LP_DvzAtlas)
    )
    ```

=== "C"

    ``` c
    void dvz_atlas_destroy(
        DvzAtlas* atlas,  // the atlas
    );
    ```

### `dvz_atlas_font()`

Load the default atlas and font.

=== "Python"

    ``` python
    dvz.atlas_font(  # returns: a DvzAtlasFont struct with DvzAtlas and DvzFont objects. (DvzAtlasFont)
        font_size,  # the font size (float, 32-bit)
    )
    ```

=== "C"

    ``` c
    DvzAtlasFont dvz_atlas_font(  // returns: a DvzAtlasFont struct with DvzAtlas and DvzFont objects.
        double font_size,  // the font size
    );
    ```

### `dvz_basic()`

Create a basic visual using the few GPU visual primitives (point, line, triangles).

=== "Python"

    ``` python
    dvz.basic(  # returns: the visual (LP_DvzVisual)
        batch,  # the batch (LP_DvzBatch)
        topology,  # the primitive topology (DvzPrimitiveTopology)
        flags,  # the visual creation flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    DvzVisual* dvz_basic(  // returns: the visual
        DvzBatch* batch,  // the batch
        DvzPrimitiveTopology topology,  // the primitive topology
        int flags,  // the visual creation flags
    );
    ```

### `dvz_basic_alloc()`

Allocate memory for a visual.

=== "Python"

    ``` python
    dvz.basic_alloc(
        visual,  # the visual (LP_DvzVisual)
        item_count,  # the total number of items to allocate for this visual (int, 32-bit unsigned)
    )
    ```

=== "C"

    ``` c
    void dvz_basic_alloc(
        DvzVisual* visual,  // the visual
        uint32_t item_count,  // the total number of items to allocate for this visual
    );
    ```

### `dvz_basic_color()`

Set the vertex colors.

=== "Python"

    ``` python
    dvz.basic_color(
        visual,  # the visual (LP_DvzVisual)
        first,  # the index of the first item to update (int, 32-bit unsigned)
        count,  # the number of items to update (int, 32-bit unsigned)
        values,  # the colors of the items to update (ndpointer_|u1_C_CONTIGUOUS)
        flags,  # the data update flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    void dvz_basic_color(
        DvzVisual* visual,  // the visual
        uint32_t first,  // the index of the first item to update
        uint32_t count,  // the number of items to update
        DvzColor* values,  // the colors of the items to update
        int flags,  // the data update flags
    );
    ```

### `dvz_basic_group()`

Set the vertex group index.

=== "Python"

    ``` python
    dvz.basic_group(
        visual,  # the visual (LP_DvzVisual)
        first,  # the index of the first item to update (int, 32-bit unsigned)
        count,  # the number of items to update (int, 32-bit unsigned)
        values,  # the group index of each vertex (ndpointer_<f4_C_CONTIGUOUS)
        flags,  # the data update flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    void dvz_basic_group(
        DvzVisual* visual,  // the visual
        uint32_t first,  // the index of the first item to update
        uint32_t count,  // the number of items to update
        float* values,  // the group index of each vertex
        int flags,  // the data update flags
    );
    ```

### `dvz_basic_position()`

Set the vertex positions.

=== "Python"

    ``` python
    dvz.basic_position(
        visual,  # the visual (LP_DvzVisual)
        first,  # the index of the first item to update (int, 32-bit unsigned)
        count,  # the number of items to update (int, 32-bit unsigned)
        values,  # the 3D positions of the items to update (ndpointer_<f4_C_CONTIGUOUS)
        flags,  # the data update flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    void dvz_basic_position(
        DvzVisual* visual,  // the visual
        uint32_t first,  // the index of the first item to update
        uint32_t count,  // the number of items to update
        vec3* values,  // the 3D positions of the items to update
        int flags,  // the data update flags
    );
    ```

### `dvz_basic_size()`

Set the point size (for POINT_LIST topology only).

=== "Python"

    ``` python
    dvz.basic_size(
        visual,  # the visual (LP_DvzVisual)
        size,  # the point size in pixels (float, 64-bit)
    )
    ```

=== "C"

    ``` c
    void dvz_basic_size(
        DvzVisual* visual,  // the visual
        float size,  // the point size in pixels
    );
    ```

### `dvz_camera_initial()`

Set the initial camera parameters.

=== "Python"

    ``` python
    dvz.camera_initial(
        camera,  # the camera (LP_DvzCamera)
        pos,  # the initial position (vec3)
        lookat,  # the lookat position (vec3)
        up,  # the up vector (vec3)
    )
    ```

=== "C"

    ``` c
    void dvz_camera_initial(
        DvzCamera* camera,  // the camera
        vec3 pos,  // the initial position
        vec3 lookat,  // the lookat position
        vec3 up,  // the up vector
    );
    ```

### `dvz_camera_lookat()`

Set a camera lookat position.

=== "Python"

    ``` python
    dvz.camera_lookat(
        camera,  # the camera (LP_DvzCamera)
        lookat,  # the lookat position (vec3)
    )
    ```

=== "C"

    ``` c
    void dvz_camera_lookat(
        DvzCamera* camera,  // the camera
        vec3 lookat,  // the lookat position
    );
    ```

### `dvz_camera_mvp()`

Apply an MVP to a camera.

=== "Python"

    ``` python
    dvz.camera_mvp(
        camera,  # the camera (LP_DvzCamera)
        mvp,  # the MVP (LP_DvzMVP)
    )
    ```

=== "C"

    ``` c
    void dvz_camera_mvp(
        DvzCamera* camera,  // the camera
        DvzMVP* mvp,  // the MVP
    );
    ```

### `dvz_camera_ortho()`

Make an orthographic camera.

=== "Python"

    ``` python
    dvz.camera_ortho(
        camera,  # the camera (LP_DvzCamera)
        left,  # the left value (float, 64-bit)
        right,  # the right value (float, 64-bit)
        bottom,  # the bottom value (float, 64-bit)
        top,  # the top value (float, 64-bit)
    )
    ```

=== "C"

    ``` c
    void dvz_camera_ortho(
        DvzCamera* camera,  // the camera
        float left,  // the left value
        float right,  // the right value
        float bottom,  // the bottom value
        float top,  // the top value
    );
    ```

### `dvz_camera_perspective()`

Set a camera perspective.

=== "Python"

    ``` python
    dvz.camera_perspective(
        camera,  # the camera (LP_DvzCamera)
        fov,  # the field of view angle (in radians) (float, 64-bit)
    )
    ```

=== "C"

    ``` c
    void dvz_camera_perspective(
        DvzCamera* camera,  // the camera
        float fov,  // the field of view angle (in radians)
    );
    ```

### `dvz_camera_position()`

Set a camera position.

=== "Python"

    ``` python
    dvz.camera_position(
        camera,  # the camera (LP_DvzCamera)
        pos,  # the pos (vec3)
    )
    ```

=== "C"

    ``` c
    void dvz_camera_position(
        DvzCamera* camera,  // the camera
        vec3 pos,  // the pos
    );
    ```

### `dvz_camera_print()`

Display information about a camera.

=== "Python"

    ``` python
    dvz.camera_print(
        camera,  # the camera (LP_DvzCamera)
    )
    ```

=== "C"

    ``` c
    void dvz_camera_print(
        DvzCamera* camera,  // the camera
    );
    ```

### `dvz_camera_reset()`

Reset a camera.

=== "Python"

    ``` python
    dvz.camera_reset(
        camera,  # the camera (LP_DvzCamera)
    )
    ```

=== "C"

    ``` c
    void dvz_camera_reset(
        DvzCamera* camera,  // the camera
    );
    ```

### `dvz_camera_resize()`

Inform a camera of a panel resize.

=== "Python"

    ``` python
    dvz.camera_resize(
        camera,  # the camera (LP_DvzCamera)
        width,  # the panel width (float, 64-bit)
        height,  # the panel height (float, 64-bit)
    )
    ```

=== "C"

    ``` c
    void dvz_camera_resize(
        DvzCamera* camera,  // the camera
        float width,  // the panel width
        float height,  // the panel height
    );
    ```

### `dvz_camera_up()`

Set a camera up vector.

=== "Python"

    ``` python
    dvz.camera_up(
        camera,  # the camera (LP_DvzCamera)
        up,  # the up vector (vec3)
    )
    ```

=== "C"

    ``` c
    void dvz_camera_up(
        DvzCamera* camera,  // the camera
        vec3 up,  // the up vector
    );
    ```

### `dvz_camera_viewproj()`

Return the view and proj matrices of the camera.

=== "Python"

    ``` python
    dvz.camera_viewproj(
        camera,  # the camera (LP_DvzCamera)
    )
    ```

=== "C"

    ``` c
    void dvz_camera_viewproj(
        DvzCamera* camera,  // the camera
    );
    ```

### `dvz_camera_zrange()`

Set the camera zrange.

=== "Python"

    ``` python
    dvz.camera_zrange(
        camera,  # the camera (LP_DvzCamera)
        near,  # the near value (float, 64-bit)
        far,  # the far value (float, 64-bit)
    )
    ```

=== "C"

    ``` c
    void dvz_camera_zrange(
        DvzCamera* camera,  // the camera
        float near,  // the near value
        float far,  // the far value
    );
    ```

### `dvz_circular_2D()`

Generate a 2D circular motion.

=== "Python"

    ``` python
    dvz.circular_2D(
        center,  # the circle center (vec2)
        radius,  # the circle radius (float, 64-bit)
        angle,  # the initial angle (float, 64-bit)
        t,  # the normalized value (float, 64-bit)
    )
    ```

=== "C"

    ``` c
    void dvz_circular_2D(
        vec2 center,  // the circle center
        float radius,  // the circle radius
        float angle,  // the initial angle
        float t,  // the normalized value
    );
    ```

### `dvz_circular_3D()`

Generate a 3D circular motion.

=== "Python"

    ``` python
    dvz.circular_3D(
        center,  # the circle center (vec3)
        u,  # the first 3D vector defining the plane containing the circle (vec3)
        v,  # the second 3D vector defining the plane containing the circle (vec3)
        radius,  # the circle radius (float, 64-bit)
        angle,  # the initial angle (float, 64-bit)
        t,  # the normalized value (float, 64-bit)
    )
    ```

=== "C"

    ``` c
    void dvz_circular_3D(
        vec3 center,  // the circle center
        vec3 u,  // the first 3D vector defining the plane containing the circle
        vec3 v,  // the second 3D vector defining the plane containing the circle
        float radius,  // the circle radius
        float angle,  // the initial angle
        float t,  // the normalized value
    );
    ```

### `dvz_colormap()`

Fetch a color from a colormap and a value (either 8-bit or float, depending on DVZ_COLOR_CVEC4).

=== "Python"

    ``` python
    dvz.colormap(
        cmap,  # the colormap (DvzColormap)
        value,  # the value (int, 8-bit unsigned)
    )
    ```

=== "C"

    ``` c
    void dvz_colormap(
        DvzColormap cmap,  // the colormap
        uint8_t value,  // the value
    );
    ```

### `dvz_colormap_8bit()`

Fetch a color from a colormap and a value (8-bit version).

=== "Python"

    ``` python
    dvz.colormap_8bit(
        cmap,  # the colormap (DvzColormap)
        value,  # the value (int, 8-bit unsigned)
    )
    ```

=== "C"

    ``` c
    void dvz_colormap_8bit(
        DvzColormap cmap,  // the colormap
        uint8_t value,  // the value
    );
    ```

### `dvz_colormap_array()`

Fetch colors from a colormap and an array of values.

=== "Python"

    ``` python
    dvz.colormap_array(
        cmap,  # the colormap (DvzColormap)
        count,  # the number of values (int, 32-bit unsigned)
        values,  # pointer to the array of float numbers (ndpointer_<f4_C_CONTIGUOUS)
        vmin,  # the minimum value (float, 64-bit)
        vmax,  # the maximum value (float, 64-bit)
    )
    ```

=== "C"

    ``` c
    void dvz_colormap_array(
        DvzColormap cmap,  // the colormap
        uint32_t count,  // the number of values
        float* values,  // pointer to the array of float numbers
        float vmin,  // the minimum value
        float vmax,  // the maximum value
    );
    ```

### `dvz_colormap_scale()`

Fetch a color from a colormap and an interpolated value.

=== "Python"

    ``` python
    dvz.colormap_scale(
        cmap,  # the colormap (DvzColormap)
        value,  # the value (float, 64-bit)
        vmin,  # the minimum value (float, 64-bit)
        vmax,  # the maximum value (float, 64-bit)
    )
    ```

=== "C"

    ``` c
    void dvz_colormap_scale(
        DvzColormap cmap,  // the colormap
        float value,  // the value
        float vmin,  // the minimum value
        float vmax,  // the maximum value
    );
    ```

### `dvz_compute_normals()`

Compute face normals.

=== "Python"

    ``` python
    dvz.compute_normals(
        vertex_count,  # number of vertices (int, 32-bit unsigned)
        index_count,  # number of indices (triple of the number of faces) (int, 32-bit unsigned)
        pos,  # array of vec3 positions (ndpointer_<f4_C_CONTIGUOUS)
        index,  # pos array of uint32_t indices (ndpointer_<u4_C_CONTIGUOUS)
    )
    ```

=== "C"

    ``` c
    void dvz_compute_normals(
        uint32_t vertex_count,  // number of vertices
        uint32_t index_count,  // number of indices (triple of the number of faces)
        vec3* pos,  // array of vec3 positions
        DvzIndex* index,  // pos array of uint32_t indices
    );
    ```

### `dvz_demo()`

Run a demo.

=== "Python"

    ``` python
    dvz.demo()
    ```

=== "C"

    ``` c
    void dvz_demo();
    ```

### `dvz_demo_panel_2D()`

Demo panel (random scatter plot).

=== "Python"

    ``` python
    dvz.demo_panel_2D(  # returns: the marker visual (LP_DvzVisual)
        panel,  # the panel (LP_DvzPanel)
    )
    ```

=== "C"

    ``` c
    DvzVisual* dvz_demo_panel_2D(  // returns: the marker visual
        DvzPanel* panel,  // the panel
    );
    ```

### `dvz_demo_panel_3D()`

Demo panel (random scatter plot).

=== "Python"

    ``` python
    dvz.demo_panel_3D(  # returns: the marker visual (LP_DvzVisual)
        panel,  # the panel (LP_DvzPanel)
    )
    ```

=== "C"

    ``` c
    DvzVisual* dvz_demo_panel_3D(  // returns: the marker visual
        DvzPanel* panel,  // the panel
    );
    ```

### `dvz_easing()`

Apply an easing function to a normalized value.

=== "Python"

    ``` python
    dvz.easing(  # returns: the eased value (c_double)
        easing,  # the easing mode (DvzEasing)
        t,  # the normalized value (float, 32-bit)
    )
    ```

=== "C"

    ``` c
    double dvz_easing(  // returns: the eased value
        DvzEasing easing,  // the easing mode
        double t,  // the normalized value
    );
    ```

### `dvz_error_callback()`

Register an error callback, a C function taking as input a string.

=== "Python"

    ``` python
    dvz.error_callback(
        cb,  # the error callback (CFunctionType)
    )
    ```

=== "C"

    ``` c
    void dvz_error_callback(
        DvzErrorCallback cb,  // the error callback
    );
    ```

### `dvz_figure()`

Create a figure, a desktop window with panels and visuals.

=== "Python"

    ``` python
    dvz.figure(  # returns: the figure (LP_DvzFigure)
        scene,  # the scene (LP_DvzScene)
        width,  # the window width (int, 32-bit unsigned)
        height,  # the window height (int, 32-bit unsigned)
        flags,  # the figure creation flags (not yet stabilized) (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    DvzFigure* dvz_figure(  // returns: the figure
        DvzScene* scene,  // the scene
        uint32_t width,  // the window width
        uint32_t height,  // the window height
        int flags,  // the figure creation flags (not yet stabilized)
    );
    ```

### `dvz_figure_destroy()`

Destroy a figure.

=== "Python"

    ``` python
    dvz.figure_destroy(
        figure,  # the figure (LP_DvzFigure)
    )
    ```

=== "C"

    ``` c
    void dvz_figure_destroy(
        DvzFigure* figure,  // the figure
    );
    ```

### `dvz_figure_id()`

Return a figure ID.

=== "Python"

    ``` python
    dvz.figure_id(  # returns: the figure ID (c_ulong)
        figure,  # the figure (LP_DvzFigure)
    )
    ```

=== "C"

    ``` c
    DvzId dvz_figure_id(  // returns: the figure ID
        DvzFigure* figure,  // the figure
    );
    ```

### `dvz_figure_resize()`

Resize a figure.

=== "Python"

    ``` python
    dvz.figure_resize(
        fig,  # the figure (LP_DvzFigure)
        width,  # the window width (int, 32-bit unsigned)
        height,  # the window height (int, 32-bit unsigned)
    )
    ```

=== "C"

    ``` c
    void dvz_figure_resize(
        DvzFigure* fig,  // the figure
        uint32_t width,  // the window width
        uint32_t height,  // the window height
    );
    ```

### `dvz_figure_update()`

Update a figure after the composition of the panels and visuals has changed.

=== "Python"

    ``` python
    dvz.figure_update(
        figure,  # the figure (LP_DvzFigure)
    )
    ```

=== "C"

    ``` c
    void dvz_figure_update(
        DvzFigure* figure,  // the figure
    );
    ```

### `dvz_font()`

Create a font.

=== "Python"

    ``` python
    dvz.font(  # returns: the font (LP_DvzFont)
        ttf_size,  # size in bytes of a TTF font raw buffer (int, 64-bit signed)
        ttf_bytes,  # TTF font raw buffer (CStringBuffer)
    )
    ```

=== "C"

    ``` c
    DvzFont* dvz_font(  // returns: the font
        long ttf_size,  // size in bytes of a TTF font raw buffer
        char* ttf_bytes,  // TTF font raw buffer
    );
    ```

### `dvz_font_ascii()`

Compute the shift of each glyph in an ASCII string, using the Freetype library.

=== "Python"

    ``` python
    dvz.font_ascii(  # returns: an array of (x,y,w,h) shifts (ndpointer_<f4_C_CONTIGUOUS)
        font,  # the font (LP_DvzFont)
        string,  # the ASCII string (CStringBuffer)
    )
    ```

=== "C"

    ``` c
    vec4* dvz_font_ascii(  // returns: an array of (x,y,w,h) shifts
        DvzFont* font,  // the font
        char* string,  // the ASCII string
    );
    ```

### `dvz_font_destroy()`

Destroy a font.

=== "Python"

    ``` python
    dvz.font_destroy(
        font,  # the font (LP_DvzFont)
    )
    ```

=== "C"

    ``` c
    void dvz_font_destroy(
        DvzFont* font,  // the font
    );
    ```

### `dvz_font_draw()`

Render a string using Freetype.

=== "Python"

    ``` python
    dvz.font_draw(  # returns: an RGBA array allocated by this function and that MUST be freed by the caller (ndpointer_|u1_C_CONTIGUOUS)
        font,  # the font (LP_DvzFont)
        length,  # the number of glyphs (int, 32-bit unsigned)
        codepoints,  # the Unicode codepoints of the glyphs (ndpointer_<u4_C_CONTIGUOUS)
        xywh,  # an array of (x,y,w,h) shifts, returned by dvz_font_layout() (ndpointer_<f4_C_CONTIGUOUS)
        flags,  # the font flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    uint8_t* dvz_font_draw(  // returns: an RGBA array allocated by this function and that MUST be freed by the caller
        DvzFont* font,  // the font
        uint32_t length,  // the number of glyphs
        uint32_t* codepoints,  // the Unicode codepoints of the glyphs
        vec4* xywh,  // an array of (x,y,w,h) shifts, returned by dvz_font_layout()
        int flags,  // the font flags
    );
    ```

### `dvz_font_layout()`

Compute the shift of each glyph in a Unicode string, using the Freetype library.

=== "Python"

    ``` python
    dvz.font_layout(  # returns: an array of (x,y,w,h) shifts (ndpointer_<f4_C_CONTIGUOUS)
        font,  # the font (LP_DvzFont)
        length,  # the number of glyphs (int, 32-bit unsigned)
        codepoints,  # the Unicode codepoints of the glyphs (ndpointer_<u4_C_CONTIGUOUS)
    )
    ```

=== "C"

    ``` c
    vec4* dvz_font_layout(  // returns: an array of (x,y,w,h) shifts
        DvzFont* font,  // the font
        uint32_t length,  // the number of glyphs
        uint32_t* codepoints,  // the Unicode codepoints of the glyphs
    );
    ```

### `dvz_font_size()`

Set the font size.

=== "Python"

    ``` python
    dvz.font_size(
        font,  # the font (LP_DvzFont)
        size,  # the font size (float, 32-bit)
    )
    ```

=== "C"

    ``` c
    void dvz_font_size(
        DvzFont* font,  // the font
        double size,  // the font size
    );
    ```

### `dvz_font_texture()`

Generate a texture with a rendered text.

=== "Python"

    ``` python
    dvz.font_texture(  # returns: a tex ID (c_ulong)
        font,  # the font (LP_DvzFont)
        batch,  # the batch (LP_DvzBatch)
        length,  # the number of Unicode codepoints (int, 32-bit unsigned)
        codepoints,  # the Unicode codepoints (ndpointer_<u4_C_CONTIGUOUS)
    )
    ```

=== "C"

    ``` c
    DvzId dvz_font_texture(  // returns: a tex ID
        DvzFont* font,  // the font
        DvzBatch* batch,  // the batch
        uint32_t length,  // the number of Unicode codepoints
        uint32_t* codepoints,  // the Unicode codepoints
    );
    ```

### `dvz_glyph()`

Create a glyph visual.

=== "Python"

    ``` python
    dvz.glyph(  # returns: the visual (LP_DvzVisual)
        batch,  # the batch (LP_DvzBatch)
        flags,  # the visual creation flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    DvzVisual* dvz_glyph(  // returns: the visual
        DvzBatch* batch,  // the batch
        int flags,  // the visual creation flags
    );
    ```

### `dvz_glyph_alloc()`

Allocate memory for a visual.

=== "Python"

    ``` python
    dvz.glyph_alloc(
        visual,  # the visual (LP_DvzVisual)
        item_count,  # the total number of items to allocate for this visual (int, 32-bit unsigned)
    )
    ```

=== "C"

    ``` c
    void dvz_glyph_alloc(
        DvzVisual* visual,  // the visual
        uint32_t item_count,  // the total number of items to allocate for this visual
    );
    ```

### `dvz_glyph_anchor()`

Set the glyph anchors.

=== "Python"

    ``` python
    dvz.glyph_anchor(
        visual,  # the visual (LP_DvzVisual)
        first,  # the index of the first item to update (int, 32-bit unsigned)
        count,  # the number of items to update (int, 32-bit unsigned)
        values,  # the anchors (x and y) of the items to update (ndpointer_<f4_C_CONTIGUOUS)
        flags,  # the data update flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    void dvz_glyph_anchor(
        DvzVisual* visual,  // the visual
        uint32_t first,  // the index of the first item to update
        uint32_t count,  // the number of items to update
        vec2* values,  // the anchors (x and y) of the items to update
        int flags,  // the data update flags
    );
    ```

### `dvz_glyph_angle()`

Set the glyph angles.

=== "Python"

    ``` python
    dvz.glyph_angle(
        visual,  # the visual (LP_DvzVisual)
        first,  # the index of the first item to update (int, 32-bit unsigned)
        count,  # the number of items to update (int, 32-bit unsigned)
        values,  # the angles of the items to update (ndpointer_<f4_C_CONTIGUOUS)
        flags,  # the data update flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    void dvz_glyph_angle(
        DvzVisual* visual,  // the visual
        uint32_t first,  // the index of the first item to update
        uint32_t count,  // the number of items to update
        float* values,  // the angles of the items to update
        int flags,  // the data update flags
    );
    ```

### `dvz_glyph_ascii()`

Set the glyph ascii characters.

=== "Python"

    ``` python
    dvz.glyph_ascii(
        visual,  # the visual (LP_DvzVisual)
        string,  # the characters (CStringBuffer)
    )
    ```

=== "C"

    ``` c
    void dvz_glyph_ascii(
        DvzVisual* visual,  // the visual
        char* string,  // the characters
    );
    ```

### `dvz_glyph_atlas_font()`

Associate an atlas and font with a glyph visual.

=== "Python"

    ``` python
    dvz.glyph_atlas_font(
        visual,  # the visual (LP_DvzVisual)
        af,  # the atlas font (LP_DvzAtlasFont)
    )
    ```

=== "C"

    ``` c
    void dvz_glyph_atlas_font(
        DvzVisual* visual,  // the visual
        DvzAtlasFont* af,  // the atlas font
    );
    ```

### `dvz_glyph_axis()`

Set the glyph axes.

=== "Python"

    ``` python
    dvz.glyph_axis(
        visual,  # the visual (LP_DvzVisual)
        first,  # the index of the first item to update (int, 32-bit unsigned)
        count,  # the number of items to update (int, 32-bit unsigned)
        values,  # the 3D axis vectors of the items to update (ndpointer_<f4_C_CONTIGUOUS)
        flags,  # the data update flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    void dvz_glyph_axis(
        DvzVisual* visual,  // the visual
        uint32_t first,  // the index of the first item to update
        uint32_t count,  // the number of items to update
        vec3* values,  // the 3D axis vectors of the items to update
        int flags,  // the data update flags
    );
    ```

### `dvz_glyph_bgcolor()`

Set the glyph background color.

=== "Python"

    ``` python
    dvz.glyph_bgcolor(
        visual,  # the visual (LP_DvzVisual)
        bgcolor,  # the background color (vec4)
    )
    ```

=== "C"

    ``` c
    void dvz_glyph_bgcolor(
        DvzVisual* visual,  // the visual
        vec4 bgcolor,  // the background color
    );
    ```

### `dvz_glyph_color()`

Set the glyph colors.

=== "Python"

    ``` python
    dvz.glyph_color(
        visual,  # the visual (LP_DvzVisual)
        first,  # the index of the first item to update (int, 32-bit unsigned)
        count,  # the number of items to update (int, 32-bit unsigned)
        values,  # the colors of the items to update (ndpointer_|u1_C_CONTIGUOUS)
        flags,  # the data update flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    void dvz_glyph_color(
        DvzVisual* visual,  // the visual
        uint32_t first,  // the index of the first item to update
        uint32_t count,  // the number of items to update
        DvzColor* values,  // the colors of the items to update
        int flags,  // the data update flags
    );
    ```

### `dvz_glyph_group_shapes()`

Set the glyph group size.

=== "Python"

    ``` python
    dvz.glyph_group_shapes(
        visual,  # the visual (LP_DvzVisual)
        first,  # the index of the first item to update (int, 32-bit unsigned)
        count,  # the number of items to update (int, 32-bit unsigned)
        values,  # the glyph group shapes (width and height, in pixels) (ndpointer_<f4_C_CONTIGUOUS)
        flags,  # the data update flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    void dvz_glyph_group_shapes(
        DvzVisual* visual,  // the visual
        uint32_t first,  // the index of the first item to update
        uint32_t count,  // the number of items to update
        vec2* values,  // the glyph group shapes (width and height, in pixels)
        int flags,  // the data update flags
    );
    ```

### `dvz_glyph_position()`

Set the glyph positions.

=== "Python"

    ``` python
    dvz.glyph_position(
        visual,  # the visual (LP_DvzVisual)
        first,  # the index of the first item to update (int, 32-bit unsigned)
        count,  # the number of items to update (int, 32-bit unsigned)
        values,  # the 3D positions of the items to update (ndpointer_<f4_C_CONTIGUOUS)
        flags,  # the data update flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    void dvz_glyph_position(
        DvzVisual* visual,  // the visual
        uint32_t first,  // the index of the first item to update
        uint32_t count,  // the number of items to update
        vec3* values,  // the 3D positions of the items to update
        int flags,  // the data update flags
    );
    ```

### `dvz_glyph_scale()`

Set the glyph scaling.

=== "Python"

    ``` python
    dvz.glyph_scale(
        visual,  # the visual (LP_DvzVisual)
        first,  # the index of the first item to update (int, 32-bit unsigned)
        count,  # the number of items to update (int, 32-bit unsigned)
        values,  # the scaling of the items to update (ndpointer_<f4_C_CONTIGUOUS)
        flags,  # the data update flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    void dvz_glyph_scale(
        DvzVisual* visual,  // the visual
        uint32_t first,  // the index of the first item to update
        uint32_t count,  // the number of items to update
        float* values,  // the scaling of the items to update
        int flags,  // the data update flags
    );
    ```

### `dvz_glyph_shift()`

Set the glyph shifts.

=== "Python"

    ``` python
    dvz.glyph_shift(
        visual,  # the visual (LP_DvzVisual)
        first,  # the index of the first item to update (int, 32-bit unsigned)
        count,  # the number of items to update (int, 32-bit unsigned)
        values,  # the shifts (x and y) of the items to update (ndpointer_<f4_C_CONTIGUOUS)
        flags,  # the data update flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    void dvz_glyph_shift(
        DvzVisual* visual,  // the visual
        uint32_t first,  // the index of the first item to update
        uint32_t count,  // the number of items to update
        vec2* values,  // the shifts (x and y) of the items to update
        int flags,  // the data update flags
    );
    ```

### `dvz_glyph_size()`

Set the glyph sizes.

=== "Python"

    ``` python
    dvz.glyph_size(
        visual,  # the visual (LP_DvzVisual)
        first,  # the index of the first item to update (int, 32-bit unsigned)
        count,  # the number of items to update (int, 32-bit unsigned)
        values,  # the sizes (width and height) of the items to update (ndpointer_<f4_C_CONTIGUOUS)
        flags,  # the data update flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    void dvz_glyph_size(
        DvzVisual* visual,  // the visual
        uint32_t first,  // the index of the first item to update
        uint32_t count,  // the number of items to update
        vec2* values,  // the sizes (width and height) of the items to update
        int flags,  // the data update flags
    );
    ```

### `dvz_glyph_strings()`

Helper function to easily set multiple strings of the same size and color on a glyph visual.

=== "Python"

    ``` python
    dvz.glyph_strings(
        visual,  # the visual (LP_DvzVisual)
        string_count,  # the number of strings (int, 32-bit unsigned)
        strings,  # the strings (CStringArrayType)
        positions,  # the positions of each string (ndpointer_<f4_C_CONTIGUOUS)
        scales,  # the scaling of each string (ndpointer_<f4_C_CONTIGUOUS)
        color,  # the same color for all strings (cvec4)
        offset,  # the same offset for all strings (vec2)
        anchor,  # the same anchor for all strings (vec2)
    )
    ```

=== "C"

    ``` c
    void dvz_glyph_strings(
        DvzVisual* visual,  // the visual
        uint32_t string_count,  // the number of strings
        char** strings,  // the strings
        vec3* positions,  // the positions of each string
        float* scales,  // the scaling of each string
        DvzColor color,  // the same color for all strings
        vec2 offset,  // the same offset for all strings
        vec2 anchor,  // the same anchor for all strings
    );
    ```

### `dvz_glyph_texcoords()`

Set the glyph texture coordinates.

=== "Python"

    ``` python
    dvz.glyph_texcoords(
        visual,  # the visual (LP_DvzVisual)
        first,  # the index of the first item to update (int, 32-bit unsigned)
        count,  # the number of items to update (int, 32-bit unsigned)
        coords,  # the x,y,w,h texture coordinates (ndpointer_<f4_C_CONTIGUOUS)
        flags,  # the data update flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    void dvz_glyph_texcoords(
        DvzVisual* visual,  // the visual
        uint32_t first,  // the index of the first item to update
        uint32_t count,  // the number of items to update
        vec4* coords,  // the x,y,w,h texture coordinates
        int flags,  // the data update flags
    );
    ```

### `dvz_glyph_texture()`

Assign a texture to a glyph visual.

=== "Python"

    ``` python
    dvz.glyph_texture(
        visual,  # the visual (LP_DvzVisual)
        tex,  # the texture ID (int, 64-bit unsigned)
    )
    ```

=== "C"

    ``` c
    void dvz_glyph_texture(
        DvzVisual* visual,  // the visual
        DvzId tex,  // the texture ID
    );
    ```

### `dvz_glyph_unicode()`

Set the glyph unicode code points.

=== "Python"

    ``` python
    dvz.glyph_unicode(
        visual,  # the visual (LP_DvzVisual)
        count,  # the number of glyphs (int, 32-bit unsigned)
        codepoints,  # the unicode codepoints (ndpointer_<u4_C_CONTIGUOUS)
    )
    ```

=== "C"

    ``` c
    void dvz_glyph_unicode(
        DvzVisual* visual,  // the visual
        uint32_t count,  // the number of glyphs
        uint32_t* codepoints,  // the unicode codepoints
    );
    ```

### `dvz_glyph_xywh()`

Set the xywh parameters of each glyph.

=== "Python"

    ``` python
    dvz.glyph_xywh(
        visual,  # the visual (LP_DvzVisual)
        first,  # the index of the first item to update (int, 32-bit unsigned)
        count,  # the number of items to update (int, 32-bit unsigned)
        values,  # the xywh values of each glyph (ndpointer_<f4_C_CONTIGUOUS)
        offset,  # the xy offsets of each glyph (vec2)
        flags,  # the data update flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    void dvz_glyph_xywh(
        DvzVisual* visual,  // the visual
        uint32_t first,  // the index of the first item to update
        uint32_t count,  // the number of items to update
        vec4* values,  // the xywh values of each glyph
        vec2 offset,  // the xy offsets of each glyph
        int flags,  // the data update flags
    );
    ```

### `dvz_gui_alpha()`

Set the alpha transparency of the next GUI dialog.

=== "Python"

    ``` python
    dvz.gui_alpha(
        alpha,  # the alpha transparency value (float, 64-bit)
    )
    ```

=== "C"

    ``` c
    void dvz_gui_alpha(
        float alpha,  // the alpha transparency value
    );
    ```

### `dvz_gui_begin()`

Start a new dialog.

=== "Python"

    ``` python
    dvz.gui_begin(
        title,  # the dialog title (CStringBuffer)
        flags,  # the flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    void dvz_gui_begin(
        char* title,  // the dialog title
        int flags,  // the flags
    );
    ```

### `dvz_gui_button()`

Add a button.

=== "Python"

    ``` python
    dvz.gui_button(  # returns: whether the button was pressed (c_bool)
        name,  # the button name (CStringBuffer)
        width,  # the button width (float, 64-bit)
        height,  # the button height (float, 64-bit)
    )
    ```

=== "C"

    ``` c
    bool dvz_gui_button(  // returns: whether the button was pressed
        char* name,  // the button name
        float width,  // the button width
        float height,  // the button height
    );
    ```

### `dvz_gui_checkbox()`

Add a checkbox.

=== "Python"

    ``` python
    dvz.gui_checkbox(  # returns: whether the checkbox's state has changed (c_bool)
        name,  # the button name (CStringBuffer)
    )
    ```

=== "C"

    ``` c
    bool dvz_gui_checkbox(  // returns: whether the checkbox's state has changed
        char* name,  // the button name
    );
    ```

### `dvz_gui_clicked()`

Close the current tree node.

=== "Python"

    ``` python
    dvz.gui_clicked()
    ```

=== "C"

    ``` c
    bool dvz_gui_clicked();
    ```

### `dvz_gui_collapse_changed()`

Return whether a dialog has just been collapsed or uncollapsed.

=== "Python"

    ``` python
    dvz.gui_collapse_changed()  # returns: whether the dialog has just been collapsed or uncollapsed. (c_bool)
    ```

=== "C"

    ``` c
    bool dvz_gui_collapse_changed();  // returns: whether the dialog has just been collapsed or uncollapsed.
    ```

### `dvz_gui_collapsed()`

Return whether a dialog is collapsed.

=== "Python"

    ``` python
    dvz.gui_collapsed()  # returns: whether the dialog is collapsed (c_bool)
    ```

=== "C"

    ``` c
    bool dvz_gui_collapsed();  // returns: whether the dialog is collapsed
    ```

### `dvz_gui_color()`

Set the color of an element.

=== "Python"

    ``` python
    dvz.gui_color(
        type,  # the element type for which to change the color (int, 32-bit signed)
        color,  # the color (cvec4)
    )
    ```

=== "C"

    ``` c
    void dvz_gui_color(
        int type,  // the element type for which to change the color
        cvec4 color,  // the color
    );
    ```

### `dvz_gui_colorpicker()`

Add a color picker

=== "Python"

    ``` python
    dvz.gui_colorpicker(
        name,  # the widget name (CStringBuffer)
        color,  # the color (vec3)
        flags,  # the widget flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    bool dvz_gui_colorpicker(
        char* name,  // the widget name
        vec3 color,  // the color
        int flags,  // the widget flags
    );
    ```

### `dvz_gui_corner()`

Set the corner position of the next GUI dialog.

=== "Python"

    ``` python
    dvz.gui_corner(
        corner,  # which corner (DvzCorner)
        pad,  # the pad (vec2)
    )
    ```

=== "C"

    ``` c
    void dvz_gui_corner(
        DvzCorner corner,  // which corner
        vec2 pad,  // the pad
    );
    ```

### `dvz_gui_demo()`

Show the demo GUI.

=== "Python"

    ``` python
    dvz.gui_demo()
    ```

=== "C"

    ``` c
    void dvz_gui_demo();
    ```

### `dvz_gui_dragging()`

Return whether mouse is dragging.

=== "Python"

    ``` python
    dvz.gui_dragging()  # returns: whether the mouse is dragging (c_bool)
    ```

=== "C"

    ``` c
    bool dvz_gui_dragging();  // returns: whether the mouse is dragging
    ```

### `dvz_gui_dropdown()`

Add a dropdown menu.

=== "Python"

    ``` python
    dvz.gui_dropdown(
        name,  # the menu name (CStringBuffer)
        count,  # the number of menu items (int, 32-bit unsigned)
        items,  # the item labels (CStringArrayType)
        flags,  # the dropdown menu flags (Out)
    )
    ```

=== "C"

    ``` c
    bool dvz_gui_dropdown(
        char* name,  // the menu name
        uint32_t count,  // the number of menu items
        char** items,  // the item labels
        int flags,  // the dropdown menu flags
    );
    ```

### `dvz_gui_end()`

Stop the creation of the dialog.

=== "Python"

    ``` python
    dvz.gui_end()
    ```

=== "C"

    ``` c
    void dvz_gui_end();
    ```

### `dvz_gui_fixed()`

Set a fixed position for a GUI dialog.

=== "Python"

    ``` python
    dvz.gui_fixed(
        pos,  # the dialog position (vec2)
        pivot,  # the pivot (vec2)
    )
    ```

=== "C"

    ``` c
    void dvz_gui_fixed(
        vec2 pos,  // the dialog position
        vec2 pivot,  // the pivot
    );
    ```

### `dvz_gui_flags()`

Set the flags of the next GUI dialog.

=== "Python"

    ``` python
    dvz.gui_flags(
        flags,  # the flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    int dvz_gui_flags(
        int flags,  // the flags
    );
    ```

### `dvz_gui_image()`

Add an image in a GUI dialog.

=== "Python"

    ``` python
    dvz.gui_image(
        tex,  # the texture (LP_DvzTex)
        width,  # the image width (float, 64-bit)
        height,  # the image height (float, 64-bit)
    )
    ```

=== "C"

    ``` c
    void dvz_gui_image(
        DvzTex* tex,  // the texture
        float width,  // the image width
        float height,  // the image height
    );
    ```

### `dvz_gui_moved()`

Return whether a dialog has just moved.

=== "Python"

    ``` python
    dvz.gui_moved()  # returns: whether the dialog has just moved (c_bool)
    ```

=== "C"

    ``` c
    bool dvz_gui_moved();  // returns: whether the dialog has just moved
    ```

### `dvz_gui_moving()`

Return whether a dialog is being moved.

=== "Python"

    ``` python
    dvz.gui_moving()  # returns: whether the dialog is being moved (c_bool)
    ```

=== "C"

    ``` c
    bool dvz_gui_moving();  // returns: whether the dialog is being moved
    ```

### `dvz_gui_node()`

Start a new tree node.

=== "Python"

    ``` python
    dvz.gui_node(
        name,  # the widget name (CStringBuffer)
    )
    ```

=== "C"

    ``` c
    bool dvz_gui_node(
        char* name,  // the widget name
    );
    ```

### `dvz_gui_pop()`

Close the current tree node.

=== "Python"

    ``` python
    dvz.gui_pop()
    ```

=== "C"

    ``` c
    void dvz_gui_pop();
    ```

### `dvz_gui_pos()`

Set the position of the next GUI dialog.

=== "Python"

    ``` python
    dvz.gui_pos(
        pos,  # the dialog position (vec2)
        pivot,  # the pivot (vec2)
    )
    ```

=== "C"

    ``` c
    void dvz_gui_pos(
        vec2 pos,  // the dialog position
        vec2 pivot,  // the pivot
    );
    ```

### `dvz_gui_resized()`

Return whether a dialog has just been resized.

=== "Python"

    ``` python
    dvz.gui_resized()  # returns: whether the dialog has just been resized (c_bool)
    ```

=== "C"

    ``` c
    bool dvz_gui_resized();  // returns: whether the dialog has just been resized
    ```

### `dvz_gui_resizing()`

Return whether a dialog is being resized

=== "Python"

    ``` python
    dvz.gui_resizing()  # returns: whether the dialog is being resized (c_bool)
    ```

=== "C"

    ``` c
    bool dvz_gui_resizing();  // returns: whether the dialog is being resized
    ```

### `dvz_gui_selectable()`

Close the current tree node.

=== "Python"

    ``` python
    dvz.gui_selectable(
        name,  # the widget name (CStringBuffer)
    )
    ```

=== "C"

    ``` c
    bool dvz_gui_selectable(
        char* name,  // the widget name
    );
    ```

### `dvz_gui_size()`

Set the size of the next GUI dialog.

=== "Python"

    ``` python
    dvz.gui_size(
        size,  # the size (vec2)
    )
    ```

=== "C"

    ``` c
    void dvz_gui_size(
        vec2 size,  // the size
    );
    ```

### `dvz_gui_slider()`

Add a slider.

=== "Python"

    ``` python
    dvz.gui_slider(  # returns: whether the value has changed (c_bool)
        name,  # the slider name (CStringBuffer)
        vmin,  # the minimum value (float, 64-bit)
        vmax,  # the maximum value (float, 64-bit)
    )
    ```

=== "C"

    ``` c
    bool dvz_gui_slider(  // returns: whether the value has changed
        char* name,  // the slider name
        float vmin,  // the minimum value
        float vmax,  // the maximum value
    );
    ```

### `dvz_gui_style()`

Set the style of an element.

=== "Python"

    ``` python
    dvz.gui_style(
        type,  # the element type for which to change the style (int, 32-bit signed)
        value,  # the value (float, 64-bit)
    )
    ```

=== "C"

    ``` c
    void dvz_gui_style(
        int type,  // the element type for which to change the style
        float value,  // the value
    );
    ```

### `dvz_gui_table()`

Display a table with selectable rows.

=== "Python"

    ``` python
    dvz.gui_table(  # returns: whether the row selection has changed (in the selected array) (c_bool)
        name,  # the widget name (CStringBuffer)
        row_count,  # the number of rows (int, 32-bit unsigned)
        column_count,  # the number of columns (int, 32-bit unsigned)
        labels,  # all cell labels (CStringArrayType)
        selected,  # a pointer to an array of boolean indicated which rows are selected (ndpointer_|b1_C_CONTIGUOUS)
        flags,  # the Dear ImGui flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    bool dvz_gui_table(  // returns: whether the row selection has changed (in the selected array)
        char* name,  // the widget name
        uint32_t row_count,  // the number of rows
        uint32_t column_count,  // the number of columns
        char** labels,  // all cell labels
        bool* selected,  // a pointer to an array of boolean indicated which rows are selected
        int flags,  // the Dear ImGui flags
    );
    ```

### `dvz_gui_textbox()`

Add a text box in a dialog.

=== "Python"

    ``` python
    dvz.gui_textbox(  # returns: whether the text has changed (c_bool)
        label,  # the label (CStringBuffer)
        str_len,  # the size of the str buffer (int, 32-bit unsigned)
        str,  # the modified string (CStringBuffer)
        flags,  # the flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    bool dvz_gui_textbox(  // returns: whether the text has changed
        char* label,  // the label
        uint32_t str_len,  // the size of the str buffer
        char* str,  // the modified string
        int flags,  // the flags
    );
    ```

### `dvz_gui_tree()`

Display a collapsible tree. Assumes the data is in the right order, with level encoding the

=== "Python"

    ``` python
    dvz.gui_tree(  # returns: whether the selection has changed (c_bool)
        count,  # the number of rows (int, 32-bit unsigned)
        ids,  # short id of each row (CStringArrayType)
        labels,  # full label of each row (CStringArrayType)
        levels,  # a positive integer indicate (ndpointer_<u4_C_CONTIGUOUS)
        colors,  # the color of each square in each row (ndpointer_|u1_C_CONTIGUOUS)
        folded,  # whether each row is currently folded (modified by this function) (ndpointer_|b1_C_CONTIGUOUS)
        selected,  # whether each row is currently selected (modified by this function) (ndpointer_|b1_C_CONTIGUOUS)
        visible,  # whether each row is visible (used for filtering) (ndpointer_|b1_C_CONTIGUOUS)
    )
    ```

=== "C"

    ``` c
    bool dvz_gui_tree(  // returns: whether the selection has changed
        uint32_t count,  // the number of rows
        char** ids,  // short id of each row
        char** labels,  // full label of each row
        uint32_t* levels,  // a positive integer indicate
        DvzColor* colors,  // the color of each square in each row
        bool* folded,  // whether each row is currently folded (modified by this function)
        bool* selected,  // whether each row is currently selected (modified by this function)
        bool* visible,  // whether each row is visible (used for filtering)
    );
    ```

### `dvz_gui_viewport()`

Get the position and size of the current dialog.

=== "Python"

    ``` python
    dvz.gui_viewport(
        viewport,  # the x, y, w, h values (vec4)
    )
    ```

=== "C"

    ``` c
    void dvz_gui_viewport(
        vec4 viewport,  // the x, y, w, h values
    );
    ```

### `dvz_gui_window_capture()`

Capture a GUI window.

=== "Python"

    ``` python
    dvz.gui_window_capture(
        gui_window,  # the GUI window (LP_DvzGuiWindow)
        is_captured,  # whether the windows should be captured (bool)
    )
    ```

=== "C"

    ``` c
    void dvz_gui_window_capture(
        DvzGuiWindow* gui_window,  // the GUI window
        bool is_captured,  // whether the windows should be captured
    );
    ```

### `dvz_image()`

Create an image visual.

=== "Python"

    ``` python
    dvz.image(  # returns: the visual (LP_DvzVisual)
        batch,  # the batch (LP_DvzBatch)
        flags,  # the visual creation flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    DvzVisual* dvz_image(  // returns: the visual
        DvzBatch* batch,  // the batch
        int flags,  // the visual creation flags
    );
    ```

### `dvz_image_alloc()`

Allocate memory for a visual.

=== "Python"

    ``` python
    dvz.image_alloc(
        visual,  # the visual (LP_DvzVisual)
        item_count,  # the total number of images to allocate for this visual (int, 32-bit unsigned)
    )
    ```

=== "C"

    ``` c
    void dvz_image_alloc(
        DvzVisual* visual,  // the visual
        uint32_t item_count,  // the total number of images to allocate for this visual
    );
    ```

### `dvz_image_anchor()`

Set the image anchors.

=== "Python"

    ``` python
    dvz.image_anchor(
        visual,  # the visual (LP_DvzVisual)
        first,  # the index of the first item to update (int, 32-bit unsigned)
        count,  # the number of items to update (int, 32-bit unsigned)
        values,  # the relative anchors of each image, (0,0 = position pertains to top left corner) (ndpointer_<f4_C_CONTIGUOUS)
        flags,  # the data update flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    void dvz_image_anchor(
        DvzVisual* visual,  // the visual
        uint32_t first,  // the index of the first item to update
        uint32_t count,  // the number of items to update
        vec2* values,  // the relative anchors of each image, (0,0 = position pertains to top left corner)
        int flags,  // the data update flags
    );
    ```

### `dvz_image_color()`

Set the image colors (only when using DVZ_IMAGE_FLAGS_FILL).

=== "Python"

    ``` python
    dvz.image_color(
        visual,  # the visual (LP_DvzVisual)
        first,  # the index of the first item to update (int, 32-bit unsigned)
        count,  # the number of items to update (int, 32-bit unsigned)
        values,  # the image colors (ndpointer_|u1_C_CONTIGUOUS)
        flags,  # the data update flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    void dvz_image_color(
        DvzVisual* visual,  // the visual
        uint32_t first,  // the index of the first item to update
        uint32_t count,  // the number of items to update
        DvzColor* values,  // the image colors
        int flags,  // the data update flags
    );
    ```

### `dvz_image_colormap()`

Specify the colormap when using DVZ_IMAGE_FLAGS_MODE_COLORMAP.

=== "Python"

    ``` python
    dvz.image_colormap(
        visual,  # the visual (LP_DvzVisual)
        cmap,  # the colormap (DvzColormap)
    )
    ```

=== "C"

    ``` c
    void dvz_image_colormap(
        DvzVisual* visual,  // the visual
        DvzColormap cmap,  // the colormap
    );
    ```

### `dvz_image_edgecolor()`

Set the edge color.

=== "Python"

    ``` python
    dvz.image_edgecolor(
        visual,  # the visual (LP_DvzVisual)
        color,  # the edge color (cvec4)
    )
    ```

=== "C"

    ``` c
    void dvz_image_edgecolor(
        DvzVisual* visual,  // the visual
        DvzColor color,  // the edge color
    );
    ```

### `dvz_image_linewidth()`

Set the edge width.

=== "Python"

    ``` python
    dvz.image_linewidth(
        visual,  # the visual (LP_DvzVisual)
        width,  # the edge width (float, 64-bit)
    )
    ```

=== "C"

    ``` c
    void dvz_image_linewidth(
        DvzVisual* visual,  // the visual
        float width,  // the edge width
    );
    ```

### `dvz_image_permutation()`

Set the texture coordinates index permutation.

=== "Python"

    ``` python
    dvz.image_permutation(
        visual,  # the visual (LP_DvzVisual)
        ij,  # index permutation (ivec2)
    )
    ```

=== "C"

    ``` c
    void dvz_image_permutation(
        DvzVisual* visual,  // the visual
        ivec2 ij,  // index permutation
    );
    ```

### `dvz_image_position()`

Set the image positions.

=== "Python"

    ``` python
    dvz.image_position(
        visual,  # the visual (LP_DvzVisual)
        first,  # the index of the first item to update (int, 32-bit unsigned)
        count,  # the number of items to update (int, 32-bit unsigned)
        values,  # the 3D positions of the top left corner (ndpointer_<f4_C_CONTIGUOUS)
        flags,  # the data update flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    void dvz_image_position(
        DvzVisual* visual,  // the visual
        uint32_t first,  // the index of the first item to update
        uint32_t count,  // the number of items to update
        vec3* values,  // the 3D positions of the top left corner
        int flags,  // the data update flags
    );
    ```

### `dvz_image_radius()`

Use a rounded rectangle for images, with a given radius in pixels.

=== "Python"

    ``` python
    dvz.image_radius(
        visual,  # the visual (LP_DvzVisual)
        radius,  # the rounded corner radius, in pixel (float, 64-bit)
    )
    ```

=== "C"

    ``` c
    void dvz_image_radius(
        DvzVisual* visual,  // the visual
        float radius,  // the rounded corner radius, in pixel
    );
    ```

### `dvz_image_size()`

Set the image sizes.

=== "Python"

    ``` python
    dvz.image_size(
        visual,  # the visual (LP_DvzVisual)
        first,  # the index of the first item to update (int, 32-bit unsigned)
        count,  # the number of items to update (int, 32-bit unsigned)
        values,  # the sizes of each image, in pixels (ndpointer_<f4_C_CONTIGUOUS)
        flags,  # the data update flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    void dvz_image_size(
        DvzVisual* visual,  // the visual
        uint32_t first,  // the index of the first item to update
        uint32_t count,  // the number of items to update
        vec2* values,  // the sizes of each image, in pixels
        int flags,  // the data update flags
    );
    ```

### `dvz_image_texcoords()`

Set the image texture coordinates.

=== "Python"

    ``` python
    dvz.image_texcoords(
        visual,  # the visual (LP_DvzVisual)
        first,  # the index of the first item to update (int, 32-bit unsigned)
        count,  # the number of items to update (int, 32-bit unsigned)
        tl_br,  # the tex coordinates of the top left and bottom right corners (vec4 u0,v0,u1,v1) (ndpointer_<f4_C_CONTIGUOUS)
        flags,  # the data update flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    void dvz_image_texcoords(
        DvzVisual* visual,  // the visual
        uint32_t first,  // the index of the first item to update
        uint32_t count,  // the number of items to update
        vec4* tl_br,  // the tex coordinates of the top left and bottom right corners (vec4 u0,v0,u1,v1)
        int flags,  // the data update flags
    );
    ```

### `dvz_image_texture()`

Assign a texture to an image visual.

=== "Python"

    ``` python
    dvz.image_texture(
        visual,  # the visual (LP_DvzVisual)
        tex,  # the texture ID (int, 64-bit unsigned)
        filter,  # the texture filtering mode (DvzFilter)
        address_mode,  # the texture address mode (DvzSamplerAddressMode)
    )
    ```

=== "C"

    ``` c
    void dvz_image_texture(
        DvzVisual* visual,  // the visual
        DvzId tex,  // the texture ID
        DvzFilter filter,  // the texture filtering mode
        DvzSamplerAddressMode address_mode,  // the texture address mode
    );
    ```

### `dvz_interpolate()`

Make a linear interpolation between two scalar value.

=== "Python"

    ``` python
    dvz.interpolate(  # returns: the interpolated value (c_float)
        p0,  # the first value (float, 64-bit)
        p1,  # the second value (float, 64-bit)
        t,  # the normalized value (float, 64-bit)
    )
    ```

=== "C"

    ``` c
    float dvz_interpolate(  // returns: the interpolated value
        float p0,  // the first value
        float p1,  // the second value
        float t,  // the normalized value
    );
    ```

### `dvz_interpolate_2D()`

Make a linear interpolation between two 2D points.

=== "Python"

    ``` python
    dvz.interpolate_2D(  # returns: the interpolated point (c_int)
        p0,  # the first point (vec2)
        p1,  # the second point (vec2)
        t,  # the normalized value (float, 64-bit)
    )
    ```

=== "C"

    ``` c
    void dvz_interpolate_2D(  // returns: the interpolated point
        vec2 p0,  // the first point
        vec2 p1,  // the second point
        float t,  // the normalized value
    );
    ```

### `dvz_interpolate_3D()`

Make a linear interpolation between two 3D points.

=== "Python"

    ``` python
    dvz.interpolate_3D(  # returns: the interpolated point (c_int)
        p0,  # the first point (vec3)
        p1,  # the second point (vec3)
        t,  # the normalized value (float, 64-bit)
    )
    ```

=== "C"

    ``` c
    void dvz_interpolate_3D(  // returns: the interpolated point
        vec3 p0,  // the first point
        vec3 p1,  // the second point
        float t,  // the normalized value
    );
    ```

### `dvz_marker()`

Create a marker visual.

=== "Python"

    ``` python
    dvz.marker(  # returns: the visual (LP_DvzVisual)
        batch,  # the batch (LP_DvzBatch)
        flags,  # the visual creation flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    DvzVisual* dvz_marker(  // returns: the visual
        DvzBatch* batch,  // the batch
        int flags,  // the visual creation flags
    );
    ```

### `dvz_marker_alloc()`

Allocate memory for a visual.

=== "Python"

    ``` python
    dvz.marker_alloc(
        visual,  # the visual (LP_DvzVisual)
        item_count,  # the total number of items to allocate for this visual (int, 32-bit unsigned)
    )
    ```

=== "C"

    ``` c
    void dvz_marker_alloc(
        DvzVisual* visual,  // the visual
        uint32_t item_count,  // the total number of items to allocate for this visual
    );
    ```

### `dvz_marker_angle()`

Set the marker angles.

=== "Python"

    ``` python
    dvz.marker_angle(
        visual,  # the visual (LP_DvzVisual)
        first,  # the index of the first item to update (int, 32-bit unsigned)
        count,  # the number of items to update (int, 32-bit unsigned)
        values,  # the angles of the items to update (ndpointer_<f4_C_CONTIGUOUS)
        flags,  # the data update flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    void dvz_marker_angle(
        DvzVisual* visual,  // the visual
        uint32_t first,  // the index of the first item to update
        uint32_t count,  // the number of items to update
        float* values,  // the angles of the items to update
        int flags,  // the data update flags
    );
    ```

### `dvz_marker_aspect()`

Set the marker aspect.

=== "Python"

    ``` python
    dvz.marker_aspect(
        visual,  # the visual (LP_DvzVisual)
        aspect,  # the marker aspect, one of DVZ_MARKER_ASPECT_FILLED, DVZ_MARKER_ASPECT_STROKE, (DvzMarkerAspect)
    )
    ```

=== "C"

    ``` c
    void dvz_marker_aspect(
        DvzVisual* visual,  // the visual
        DvzMarkerAspect aspect,  // the marker aspect, one of DVZ_MARKER_ASPECT_FILLED, DVZ_MARKER_ASPECT_STROKE,
    );
    ```

### `dvz_marker_color()`

Set the marker colors.

=== "Python"

    ``` python
    dvz.marker_color(
        visual,  # the visual (LP_DvzVisual)
        first,  # the index of the first item to update (int, 32-bit unsigned)
        count,  # the number of items to update (int, 32-bit unsigned)
        values,  # the colors of the items to update (ndpointer_|u1_C_CONTIGUOUS)
        flags,  # the data update flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    void dvz_marker_color(
        DvzVisual* visual,  // the visual
        uint32_t first,  // the index of the first item to update
        uint32_t count,  // the number of items to update
        DvzColor* values,  // the colors of the items to update
        int flags,  // the data update flags
    );
    ```

### `dvz_marker_edgecolor()`

Set the marker edge color.

=== "Python"

    ``` python
    dvz.marker_edgecolor(
        visual,  # the visual (LP_DvzVisual)
        color,  # the edge color (cvec4)
    )
    ```

=== "C"

    ``` c
    void dvz_marker_edgecolor(
        DvzVisual* visual,  // the visual
        DvzColor color,  // the edge color
    );
    ```

### `dvz_marker_linewidth()`

Set the marker edge width.

=== "Python"

    ``` python
    dvz.marker_linewidth(
        visual,  # the visual (LP_DvzVisual)
        width,  # the edge width (float, 64-bit)
    )
    ```

=== "C"

    ``` c
    void dvz_marker_linewidth(
        DvzVisual* visual,  // the visual
        float width,  // the edge width
    );
    ```

### `dvz_marker_mode()`

Set the marker mode.

=== "Python"

    ``` python
    dvz.marker_mode(
        visual,  # the visual (LP_DvzVisual)
        mode,  # the marker mode, one of DVZ_MARKER_MODE_CODE, DVZ_MARKER_MODE_BITMAP, (DvzMarkerMode)
    )
    ```

=== "C"

    ``` c
    void dvz_marker_mode(
        DvzVisual* visual,  // the visual
        DvzMarkerMode mode,  // the marker mode, one of DVZ_MARKER_MODE_CODE, DVZ_MARKER_MODE_BITMAP,
    );
    ```

### `dvz_marker_position()`

Set the marker positions.

=== "Python"

    ``` python
    dvz.marker_position(
        visual,  # the visual (LP_DvzVisual)
        first,  # the index of the first item to update (int, 32-bit unsigned)
        count,  # the number of items to update (int, 32-bit unsigned)
        values,  # the 3D positions of the items to update (ndpointer_<f4_C_CONTIGUOUS)
        flags,  # the data update flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    void dvz_marker_position(
        DvzVisual* visual,  // the visual
        uint32_t first,  // the index of the first item to update
        uint32_t count,  // the number of items to update
        vec3* values,  // the 3D positions of the items to update
        int flags,  // the data update flags
    );
    ```

### `dvz_marker_shape()`

Set the marker shape.

=== "Python"

    ``` python
    dvz.marker_shape(
        visual,  # the visual (LP_DvzVisual)
        shape,  # the marker shape (DvzMarkerShape)
    )
    ```

=== "C"

    ``` c
    void dvz_marker_shape(
        DvzVisual* visual,  // the visual
        DvzMarkerShape shape,  // the marker shape
    );
    ```

### `dvz_marker_size()`

Set the marker sizes.

=== "Python"

    ``` python
    dvz.marker_size(
        visual,  # the visual (LP_DvzVisual)
        first,  # the index of the first item to update (int, 32-bit unsigned)
        count,  # the number of items to update (int, 32-bit unsigned)
        values,  # the colors of the items to update (ndpointer_<f4_C_CONTIGUOUS)
        flags,  # the data update flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    void dvz_marker_size(
        DvzVisual* visual,  // the visual
        uint32_t first,  // the index of the first item to update
        uint32_t count,  // the number of items to update
        float* values,  // the colors of the items to update
        int flags,  // the data update flags
    );
    ```

### `dvz_marker_tex()`

Set the marker texture.

=== "Python"

    ``` python
    dvz.marker_tex(
        visual,  # the visual (LP_DvzVisual)
        tex,  # the texture ID (int, 64-bit unsigned)
        sampler,  # the sampler ID (int, 64-bit unsigned)
    )
    ```

=== "C"

    ``` c
    void dvz_marker_tex(
        DvzVisual* visual,  // the visual
        DvzId tex,  // the texture ID
        DvzId sampler,  // the sampler ID
    );
    ```

### `dvz_marker_tex_scale()`

Set the texture scale.

=== "Python"

    ``` python
    dvz.marker_tex_scale(
        visual,  # the visual (LP_DvzVisual)
        scale,  # the texture scale (float, 64-bit)
    )
    ```

=== "C"

    ``` c
    void dvz_marker_tex_scale(
        DvzVisual* visual,  // the visual
        float scale,  // the texture scale
    );
    ```

### `dvz_mesh()`

Create a mesh visual.

=== "Python"

    ``` python
    dvz.mesh(  # returns: the visual (LP_DvzVisual)
        batch,  # the batch (LP_DvzBatch)
        flags,  # the visual creation flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    DvzVisual* dvz_mesh(  // returns: the visual
        DvzBatch* batch,  // the batch
        int flags,  // the visual creation flags
    );
    ```

### `dvz_mesh_alloc()`

Allocate memory for a visual.

=== "Python"

    ``` python
    dvz.mesh_alloc(
        visual,  # the visual (LP_DvzVisual)
        vertex_count,  # the number of vertices (int, 32-bit unsigned)
        index_count,  # the number of indices (int, 32-bit unsigned)
    )
    ```

=== "C"

    ``` c
    void dvz_mesh_alloc(
        DvzVisual* visual,  // the visual
        uint32_t vertex_count,  // the number of vertices
        uint32_t index_count,  // the number of indices
    );
    ```

### `dvz_mesh_color()`

Set the mesh colors.

=== "Python"

    ``` python
    dvz.mesh_color(
        visual,  # the visual (LP_DvzVisual)
        first,  # the index of the first item to update (int, 32-bit unsigned)
        count,  # the number of items to update (int, 32-bit unsigned)
        values,  # the vertex colors (ndpointer_|u1_C_CONTIGUOUS)
        flags,  # the data update flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    void dvz_mesh_color(
        DvzVisual* visual,  // the visual
        uint32_t first,  // the index of the first item to update
        uint32_t count,  // the number of items to update
        DvzColor* values,  // the vertex colors
        int flags,  // the data update flags
    );
    ```

### `dvz_mesh_contour()`

Set the contour information for polygon contours.

=== "Python"

    ``` python
    dvz.mesh_contour(
        visual,  # the visual (LP_DvzVisual)
        first,  # the index of the first item to update (int, 32-bit unsigned)
        count,  # the number of items to update (int, 32-bit unsigned)
        values,  # for vertex A, B, C, the least significant bit is 1 if the opposite edge is a (ndpointer_|u1_C_CONTIGUOUS)
        flags,  # the data update flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    void dvz_mesh_contour(
        DvzVisual* visual,  // the visual
        uint32_t first,  // the index of the first item to update
        uint32_t count,  // the number of items to update
        cvec4* values,  // for vertex A, B, C, the least significant bit is 1 if the opposite edge is a
        int flags,  // the data update flags
    );
    ```

### `dvz_mesh_density()`

Set the number of isolines

=== "Python"

    ``` python
    dvz.mesh_density(
        visual,  # the mesh (LP_DvzVisual)
        count,  # the number of isolines (int, 32-bit unsigned)
    )
    ```

=== "C"

    ``` c
    void dvz_mesh_density(
        DvzVisual* visual,  // the mesh
        uint32_t count,  // the number of isolines
    );
    ```

### `dvz_mesh_edgecolor()`

Set the stroke color.

=== "Python"

    ``` python
    dvz.mesh_edgecolor(
        visual,  # the mesh (LP_DvzVisual)
        stroke,  # the rgba components (cvec4)
    )
    ```

=== "C"

    ``` c
    void dvz_mesh_edgecolor(
        DvzVisual* visual,  // the mesh
         stroke,  // the rgba components
    );
    ```

### `dvz_mesh_index()`

Set the mesh indices.

=== "Python"

    ``` python
    dvz.mesh_index(
        visual,  # the visual (LP_DvzVisual)
        first,  # the index of the first item to update (int, 32-bit unsigned)
        count,  # the number of items to update (int, 32-bit unsigned)
        values,  # the face indices (three vertex indices per triangle) (ndpointer_<u4_C_CONTIGUOUS)
        flags,  # the data update flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    void dvz_mesh_index(
        DvzVisual* visual,  // the visual
        uint32_t first,  // the index of the first item to update
        uint32_t count,  // the number of items to update
        DvzIndex* values,  // the face indices (three vertex indices per triangle)
        int flags,  // the data update flags
    );
    ```

### `dvz_mesh_isoline()`

Set the isolines values.

=== "Python"

    ``` python
    dvz.mesh_isoline(
        visual,  # the visual (LP_DvzVisual)
        first,  # the index of the first item to update (int, 32-bit unsigned)
        count,  # the number of items to update (int, 32-bit unsigned)
        values,  # the scalar field for which to draw isolines (ndpointer_<f4_C_CONTIGUOUS)
        flags,  # the data update flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    void dvz_mesh_isoline(
        DvzVisual* visual,  // the visual
        uint32_t first,  // the index of the first item to update
        uint32_t count,  // the number of items to update
        float* values,  // the scalar field for which to draw isolines
        int flags,  // the data update flags
    );
    ```

### `dvz_mesh_left()`

Set the distance between the current vertex to the left edge at corner A, B, or C in triangle

=== "Python"

    ``` python
    dvz.mesh_left(
        visual,  # the visual (LP_DvzVisual)
        first,  # the index of the first item to update (int, 32-bit unsigned)
        count,  # the number of items to update (int, 32-bit unsigned)
        values,  # the distance to the left edge adjacent to each triangle vertex (ndpointer_<f4_C_CONTIGUOUS)
        flags,  # the data update flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    void dvz_mesh_left(
        DvzVisual* visual,  // the visual
        uint32_t first,  // the index of the first item to update
        uint32_t count,  // the number of items to update
        vec3* values,  // the distance to the left edge adjacent to each triangle vertex
        int flags,  // the data update flags
    );
    ```

### `dvz_mesh_light_color()`

Set the light color.

=== "Python"

    ``` python
    dvz.mesh_light_color(
        visual,  # the mesh (LP_DvzVisual)
        idx,  # the light index (0, 1, 2, or 3) (int, 32-bit unsigned)
        color,  # the light color (rgba, but the a component is ignored) (cvec4)
    )
    ```

=== "C"

    ``` c
    void dvz_mesh_light_color(
        DvzVisual* visual,  // the mesh
        uint32_t idx,  // the light index (0, 1, 2, or 3)
         color,  // the light color (rgba, but the a component is ignored)
    );
    ```

### `dvz_mesh_light_dir()`

Set the light direction.

=== "Python"

    ``` python
    dvz.mesh_light_dir(
        visual,  # the mesh (LP_DvzVisual)
        idx,  # the light index (0, 1, 2, or 3) (int, 32-bit unsigned)
        dir,  # the light direction (vec3)
    )
    ```

=== "C"

    ``` c
    void dvz_mesh_light_dir(
        DvzVisual* visual,  // the mesh
        uint32_t idx,  // the light index (0, 1, 2, or 3)
        vec3 dir,  // the light direction
    );
    ```

### `dvz_mesh_light_params()`

Set the light parameters.

=== "Python"

    ``` python
    dvz.mesh_light_params(
        visual,  # the mesh (LP_DvzVisual)
        idx,  # the light index (0, 1, 2, or 3) (int, 32-bit unsigned)
        params,  # the light parameters (vec4 ambient, diffuse, specular, exponent) (vec4)
    )
    ```

=== "C"

    ``` c
    void dvz_mesh_light_params(
        DvzVisual* visual,  // the mesh
        uint32_t idx,  // the light index (0, 1, 2, or 3)
        vec4 params,  // the light parameters (vec4 ambient, diffuse, specular, exponent)
    );
    ```

### `dvz_mesh_linewidth()`

Set the stroke linewidth (wireframe or isoline).

=== "Python"

    ``` python
    dvz.mesh_linewidth(
        visual,  # the mesh (LP_DvzVisual)
        linewidth,  # the line width (float, 64-bit)
    )
    ```

=== "C"

    ``` c
    void dvz_mesh_linewidth(
        DvzVisual* visual,  // the mesh
        float linewidth,  // the line width
    );
    ```

### `dvz_mesh_normal()`

Set the mesh normals.

=== "Python"

    ``` python
    dvz.mesh_normal(
        visual,  # the visual (LP_DvzVisual)
        first,  # the index of the first item to update (int, 32-bit unsigned)
        count,  # the number of items to update (int, 32-bit unsigned)
        values,  # the vertex normal vectors (ndpointer_<f4_C_CONTIGUOUS)
        flags,  # the data update flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    void dvz_mesh_normal(
        DvzVisual* visual,  // the visual
        uint32_t first,  // the index of the first item to update
        uint32_t count,  // the number of items to update
        vec3* values,  // the vertex normal vectors
        int flags,  // the data update flags
    );
    ```

### `dvz_mesh_position()`

Set the mesh vertex positions.

=== "Python"

    ``` python
    dvz.mesh_position(
        visual,  # the visual (LP_DvzVisual)
        first,  # the index of the first item to update (int, 32-bit unsigned)
        count,  # the number of items to update (int, 32-bit unsigned)
        values,  # the 3D vertex positions (ndpointer_<f4_C_CONTIGUOUS)
        flags,  # the data update flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    void dvz_mesh_position(
        DvzVisual* visual,  // the visual
        uint32_t first,  // the index of the first item to update
        uint32_t count,  // the number of items to update
        vec3* values,  // the 3D vertex positions
        int flags,  // the data update flags
    );
    ```

### `dvz_mesh_reshape()`

Update a mesh once a shape has been updated.

=== "Python"

    ``` python
    dvz.mesh_reshape(
        visual,  # the mesh (LP_DvzVisual)
        shape,  # the shape (LP_DvzShape)
    )
    ```

=== "C"

    ``` c
    void dvz_mesh_reshape(
        DvzVisual* visual,  // the mesh
        DvzShape* shape,  // the shape
    );
    ```

### `dvz_mesh_right()`

Set the distance between the current vertex to the right edge at corner A, B, or C in triangle

=== "Python"

    ``` python
    dvz.mesh_right(
        visual,  # the visual (LP_DvzVisual)
        first,  # the index of the first item to update (int, 32-bit unsigned)
        count,  # the number of items to update (int, 32-bit unsigned)
        values,  # the distance to the right edge adjacent to each triangle vertex (ndpointer_<f4_C_CONTIGUOUS)
        flags,  # the data update flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    void dvz_mesh_right(
        DvzVisual* visual,  // the visual
        uint32_t first,  // the index of the first item to update
        uint32_t count,  // the number of items to update
        vec3* values,  // the distance to the right edge adjacent to each triangle vertex
        int flags,  // the data update flags
    );
    ```

### `dvz_mesh_shape()`

Create a mesh out of a shape.

=== "Python"

    ``` python
    dvz.mesh_shape(  # returns: the mesh (LP_DvzVisual)
        batch,  # the batch (LP_DvzBatch)
        shape,  # the shape (LP_DvzShape)
        flags,  # the visual creation flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    DvzVisual* dvz_mesh_shape(  // returns: the mesh
        DvzBatch* batch,  // the batch
        DvzShape* shape,  // the shape
        int flags,  // the visual creation flags
    );
    ```

### `dvz_mesh_texcoords()`

Set the mesh texture coordinates.

=== "Python"

    ``` python
    dvz.mesh_texcoords(
        visual,  # the visual (LP_DvzVisual)
        first,  # the index of the first item to update (int, 32-bit unsigned)
        count,  # the number of items to update (int, 32-bit unsigned)
        values,  # the vertex texture coordinates (vec4 u,v,*,alpha) (ndpointer_<f4_C_CONTIGUOUS)
        flags,  # the data update flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    void dvz_mesh_texcoords(
        DvzVisual* visual,  // the visual
        uint32_t first,  // the index of the first item to update
        uint32_t count,  // the number of items to update
        vec4* values,  // the vertex texture coordinates (vec4 u,v,*,alpha)
        int flags,  // the data update flags
    );
    ```

### `dvz_mesh_texture()`

Assign a 2D texture to a mesh visual.

=== "Python"

    ``` python
    dvz.mesh_texture(
        visual,  # the visual (LP_DvzVisual)
        tex,  # the texture ID (int, 64-bit unsigned)
        filter,  # the texture filtering mode (DvzFilter)
        address_mode,  # the texture address mode (DvzSamplerAddressMode)
    )
    ```

=== "C"

    ``` c
    void dvz_mesh_texture(
        DvzVisual* visual,  // the visual
        DvzId tex,  // the texture ID
        DvzFilter filter,  // the texture filtering mode
        DvzSamplerAddressMode address_mode,  // the texture address mode
    );
    ```

### `dvz_monoglyph()`

Create a monoglyph visual.

=== "Python"

    ``` python
    dvz.monoglyph(  # returns: the visual (LP_DvzVisual)
        batch,  # the batch (LP_DvzBatch)
        flags,  # the visual creation flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    DvzVisual* dvz_monoglyph(  // returns: the visual
        DvzBatch* batch,  // the batch
        int flags,  // the visual creation flags
    );
    ```

### `dvz_monoglyph_alloc()`

Allocate memory for a visual.

=== "Python"

    ``` python
    dvz.monoglyph_alloc(
        visual,  # the visual (LP_DvzVisual)
        item_count,  # the total number of items to allocate for this visual (int, 32-bit unsigned)
    )
    ```

=== "C"

    ``` c
    void dvz_monoglyph_alloc(
        DvzVisual* visual,  // the visual
        uint32_t item_count,  // the total number of items to allocate for this visual
    );
    ```

### `dvz_monoglyph_anchor()`

Set the glyph anchor (relative to the glyph size).

=== "Python"

    ``` python
    dvz.monoglyph_anchor(
        visual,  # the visual (LP_DvzVisual)
        anchor,  # the anchor (vec2)
    )
    ```

=== "C"

    ``` c
    void dvz_monoglyph_anchor(
        DvzVisual* visual,  // the visual
        vec2 anchor,  // the anchor
    );
    ```

### `dvz_monoglyph_color()`

Set the glyph colors.

=== "Python"

    ``` python
    dvz.monoglyph_color(
        visual,  # the visual (LP_DvzVisual)
        first,  # the index of the first item to update (int, 32-bit unsigned)
        count,  # the number of items to update (int, 32-bit unsigned)
        values,  # the colors of the items to update (ndpointer_|u1_C_CONTIGUOUS)
        flags,  # the data update flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    void dvz_monoglyph_color(
        DvzVisual* visual,  // the visual
        uint32_t first,  // the index of the first item to update
        uint32_t count,  // the number of items to update
        DvzColor* values,  // the colors of the items to update
        int flags,  // the data update flags
    );
    ```

### `dvz_monoglyph_glyph()`

Set the text.

=== "Python"

    ``` python
    dvz.monoglyph_glyph(
        visual,  # the visual (LP_DvzVisual)
        text,  # the ASCII test (string length without the null terminal byte = number of glyphs) (int, 32-bit unsigned)
    )
    ```

=== "C"

    ``` c
    void dvz_monoglyph_glyph(
        DvzVisual* visual,  // the visual
        char* text,  // the ASCII test (string length without the null terminal byte = number of glyphs)
    );
    ```

### `dvz_monoglyph_offset()`

Set the glyph offsets.

=== "Python"

    ``` python
    dvz.monoglyph_offset(
        visual,  # the visual (LP_DvzVisual)
        first,  # the index of the first item to update (int, 32-bit unsigned)
        count,  # the number of items to update (int, 32-bit unsigned)
        values,  # the glyph offsets (ivec2 integers: row,column) (LP_ivec2)
        flags,  # the data update flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    void dvz_monoglyph_offset(
        DvzVisual* visual,  // the visual
        uint32_t first,  // the index of the first item to update
        uint32_t count,  // the number of items to update
        ivec2* values,  // the glyph offsets (ivec2 integers: row,column)
        int flags,  // the data update flags
    );
    ```

### `dvz_monoglyph_position()`

Set the glyph positions.

=== "Python"

    ``` python
    dvz.monoglyph_position(
        visual,  # the visual (LP_DvzVisual)
        first,  # the index of the first item to update (int, 32-bit unsigned)
        count,  # the number of items to update (int, 32-bit unsigned)
        values,  # the 3D positions of the items to update (ndpointer_<f4_C_CONTIGUOUS)
        flags,  # the data update flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    void dvz_monoglyph_position(
        DvzVisual* visual,  // the visual
        uint32_t first,  // the index of the first item to update
        uint32_t count,  // the number of items to update
        vec3* values,  // the 3D positions of the items to update
        int flags,  // the data update flags
    );
    ```

### `dvz_monoglyph_size()`

Set the glyph size (relative to the initial glyph size).

=== "Python"

    ``` python
    dvz.monoglyph_size(
        visual,  # the visual (LP_DvzVisual)
        size,  # the glyph size (float, 64-bit)
    )
    ```

=== "C"

    ``` c
    void dvz_monoglyph_size(
        DvzVisual* visual,  // the visual
        float size,  // the glyph size
    );
    ```

### `dvz_monoglyph_textarea()`

All-in-one function for multiline text.

=== "Python"

    ``` python
    dvz.monoglyph_textarea(
        visual,  # the visual (LP_DvzVisual)
        pos,  # the text position (vec3)
        color,  # the text color (cvec4)
        size,  # the glyph size (float, 64-bit)
        text,  # the text, can contain `\n` new lines (CStringBuffer)
    )
    ```

=== "C"

    ``` c
    void dvz_monoglyph_textarea(
        DvzVisual* visual,  // the visual
        vec3 pos,  // the text position
        DvzColor color,  // the text color
        float size,  // the glyph size
        char* text,  // the text, can contain `\n` new lines
    );
    ```

### `dvz_mouse()`

Create a mouse object.

=== "Python"

    ``` python
    dvz.mouse()  # returns: the mouse (LP_DvzMouse)
    ```

=== "C"

    ``` c
    DvzMouse* dvz_mouse();  // returns: the mouse
    ```

### `dvz_mouse_destroy()`

Destroy a mouse.

=== "Python"

    ``` python
    dvz.mouse_destroy(
        mouse,  # the mouse (LP_DvzMouse)
    )
    ```

=== "C"

    ``` c
    void dvz_mouse_destroy(
        DvzMouse* mouse,  // the mouse
    );
    ```

### `dvz_mouse_event()`

Create a generic mouse event.

=== "Python"

    ``` python
    dvz.mouse_event(
        mouse,  # the mouse (LP_DvzMouse)
        ev,  # the mouse event (DvzMouseEvent)
    )
    ```

=== "C"

    ``` c
    void dvz_mouse_event(
        DvzMouse* mouse,  // the mouse
        DvzMouseEvent ev,  // the mouse event
    );
    ```

### `dvz_mouse_move()`

Create a mouse move event.

=== "Python"

    ``` python
    dvz.mouse_move(  # returns: the generated mouse event (DvzMouseEvent)
        mouse,  # the mouse (LP_DvzMouse)
        pos,  # the cursor position, in pixels (vec2)
        mods,  # the keyboard modifier flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    DvzMouseEvent dvz_mouse_move(  // returns: the generated mouse event
        DvzMouse* mouse,  // the mouse
        vec2 pos,  // the cursor position, in pixels
        int mods,  // the keyboard modifier flags
    );
    ```

### `dvz_mouse_press()`

Create a mouse press event.

=== "Python"

    ``` python
    dvz.mouse_press(  # returns: the generated mouse event (DvzMouseEvent)
        mouse,  # the mouse (LP_DvzMouse)
        button,  # the mouse button (enum int) (DvzMouseButton)
        mods,  # the keyboard modifier flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    DvzMouseEvent dvz_mouse_press(  // returns: the generated mouse event
        DvzMouse* mouse,  // the mouse
        DvzMouseButton button,  // the mouse button (enum int)
        int mods,  // the keyboard modifier flags
    );
    ```

### `dvz_mouse_release()`

Create a mouse release event.

=== "Python"

    ``` python
    dvz.mouse_release(  # returns: the generated mouse event (DvzMouseEvent)
        mouse,  # the mouse (LP_DvzMouse)
        button,  # the mouse button (enum int) (DvzMouseButton)
        mods,  # the keyboard modifier flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    DvzMouseEvent dvz_mouse_release(  // returns: the generated mouse event
        DvzMouse* mouse,  // the mouse
        DvzMouseButton button,  // the mouse button (enum int)
        int mods,  // the keyboard modifier flags
    );
    ```

### `dvz_mouse_wheel()`

Create a mouse wheel event.

=== "Python"

    ``` python
    dvz.mouse_wheel(  # returns: the generated mouse event (DvzMouseEvent)
        mouse,  # the mouse (LP_DvzMouse)
        button,  # the mouse wheel direction (x, y) (vec2)
        mods,  # the keyboard modifier flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    DvzMouseEvent dvz_mouse_wheel(  // returns: the generated mouse event
        DvzMouse* mouse,  // the mouse
         button,  // the mouse wheel direction (x, y)
        int mods,  // the keyboard modifier flags
    );
    ```

### `dvz_msdf_from_svg()`

Generate a multichannel SDF from an SVG path.

=== "Python"

    ``` python
    dvz.msdf_from_svg(  # returns: the generated texture as RGB floats (ndpointer_<f4_C_CONTIGUOUS)
        svg_path,  # the SVG path (CStringBuffer)
        width,  # the width of the generated SDF, in pixels (int, 32-bit unsigned)
        height,  # the height of the generated SDF, in pixels (int, 32-bit unsigned)
    )
    ```

=== "C"

    ``` c
    float* dvz_msdf_from_svg(  // returns: the generated texture as RGB floats
        char* svg_path,  // the SVG path
        uint32_t width,  // the width of the generated SDF, in pixels
        uint32_t height,  // the height of the generated SDF, in pixels
    );
    ```

### `dvz_msdf_to_rgb()`

Convert a multichannel SDF float texture to a byte texture.

=== "Python"

    ``` python
    dvz.msdf_to_rgb(  # returns: the byte texture (ndpointer_|u1_C_CONTIGUOUS)
        sdf,  # the SDF float texture (ndpointer_<f4_C_CONTIGUOUS)
        width,  # the width of the texture (int, 32-bit unsigned)
        height,  # the height of the texture (int, 32-bit unsigned)
    )
    ```

=== "C"

    ``` c
    uint8_t* dvz_msdf_to_rgb(  // returns: the byte texture
        float* sdf,  // the SDF float texture
        uint32_t width,  // the width of the texture
        uint32_t height,  // the height of the texture
    );
    ```

### `dvz_ortho_end()`

End an ortho interaction.

=== "Python"

    ``` python
    dvz.ortho_end(
        ortho,  # the ortho (LP_DvzOrtho)
    )
    ```

=== "C"

    ``` c
    void dvz_ortho_end(
        DvzOrtho* ortho,  // the ortho
    );
    ```

### `dvz_ortho_flags()`

Set the ortho flags.

=== "Python"

    ``` python
    dvz.ortho_flags(
        ortho,  # the ortho (LP_DvzOrtho)
        flags,  # the flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    void dvz_ortho_flags(
        DvzOrtho* ortho,  // the ortho
        int flags,  // the flags
    );
    ```

### `dvz_ortho_mvp()`

Apply an MVP matrix to an ortho.

=== "Python"

    ``` python
    dvz.ortho_mvp(
        ortho,  # the ortho (LP_DvzOrtho)
        mvp,  # the MVP (LP_DvzMVP)
    )
    ```

=== "C"

    ``` c
    void dvz_ortho_mvp(
        DvzOrtho* ortho,  // the ortho
        DvzMVP* mvp,  // the MVP
    );
    ```

### `dvz_ortho_pan()`

Apply a pan value to an ortho.

=== "Python"

    ``` python
    dvz.ortho_pan(
        ortho,  # the ortho (LP_DvzOrtho)
        pan,  # the pan, in NDC (vec2)
    )
    ```

=== "C"

    ``` c
    void dvz_ortho_pan(
        DvzOrtho* ortho,  // the ortho
        vec2 pan,  // the pan, in NDC
    );
    ```

### `dvz_ortho_pan_shift()`

Apply a pan shift to an ortho.

=== "Python"

    ``` python
    dvz.ortho_pan_shift(
        ortho,  # the ortho (LP_DvzOrtho)
        shift_px,  # the shift value, in pixels (vec2)
        center_px,  # the center position, in pixels (vec2)
    )
    ```

=== "C"

    ``` c
    void dvz_ortho_pan_shift(
        DvzOrtho* ortho,  // the ortho
        vec2 shift_px,  // the shift value, in pixels
        vec2 center_px,  // the center position, in pixels
    );
    ```

### `dvz_ortho_reset()`

Reset an ortho.

=== "Python"

    ``` python
    dvz.ortho_reset(
        ortho,  # the ortho (LP_DvzOrtho)
    )
    ```

=== "C"

    ``` c
    void dvz_ortho_reset(
        DvzOrtho* ortho,  // the ortho
    );
    ```

### `dvz_ortho_resize()`

Inform an ortho of a panel resize.

=== "Python"

    ``` python
    dvz.ortho_resize(
        ortho,  # the ortho (LP_DvzOrtho)
        width,  # the panel width (float, 64-bit)
        height,  # the panel height (float, 64-bit)
    )
    ```

=== "C"

    ``` c
    void dvz_ortho_resize(
        DvzOrtho* ortho,  // the ortho
        float width,  // the panel width
        float height,  // the panel height
    );
    ```

### `dvz_ortho_zoom()`

Apply a zoom value to an ortho.

=== "Python"

    ``` python
    dvz.ortho_zoom(
        ortho,  # the ortho (LP_DvzOrtho)
        zoom,  # the zoom level (float, 64-bit)
    )
    ```

=== "C"

    ``` c
    void dvz_ortho_zoom(
        DvzOrtho* ortho,  // the ortho
        float zoom,  // the zoom level
    );
    ```

### `dvz_ortho_zoom_shift()`

Apply a zoom shift to an ortho.

=== "Python"

    ``` python
    dvz.ortho_zoom_shift(
        ortho,  # the ortho (LP_DvzOrtho)
        shift_px,  # the shift value, in pixels (vec2)
        center_px,  # the center position, in pixels (vec2)
    )
    ```

=== "C"

    ``` c
    void dvz_ortho_zoom_shift(
        DvzOrtho* ortho,  // the ortho
        vec2 shift_px,  // the shift value, in pixels
        vec2 center_px,  // the center position, in pixels
    );
    ```

### `dvz_ortho_zoom_wheel()`

Apply a wheel zoom to an ortho.

=== "Python"

    ``` python
    dvz.ortho_zoom_wheel(
        ortho,  # the ortho (LP_DvzOrtho)
        dir,  # the wheel direction (vec2)
        center_px,  # the center position, in pixels (vec2)
    )
    ```

=== "C"

    ``` c
    void dvz_ortho_zoom_wheel(
        DvzOrtho* ortho,  // the ortho
        vec2 dir,  // the wheel direction
        vec2 center_px,  // the center position, in pixels
    );
    ```

### `dvz_panel()`

Create a panel in a figure (partial or complete rectangular portion of a figure).

=== "Python"

    ``` python
    dvz.panel(
        fig,  # the figure (LP_DvzFigure)
        x,  # the x coordinate of the top left corner, in pixels (float, 64-bit)
        y,  # the y coordinate of the top left corner, in pixels (float, 64-bit)
        width,  # the panel width, in pixels (float, 64-bit)
        height,  # the panel height, in pixels (float, 64-bit)
    )
    ```

=== "C"

    ``` c
    DvzPanel* dvz_panel(
        DvzFigure* fig,  // the figure
        float x,  // the x coordinate of the top left corner, in pixels
        float y,  // the y coordinate of the top left corner, in pixels
        float width,  // the panel width, in pixels
        float height,  // the panel height, in pixels
    );
    ```

### `dvz_panel_arcball()`

Set arcball interactivity for a panel.

=== "Python"

    ``` python
    dvz.panel_arcball(  # returns: the arcball (LP_DvzArcball)
        panel,  # the panel (LP_DvzPanel)
    )
    ```

=== "C"

    ``` c
    DvzArcball* dvz_panel_arcball(  // returns: the arcball
        DvzPanel* panel,  // the panel
    );
    ```

### `dvz_panel_at()`

Return the panel containing a given point.

=== "Python"

    ``` python
    dvz.panel_at(  # returns: the panel containing the point, or NULL if there is none (LP_DvzPanel)
        figure,  # the figure (LP_DvzFigure)
        pos,  # the position (vec2)
    )
    ```

=== "C"

    ``` c
    DvzPanel* dvz_panel_at(  // returns: the panel containing the point, or NULL if there is none
        DvzFigure* figure,  // the figure
        vec2 pos,  // the position
    );
    ```

### `dvz_panel_batch()`

Return the batch from a panel.

=== "Python"

    ``` python
    dvz.panel_batch(  # returns: the batch (LP_DvzBatch)
        panel,  # the panel (LP_DvzPanel)
    )
    ```

=== "C"

    ``` c
    DvzBatch* dvz_panel_batch(  // returns: the batch
        DvzPanel* panel,  // the panel
    );
    ```

### `dvz_panel_camera()`

Set a camera for a panel.

=== "Python"

    ``` python
    dvz.panel_camera(  # returns: the camera (LP_DvzCamera)
        panel,  # the panel (LP_DvzPanel)
        flags,  # the camera flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    DvzCamera* dvz_panel_camera(  // returns: the camera
        DvzPanel* panel,  // the panel
        int flags,  // the camera flags
    );
    ```

### `dvz_panel_contains()`

Return whether a point is inside a panel.

=== "Python"

    ``` python
    dvz.panel_contains(  # returns: true if the position lies within the panel (c_bool)
        panel,  # the panel (LP_DvzPanel)
        pos,  # the position (vec2)
    )
    ```

=== "C"

    ``` c
    bool dvz_panel_contains(  // returns: true if the position lies within the panel
        DvzPanel* panel,  // the panel
        vec2 pos,  // the position
    );
    ```

### `dvz_panel_default()`

Return the default full panel spanning an entire figure.

=== "Python"

    ``` python
    dvz.panel_default(  # returns: the panel spanning the entire figure (LP_DvzPanel)
        fig,  # the figure (LP_DvzFigure)
    )
    ```

=== "C"

    ``` c
    DvzPanel* dvz_panel_default(  // returns: the panel spanning the entire figure
        DvzFigure* fig,  // the figure
    );
    ```

### `dvz_panel_destroy()`

Destroy a panel.

=== "Python"

    ``` python
    dvz.panel_destroy(
        panel,  # the panel (LP_DvzPanel)
    )
    ```

=== "C"

    ``` c
    void dvz_panel_destroy(
        DvzPanel* panel,  // the panel
    );
    ```

### `dvz_panel_figure()`

Return the figure from a panel.

=== "Python"

    ``` python
    dvz.panel_figure(  # returns: the figure (LP_DvzFigure)
        panel,  # the panel (LP_DvzPanel)
    )
    ```

=== "C"

    ``` c
    DvzFigure* dvz_panel_figure(  // returns: the figure
        DvzPanel* panel,  // the panel
    );
    ```

### `dvz_panel_flags()`

Set the panel flags

=== "Python"

    ``` python
    dvz.panel_flags(
        panel,  # the panel (LP_DvzPanel)
        flags,  # the panel flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    void dvz_panel_flags(
        DvzPanel* panel,  // the panel
        int flags,  // the panel flags
    );
    ```

### `dvz_panel_gui()`

Set a panel as a GUI panel.

=== "Python"

    ``` python
    dvz.panel_gui(
        panel,  # the panel (LP_DvzPanel)
        title,  # the GUI dialog title (CStringBuffer)
        flags,  # the GUI dialog flags (unused at the moment) (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    void dvz_panel_gui(
        DvzPanel* panel,  // the panel
        char* title,  // the GUI dialog title
        int flags,  // the GUI dialog flags (unused at the moment)
    );
    ```

### `dvz_panel_margins()`

Set the margins of a panel.

=== "Python"

    ``` python
    dvz.panel_margins(
        panel,  # the panel (LP_DvzPanel)
        top,  # the top margin, in pixels (float, 64-bit)
        right,  # the right margin, in pixels (float, 64-bit)
        bottom,  # the bottom margin, in pixels (float, 64-bit)
        left,  # the left margin, in pixels (float, 64-bit)
    )
    ```

=== "C"

    ``` c
    void dvz_panel_margins(
        DvzPanel* panel,  // the panel
        float top,  // the top margin, in pixels
        float right,  // the right margin, in pixels
        float bottom,  // the bottom margin, in pixels
        float left,  // the left margin, in pixels
    );
    ```

### `dvz_panel_mvp()`

Assign a MVP structure to a panel.

=== "Python"

    ``` python
    dvz.panel_mvp(
        panel,  # the panel (LP_DvzPanel)
        mvp,  # a pointer to the MVP structure (LP_DvzMVP)
    )
    ```

=== "C"

    ``` c
    void dvz_panel_mvp(
        DvzPanel* panel,  // the panel
        DvzMVP* mvp,  // a pointer to the MVP structure
    );
    ```

### `dvz_panel_mvpmat()`

Assign the model-view-proj matrices to a panel.

=== "Python"

    ``` python
    dvz.panel_mvpmat(
        panel,  # the panel (LP_DvzPanel)
        model,  # the model matrix (mat4)
        view,  # the view matrix (mat4)
        proj,  # the projection matrix (mat4)
    )
    ```

=== "C"

    ``` c
    void dvz_panel_mvpmat(
        DvzPanel* panel,  // the panel
        mat4 model,  // the model matrix
        mat4 view,  // the view matrix
        mat4 proj,  // the projection matrix
    );
    ```

### `dvz_panel_ortho()`

Set ortho interactivity for a panel.

=== "Python"

    ``` python
    dvz.panel_ortho(  # returns: the ortho (LP_DvzOrtho)
        panel,  # the panel (LP_DvzPanel)
    )
    ```

=== "C"

    ``` c
    DvzOrtho* dvz_panel_ortho(  // returns: the ortho
        DvzPanel* panel,  // the panel
    );
    ```

### `dvz_panel_panzoom()`

Set panzoom interactivity for a panel.

=== "Python"

    ``` python
    dvz.panel_panzoom(  # returns: the panzoom (LP_DvzPanzoom)
        panel,  # the panel (LP_DvzPanel)
    )
    ```

=== "C"

    ``` c
    DvzPanzoom* dvz_panel_panzoom(  // returns: the panzoom
        DvzPanel* panel,  // the panel
    );
    ```

### `dvz_panel_ref()`

Get or create a Reference for a panel.

=== "Python"

    ``` python
    dvz.panel_ref(  # returns: the reference (LP_DvzRef)
        panel,  # the panel (LP_DvzPanel)
        flags,  # the reference creation flags. (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    DvzRef* dvz_panel_ref(  // returns: the reference
        DvzPanel* panel,  // the panel
        int flags,  // the reference creation flags.
    );
    ```

### `dvz_panel_resize()`

Resize a panel.

=== "Python"

    ``` python
    dvz.panel_resize(
        panel,  # the panel (LP_DvzPanel)
        x,  # the x coordinate of the top left corner, in pixels (float, 64-bit)
        y,  # the y coordinate of the top left corner, in pixels (float, 64-bit)
        width,  # the panel width, in pixels (float, 64-bit)
        height,  # the panel height, in pixels (float, 64-bit)
    )
    ```

=== "C"

    ``` c
    void dvz_panel_resize(
        DvzPanel* panel,  // the panel
        float x,  // the x coordinate of the top left corner, in pixels
        float y,  // the y coordinate of the top left corner, in pixels
        float width,  // the panel width, in pixels
        float height,  // the panel height, in pixels
    );
    ```

### `dvz_panel_show()`

Show or hide a panel.

=== "Python"

    ``` python
    dvz.panel_show(
        panel,  # the panel (LP_DvzPanel)
        is_visible,  # whether to show or hide the panel (bool)
    )
    ```

=== "C"

    ``` c
    void dvz_panel_show(
        DvzPanel* panel,  // the panel
        bool is_visible,  // whether to show or hide the panel
    );
    ```

### `dvz_panel_transform()`

Assign a transform to a panel.

=== "Python"

    ``` python
    dvz.panel_transform(
        panel,  # the panel (LP_DvzPanel)
        tr,  # the transform (LP_DvzTransform)
    )
    ```

=== "C"

    ``` c
    void dvz_panel_transform(
        DvzPanel* panel,  // the panel
        DvzTransform* tr,  // the transform
    );
    ```

### `dvz_panel_update()`

Trigger a panel update.

=== "Python"

    ``` python
    dvz.panel_update(
        panel,  # the panel (LP_DvzPanel)
    )
    ```

=== "C"

    ``` c
    void dvz_panel_update(
        DvzPanel* panel,  // the panel
    );
    ```

### `dvz_panel_visual()`

Add a visual to a panel.

=== "Python"

    ``` python
    dvz.panel_visual(
        panel,  # the panel (LP_DvzPanel)
        visual,  # the visual (LP_DvzVisual)
    )
    ```

=== "C"

    ``` c
    void dvz_panel_visual(
        DvzPanel* panel,  // the panel
        DvzVisual* visual,  // the visual
    );
    ```

### `dvz_panzoom()`

Create a panzoom object (usually you'd rather use `dvz_panel_panzoom()`).

=== "Python"

    ``` python
    dvz.panzoom(  # returns: the Panzoom object (LP_DvzPanzoom)
        width,  # the panel width (float, 64-bit)
        height,  # the panel height (float, 64-bit)
        flags,  # the panzoom creation flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    DvzPanzoom* dvz_panzoom(  // returns: the Panzoom object
        float width,  // the panel width
        float height,  // the panel height
        int flags,  // the panzoom creation flags
    );
    ```

### `dvz_panzoom_bounds()`

Get x-y bounds.

=== "Python"

    ``` python
    dvz.panzoom_bounds(
        pz,  # the panzoom (LP_DvzPanzoom)
        ref,  # the ref (LP_DvzRef)
    )
    ```

=== "C"

    ``` c
    void dvz_panzoom_bounds(
        DvzPanzoom* pz,  // the panzoom
        DvzRef* ref,  // the ref
    );
    ```

### `dvz_panzoom_destroy()`

Destroy a panzoom.

=== "Python"

    ``` python
    dvz.panzoom_destroy(
        pz,  # the pz (LP_DvzPanzoom)
    )
    ```

=== "C"

    ``` c
    void dvz_panzoom_destroy(
        DvzPanzoom* pz,  // the pz
    );
    ```

### `dvz_panzoom_end()`

End a panzoom interaction.

=== "Python"

    ``` python
    dvz.panzoom_end(
        pz,  # the panzoom (LP_DvzPanzoom)
    )
    ```

=== "C"

    ``` c
    void dvz_panzoom_end(
        DvzPanzoom* pz,  // the panzoom
    );
    ```

### `dvz_panzoom_extent()`

Get the extent box.

=== "Python"

    ``` python
    dvz.panzoom_extent(
        pz,  # the panzoom (LP_DvzPanzoom)
    )
    ```

=== "C"

    ``` c
    void dvz_panzoom_extent(
        DvzPanzoom* pz,  // the panzoom
    );
    ```

### `dvz_panzoom_flags()`

Set the panzoom flags.

=== "Python"

    ``` python
    dvz.panzoom_flags(
        pz,  # the panzoom (LP_DvzPanzoom)
        flags,  # the flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    void dvz_panzoom_flags(
        DvzPanzoom* pz,  // the panzoom
        int flags,  // the flags
    );
    ```

### `dvz_panzoom_level()`

Get the current zoom level.

=== "Python"

    ``` python
    dvz.panzoom_level(
        pz,  # the panzoom (LP_DvzPanzoom)
        dim,  # the dimension (DvzDim)
    )
    ```

=== "C"

    ``` c
    float dvz_panzoom_level(
        DvzPanzoom* pz,  // the panzoom
        DvzDim dim,  // the dimension
    );
    ```

### `dvz_panzoom_mouse()`

Register a mouse event to a panzoom.

=== "Python"

    ``` python
    dvz.panzoom_mouse(  # returns: whether the panzoom is affected by the mouse event (c_bool)
        pz,  # the panzoom (LP_DvzPanzoom)
        ev,  # the mouse event (DvzMouseEvent)
    )
    ```

=== "C"

    ``` c
    bool dvz_panzoom_mouse(  // returns: whether the panzoom is affected by the mouse event
        DvzPanzoom* pz,  // the panzoom
        DvzMouseEvent ev,  // the mouse event
    );
    ```

### `dvz_panzoom_mvp()`

Apply an MVP matrix to a panzoom.

=== "Python"

    ``` python
    dvz.panzoom_mvp(
        pz,  # the panzoom (LP_DvzPanzoom)
        mvp,  # the MVP (LP_DvzMVP)
    )
    ```

=== "C"

    ``` c
    void dvz_panzoom_mvp(
        DvzPanzoom* pz,  // the panzoom
        DvzMVP* mvp,  // the MVP
    );
    ```

### `dvz_panzoom_pan()`

Apply a pan value to a panzoom.

=== "Python"

    ``` python
    dvz.panzoom_pan(
        pz,  # the panzoom (LP_DvzPanzoom)
        pan,  # the pan, in NDC (vec2)
    )
    ```

=== "C"

    ``` c
    void dvz_panzoom_pan(
        DvzPanzoom* pz,  // the panzoom
        vec2 pan,  // the pan, in NDC
    );
    ```

### `dvz_panzoom_pan_shift()`

Apply a pan shift to a panzoom.

=== "Python"

    ``` python
    dvz.panzoom_pan_shift(
        pz,  # the panzoom (LP_DvzPanzoom)
        shift_px,  # the shift value, in pixels (vec2)
        center_px,  # the center position, in pixels (vec2)
    )
    ```

=== "C"

    ``` c
    void dvz_panzoom_pan_shift(
        DvzPanzoom* pz,  // the panzoom
        vec2 shift_px,  // the shift value, in pixels
        vec2 center_px,  // the center position, in pixels
    );
    ```

### `dvz_panzoom_reset()`

Reset a panzoom.

=== "Python"

    ``` python
    dvz.panzoom_reset(
        pz,  # the panzoom (LP_DvzPanzoom)
    )
    ```

=== "C"

    ``` c
    void dvz_panzoom_reset(
        DvzPanzoom* pz,  // the panzoom
    );
    ```

### `dvz_panzoom_resize()`

Inform a panzoom of a panel resize.

=== "Python"

    ``` python
    dvz.panzoom_resize(
        pz,  # the panzoom (LP_DvzPanzoom)
        width,  # the panel width (float, 64-bit)
        height,  # the panel height (float, 64-bit)
    )
    ```

=== "C"

    ``` c
    void dvz_panzoom_resize(
        DvzPanzoom* pz,  // the panzoom
        float width,  // the panel width
        float height,  // the panel height
    );
    ```

### `dvz_panzoom_set()`

Set the extent box.

=== "Python"

    ``` python
    dvz.panzoom_set(
        pz,  # the panzoom (LP_DvzPanzoom)
        extent,  # the extent box (LP_DvzBox)
    )
    ```

=== "C"

    ``` c
    void dvz_panzoom_set(
        DvzPanzoom* pz,  // the panzoom
        DvzBox* extent,  // the extent box
    );
    ```

### `dvz_panzoom_xlim()`

Set x bounds.

=== "Python"

    ``` python
    dvz.panzoom_xlim(
        pz,  # the panzoom (LP_DvzPanzoom)
        ref,  # the ref (LP_DvzRef)
        xmin,  # xmin (float, 32-bit)
        xmax,  # xmax (float, 32-bit)
    )
    ```

=== "C"

    ``` c
    void dvz_panzoom_xlim(
        DvzPanzoom* pz,  // the panzoom
        DvzRef* ref,  // the ref
        double xmin,  // xmin
        double xmax,  // xmax
    );
    ```

### `dvz_panzoom_ylim()`

Set y bounds.

=== "Python"

    ``` python
    dvz.panzoom_ylim(
        pz,  # the panzoom (LP_DvzPanzoom)
        ref,  # the ref (LP_DvzRef)
        ymin,  # ymin (float, 32-bit)
        ymax,  # ymax (float, 32-bit)
    )
    ```

=== "C"

    ``` c
    void dvz_panzoom_ylim(
        DvzPanzoom* pz,  // the panzoom
        DvzRef* ref,  // the ref
        double ymin,  // ymin
        double ymax,  // ymax
    );
    ```

### `dvz_panzoom_zoom()`

Apply a zoom value to a panzoom.

=== "Python"

    ``` python
    dvz.panzoom_zoom(
        pz,  # the panzoom (LP_DvzPanzoom)
        zoom,  # the zoom, in NDC (vec2)
    )
    ```

=== "C"

    ``` c
    void dvz_panzoom_zoom(
        DvzPanzoom* pz,  // the panzoom
        vec2 zoom,  // the zoom, in NDC
    );
    ```

### `dvz_panzoom_zoom_shift()`

Apply a zoom shift to a panzoom.

=== "Python"

    ``` python
    dvz.panzoom_zoom_shift(
        pz,  # the panzoom (LP_DvzPanzoom)
        shift_px,  # the shift value, in pixels (vec2)
        center_px,  # the center position, in pixels (vec2)
    )
    ```

=== "C"

    ``` c
    void dvz_panzoom_zoom_shift(
        DvzPanzoom* pz,  // the panzoom
        vec2 shift_px,  // the shift value, in pixels
        vec2 center_px,  // the center position, in pixels
    );
    ```

### `dvz_panzoom_zoom_wheel()`

Apply a wheel zoom to a panzoom.

=== "Python"

    ``` python
    dvz.panzoom_zoom_wheel(
        pz,  # the panzoom (LP_DvzPanzoom)
        dir,  # the wheel direction (vec2)
        center_px,  # the center position, in pixels (vec2)
    )
    ```

=== "C"

    ``` c
    void dvz_panzoom_zoom_wheel(
        DvzPanzoom* pz,  // the panzoom
        vec2 dir,  // the wheel direction
        vec2 center_px,  // the center position, in pixels
    );
    ```

### `dvz_path()`

Create a path visual.

=== "Python"

    ``` python
    dvz.path(  # returns: the visual (LP_DvzVisual)
        batch,  # the batch (LP_DvzBatch)
        flags,  # the visual creation flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    DvzVisual* dvz_path(  // returns: the visual
        DvzBatch* batch,  // the batch
        int flags,  // the visual creation flags
    );
    ```

### `dvz_path_alloc()`

Allocate memory for a visual.

=== "Python"

    ``` python
    dvz.path_alloc(
        visual,  # the visual (LP_DvzVisual)
        total_point_count,  # the total number of points to allocate for this visual (int, 32-bit unsigned)
    )
    ```

=== "C"

    ``` c
    void dvz_path_alloc(
        DvzVisual* visual,  // the visual
        uint32_t total_point_count,  // the total number of points to allocate for this visual
    );
    ```

### `dvz_path_cap()`

Set the path cap.

=== "Python"

    ``` python
    dvz.path_cap(
        visual,  # the visual (LP_DvzVisual)
        cap,  # the cap (DvzCapType)
    )
    ```

=== "C"

    ``` c
    void dvz_path_cap(
        DvzVisual* visual,  // the visual
        DvzCapType cap,  // the cap
    );
    ```

### `dvz_path_color()`

Set the path colors.

=== "Python"

    ``` python
    dvz.path_color(
        visual,  # the visual (LP_DvzVisual)
        first,  # the index of the first item to update (int, 32-bit unsigned)
        count,  # the number of items to update (int, 32-bit unsigned)
        values,  # the colors of the items to update (ndpointer_|u1_C_CONTIGUOUS)
        flags,  # the data update flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    void dvz_path_color(
        DvzVisual* visual,  // the visual
        uint32_t first,  // the index of the first item to update
        uint32_t count,  // the number of items to update
        DvzColor* values,  // the colors of the items to update
        int flags,  // the data update flags
    );
    ```

### `dvz_path_join()`

Set the path join.

=== "Python"

    ``` python
    dvz.path_join(
        visual,  # the visual (LP_DvzVisual)
        join,  # the join (DvzJoinType)
    )
    ```

=== "C"

    ``` c
    void dvz_path_join(
        DvzVisual* visual,  // the visual
        DvzJoinType join,  // the join
    );
    ```

### `dvz_path_linewidth()`

Set the path line width (may be variable along a path).

=== "Python"

    ``` python
    dvz.path_linewidth(
        visual,  # the visual (LP_DvzVisual)
        first,  # the index of the first item to update (int, 32-bit unsigned)
        count,  # the number of items to update (int, 32-bit unsigned)
        values,  # the line width of the vertex, in pixels (ndpointer_<f4_C_CONTIGUOUS)
        flags,  # the data update flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    void dvz_path_linewidth(
        DvzVisual* visual,  // the visual
        uint32_t first,  // the index of the first item to update
        uint32_t count,  // the number of items to update
        float* values,  // the line width of the vertex, in pixels
        int flags,  // the data update flags
    );
    ```

### `dvz_path_position()`

Set the path positions. Note: all path point positions must be updated at once for now.

=== "Python"

    ``` python
    dvz.path_position(
        visual,  # the visual (LP_DvzVisual)
        vertex_count,  # the total number of points across all paths (int, 32-bit unsigned)
        positions,  # the path point positions (ndpointer_<f4_C_CONTIGUOUS)
        path_count,  # the number of different paths (int, 32-bit unsigned)
        path_lengths,  # the number of points in each path (ndpointer_<u4_C_CONTIGUOUS)
        flags,  # the data update flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    void dvz_path_position(
        DvzVisual* visual,  // the visual
         vertex_count,  // the total number of points across all paths
        vec3* positions,  // the path point positions
        uint32_t path_count,  // the number of different paths
        uint32_t* path_lengths,  // the number of points in each path
        int flags,  // the data update flags
    );
    ```

### `dvz_pixel()`

Create a pixel visual.

=== "Python"

    ``` python
    dvz.pixel(  # returns: the visual (LP_DvzVisual)
        batch,  # the batch (LP_DvzBatch)
        flags,  # the visual creation flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    DvzVisual* dvz_pixel(  // returns: the visual
        DvzBatch* batch,  // the batch
        int flags,  // the visual creation flags
    );
    ```

### `dvz_pixel_alloc()`

Allocate memory for a visual.

=== "Python"

    ``` python
    dvz.pixel_alloc(
        visual,  # the visual (LP_DvzVisual)
        item_count,  # the total number of items to allocate for this visual (int, 32-bit unsigned)
    )
    ```

=== "C"

    ``` c
    void dvz_pixel_alloc(
        DvzVisual* visual,  // the visual
        uint32_t item_count,  // the total number of items to allocate for this visual
    );
    ```

### `dvz_pixel_color()`

Set the pixel colors.

=== "Python"

    ``` python
    dvz.pixel_color(
        visual,  # the visual (LP_DvzVisual)
        first,  # the index of the first item to update (int, 32-bit unsigned)
        count,  # the number of items to update (int, 32-bit unsigned)
        values,  # the colors of the items to update (ndpointer_|u1_C_CONTIGUOUS)
        flags,  # the data update flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    void dvz_pixel_color(
        DvzVisual* visual,  // the visual
        uint32_t first,  // the index of the first item to update
        uint32_t count,  // the number of items to update
        DvzColor* values,  // the colors of the items to update
        int flags,  // the data update flags
    );
    ```

### `dvz_pixel_position()`

Set the pixel positions.

=== "Python"

    ``` python
    dvz.pixel_position(
        visual,  # the visual (LP_DvzVisual)
        first,  # the index of the first item to update (int, 32-bit unsigned)
        count,  # the number of items to update (int, 32-bit unsigned)
        values,  # the 3D positions of the items to update (ndpointer_<f4_C_CONTIGUOUS)
        flags,  # the data update flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    void dvz_pixel_position(
        DvzVisual* visual,  // the visual
        uint32_t first,  // the index of the first item to update
        uint32_t count,  // the number of items to update
        vec3* values,  // the 3D positions of the items to update
        int flags,  // the data update flags
    );
    ```

### `dvz_point()`

Create a point visual.

=== "Python"

    ``` python
    dvz.point(  # returns: the visual (LP_DvzVisual)
        batch,  # the batch (LP_DvzBatch)
        flags,  # the visual creation flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    DvzVisual* dvz_point(  // returns: the visual
        DvzBatch* batch,  // the batch
        int flags,  // the visual creation flags
    );
    ```

### `dvz_point_alloc()`

Allocate memory for a visual.

=== "Python"

    ``` python
    dvz.point_alloc(
        visual,  # the visual (LP_DvzVisual)
        item_count,  # the total number of items to allocate for this visual (int, 32-bit unsigned)
    )
    ```

=== "C"

    ``` c
    void dvz_point_alloc(
        DvzVisual* visual,  // the visual
        uint32_t item_count,  // the total number of items to allocate for this visual
    );
    ```

### `dvz_point_color()`

Set the point colors.

=== "Python"

    ``` python
    dvz.point_color(
        visual,  # the visual (LP_DvzVisual)
        first,  # the index of the first item to update (int, 32-bit unsigned)
        count,  # the number of items to update (int, 32-bit unsigned)
        values,  # the colors of the items to update (ndpointer_|u1_C_CONTIGUOUS)
        flags,  # the data update flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    void dvz_point_color(
        DvzVisual* visual,  // the visual
        uint32_t first,  // the index of the first item to update
        uint32_t count,  // the number of items to update
        DvzColor* values,  // the colors of the items to update
        int flags,  // the data update flags
    );
    ```

### `dvz_point_position()`

Set the point positions.

=== "Python"

    ``` python
    dvz.point_position(
        visual,  # the visual (LP_DvzVisual)
        first,  # the index of the first item to update (int, 32-bit unsigned)
        count,  # the number of items to update (int, 32-bit unsigned)
        values,  # the 3D positions of the items to update (ndpointer_<f4_C_CONTIGUOUS)
        flags,  # the data update flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    void dvz_point_position(
        DvzVisual* visual,  // the visual
        uint32_t first,  // the index of the first item to update
        uint32_t count,  // the number of items to update
        vec3* values,  // the 3D positions of the items to update
        int flags,  // the data update flags
    );
    ```

### `dvz_point_size()`

Set the point sizes.

=== "Python"

    ``` python
    dvz.point_size(
        visual,  # the visual (LP_DvzVisual)
        first,  # the index of the first item to update (int, 32-bit unsigned)
        count,  # the number of items to update (int, 32-bit unsigned)
        values,  # the sizes of the items to update (ndpointer_<f4_C_CONTIGUOUS)
        flags,  # the data update flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    void dvz_point_size(
        DvzVisual* visual,  // the visual
        uint32_t first,  // the index of the first item to update
        uint32_t count,  // the number of items to update
        float* values,  // the sizes of the items to update
        int flags,  // the data update flags
    );
    ```

### `dvz_qt_app()`

Placeholder.

=== "Python"

    ``` python
    dvz.qt_app(
        placeholder,  # placeholder (LP_QApplication)
    )
    ```

=== "C"

    ``` c
    DvzQtApp* dvz_qt_app(
         placeholder,  // placeholder
    );
    ```

### `dvz_qt_app_destroy()`

Placeholder.

=== "Python"

    ``` python
    dvz.qt_app_destroy(
        placeholder,  # placeholder (LP_DvzQtApp)
    )
    ```

=== "C"

    ``` c
    void dvz_qt_app_destroy(
         placeholder,  // placeholder
    );
    ```

### `dvz_qt_batch()`

Placeholder.

=== "Python"

    ``` python
    dvz.qt_batch(
        placeholder,  # placeholder (LP_DvzQtApp)
    )
    ```

=== "C"

    ``` c
    DvzBatch* dvz_qt_batch(
         placeholder,  // placeholder
    );
    ```

### `dvz_qt_submit()`

Placeholder.

=== "Python"

    ``` python
    dvz.qt_submit(
        placeholder,  # placeholder (LP_DvzQtApp)
    )
    ```

=== "C"

    ``` c
    void dvz_qt_submit(
         placeholder,  // placeholder
    );
    ```

### `dvz_qt_window()`

Placeholder.

=== "Python"

    ``` python
    dvz.qt_window(
        placeholder,  # placeholder (LP_DvzQtApp)
    )
    ```

=== "C"

    ``` c
    DvzQtWindow* dvz_qt_window(
         placeholder,  // placeholder
    );
    ```

### `dvz_ref()`

Create a reference frame (wrapping a 3D box representing the data in its original coordinates).

=== "Python"

    ``` python
    dvz.ref(  # returns: the reference frame (LP_DvzRef)
        flags,  # the flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    DvzRef* dvz_ref(  // returns: the reference frame
        int flags,  // the flags
    );
    ```

### `dvz_ref_destroy()`

Destroy a reference frame.

=== "Python"

    ``` python
    dvz.ref_destroy(
        ref,  # the reference frame (LP_DvzRef)
    )
    ```

=== "C"

    ``` c
    void dvz_ref_destroy(
        DvzRef* ref,  // the reference frame
    );
    ```

### `dvz_ref_expand()`

Expand the reference by ensuring it contains the specified range.

=== "Python"

    ``` python
    dvz.ref_expand(
        ref,  # the reference frame (LP_DvzRef)
        dim,  # the dimension axis (DvzDim)
        vmin,  # the minimum value (float, 32-bit)
        vmax,  # the maximum value (float, 32-bit)
    )
    ```

=== "C"

    ``` c
    void dvz_ref_expand(
        DvzRef* ref,  // the reference frame
        DvzDim dim,  // the dimension axis
        double vmin,  // the minimum value
        double vmax,  // the maximum value
    );
    ```

### `dvz_ref_expand_2D()`

Expand the reference by ensuring it contains the specified 2D data.

=== "Python"

    ``` python
    dvz.ref_expand_2D(
        ref,  # the reference frame (LP_DvzRef)
        count,  # the number of positions (int, 32-bit unsigned)
        pos,  # the 2D positions (ndpointer_<f8_C_CONTIGUOUS)
    )
    ```

=== "C"

    ``` c
    void dvz_ref_expand_2D(
        DvzRef* ref,  // the reference frame
        uint32_t count,  // the number of positions
        dvec2* pos,  // the 2D positions
    );
    ```

### `dvz_ref_expand_3D()`

Expand the reference by ensuring it contains the specified 3D data.

=== "Python"

    ``` python
    dvz.ref_expand_3D(
        ref,  # the reference frame (LP_DvzRef)
        count,  # the number of positions (int, 32-bit unsigned)
        pos,  # the 3D positions (ndpointer_<f8_C_CONTIGUOUS)
    )
    ```

=== "C"

    ``` c
    void dvz_ref_expand_3D(
        DvzRef* ref,  // the reference frame
        uint32_t count,  // the number of positions
        dvec3* pos,  // the 3D positions
    );
    ```

### `dvz_ref_get()`

Get the range on a given axis.

=== "Python"

    ``` python
    dvz.ref_get(
        ref,  # the reference frame (LP_DvzRef)
        dim,  # the dimension axis (DvzDim)
    )
    ```

=== "C"

    ``` c
    void dvz_ref_get(
        DvzRef* ref,  // the reference frame
        DvzDim dim,  // the dimension axis
    );
    ```

### `dvz_ref_inverse()`

Inverse transform from normalized device coordinates [-1..+1] to the reference frame.

=== "Python"

    ``` python
    dvz.ref_inverse(
        ref,  # the reference frame (LP_DvzRef)
        pos_tr,  # the 3D position in normalized device coordinates (vec3)
    )
    ```

=== "C"

    ``` c
    void dvz_ref_inverse(
        DvzRef* ref,  // the reference frame
        vec3 pos_tr,  // the 3D position in normalized device coordinates
    );
    ```

### `dvz_ref_set()`

Set the range on a given axis.

=== "Python"

    ``` python
    dvz.ref_set(
        ref,  # the reference frame (LP_DvzRef)
        dim,  # the dimension axis (DvzDim)
        vmin,  # the minimum value (float, 32-bit)
        vmax,  # the maximum value (float, 32-bit)
    )
    ```

=== "C"

    ``` c
    void dvz_ref_set(
        DvzRef* ref,  // the reference frame
        DvzDim dim,  // the dimension axis
        double vmin,  // the minimum value
        double vmax,  // the maximum value
    );
    ```

### `dvz_ref_transform1D()`

Transform 1D data from the reference frame to normalized device coordinates [-1..+1].

=== "Python"

    ``` python
    dvz.ref_transform1D(
        ref,  # the reference frame (LP_DvzRef)
        dim,  # which dimension (DvzDim)
        count,  # the number of positions (int, 32-bit unsigned)
        pos,  # the 1D positions (ndpointer_<f8_C_CONTIGUOUS)
    )
    ```

=== "C"

    ``` c
    void dvz_ref_transform1D(
        DvzRef* ref,  // the reference frame
        DvzDim dim,  // which dimension
        uint32_t count,  // the number of positions
        double* pos,  // the 1D positions
    );
    ```

### `dvz_ref_transform_2D()`

Transform 2D data from the reference frame to normalized device coordinates [-1..+1].

=== "Python"

    ``` python
    dvz.ref_transform_2D(
        ref,  # the reference frame (LP_DvzRef)
        count,  # the number of positions (int, 32-bit unsigned)
        pos,  # the 2D positions (ndpointer_<f8_C_CONTIGUOUS)
    )
    ```

=== "C"

    ``` c
    void dvz_ref_transform_2D(
        DvzRef* ref,  // the reference frame
        uint32_t count,  // the number of positions
        dvec2* pos,  // the 2D positions
    );
    ```

### `dvz_ref_transform_3D()`

Transform 3D data from the reference frame to normalized device coordinates [-1..+1].

=== "Python"

    ``` python
    dvz.ref_transform_3D(
        ref,  # the reference frame (LP_DvzRef)
        count,  # the number of positions (int, 32-bit unsigned)
        pos,  # the 3D positions (ndpointer_<f8_C_CONTIGUOUS)
    )
    ```

=== "C"

    ``` c
    void dvz_ref_transform_3D(
        DvzRef* ref,  // the reference frame
        uint32_t count,  // the number of positions
        dvec3* pos,  // the 3D positions
    );
    ```

### `dvz_ref_transform_polygon()`

Transform 2D data from the reference frame to normalized device coordinates [-1..+1] in 2D.

=== "Python"

    ``` python
    dvz.ref_transform_polygon(
        ref,  # the reference frame (LP_DvzRef)
        count,  # the number of positions (int, 32-bit unsigned)
        pos,  # the 2D positions (ndpointer_<f8_C_CONTIGUOUS)
    )
    ```

=== "C"

    ``` c
    void dvz_ref_transform_polygon(
        DvzRef* ref,  // the reference frame
        uint32_t count,  // the number of positions
        dvec2* pos,  // the 2D positions
    );
    ```

### `dvz_resample()`

Normalize a value in an interval.

=== "Python"

    ``` python
    dvz.resample(  # returns: the normalized value between 0 and 1 (c_double)
        t0,  # the interval start (float, 32-bit)
        t1,  # the interval end (float, 32-bit)
        t,  # the value within the interval (float, 32-bit)
    )
    ```

=== "C"

    ``` c
    double dvz_resample(  // returns: the normalized value between 0 and 1
        double t0,  // the interval start
        double t1,  // the interval end
        double t,  // the value within the interval
    );
    ```

### `dvz_rgb_to_rgba_char()`

Convert an RGB byte texture to an RGBA one.

=== "Python"

    ``` python
    dvz.rgb_to_rgba_char(  # returns: the RGBA texture (ndpointer_|u1_C_CONTIGUOUS)
        count,  # the number of pixels (and NOT the number of bytes) in the byte texture (int, 32-bit unsigned)
        rgb,  # the RGB texture (ndpointer_|u1_C_CONTIGUOUS)
    )
    ```

=== "C"

    ``` c
    uint8_t* dvz_rgb_to_rgba_char(  // returns: the RGBA texture
        uint32_t count,  // the number of pixels (and NOT the number of bytes) in the byte texture
        uint8_t* rgb,  // the RGB texture
    );
    ```

### `dvz_rgb_to_rgba_float()`

Convert an RGB float texture to an RGBA one.

=== "Python"

    ``` python
    dvz.rgb_to_rgba_float(  # returns: the RGBA texture (ndpointer_<f4_C_CONTIGUOUS)
        count,  # the number of pixels (and NOT the number of bytes) in the float texture (int, 32-bit unsigned)
        rgb,  # the RGB texture (ndpointer_<f4_C_CONTIGUOUS)
    )
    ```

=== "C"

    ``` c
    float* dvz_rgb_to_rgba_float(  // returns: the RGBA texture
        uint32_t count,  // the number of pixels (and NOT the number of bytes) in the float texture
        float* rgb,  // the RGB texture
    );
    ```

### `dvz_scene()`

Create a scene.

=== "Python"

    ``` python
    dvz.scene(  # returns: the scene (LP_DvzScene)
        batch,  # the batch (LP_DvzBatch)
    )
    ```

=== "C"

    ``` c
    DvzScene* dvz_scene(  // returns: the scene
        DvzBatch* batch,  // the batch
    );
    ```

### `dvz_scene_batch()`

Return the batch from a scene.

=== "Python"

    ``` python
    dvz.scene_batch(  # returns: the batch (LP_DvzBatch)
        scene,  # the scene (LP_DvzScene)
    )
    ```

=== "C"

    ``` c
    DvzBatch* dvz_scene_batch(  // returns: the batch
        DvzScene* scene,  // the scene
    );
    ```

### `dvz_scene_destroy()`

Destroy a scene.

=== "Python"

    ``` python
    dvz.scene_destroy(
        scene,  # the scene (LP_DvzScene)
    )
    ```

=== "C"

    ``` c
    void dvz_scene_destroy(
        DvzScene* scene,  // the scene
    );
    ```

### `dvz_scene_figure()`

Get a figure from its id.

=== "Python"

    ``` python
    dvz.scene_figure(  # returns: the figure (LP_DvzFigure)
        scene,  # the scene (LP_DvzScene)
        id,  # the figure id (int, 64-bit unsigned)
    )
    ```

=== "C"

    ``` c
    DvzFigure* dvz_scene_figure(  // returns: the figure
        DvzScene* scene,  // the scene
        DvzId id,  // the figure id
    );
    ```

### `dvz_scene_mouse()`

Manually pass a mouse event to the scene.

=== "Python"

    ``` python
    dvz.scene_mouse(
        scene,  # the scene (LP_DvzScene)
        fig,  # the figure (LP_DvzFigure)
        ev,  # the mouse event (DvzMouseEvent)
    )
    ```

=== "C"

    ``` c
    void dvz_scene_mouse(
        DvzScene* scene,  // the scene
        DvzFigure* fig,  // the figure
        DvzMouseEvent ev,  // the mouse event
    );
    ```

### `dvz_scene_render()`

Placeholder.

=== "Python"

    ``` python
    dvz.scene_render(
        placeholder,  # placeholder (LP_DvzScene)
    )
    ```

=== "C"

    ``` c
    void dvz_scene_render(
         placeholder,  // placeholder
    );
    ```

### `dvz_scene_run()`

Start the event loop and render the scene in a window.

=== "Python"

    ``` python
    dvz.scene_run(
        scene,  # the scene (LP_DvzScene)
        app,  # the app (LP_DvzApp)
        n_frames,  # the maximum number of frames, 0 for infinite loop (int, 64-bit unsigned)
    )
    ```

=== "C"

    ``` c
    void dvz_scene_run(
        DvzScene* scene,  // the scene
        DvzApp* app,  // the app
        uint64_t n_frames,  // the maximum number of frames, 0 for infinite loop
    );
    ```

### `dvz_sdf_from_svg()`

Generate an SDF from an SVG path.

=== "Python"

    ``` python
    dvz.sdf_from_svg(  # returns: the generated texture as RGB floats (ndpointer_<f4_C_CONTIGUOUS)
        svg_path,  # the SVG path (CStringBuffer)
        width,  # the width of the generated SDF, in pixels (int, 32-bit unsigned)
        height,  # the height of the generated SDF, in pixels (int, 32-bit unsigned)
    )
    ```

=== "C"

    ``` c
    float* dvz_sdf_from_svg(  // returns: the generated texture as RGB floats
        char* svg_path,  // the SVG path
        uint32_t width,  // the width of the generated SDF, in pixels
        uint32_t height,  // the height of the generated SDF, in pixels
    );
    ```

### `dvz_sdf_to_rgb()`

Convert an SDF float texture to a byte texture.

=== "Python"

    ``` python
    dvz.sdf_to_rgb(  # returns: the byte texture (ndpointer_|u1_C_CONTIGUOUS)
        sdf,  # the SDF float texture (ndpointer_<f4_C_CONTIGUOUS)
        width,  # the width of the texture (int, 32-bit unsigned)
        height,  # the height of the texture (int, 32-bit unsigned)
    )
    ```

=== "C"

    ``` c
    uint8_t* dvz_sdf_to_rgb(  // returns: the byte texture
        float* sdf,  // the SDF float texture
        uint32_t width,  // the width of the texture
        uint32_t height,  // the height of the texture
    );
    ```

### `dvz_segment()`

Create a segment visual.

=== "Python"

    ``` python
    dvz.segment(  # returns: the visual (LP_DvzVisual)
        batch,  # the batch (LP_DvzBatch)
        flags,  # the visual creation flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    DvzVisual* dvz_segment(  // returns: the visual
        DvzBatch* batch,  // the batch
        int flags,  // the visual creation flags
    );
    ```

### `dvz_segment_alloc()`

Allocate memory for a visual.

=== "Python"

    ``` python
    dvz.segment_alloc(
        visual,  # the visual (LP_DvzVisual)
        item_count,  # the total number of items to allocate for this visual (int, 32-bit unsigned)
    )
    ```

=== "C"

    ``` c
    void dvz_segment_alloc(
        DvzVisual* visual,  // the visual
        uint32_t item_count,  // the total number of items to allocate for this visual
    );
    ```

### `dvz_segment_cap()`

Set the segment cap types.

=== "Python"

    ``` python
    dvz.segment_cap(
        visual,  # the visual (LP_DvzVisual)
        first,  # the index of the first item to update (int, 32-bit unsigned)
        count,  # the number of items to update (int, 32-bit unsigned)
        initial,  # the initial segment cap types (ndpointer_<i4_C_CONTIGUOUS)
        terminal,  # the terminal segment cap types (ndpointer_<i4_C_CONTIGUOUS)
        flags,  # the data update flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    void dvz_segment_cap(
        DvzVisual* visual,  // the visual
        uint32_t first,  // the index of the first item to update
        uint32_t count,  // the number of items to update
        DvzCapType* initial,  // the initial segment cap types
        DvzCapType* terminal,  // the terminal segment cap types
        int flags,  // the data update flags
    );
    ```

### `dvz_segment_color()`

Set the segment colors.

=== "Python"

    ``` python
    dvz.segment_color(
        visual,  # the visual (LP_DvzVisual)
        first,  # the index of the first item to update (int, 32-bit unsigned)
        count,  # the number of items to update (int, 32-bit unsigned)
        values,  # the colors of the items to update (ndpointer_|u1_C_CONTIGUOUS)
        flags,  # the data update flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    void dvz_segment_color(
        DvzVisual* visual,  // the visual
        uint32_t first,  // the index of the first item to update
        uint32_t count,  // the number of items to update
        DvzColor* values,  // the colors of the items to update
        int flags,  // the data update flags
    );
    ```

### `dvz_segment_linewidth()`

Set the segment line widths.

=== "Python"

    ``` python
    dvz.segment_linewidth(
        visual,  # the visual (LP_DvzVisual)
        first,  # the index of the first item to update (int, 32-bit unsigned)
        count,  # the number of items to update (int, 32-bit unsigned)
        values,  # the segment line widths (ndpointer_<f4_C_CONTIGUOUS)
        flags,  # the data update flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    void dvz_segment_linewidth(
        DvzVisual* visual,  // the visual
        uint32_t first,  // the index of the first item to update
        uint32_t count,  // the number of items to update
        float* values,  // the segment line widths
        int flags,  // the data update flags
    );
    ```

### `dvz_segment_position()`

Set the segment positions.

=== "Python"

    ``` python
    dvz.segment_position(
        visual,  # the visual (LP_DvzVisual)
        first,  # the index of the first item to update (int, 32-bit unsigned)
        count,  # the number of items to update (int, 32-bit unsigned)
        initial,  # the initial 3D positions of the segments (ndpointer_<f4_C_CONTIGUOUS)
        terminal,  # the terminal 3D positions of the segments (ndpointer_<f4_C_CONTIGUOUS)
        flags,  # the data update flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    void dvz_segment_position(
        DvzVisual* visual,  // the visual
        uint32_t first,  // the index of the first item to update
        uint32_t count,  // the number of items to update
        vec3* initial,  // the initial 3D positions of the segments
        vec3* terminal,  // the terminal 3D positions of the segments
        int flags,  // the data update flags
    );
    ```

### `dvz_segment_shift()`

Set the segment shift.

=== "Python"

    ``` python
    dvz.segment_shift(
        visual,  # the visual (LP_DvzVisual)
        first,  # the index of the first item to update (int, 32-bit unsigned)
        count,  # the number of items to update (int, 32-bit unsigned)
        values,  # the dx0,dy0,dx1,dy1 shift quadriplets of the segments to update (ndpointer_<f4_C_CONTIGUOUS)
        flags,  # the data update flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    void dvz_segment_shift(
        DvzVisual* visual,  // the visual
        uint32_t first,  // the index of the first item to update
        uint32_t count,  // the number of items to update
        vec4* values,  // the dx0,dy0,dx1,dy1 shift quadriplets of the segments to update
        int flags,  // the data update flags
    );
    ```

### `dvz_server()`

Placeholder.

=== "Python"

    ``` python
    dvz.server(
        placeholder,  # placeholder (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    DvzServer* dvz_server(
         placeholder,  // placeholder
    );
    ```

### `dvz_server_destroy()`

Placeholder.

=== "Python"

    ``` python
    dvz.server_destroy(
        placeholder,  # placeholder (LP_DvzServer)
    )
    ```

=== "C"

    ``` c
    void dvz_server_destroy(
         placeholder,  // placeholder
    );
    ```

### `dvz_server_grab()`

Placeholder.

=== "Python"

    ``` python
    dvz.server_grab(
        placeholder,  # placeholder (LP_DvzServer)
    )
    ```

=== "C"

    ``` c
    uint8_t* dvz_server_grab(
         placeholder,  // placeholder
    );
    ```

### `dvz_server_keyboard()`

Placeholder.

=== "Python"

    ``` python
    dvz.server_keyboard(
        placeholder,  # placeholder (LP_DvzServer)
    )
    ```

=== "C"

    ``` c
    DvzKeyboard* dvz_server_keyboard(
         placeholder,  // placeholder
    );
    ```

### `dvz_server_mouse()`

Placeholder.

=== "Python"

    ``` python
    dvz.server_mouse(
        placeholder,  # placeholder (LP_DvzServer)
    )
    ```

=== "C"

    ``` c
    DvzMouse* dvz_server_mouse(
         placeholder,  // placeholder
    );
    ```

### `dvz_server_resize()`

Placeholder.

=== "Python"

    ``` python
    dvz.server_resize(
        placeholder,  # placeholder (LP_DvzServer)
    )
    ```

=== "C"

    ``` c
    void dvz_server_resize(
         placeholder,  // placeholder
    );
    ```

### `dvz_server_submit()`

Placeholder.

=== "Python"

    ``` python
    dvz.server_submit(
        placeholder,  # placeholder (LP_DvzServer)
    )
    ```

=== "C"

    ``` c
    void dvz_server_submit(
         placeholder,  // placeholder
    );
    ```

### `dvz_shape()`

Create a shape out of an array of vertices and faces.

=== "Python"

    ``` python
    dvz.shape(
        vertex_count,  # number of vertices (int, 32-bit unsigned)
        positions,  # 3D positions of the vertices (ndpointer_<f4_C_CONTIGUOUS)
        normals,  # normal vectors (optional, will be otherwise computed automatically) (ndpointer_<f4_C_CONTIGUOUS)
        colors,  # vertex vectors (optional) (ndpointer_|u1_C_CONTIGUOUS)
        texcoords,  # texture uv*a coordinates (optional) (ndpointer_<f4_C_CONTIGUOUS)
        index_count,  # number of indices (3x the number of triangular faces) (int, 32-bit unsigned)
        indices,  # vertex indices, three per face (ndpointer_<u4_C_CONTIGUOUS)
    )
    ```

=== "C"

    ``` c
    DvzShape dvz_shape(
        uint32_t vertex_count,  // number of vertices
        vec3* positions,  // 3D positions of the vertices
        vec3* normals,  // normal vectors (optional, will be otherwise computed automatically)
        DvzColor* colors,  // vertex vectors (optional)
        vec4* texcoords,  // texture uv*a coordinates (optional)
        uint32_t index_count,  // number of indices (3x the number of triangular faces)
        DvzIndex* indices,  // vertex indices, three per face
    );
    ```

### `dvz_shape_begin()`

Start a transformation sequence.

=== "Python"

    ``` python
    dvz.shape_begin(
        shape,  # the shape (LP_DvzShape)
        first,  # the first vertex to modify (int, 32-bit unsigned)
        count,  # the number of vertices to modify (int, 32-bit unsigned)
    )
    ```

=== "C"

    ``` c
    void dvz_shape_begin(
        DvzShape* shape,  // the shape
        uint32_t first,  // the first vertex to modify
        uint32_t count,  // the number of vertices to modify
    );
    ```

### `dvz_shape_cone()`

Create a cone shape.

=== "Python"

    ``` python
    dvz.shape_cone(  # returns: the shape (DvzShape)
        count,  # the number of points along the disc border (int, 32-bit unsigned)
        color,  # the cone color (cvec4)
    )
    ```

=== "C"

    ``` c
    DvzShape dvz_shape_cone(  // returns: the shape
        uint32_t count,  // the number of points along the disc border
        DvzColor color,  // the cone color
    );
    ```

### `dvz_shape_cube()`

Create a cube shape.

=== "Python"

    ``` python
    dvz.shape_cube(  # returns: the shape (DvzShape)
        colors,  # the colors of the six faces (ndpointer_|u1_C_CONTIGUOUS)
    )
    ```

=== "C"

    ``` c
    DvzShape dvz_shape_cube(  // returns: the shape
        DvzColor* colors,  // the colors of the six faces
    );
    ```

### `dvz_shape_cylinder()`

Create a cylinder shape.

=== "Python"

    ``` python
    dvz.shape_cylinder(  # returns: the shape (DvzShape)
        count,  # the number of points along the cylinder border (int, 32-bit unsigned)
        color,  # the cylinder color (cvec4)
    )
    ```

=== "C"

    ``` c
    DvzShape dvz_shape_cylinder(  // returns: the shape
        uint32_t count,  // the number of points along the cylinder border
        DvzColor color,  // the cylinder color
    );
    ```

### `dvz_shape_destroy()`

Destroy a shape.

=== "Python"

    ``` python
    dvz.shape_destroy(
        shape,  # the shape (LP_DvzShape)
    )
    ```

=== "C"

    ``` c
    void dvz_shape_destroy(
        DvzShape* shape,  // the shape
    );
    ```

### `dvz_shape_disc()`

Create a disc shape.

=== "Python"

    ``` python
    dvz.shape_disc(  # returns: the shape (DvzShape)
        count,  # the number of points along the disc border (int, 32-bit unsigned)
        color,  # the disc color (cvec4)
    )
    ```

=== "C"

    ``` c
    DvzShape dvz_shape_disc(  // returns: the shape
        uint32_t count,  // the number of points along the disc border
        DvzColor color,  // the disc color
    );
    ```

### `dvz_shape_dodecahedron()`

Create a dodecahedron.

=== "Python"

    ``` python
    dvz.shape_dodecahedron(  # returns: the shape (DvzShape)
        color,  # the color (cvec4)
    )
    ```

=== "C"

    ``` c
    DvzShape dvz_shape_dodecahedron(  // returns: the shape
        DvzColor color,  // the color
    );
    ```

### `dvz_shape_end()`

Apply the transformation sequence and reset it.

=== "Python"

    ``` python
    dvz.shape_end(
        shape,  # the shape (LP_DvzShape)
    )
    ```

=== "C"

    ``` c
    void dvz_shape_end(
        DvzShape* shape,  // the shape
    );
    ```

### `dvz_shape_hexahedron()`

Create a tetrahedron.

=== "Python"

    ``` python
    dvz.shape_hexahedron(  # returns: the shape (DvzShape)
        color,  # the color (cvec4)
    )
    ```

=== "C"

    ``` c
    DvzShape dvz_shape_hexahedron(  // returns: the shape
        DvzColor color,  // the color
    );
    ```

### `dvz_shape_icosahedron()`

Create a icosahedron.

=== "Python"

    ``` python
    dvz.shape_icosahedron(  # returns: the shape (DvzShape)
        color,  # the color (cvec4)
    )
    ```

=== "C"

    ``` c
    DvzShape dvz_shape_icosahedron(  // returns: the shape
        DvzColor color,  // the color
    );
    ```

### `dvz_shape_merge()`

Merge several shapes.

=== "Python"

    ``` python
    dvz.shape_merge(  # returns: the merged shape (DvzShape)
        count,  # the number of shapes to merge (int, 32-bit unsigned)
        shapes,  # the shapes to merge (LP_DvzShape)
    )
    ```

=== "C"

    ``` c
    DvzShape dvz_shape_merge(  // returns: the merged shape
        uint32_t count,  // the number of shapes to merge
        DvzShape* shapes,  // the shapes to merge
    );
    ```

### `dvz_shape_normalize()`

Normalize a shape.

=== "Python"

    ``` python
    dvz.shape_normalize(
        shape,  # the shape (LP_DvzShape)
    )
    ```

=== "C"

    ``` c
    void dvz_shape_normalize(
        DvzShape* shape,  // the shape
    );
    ```

### `dvz_shape_normals()`

Recompute the face normals.

=== "Python"

    ``` python
    dvz.shape_normals(
        shape,  # the shape (LP_DvzShape)
    )
    ```

=== "C"

    ``` c
    void dvz_shape_normals(
        DvzShape* shape,  // the shape
    );
    ```

### `dvz_shape_obj()`

Load a .obj shape.

=== "Python"

    ``` python
    dvz.shape_obj(  # returns: the shape (DvzShape)
        file_path,  # the path to the .obj file (CStringBuffer)
    )
    ```

=== "C"

    ``` c
    DvzShape dvz_shape_obj(  // returns: the shape
        char* file_path,  // the path to the .obj file
    );
    ```

### `dvz_shape_octahedron()`

Create a octahedron.

=== "Python"

    ``` python
    dvz.shape_octahedron(  # returns: the shape (DvzShape)
        color,  # the color (cvec4)
    )
    ```

=== "C"

    ``` c
    DvzShape dvz_shape_octahedron(  // returns: the shape
        DvzColor color,  // the color
    );
    ```

### `dvz_shape_polygon()`

Create a polygon shape using the simple earcut polygon triangulation algorithm.

=== "Python"

    ``` python
    dvz.shape_polygon(  # returns: the shape (DvzShape)
        count,  # the number of points along the polygon border (int, 32-bit unsigned)
        points,  # the points 2D coordinates (ndpointer_<f8_C_CONTIGUOUS)
        color,  # the polygon color (cvec4)
    )
    ```

=== "C"

    ``` c
    DvzShape dvz_shape_polygon(  // returns: the shape
        uint32_t count,  // the number of points along the polygon border
        dvec2* points,  // the points 2D coordinates
        DvzColor color,  // the polygon color
    );
    ```

### `dvz_shape_print()`

Show information about a shape.

=== "Python"

    ``` python
    dvz.shape_print(
        shape,  # the shape (LP_DvzShape)
    )
    ```

=== "C"

    ``` c
    void dvz_shape_print(
        DvzShape* shape,  // the shape
    );
    ```

### `dvz_shape_rescaling()`

Compute the rescaling factor to renormalize a shape.

=== "Python"

    ``` python
    dvz.shape_rescaling(
        shape,  # the shape (LP_DvzShape)
        flags,  # the rescaling flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    float dvz_shape_rescaling(
        DvzShape* shape,  // the shape
        int flags,  // the rescaling flags
    );
    ```

### `dvz_shape_rotate()`

Append a rotation to a shape.

=== "Python"

    ``` python
    dvz.shape_rotate(
        shape,  # the shape (LP_DvzShape)
        angle,  # the rotation angle (float, 64-bit)
        axis,  # the rotation axis (vec3)
    )
    ```

=== "C"

    ``` c
    void dvz_shape_rotate(
        DvzShape* shape,  // the shape
        float angle,  // the rotation angle
        vec3 axis,  // the rotation axis
    );
    ```

### `dvz_shape_scale()`

Append a scaling transform to a shape.

=== "Python"

    ``` python
    dvz.shape_scale(
        shape,  # the shape (LP_DvzShape)
        scale,  # the scaling factors (vec3)
    )
    ```

=== "C"

    ``` c
    void dvz_shape_scale(
        DvzShape* shape,  // the shape
        vec3 scale,  // the scaling factors
    );
    ```

### `dvz_shape_sphere()`

Create a sphere shape.

=== "Python"

    ``` python
    dvz.shape_sphere(  # returns: the shape (DvzShape)
        rows,  # the number of rows (int, 32-bit unsigned)
        cols,  # the number of columns (int, 32-bit unsigned)
        color,  # the sphere color (cvec4)
    )
    ```

=== "C"

    ``` c
    DvzShape dvz_shape_sphere(  // returns: the shape
        uint32_t rows,  // the number of rows
        uint32_t cols,  // the number of columns
        DvzColor color,  // the sphere color
    );
    ```

### `dvz_shape_square()`

Create a square shape.

=== "Python"

    ``` python
    dvz.shape_square(  # returns: the shape (DvzShape)
        color,  # the square color (cvec4)
    )
    ```

=== "C"

    ``` c
    DvzShape dvz_shape_square(  // returns: the shape
        DvzColor color,  // the square color
    );
    ```

### `dvz_shape_surface()`

Create a grid shape.

=== "Python"

    ``` python
    dvz.shape_surface(  # returns: the shape (DvzShape)
        row_count,  # number of rows (int, 32-bit unsigned)
        col_count,  # number of cols (int, 32-bit unsigned)
        heights,  # a pointer to row_count*col_count height values (floats) (ndpointer_<f4_C_CONTIGUOUS)
        colors,  # a pointer to row_count*col_count color values (cvec4 or vec4) (ndpointer_|u1_C_CONTIGUOUS)
        o,  # the origin (vec3)
        u,  # the unit vector parallel to each column (vec3)
        v,  # the unit vector parallel to each row (vec3)
        flags,  # the grid creation flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    DvzShape dvz_shape_surface(  // returns: the shape
        uint32_t row_count,  // number of rows
        uint32_t col_count,  // number of cols
        float* heights,  // a pointer to row_count*col_count height values (floats)
        DvzColor* colors,  // a pointer to row_count*col_count color values (cvec4 or vec4)
        vec3 o,  // the origin
        vec3 u,  // the unit vector parallel to each column
        vec3 v,  // the unit vector parallel to each row
        int flags,  // the grid creation flags
    );
    ```

### `dvz_shape_tetrahedron()`

Create a tetrahedron.

=== "Python"

    ``` python
    dvz.shape_tetrahedron(  # returns: the shape (DvzShape)
        color,  # the color (cvec4)
    )
    ```

=== "C"

    ``` c
    DvzShape dvz_shape_tetrahedron(  // returns: the shape
        DvzColor color,  // the color
    );
    ```

### `dvz_shape_transform()`

Append an arbitrary transformation.

=== "Python"

    ``` python
    dvz.shape_transform(
        shape,  # the shape (LP_DvzShape)
        transform,  # the transform mat4 matrix (mat4)
    )
    ```

=== "C"

    ``` c
    void dvz_shape_transform(
        DvzShape* shape,  // the shape
        mat4 transform,  // the transform mat4 matrix
    );
    ```

### `dvz_shape_translate()`

Append a translation to a shape.

=== "Python"

    ``` python
    dvz.shape_translate(
        shape,  # the shape (LP_DvzShape)
        translate,  # the translation vector (vec3)
    )
    ```

=== "C"

    ``` c
    void dvz_shape_translate(
        DvzShape* shape,  // the shape
        vec3 translate,  // the translation vector
    );
    ```

### `dvz_shape_unindex()`

Convert an indexed shape to a non-indexed one by duplicating the vertex values according

=== "Python"

    ``` python
    dvz.shape_unindex(
        shape,  # the shape (LP_DvzShape)
        flags,  # the flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    void dvz_shape_unindex(
        DvzShape* shape,  // the shape
        int flags,  // the flags
    );
    ```

### `dvz_slice()`

Create a slice visual (multiple 2D images with slices of a 3D texture).

=== "Python"

    ``` python
    dvz.slice(  # returns: the visual (LP_DvzVisual)
        batch,  # the batch (LP_DvzBatch)
        flags,  # the visual creation flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    DvzVisual* dvz_slice(  // returns: the visual
        DvzBatch* batch,  // the batch
        int flags,  // the visual creation flags
    );
    ```

### `dvz_slice_alloc()`

Allocate memory for a visual.

=== "Python"

    ``` python
    dvz.slice_alloc(
        visual,  # the visual (LP_DvzVisual)
        item_count,  # the total number of slices to allocate for this visual (int, 32-bit unsigned)
    )
    ```

=== "C"

    ``` c
    void dvz_slice_alloc(
        DvzVisual* visual,  // the visual
        uint32_t item_count,  // the total number of slices to allocate for this visual
    );
    ```

### `dvz_slice_alpha()`

Set the slice transparency alpha value.

=== "Python"

    ``` python
    dvz.slice_alpha(
        visual,  # the visual (LP_DvzVisual)
        alpha,  # the alpha value (float, 64-bit)
    )
    ```

=== "C"

    ``` c
    void dvz_slice_alpha(
        DvzVisual* visual,  // the visual
        float alpha,  // the alpha value
    );
    ```

### `dvz_slice_position()`

Set the slice positions.

=== "Python"

    ``` python
    dvz.slice_position(
        visual,  # the visual (LP_DvzVisual)
        first,  # the index of the first item to update (int, 32-bit unsigned)
        count,  # the number of items to update (int, 32-bit unsigned)
        p0,  # the 3D positions of the top left corner (ndpointer_<f4_C_CONTIGUOUS)
        p1,  # the 3D positions of the top right corner (ndpointer_<f4_C_CONTIGUOUS)
        p2,  # the 3D positions of the bottom left corner (ndpointer_<f4_C_CONTIGUOUS)
        p3,  # the 3D positions of the bottom right corner (ndpointer_<f4_C_CONTIGUOUS)
        flags,  # the data update flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    void dvz_slice_position(
        DvzVisual* visual,  // the visual
        uint32_t first,  // the index of the first item to update
        uint32_t count,  // the number of items to update
        vec3* p0,  // the 3D positions of the top left corner
        vec3* p1,  // the 3D positions of the top right corner
        vec3* p2,  // the 3D positions of the bottom left corner
        vec3* p3,  // the 3D positions of the bottom right corner
        int flags,  // the data update flags
    );
    ```

### `dvz_slice_texcoords()`

Set the slice texture coordinates.

=== "Python"

    ``` python
    dvz.slice_texcoords(
        visual,  # the visual (LP_DvzVisual)
        first,  # the index of the first item to update (int, 32-bit unsigned)
        count,  # the number of items to update (int, 32-bit unsigned)
        uvw0,  # the 3D texture coordinates of the top left corner (ndpointer_<f4_C_CONTIGUOUS)
        uvw1,  # the 3D texture coordinates of the top right corner (ndpointer_<f4_C_CONTIGUOUS)
        uvw2,  # the 3D texture coordinates of the bottom left corner (ndpointer_<f4_C_CONTIGUOUS)
        uvw3,  # the 3D texture coordinates of the bottom right corner (ndpointer_<f4_C_CONTIGUOUS)
        flags,  # the data update flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    void dvz_slice_texcoords(
        DvzVisual* visual,  // the visual
        uint32_t first,  // the index of the first item to update
        uint32_t count,  // the number of items to update
        vec3* uvw0,  // the 3D texture coordinates of the top left corner
        vec3* uvw1,  // the 3D texture coordinates of the top right corner
        vec3* uvw2,  // the 3D texture coordinates of the bottom left corner
        vec3* uvw3,  // the 3D texture coordinates of the bottom right corner
        int flags,  // the data update flags
    );
    ```

### `dvz_slice_texture()`

Assign a texture to a slice visual.

=== "Python"

    ``` python
    dvz.slice_texture(
        visual,  # the visual (LP_DvzVisual)
        tex,  # the texture ID (int, 64-bit unsigned)
        filter,  # the texture filtering mode (DvzFilter)
        address_mode,  # the texture address mode (DvzSamplerAddressMode)
    )
    ```

=== "C"

    ``` c
    void dvz_slice_texture(
        DvzVisual* visual,  // the visual
        DvzId tex,  // the texture ID
        DvzFilter filter,  // the texture filtering mode
        DvzSamplerAddressMode address_mode,  // the texture address mode
    );
    ```

### `dvz_sphere()`

Create a sphere visual.

=== "Python"

    ``` python
    dvz.sphere(  # returns: the visual (LP_DvzVisual)
        batch,  # the batch (LP_DvzBatch)
        flags,  # the visual creation flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    DvzVisual* dvz_sphere(  // returns: the visual
        DvzBatch* batch,  // the batch
        int flags,  // the visual creation flags
    );
    ```

### `dvz_sphere_alloc()`

Allocate memory for a visual.

=== "Python"

    ``` python
    dvz.sphere_alloc(
        visual,  # the visual (LP_DvzVisual)
        item_count,  # the total number of spheres to allocate for this visual (int, 32-bit unsigned)
    )
    ```

=== "C"

    ``` c
    void dvz_sphere_alloc(
        DvzVisual* visual,  // the visual
        uint32_t item_count,  // the total number of spheres to allocate for this visual
    );
    ```

### `dvz_sphere_color()`

Set the sphere colors.

=== "Python"

    ``` python
    dvz.sphere_color(
        visual,  # the visual (LP_DvzVisual)
        first,  # the index of the first item to update (int, 32-bit unsigned)
        count,  # the number of items to update (int, 32-bit unsigned)
        color,  # the sphere colors (ndpointer_|u1_C_CONTIGUOUS)
        flags,  # the data update flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    void dvz_sphere_color(
        DvzVisual* visual,  // the visual
        uint32_t first,  // the index of the first item to update
        uint32_t count,  // the number of items to update
        DvzColor* color,  // the sphere colors
        int flags,  // the data update flags
    );
    ```

### `dvz_sphere_light_params()`

Set the sphere light parameters.

=== "Python"

    ``` python
    dvz.sphere_light_params(
        visual,  # the visual (LP_DvzVisual)
        params,  # the light parameters (vec4 ambient, diffuse, specular, exponent) (vec4)
    )
    ```

=== "C"

    ``` c
    void dvz_sphere_light_params(
        DvzVisual* visual,  // the visual
        vec4 params,  // the light parameters (vec4 ambient, diffuse, specular, exponent)
    );
    ```

### `dvz_sphere_light_pos()`

Set the sphere light position.

=== "Python"

    ``` python
    dvz.sphere_light_pos(
        visual,  # the visual (LP_DvzVisual)
        pos,  # the light position (vec3)
    )
    ```

=== "C"

    ``` c
    void dvz_sphere_light_pos(
        DvzVisual* visual,  // the visual
        vec3 pos,  // the light position
    );
    ```

### `dvz_sphere_position()`

Set the sphere positions.

=== "Python"

    ``` python
    dvz.sphere_position(
        visual,  # the visual (LP_DvzVisual)
        first,  # the index of the first item to update (int, 32-bit unsigned)
        count,  # the number of items to update (int, 32-bit unsigned)
        pos,  # the 3D positions of the sphere centers (ndpointer_<f4_C_CONTIGUOUS)
        flags,  # the data update flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    void dvz_sphere_position(
        DvzVisual* visual,  // the visual
        uint32_t first,  // the index of the first item to update
        uint32_t count,  // the number of items to update
        vec3* pos,  // the 3D positions of the sphere centers
        int flags,  // the data update flags
    );
    ```

### `dvz_sphere_size()`

Set the sphere sizes.

=== "Python"

    ``` python
    dvz.sphere_size(
        visual,  # the visual (LP_DvzVisual)
        first,  # the index of the first item to update (int, 32-bit unsigned)
        count,  # the number of items to update (int, 32-bit unsigned)
        size,  # the radius of the spheres (ndpointer_<f4_C_CONTIGUOUS)
        flags,  # the data update flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    void dvz_sphere_size(
        DvzVisual* visual,  // the visual
        uint32_t first,  // the index of the first item to update
        uint32_t count,  // the number of items to update
        float* size,  // the radius of the spheres
        int flags,  // the data update flags
    );
    ```

### `dvz_tex_image()`

Create a 2D texture to be used in an image visual.

=== "Python"

    ``` python
    dvz.tex_image(  # returns: the texture ID (c_ulong)
        batch,  # the batch (LP_DvzBatch)
        format,  # the texture format (DvzFormat)
        width,  # the texture width (int, 32-bit unsigned)
        height,  # the texture height (int, 32-bit unsigned)
        data,  # the texture data to upload (ndpointer_any_C_CONTIGUOUS)
        flags,  # the texture creation flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    DvzId dvz_tex_image(  // returns: the texture ID
        DvzBatch* batch,  // the batch
        DvzFormat format,  // the texture format
        uint32_t width,  // the texture width
        uint32_t height,  // the texture height
        void* data,  // the texture data to upload
        int flags,  // the texture creation flags
    );
    ```

### `dvz_tex_slice()`

Create a 3D texture to be used in a slice visual.

=== "Python"

    ``` python
    dvz.tex_slice(  # returns: the texture ID (c_ulong)
        batch,  # the batch (LP_DvzBatch)
        format,  # the texture format (DvzFormat)
        width,  # the texture width (int, 32-bit unsigned)
        height,  # the texture height (int, 32-bit unsigned)
        depth,  # the texture depth (int, 32-bit unsigned)
        data,  # the texture data to upload (ndpointer_any_C_CONTIGUOUS)
    )
    ```

=== "C"

    ``` c
    DvzId dvz_tex_slice(  // returns: the texture ID
        DvzBatch* batch,  // the batch
        DvzFormat format,  // the texture format
        uint32_t width,  // the texture width
        uint32_t height,  // the texture height
        uint32_t depth,  // the texture depth
        void* data,  // the texture data to upload
    );
    ```

### `dvz_tex_volume()`

Create a 3D texture to be used in a volume visual.

=== "Python"

    ``` python
    dvz.tex_volume(  # returns: the texture ID (c_ulong)
        batch,  # the batch (LP_DvzBatch)
        format,  # the texture format (DvzFormat)
        width,  # the texture width (int, 32-bit unsigned)
        height,  # the texture height (int, 32-bit unsigned)
        depth,  # the texture depth (int, 32-bit unsigned)
        data,  # the texture data to upload (ndpointer_any_C_CONTIGUOUS)
    )
    ```

=== "C"

    ``` c
    DvzId dvz_tex_volume(  // returns: the texture ID
        DvzBatch* batch,  // the batch
        DvzFormat format,  // the texture format
        uint32_t width,  // the texture width
        uint32_t height,  // the texture height
        uint32_t depth,  // the texture depth
        void* data,  // the texture data to upload
    );
    ```

### `dvz_version()`

Return the current version string.

=== "Python"

    ``` python
    dvz.version()  # returns: the version string (c_char_p)
    ```

=== "C"

    ``` c
    char* dvz_version();  // returns: the version string
    ```

### `dvz_visual_alloc()`

Allocate a visual.

=== "Python"

    ``` python
    dvz.visual_alloc(
        visual,  # the visual (LP_DvzVisual)
        item_count,  # the number of items (int, 32-bit unsigned)
        vertex_count,  # the number of vertices (int, 32-bit unsigned)
        index_count,  # the number of indices (int, 32-bit unsigned)
    )
    ```

=== "C"

    ``` c
    void dvz_visual_alloc(
        DvzVisual* visual,  // the visual
        uint32_t item_count,  // the number of items
        uint32_t vertex_count,  // the number of vertices
        uint32_t index_count,  // the number of indices
    );
    ```

### `dvz_visual_attr()`

Declare a visual attribute.

=== "Python"

    ``` python
    dvz.visual_attr(
        visual,  # the visual (LP_DvzVisual)
        attr_idx,  # the attribute index (int, 32-bit unsigned)
        offset,  # the attribute offset within the vertex buffer, in bytes (int, 64-bit unsigned)
        item_size,  # the attribute size, in bytes (int, 64-bit unsigned)
        format,  # the attribute data format (DvzFormat)
        flags,  # the attribute flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    void dvz_visual_attr(
        DvzVisual* visual,  // the visual
        uint32_t attr_idx,  // the attribute index
        DvzSize offset,  // the attribute offset within the vertex buffer, in bytes
        DvzSize item_size,  // the attribute size, in bytes
        DvzFormat format,  // the attribute data format
        int flags,  // the attribute flags
    );
    ```

### `dvz_visual_blend()`

Set the blend type of a visual.

=== "Python"

    ``` python
    dvz.visual_blend(
        visual,  # the visual (LP_DvzVisual)
        blend_type,  # the blend type (DvzBlendType)
    )
    ```

=== "C"

    ``` c
    void dvz_visual_blend(
        DvzVisual* visual,  // the visual
        DvzBlendType blend_type,  // the blend type
    );
    ```

### `dvz_visual_clip()`

Set the visual clipping.

=== "Python"

    ``` python
    dvz.visual_clip(
        visual,  # the visual (LP_DvzVisual)
        clip,  # the viewport clipping (DvzViewportClip)
    )
    ```

=== "C"

    ``` c
    void dvz_visual_clip(
        DvzVisual* visual,  // the visual
        DvzViewportClip clip,  // the viewport clipping
    );
    ```

### `dvz_visual_cull()`

Set the cull mode of a visual.

=== "Python"

    ``` python
    dvz.visual_cull(
        visual,  # the visual (LP_DvzVisual)
        cull_mode,  # the cull mode (DvzCullMode)
    )
    ```

=== "C"

    ``` c
    void dvz_visual_cull(
        DvzVisual* visual,  // the visual
        DvzCullMode cull_mode,  // the cull mode
    );
    ```

### `dvz_visual_dat()`

Bind a dat to a visual slot.

=== "Python"

    ``` python
    dvz.visual_dat(
        visual,  # the visual (LP_DvzVisual)
        slot_idx,  # the slot index (int, 32-bit unsigned)
        dat,  # the dat ID (int, 64-bit unsigned)
    )
    ```

=== "C"

    ``` c
    void dvz_visual_dat(
        DvzVisual* visual,  // the visual
        uint32_t slot_idx,  // the slot index
        DvzId dat,  // the dat ID
    );
    ```

### `dvz_visual_data()`

Set visual data.

=== "Python"

    ``` python
    dvz.visual_data(
        visual,  # the visual (LP_DvzVisual)
        attr_idx,  # the attribute index (int, 32-bit unsigned)
        first,  # the index of the first item to set (int, 32-bit unsigned)
        count,  # the number of items to set (int, 32-bit unsigned)
        data,  # a pointer to the data buffer (ndpointer_any_C_CONTIGUOUS)
    )
    ```

=== "C"

    ``` c
    void dvz_visual_data(
        DvzVisual* visual,  // the visual
        uint32_t attr_idx,  // the attribute index
        uint32_t first,  // the index of the first item to set
        uint32_t count,  // the number of items to set
        void* data,  // a pointer to the data buffer
    );
    ```

### `dvz_visual_depth()`

Set the visual depth.

=== "Python"

    ``` python
    dvz.visual_depth(
        visual,  # the visual (LP_DvzVisual)
        depth_test,  # whether to activate the depth test (DvzDepthTest)
    )
    ```

=== "C"

    ``` c
    void dvz_visual_depth(
        DvzVisual* visual,  // the visual
        DvzDepthTest depth_test,  // whether to activate the depth test
    );
    ```

### `dvz_visual_dynamic()`

Declare a dynamic attribute, meaning that it is stored in a separate dat rather than being

=== "Python"

    ``` python
    dvz.visual_dynamic(
        visual,  # the visual (LP_DvzVisual)
        attr_idx,  # the attribute index (int, 32-bit unsigned)
        binding_idx,  # the binding index (0 = common vertex buffer, use 1 or 2, 3... for each (int, 32-bit unsigned)
    )
    ```

=== "C"

    ``` c
    void dvz_visual_dynamic(
        DvzVisual* visual,  // the visual
        uint32_t attr_idx,  // the attribute index
        uint32_t binding_idx,  // the binding index (0 = common vertex buffer, use 1 or 2, 3... for each
    );
    ```

### `dvz_visual_fixed()`

Fix some axes in a visual.

=== "Python"

    ``` python
    dvz.visual_fixed(
        visual,  # the visual (LP_DvzVisual)
        fixed_x,  # whether the x axis should be fixed (bool)
        fixed_y,  # whether the y axis should be fixed (bool)
        fixed_z,  # whether the z axis should be fixed (bool)
    )
    ```

=== "C"

    ``` c
    void dvz_visual_fixed(
        DvzVisual* visual,  // the visual
        bool fixed_x,  // whether the x axis should be fixed
        bool fixed_y,  // whether the y axis should be fixed
        bool fixed_z,  // whether the z axis should be fixed
    );
    ```

### `dvz_visual_front()`

Set the front face mode of a visual.

=== "Python"

    ``` python
    dvz.visual_front(
        visual,  # the visual (LP_DvzVisual)
        front_face,  # the front face mode (DvzFrontFace)
    )
    ```

=== "C"

    ``` c
    void dvz_visual_front(
        DvzVisual* visual,  // the visual
        DvzFrontFace front_face,  // the front face mode
    );
    ```

### `dvz_visual_groups()`

Set groups in a visual.

=== "Python"

    ``` python
    dvz.visual_groups(
        visual,  # the visual (LP_DvzVisual)
        group_count,  # the number of groups (int, 32-bit unsigned)
        group_sizes,  # the size of each group (ndpointer_<u4_C_CONTIGUOUS)
    )
    ```

=== "C"

    ``` c
    void dvz_visual_groups(
        DvzVisual* visual,  // the visual
        uint32_t group_count,  // the number of groups
        uint32_t* group_sizes,  // the size of each group
    );
    ```

### `dvz_visual_index()`

Set the visual index data.

=== "Python"

    ``` python
    dvz.visual_index(
        visual,  # the visual (LP_DvzVisual)
        first,  # the index of the first index to set (int, 32-bit unsigned)
        count,  # the number of indices (int, 32-bit unsigned)
        data,  # a pointer to a buffer of DvzIndex (uint32_t) values with the indices (ndpointer_<u4_C_CONTIGUOUS)
    )
    ```

=== "C"

    ``` c
    void dvz_visual_index(
        DvzVisual* visual,  // the visual
        uint32_t first,  // the index of the first index to set
        uint32_t count,  // the number of indices
        DvzIndex* data,  // a pointer to a buffer of DvzIndex (uint32_t) values with the indices
    );
    ```

### `dvz_visual_param()`

Set a visual parameter value.

=== "Python"

    ``` python
    dvz.visual_param(
        visual,  # the visual (LP_DvzVisual)
        slot_idx,  # the slot index (int, 32-bit unsigned)
        attr_idx,  # the index of the parameter attribute within the params structure (int, 32-bit unsigned)
        item,  # a pointer to the value to use for that parameter (array)
    )
    ```

=== "C"

    ``` c
    void dvz_visual_param(
        DvzVisual* visual,  // the visual
        uint32_t slot_idx,  // the slot index
        uint32_t attr_idx,  // the index of the parameter attribute within the params structure
        void* item,  // a pointer to the value to use for that parameter
    );
    ```

### `dvz_visual_params()`

Declare a set of visual parameters.

=== "Python"

    ``` python
    dvz.visual_params(
        visual,  # the visual (LP_DvzVisual)
        slot_idx,  # the slot index of the uniform buffer storing the parameter values (int, 32-bit unsigned)
        size,  # the size, in bytes, of that uniform buffer (int, 64-bit unsigned)
    )
    ```

=== "C"

    ``` c
    DvzParams* dvz_visual_params(
        DvzVisual* visual,  // the visual
        uint32_t slot_idx,  // the slot index of the uniform buffer storing the parameter values
        DvzSize size,  // the size, in bytes, of that uniform buffer
    );
    ```

### `dvz_visual_polygon()`

Set the polygon mode of a visual.

=== "Python"

    ``` python
    dvz.visual_polygon(
        visual,  # the visual (LP_DvzVisual)
        polygon_mode,  # the polygon mode (DvzPolygonMode)
    )
    ```

=== "C"

    ``` c
    void dvz_visual_polygon(
        DvzVisual* visual,  // the visual
        DvzPolygonMode polygon_mode,  // the polygon mode
    );
    ```

### `dvz_visual_primitive()`

Set the primitive topology of a visual.

=== "Python"

    ``` python
    dvz.visual_primitive(
        visual,  # the visual (LP_DvzVisual)
        primitive,  # the primitive topology (DvzPrimitiveTopology)
    )
    ```

=== "C"

    ``` c
    void dvz_visual_primitive(
        DvzVisual* visual,  // the visual
        DvzPrimitiveTopology primitive,  // the primitive topology
    );
    ```

### `dvz_visual_push()`

Set a push constant of a visual.

=== "Python"

    ``` python
    dvz.visual_push(
        visual,  # the visual (LP_DvzVisual)
        shader_stages,  # the shader stage flags (int, 32-bit signed)
        offset,  # the offset, in bytes (int, 64-bit unsigned)
        size,  # the size, in bytes (int, 64-bit unsigned)
    )
    ```

=== "C"

    ``` c
    void dvz_visual_push(
        DvzVisual* visual,  // the visual
        DvzShaderStageFlags shader_stages,  // the shader stage flags
        DvzSize offset,  // the offset, in bytes
        DvzSize size,  // the size, in bytes
    );
    ```

### `dvz_visual_quads()`

Set visual data as quads.

=== "Python"

    ``` python
    dvz.visual_quads(
        visual,  # the visual (LP_DvzVisual)
        attr_idx,  # the attribute index (int, 32-bit unsigned)
        first,  # the index of the first item to set (int, 32-bit unsigned)
        count,  # the number of items to set (int, 32-bit unsigned)
        tl_br,  # a pointer to a buffer of vec4 with the 2D coordinates of the top-left and (ndpointer_<f4_C_CONTIGUOUS)
    )
    ```

=== "C"

    ``` c
    void dvz_visual_quads(
        DvzVisual* visual,  // the visual
        uint32_t attr_idx,  // the attribute index
        uint32_t first,  // the index of the first item to set
        uint32_t count,  // the number of items to set
        vec4* tl_br,  // a pointer to a buffer of vec4 with the 2D coordinates of the top-left and
    );
    ```

### `dvz_visual_resize()`

Resize a visual allocation.

=== "Python"

    ``` python
    dvz.visual_resize(
        visual,  # the visual (LP_DvzVisual)
        item_count,  # the number of items (int, 32-bit unsigned)
        vertex_count,  # the number of vertices (int, 32-bit unsigned)
        index_count,  # the number of indices (0 if there is no index buffer) (int, 32-bit unsigned)
    )
    ```

=== "C"

    ``` c
    void dvz_visual_resize(
        DvzVisual* visual,  // the visual
        uint32_t item_count,  // the number of items
        uint32_t vertex_count,  // the number of vertices
        uint32_t index_count,  // the number of indices (0 if there is no index buffer)
    );
    ```

### `dvz_visual_shader()`

Set the shader SPIR-V name of a visual.

=== "Python"

    ``` python
    dvz.visual_shader(
        visual,  # the visual (LP_DvzVisual)
        name,  # the built-in resource name of the shader (_vert and _frag are appended) (CStringBuffer)
    )
    ```

=== "C"

    ``` c
    void dvz_visual_shader(
        DvzVisual* visual,  // the visual
        char* name,  // the built-in resource name of the shader (_vert and _frag are appended)
    );
    ```

### `dvz_visual_show()`

Set the visibility of a visual.

=== "Python"

    ``` python
    dvz.visual_show(
        visual,  # the visual (LP_DvzVisual)
        is_visible,  # the visual visibility (bool)
    )
    ```

=== "C"

    ``` c
    void dvz_visual_show(
        DvzVisual* visual,  // the visual
        bool is_visible,  // the visual visibility
    );
    ```

### `dvz_visual_slot()`

Declare a visual slot.

=== "Python"

    ``` python
    dvz.visual_slot(
        visual,  # the visual (LP_DvzVisual)
        slot_idx,  # the slot index (int, 32-bit unsigned)
        type,  # the slot type (DvzSlotType)
    )
    ```

=== "C"

    ``` c
    void dvz_visual_slot(
        DvzVisual* visual,  // the visual
        uint32_t slot_idx,  // the slot index
        DvzSlotType type,  // the slot type
    );
    ```

### `dvz_visual_specialization()`

Set a specialization constant of a visual.

=== "Python"

    ``` python
    dvz.visual_specialization(
        visual,  # the visual (LP_DvzVisual)
        shader,  # the shader type (DvzShaderType)
        idx,  # the specialization constant index (int, 32-bit unsigned)
        size,  # the size, in bytes, of the value passed to this function (int, 64-bit unsigned)
        value,  # a pointer to the value to use for that specialization constant (array)
    )
    ```

=== "C"

    ``` c
    void dvz_visual_specialization(
        DvzVisual* visual,  // the visual
        DvzShaderType shader,  // the shader type
        uint32_t idx,  // the specialization constant index
        DvzSize size,  // the size, in bytes, of the value passed to this function
        void* value,  // a pointer to the value to use for that specialization constant
    );
    ```

### `dvz_visual_spirv()`

Set the shader SPIR-V code of a visual.

=== "Python"

    ``` python
    dvz.visual_spirv(
        visual,  # the visual (LP_DvzVisual)
        type,  # the shader type (DvzShaderType)
        size,  # the size, in bytes, of the SPIR-V buffer (int, 64-bit unsigned)
        buffer,  # a pointer to the SPIR-V buffer (CStringBuffer)
    )
    ```

=== "C"

    ``` c
    void dvz_visual_spirv(
        DvzVisual* visual,  // the visual
        DvzShaderType type,  // the shader type
        DvzSize size,  // the size, in bytes, of the SPIR-V buffer
        char* buffer,  // a pointer to the SPIR-V buffer
    );
    ```

### `dvz_visual_stride()`

Declare a visual binding.

=== "Python"

    ``` python
    dvz.visual_stride(
        visual,  # the visual (LP_DvzVisual)
        binding_idx,  # the binding index (int, 32-bit unsigned)
        stride,  # the binding stride, in bytes (int, 64-bit unsigned)
    )
    ```

=== "C"

    ``` c
    void dvz_visual_stride(
        DvzVisual* visual,  // the visual
        uint32_t binding_idx,  // the binding index
        DvzSize stride,  // the binding stride, in bytes
    );
    ```

### `dvz_visual_tex()`

Bind a tex to a visual slot.

=== "Python"

    ``` python
    dvz.visual_tex(
        visual,  # the visual (LP_DvzVisual)
        slot_idx,  # the slot index (int, 32-bit unsigned)
        tex,  # the tex ID (int, 64-bit unsigned)
        sampler,  # the sampler ID (int, 64-bit unsigned)
        offset,  # the texture offset (uvec3)
    )
    ```

=== "C"

    ``` c
    void dvz_visual_tex(
        DvzVisual* visual,  // the visual
        uint32_t slot_idx,  // the slot index
        DvzId tex,  // the tex ID
        DvzId sampler,  // the sampler ID
        uvec3 offset,  // the texture offset
    );
    ```

### `dvz_visual_transform()`

Set a visual transform.

=== "Python"

    ``` python
    dvz.visual_transform(
        visual,  # the visual (LP_DvzVisual)
        tr,  # the transform (LP_DvzTransform)
        vertex_attr,  # the vertex attribute on which the transform applies to (int, 32-bit unsigned)
    )
    ```

=== "C"

    ``` c
    void dvz_visual_transform(
        DvzVisual* visual,  // the visual
        DvzTransform* tr,  // the transform
        uint32_t vertex_attr,  // the vertex attribute on which the transform applies to
    );
    ```

### `dvz_visual_update()`

Update a visual after its data has changed.

=== "Python"

    ``` python
    dvz.visual_update(
        visual,  # the visual (LP_DvzVisual)
    )
    ```

=== "C"

    ``` c
    void dvz_visual_update(
        DvzVisual* visual,  // the visual
    );
    ```

### `dvz_volume()`

Create a volume visual.

=== "Python"

    ``` python
    dvz.volume(  # returns: the visual (LP_DvzVisual)
        batch,  # the batch (LP_DvzBatch)
        flags,  # the visual creation flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    DvzVisual* dvz_volume(  // returns: the visual
        DvzBatch* batch,  // the batch
        int flags,  // the visual creation flags
    );
    ```

### `dvz_volume_bounds()`

Set the volume bounds.

=== "Python"

    ``` python
    dvz.volume_bounds(
        visual,  # the visual (LP_DvzVisual)
        xlim,  # xmin and xmax (vec2)
        ylim,  # ymin and ymax (vec2)
        zlim,  # zmin and zmax (vec2)
    )
    ```

=== "C"

    ``` c
    void dvz_volume_bounds(
        DvzVisual* visual,  // the visual
        vec2 xlim,  // xmin and xmax
        vec2 ylim,  // ymin and ymax
        vec2 zlim,  // zmin and zmax
    );
    ```

### `dvz_volume_permutation()`

Set the texture coordinates index permutation.

=== "Python"

    ``` python
    dvz.volume_permutation(
        visual,  # the visual (LP_DvzVisual)
        ijk,  # index permutation (ivec3)
    )
    ```

=== "C"

    ``` c
    void dvz_volume_permutation(
        DvzVisual* visual,  // the visual
        ivec3 ijk,  // index permutation
    );
    ```

### `dvz_volume_slice()`

Set the bounding box face index on which to slice (showing the texture itself).

=== "Python"

    ``` python
    dvz.volume_slice(
        visual,  # the visual (LP_DvzVisual)
        face_index,  # -1 to disable, or the face index between 0 and 5 included (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    void dvz_volume_slice(
        DvzVisual* visual,  // the visual
        int32_t face_index,  // -1 to disable, or the face index between 0 and 5 included
    );
    ```

### `dvz_volume_texcoords()`

Set the texture coordinates of two corner points.

=== "Python"

    ``` python
    dvz.volume_texcoords(
        visual,  # the visual (LP_DvzVisual)
        uvw0,  # coordinates of one of the corner points (vec3)
        uvw1,  # coordinates of one of the corner points (vec3)
    )
    ```

=== "C"

    ``` c
    void dvz_volume_texcoords(
        DvzVisual* visual,  // the visual
        vec3 uvw0,  // coordinates of one of the corner points
        vec3 uvw1,  // coordinates of one of the corner points
    );
    ```

### `dvz_volume_texture()`

Assign a 3D texture to a volume visual.

=== "Python"

    ``` python
    dvz.volume_texture(
        visual,  # the visual (LP_DvzVisual)
        tex,  # the texture ID (int, 64-bit unsigned)
        filter,  # the texture filtering mode (DvzFilter)
        address_mode,  # the texture address mode (DvzSamplerAddressMode)
    )
    ```

=== "C"

    ``` c
    void dvz_volume_texture(
        DvzVisual* visual,  // the visual
        DvzId tex,  // the texture ID
        DvzFilter filter,  // the texture filtering mode
        DvzSamplerAddressMode address_mode,  // the texture address mode
    );
    ```

### `dvz_volume_transfer()`

Set the volume size.

=== "Python"

    ``` python
    dvz.volume_transfer(
        visual,  # the visual (LP_DvzVisual)
        transfer,  # transfer function, for now `vec4(x, 0, 0, 0)` where x is a scaling factor (vec4)
    )
    ```

=== "C"

    ``` c
    void dvz_volume_transfer(
        DvzVisual* visual,  // the visual
        vec4 transfer,  // transfer function, for now `vec4(x, 0, 0, 0)` where x is a scaling factor
    );
    ```

### `dvz_app()`

Create an app.

=== "Python"

    ``` python
    dvz.app(  # returns: the app (LP_DvzApp)
        flags,  # the app creation flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    DvzApp* dvz_app(  // returns: the app
        int flags,  // the app creation flags
    );
    ```

### `dvz_app_batch()`

Return the app batch.

=== "Python"

    ``` python
    dvz.app_batch(  # returns: the batch (LP_DvzBatch)
        app,  # the app (LP_DvzApp)
    )
    ```

=== "C"

    ``` c
    DvzBatch* dvz_app_batch(  // returns: the batch
        DvzApp* app,  // the app
    );
    ```

### `dvz_app_destroy()`

Destroy the app.

=== "Python"

    ``` python
    dvz.app_destroy(
        app,  # the app (LP_DvzApp)
    )
    ```

=== "C"

    ``` c
    void dvz_app_destroy(
        DvzApp* app,  // the app
    );
    ```

### `dvz_app_frame()`

Run one frame.

=== "Python"

    ``` python
    dvz.app_frame(
        app,  # the app (LP_DvzApp)
    )
    ```

=== "C"

    ``` c
    void dvz_app_frame(
        DvzApp* app,  // the app
    );
    ```

### `dvz_app_gui()`

Register a GUI callback.

=== "Python"

    ``` python
    dvz.app_gui(
        app,  # the app (LP_DvzApp)
        canvas_id,  # the canvas ID (int, 64-bit unsigned)
        callback,  # the GUI callback (CFunctionType)
        user_data,  # the user data (array)
    )
    ```

=== "C"

    ``` c
    void dvz_app_gui(
        DvzApp* app,  // the app
        DvzId canvas_id,  // the canvas ID
        DvzAppGuiCallback callback,  // the GUI callback
        void* user_data,  // the user data
    );
    ```

### `dvz_app_keyboard()`

Return the last keyboard key pressed.

=== "Python"

    ``` python
    dvz.app_keyboard(
        app,  # the app (LP_DvzApp)
        canvas_id,  # the canvas id (int, 64-bit unsigned)
    )
    ```

=== "C"

    ``` c
    void dvz_app_keyboard(
        DvzApp* app,  // the app
        DvzId canvas_id,  // the canvas id
    );
    ```

### `dvz_app_mouse()`

Return the last mouse position and pressed button.

=== "Python"

    ``` python
    dvz.app_mouse(
        app,  # the app (LP_DvzApp)
        canvas_id,  # the canvas id (int, 64-bit unsigned)
    )
    ```

=== "C"

    ``` c
    void dvz_app_mouse(
        DvzApp* app,  // the app
        DvzId canvas_id,  // the canvas id
    );
    ```

### `dvz_app_onframe()`

Register a frame callback.

=== "Python"

    ``` python
    dvz.app_onframe(
        app,  # the app (LP_DvzApp)
        callback,  # the callback (CFunctionType)
        user_data,  # the user data (array)
    )
    ```

=== "C"

    ``` c
    void dvz_app_onframe(
        DvzApp* app,  // the app
        DvzAppFrameCallback callback,  // the callback
        void* user_data,  // the user data
    );
    ```

### `dvz_app_onkeyboard()`

Register a keyboard callback.

=== "Python"

    ``` python
    dvz.app_onkeyboard(
        app,  # the app (LP_DvzApp)
        callback,  # the callback (CFunctionType)
        user_data,  # the user data (array)
    )
    ```

=== "C"

    ``` c
    void dvz_app_onkeyboard(
        DvzApp* app,  // the app
        DvzAppKeyboardCallback callback,  // the callback
        void* user_data,  // the user data
    );
    ```

### `dvz_app_onmouse()`

Register a mouse callback.

=== "Python"

    ``` python
    dvz.app_onmouse(
        app,  # the app (LP_DvzApp)
        callback,  # the callback (CFunctionType)
        user_data,  # the user data (array)
    )
    ```

=== "C"

    ``` c
    void dvz_app_onmouse(
        DvzApp* app,  // the app
        DvzAppMouseCallback callback,  // the callback
        void* user_data,  // the user data
    );
    ```

### `dvz_app_onresize()`

Register a resize callback.

=== "Python"

    ``` python
    dvz.app_onresize(
        app,  # the app (LP_DvzApp)
        callback,  # the callback (CFunctionType)
        user_data,  # the user data (array)
    )
    ```

=== "C"

    ``` c
    void dvz_app_onresize(
        DvzApp* app,  // the app
        DvzAppResizeCallback callback,  // the callback
        void* user_data,  // the user data
    );
    ```

### `dvz_app_ontimer()`

Register a timer callback.

=== "Python"

    ``` python
    dvz.app_ontimer(
        app,  # the app (LP_DvzApp)
        callback,  # the timer callback (CFunctionType)
        user_data,  # the user data (array)
    )
    ```

=== "C"

    ``` c
    void dvz_app_ontimer(
        DvzApp* app,  // the app
        DvzAppTimerCallback callback,  // the timer callback
        void* user_data,  // the user data
    );
    ```

### `dvz_app_run()`

Start the application event loop.

=== "Python"

    ``` python
    dvz.app_run(
        app,  # the app (LP_DvzApp)
        n_frames,  # the maximum number of frames, 0 for infinite loop (int, 64-bit unsigned)
    )
    ```

=== "C"

    ``` c
    void dvz_app_run(
        DvzApp* app,  // the app
        uint64_t n_frames,  // the maximum number of frames, 0 for infinite loop
    );
    ```

### `dvz_app_screenshot()`

Make a screenshot of a canvas.

=== "Python"

    ``` python
    dvz.app_screenshot(
        app,  # the app (LP_DvzApp)
        canvas_id,  # the ID of the canvas (int, 64-bit unsigned)
        filename,  # the path to the PNG file with the screenshot (CStringBuffer)
    )
    ```

=== "C"

    ``` c
    void dvz_app_screenshot(
        DvzApp* app,  // the app
        DvzId canvas_id,  // the ID of the canvas
        char* filename,  // the path to the PNG file with the screenshot
    );
    ```

### `dvz_app_submit()`

Submit the current batch to the application.

=== "Python"

    ``` python
    dvz.app_submit(
        app,  # the app (LP_DvzApp)
    )
    ```

=== "C"

    ``` c
    void dvz_app_submit(
        DvzApp* app,  // the app
    );
    ```

### `dvz_app_timer()`

Create a timer.

=== "Python"

    ``` python
    dvz.app_timer(  # returns: the timer (LP_DvzTimerItem)
        app,  # the app (LP_DvzApp)
        delay,  # the delay, in seconds, until the first event (float, 32-bit)
        period,  # the period, in seconds, between two events (float, 32-bit)
        max_count,  # the maximum number of events (int, 64-bit unsigned)
    )
    ```

=== "C"

    ``` c
    DvzTimerItem* dvz_app_timer(  // returns: the timer
        DvzApp* app,  // the app
        double delay,  // the delay, in seconds, until the first event
        double period,  // the period, in seconds, between two events
        uint64_t max_count,  // the maximum number of events
    );
    ```

### `dvz_app_timestamps()`

Return the precise display timestamps of the last `count` frames.

=== "Python"

    ``` python
    dvz.app_timestamps(
        app,  # the app (LP_DvzApp)
        canvas_id,  # the ID of the canvas (int, 64-bit unsigned)
        count,  # number of frames (int, 32-bit unsigned)
    )
    ```

=== "C"

    ``` c
    void dvz_app_timestamps(
        DvzApp* app,  // the app
        DvzId canvas_id,  // the ID of the canvas
        uint32_t count,  // number of frames
    );
    ```

### `dvz_app_wait()`

Wait until the GPU has finished processing.

=== "Python"

    ``` python
    dvz.app_wait(
        app,  # the app (LP_DvzApp)
    )
    ```

=== "C"

    ``` c
    void dvz_app_wait(
        DvzApp* app,  // the app
    );
    ```

### `dvz_free()`

Free a pointer.

=== "Python"

    ``` python
    dvz.free(
        pointer,  # a pointer (array)
    )
    ```

=== "C"

    ``` c
    void dvz_free(
        void* pointer,  // a pointer
    );
    ```

### `dvz_time()`

Get the current time.

=== "Python"

    ``` python
    dvz.time()
    ```

=== "C"

    ``` c
    void dvz_time();
    ```

### `dvz_time_print()`

Display a time.

=== "Python"

    ``` python
    dvz.time_print(
        time,  # a time structure (LP_DvzTime)
    )
    ```

=== "C"

    ``` c
    void dvz_time_print(
        DvzTime* time,  // a time structure
    );
    ```

### `dvz_external_dat()`

Get an external memory handle of a dat.

=== "Python"

    ``` python
    dvz.external_dat(  # returns: the external memory handle of that buffer (c_int)
        rd,  # the renderer (LP_DvzRenderer)
        visual,  # the visual (LP_DvzVisual)
        slot_idx,  # the slot index of the dat (int, 32-bit unsigned)
    )
    ```

=== "C"

    ``` c
    int dvz_external_dat(  // returns: the external memory handle of that buffer
        DvzRenderer* rd,  // the renderer
        DvzVisual* visual,  // the visual
        uint32_t slot_idx,  // the slot index of the dat
    );
    ```

### `dvz_external_index()`

Get an external memory handle of an index dat.

=== "Python"

    ``` python
    dvz.external_index(  # returns: the external memory handle of that buffer (c_int)
        rd,  # the renderer (LP_DvzRenderer)
        visual,  # the visual (LP_DvzVisual)
    )
    ```

=== "C"

    ``` c
    int dvz_external_index(  // returns: the external memory handle of that buffer
        DvzRenderer* rd,  // the renderer
        DvzVisual* visual,  // the visual
    );
    ```

### `dvz_external_tex()`

Get an external memory handle of a tex's staging buffer.

=== "Python"

    ``` python
    dvz.external_tex(  # returns: the external memory handle of that buffer (c_int)
        rd,  # the renderer (LP_DvzRenderer)
        visual,  # the visual (LP_DvzVisual)
        slot_idx,  # the slot index of the tex (int, 32-bit unsigned)
    )
    ```

=== "C"

    ``` c
    int dvz_external_tex(  // returns: the external memory handle of that buffer
        DvzRenderer* rd,  // the renderer
        DvzVisual* visual,  // the visual
        uint32_t slot_idx,  // the slot index of the tex
    );
    ```

### `dvz_external_vertex()`

Get an external memory handle of a vertex dat.

=== "Python"

    ``` python
    dvz.external_vertex(  # returns: the external memory handle of that buffer (c_int)
        rd,  # the renderer (LP_DvzRenderer)
        visual,  # the visual (LP_DvzVisual)
        binding_idx,  # the binding index of the dat that is being used as vertex buffer (int, 32-bit unsigned)
    )
    ```

=== "C"

    ``` c
    int dvz_external_vertex(  // returns: the external memory handle of that buffer
        DvzRenderer* rd,  // the renderer
        DvzVisual* visual,  // the visual
        uint32_t binding_idx,  // the binding index of the dat that is being used as vertex buffer
    );
    ```

### `dvz_earcut()`

Compute a polygon triangulation with only indexing on the polygon contour vertices.

=== "Python"

    ``` python
    dvz.earcut(  # returns: the computed indices (must be FREED by the caller) (ndpointer_<u4_C_CONTIGUOUS)
        point_count,  # the number of points (int, 32-bit unsigned)
        polygon,  # the polygon 2D positions (ndpointer_<f8_C_CONTIGUOUS)
    )
    ```

=== "C"

    ``` c
    DvzIndex* dvz_earcut(  // returns: the computed indices (must be FREED by the caller)
        uint32_t point_count,  // the number of points
        dvec2* polygon,  // the polygon 2D positions
    );
    ```

### `dvz_mean()`

Compute the mean of an array of double values.

=== "Python"

    ``` python
    dvz.mean(  # returns: the mean (c_double)
        n,  # the number of values (int, 32-bit unsigned)
        values,  # an array of double numbers (ndpointer_<f8_C_CONTIGUOUS)
    )
    ```

=== "C"

    ``` c
    double dvz_mean(  // returns: the mean
        uint32_t n,  // the number of values
        double* values,  // an array of double numbers
    );
    ```

### `dvz_min_max()`

Compute the min and max of an array of float values.

=== "Python"

    ``` python
    dvz.min_max(  # returns: the mean (c_int)
        n,  # the number of values (int, 32-bit unsigned)
        values,  # an array of float numbers (ndpointer_<f4_C_CONTIGUOUS)
        out_min_max,  # the min and max (vec2)
    )
    ```

=== "C"

    ``` c
    void dvz_min_max(  // returns: the mean
        uint32_t n,  // the number of values
        float* values,  // an array of float numbers
        vec2 out_min_max,  // the min and max
    );
    ```

### `dvz_mock_band()`

Generate points on a band.

=== "Python"

    ``` python
    dvz.mock_band(  # returns: the positions (ndpointer_<f4_C_CONTIGUOUS)
        count,  # the number of positions to generate (int, 32-bit unsigned)
        size,  # the size of the band (vec2)
    )
    ```

=== "C"

    ``` c
    vec3* dvz_mock_band(  // returns: the positions
        uint32_t count,  // the number of positions to generate
        vec2 size,  // the size of the band
    );
    ```

### `dvz_mock_circle()`

Generate points on a circle.

=== "Python"

    ``` python
    dvz.mock_circle(  # returns: the positions (ndpointer_<f4_C_CONTIGUOUS)
        count,  # the number of positions to generate (int, 32-bit unsigned)
        radius,  # the radius of the circle (float, 64-bit)
    )
    ```

=== "C"

    ``` c
    vec3* dvz_mock_circle(  // returns: the positions
        uint32_t count,  // the number of positions to generate
        float radius,  // the radius of the circle
    );
    ```

### `dvz_mock_cmap()`

Generate a set of HSV colors.

=== "Python"

    ``` python
    dvz.mock_cmap(  # returns: colors (ndpointer_|u1_C_CONTIGUOUS)
        count,  # the number of colors to generate (int, 32-bit unsigned)
        alpha,  # the alpha value (DvzColormap)
    )
    ```

=== "C"

    ``` c
    DvzColor* dvz_mock_cmap(  // returns: colors
        uint32_t count,  // the number of colors to generate
        DvzAlpha alpha,  // the alpha value
    );
    ```

### `dvz_mock_color()`

Generate a set of random colors.

=== "Python"

    ``` python
    dvz.mock_color(  # returns: random colors (ndpointer_|u1_C_CONTIGUOUS)
        count,  # the number of colors to generate (int, 32-bit unsigned)
        alpha,  # the alpha value (int, 8-bit unsigned)
    )
    ```

=== "C"

    ``` c
    DvzColor* dvz_mock_color(  // returns: random colors
        uint32_t count,  // the number of colors to generate
        DvzAlpha alpha,  // the alpha value
    );
    ```

### `dvz_mock_fixed()`

Generate identical 3D positions.

=== "Python"

    ``` python
    dvz.mock_fixed(  # returns: the repeated positions (ndpointer_<f4_C_CONTIGUOUS)
        count,  # the number of positions to generate (int, 32-bit unsigned)
        fixed,  # the position (vec3)
    )
    ```

=== "C"

    ``` c
    vec3* dvz_mock_fixed(  // returns: the repeated positions
        uint32_t count,  // the number of positions to generate
        vec3 fixed,  // the position
    );
    ```

### `dvz_mock_full()`

Generate an array with the same value.

=== "Python"

    ``` python
    dvz.mock_full(  # returns: the values (ndpointer_<f4_C_CONTIGUOUS)
        count,  # the number of scalars to generate (int, 32-bit unsigned)
        value,  # the value (float, 64-bit)
    )
    ```

=== "C"

    ``` c
    float* dvz_mock_full(  // returns: the values
        uint32_t count,  // the number of scalars to generate
        float value,  // the value
    );
    ```

### `dvz_mock_line()`

Generate 3D positions on a line.

=== "Python"

    ``` python
    dvz.mock_line(  # returns: the positions (ndpointer_<f4_C_CONTIGUOUS)
        count,  # the number of positions to generate (int, 32-bit unsigned)
        p0,  # initial position (vec3)
        p1,  # terminal position (vec3)
    )
    ```

=== "C"

    ``` c
    vec3* dvz_mock_line(  // returns: the positions
        uint32_t count,  // the number of positions to generate
        vec3 p0,  // initial position
        vec3 p1,  // terminal position
    );
    ```

### `dvz_mock_linspace()`

Generate an array ranging from an initial value to a final value.

=== "Python"

    ``` python
    dvz.mock_linspace(  # returns: the values (ndpointer_<f4_C_CONTIGUOUS)
        count,  # the number of scalars to generate (int, 32-bit unsigned)
        initial,  # the initial value (float, 64-bit)
        final,  # the final value (float, 64-bit)
    )
    ```

=== "C"

    ``` c
    float* dvz_mock_linspace(  // returns: the values
        uint32_t count,  // the number of scalars to generate
        float initial,  // the initial value
        float final,  // the final value
    );
    ```

### `dvz_mock_monochrome()`

Repeat a color in an array.

=== "Python"

    ``` python
    dvz.mock_monochrome(  # returns: colors (ndpointer_|u1_C_CONTIGUOUS)
        count,  # the number of colors to generate (int, 32-bit unsigned)
        mono,  # the color to repeat (cvec4)
    )
    ```

=== "C"

    ``` c
    DvzColor* dvz_mock_monochrome(  // returns: colors
        uint32_t count,  // the number of colors to generate
        DvzColor mono,  // the color to repeat
    );
    ```

### `dvz_mock_pos_2D()`

Generate a set of random 2D positions.

=== "Python"

    ``` python
    dvz.mock_pos_2D(  # returns: the positions (ndpointer_<f4_C_CONTIGUOUS)
        count,  # the number of positions to generate (int, 32-bit unsigned)
        std,  # the standard deviation (float, 64-bit)
    )
    ```

=== "C"

    ``` c
    vec3* dvz_mock_pos_2D(  // returns: the positions
        uint32_t count,  // the number of positions to generate
        float std,  // the standard deviation
    );
    ```

### `dvz_mock_pos_3D()`

Generate a set of random 3D positions.

=== "Python"

    ``` python
    dvz.mock_pos_3D(  # returns: the positions (ndpointer_<f4_C_CONTIGUOUS)
        count,  # the number of positions to generate (int, 32-bit unsigned)
        std,  # the standard deviation (float, 64-bit)
    )
    ```

=== "C"

    ``` c
    vec3* dvz_mock_pos_3D(  // returns: the positions
        uint32_t count,  // the number of positions to generate
        float std,  // the standard deviation
    );
    ```

### `dvz_mock_range()`

Generate an array of consecutive positive numbers.

=== "Python"

    ``` python
    dvz.mock_range(  # returns: the values (ndpointer_<u4_C_CONTIGUOUS)
        count,  # the number of consecutive integers to generate (int, 32-bit unsigned)
        initial,  # the initial value (int, 32-bit unsigned)
    )
    ```

=== "C"

    ``` c
    uint32_t* dvz_mock_range(  // returns: the values
        uint32_t count,  // the number of consecutive integers to generate
        uint32_t initial,  // the initial value
    );
    ```

### `dvz_mock_uniform()`

Generate a set of uniformly random scalar values.

=== "Python"

    ``` python
    dvz.mock_uniform(  # returns: the values (ndpointer_<f4_C_CONTIGUOUS)
        count,  # the number of values to generate (int, 32-bit unsigned)
        vmin,  # the minimum value of the interval (float, 64-bit)
        vmax,  # the maximum value of the interval (float, 64-bit)
    )
    ```

=== "C"

    ``` c
    float* dvz_mock_uniform(  // returns: the values
        uint32_t count,  // the number of values to generate
        float vmin,  // the minimum value of the interval
        float vmax,  // the maximum value of the interval
    );
    ```

### `dvz_next_pow2()`

Return the smallest power of 2 larger or equal than a positive integer.

=== "Python"

    ``` python
    dvz.next_pow2(  # returns: the power of 2 (c_ulong)
        x,  # the value (int, 64-bit unsigned)
    )
    ```

=== "C"

    ``` c
    uint64_t dvz_next_pow2(  // returns: the power of 2
        uint64_t x,  // the value
    );
    ```

### `dvz_normalize_bytes()`

Normalize the array.

=== "Python"

    ``` python
    dvz.normalize_bytes(  # returns: the normalized array (ndpointer_|u1_C_CONTIGUOUS)
        count,  # the number of values (int, 32-bit unsigned)
        values,  # an array of float numbers (ndpointer_<f4_C_CONTIGUOUS)
    )
    ```

=== "C"

    ``` c
    uint8_t* dvz_normalize_bytes(  // returns: the normalized array
        uint32_t count,  // the number of values
        float* values,  // an array of float numbers
    );
    ```

### `dvz_num_procs()`

Return the number of processors on the current system.

=== "Python"

    ``` python
    dvz.num_procs()  # returns: the number of processors (c_int)
    ```

=== "C"

    ``` c
    int dvz_num_procs();  // returns: the number of processors
    ```

### `dvz_rand_byte()`

Return a random integer number between 0 and 255.

=== "Python"

    ``` python
    dvz.rand_byte()  # returns: random number (c_ubyte)
    ```

=== "C"

    ``` c
    uint8_t dvz_rand_byte();  // returns: random number
    ```

### `dvz_rand_double()`

Return a random floating-point number between 0 and 1.

=== "Python"

    ``` python
    dvz.rand_double()  # returns: random number (c_double)
    ```

=== "C"

    ``` c
    double dvz_rand_double();  // returns: random number
    ```

### `dvz_rand_float()`

Return a random floating-point number between 0 and 1.

=== "Python"

    ``` python
    dvz.rand_float()  # returns: random number (c_float)
    ```

=== "C"

    ``` c
    float dvz_rand_float();  // returns: random number
    ```

### `dvz_rand_int()`

Return a random integer number.

=== "Python"

    ``` python
    dvz.rand_int()  # returns: random number (c_int)
    ```

=== "C"

    ``` c
    int dvz_rand_int();  // returns: random number
    ```

### `dvz_rand_normal()`

Return a random normal floating-point number.

=== "Python"

    ``` python
    dvz.rand_normal()  # returns: random number (c_double)
    ```

=== "C"

    ``` c
    double dvz_rand_normal();  // returns: random number
    ```

### `dvz_range()`

Compute the range of an array of double values.

=== "Python"

    ``` python
    dvz.range(
        n,  # the number of values (int, 32-bit unsigned)
        values,  # an array of double numbers (ndpointer_<f8_C_CONTIGUOUS)
    )
    ```

=== "C"

    ``` c
    void dvz_range(
        uint32_t n,  // the number of values
        double* values,  // an array of double numbers
    );
    ```

### `dvz_threads_default()`

Set the number of threads to use in OpenMP-aware functions based on DVZ_NUM_THREADS, or take

=== "Python"

    ``` python
    dvz.threads_default()
    ```

=== "C"

    ``` c
    void dvz_threads_default();
    ```

### `dvz_threads_get()`

Get the number of threads to use in OpenMP-aware functions.

=== "Python"

    ``` python
    dvz.threads_get()  # returns: the current number of threads specified to OpenMP (c_int)
    ```

=== "C"

    ``` c
    int dvz_threads_get();  // returns: the current number of threads specified to OpenMP
    ```

### `dvz_threads_set()`

Set the number of threads to use in OpenMP-aware functions.

=== "Python"

    ``` python
    dvz.threads_set(
        num_threads,  # the requested number of threads (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    void dvz_threads_set(
        int num_threads,  // the requested number of threads
    );
    ```

## Datoviz Rendering Protocol functions

### `dvz_batch()`

Create a batch holding a number of requests.

=== "Python"

    ``` python
    dvz.batch()
    ```

=== "C"

    ``` c
    DvzBatch* dvz_batch();
    ```

### `dvz_batch_add()`

Add a request to a batch.

=== "Python"

    ``` python
    dvz.batch_add(
        batch,  # the batch (LP_DvzBatch)
        req,  # the request (DvzRequest)
    )
    ```

=== "C"

    ``` c
    void dvz_batch_add(
        DvzBatch* batch,  // the batch
        DvzRequest req,  // the request
    );
    ```

### `dvz_batch_clear()`

Remove all requests in a batch.

=== "Python"

    ``` python
    dvz.batch_clear(
        batch,  # the batch (LP_DvzBatch)
    )
    ```

=== "C"

    ``` c
    void dvz_batch_clear(
        DvzBatch* batch,  // the batch
    );
    ```

### `dvz_batch_copy()`

Create a copy of a batch.

=== "Python"

    ``` python
    dvz.batch_copy(
        batch,  # the batch (LP_DvzBatch)
    )
    ```

=== "C"

    ``` c
    DvzBatch* dvz_batch_copy(
        DvzBatch* batch,  // the batch
    );
    ```

### `dvz_batch_desc()`

Set the description of the last added request.

=== "Python"

    ``` python
    dvz.batch_desc(
        batch,  # the batch (LP_DvzBatch)
        desc,  # the description (CStringBuffer)
    )
    ```

=== "C"

    ``` c
    void dvz_batch_desc(
        DvzBatch* batch,  // the batch
        char* desc,  // the description
    );
    ```

### `dvz_batch_destroy()`

Destroy a batch.

=== "Python"

    ``` python
    dvz.batch_destroy(
        batch,  # the batch (LP_DvzBatch)
    )
    ```

=== "C"

    ``` c
    void dvz_batch_destroy(
        DvzBatch* batch,  // the batch
    );
    ```

### `dvz_batch_dump()`

Dump all batch requests in raw binary file.

=== "Python"

    ``` python
    dvz.batch_dump(
        batch,  # the batch (LP_DvzBatch)
        filename,  # the dump filename (CStringBuffer)
    )
    ```

=== "C"

    ``` c
    int dvz_batch_dump(
        DvzBatch* batch,  // the batch
        char* filename,  // the dump filename
    );
    ```

### `dvz_batch_load()`

Load a dump of batch requests into an existing batch object.

=== "Python"

    ``` python
    dvz.batch_load(
        batch,  # the batch (LP_DvzBatch)
        filename,  # the dump filename (CStringBuffer)
    )
    ```

=== "C"

    ``` c
    void dvz_batch_load(
        DvzBatch* batch,  // the batch
        char* filename,  // the dump filename
    );
    ```

### `dvz_batch_print()`

Display information about all requests in the batch.

=== "Python"

    ``` python
    dvz.batch_print(
        batch,  # the batch (LP_DvzBatch)
        flags,  # the flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    void dvz_batch_print(
        DvzBatch* batch,  // the batch
        int flags,  // the flags
    );
    ```

### `dvz_batch_requests()`

Return a pointer to the array of all requests in the batch.

=== "Python"

    ``` python
    dvz.batch_requests(
        batch,  # the batch (LP_DvzBatch)
    )
    ```

=== "C"

    ``` c
    DvzRequest* dvz_batch_requests(
        DvzBatch* batch,  // the batch
    );
    ```

### `dvz_batch_size()`

Return the number of requests in the batch.

=== "Python"

    ``` python
    dvz.batch_size(
        batch,  # the batch (LP_DvzBatch)
    )
    ```

=== "C"

    ``` c
    uint32_t dvz_batch_size(
        DvzBatch* batch,  // the batch
    );
    ```

### `dvz_batch_yaml()`

Export requests in a YAML file.

=== "Python"

    ``` python
    dvz.batch_yaml(
        batch,  # the batch (LP_DvzBatch)
        filename,  # the YAML filename (CStringBuffer)
    )
    ```

=== "C"

    ``` c
    void dvz_batch_yaml(
        DvzBatch* batch,  // the batch
        char* filename,  // the YAML filename
    );
    ```

### `dvz_bind_dat()`

Create a request for associating a dat to a pipe's slot.

=== "Python"

    ``` python
    dvz.bind_dat(  # returns: the request (DvzRequest)
        batch,  # the batch (LP_DvzBatch)
        pipe,  # the id of the pipe (int, 64-bit unsigned)
        slot_idx,  # the index of the descriptor slot (int, 32-bit unsigned)
        dat,  # the id of the dat to bind to the pipe (int, 64-bit unsigned)
        offset,  # the offset (int, 64-bit unsigned)
    )
    ```

=== "C"

    ``` c
    DvzRequest dvz_bind_dat(  // returns: the request
        DvzBatch* batch,  // the batch
        DvzId pipe,  // the id of the pipe
        uint32_t slot_idx,  // the index of the descriptor slot
        DvzId dat,  // the id of the dat to bind to the pipe
        DvzSize offset,  // the offset
    );
    ```

### `dvz_bind_index()`

Create a request for associating an index dat to a graphics pipe.

=== "Python"

    ``` python
    dvz.bind_index(  # returns: the request (DvzRequest)
        batch,  # the batch (LP_DvzBatch)
        graphics,  # the id of the graphics pipe (int, 64-bit unsigned)
        dat,  # the id of the dat with the index data (int, 64-bit unsigned)
        offset,  # the offset within the dat (int, 64-bit unsigned)
    )
    ```

=== "C"

    ``` c
    DvzRequest dvz_bind_index(  // returns: the request
        DvzBatch* batch,  // the batch
         graphics,  // the id of the graphics pipe
        DvzId dat,  // the id of the dat with the index data
        DvzSize offset,  // the offset within the dat
    );
    ```

### `dvz_bind_tex()`

Create a request for associating a tex to a pipe's slot.

=== "Python"

    ``` python
    dvz.bind_tex(  # returns: the request (DvzRequest)
        batch,  # the batch (LP_DvzBatch)
        pipe,  # the id of the pipe (int, 64-bit unsigned)
        slot_idx,  # the index of the descriptor slot (int, 32-bit unsigned)
        tex,  # the id of the tex to bind to the pipe (int, 64-bit unsigned)
        tex,  # the id of the sampler (int, 64-bit unsigned)
        offset,  # the offset (uvec3)
    )
    ```

=== "C"

    ``` c
    DvzRequest dvz_bind_tex(  // returns: the request
        DvzBatch* batch,  // the batch
        DvzId pipe,  // the id of the pipe
        uint32_t slot_idx,  // the index of the descriptor slot
        DvzId tex,  // the id of the tex to bind to the pipe
        DvzId tex,  // the id of the sampler
        uvec3 offset,  // the offset
    );
    ```

### `dvz_bind_vertex()`

Create a request for associating a vertex dat to a graphics pipe.

=== "Python"

    ``` python
    dvz.bind_vertex(  # returns: the request (DvzRequest)
        batch,  # the batch (LP_DvzBatch)
        graphics,  # the id of the graphics pipe (int, 64-bit unsigned)
        dat,  # the id of the dat with the vertex data (int, 32-bit unsigned)
        offset,  # the offset within the dat (int, 64-bit unsigned)
    )
    ```

=== "C"

    ``` c
    DvzRequest dvz_bind_vertex(  // returns: the request
        DvzBatch* batch,  // the batch
         graphics,  // the id of the graphics pipe
        DvzId dat,  // the id of the dat with the vertex data
        DvzSize offset,  // the offset within the dat
    );
    ```

### `dvz_create_canvas()`

Create a request for canvas creation.

=== "Python"

    ``` python
    dvz.create_canvas(  # returns: the request, containing a newly-generated id for the canvas to be created (DvzRequest)
        batch,  # the batch (LP_DvzBatch)
        width,  # the canvas width (in screen pixels) (int, 32-bit unsigned)
        height,  # the canvas height (in screen pixels) (int, 32-bit unsigned)
        background,  # the background color (cvec4)
        flags,  # the canvas creation flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    DvzRequest dvz_create_canvas(  // returns: the request, containing a newly-generated id for the canvas to be created
        DvzBatch* batch,  // the batch
        uint32_t width,  // the canvas width (in screen pixels)
        uint32_t height,  // the canvas height (in screen pixels)
        cvec4 background,  // the background color
        int flags,  // the canvas creation flags
    );
    ```

### `dvz_create_dat()`

Create a request for a dat creation.

=== "Python"

    ``` python
    dvz.create_dat(  # returns: the request, containing a newly-generated id for the dat to be created (DvzRequest)
        batch,  # the batch (LP_DvzBatch)
        type,  # the buffer type (DvzBufferType)
        size,  # the dat size, in bytes (int, 64-bit unsigned)
        flags,  # the dat creation flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    DvzRequest dvz_create_dat(  // returns: the request, containing a newly-generated id for the dat to be created
        DvzBatch* batch,  // the batch
        DvzBufferType type,  // the buffer type
        DvzSize size,  // the dat size, in bytes
        int flags,  // the dat creation flags
    );
    ```

### `dvz_create_glsl()`

Create a request for GLSL shader creation.

=== "Python"

    ``` python
    dvz.create_glsl(  # returns: the request (DvzRequest)
        batch,  # the batch (LP_DvzBatch)
        shader_type,  # the shader type (DvzShaderType)
        code,  # an ASCII string with the GLSL code (CStringBuffer)
    )
    ```

=== "C"

    ``` c
    DvzRequest dvz_create_glsl(  // returns: the request
        DvzBatch* batch,  // the batch
        DvzShaderType shader_type,  // the shader type
        char* code,  // an ASCII string with the GLSL code
    );
    ```

### `dvz_create_graphics()`

Create a request for a builtin graphics pipe creation.

=== "Python"

    ``` python
    dvz.create_graphics(  # returns: the request, containing a newly-generated id for the graphics pipe to be created (DvzRequest)
        batch,  # the batch (LP_DvzBatch)
        parent,  # the parent canvas id (DvzGraphicsType)
        type,  # the graphics type (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    DvzRequest dvz_create_graphics(  // returns: the request, containing a newly-generated id for the graphics pipe to be created
        DvzBatch* batch,  // the batch
         parent,  // the parent canvas id
        DvzGraphicsType type,  // the graphics type
        int flags,  // the graphics creation flags
    );
    ```

### `dvz_create_sampler()`

Create a request for a sampler creation.

=== "Python"

    ``` python
    dvz.create_sampler(  # returns: the request, containing a newly-generated id for the sampler to be created (DvzRequest)
        batch,  # the batch (LP_DvzBatch)
        filter,  # the sampler filter (DvzFilter)
        mode,  # the sampler address mode (DvzSamplerAddressMode)
    )
    ```

=== "C"

    ``` c
    DvzRequest dvz_create_sampler(  // returns: the request, containing a newly-generated id for the sampler to be created
        DvzBatch* batch,  // the batch
        DvzFilter filter,  // the sampler filter
        DvzSamplerAddressMode mode,  // the sampler address mode
    );
    ```

### `dvz_create_spirv()`

Create a request for SPIR-V shader creation.

=== "Python"

    ``` python
    dvz.create_spirv(  # returns: the request (DvzRequest)
        batch,  # the batch (LP_DvzBatch)
        shader_type,  # the shader type (DvzShaderType)
        size,  # the size in bytes of the SPIR-V buffer (int, 64-bit unsigned)
        buffer,  # pointer to a buffer with the SPIR-V bytecode (CStringBuffer)
    )
    ```

=== "C"

    ``` c
    DvzRequest dvz_create_spirv(  // returns: the request
        DvzBatch* batch,  // the batch
        DvzShaderType shader_type,  // the shader type
        DvzSize size,  // the size in bytes of the SPIR-V buffer
        char* buffer,  // pointer to a buffer with the SPIR-V bytecode
    );
    ```

### `dvz_create_tex()`

Create a request for a tex creation.

=== "Python"

    ``` python
    dvz.create_tex(  # returns: the request, containing a newly-generated id for the tex to be created (DvzRequest)
        batch,  # the batch (LP_DvzBatch)
        dims,  # the number of dimensions, 1, 2, or 3 (DvzTexDims)
        format,  # the image format (DvzFormat)
        shape,  # the texture shape (uvec3)
        flags,  # the dat creation flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    DvzRequest dvz_create_tex(  // returns: the request, containing a newly-generated id for the tex to be created
        DvzBatch* batch,  // the batch
        DvzTexDims dims,  // the number of dimensions, 1, 2, or 3
        DvzFormat format,  // the image format
        uvec3 shape,  // the texture shape
        int flags,  // the dat creation flags
    );
    ```

### `dvz_delete_canvas()`

Create a request for a canvas deletion.

=== "Python"

    ``` python
    dvz.delete_canvas(  # returns: the request (DvzRequest)
        batch,  # the batch (LP_DvzBatch)
        id,  # the canvas id (int, 64-bit unsigned)
    )
    ```

=== "C"

    ``` c
    DvzRequest dvz_delete_canvas(  // returns: the request
        DvzBatch* batch,  // the batch
        DvzId id,  // the canvas id
    );
    ```

### `dvz_delete_dat()`

Create a request for dat deletion.

=== "Python"

    ``` python
    dvz.delete_dat(  # returns: the request (DvzRequest)
        batch,  # the batch (LP_DvzBatch)
        id,  # the dat id (int, 64-bit unsigned)
    )
    ```

=== "C"

    ``` c
    DvzRequest dvz_delete_dat(  // returns: the request
        DvzBatch* batch,  // the batch
        DvzId id,  // the dat id
    );
    ```

### `dvz_delete_graphics()`

Create a request for graphics deletion.

=== "Python"

    ``` python
    dvz.delete_graphics(  # returns: the request (DvzRequest)
        batch,  # the batch (LP_DvzBatch)
        id,  # the graphics id (int, 64-bit unsigned)
    )
    ```

=== "C"

    ``` c
    DvzRequest dvz_delete_graphics(  // returns: the request
        DvzBatch* batch,  // the batch
        DvzId id,  // the graphics id
    );
    ```

### `dvz_delete_sampler()`

Create a request for sampler deletion.

=== "Python"

    ``` python
    dvz.delete_sampler(  # returns: the request (DvzRequest)
        batch,  # the batch (LP_DvzBatch)
        id,  # the sampler id (int, 64-bit unsigned)
    )
    ```

=== "C"

    ``` c
    DvzRequest dvz_delete_sampler(  // returns: the request
        DvzBatch* batch,  // the batch
        DvzId id,  // the sampler id
    );
    ```

### `dvz_delete_tex()`

Create a request for tex deletion.

=== "Python"

    ``` python
    dvz.delete_tex(  # returns: the request (DvzRequest)
        batch,  # the batch (LP_DvzBatch)
        id,  # the tex id (int, 64-bit unsigned)
    )
    ```

=== "C"

    ``` c
    DvzRequest dvz_delete_tex(  // returns: the request
        DvzBatch* batch,  // the batch
        DvzId id,  // the tex id
    );
    ```

### `dvz_mvp()`

Create a MVP structure.

=== "Python"

    ``` python
    dvz.mvp(  # returns: the MVP structure (DvzMVP)
        model,  # the model matrix (mat4)
        view,  # the view matrix (mat4)
        proj,  # the projection matrix (mat4)
    )
    ```

=== "C"

    ``` c
    DvzMVP dvz_mvp(  // returns: the MVP structure
        mat4 model,  // the model matrix
        mat4 view,  // the view matrix
        mat4 proj,  // the projection matrix
    );
    ```

### `dvz_mvp_default()`

Return a default DvzMVP struct

=== "Python"

    ``` python
    dvz.mvp_default()  # returns: the DvzMVP struct (DvzMVP)
    ```

=== "C"

    ``` c
    DvzMVP dvz_mvp_default();  // returns: the DvzMVP struct
    ```

### `dvz_record_begin()`

Create a request for starting recording of command buffer.

=== "Python"

    ``` python
    dvz.record_begin(  # returns: the request (DvzRequest)
        batch,  # the batch (LP_DvzBatch)
        canvas_id,  # the id of the canvas (int, 64-bit unsigned)
    )
    ```

=== "C"

    ``` c
    DvzRequest dvz_record_begin(  // returns: the request
        DvzBatch* batch,  // the batch
        DvzId canvas_id,  // the id of the canvas
    );
    ```

### `dvz_record_draw()`

Create a request for a direct draw of a graphics during command buffer recording.

=== "Python"

    ``` python
    dvz.record_draw(  # returns: the request (DvzRequest)
        batch,  # the batch (LP_DvzBatch)
        canvas_id,  # the id of the canvas (int, 64-bit unsigned)
        graphics,  # the id of the graphics pipe to draw (int, 64-bit unsigned)
        first_vertex,  # the index of the first vertex to draw (int, 32-bit unsigned)
        vertex_count,  # the number of vertices to draw (int, 32-bit unsigned)
        first_instance,  # the index of the first instance to draw (int, 32-bit unsigned)
        instance_count,  # the number of instances to draw (int, 32-bit unsigned)
    )
    ```

=== "C"

    ``` c
    DvzRequest dvz_record_draw(  // returns: the request
        DvzBatch* batch,  // the batch
        DvzId canvas_id,  // the id of the canvas
        DvzId graphics,  // the id of the graphics pipe to draw
        uint32_t first_vertex,  // the index of the first vertex to draw
        uint32_t vertex_count,  // the number of vertices to draw
        uint32_t first_instance,  // the index of the first instance to draw
        uint32_t instance_count,  // the number of instances to draw
    );
    ```

### `dvz_record_draw_indexed()`

Create a request for an indexed draw of a graphics during command buffer recording.

=== "Python"

    ``` python
    dvz.record_draw_indexed(  # returns: the request (DvzRequest)
        batch,  # the batch (LP_DvzBatch)
        canvas_id,  # the id of the canvas (int, 64-bit unsigned)
        graphics,  # the id of the graphics pipe to draw (int, 64-bit unsigned)
        first_index,  # the index of the first index to draw (int, 32-bit unsigned)
        vertex_offset,  # the vertex offset within the vertices indexed by the indexes (int, 32-bit unsigned)
        index_count,  # the number of indexes to draw (int, 32-bit unsigned)
        first_instance,  # the index of the first instance to draw (int, 32-bit unsigned)
        instance_count,  # the number of instances to draw (int, 32-bit unsigned)
    )
    ```

=== "C"

    ``` c
    DvzRequest dvz_record_draw_indexed(  // returns: the request
        DvzBatch* batch,  // the batch
        DvzId canvas_id,  // the id of the canvas
        DvzId graphics,  // the id of the graphics pipe to draw
        uint32_t first_index,  // the index of the first index to draw
        uint32_t vertex_offset,  // the vertex offset within the vertices indexed by the indexes
        uint32_t index_count,  // the number of indexes to draw
        uint32_t first_instance,  // the index of the first instance to draw
        uint32_t instance_count,  // the number of instances to draw
    );
    ```

### `dvz_record_draw_indexed_indirect()`

Create a request for an indexed indirect draw of a graphics during command buffer recording.

=== "Python"

    ``` python
    dvz.record_draw_indexed_indirect(  # returns: the request (DvzRequest)
        batch,  # the batch (LP_DvzBatch)
        canvas_id,  # the id of the canvas (int, 64-bit unsigned)
        graphics,  # the id of the graphics pipe to draw (int, 64-bit unsigned)
        indirect,  # the id of the dat containing the indirect draw data (int, 64-bit unsigned)
        draw_count,  # the number of draws to make (int, 32-bit unsigned)
    )
    ```

=== "C"

    ``` c
    DvzRequest dvz_record_draw_indexed_indirect(  // returns: the request
        DvzBatch* batch,  // the batch
        DvzId canvas_id,  // the id of the canvas
        DvzId graphics,  // the id of the graphics pipe to draw
        DvzId indirect,  // the id of the dat containing the indirect draw data
        uint32_t draw_count,  // the number of draws to make
    );
    ```

### `dvz_record_draw_indirect()`

Create a request for an indirect draw of a graphics during command buffer recording.

=== "Python"

    ``` python
    dvz.record_draw_indirect(  # returns: the request (DvzRequest)
        batch,  # the batch (LP_DvzBatch)
        canvas_id,  # the id of the canvas (int, 64-bit unsigned)
        graphics,  # the id of the graphics pipe to draw (int, 64-bit unsigned)
        indirect,  # the id of the dat containing the indirect draw data (int, 64-bit unsigned)
        draw_count,  # the number of draws to make (int, 32-bit unsigned)
    )
    ```

=== "C"

    ``` c
    DvzRequest dvz_record_draw_indirect(  // returns: the request
        DvzBatch* batch,  // the batch
        DvzId canvas_id,  // the id of the canvas
        DvzId graphics,  // the id of the graphics pipe to draw
        DvzId indirect,  // the id of the dat containing the indirect draw data
        uint32_t draw_count,  // the number of draws to make
    );
    ```

### `dvz_record_end()`

Create a request for ending recording of command buffer.

=== "Python"

    ``` python
    dvz.record_end(  # returns: the request (DvzRequest)
        batch,  # the batch (LP_DvzBatch)
        canvas_id,  # the id of the canvas (int, 64-bit unsigned)
    )
    ```

=== "C"

    ``` c
    DvzRequest dvz_record_end(  // returns: the request
        DvzBatch* batch,  // the batch
        DvzId canvas_id,  // the id of the canvas
    );
    ```

### `dvz_record_push()`

Create a request for sending a push constant value while recording a command buffer.

=== "Python"

    ``` python
    dvz.record_push(  # returns: the request (DvzRequest)
        batch,  # the batch (LP_DvzBatch)
        canvas_id,  # the id of the canvas (int, 64-bit unsigned)
        graphics_id,  # the id of the graphics pipeline (int, 64-bit unsigned)
        shader_stages,  # the shader stages (int, 32-bit signed)
        offset,  # the byte offset (int, 64-bit unsigned)
        size,  # the size of the data to upload (int, 64-bit unsigned)
        data,  # the push constant data to upload (ndpointer_any_C_CONTIGUOUS)
    )
    ```

=== "C"

    ``` c
    DvzRequest dvz_record_push(  // returns: the request
        DvzBatch* batch,  // the batch
        DvzId canvas_id,  // the id of the canvas
        DvzId graphics_id,  // the id of the graphics pipeline
        DvzShaderStageFlags shader_stages,  // the shader stages
        DvzSize offset,  // the byte offset
        DvzSize size,  // the size of the data to upload
        void* data,  // the push constant data to upload
    );
    ```

### `dvz_record_viewport()`

Create a request for setting the viewport during command buffer recording.

=== "Python"

    ``` python
    dvz.record_viewport(  # returns: the request (DvzRequest)
        batch,  # the batch (LP_DvzBatch)
        canvas_id,  # the id of the canvas (int, 64-bit unsigned)
        offset,  # the viewport offset, in framebuffer pixels (vec2)
        shape,  # the viewport size, in framebuffer pixels (vec2)
    )
    ```

=== "C"

    ``` c
    DvzRequest dvz_record_viewport(  // returns: the request
        DvzBatch* batch,  // the batch
        DvzId canvas_id,  // the id of the canvas
        vec2 offset,  // the viewport offset, in framebuffer pixels
        vec2 shape,  // the viewport size, in framebuffer pixels
    );
    ```

### `dvz_request_print()`

Display information about a request.

=== "Python"

    ``` python
    dvz.request_print(
        req,  # the request (LP_DvzRequest)
        flags,  # the flags (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    void dvz_request_print(
        DvzRequest* req,  // the request
        int flags,  // the flags
    );
    ```

### `dvz_requester()`

Create a requester, used to create requests.

=== "Python"

    ``` python
    dvz.requester()  # returns: the requester struct (LP_DvzRequester)
    ```

=== "C"

    ``` c
    DvzRequester* dvz_requester();  // returns: the requester struct
    ```

### `dvz_requester_commit()`

Add a batch's requests to a requester.

=== "Python"

    ``` python
    dvz.requester_commit(
        rqr,  # the requester (LP_DvzRequester)
        batch,  # the batch (LP_DvzBatch)
    )
    ```

=== "C"

    ``` c
    void dvz_requester_commit(
        DvzRequester* rqr,  // the requester
        DvzBatch* batch,  // the batch
    );
    ```

### `dvz_requester_destroy()`

Destroy a requester.

=== "Python"

    ``` python
    dvz.requester_destroy(
        rqr,  # the requester (LP_DvzRequester)
    )
    ```

=== "C"

    ``` c
    void dvz_requester_destroy(
        DvzRequester* rqr,  // the requester
    );
    ```

### `dvz_requester_flush()`

Return the requests in the requester and clear it.

=== "Python"

    ``` python
    dvz.requester_flush(  # returns: an array with all requests in the requester (LP_DvzBatch)
        rqr,  # the requester (LP_DvzRequester)
    )
    ```

=== "C"

    ``` c
    DvzBatch* dvz_requester_flush(  // returns: an array with all requests in the requester
        DvzRequester* rqr,  // the requester
    );
    ```

### `dvz_resize_canvas()`

Create a request to resize an offscreen canvas (regular canvases are resized by the client).

=== "Python"

    ``` python
    dvz.resize_canvas(  # returns: the request (DvzRequest)
        batch,  # the batch (LP_DvzBatch)
        canvas,  # the canvas id (int, 64-bit unsigned)
        width,  # the new canvas width (int, 32-bit unsigned)
        height,  # the new canvas height (int, 32-bit unsigned)
    )
    ```

=== "C"

    ``` c
    DvzRequest dvz_resize_canvas(  // returns: the request
        DvzBatch* batch,  // the batch
        DvzId canvas,  // the canvas id
        uint32_t width,  // the new canvas width
        uint32_t height,  // the new canvas height
    );
    ```

### `dvz_resize_dat()`

Create a request to resize a dat.

=== "Python"

    ``` python
    dvz.resize_dat(  # returns: the request (DvzRequest)
        batch,  # the batch (LP_DvzBatch)
        dat,  # the dat id (int, 64-bit unsigned)
        size,  # the new dat size, in bytes (int, 64-bit unsigned)
    )
    ```

=== "C"

    ``` c
    DvzRequest dvz_resize_dat(  // returns: the request
        DvzBatch* batch,  // the batch
        DvzId dat,  // the dat id
        DvzSize size,  // the new dat size, in bytes
    );
    ```

### `dvz_resize_tex()`

Create a request to resize a tex.

=== "Python"

    ``` python
    dvz.resize_tex(  # returns: the request (DvzRequest)
        batch,  # the batch (LP_DvzBatch)
        tex,  # the tex id (int, 64-bit unsigned)
        shape,  # the new tex shape (uvec3)
    )
    ```

=== "C"

    ``` c
    DvzRequest dvz_resize_tex(  // returns: the request
        DvzBatch* batch,  // the batch
        DvzId tex,  // the tex id
        uvec3 shape,  // the new tex shape
    );
    ```

### `dvz_set_attr()`

Create a request for setting a vertex attribute of a graphics pipe.

=== "Python"

    ``` python
    dvz.set_attr(  # returns: the request (DvzRequest)
        batch,  # the batch (LP_DvzBatch)
        graphics,  # the graphics pipe id (int, 64-bit unsigned)
        binding_idx,  # the index of the vertex binding (int, 32-bit unsigned)
        location,  # the GLSL attribute location (int, 32-bit unsigned)
        format,  # the attribute format (DvzFormat)
        offset,  # the byte offset of the attribute within the vertex binding (int, 64-bit unsigned)
    )
    ```

=== "C"

    ``` c
    DvzRequest dvz_set_attr(  // returns: the request
        DvzBatch* batch,  // the batch
        DvzId graphics,  // the graphics pipe id
        uint32_t binding_idx,  // the index of the vertex binding
        uint32_t location,  // the GLSL attribute location
        DvzFormat format,  // the attribute format
        DvzSize offset,  // the byte offset of the attribute within the vertex binding
    );
    ```

### `dvz_set_background()`

Change the background color of the canvas.

=== "Python"

    ``` python
    dvz.set_background(  # returns: the request (DvzRequest)
        batch,  # the batch (LP_DvzBatch)
        id,  # the canvas id (int, 64-bit unsigned)
        background,  # the background color (cvec4)
    )
    ```

=== "C"

    ``` c
    DvzRequest dvz_set_background(  // returns: the request
        DvzBatch* batch,  // the batch
        DvzId id,  // the canvas id
        cvec4 background,  // the background color
    );
    ```

### `dvz_set_blend()`

Create a request for setting the blend type of a graphics pipe.

=== "Python"

    ``` python
    dvz.set_blend(  # returns: the request (DvzRequest)
        batch,  # the batch (LP_DvzBatch)
        graphics,  # the graphics pipe id (int, 64-bit unsigned)
        blend_type,  # the graphics blend type (DvzBlendType)
    )
    ```

=== "C"

    ``` c
    DvzRequest dvz_set_blend(  // returns: the request
        DvzBatch* batch,  // the batch
        DvzId graphics,  // the graphics pipe id
        DvzBlendType blend_type,  // the graphics blend type
    );
    ```

### `dvz_set_cull()`

Create a request for setting the cull mode of a graphics pipe.

=== "Python"

    ``` python
    dvz.set_cull(  # returns: the request (DvzRequest)
        batch,  # the batch (LP_DvzBatch)
        graphics,  # the graphics pipe id (int, 64-bit unsigned)
        cull_mode,  # the cull mode (DvzCullMode)
    )
    ```

=== "C"

    ``` c
    DvzRequest dvz_set_cull(  // returns: the request
        DvzBatch* batch,  // the batch
        DvzId graphics,  // the graphics pipe id
        DvzCullMode cull_mode,  // the cull mode
    );
    ```

### `dvz_set_depth()`

Create a request for setting the depth test of a graphics pipe.

=== "Python"

    ``` python
    dvz.set_depth(  # returns: the request (DvzRequest)
        batch,  # the batch (LP_DvzBatch)
        graphics,  # the graphics pipe id (int, 64-bit unsigned)
        depth_test,  # the graphics depth test (DvzDepthTest)
    )
    ```

=== "C"

    ``` c
    DvzRequest dvz_set_depth(  // returns: the request
        DvzBatch* batch,  // the batch
        DvzId graphics,  // the graphics pipe id
        DvzDepthTest depth_test,  // the graphics depth test
    );
    ```

### `dvz_set_front()`

Create a request for setting the front face of a graphics pipe.

=== "Python"

    ``` python
    dvz.set_front(  # returns: the request (DvzRequest)
        batch,  # the batch (LP_DvzBatch)
        graphics,  # the graphics pipe id (int, 64-bit unsigned)
        front_face,  # the front face (DvzFrontFace)
    )
    ```

=== "C"

    ``` c
    DvzRequest dvz_set_front(  // returns: the request
        DvzBatch* batch,  // the batch
        DvzId graphics,  // the graphics pipe id
        DvzFrontFace front_face,  // the front face
    );
    ```

### `dvz_set_mask()`

Create a request for setting the color mask of a graphics pipe.

=== "Python"

    ``` python
    dvz.set_mask(  # returns: the request (DvzRequest)
        batch,  # the batch (LP_DvzBatch)
        graphics,  # the graphics pipe id (int, 64-bit unsigned)
        mask,  # the mask with RGBA boolean masks on the lower bits (int, 32-bit signed)
    )
    ```

=== "C"

    ``` c
    DvzRequest dvz_set_mask(  // returns: the request
        DvzBatch* batch,  // the batch
        DvzId graphics,  // the graphics pipe id
        int32_t mask,  // the mask with RGBA boolean masks on the lower bits
    );
    ```

### `dvz_set_polygon()`

Create a request for setting the polygon mode of a graphics pipe.

=== "Python"

    ``` python
    dvz.set_polygon(  # returns: the request (DvzRequest)
        batch,  # the batch (LP_DvzBatch)
        graphics,  # the graphics pipe id (int, 64-bit unsigned)
        polygon_mode,  # the polygon mode (DvzPolygonMode)
    )
    ```

=== "C"

    ``` c
    DvzRequest dvz_set_polygon(  // returns: the request
        DvzBatch* batch,  // the batch
        DvzId graphics,  // the graphics pipe id
        DvzPolygonMode polygon_mode,  // the polygon mode
    );
    ```

### `dvz_set_primitive()`

Create a request for setting the primitive topology of a graphics pipe.

=== "Python"

    ``` python
    dvz.set_primitive(  # returns: the request (DvzRequest)
        batch,  # the batch (LP_DvzBatch)
        graphics,  # the graphics pipe id (int, 64-bit unsigned)
        primitive,  # the graphics primitive topology (DvzPrimitiveTopology)
    )
    ```

=== "C"

    ``` c
    DvzRequest dvz_set_primitive(  // returns: the request
        DvzBatch* batch,  // the batch
        DvzId graphics,  // the graphics pipe id
        DvzPrimitiveTopology primitive,  // the graphics primitive topology
    );
    ```

### `dvz_set_push()`

Create a request for setting a push constant layout for a graphics pipe.

=== "Python"

    ``` python
    dvz.set_push(  # returns: the request (DvzRequest)
        batch,  # the batch (LP_DvzBatch)
        graphics,  # the graphics pipe id (int, 64-bit unsigned)
        shader_stages,  # the shader stages with the push constant (int, 32-bit signed)
        offset,  # the byte offset for the push data visibility from the shader (int, 64-bit unsigned)
        size,  # how much bytes the shader can see from the push constant (int, 64-bit unsigned)
    )
    ```

=== "C"

    ``` c
    DvzRequest dvz_set_push(  // returns: the request
        DvzBatch* batch,  // the batch
        DvzId graphics,  // the graphics pipe id
        DvzShaderStageFlags shader_stages,  // the shader stages with the push constant
        DvzSize offset,  // the byte offset for the push data visibility from the shader
        DvzSize size,  // how much bytes the shader can see from the push constant
    );
    ```

### `dvz_set_shader()`

Create a request for setting a shader a graphics pipe.

=== "Python"

    ``` python
    dvz.set_shader(  # returns: the request (DvzRequest)
        batch,  # the batch (LP_DvzBatch)
        graphics,  # the graphics pipe id (int, 64-bit unsigned)
        shader,  # the id of the shader object (int, 64-bit unsigned)
    )
    ```

=== "C"

    ``` c
    DvzRequest dvz_set_shader(  // returns: the request
        DvzBatch* batch,  // the batch
        DvzId graphics,  // the graphics pipe id
        DvzId shader,  // the id of the shader object
    );
    ```

### `dvz_set_slot()`

Create a request for setting a binding slot (descriptor) of a graphics pipe.

=== "Python"

    ``` python
    dvz.set_slot(  # returns: the request (DvzRequest)
        batch,  # the batch (LP_DvzBatch)
        graphics,  # the graphics pipe id (int, 64-bit unsigned)
        slot_idx,  # the index of the GLSL binding slot (int, 32-bit unsigned)
        type,  # the descriptor type (DvzDescriptorType)
    )
    ```

=== "C"

    ``` c
    DvzRequest dvz_set_slot(  // returns: the request
        DvzBatch* batch,  // the batch
        DvzId graphics,  // the graphics pipe id
        uint32_t slot_idx,  // the index of the GLSL binding slot
        DvzDescriptorType type,  // the descriptor type
    );
    ```

### `dvz_set_specialization()`

Create a request for setting a specialization constant of a graphics pipe.

=== "Python"

    ``` python
    dvz.set_specialization(  # returns: the request (DvzRequest)
        batch,  # the batch (LP_DvzBatch)
        graphics,  # the graphics pipe id (int, 64-bit unsigned)
        shader,  # the shader with the specialization constant (DvzShaderType)
        idx,  # the specialization constant index as specified in the GLSL code (int, 32-bit unsigned)
        size,  # the byte size of the value (int, 64-bit unsigned)
        value,  # a pointer to the specialization constant value (array)
    )
    ```

=== "C"

    ``` c
    DvzRequest dvz_set_specialization(  // returns: the request
        DvzBatch* batch,  // the batch
        DvzId graphics,  // the graphics pipe id
        DvzShaderType shader,  // the shader with the specialization constant
        uint32_t idx,  // the specialization constant index as specified in the GLSL code
        DvzSize size,  // the byte size of the value
        void* value,  // a pointer to the specialization constant value
    );
    ```

### `dvz_set_vertex()`

Create a request for setting a vertex binding of a graphics pipe.

=== "Python"

    ``` python
    dvz.set_vertex(  # returns: the request (DvzRequest)
        batch,  # the batch (LP_DvzBatch)
        graphics,  # the graphics pipe id (int, 64-bit unsigned)
        binding_idx,  # the index of the vertex binding (int, 32-bit unsigned)
        stride,  # the binding stride (int, 64-bit unsigned)
    )
    ```

=== "C"

    ``` c
    DvzRequest dvz_set_vertex(  // returns: the request
        DvzBatch* batch,  // the batch
        DvzId graphics,  // the graphics pipe id
        uint32_t binding_idx,  // the index of the vertex binding
        DvzSize stride,  // the binding stride
    );
    ```

### `dvz_update_canvas()`

Create a request for a canvas redraw (command buffer submission).

=== "Python"

    ``` python
    dvz.update_canvas(  # returns: the request (DvzRequest)
        batch,  # the batch (LP_DvzBatch)
        id,  # the canvas id (int, 64-bit unsigned)
    )
    ```

=== "C"

    ``` c
    DvzRequest dvz_update_canvas(  // returns: the request
        DvzBatch* batch,  // the batch
        DvzId id,  // the canvas id
    );
    ```

### `dvz_upload_dat()`

Create a request for dat upload.

=== "Python"

    ``` python
    dvz.upload_dat(  # returns: the request (DvzRequest)
        batch,  # the batch (LP_DvzBatch)
        dat,  # the id of the dat to upload to (int, 64-bit unsigned)
        offset,  # the byte offset of the upload transfer (int, 64-bit unsigned)
        size,  # the number of bytes in data to transfer (int, 64-bit unsigned)
        data,  # a pointer to the data to upload (ndpointer_any_C_CONTIGUOUS)
    )
    ```

=== "C"

    ``` c
    DvzRequest dvz_upload_dat(  // returns: the request
        DvzBatch* batch,  // the batch
        DvzId dat,  // the id of the dat to upload to
        DvzSize offset,  // the byte offset of the upload transfer
        DvzSize size,  // the number of bytes in data to transfer
        void* data,  // a pointer to the data to upload
    );
    ```

### `dvz_upload_tex()`

Create a request for tex upload.

=== "Python"

    ``` python
    dvz.upload_tex(  # returns: the request (DvzRequest)
        batch,  # the batch (LP_DvzBatch)
        tex,  # the id of the tex to upload to (int, 64-bit unsigned)
        offset,  # the offset (uvec3)
        shape,  # the shape (uvec3)
        size,  # the number of bytes in data to transfer (int, 64-bit unsigned)
        data,  # a pointer to the data to upload (ndpointer_any_C_CONTIGUOUS)
    )
    ```

=== "C"

    ``` c
    DvzRequest dvz_upload_tex(  // returns: the request
        DvzBatch* batch,  // the batch
        DvzId tex,  // the id of the tex to upload to
        uvec3 offset,  // the offset
        uvec3 shape,  // the shape
        DvzSize size,  // the number of bytes in data to transfer
        void* data,  // a pointer to the data to upload
    );
    ```

### `dvz_viewport_default()`

Return a default viewport

=== "Python"

    ``` python
    dvz.viewport_default(  # returns: the viewport (DvzViewport)
        width,  # the viewport width, in framebuffer pixels (int, 32-bit unsigned)
        height,  # the viewport height, in framebuffer pixels (int, 32-bit unsigned)
    )
    ```

=== "C"

    ``` c
    DvzViewport dvz_viewport_default(  // returns: the viewport
        uint32_t width,  // the viewport width, in framebuffer pixels
        uint32_t height,  // the viewport height, in framebuffer pixels
    );
    ```

## Enumerations

### `DvzAlign`

```
DVZ_ALIGN_NONE
DVZ_ALIGN_LOW
DVZ_ALIGN_MIDDLE
DVZ_ALIGN_HIGH
```

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

### `DvzBlendType`

```
DVZ_BLEND_DISABLE
DVZ_BLEND_STANDARD
DVZ_BLEND_DESTINATION
DVZ_BLEND_OIT
```

### `DvzBoxExtentStrategy`

```
DVZ_BOX_EXTENT_DEFAULT
DVZ_BOX_EXTENT_FIXED_ASPECT_EXPAND
DVZ_BOX_EXTENT_FIXED_ASPECT_CONTRACT
```

### `DvzBoxMergeStrategy`

```
DVZ_BOX_MERGE_DEFAULT
DVZ_BOX_MERGE_CENTER
DVZ_BOX_MERGE_CORNER
```

### `DvzBufferType`

```
DVZ_BUFFER_TYPE_UNDEFINED
DVZ_BUFFER_TYPE_STAGING
DVZ_BUFFER_TYPE_VERTEX
DVZ_BUFFER_TYPE_INDEX
DVZ_BUFFER_TYPE_STORAGE
DVZ_BUFFER_TYPE_UNIFORM
DVZ_BUFFER_TYPE_INDIRECT
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

### `DvzColorMask`

```
DVZ_MASK_COLOR_R
DVZ_MASK_COLOR_G
DVZ_MASK_COLOR_B
DVZ_MASK_COLOR_A
DVZ_MASK_COLOR_ALL
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

### `DvzCullMode`

```
DVZ_CULL_MODE_NONE
DVZ_CULL_MODE_FRONT
DVZ_CULL_MODE_BACK
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

### `DvzDescriptorType`

```
DVZ_DESCRIPTOR_TYPE_SAMPLER
DVZ_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
DVZ_DESCRIPTOR_TYPE_SAMPLED_IMAGE
DVZ_DESCRIPTOR_TYPE_STORAGE_IMAGE
DVZ_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER
DVZ_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER
DVZ_DESCRIPTOR_TYPE_UNIFORM_BUFFER
DVZ_DESCRIPTOR_TYPE_STORAGE_BUFFER
DVZ_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC
DVZ_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC
```

### `DvzDialogFlags`

```
DVZ_DIALOG_FLAGS_NONE
DVZ_DIALOG_FLAGS_OVERLAY
DVZ_DIALOG_FLAGS_BLANK
DVZ_DIALOG_FLAGS_PANEL
```

### `DvzDim`

```
DVZ_DIM_X
DVZ_DIM_Y
DVZ_DIM_Z
DVZ_DIM_COUNT
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
DVZ_FORMAT_R64_UINT
DVZ_FORMAT_R64_SINT
DVZ_FORMAT_R64_SFLOAT
DVZ_FORMAT_R64G64_UINT
DVZ_FORMAT_R64G64_SINT
DVZ_FORMAT_R64G64_SFLOAT
DVZ_FORMAT_R64G64B64_UINT
DVZ_FORMAT_R64G64B64_SINT
DVZ_FORMAT_R64G64B64_SFLOAT
DVZ_FORMAT_R64G64B64A64_UINT
DVZ_FORMAT_R64G64B64A64_SINT
DVZ_FORMAT_R64G64B64A64_SFLOAT
```

### `DvzFrontFace`

```
DVZ_FRONT_FACE_COUNTER_CLOCKWISE
DVZ_FRONT_FACE_CLOCKWISE
```

### `DvzGraphicsType`

```
DVZ_GRAPHICS_NONE
DVZ_GRAPHICS_POINT
DVZ_GRAPHICS_TRIANGLE
DVZ_GRAPHICS_CUSTOM
```

### `DvzGuiFlags`

```
DVZ_GUI_FLAGS_NONE
DVZ_GUI_FLAGS_OFFSCREEN
DVZ_GUI_FLAGS_DOCKING
```

### `DvzImageFlags`

```
DVZ_IMAGE_FLAGS_SIZE_PIXELS
DVZ_IMAGE_FLAGS_SIZE_NDC
DVZ_IMAGE_FLAGS_RESCALE_KEEP_RATIO
DVZ_IMAGE_FLAGS_RESCALE
DVZ_IMAGE_FLAGS_MODE_RGBA
DVZ_IMAGE_FLAGS_MODE_COLORMAP
DVZ_IMAGE_FLAGS_MODE_FILL
DVZ_IMAGE_FLAGS_BORDER
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

### `DvzOrientation`

```
DVZ_ORIENTATION_DEFAULT
DVZ_ORIENTATION_UP
DVZ_ORIENTATION_REVERSE
DVZ_ORIENTATION_DOWN
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

### `DvzPolygonMode`

```
DVZ_POLYGON_MODE_FILL
DVZ_POLYGON_MODE_LINE
DVZ_POLYGON_MODE_POINT
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

### `DvzRecorderCommandType`

```
DVZ_RECORDER_NONE
DVZ_RECORDER_BEGIN
DVZ_RECORDER_DRAW
DVZ_RECORDER_DRAW_INDEXED
DVZ_RECORDER_DRAW_INDIRECT
DVZ_RECORDER_DRAW_INDEXED_INDIRECT
DVZ_RECORDER_VIEWPORT
DVZ_RECORDER_PUSH
DVZ_RECORDER_END
DVZ_RECORDER_COUNT
```

### `DvzRefFlags`

```
DVZ_REF_FLAGS_NONE
DVZ_REF_FLAGS_EQUAL
```

### `DvzRequestAction`

```
DVZ_REQUEST_ACTION_NONE
DVZ_REQUEST_ACTION_CREATE
DVZ_REQUEST_ACTION_DELETE
DVZ_REQUEST_ACTION_RESIZE
DVZ_REQUEST_ACTION_UPDATE
DVZ_REQUEST_ACTION_BIND
DVZ_REQUEST_ACTION_RECORD
DVZ_REQUEST_ACTION_UPLOAD
DVZ_REQUEST_ACTION_UPFILL
DVZ_REQUEST_ACTION_DOWNLOAD
DVZ_REQUEST_ACTION_SET
DVZ_REQUEST_ACTION_GET
```

### `DvzRequestObject`

```
DVZ_REQUEST_OBJECT_NONE
DVZ_REQUEST_OBJECT_CANVAS
DVZ_REQUEST_OBJECT_DAT
DVZ_REQUEST_OBJECT_TEX
DVZ_REQUEST_OBJECT_SAMPLER
DVZ_REQUEST_OBJECT_COMPUTE
DVZ_REQUEST_OBJECT_PRIMITIVE
DVZ_REQUEST_OBJECT_DEPTH
DVZ_REQUEST_OBJECT_BLEND
DVZ_REQUEST_OBJECT_MASK
DVZ_REQUEST_OBJECT_POLYGON
DVZ_REQUEST_OBJECT_CULL
DVZ_REQUEST_OBJECT_FRONT
DVZ_REQUEST_OBJECT_SHADER
DVZ_REQUEST_OBJECT_VERTEX
DVZ_REQUEST_OBJECT_VERTEX_ATTR
DVZ_REQUEST_OBJECT_SLOT
DVZ_REQUEST_OBJECT_PUSH
DVZ_REQUEST_OBJECT_SPECIALIZATION
DVZ_REQUEST_OBJECT_GRAPHICS
DVZ_REQUEST_OBJECT_INDEX
DVZ_REQUEST_OBJECT_BACKGROUND
DVZ_REQUEST_OBJECT_RECORD
```

### `DvzSamplerAddressMode`

```
DVZ_SAMPLER_ADDRESS_MODE_REPEAT
DVZ_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT
DVZ_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
DVZ_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER
DVZ_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE
```

### `DvzSamplerAxis`

```
DVZ_SAMPLER_AXIS_U
DVZ_SAMPLER_AXIS_V
DVZ_SAMPLER_AXIS_W
```

### `DvzSceneFont`

```
DVZ_SCENE_FONT_MONO
DVZ_SCENE_FONT_LABEL
DVZ_SCENE_FONT_COUNT
```

### `DvzShaderFormat`

```
DVZ_SHADER_NONE
DVZ_SHADER_SPIRV
DVZ_SHADER_GLSL
```

### `DvzShaderType`

```
DVZ_SHADER_VERTEX
DVZ_SHADER_TESSELLATION_CONTROL
DVZ_SHADER_TESSELLATION_EVALUATION
DVZ_SHADER_GEOMETRY
DVZ_SHADER_FRAGMENT
DVZ_SHADER_COMPUTE
```

### `DvzShapeIndexingFlags`

```
DVZ_INDEXING_NONE
DVZ_INDEXING_EARCUT
DVZ_INDEXING_SURFACE
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
DVZ_SHAPE_TETRAHEDRON
DVZ_SHAPE_HEXAHEDRON
DVZ_SHAPE_OCTAHEDRON
DVZ_SHAPE_DODECAHEDRON
DVZ_SHAPE_ICOSAHEDRON
DVZ_SHAPE_SURFACE
DVZ_SHAPE_OBJ
DVZ_SHAPE_OTHER
```

### `DvzSlotType`

```
DVZ_SLOT_DAT
DVZ_SLOT_TEX
DVZ_SLOT_COUNT
```

### `DvzTexDims`

```
DVZ_TEX_NONE
DVZ_TEX_1D
DVZ_TEX_2D
DVZ_TEX_3D
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

### `DvzVertexInputRate`

```
DVZ_VERTEX_INPUT_RATE_VERTEX
DVZ_VERTEX_INPUT_RATE_INSTANCE
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

### `DvzGraphicsRequestFlags`

```
DVZ_GRAPHICS_REQUEST_FLAGS_NONE
DVZ_GRAPHICS_REQUEST_FLAGS_OFFSCREEN
```

### `DvzPrintFlagsFlags`

```
DVZ_PRINT_FLAGS_NONE
DVZ_PRINT_FLAGS_DATA
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

### `DvzBatch`

```
struct DvzBatch
    uint32_t capacity
    uint32_t count
    DvzRequest* requests
    DvzList* pointers_to_free
    int flags
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
    DvzGuiWindow* gui_window
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
    bool is_press_valid
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

### `DvzRecorderCommand`

```
struct DvzRecorderCommand
    DvzRecorderCommandType type
    DvzId canvas_id
    DvzRequestObject object_type
    DvzRecorderUnion contents
```

### `DvzRecorderDraw`

```
struct DvzRecorderDraw
    DvzId pipe_id
    uint32_t first_vertex
    uint32_t vertex_count
    uint32_t first_instance
    uint32_t instance_count
```

### `DvzRecorderDrawIndexed`

```
struct DvzRecorderDrawIndexed
    DvzId pipe_id
    uint32_t first_index
    uint32_t vertex_offset
    uint32_t index_count
    uint32_t first_instance
    uint32_t instance_count
```

### `DvzRecorderDrawIndexedIndirect`

```
struct DvzRecorderDrawIndexedIndirect
    DvzId pipe_id
    DvzId dat_indirect_id
    uint32_t draw_count
```

### `DvzRecorderDrawIndirect`

```
struct DvzRecorderDrawIndirect
    DvzId pipe_id
    DvzId dat_indirect_id
    uint32_t draw_count
```

### `DvzRecorderPush`

```
struct DvzRecorderPush
    DvzId pipe_id
    DvzShaderStageFlags shader_stages
    DvzSize offset
    DvzSize size
    void* data
```

### `DvzRecorderUnion`

```
union DvzRecorderUnion
    DvzRecorderViewport v
    DvzRecorderPush p
    DvzRecorderDraw draw
    DvzRecorderDrawIndexed draw_indexed
    DvzRecorderDrawIndirect draw_indirect
    DvzRecorderDrawIndexedIndirect draw_indexed_indirect
```

### `DvzRecorderViewport`

```
struct DvzRecorderViewport
    vec2 offset
    vec2 shape
```

### `DvzRequest`

```
struct DvzRequest
    uint32_t version
    DvzRequestAction action
    DvzRequestObject type
    DvzId id
    DvzRequestContent content
    int tag
    int flags
    char* desc
```

### `DvzRequestAttr`

```
struct DvzRequestAttr
    uint32_t binding_idx
    uint32_t location
    DvzFormat format
    DvzSize offset
```

### `DvzRequestBindDat`

```
struct DvzRequestBindDat
    uint32_t slot_idx
    DvzId dat
    DvzSize offset
```

### `DvzRequestBindIndex`

```
struct DvzRequestBindIndex
    DvzId dat
    DvzSize offset
```

### `DvzRequestBindTex`

```
struct DvzRequestBindTex
    uint32_t slot_idx
    DvzId tex
    DvzId sampler
    uvec3 offset
```

### `DvzRequestBindVertex`

```
struct DvzRequestBindVertex
    uint32_t binding_idx
    DvzId dat
    DvzSize offset
```

### `DvzRequestBlend`

```
struct DvzRequestBlend
    DvzBlendType blend
```

### `DvzRequestBoard`

```
struct DvzRequestBoard
    uint32_t width
    uint32_t height
    cvec4 background
```

### `DvzRequestCanvas`

```
struct DvzRequestCanvas
    uint32_t framebuffer_width
    uint32_t framebuffer_height
    uint32_t screen_width
    uint32_t screen_height
    bool is_offscreen
    cvec4 background
```

### `DvzRequestContent`

```
union DvzRequestContent
    DvzRequestCanvas canvas
    DvzRequestDat dat
    DvzRequestTex tex
    DvzRequestSampler sampler
    DvzRequestShader shader
    DvzRequestDatUpload dat_upload
    DvzRequestTexUpload tex_upload
    DvzRequestGraphics graphics
    DvzRequestPrimitive set_primitive
    DvzRequestBlend set_blend
    DvzRequestMask set_mask
    DvzRequestDepth set_depth
    DvzRequestPolygon set_polygon
    DvzRequestCull set_cull
    DvzRequestFront set_front
    DvzRequestShaderSet set_shader
    DvzRequestVertex set_vertex
    DvzRequestAttr set_attr
    DvzRequestSlot set_slot
    DvzRequestPush set_push
    DvzRequestSpecialization set_specialization
    DvzRequestBindVertex bind_vertex
    DvzRequestBindIndex bind_index
    DvzRequestBindDat bind_dat
    DvzRequestBindTex bind_tex
    DvzRequestRecord record
```

### `DvzRequestCull`

```
struct DvzRequestCull
    DvzCullMode cull
```

### `DvzRequestDat`

```
struct DvzRequestDat
    DvzBufferType type
    DvzSize size
```

### `DvzRequestDatUpload`

```
struct DvzRequestDatUpload
    int upload_type
    DvzSize offset
    DvzSize size
    void* data
```

### `DvzRequestDepth`

```
struct DvzRequestDepth
    DvzDepthTest depth
```

### `DvzRequestFront`

```
struct DvzRequestFront
    DvzFrontFace front
```

### `DvzRequestGraphics`

```
struct DvzRequestGraphics
    DvzGraphicsType type
```

### `DvzRequestMask`

```
struct DvzRequestMask
    int32_t mask
```

### `DvzRequestPolygon`

```
struct DvzRequestPolygon
    DvzPolygonMode polygon
```

### `DvzRequestPrimitive`

```
struct DvzRequestPrimitive
    DvzPrimitiveTopology primitive
```

### `DvzRequestPush`

```
struct DvzRequestPush
    DvzShaderStageFlags shader_stages
    DvzSize offset
    DvzSize size
```

### `DvzRequestRecord`

```
struct DvzRequestRecord
    DvzRecorderCommand command
```

### `DvzRequestSampler`

```
struct DvzRequestSampler
    DvzFilter filter
    DvzSamplerAddressMode mode
```

### `DvzRequestShader`

```
struct DvzRequestShader
    DvzShaderFormat format
    DvzShaderType type
    DvzSize size
    char* code
    uint32_t* buffer
```

### `DvzRequestShaderSet`

```
struct DvzRequestShaderSet
    DvzId shader
```

### `DvzRequestSlot`

```
struct DvzRequestSlot
    uint32_t slot_idx
    DvzDescriptorType type
```

### `DvzRequestSpecialization`

```
struct DvzRequestSpecialization
    DvzShaderType shader
    uint32_t idx
    DvzSize size
    void* value
```

### `DvzRequestTex`

```
struct DvzRequestTex
    DvzTexDims dims
    uvec3 shape
    DvzFormat format
```

### `DvzRequestTexUpload`

```
struct DvzRequestTexUpload
    int upload_type
    uvec3 offset
    uvec3 shape
    DvzSize size
    void* data
```

### `DvzRequestVertex`

```
struct DvzRequestVertex
    uint32_t binding_idx
    DvzSize stride
    DvzVertexInputRate input_rate
```

### `DvzRequester`

```
struct DvzRequester
    DvzFifo* fifo
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
    DvzColor* color
    vec4* texcoords
    float* isoline
    vec3* d_left
    vec3* d_right
    cvec4* contour
    DvzIndex* index
```

### `DvzTime`

```
struct DvzTime
    uint64_t seconds
    uint64_t nanoseconds
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

### `DvzViewport`

```
struct DvzViewport
    _VkViewport viewport
    vec4 margins
    uvec2 offset_screen
    uvec2 size_screen
    uvec2 offset_framebuffer
    uvec2 size_framebuffer
    int flags
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

### `_VkViewport`

```
struct _VkViewport
    float x
    float y
    float width
    float height
    float minDepth
    float maxDepth
```

