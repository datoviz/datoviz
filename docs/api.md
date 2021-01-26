# C API documentation

## Scene API

### `vkl_scene()`

=== "C"
    ```c
    VklScene* vkl_scene(VklCanvas* canvas, uint32_t n_rows, uint32_t n_cols);
    ```

| Parameter | Type | Description |
| ---- | ---- | ---- |
| :octicons-arrow-right-16: `canvas` | `VklCanvas*` | the canvas |
| :octicons-arrow-right-16: `n_rows` | `uint32_t` | number of rows in the grid |
| :octicons-arrow-right-16: `n_cols` | `uint32_t` | number of columns in the grid |
| :octicons-arrow-left-16: `returns` | `VklScene*` | pointer to the created scene |

Create a scene with a grid layout.

The scene defines a 2D grid where each cell contains a panel (subplot). Panels may support
various kinds of interactivity.

### `vkl_scene_destroy()`

=== "C"
    ```c
    void vkl_scene_destroy(VklScene* scene);
    ```

| Parameter | Type | Description |
| ---- | ---- | ---- |
| :octicons-arrow-right-16: `scene` | `VklScene*` | the scene |

Destroy a scene.

Destroy all panels and visuals in the scene.

## App API

### `vkl_app()`

=== "C"
    ```c
    VklApp* vkl_app(VklBackend backend);
    ```

| Parameter | Type | Description |
| ---- | ---- | ---- |
| :octicons-arrow-right-16: `backend` | `VklBackend` | the backend |
| :octicons-arrow-left-16: `returns` | `VklApp*` | pointer to the created app |

Create an application instance.

There is typically only one App object in a given application. This object holds a pointer to
the Vulkan instance and is responsible for discovering the available GPUs.

### `vkl_app_destroy()`

=== "C"
    ```c
    int vkl_app_destroy(VklApp* app);
    ```

| Parameter | Type | Description |
| ---- | ---- | ---- |
| :octicons-arrow-right-16: `app` | `VklApp*` | the application to destroy |

Destroy the application.

This function automatically destroys all objects created within the application.

## Canvas API

### `vkl_canvas()`

=== "C"
    ```c
    VklCanvas* vkl_canvas(VklGpu* gpu, uint32_t width, uint32_t height, int flags);
    ```

| Parameter | Type | Description |
| ---- | ---- | ---- |
| :octicons-arrow-right-16: `gpu` | `VklGpu*` | the GPU to use for swapchain presentation |
| :octicons-arrow-right-16: `width` | `uint32_t` | the initial window width, in pixels |
| :octicons-arrow-right-16: `height` | `uint32_t` | the initial window height, in pixels |
| :octicons-arrow-right-16: `flags` | `int` | the creation flags for the canvas |

Create a canvas.

### `vkl_canvas_offscreen()`

=== "C"
    ```c
    VklCanvas* vkl_canvas_offscreen(
        VklGpu* gpu, uint32_t width, uint32_t height, int flags);
    ```

| Parameter | Type | Description |
| ---- | ---- | ---- |
| :octicons-arrow-right-16: `gpu` | `VklGpu*` | the GPU to use for swapchain presentation |
| :octicons-arrow-right-16: `width` | `uint32_t` | the canvas width, in pixels |
| :octicons-arrow-right-16: `height` | `uint32_t` | the canvas height, in pixels |
| :octicons-arrow-right-16: `flags` | `int` | the creation flags for the canvas |

Create an offscreen canvas.

### `vkl_canvas_recreate()`

=== "C"
    ```c
    void vkl_canvas_recreate(VklCanvas* canvas);
    ```

| Parameter | Type | Description |
| ---- | ---- | ---- |
| :octicons-arrow-right-16: `canvas` | `VklCanvas*` | the canvas to recreate |

Recreate the canvas GPU resources and swapchain.

### `vkl_canvas_commands()`

=== "C"
    ```c
    VklCommands* vkl_canvas_commands(
        VklCanvas* canvas, uint32_t queue_idx, uint32_t count);
    ```

| Parameter | Type | Description |
| ---- | ---- | ---- |
| :octicons-arrow-right-16: `canvas` | `VklCanvas*` | the canvas |
| :octicons-arrow-right-16: `queue_idx` | `uint32_t` | the index of the GPU queue within the GPU context |
| :octicons-arrow-right-16: `count` | `uint32_t` | number of command buffers to create |
| :octicons-arrow-left-16: `returns` | `VklCommands*` | set of created command buffers |

Create a set of Vulkan command buffers on a given GPU queue.

## Array API

### `vkl_array()`

=== "C"
    ```c
    VklArray vkl_array(uint32_t item_count, VklDataType dtype);
    ```

| Parameter | Type | Description |
| ---- | ---- | ---- |
| :octicons-arrow-right-16: `item_count` | `uint32_t` | initial number of elements |
| :octicons-arrow-right-16: `dtype` | `VklDataType` | the data type of the array |
| :octicons-arrow-left-16: `returns` | `VklArray` | new array |

Create a new 1D array.

### `vkl_array_point()`

=== "C"
    ```c
    VklArray vkl_array_point(dvec3 pos);
    ```

| Parameter | Type | Description |
| ---- | ---- | ---- |
| :octicons-arrow-right-16: `pos` | `dvec3` | initial number of elements |
| :octicons-arrow-left-16: `returns` | `VklArray` | new array |

Create an array with a single dvec3 position.

### `vkl_array_wrap()`

=== "C"
    ```c
    VklArray vkl_array_wrap(uint32_t item_count, VklDataType dtype, void* data);
    ```

| Parameter | Type | Description |
| ---- | ---- | ---- |
| :octicons-arrow-right-16: `item_count` | `uint32_t` | number of elements in the passed buffer |
| :octicons-arrow-right-16: `dtype` | `VklDataType` | the data type of the array |
| :octicons-arrow-left-16: `returns` | `VklArray` | array wrapping the buffer |

Create a 1D array from an existing compatible memory buffer.

The created array does not allocate memory, it uses the passed buffer instead.

!!! warning
    Destroying the array will free the passed pointer!

### `vkl_array_struct()`

=== "C"
    ```c
    VklArray vkl_array_struct(uint32_t item_count, VkDeviceSize item_size);
    ```

| Parameter | Type | Description |
| ---- | ---- | ---- |
| :octicons-arrow-right-16: `item_count` | `uint32_t` | number of elements |
| :octicons-arrow-right-16: `item_size` | `VkDeviceSize` | size, in bytes, of each item |
| :octicons-arrow-left-16: `returns` | `VklArray` | array |

Create a 1D record array with heterogeneous data type.

### `vkl_array_3D()`

=== "C"
    ```c
    VklArray vkl_array_3D(
        uint32_t ndims, uint32_t width, uint32_t height,
        uint32_t depth, VkDeviceSize item_size);
    ```

| Parameter | Type | Description |
| ---- | ---- | ---- |
| :octicons-arrow-right-16: `ndims` | `uint32_t` | number of dimensions (1, 2, 3) |
| :octicons-arrow-right-16: `width` | `uint32_t` | number of elements along the 1st dimension |
| :octicons-arrow-right-16: `height` | `uint32_t` | number of elements along the 2nd dimension |
| :octicons-arrow-right-16: `depth` | `uint32_t` | number of elements along the 3rd dimension |
| :octicons-arrow-right-16: `item_size` | `VkDeviceSize` | size of each item in bytes |
| :octicons-arrow-left-16: `returns` | `VklArray` | array |

Create a 3D array holding a texture.

### `vkl_array_resize()`

=== "C"
    ```c
    void vkl_array_resize(VklArray* array, uint32_t item_count);
    ```

| Parameter | Type | Description |
| ---- | ---- | ---- |
| :octicons-arrow-right-16: `array` | `VklArray*` | the array to resize |
| :octicons-arrow-right-16: `item_count` | `uint32_t` | the new number of items |

Resize an existing array.

* If the new size is equal to the old size, do nothing.
* If the new size is smaller than the old size, change the size attribute but do not reallocate
* If the new size is larger than the old size, reallocate memory and copy over the old values

### `vkl_array_clear()`

=== "C"
    ```c
    void vkl_array_clear(VklArray* array);
    ```

| Parameter | Type | Description |
| ---- | ---- | ---- |
| :octicons-arrow-right-16: `array` | `VklArray*` | the array to clear |

Reset to 0 the contents of an existing array.

### `vkl_array_reshape()`

=== "C"
    ```c
    void vkl_array_reshape(
        VklArray* array, uint32_t width, uint32_t height, uint32_t depth);
    ```

| Parameter | Type | Description |
| ---- | ---- | ---- |
| :octicons-arrow-right-16: `array` | `VklArray*` | the array to reshape and clear |
| :octicons-arrow-right-16: `width` | `uint32_t` | number of elements along the 1st dimension |
| :octicons-arrow-right-16: `height` | `uint32_t` | number of elements along the 2nd dimension |
| :octicons-arrow-right-16: `depth` | `uint32_t` | number of elements along the 3rd dimension |

Reshape a 3D array and *delete all the data in it*.

!!! warning
    The contents of the array will be cleared. Copying the existing data would require more work
    and is not necessary at the moment.

### `vkl_array_data()`

=== "C"
    ```c
    void vkl_array_data(
        VklArray* array, uint32_t first_item, uint32_t item_count,
        uint32_t data_item_count, void* data);
    ```

| Parameter | Type | Description |
| ---- | ---- | ---- |
| :octicons-arrow-right-16: `array` | `VklArray*` | the array |
| :octicons-arrow-right-16: `first_item` | `uint32_t` | first element in the array to be overwritten |
| :octicons-arrow-right-16: `item_count` | `uint32_t` | number of items to write |
| :octicons-arrow-right-16: `data_item_count` | `uint32_t` | number of elements in `data` |
| :octicons-arrow-right-16: `data` | `void*` | the buffer containing the data to copy |

Copy data into an array.

* There will be `item_count` values copied between `first_item` and `first_item + item_count` in
  the array.
* There are `data_item_count` values in the passed buffer.
* If `item_count > data_item_count`, the last value of `data` will be repeated until the last
value.

Example:

=== "C"
    ```c
    // Create an array of 10 double numbers, initialize all elements with 1.23.
    VklArray arr = vkl_array(10, VKL_DTYPE_DOUBLE);
    double item = 1.23;
    vkl_array_data(&arr, 0, 10, 1, &item);
    ```

### `vkl_array_item()`

=== "C"
    ```c
    void* vkl_array_item(VklArray* array, uint32_t idx);
    ```

| Parameter | Type | Description |
| ---- | ---- | ---- |
| :octicons-arrow-right-16: `array` | `VklArray*` | the array |
| :octicons-arrow-right-16: `idx` | `uint32_t` | the index of the element to retrieve |
| :octicons-arrow-left-16: `returns` | `void*` | pointer to the requested element |

Retrieve a single element from an array.

### `vkl_array_column()`

=== "C"
    ```c
    void vkl_array_column(
        VklArray* array, VkDeviceSize offset, VkDeviceSize col_size,
        uint32_t first_item, uint32_t item_count, uint32_t data_item_count,
        void* data, VklDataType source_dtype, VklDataType target_dtype,
        VklArrayCopyType copy_type, uint32_t reps);
    ```

| Parameter | Type | Description |
| ---- | ---- | ---- |
| :octicons-arrow-right-16: `array` | `VklArray*` | the array |
| :octicons-arrow-right-16: `offset` | `VkDeviceSize` | the offset within the array, in bytes |
| :octicons-arrow-right-16: `col_size` | `VkDeviceSize` | stride in the source array, in bytes |
| :octicons-arrow-right-16: `first_item` | `uint32_t` | first element in the array to be overwritten |
| :octicons-arrow-right-16: `item_count` | `uint32_t` | number of elements to write |
| :octicons-arrow-right-16: `data_item_count` | `uint32_t` | number of elements in `data` |
| :octicons-arrow-right-16: `data` | `void*` | the buffer containing the data to copy |
| :octicons-arrow-right-16: `source_dtype` | `VklDataType` | the source dtype (only used when casting) |
| :octicons-arrow-right-16: `target_dtype` | `VklDataType` | the target dtype (only used when casting) |
| :octicons-arrow-right-16: `copy_type` | `VklArrayCopyType` | the type of copy |
| :octicons-arrow-right-16: `reps` | `uint32_t` | the number of repeats for each copied element |

Copy data into the column of a record array.

This function is used by the default visual baking function, which copies to the vertex buffer
(corresponding to a record array with as many fields as GLSL attributes in the vertex shader)
the user-specified visual props (data for the individual elements).

### `vkl_array_destroy()`

=== "C"
    ```c
    void vkl_array_destroy(VklArray* array);
    ```

| Parameter | Type | Description |
| ---- | ---- | ---- |
| :octicons-arrow-right-16: `array` | `VklArray*` | the array to destroy |

Destroy an array.

This function frees the allocated underlying data buffer.
