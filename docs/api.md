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
## Array API

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
