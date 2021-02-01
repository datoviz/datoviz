# Writing custom visuals

In this section, we'll show how to create a custom visual based on an existing graphics pipeline.

!!! note
    Only the C API supports custom visuals at the moment. Python bindings for custom visuals will come in an upcoming version.

The full source code for this example can be found in `examples/custom_visual.c`.



## Distinction between graphics and visuals

Datoviz makes the distinction between a **graphics** (graphics pipeline) and a **visual**:

* the **graphics** is a GPU-level object. It is defined by a vertex shader, a fragment shader, a primitive type (point, line, triangle), and other details.
* the **visual** is a user-level object. It encapsulates a particular type of visual element and abstracts away the GPU implementation details. A visual is defined by one or several graphics pipelines, optional compute pipelines, and a set of **visual properties** that allow the user to specify the visual's data.

Importantly, the user doesn't need to know the internal implementation details of a visual to use it. It is normally sufficient to know the props specification.



## Vertex buffer and structure

TODO



## Making a custom visual based on existing graphics

In this section, we'll show how to **create a custom visual by reusing an existing graphics**. The main use-case for this scenario is making a visual with a **custom CPU data transformation pipeline**.

An example is polygon triangulation: since the GPU can only render triangles, one needs to triangulate an arbitrary shape before rendering it. The triangulation is implemented at the level of the visual, so that the user can pass the polygon points without having to triangulate it manually. The visual makes the triangulation internally, and generates the triangles required by the underlying graphics pipeline.

In the simple example below, we'll implement a simple **square visual** with a trivial triangulation (two triangles per square). The custom rectangle visual will provide the following props:

| Type | Index | Type | Description |
| ---- | ---- | ---- | ---- |
| `pos` | 0 | `dvec3` | square center position |
| `color` | 0 | `cvec4` | square color |
| `length` | 0 | `float` | edge length |

The underlying graphics will be the `triangle` graphics, where three successive vertices define a single independent triangle. Each square will be triangulated into two triangles, or six vertices. This square visual will be easier to use than the triangle one, since the user won't have to deal with triangulation manually.

```c
// pos prop, dvec3 data type
dvz_visual_prop(visual, DVZ_PROP_POS, 0, DVZ_DTYPE_DVEC3, DVZ_SOURCE_TYPE_VERTEX, 0);
// color prop, cvec4 data type
dvz_visual_prop(visual, DVZ_PROP_COLOR, 0, DVZ_DTYPE_CVEC4, DVZ_SOURCE_TYPE_VERTEX, 0);
// length prop, float data type
dvz_visual_prop(visual, DVZ_PROP_LENGTH, 0, DVZ_DTYPE_FLOAT, DVZ_SOURCE_TYPE_VERTEX, 0);
```



## Visual baking function

Once the props are defined, the most important step when creating a custom visual is to implement the **baking function**.

The baking function takes the props data as input, and fills in the vertex buffer, as well as, possibly, other GPU data sources (uniforms, storage buffers, textures).

Here, the square baking function must recover the `pos` and `length` props in order to compute the four corner position of each square, and make the triangulation with two triangles per square.

The props and the vertex buffer (and other sources) come with `DvzArray` instances, which are thin wrappers around 1D arrays of homogeneous data types. They support few features: no multidimensional arrays, no vectorized operations.

```c
// The following code snippet contains the body of the baking function.

static void _bake_callback(DvzVisual* visual, DvzVisualDataEvent ev)
{
    ASSERT(visual != NULL);

    // First, we obtain the array instances holding the prop data as specified by the user.
    DvzArray* arr_pos = dvz_prop_array(visual, DVZ_PROP_POS, 0);
    DvzArray* arr_color = dvz_prop_array(visual, DVZ_PROP_COLOR, 0);
    DvzArray* arr_length = dvz_prop_array(visual, DVZ_PROP_LENGTH, 0);

    // We also get the array of the vertex buffer, which we'll need to fill with the triangulation.
    DvzArray* arr_vertex = dvz_source_array(visual, DVZ_SOURCE_TYPE_VERTEX, 0);

    // The number of rows in the 1D position array (set by the user) is the number of squares
    // requested by the user.
    uint32_t square_count = arr_pos->item_count;

    // We resize the vertex buffer array so that it holds six vertices per square (two triangles).
    dvz_array_resize(arr_vertex, 6 * square_count);

    // Pointers to the input data.
    dvec3* pos = NULL;
    cvec4* color = NULL;
    float* length = NULL;

    // Pointer to the output vertex.
    DvzVertex* vertex = (DvzVertex*)arr_vertex->data;

    // Here, we triangulate each square by computing the position of each square corner.
    float hl = 0;
    for (uint32_t i = 0; i < square_count; i++)
    {
        // We get a pointer to the current item in each prop array.
        pos = dvz_array_item(arr_pos, i);
        color = dvz_array_item(arr_color, i);
        length = dvz_array_item(arr_length, i);

        // This is the half of the square size.
        hl = (*length) / 2;

        // First triangle:

        // Bottom-left corner.
        vertex[6 * i + 0].pos[0] = pos[0][0] - hl;
        vertex[6 * i + 0].pos[1] = pos[0][1] - hl;

        // Bottom-right corner.
        vertex[6 * i + 1].pos[0] = pos[0][0] + hl;
        vertex[6 * i + 1].pos[1] = pos[0][1] - hl;

        // Top-right corner.
        vertex[6 * i + 2].pos[0] = pos[0][0] + hl;
        vertex[6 * i + 2].pos[1] = pos[0][1] + hl;

        // Second triangle:

        // Top-right corner again.
        vertex[6 * i + 3].pos[0] = pos[0][0] + hl;
        vertex[6 * i + 3].pos[1] = pos[0][1] + hl;

        // Top-left corner.
        vertex[6 * i + 4].pos[0] = pos[0][0] - hl;
        vertex[6 * i + 4].pos[1] = pos[0][1] + hl;

        // Bottom-left corner (again).
        vertex[6 * i + 5].pos[0] = pos[0][0] - hl;
        vertex[6 * i + 5].pos[1] = pos[0][1] - hl;

        // We copy the square color to each of the six vertices making the current square.
        // This is a choice made in this example, and it is up to the custom visual creator
        // to define how the user data, passed via props, will be used to fill in the vertices.
        for (uint32_t j = 0; j < 6; j++)
            memcpy(vertex[6 * i + j].color, color, sizeof(cvec4));
    }
}
```


## Putting everything together

Here is the code to create the custom visual.

```c
    // We create a blank visual in the scene.
    // For demo purposes, we disable the automatic position normalization.
    DvzVisual* visual = dvz_scene_visual_blank(scene, DVZ_VISUAL_FLAGS_TRANSFORM_NONE);

    // We add the existing graphics triangle graphics pipeline.
    dvz_visual_graphics(visual, dvz_graphics_builtin(canvas, DVZ_GRAPHICS_TRIANGLE, 0));

    // We add the vertex buffer source, and we must specify the same vertex struct type
    // as the one used by the graphics pipeline (standard vertex structure, with pos and color).
    dvz_visual_source(
        visual, DVZ_SOURCE_TYPE_VERTEX, 0, DVZ_PIPELINE_GRAPHICS, 0, 0, sizeof(DvzVertex), 0);

    // We specify the visual props.
    dvz_visual_prop(visual, DVZ_PROP_POS, 0, DVZ_DTYPE_DVEC3, DVZ_SOURCE_TYPE_VERTEX, 0);
    dvz_visual_prop(visual, DVZ_PROP_COLOR, 0, DVZ_DTYPE_CVEC4, DVZ_SOURCE_TYPE_VERTEX, 0);
    dvz_visual_prop(visual, DVZ_PROP_LENGTH, 0, DVZ_DTYPE_FLOAT, DVZ_SOURCE_TYPE_VERTEX, 0);

    // We declare our custom baking function.
    dvz_visual_callback_bake(visual, _bake_callback);

    // Finally, once the custom visual has been created, we can add it to the panel.
    dvz_scene_visual_custom(panel, visual);
```

Once the custom visual has been created and added to the scene, the last step consists of setting some data, as usual:

```c
// We define three squares.
dvz_visual_data(
    visual, DVZ_PROP_POS, 0, 3, (dvec3[]){{-.5, 0, 0}, {0, 0, 0}, {+.5, 0, 0}});

// We use a different color for each square.
dvz_visual_data(
    visual, DVZ_PROP_COLOR, 0, 3,
    (cvec4[]){{255, 0, 0, 255}, {0, 255, 0, 255}, {0, 0, 255, 255}});

// NOTE: we use the same length for all squares.
dvz_visual_data(visual, DVZ_PROP_LENGTH, 0, 1, (float[]){.25});
```

Note that we used a single value for the last prop (edge length). Datoviz uses the convention that a prop may have less values than objects, in which case **the last value is repeated over**. In particular, defining a prop with a single element means using the same value for all items in the visual. This is a sort of "broadcasting" rule (following NumPy's terminology).
