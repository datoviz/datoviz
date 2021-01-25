# C API documentation

## Scene API

### `vkl_scene()`

=== "C"
    ```c
    VklScene* vkl_scene(VklCanvas* canvas, uint32_t n_rows, uint32_t n_cols);
    ```

Create a scene with a grid layout.

The scene defines a 2D grid where each cell contains a panel (subplot). Panels may support
various kinds of interactivity.

| Parameter | Type | Description |
| ---- | ---- | ---- |
| :octicons-arrow-right-16: `canvas` | `VklCanvas*` | the canvas |
| :octicons-arrow-right-16: `n_rows` | `uint32_t` | number of rows in the grid |
| :octicons-arrow-right-16: `n_cols` | `uint32_t` | number of columns in the grid |
| :octicons-arrow-left-16: `returns` | `VklScene*` | pointer to the created scene |

### `vkl_scene_destroy()`

=== "C"
    ```c
    void vkl_scene_destroy(VklScene* scene);
    ```

Destroy a scene.

Destroy all panels and visuals in the scene.

| Parameter | Type | Description |
| ---- | ---- | ---- |
| :octicons-arrow-right-16: `scene` | `VklScene*` | the scene |

## App API

### `vkl_app()`

=== "C"
    ```c
    VklApp* vkl_app(VklBackend backend);
    ```

Create an application instance.

There is typically only one App object in a given application. This object holds a pointer to
the Vulkan instance and is responsible for discovering the available GPUs.

| Parameter | Type | Description |
| ---- | ---- | ---- |
| :octicons-arrow-right-16: `backend` | `VklBackend` | the backend |
| :octicons-arrow-left-16: `returns` | `VklApp*` | pointer to the created app |

### `vkl_app_destroy()`

=== "C"
    ```c
    int vkl_app_destroy(VklApp* app);
    ```

Destroy the application.

This function automatically destroys all objects created within the application.

| Parameter | Type | Description |
| ---- | ---- | ---- |
| :octicons-arrow-right-16: `app` | `VklApp*` | the application to destroy |
